#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <cmath>
#include <portaudio.h>
#include <fftw3.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512

class AudioCapture {
public:
    AudioCapture() {
        Pa_Initialize();
        Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, SAMPLE_RATE, FRAMES_PER_BUFFER, audioCallback, this);
        Pa_StartStream(stream);
    }

    ~AudioCapture() {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
    }

    std::vector<float> getBuffer() {
        std::lock_guard<std::mutex> lock(bufferMutex);
        return buffer;
    }

private:
    PaStream *stream;
    std::vector<float> buffer;
    std::mutex bufferMutex;

    static int audioCallback(const void *input, void *output, unsigned long frameCount,
                             const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
        auto *capture = static_cast<AudioCapture*>(userData);
        std::lock_guard<std::mutex> lock(capture->bufferMutex);
        const float *in = static_cast<const float*>(input);
        capture->buffer.assign(in, in + frameCount);
        return paContinue;
    }
};

std::vector<double> applyFFT(const std::vector<float>& input) {
    int N = input.size();
    fftw_complex *in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_complex *out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
    fftw_plan plan = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    for (int i = 0; i < N; ++i) {
        in[i][0] = input[i];
        in[i][1] = 0.0;
    }

    fftw_execute(plan);

    std::vector<double> magnitude(N);
    for (int i = 0; i < N; ++i) {
        magnitude[i] = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
    }

    fftw_destroy_plan(plan);
    fftw_free(in);
    fftw_free(out);

    return magnitude;
}

void visualize(const std::vector<double>& data) {
    const int width = 80;
    const int height = 20;
    const char* shades = " .:-=+*#%@";

    std::vector<int> scaled(data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        scaled[i] = std::min(height - 1, static_cast<int>(data[i] / 10));
    }

    for (int y = height - 1; y >= 0; --y) {
        for (size_t x = 0; x < scaled.size(); ++x) {
            if (scaled[x] >= y) {
                std::cout << shades[scaled[x] * (sizeof(shades) - 1) / height];
            } else {
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }
}

int main() {
    AudioCapture capture;
    while (true) {
        auto buffer = capture.getBuffer();
        if (!buffer.empty()) {
            auto spectrum = applyFFT(buffer);
            visualize(spectrum);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}


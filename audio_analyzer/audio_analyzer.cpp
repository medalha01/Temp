#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <fstream>
#include <portaudio.h>
#include <fftw3.h>
#include <thread>
#include <atomic>

// Constants for audio recording
const int SAMPLE_RATE = 44100; // Common audio sample rate
const int FRAMES_PER_BUFFER = 1024; // Number of samples per buffer

std::atomic<bool> stopRecording(false);

// Function to handle PortAudio errors
void checkPaError(PaError err) {
  if (err != paNoError) {
    std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    exit(1);
  }
}

// Function to capture audio from the microphone
std::vector<double> recordAudio() {
  PaStream *stream;
  PaError err;
  std::vector<double> audioData;

  // Initialize PortAudio
  err = Pa_Initialize();
  checkPaError(err);

  // Open an input stream
  err = Pa_OpenDefaultStream(&stream, 1, 0, paFloat32, SAMPLE_RATE,
                            FRAMES_PER_BUFFER, NULL, NULL);
  checkPaError(err);

  // Start the stream
  err = Pa_StartStream(stream);
  checkPaError(err);

  std::cout << "Press 'q' to start recording..." << std::endl;
  char input;
  while (std::cin >> input && input != 'q') {

  }

  std::cout << "Recording... Press 'q' to stop." << std::endl;

  std::thread inputThread([]() {
    char c;
    while (std::cin >> c) {
      if (c == 'q') {
        stopRecording.store(true);
        break;
      }
    }
  });

  while (!stopRecording.load()) {
    float buffer[FRAMES_PER_BUFFER];
    err = Pa_ReadStream(stream, buffer, FRAMES_PER_BUFFER);
    checkPaError(err);

    // Store the audio data
    for (int i = 0; i < FRAMES_PER_BUFFER; ++i) {
      audioData.push_back(buffer[i]);
    }
  }

  inputThread.join();

  // Stop the stream
  err = Pa_StopStream(stream);
  checkPaError(err);

  // Close the stream
  err = Pa_CloseStream(stream);
  checkPaError(err);

  // Terminate PortAudio
  err = Pa_Terminate();
  checkPaError(err);

  return audioData;
}

// Function to perform FFT using FFTW
std::vector<double> performFFT(const std::vector<double> &audioData) {
  int n = audioData.size();
  if (n == 0) {
    std::cerr << "No audio data to process." << std::endl;
    exit(1);
  }


  std::vector<std::complex<double>> complexData(n);
  std::vector<double> magnitudes(n / 2);

  // Convert real audio data to complex data
  for (int i = 0; i < n; ++i) {
    complexData[i] = std::complex<double>(audioData[i], 0.0);
  }

  // Create FFTW plan
  fftw_complex *in = reinterpret_cast<fftw_complex *>(complexData.data());
  fftw_complex *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * n);
  if (!out) {
    std::cerr << "Failed to allocate memory for FFTW output." << std::endl;
    exit(1);
  }

  fftw_plan plan = fftw_plan_dft_1d(n, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

  // Execute FFT
  fftw_execute(plan);

  // Calculate magnitudes
  for (int i = 0; i < n / 2; ++i) {
    magnitudes[i] = std::abs(std::complex<double>(out[i][0], out[i][1]));
  }

  // Clean up
  fftw_destroy_plan(plan);
  fftw_free(out);

  return magnitudes;
}

// Function to display the frequency spectrum
void displaySpectrum(const std::vector<double> &magnitudes) {
  for (int i = 0; i < magnitudes.size(); ++i) {
    std::cout << "Frequency bin " << i << ": " << magnitudes[i] << std::endl;
  }
}

// Function to save the frequency spectrum to a file
void saveSpectrum(const std::vector<double> &magnitudes,
                  const std::string &filename) {
  std::ofstream file(filename);
  if (file.is_open()) {
    for (int i = 0; i < magnitudes.size(); ++i) {
      file << magnitudes[i] << std::endl;
    }
    file.close();
    std::cout << "Frequency spectrum saved to " << filename << std::endl;
  } else {
    std::cerr << "Unable to open file: " << filename << std::endl;
  }
}

int main() {
  // Record audio
  std::vector<double> audioData = recordAudio();

  // Perform FFT
  std::vector<double> magnitudes = performFFT(audioData);

  // Display frequency spectrum
  displaySpectrum(magnitudes);

  // Save frequency spectrum to file
  saveSpectrum(magnitudes, "data");

  return 0;
}

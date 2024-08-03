#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <chrono>
#include <openssl/aes.h>
#include <openssl/rand.h>

class FileEncryptor {
public:
    static bool encryptFile(const std::string& filePath, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv);
    static bool decryptFile(const std::string& filePath, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv);
    static void logOperation(const std::string& operation, const std::string& filePath, const std::chrono::steady_clock::time_point& start, bool success);

private:
    static void handleErrors();
    static std::vector<unsigned char> readFile(const std::string& filePath);
    static bool writeFile(const std::string& filePath, const std::vector<unsigned char>& data);
};

void FileEncryptor::handleErrors() {
    std::cerr << "An error occurred" << std::endl;
    exit(1);
}

std::vector<unsigned char> FileEncryptor::readFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        handleErrors();
    }
    return std::vector<unsigned char>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

bool FileEncryptor::writeFile(const std::string& filePath, const std::vector<unsigned char>& data) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        handleErrors();
    }
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

void FileEncryptor::logOperation(const std::string& operation, const std::string& filePath, const std::chrono::steady_clock::time_point& start, bool success) {
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::ofstream logFile("operation.log", std::ios::app);
    logFile << "Operation: " << operation << ", File: " << filePath << ", Status: " << (success ? "Success" : "Failure")
            << ", Time: " << elapsed.count() << " seconds" << std::endl;
}

bool FileEncryptor::encryptFile(const std::string& filePath, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv) {
    auto start = std::chrono::steady_clock::now();
    std::vector<unsigned char> fileData = readFile(filePath);

    AES_KEY encryptKey;
    if (AES_set_encrypt_key(key.data(), 256, &encryptKey) < 0) {
        handleErrors();
    }

    std::vector<unsigned char> encOut(fileData.size() + AES_BLOCK_SIZE);
    int numBlocks = fileData.size() / AES_BLOCK_SIZE + 1;
    for (int i = 0; i < numBlocks; ++i) {
        AES_cbc_encrypt(fileData.data() + i * AES_BLOCK_SIZE, encOut.data() + i * AES_BLOCK_SIZE, AES_BLOCK_SIZE, &encryptKey, const_cast<unsigned char*>(iv.data()), AES_ENCRYPT);
    }

    bool success = writeFile(filePath + ".enc", encOut);
    logOperation("Encrypt", filePath, start, success);
    return success;
}

bool FileEncryptor::decryptFile(const std::string& filePath, const std::vector<unsigned char>& key, const std::vector<unsigned char>& iv) {
    auto start = std::chrono::steady_clock::now();
    std::vector<unsigned char> fileData = readFile(filePath);

    AES_KEY decryptKey;
    if (AES_set_decrypt_key(key.data(), 256, &decryptKey) < 0) {
        handleErrors();
    }

    std::vector<unsigned char> decOut(fileData.size());
    int numBlocks = fileData.size() / AES_BLOCK_SIZE;
    for (int i = 0; i < numBlocks; ++i) {
        AES_cbc_encrypt(fileData.data() + i * AES_BLOCK_SIZE, decOut.data() + i * AES_BLOCK_SIZE, AES_BLOCK_SIZE, &decryptKey, const_cast<unsigned char*>(iv.data()), AES_DECRYPT);
    }

    bool success = writeFile(filePath + ".dec", decOut);
    logOperation("Decrypt", filePath, start, success);
    return success;
}

std::vector<unsigned char> hexStringToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <encrypt|decrypt> <file_path> <key> <iv>" << std::endl;
        return 1;
    }

    std::string operation = argv[1];
    std::string filePath = argv[2];
    std::string keyHex = argv[3];
    std::string ivHex = argv[4];

    if (keyHex.size() != 64 || ivHex.size() != 32) {
        std::cerr << "Key must be 64 hex characters (32 bytes) and IV must be 32 hex characters (16 bytes)" << std::endl;
        return 1;
    }

    std::vector<unsigned char> key = hexStringToBytes(keyHex);
    std::vector<unsigned char> iv = hexStringToBytes(ivHex);

    bool success;
    if (operation == "encrypt") {
        success = FileEncryptor::encryptFile(filePath, key, iv);
    } else if (operation == "decrypt") {
        success = FileEncryptor::decryptFile(filePath, key, iv);
    } else {
        std::cerr << "Unknown operation: " << operation << std::endl;
        return 1;
    }

    return success ? 0 : 1;
}


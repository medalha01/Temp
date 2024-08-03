#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <ctime>
#include <iomanip>
#include <sstream>

// Function to convert hexadecimal string to binary data
std::vector<unsigned char> hexToBin(const std::string &hex) {
    std::vector<unsigned char> bin(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        bin[i / 2] = std::stoi(hex.substr(i, 2), nullptr, 16);
    }
    return bin;
}

// Function to log operations
void logOperation(const std::string &operation, const std::string &filename, const std::string &status) {
    std::time_t now = std::time(nullptr);
    std::tm *local_time = std::localtime(&now);

    std::cout << "[" << std::put_time(local_time, "%Y-%m-%d %H:%M:%S") << "] "
              << operation << " " << filename << " " << status << std::endl;
}

// Function to encrypt a file
bool encryptFile(const std::string &filename, const std::vector<unsigned char> &key, const std::vector<unsigned char> &iv) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open()) {
        logOperation("Encrypt", filename, "File not found");
        return false;
    }

    std::vector<unsigned char> inBuffer((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    size_t paddedSize = inBuffer.size() + AES_BLOCK_SIZE - (inBuffer.size() % AES_BLOCK_SIZE);
    std::vector<unsigned char> outBuffer(paddedSize);

    AES_KEY encryptKey;
    if (AES_set_encrypt_key(key.data(), 256, &encryptKey) < 0) {
        logOperation("Encrypt", filename, "Invalid key");
        return false;
    }

    AES_cbc_encrypt(inBuffer.data(), outBuffer.data(), inBuffer.size(), &encryptKey, const_cast<unsigned char*>(iv.data()), AES_ENCRYPT);

    std::ofstream outFile(filename + ".enc", std::ios::binary);
    if (!outFile.is_open()) {
        logOperation("Encrypt", filename, "Error writing file");
        return false;
    }
    outFile.write(reinterpret_cast<const char*>(outBuffer.data()), outBuffer.size());
    outFile.close();

    logOperation("Encrypt", filename, "Success");
    return true;
}

// Function to decrypt a file
bool decryptFile(const std::string &filename, const std::vector<unsigned char> &key, const std::vector<unsigned char> &iv) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile.is_open()) {
        logOperation("Decrypt", filename, "File not found");
        return false;
    }

    std::vector<unsigned char> inBuffer((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    std::vector<unsigned char> outBuffer(inBuffer.size());

    AES_KEY decryptKey;
    if (AES_set_decrypt_key(key.data(), 256, &decryptKey) < 0) {
        logOperation("Decrypt", filename, "Invalid key");
        return false;
    }

    AES_cbc_encrypt(inBuffer.data(), outBuffer.data(), inBuffer.size(), &decryptKey, const_cast<unsigned char*>(iv.data()), AES_DECRYPT);

    std::ofstream outFile(filename.substr(0, filename.size() - 4) + ".dec", std::ios::binary);
    if (!outFile.is_open()) {
        logOperation("Decrypt", filename, "Error writing file");
        return false;
    }
    outFile.write(reinterpret_cast<const char*>(outBuffer.data()), outBuffer.size());
    outFile.close();

    logOperation("Decrypt", filename, "Success");
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <encrypt|decrypt> <filename> <32-byte key in hex> <16-byte IV in hex>" << std::endl;
        return 1;
    }

    std::string operation = argv[1];
    std::string filename = argv[2];
    std::string hexKey = argv[3];
    std::string hexIv = argv[4];

    if (hexKey.size() != 64 || hexIv.size() != 32) {
        std::cerr << "Error: Key must be 32 bytes in hex and IV must be 16 bytes in hex" << std::endl;
        return 1;
    }

    std::vector<unsigned char> key = hexToBin(hexKey);
    std::vector<unsigned char> iv = hexToBin(hexIv);

    if (operation == "encrypt") {
        if (!encryptFile(filename, key, iv)) {
            return 1;
        }
    } else if (operation == "decrypt") {
        if (!decryptFile(filename, key, iv)) {
            return 1;
        }
    } else {
        std::cerr << "Error: Invalid operation. Use 'encrypt' or 'decrypt'." << std::endl;
        return 1;
    }

    return 0;
}


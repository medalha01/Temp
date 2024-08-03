#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <iomanip>
#include <chrono>
#include <ctime>

using namespace std;

// Function to convert a hexadecimal string to a binary byte array
vector<unsigned char> hexStringToBytes(const string &hex)
{
    vector<unsigned char> bytes;
    for (unsigned int i = 0; i < hex.length(); i += 2)
    {
        string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

// Function to encrypt a file
bool encryptFile(const string &inputFilename, const string &outputFilename, const unsigned char *key, const unsigned char *iv)
{
    // Open input file
    ifstream inputFile(inputFilename, ios::binary);
    if (!inputFile.is_open())
    {
        cerr << "Error: Could not open input file: " << inputFilename << endl;
        return false;
    }

    // Open output file
    ofstream outputFile(outputFilename, ios::binary);
    if (!outputFile.is_open())
    {
        cerr << "Error: Could not open output file: " << outputFilename << endl;
        inputFile.close();
        return false;
    }

    // Get input file size
    inputFile.seekg(0, ios::end);
    long fileSize = inputFile.tellg();
    inputFile.seekg(0, ios::beg);

    // Initialize encryption context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        cerr << "Error: Could not create encryption context" << endl;
        inputFile.close();
        outputFile.close();
        return false;
    }

    // Set encryption parameters
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
    {
        cerr << "Error: Could not initialize encryption" << endl;
        EVP_CIPHER_CTX_free(ctx);
        inputFile.close();
        outputFile.close();
        return false;
    }

    // Encrypt data
    const int blockSize = 16;
    unsigned char inputBuffer[blockSize];
    unsigned char outputBuffer[blockSize + AES_BLOCK_SIZE];
    int outputLength;

    while (inputFile.read((char *)inputBuffer, blockSize))
    {
        if (EVP_EncryptUpdate(ctx, outputBuffer, &outputLength, inputBuffer, inputFile.gcount()) != 1)
        {
            cerr << "Error: Encryption failed" << endl;
            EVP_CIPHER_CTX_free(ctx);
            inputFile.close();
            outputFile.close();
            return false;
        }
        outputFile.write((char *)outputBuffer, outputLength);
    }

    // Finalize encryption
    if (EVP_EncryptFinal_ex(ctx, outputBuffer, &outputLength) != 1)
    {
        cerr << "Error: Encryption finalization failed" << endl;
        EVP_CIPHER_CTX_free(ctx);
        inputFile.close();
        outputFile.close();
        return false;
    }
    outputFile.write((char *)outputBuffer, outputLength);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);
    inputFile.close();
    outputFile.close();

    // Log operation
    auto now = chrono::system_clock::now();
    time_t timestamp = chrono::system_clock::to_time_t(now);
    cout << "Encryption - Timestamp: " << put_time(localtime(&timestamp), "%F %T") << " - File Size: " << fileSize << " bytes - Status: Success" << endl;

    return true;
}

// Function to decrypt a file
bool decryptFile(const string &inputFilename, const string &outputFilename, const unsigned char *key, const unsigned char *iv)
{
    // Open input file
    ifstream inputFile(inputFilename, ios::binary);
    if (!inputFile.is_open())
    {
        cerr << "Error: Could not open input file: " << inputFilename << endl;
        return false;
    }

    // Open output file
    ofstream outputFile(outputFilename, ios::binary);
    if (!outputFile.is_open())
    {
        cerr << "Error: Could not open output file: " << outputFilename << endl;
        inputFile.close();
        return false;
    }

    // Get input file size
    inputFile.seekg(0, ios::end);
    long fileSize = inputFile.tellg();
    inputFile.seekg(0, ios::beg);

    // Initialize decryption context
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        cerr << "Error: Could not create decryption context" << endl;
        inputFile.close();
        outputFile.close();
        return false;
    }

    // Set decryption parameters
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
    {
        cerr << "Error: Could not initialize decryption" << endl;
        EVP_CIPHER_CTX_free(ctx);
        inputFile.close();
        outputFile.close();
        return false;
    }

    // Decrypt data
    const int blockSize = 16;
    unsigned char inputBuffer[blockSize + AES_BLOCK_SIZE];
    unsigned char outputBuffer[blockSize + AES_BLOCK_SIZE];
    int outputLength;

    while (inputFile.read((char *)inputBuffer, blockSize))
    {
        if (EVP_DecryptUpdate(ctx, outputBuffer, &outputLength, inputBuffer, inputFile.gcount()) != 1)
        {
            cerr << "Error: Decryption failed" << endl;
            EVP_CIPHER_CTX_free(ctx);
            inputFile.close();
            outputFile.close();
            return false;
        }
        outputFile.write((char *)outputBuffer, outputLength);
    }

    // Finalize decryption
    if (EVP_DecryptFinal_ex(ctx, outputBuffer, &outputLength) != 1)
    {
        cerr << "Error: Decryption finalization failed" << endl;
        EVP_CIPHER_CTX_free(ctx);
        inputFile.close();
        outputFile.close();
        return false;
    }
    // Remove padding
    outputFile.write((char *)outputBuffer, outputLength);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);
    inputFile.close();
    outputFile.close();

    // Log operation
    auto now = chrono::system_clock::now();
    time_t timestamp = chrono::system_clock::to_time_t(now);
    cout << "Decryption - Timestamp: " << put_time(localtime(&timestamp), "%F %T") << " - File Size: " << fileSize << " bytes - Status: Success" << endl;

    return true;
}

int main()
{
    // Get key and IV from user
    string keyHex, ivHex;
    cout << "Enter 32-byte key in hexadecimal format: ";
    cin >> keyHex;
    cout << "Enter 16-byte IV in hexadecimal format: ";
    cin >> ivHex;

    // Validate key and IV length
    if (keyHex.length() != 64 || ivHex.length() != 32)
    {
        cerr << "Error: Invalid key or IV length" << endl;
        return 1;
    }

    // Convert key and IV to binary
    vector<unsigned char> key = hexStringToBytes(keyHex);
    vector<unsigned char> iv = hexStringToBytes(ivHex);

    // Get operation type from user
    char operation;
    cout << "Enter operation type (e for encryption, d for decryption): ";
    cin >> operation;

    // Get filename from user
    string filename;
    cout << "Enter filename: ";
    cin >> filename;

    // Perform encryption or decryption
    if (operation == 'e')
    {
        string outputFilename = filename + ".enc";
        if (encryptFile(filename, outputFilename, key.data(), iv.data()))
        {
            cout << "File encrypted successfully: " << outputFilename << endl;
        }
        else
        {
            cerr << "Error: File encryption failed" << endl;
            return 1;
        }
    }
    else if (operation == 'd')
    {
        string outputFilename = filename + ".dec";
        if (decryptFile(filename, outputFilename, key.data(), iv.data()))
        {
            cout << "File decrypted successfully: " << outputFilename << endl;
        }
        else
        {
            cerr << "Error: File decryption failed" << endl;
            return 1;
        }
    }
    else
    {
        cerr << "Error: Invalid operation type" << endl;
        return 1;
    }

    return 0;
}

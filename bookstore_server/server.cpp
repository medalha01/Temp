#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include "parser.h"

namespace fs = std::filesystem;

std::mutex requestMutex;
int currentUsers = 0;

std::string getMimeType(const std::string& filename) {
    // Simple MIME type mapping based on file extension
    if (filename.find(".html") != std::string::npos) return "text/html";
    if (filename.find(".css") != std::string::npos) return "text/css";
    if (filename.find(".js") != std::string::npos) return "application/javascript";
    if (filename.find(".png") != std::string::npos) return "image/png";
    if (filename.find(".jpg") != std::string::npos) return "image/jpeg";
    return "text/plain";
}

std::string timePointToString(const std::chrono::system_clock::time_point& tp) {
    std::time_t time = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void logRequest(const std::string& method, const std::string& resource, const std::string& status, const std::string& ip, long long duration) {
    std::lock_guard<std::mutex> lock(requestMutex);
    std::ofstream logFile("server_requests.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << "[" << timePointToString(std::chrono::system_clock::now()) << "] "
                << method << " " << resource << " " << status << " " << ip << " " << duration << "ms" << std::endl;
        logFile.close();
    } else {
        std::cerr << "Error: Unable to open log file." << std::endl;
    }
}

std::string handleRequest(const std::string& request, const std::string& webRoot, std::string& method, std::string& resource) {
    std::string protocol;
    std::istringstream requestStream(request);
    requestStream >> method >> resource >> protocol;

    if (resource == "/") {
        resource = "/index.html";
    }

    std::string filePath = webRoot + resource;

    if (fs::exists(filePath)) {
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            return "HTTP/1.1 200 OK\r\nContent-Type: " + getMimeType(filePath) + "\r\n\r\n" + content;
        } else {
            return "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        }
    } else {
        std::ifstream file(webRoot + "/unavailable.html", std::ios::binary);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            return "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n" + content;
        } else {
            return "HTTP/1.1 404 Not Found\r\n\r\n";
        }
    }
}

std::string listResources(const std::string& webRoot) {
    std::string json = "{ \"resources\": [";
    for (const auto& entry : fs::directory_iterator(webRoot)) {
        if (fs::is_regular_file(entry)) {
            json += "\"" + entry.path().filename().string() + "\",";
        }
    }
    if (json.back() == ',') {
        json.pop_back();
    }
    json += "] }";
    return json;
}

void handleClient(int clientSocket, const std::string& webRoot) {
    char buffer[1024];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead > 0) {
        std::string request(buffer, bytesRead);
        auto start = std::chrono::high_resolution_clock::now();

        std::string method, resource;
        std::string response;
        if (request.find("GET /resources") == 0) {
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + listResources(webRoot);
        } else if (request.find("HEAD ") == 0) {
            response = "HTTP/1.1 200 OK\r\n"; // Only headers for HEAD request
        } else {
            response = handleRequest(request, webRoot, method, resource);
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        send(clientSocket, response.c_str(), response.size(), 0);
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        getpeername(clientSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        std::string clientIP = inet_ntoa(clientAddr.sin_addr);
        logRequest(method, resource, response.substr(9, 3), clientIP, duration);
    }
    close(clientSocket);
    {
        std::lock_guard<std::mutex> lock(requestMutex);
        currentUsers--;
    }
}

int main() {
    std::map<std::string, std::string> config = parseConfig("server_config.cfg");
    int port = std::stoi(config["port"]);
    std::string webRoot = config["web_root"];
    int maxUsers = std::stoi(config["max_users"]);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding socket." << std::endl;
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Error listening on socket." << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server started on port " << port << "." << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == -1) {
            std::cerr << "Error accepting client connection." << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(requestMutex);
            if (currentUsers >= maxUsers) {
                std::string response = "HTTP/1.1 503 Service Unavailable\r\n\r\n";
                send(clientSocket, response.c_str(), response.size(), 0);
                close(clientSocket);
                continue;
            }
            currentUsers++;
        }

        std::thread clientThread(handleClient, clientSocket, webRoot);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}

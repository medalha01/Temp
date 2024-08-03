#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include "parser.h"

std::map<std::string, std::string> parseConfig(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                config[key] = value;
            }
        }
        file.close();
    } else {
        std::cerr << "Error: Unable to open config file: " << filename << std::endl;
    }

    return config;
}

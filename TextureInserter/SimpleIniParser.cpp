#include "SimpleIniParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>

SimpleIniParser::SimpleIniParser(const std::string& filename) {
    parseFile(filename);
}

void SimpleIniParser::parseFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    std::string line, currentSection;
    while (std::getline(file, line)) {
        line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end()); // Remove whitespaces
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue; // Skip comments and empty lines
        }
        if (line[0] == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos && !currentSection.empty()) {
            std::string key = line.substr(0, equalPos);
            int value = std::stoi(line.substr(equalPos + 1));
            data[currentSection][key] = value;
        }
    }
    file.close();
}

std::unordered_map<std::string, int> SimpleIniParser::getSection(const std::string& section) {
    return data[section];
}

#pragma once
#include <string>
#include <unordered_map>

class SimpleIniParser {
public:
    SimpleIniParser(const std::string& filename);
    std::unordered_map<std::string, int> getSection(const std::string& section);

private:
    std::unordered_map<std::string, std::unordered_map<std::string, int>> data;

    void parseFile(const std::string& filename);
};
#pragma once

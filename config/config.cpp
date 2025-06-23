#include "parserExceptions.hpp"
#include "../print.hpp"
#include "config.hpp"

#include <fstream>
#include <variant>

ConfigFile *ConfigurationParser::_loadConfigFile(const std::string &filePath) {
    std::fstream file(filePath);
    if (!file.is_open())
        throw ParserException("Failed to open configuration file: " + filePath);

    auto it = _configFiles.emplace(filePath, _arena.alloc<ConfigFile>(filePath, std::string(), std::vector<Token*>(), std::vector<size_t>()));
    if (!it.second)
        throw ParserException("Circulair import detected for: " + filePath);
    ConfigFile *configFile = it.first->second;

    std::string line;
    while (std::getline(file, line)) {
        configFile->lineStarts.push_back(configFile->fileContent.size());
        configFile->fileContent.insert(configFile->fileContent.end(), line.begin(), line.end());
        configFile->fileContent.push_back('\n');
    }

    configFile->fileContent.push_back('\0');
    file.close();

    _tokenize(configFile);
    _objects[filePath] = _getObjectFromFile(configFile);
    return (configFile);
}

bool ConfigurationParser::parseFile(const std::string &filePath) {
    if (isFileLoaded(filePath)) {
        ERROR("File already loaded: " + filePath);
        return (false);
    }

    try {
        _loadConfigFile(filePath);
    } catch (const ParserException &e) {
        std::cerr << e.getMessage();
        return (false);
    } catch (const std::exception &e) {
        ERROR("Exception while parsing file: " + filePath + "\n" + e.what());
        return (false);
    }

    return (true);
}

Object *ConfigurationParser::getResult(const std::string &filePath) {
    auto it = _objects.find(filePath);
    if (it != _objects.end())
        return it->second;
    return nullptr;
}

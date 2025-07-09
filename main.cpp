#include "config/rules/ruleTemplates/serverconfigRule.hpp"
#include "config/rules/objectParser.hpp"
#include "config/parserExceptions.hpp"
#include "config/config.hpp"
#include "print.hpp"

int main(int argc, char **argv) {
    if (argc > 2) {
        ERROR("Too many arguments provided.");
        return (1);
    }

    std::string filePath = "default.conf";
    if (argc == 2) filePath = argv[1];

    ConfigurationParser* parser = new ConfigurationParser();
    parser->parseFile(filePath);
    std::vector<ServerConfig> servers = parser->getResult(filePath);
    delete parser;

    if (servers.empty())
        exit(1);

    for (const auto &server : servers)
        std::cout << server << std::endl;

    return (0);
}

#include "config/config.hpp"
#include "ruleParser.hpp"
#include "print.hpp"

int main(int argc, char **argv) {
    PRINT("Starting the program...");

    if (argc > 2) {
        ERROR("Too many arguments provided.");
        return (1);
    }

    std::string filePath = "default.conf";
    if (argc == 2) filePath = argv[1];

    PRINT("Using configuration file: " + filePath);

    ConfigurationParser* parser = new ConfigurationParser();
    if (!parser->parseFile(filePath)) {
        delete parser;
        return (1);
    }

    try {
        Object *result = parser->getResult(filePath);

        Rule *serverRule = result->rules[Key::SERVER][0];
        Rule *locationRule = std::get<Object *>(serverRule->arguments[0]->value)->rules[Key::LOCATION][0];

        RouteRule routeRule(locationRule);
    } catch (const ParserException &e) {
        std::cerr << e.getMessage() << std::endl;
        delete parser;
        return (1);
    } catch (const std::exception &e) {
        ERROR("Exception while processing configuration: " + std::string(e.what()));
        delete parser;
        return (1);
    }

    delete parser;
    return (0);
}
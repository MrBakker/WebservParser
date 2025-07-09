#include "serverconfigRule.hpp"
#include "../objectParser.hpp"
#include "../ruleParser.hpp"
#include "../rules.hpp"

ServerConfig::ServerConfig(Rule *rule) {
    Object *object;

    RuleParser::create(rule, *this)
        .expectArgumentCount(1)
        .parseArgument(object);

    ObjectParser objectParser(object);
    objectParser.local().required()
        .parseFromOne(port)
        .parseFromOne(serverName)
        .optional() // The routes are optional -> if none can be found the default location will be used.
        .parseRange(_locations);

    _defaultLocation = LocationRule(object);
}

/// @brief Check if the server configuration rule is set (i.e., if it contains any locations).
bool ServerConfig::isSet() const {
    return (true);
}

/// @brief Get the list of location rules defined in the server configuration.
const std::vector<LocationRule>& ServerConfig::getLocations() const {
    return _locations;
}

/// @brief Get the default location rule.
const LocationRule& ServerConfig::getDefaultLocation() const {
    return _defaultLocation;
}

/// @brief Get the location rule for a specific URL.
/// @param url The URL for which the location rule is requested.
/// @return The location rule that matches the given URL, or the default location if no specific match is found.
const LocationRule& ServerConfig::getLocation(const std::string &url) const {
    const LocationRule *bestMatch = &_defaultLocation;
    size_t longestMatch = 0;

    for (const LocationRule &location : _locations) {
        if (!location.isSet())
            continue;

        if (location.path.str().length() > longestMatch && url.starts_with(location.path.str())) {
            longestMatch = location.path.str().length();
            bestMatch = &location;
        }
    }

    return (*bestMatch);
}

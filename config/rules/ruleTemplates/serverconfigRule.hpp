#pragma once

#include "../../types/customTypes.hpp"
#include "servernameRule.hpp"
#include "locationRule.hpp"
#include "../../config.hpp"
#include "../baserule.hpp"
#include "portRule.hpp"

#include <string>

class ServerConfig : public BaseRule {
private:
    std::vector<LocationRule> _locations;
    LocationRule _defaultLocation;


public:
    PortRule port;
    ServerNameRule serverName;

    constexpr static Key getKey() { return Key::SERVER; }
    constexpr static const std::string getRuleName() { return "server"; }
    constexpr static const std::string getRuleFormat() { return ServerConfig::getRuleName() + " { ... }"; }

    ServerConfig() = default;
    ServerConfig(Rule *rule);

    bool isSet() const;
    const std::vector<LocationRule>& getLocations() const;
    const LocationRule& getDefaultLocation() const;
    const LocationRule& getLocation(const std::string &url) const;
};
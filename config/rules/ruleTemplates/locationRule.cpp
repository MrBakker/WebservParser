#include "../../types/customTypes.hpp"
#include "../objectParser.hpp"
#include "../ruleParser.hpp"
#include "locationRule.hpp"
#include "../rules.hpp"

#include <ostream>

LocationRule::LocationRule(Object *object) {
    _isSet = true;
    path = Path::createDummy();

    _parseFromObject(object);
}

LocationRule::LocationRule(Rule *rule) {
    _isSet = true;
    Object *object;

    RuleParser::create(rule, *this)
        .expectArgumentCount(2)
        .parseArgument(path)
        .parseArgument(object);

    _parseFromObject(object);
}

/// @brief Parse the location rule from an Object instance
/// @param object The Object instance containing the rules to be parsed
void LocationRule::_parseFromObject(Object *object) {
    ObjectParser objectParser(object);

    objectParser.bound(Key::SERVER).optional()
        .parseFromRange(methods)
        .parseFromOne(root)
        .parseFromOne(uploadStore)
        .parseFromOne(autoIndex)
        .parseFromRange(index)
        .parseFromOne(returnRule)
        .parseFromRange(errorPages)
        .parseFromOne(maxBodySize)
        .parseFromOne(cgi)
        .parseFromOne(cgiTimeout)
        .parseFromRange(cgiExtention);
}

/// @brief Check if the location rule is set (i.e., if it has a non-empty path).
bool LocationRule::isSet() const {
    return _isSet;
}

std::ostream& operator<<(std::ostream &os, const LocationRule &rule) {
    os << "LocationRule: ";
    os << "(" << rule.path.str() << ")\n";
    os << rule.methods << "\n";
    os << rule.root << "\n";
    os << rule.uploadStore << "\n";
    os << rule.autoIndex << "\n";
    os << rule.index << "\n";
    os << rule.returnRule << "\n";
    os << rule.errorPages << "\n";
    os << rule.maxBodySize << "\n";
    os << rule.cgi << "\n";
    os << rule.cgiTimeout << "\n";
    os << rule.cgiExtention;
    return os;
}

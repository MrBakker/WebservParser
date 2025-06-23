#pragma once

#include "config/parserExceptions.hpp"
#include "config/config.hpp"
#include "print.hpp"

#include "argumentConverter.hpp"

#include <string>

class BaseRule {
public:
    constexpr static Key getKey() { return Key::ALLOWED_METHODS; };
    constexpr static const std::string getRuleName() { return "allowed_methods"; }
    constexpr static const std::string getRuleFormat() { return "allowed_methods <method1> [<method2> ...]"; }

    bool isSet() const;
};

enum RulesScope {
    LOCAL = 0,
    GLOBAL = 1,
};

enum ExpectedRuleCount {
    ONE = 1,
    MULTIPLE = 2,
};

class RuleParser {
private:
    Rule* _rule;
    std::string _ruleName;
    std::string _ruleFormat;

    size_t _argumentIndex = 0;

public:
    RuleParser(Rule* rule, const std::string &name, const std::string &format) 
        : _rule(rule), _ruleName(name), _ruleFormat(format) {}
    ~RuleParser() = default;

    static RuleParser create(Rule *rule, BaseRule &context) {
        return (RuleParser(rule, context.getRuleName(), context.getRuleFormat()));
    }

    RuleParser& expectMinNumArguments(size_t min) {
        if (_rule->arguments.size() < min)
            throw ParserRuleException("Invalid number of arguments; found " + std::to_string(_rule->arguments.size()) + 
                ", expected at least " + std::to_string(min), _rule,
                "Check the syntax of the rule. Expected format:\n\t" + _ruleFormat);
        return (*this);
    }

    RuleParser& expectArgumentCount(size_t min, size_t max) {
        if (_rule->arguments.size() < min || _rule->arguments.size() > max)
            throw ParserRuleException("Invalid number of arguments; found " + std::to_string(_rule->arguments.size()) + 
                ", expected between " + std::to_string(min) + " and " + std::to_string(max), _rule,
                "Check the syntax of the rule. Expected format:\n\t" + _ruleFormat);
        return (*this);
    }

    template <typename T>
    RuleParser& parseArgument(T &target) {
        if (_argumentIndex >= _rule->arguments.size())
            throw ParserRuleException("Not enough arguments provided for rule '" + _ruleName + "'", _rule,
                "Check the syntax of the rule. Expected format:\n\t" + _ruleFormat);

        target = std::move(ArgumentConverter<T, Argument*>::convert(_rule->arguments[_argumentIndex++]));
        return (*this);
    }

    template <typename T>
    RuleParser& parseAllButX(std::vector<T> &target, size_t argumentsToSave) {
        if (argumentsToSave >= _rule->arguments.size() - _argumentIndex)
            throw ParserRuleException("Not enough arguments provided for rule '" + _ruleName + "'", _rule,
                "Check the syntax of the rule. Expected format:\n\t" + _ruleFormat);
        size_t argumentsToParse = _rule->arguments.size() - _argumentIndex - argumentsToSave;

        for (size_t i = 0; i < argumentsToParse; ++i)
            target.emplace_back(ArgumentConverter<T, Argument*>::convert(_rule->arguments[_argumentIndex++]));
        return (*this);
    }

    template <typename T>
    RuleParser& parseAll(std::vector<T> &target) {
        return parseAllButX<T>(target, 0);
    }
};

class ObjectParser {
private:
    ExpectedRuleCount _expectedRuleCount = ExpectedRuleCount::ONE;
    RulesScope _scopeFallback = RulesScope::LOCAL;
    bool _optional = false;
    Object *_object;

    std::vector<Rule*> _fetchRules(Key key) {
        std::vector<Rule*> rules;
        Object *object = _object;
        int maxDepth = 10;

        while (object && maxDepth-- > 0) {
            DEBUG(object);
            auto it = object->rules.find(key);
            if (it != object->rules.end())
                rules.insert(rules.begin(), it->second.begin(), it->second.end());
                
            if ((_expectedRuleCount == ExpectedRuleCount::ONE && rules.size())
                || _scopeFallback == RulesScope::LOCAL)
                break ;

            if (object->parentRule)
                object = object->parentRule->parentObject;
        }

        if (rules.empty() && !_optional)
            throw ParserMissingException("Missing rule for key: " + std::to_string(static_cast<int>(key)), 
                "Check the configuration file for the required rule.");
        if (_expectedRuleCount == ExpectedRuleCount::ONE && rules.size() > 1)
            throw ParserDuplicateRuleException("Duplicated rule found", rules[0], rules[1],
                "Remove the duplicate rule outside of its scope to avoid conflicts.");
        
        return (rules);
    }

public:
    ObjectParser(Object *object)
        : _object(object) {}
    ~ObjectParser() = default;

    ObjectParser& local() {
        _scopeFallback = RulesScope::LOCAL;
        return (*this);
    }

    ObjectParser& global() {
        _scopeFallback = RulesScope::GLOBAL;
        return (*this);
    }

    ObjectParser& optional() {
        _optional = true;
        return (*this);
    }

    ObjectParser& required() {
        _optional = false;
        return (*this);
    }

    template <typename T>
    ObjectParser& parseOne(T& target) {
        _expectedRuleCount = ExpectedRuleCount::ONE;

        Key key = T::getKey();
        std::vector<Rule*> rules = _fetchRules(key);

        if (rules.empty()) target = std::move(T(nullptr));
        else target = std::move(T(rules[0]));

        return (*this);
    }

    template <typename T>
    ObjectParser& parseRange(T &target) {
        _expectedRuleCount = ExpectedRuleCount::MULTIPLE;

        Key key = T::getKey();
        std::vector<Rule*> rules = _fetchRules(key);
        target = std::move(T(rules));

        return (*this);
    }
};

class ErrorPageRule : public BaseRule {
private:
    std::map<ErrorCode, std::string> _errorPages;

public:
    constexpr static Key getKey() { return Key::ERROR_PAGE; }
    constexpr static const std::string getRuleName() { return "error_page"; }
    constexpr static const std::string getRuleFormat() { return ErrorPageRule::getRuleName() + " <status_code> <path>"; }

    ErrorPageRule() = default;
    ErrorPageRule(Rule *rule) : _errorPages() { (void)rule; }
    ErrorPageRule(std::vector<Rule*> &rules) {
        for (Rule *rule : rules) {
            std::vector<ErrorCode> errorCodes;
            std::string errorPagePath;

            RuleParser::create(rule, *this)
                .expectMinNumArguments(2)
                .parseAllButX(errorCodes, 1)
                .parseArgument(errorPagePath);

            for (ErrorCode code : errorCodes)
                _errorPages[code] = errorPagePath;
        }
    }

    inline bool isSet() const { return (!_errorPages.empty()); }
};

class RootRule : public BaseRule {
private:
    std::string _rootPath;

public:
    constexpr static Key getKey() { return Key::ROOT; }
    constexpr static const std::string getRuleName() { return "root"; }
    constexpr static const std::string getRuleFormat() { return RootRule::getRuleName() + " <path>"; }

    RootRule() = default;
    RootRule(Rule *rule) {
        RuleParser::create(rule, *this)
            .expectArgumentCount(1, 1)
            .parseArgument(_rootPath);
    }

    inline bool isSet() const { return (!_rootPath.empty()); }
};

class RouteRule : public BaseRule {
private:
    bool _isSet = false;

public:
    std::string path;
    ErrorPageRule errorPageRule;
    RootRule rootRule;

    constexpr static Key getKey() { return Key::LOCATION; }
    constexpr static const std::string getRuleName() { return "location"; }
    constexpr static const std::string getRuleFormat() { return RouteRule::getRuleName() + " <path> { ... }"; }

    RouteRule() = default;
    RouteRule(Rule *rules) {
        _isSet = true;
        Object *object;

        RuleParser::create(rules, *this)
            .expectArgumentCount(2, 2)
            .parseArgument(path)
            .parseArgument(object);

        ObjectParser objectParser(object);
        objectParser.local().optional().parseRange(errorPageRule);
        objectParser.local().required().parseOne(rootRule);
    }

    inline bool isSet() const { return _isSet; }
};
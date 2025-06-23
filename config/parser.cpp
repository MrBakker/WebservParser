#include "parserExceptions.hpp"
#include "../print.hpp"
#include "config.hpp"

#include <memory>

static Key getRuleKeyFromToken(Token *token) {
    static const std::map<std::string, Key> keyMap = {
		{"server", SERVER},
		{"listen", LISTEN},
		{"server_name", SERVER_NAME},
		{"client_max_body_size", MAX_BODY_SIZE},
		{"error_page", ERROR_PAGE},
		{"root", ROOT},
		{"index", INDEX},
		{"autoindex", AUTOINDEX},
		{"redirect", REDIRECT},
		{"location", LOCATION},
		{"allowed_methods", ALLOWED_METHODS},
		{"upload_store", UPLOAD_DIR},
		{"cgi", CGI_PASS},
		{"cgi_timeout", CGI_TIMEOUT},
		{"define", DEFINE},
		{"include", INCLUDE},
	};

    auto it = keyMap.find(token->value);
    if (it == keyMap.end())
        throw ParserTokenException("Unknown rule key \"" + token->value + "\"", token);
    return (it->second);
}

static Keyword getKeyword(const std::string &str) {
	static const std::map<std::string, Keyword> keywordMap = {
		{"on", ON},
		{"off", OFF},
		{"true", TRUE},
		{"false", FALSE},
		{"default", DEFAULT},
		{"enable", ENABLE},
		{"disable", DISABLE},
		{"auto", AUTO},
	};

	auto it = keywordMap.find(str);
	if (it == keywordMap.end())
		return (NO_KEYWORD);
	return it->second;
}

void ConfigurationParser::_handleDefineRule(Rule *rule) {
    if (rule->arguments.size() != 2)
        throw ParserRuleException("Expected 2 arguments; found " + std::to_string(rule->arguments.size()), rule,
            "Check the syntax of the DEFINE rule. Expected format:\n\tdefine <name> <object>");
    if (rule->arguments[0]->type != ArgumentType::STRING)
        throw ParserArgumentException("Expected a string; found \"" + rule->arguments[0]->token->value + "\" instead.",
            rule->arguments[0],
            "Check the syntax of the DEFINE rule. Expected format:\n\tdefine <name> <object>");
    if (rule->arguments[1]->type != ArgumentType::OBJECT)
        throw ParserArgumentException("Expected an object; found \"" + rule->arguments[0]->token->value + "\" instead.",
            rule->arguments[1],
            "Check the syntax of the DEFINE rule. Expected format:\n\tdefine <name> <object>");

    std::string defineKey = std::get<std::string>(rule->arguments[0]->value);
    for (const auto &[key, value] : _objects)
        if (key == defineKey)
            throw ParserArgumentException("Object with name '" + defineKey + "' already exists", rule->arguments[0],
                "Give the object a different (unique) name to avoid conflicts.");
    if (defineKey == rule->configFile->fileName)
        throw ParserArgumentException("Object name cannot be the same as the configuration file name", rule->arguments[0],
            "Choose a different name for the object to avoid conflicts with the configuration file name.");
    _objects[defineKey] = std::get<Object*>(rule->arguments[1]->value);
}

void ConfigurationParser::_includeObjectIntoScope(Object *object, const Object *includedObject, Rule *includeRuleRef) {
    for (const auto &[key, rules] : includedObject->rules) {
        if (object->rules.find(key) == object->rules.end())
            object->rules[key] = {};
        for (const Rule *rule : rules) {
            Rule *newRule = _arena.alloc<Rule>(rule->key, std::vector<Argument*>(rule->arguments), nullptr, object, std::vector<Rule*>(rule->includePath), rule->configFile, rule->token);
            newRule->includePath.push_back(includeRuleRef);
            object->rules[key].push_back(newRule);
        }
    }
}

void ConfigurationParser::_handleIncludeRule(ConfigFile *file, size_t &pos, Rule *rule, Object *object) {
    if (rule->arguments.size() != 1)
        throw ParserTokenException("Invalid INCLUDE rule format. Expected 1 argument; found " + std::to_string(rule->arguments.size()), file->tokens[pos]);
    if (rule->arguments[0]->type != ArgumentType::STRING)
        throw ParserArgumentException("Argument of INCLUDE rule must be a string", rule->arguments[0]);

    std::string includePath = std::get<std::string>(rule->arguments[0]->value);
    if (!isFileLoaded(includePath)) {
        try { _loadConfigFile(includePath); }
        catch (ParserException &e) {
            e.addTracebackFromRule(rule);
            throw;
        }
    }

    auto it = _objects.find(includePath);
    if (it == _objects.end())
        throw ParserTokenException("Included object '" + includePath + "' not found in the configuration", file->tokens[pos]);
    _includeObjectIntoScope(object, it->second, rule);
}

Rule *ConfigurationParser::_parseRule(ConfigFile *file, size_t &pos, Rule *parentRule, Object *parentObject) {
    if (file->tokens[pos]->type != TokenType::STR)
        throw ParserTokenException("Expected a rule key, but found something else", file->tokens[pos]);

    Token *ruleToken = file->tokens[pos++];
    Rule *rule = _arena.alloc<Rule>(getRuleKeyFromToken(ruleToken), std::vector<Argument*>(), parentRule, parentObject, std::vector<Rule*>(), file, ruleToken);

    while (true) {
        if (file->tokens[pos]->type == TokenType::RULE_END) {
            ++pos;
            break;
        }

        else if (file->tokens[pos]->type == TokenType::OBJECT_OPEN) {
            rule->arguments.push_back(_arena.alloc<Argument>(ArgumentType::OBJECT, _parseObject(file, pos, rule), file, rule, file->tokens[pos]));
            break;
        }

        else if (file->tokens[pos]->type == TokenType::STR) {
            Keyword keyword = getKeyword(file->tokens[pos]->value);
            if (keyword != Keyword::NO_KEYWORD)
                rule->arguments.push_back(_arena.alloc<Argument>(ArgumentType::KEYWORD, keyword, file, rule, file->tokens[pos]));
            else
                rule->arguments.push_back(_arena.alloc<Argument>(ArgumentType::STRING, file->tokens[pos]->value, file, rule, file->tokens[pos]));
            ++pos;
        }

        else
            throw ParserTokenException("Unexpected token type while parsing rule", file->tokens[pos]);
    }

    return (rule);
}

Object *ConfigurationParser::_parseObject(ConfigFile *file, size_t &pos, Rule *parentRule) {
    Object *object = _arena.alloc<Object>(std::map<Key, Rules>(), file, parentRule, file->tokens[pos++], nullptr);

    while (file->tokens[pos]->type != TokenType::OBJECT_CLOSE) {
        Rule *rule = _parseRule(file, pos, parentRule, object);
        if (rule->key == Key::DEFINE)
            _handleDefineRule(rule);
        else if (rule->key == Key::INCLUDE)
            _handleIncludeRule(file, pos, rule, object);
        else {
            auto it = object->rules.find(rule->key);
            if (it == object->rules.end())
                object->rules[rule->key] = Rules();
            object->rules[rule->key].push_back(rule);
        }
    }

    object->objectCloseToken = file->tokens[pos++];
    return (object);
}

Object *ConfigurationParser::_getObjectFromFile(ConfigFile *file) {
    size_t pos = 0;

    return (_parseObject(file, pos, nullptr));
}

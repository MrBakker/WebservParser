#pragma once

#include "arena.hpp"

#include <algorithm>
#include <ostream>
#include <variant>
#include <vector>
#include <string>
#include <memory>
#include <map>

struct ConfigFile;
struct Argument;
struct Token;
struct Object;
struct Rule;

enum TokenType {
	UNKNOWN_TOKEN = 0,
	STR = 1 << 0,
	OBJECT_OPEN = 1 << 1,
	OBJECT_CLOSE = 1 << 2,
	RULE_END = 1 << 3,
	WHITESPACE = 1 << 4,
	COMMENT = 1 << 5,
	END = 1 << 6,
    QUOTE1 = 1 << 7,
    QUOTE2 = 1 << 8,
    LINE_END = 1 << 9,
};

enum Keyword {
	NO_KEYWORD = 0,
	ON = 1 << 0,
	OFF = 1 << 1,
	TRUE = 1 << 2,
	FALSE = 1 << 3,
	DEFAULT = 1 << 4,
	ENABLE = 1 << 5,
	DISABLE = 1 << 6,
	AUTO = 1 << 7,
};

enum Key {
	SERVER = 1 << 0,
	LISTEN = 1 << 1,
	SERVER_NAME = 1 << 2,
	MAX_BODY_SIZE = 1 << 3,
	ERROR_PAGE = 1 << 4,
	ROOT = 1 << 5,
	INDEX = 1 << 6,
	AUTOINDEX = 1 << 7,
	REDIRECT = 1 << 8,
	LOCATION = 1 << 9,
	ALLOWED_METHODS = 1 << 10,
	UPLOAD_DIR = 1 << 11,
	CGI_PASS = 1 << 12,
	CGI_TIMEOUT = 1 << 13,
	DEFINE = 1 << 14,
	INCLUDE = 1 << 15,
};

enum ArgumentType {
	STRING = 1 << 0,
	OBJECT = 1 << 1,
	KEYWORD = 1 << 2,
};

struct TokenPattern {
    const char *pattern;
    size_t length;
    TokenType type;
};

typedef std::variant<std::string, Object*, Keyword> ArgumentValue;
typedef std::vector<Rule*> Rules;

#define EOS_MASK_DEFAULT static_cast<TokenType>(~0 ^ TokenType::STR)
#define EOS_MASK_QUOTE(quoteType) static_cast<TokenType>(quoteType | TokenType::END)
#define COMMENT_MASK static_cast<TokenType>(TokenType::LINE_END | TokenType::END)
#define IS_USABLE_TOKEN_TYPE(type) ((type) & (STR | OBJECT_OPEN | OBJECT_CLOSE | RULE_END | END))

struct ErrorContext {
    std::string filename;
    std::string line;
    size_t lineNumber;
    size_t columnNumber;
};

struct Token {
    TokenType type;
    std::string value;
    ConfigFile *configFile;
    size_t filePos;
};

struct ConfigFile {
    std::string fileName;
    std::string fileContent;
    std::vector<Token*> tokens;
    std::vector<size_t> lineStarts;

    ErrorContext getErrorContext(size_t pos) const;
};

struct Object {
    std::map<Key, Rules> rules;
    ConfigFile *configFile;
    Rule *parentRule;
    Token *objectOpenToken;
    Token *objectCloseToken;
};

struct Rule {
    Key key;
    std::vector<Argument*> arguments;
    Rule *parent;
    Object *parentObject;
    std::vector<Rule*> includePath;
    ConfigFile *configFile;
    Token *token;
};

struct Argument {
    ArgumentType type;
    ArgumentValue value;
    ConfigFile *configFile;
    Rule *parentRule;
    Token *token;
};

class ConfigurationParser {
private:
    Arena _arena;
    std::map<std::string, Object*> _objects;
    std::map<std::string, ConfigFile*> _configFiles;
    std::vector<std::string> _includePaths;

    ConfigFile *_loadConfigFile(const std::string &filePath);

    Token *getNextToken(ConfigFile *configFile, size_t &pos);
    Token *parseContinousToken(ConfigFile *configFile, size_t &pos, Token *currentToken, TokenType endOfStringTypeMask);
    void _tokenize(ConfigFile *file);
    
    void _includeObjectIntoScope(Object *object, const Object *includedObject, Rule *includeRuleRef);
    void _handleDefineRule(Rule *rule);
    void _handleIncludeRule(ConfigFile *file, size_t &pos, Rule *rule, Object *object);

    Rule *_parseRule(ConfigFile *file, size_t &pos, Rule *parentRule, Object *parentObject);
    Object *_parseObject(ConfigFile *file, size_t &pos, Rule *parentRule);
    Object *_getObjectFromFile(ConfigFile *file);

public:
    ConfigurationParser() = default;
    ConfigurationParser(const ConfigurationParser&) = delete;
    ConfigurationParser& operator=(const ConfigurationParser&) = delete;
    ~ConfigurationParser() = default;

    bool parseFile(const std::string &filePath);
    Object *getResult(const std::string &filePath);

    /// @brief Check if a file is already loaded in the lexer - even if the file is not parsed yet.
    /// @param filePath The path of the file to check.
    /// @return True if the file is loaded, false otherwise.
    inline bool isFileLoaded(const std::string &filePath) { return (_objects.find(filePath) != _objects.end()); }
};

std::ostream &operator<<(std::ostream &os, const Token &token);
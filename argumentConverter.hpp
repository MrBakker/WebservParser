#pragma once

#include "config/parserExceptions.hpp"
#include "config/config.hpp"

#include <limits>
#include <string>

using ErrorCode = uint64_t;
using PortNumber = uint16_t;

template <typename To, typename From>
struct ArgumentConverter {
    static To convert(const From& from) {
        (void)from;
        throw std::runtime_error("Conversion not implemented for type " + std::string(typeid(To).name()) + " from type " + std::string(typeid(From).name()));
    }
};

template <>
struct ArgumentConverter<int, Argument*> {
    static int convert(const Argument* arg) {
        try {
            return (std::stoi(std::get<std::string>(arg->value)));
        } catch (...) {
            throw ParserArgumentException("Expected an integer", arg, \
                "Check the argument type. Expected an integer, but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<PortNumber, Argument*> {
    static PortNumber convert(const Argument* arg) {
        try {
            int value = std::stoi(std::get<std::string>(arg->value));
            if (value < std::numeric_limits<PortNumber>::min() || value > std::numeric_limits<PortNumber>::max())
                throw std::out_of_range("Value out of range for PortNumber");
            return static_cast<PortNumber>(value);
        } catch (...) {
            throw ParserArgumentException("Expected an unsigned 16-bit integer, but found: \"" + arg->token->value + "\"" , arg, \
                "Check the argument type. Expected an unsigned 16-bit integer, but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<ErrorCode, Argument*> {
    static ErrorCode convert(const Argument* arg) {
        try {
            int value = std::stoi(std::get<std::string>(arg->value));
            if (value < 100 || value > 599)
                throw std::out_of_range("Value out of range for ErrorCode");
            return static_cast<ErrorCode>(value);
        } catch (...) {
            throw ParserArgumentException("Expected a valid error code, but found: \"" + arg->token->value + "\"", arg, \
                "Check the argument type. Expected a valid error code, but found: " + arg->token->value);
        }
    }
};

template <>
struct ArgumentConverter<std::string, Argument*> {
    static std::string convert(const Argument* arg) {
        if (arg->type != ArgumentType::STRING)
            throw ParserArgumentException("Expected a string", arg, \
                "Check the argument type. Expected a string, but found: " + arg->token->value);
        return std::get<std::string>(arg->value);
    }
};

template <>
struct ArgumentConverter<Object*, Argument*> {
    static Object* convert(const Argument* arg) {
        if (arg->type != ArgumentType::OBJECT)
            throw ParserArgumentException("Expected an object", arg, \
                "Check the argument type. Expected an object, but found: " + arg->token ->value);
        return std::get<Object*>(arg->value);
    }
};

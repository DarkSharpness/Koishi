#pragma once
#include "MxLexer.h"
#include "MxParser.h"

struct MxErrorListener : antlr4::BaseErrorListener {
    void syntaxError(antlr4::Recognizer *recognizer, antlr4::Token * offendingSymbol, size_t line, size_t charPositionInLine,
      const std::string &msg, std::exception_ptr e) override {
        throw std::runtime_error("Syntax error: " + msg);
    }
};

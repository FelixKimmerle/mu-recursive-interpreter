#pragma once

#include "diagnostics.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mu {

enum class TokenKind {
    LeftParen,
    RightParen,
    Comma,
    Semicolon,
    Equal,
    Mu,         // '|'
    Successor,  // 's'
    Constant,   // 'c'
    Projection, // 'pi'
    Recursion,  // 'rec'
    Import,     // 'import'
    Identifier,
    Number,
    String, // "...", `text` holds the contents without quotes
    Invalid, // lexical error, `text` holds the message
    End,
};

struct Token {
    TokenKind kind;
    std::string text;
    SourceLocation location;
};

// Splits the whole source into tokens. The result always ends with an
// End token. Lexical errors are reported as Invalid tokens so that the
// parser can report them in order and recover. `firstLine` is the line
// number of the first line of `source` (for input that arrives in pieces).
std::vector<Token> tokenize(std::string_view source, std::size_t firstLine = 1);

std::string describe(TokenKind kind);

} // namespace mu

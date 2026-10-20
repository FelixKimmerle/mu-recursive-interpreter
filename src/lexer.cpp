#include "lexer.hpp"

#include <unordered_map>

namespace mu {

namespace {

bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool isIdentifierStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool isIdentifierPart(char c) { return isIdentifierStart(c) || isDigit(c); }

class Lexer {
public:
    Lexer(std::string_view source, std::size_t firstLine) : source_(source) { location_.line = firstLine; }

    std::vector<Token> run() {
        while (true) {
            skipWhitespaceAndComments();
            start_ = location_;
            if (atEnd()) {
                break;
            }
            scanToken();
        }
        tokens_.push_back({TokenKind::End, "", location_});
        return std::move(tokens_);
    }

private:
    bool atEnd() const { return pos_ >= source_.size(); }

    char peek(std::size_t ahead = 0) const {
        return pos_ + ahead < source_.size() ? source_[pos_ + ahead] : '\0';
    }

    char advance() {
        char c = source_[pos_++];
        if (c == '\n') {
            ++location_.line;
            location_.column = 1;
        } else {
            ++location_.column;
        }
        return c;
    }

    void add(TokenKind kind, std::string text) {
        tokens_.push_back({kind, std::move(text), start_});
    }

    void skipWhitespaceAndComments() {
        while (!atEnd()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                advance();
            } else if (c == '/' && peek(1) == '/') {
                while (!atEnd() && peek() != '\n') {
                    advance();
                }
            } else if (c == '/' && peek(1) == '*') {
                skipBlockComment();
            } else {
                return;
            }
        }
    }

    // Block comments nest: /* a /* b */ c */ is a single comment.
    void skipBlockComment() {
        SourceLocation opened = location_;
        advance();
        advance();
        int depth = 1;
        while (depth > 0) {
            if (atEnd()) {
                tokens_.push_back({TokenKind::Invalid, "unterminated block comment", opened});
                return;
            }
            if (peek() == '/' && peek(1) == '*') {
                advance();
                advance();
                ++depth;
            } else if (peek() == '*' && peek(1) == '/') {
                advance();
                advance();
                --depth;
            } else {
                advance();
            }
        }
    }

    // A string literal on a single line, without escape sequences.
    void string() {
        std::size_t begin = pos_;
        while (!atEnd() && peek() != '"' && peek() != '\n') {
            advance();
        }
        if (peek() != '"') {
            add(TokenKind::Invalid, "unterminated string");
            return;
        }
        add(TokenKind::String, std::string(source_.substr(begin, pos_ - begin)));
        advance();
    }

    void scanToken() {
        char c = advance();
        switch (c) {
        case '(': add(TokenKind::LeftParen, "("); return;
        case ')': add(TokenKind::RightParen, ")"); return;
        case ',': add(TokenKind::Comma, ","); return;
        case ';': add(TokenKind::Semicolon, ";"); return;
        case '=': add(TokenKind::Equal, "="); return;
        case '|': add(TokenKind::Mu, "|"); return;
        case '"': string(); return;
        default: break;
        }

        if (isDigit(c)) {
            std::size_t begin = pos_ - 1;
            while (isDigit(peek())) {
                advance();
            }
            add(TokenKind::Number, std::string(source_.substr(begin, pos_ - begin)));
            return;
        }

        if (isIdentifierStart(c)) {
            static const std::unordered_map<std::string_view, TokenKind> keywords = {
                {"s", TokenKind::Successor},
                {"c", TokenKind::Constant},
                {"pi", TokenKind::Projection},
                {"rec", TokenKind::Recursion},
                {"import", TokenKind::Import},
            };
            std::size_t begin = pos_ - 1;
            while (isIdentifierPart(peek())) {
                advance();
            }
            std::string_view word = source_.substr(begin, pos_ - begin);
            auto keyword = keywords.find(word);
            add(keyword != keywords.end() ? keyword->second : TokenKind::Identifier, std::string(word));
            return;
        }

        add(TokenKind::Invalid, "unexpected character '" + std::string(1, c) + "'");
    }

    std::string_view source_;
    std::size_t pos_ = 0;
    SourceLocation location_;
    SourceLocation start_;
    std::vector<Token> tokens_;
};

} // namespace

std::vector<Token> tokenize(std::string_view source, std::size_t firstLine) {
    return Lexer(source, firstLine).run();
}

std::string describe(TokenKind kind) {
    switch (kind) {
    case TokenKind::LeftParen: return "'('";
    case TokenKind::RightParen: return "')'";
    case TokenKind::Comma: return "','";
    case TokenKind::Semicolon: return "';'";
    case TokenKind::Equal: return "'='";
    case TokenKind::Mu: return "'|'";
    case TokenKind::Successor: return "'s'";
    case TokenKind::Constant: return "'c'";
    case TokenKind::Projection: return "'pi'";
    case TokenKind::Recursion: return "'rec'";
    case TokenKind::Import: return "'import'";
    case TokenKind::Identifier: return "identifier";
    case TokenKind::Number: return "number";
    case TokenKind::String: return "string";
    case TokenKind::Invalid: return "invalid token";
    case TokenKind::End: return "end of input";
    }
    return "token";
}

} // namespace mu

#pragma once

#include "function.hpp"
#include "lexer.hpp"

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace mu
{

// Named functions defined so far. Names are resolved while parsing, so a
// definition can only refer to functions defined before it (no general
// recursion sneaks in) and redefining a name does not change earlier uses.
using Environment = std::unordered_map<std::string, FunctionPtr>;

// name = expr;
struct Definition
{
    std::string name;
    FunctionPtr function;
};

// expr(n_1, ..., n_k);
struct Evaluation
{
    std::string text;
    FunctionPtr function;
    std::vector<Nat> arguments;
};

// expr;
struct Signature
{
    std::string text;
    FunctionPtr function;
};

// import "file.pr";
struct Import
{
    std::string path;
    SourceLocation location;
};

using Statement = std::variant<Definition, Evaluation, Signature, Import>;

class Parser
{
  public:
    Parser(std::vector<Token> tokens, const Environment& environment);

    bool atEnd() const;

    // Parses the next statement. Throws CompileError on malformed input;
    // call synchronize() afterwards to continue with the next statement.
    Statement parseStatement();

    // Skips the rest of the current (broken) statement.
    void synchronize();

  private:
    FunctionPtr expression();
    FunctionPtr postfix();
    FunctionPtr primary();
    FunctionPtr composition(FunctionPtr outer, std::size_t outerStart);
    std::vector<Nat> argumentList();

    const Token& peek(std::size_t ahead = 0) const;
    const Token& advance();
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    const Token& expect(TokenKind kind, const char* context);
    Nat number(const char* context);
    std::size_t arityNumber(const char* context);
    std::string text(std::size_t begin, std::size_t end) const;

    std::vector<Token> tokens_;
    std::size_t current_ = 0;
    const Environment& environment_;
};

} // namespace mu

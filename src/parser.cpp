#include "parser.hpp"

#include <charconv>
#include <limits>

namespace mu
{

namespace
{

FunctionPtr make(std::size_t arity, auto node)
{
    return std::make_shared<const Function>(Function{arity, std::move(node)});
}

std::string plural(std::size_t n, const char* word)
{
    return std::to_string(n) + " " + word + (n == 1 ? "" : "s");
}

// Arities are bounded so that evaluation never has to allocate absurd
// argument vectors. Nobody writes a function with a million parameters.
constexpr std::size_t maxArity = 1'000'000;

} // namespace

Parser::Parser(std::vector<Token> tokens, const Environment& environment)
    : tokens_(std::move(tokens)), environment_(environment)
{
}

bool Parser::atEnd() const
{
    return check(TokenKind::End);
}

Statement Parser::parseStatement()
{
    if (check(TokenKind::Import))
    {
        const SourceLocation location = advance().location;
        std::string path = expect(TokenKind::String, "after 'import'").text;
        expect(TokenKind::Semicolon, "after import");
        return Import{std::move(path), location};
    }

    if (check(TokenKind::Identifier) && peek(1).kind == TokenKind::Equal)
    {
        std::string name = advance().text;
        advance();
        FunctionPtr function = expression();
        expect(TokenKind::Semicolon, "after definition");
        return Definition{std::move(name), std::move(function)};
    }

    const bool isKeyword = check(TokenKind::Successor) || check(TokenKind::Constant) || check(TokenKind::Projection) ||
                           check(TokenKind::Recursion) || check(TokenKind::Import);
    if (isKeyword && peek(1).kind == TokenKind::Equal)
    {
        throw CompileError(peek().location, "cannot redefine built-in '" + peek().text + "'");
    }

    const std::size_t start = current_;
    const SourceLocation location = peek().location;
    FunctionPtr function = expression();
    const std::size_t functionEnd = current_;

    if (check(TokenKind::LeftParen))
    {
        std::vector<Nat> arguments = argumentList();
        std::string statementText = text(start, current_);
        if (arguments.size() != function->arity)
        {
            throw CompileError(location, "'" + text(start, functionEnd) + "' expects " +
                                             plural(function->arity, "argument") + ", got " +
                                             std::to_string(arguments.size()));
        }
        expect(TokenKind::Semicolon, "after evaluation");
        return Evaluation{std::move(statementText), std::move(function), std::move(arguments)};
    }

    std::string statementText = text(start, current_);
    expect(TokenKind::Semicolon, "after expression (expected '(' to evaluate it or ';')");
    return Signature{std::move(statementText), std::move(function)};
}

void Parser::synchronize()
{
    while (!atEnd())
    {
        if (advance().kind == TokenKind::Semicolon)
        {
            return;
        }
    }
}

// expression := '|' expression | postfix
FunctionPtr Parser::expression()
{
    if (check(TokenKind::Mu))
    {
        const SourceLocation location = advance().location;
        FunctionPtr predicate = expression();
        if (predicate->arity == 0)
        {
            throw CompileError(location, "the mu operator needs a function with at least one argument");
        }
        const std::size_t arity = predicate->arity - 1;
        return make(arity, Minimization{std::move(predicate)});
    }
    return postfix();
}

// postfix := primary { '(' expression { ',' expression } ')' }
//
// A parenthesized list that starts with a number (or is empty) is not a
// composition but the argument list of an evaluation statement, so it is
// left for parseStatement().
FunctionPtr Parser::postfix()
{
    const std::size_t start = current_;
    FunctionPtr function = primary();
    while (check(TokenKind::LeftParen) && peek(1).kind != TokenKind::Number && peek(1).kind != TokenKind::RightParen)
    {
        function = composition(std::move(function), start);
    }
    return function;
}

FunctionPtr Parser::composition(FunctionPtr outer, std::size_t outerStart)
{
    const std::string outerText = text(outerStart, current_);
    const SourceLocation location = advance().location;

    std::vector<FunctionPtr> inner;
    std::vector<SourceLocation> innerLocations;
    do
    {
        innerLocations.push_back(peek().location);
        inner.push_back(expression());
    } while (match(TokenKind::Comma));
    expect(TokenKind::RightParen, "after composition arguments");

    if (inner.size() != outer->arity)
    {
        throw CompileError(location, "'" + outerText + "' expects " + plural(outer->arity, "argument") +
                                         ", but is composed with " + plural(inner.size(), "function"));
    }
    const std::size_t arity = inner.front()->arity;
    for (std::size_t i = 1; i < inner.size(); ++i)
    {
        if (inner[i]->arity != arity)
        {
            throw CompileError(innerLocations[i],
                               "all functions in a composition must have the same arity: argument 1 has arity " +
                                   std::to_string(arity) + ", argument " + std::to_string(i + 1) + " has arity " +
                                   std::to_string(inner[i]->arity));
        }
    }
    return make(arity, Composition{std::move(outer), std::move(inner)});
}

FunctionPtr Parser::primary()
{
    const Token& token = advance();
    switch (token.kind)
    {
    case TokenKind::Successor:
        return make(1, Successor{});

    case TokenKind::Constant: {
        expect(TokenKind::LeftParen, "after 'c'");
        std::size_t arity = arityNumber("as arity of c(k, n)");
        expect(TokenKind::Comma, "in c(k, n)");
        Nat value = number("as value of c(k, n)");
        expect(TokenKind::RightParen, "after c(k, n)");
        return make(arity, Constant{value});
    }

    case TokenKind::Projection: {
        expect(TokenKind::LeftParen, "after 'pi'");
        std::size_t arity = arityNumber("as arity of pi(k, i)");
        expect(TokenKind::Comma, "in pi(k, i)");
        const SourceLocation indexLocation = peek().location;
        Nat index = number("as index of pi(k, i)");
        expect(TokenKind::RightParen, "after pi(k, i)");
        if (index < 1 || index > arity)
        {
            throw CompileError(indexLocation, "projection index must satisfy 1 <= i <= k, but pi(" +
                                                  std::to_string(arity) + ", " + std::to_string(index) + ")");
        }
        return make(arity, Projection{static_cast<std::size_t>(index - 1)});
    }

    case TokenKind::Recursion: {
        expect(TokenKind::LeftParen, "after 'rec'");
        FunctionPtr base = expression();
        expect(TokenKind::Comma, "between base and step of rec(g, h)");
        const SourceLocation stepLocation = peek().location;
        FunctionPtr step = expression();
        expect(TokenKind::RightParen, "after rec(g, h)");
        if (step->arity != base->arity + 2)
        {
            throw CompileError(stepLocation, "in rec(g, h), h must take 2 more arguments than g: g has arity " +
                                                 std::to_string(base->arity) + ", so h needs arity " +
                                                 std::to_string(base->arity + 2) + ", but has arity " +
                                                 std::to_string(step->arity));
        }
        const std::size_t arity = base->arity + 1;
        return make(arity, PrimitiveRecursion{std::move(base), std::move(step)});
    }

    case TokenKind::Identifier: {
        auto found = environment_.find(token.text);
        if (found == environment_.end())
        {
            throw CompileError(token.location, "unknown function '" + token.text + "'");
        }
        return found->second;
    }

    case TokenKind::LeftParen: {
        FunctionPtr inner = expression();
        expect(TokenKind::RightParen, "to close '('");
        return inner;
    }

    case TokenKind::Invalid:
        throw CompileError(token.location, token.text);

    default:
        throw CompileError(token.location,
                           "expected a function, found " +
                               (token.kind == TokenKind::End ? describe(token.kind) : "'" + token.text + "'"));
    }
}

// '(' [ number { ',' number } ] ')'
std::vector<Nat> Parser::argumentList()
{
    expect(TokenKind::LeftParen, "before arguments");
    std::vector<Nat> arguments;
    if (!check(TokenKind::RightParen))
    {
        do
        {
            arguments.push_back(number("as argument (only numbers can be passed when evaluating)"));
        } while (match(TokenKind::Comma));
    }
    expect(TokenKind::RightParen, "after arguments");
    return arguments;
}

const Token& Parser::peek(std::size_t ahead) const
{
    return tokens_[std::min(current_ + ahead, tokens_.size() - 1)];
}

const Token& Parser::advance()
{
    const Token& token = tokens_[current_];
    if (token.kind != TokenKind::End)
    {
        ++current_;
    }
    return token;
}

bool Parser::check(TokenKind kind) const
{
    return peek().kind == kind;
}

bool Parser::match(TokenKind kind)
{
    if (check(kind))
    {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenKind kind, const char* context)
{
    if (check(kind))
    {
        return advance();
    }
    const Token& token = peek();
    if (token.kind == TokenKind::Invalid)
    {
        throw CompileError(token.location, token.text);
    }
    const std::string found = token.kind == TokenKind::End ? describe(token.kind) : "'" + token.text + "'";
    throw CompileError(token.location, "expected " + describe(kind) + " " + context + ", found " + found);
}

Nat Parser::number(const char* context)
{
    const Token& token = expect(TokenKind::Number, context);
    Nat value = 0;
    auto [end, error] = std::from_chars(token.text.data(), token.text.data() + token.text.size(), value);
    if (error != std::errc{} || end != token.text.data() + token.text.size())
    {
        throw CompileError(token.location, "number " + token.text + " is too large (maximum is " +
                                               std::to_string(std::numeric_limits<Nat>::max()) + ")");
    }
    return value;
}

std::size_t Parser::arityNumber(const char* context)
{
    const SourceLocation location = peek().location;
    Nat value = number(context);
    if (value > maxArity)
    {
        throw CompileError(location, "arity " + std::to_string(value) + " is too large (maximum is " +
                                         std::to_string(maxArity) + ")");
    }
    return static_cast<std::size_t>(value);
}

// Reconstructs normalized source text for tokens [begin, end) for messages
// and output, e.g. "add( 5,7 )" becomes "add(5, 7)".
std::string Parser::text(std::size_t begin, std::size_t end) const
{
    std::string result;
    for (std::size_t i = begin; i < end && i < tokens_.size(); ++i)
    {
        result += tokens_[i].text;
        if (tokens_[i].kind == TokenKind::Comma)
        {
            result += ' ';
        }
    }
    return result;
}

} // namespace mu

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace mu
{

struct SourceLocation
{
    std::size_t line = 1;
    std::size_t column = 1;
};

// Raised for anything that is wrong with the program text: bad tokens,
// syntax errors, unknown names and arity mismatches.
class CompileError : public std::runtime_error
{
  public:
    CompileError(SourceLocation location, const std::string& message) : std::runtime_error(message), location_(location)
    {
    }

    SourceLocation location() const
    {
        return location_;
    }

  private:
    SourceLocation location_;
};

// Raised while evaluating a well-formed function (overflow, interrupt).
class RuntimeError : public std::runtime_error
{
  public:
    using std::runtime_error::runtime_error;
};

} // namespace mu

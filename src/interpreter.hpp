#pragma once

#include "parser.hpp"

#include <filesystem>
#include <iosfwd>
#include <set>
#include <string_view>

namespace mu
{

struct Options
{
    // Print only the value of an evaluation instead of "expr(args) = value".
    bool quiet = false;
};

// Runs programs statement by statement. Definitions persist across calls
// to run(), so several files (or REPL inputs) share one environment.
class Interpreter
{
  public:
    Interpreter(std::ostream& out, std::ostream& err, Options options = {});

    // Returns false if any statement failed. Errors are reported to `err`
    // and execution continues with the next statement. Imports in `source`
    // are resolved relative to the current working directory.
    bool run(std::string_view source, std::string_view sourceName, std::size_t firstLine = 1);

    // Runs a file; its imports are resolved relative to its directory.
    // Each file is loaded at most once, later imports of it are skipped.
    bool runFile(const std::filesystem::path& path);

  private:
    bool run(std::string_view source, std::string_view sourceName, std::size_t firstLine,
             const std::filesystem::path& directory);
    bool execute(const Statement& statement, const std::filesystem::path& directory);
    bool load(const std::filesystem::path& path, std::string& contents);

    Environment environment_;
    std::set<std::filesystem::path> loaded_;
    std::ostream& out_;
    std::ostream& err_;
    Options options_;
};

} // namespace mu

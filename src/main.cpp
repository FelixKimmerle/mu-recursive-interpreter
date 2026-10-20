#include "evaluator.hpp"
#include "interpreter.hpp"

#include <csignal>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

namespace {

constexpr const char* usage = R"(usage: mu-interp [options] [file ...]

Runs mu-recursive programs. Files are executed in order and share their
definitions. A program can load a library itself with: import "file.pr";
Without files (and without -e), an interactive session is started when
stdin is a terminal; otherwise the program is read from stdin.

options:
  -e <code>    run <code> as if it were a file
  -i           start an interactive session after running files / -e
  -q           print only the values of evaluations
  -h, --help   show this help

Use '-' as a file name to read from stdin.
)";

// A file to run, or inline code (from -e or stdin) when `isFile` is false.
struct Input {
    bool isFile;
    std::string text;
    std::string name = "";
};

// A REPL input is complete once its last token is a ';' (or it holds no
// tokens at all), so statements may span several lines.
bool isComplete(const std::string& input) {
    std::vector<mu::Token> tokens = mu::tokenize(input);
    return tokens.size() == 1 || tokens[tokens.size() - 2].kind == mu::TokenKind::Semicolon;
}

// Returns false if any statement failed.
bool repl(mu::Interpreter& interpreter) {
    std::signal(SIGINT, [](int) { mu::interruptRequested = true; });

    const bool interactive = isatty(STDIN_FILENO);
    if (interactive) {
        std::cout << "mu-recursive interpreter. End statements with ';', exit with Ctrl-D.\n"
                     "Ctrl-C aborts a running evaluation.\n";
    }

    bool ok = true;
    std::string pending;
    std::size_t pendingFirstLine = 1;
    std::string line;
    std::size_t lineNumber = 0;
    while (true) {
        if (interactive) {
            std::cout << (pending.empty() ? "> " : ". ") << std::flush;
        }
        if (!std::getline(std::cin, line)) {
            break;
        }
        ++lineNumber;
        if (pending.empty()) {
            pendingFirstLine = lineNumber;
        }
        pending += line;
        pending += '\n';
        if (!isComplete(pending)) {
            continue;
        }
        mu::interruptRequested = false;
        ok = interpreter.run(pending, "<stdin>", pendingFirstLine) && ok;
        pending.clear();
    }
    if (!pending.empty()) {
        ok = interpreter.run(pending, "<stdin>", pendingFirstLine) && ok;
    }
    if (interactive) {
        std::cout << '\n';
    }
    return ok;
}

} // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    mu::Options options;
    bool interactiveAfterwards = false;
    std::vector<Input> inputs;

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "-h" || argument == "--help") {
            std::cout << usage;
            return 0;
        } else if (argument == "-q") {
            options.quiet = true;
        } else if (argument == "-i") {
            interactiveAfterwards = true;
        } else if (argument == "-e") {
            if (i + 1 >= argc) {
                std::cerr << "mu-interp: -e needs an argument\n" << usage;
                return 2;
            }
            inputs.push_back({false, argv[++i], "<-e>"});
        } else if (argument.size() > 1 && argument[0] == '-') {
            std::cerr << "mu-interp: unknown option '" << argument << "'\n" << usage;
            return 2;
        } else if (argument == "-") {
            std::ostringstream buffer;
            buffer << std::cin.rdbuf();
            inputs.push_back({false, buffer.str(), "<stdin>"});
        } else {
            inputs.push_back({true, argument});
        }
    }

    mu::Interpreter interpreter(std::cout, std::cerr, options);
    bool ok = true;
    for (const Input& input : inputs) {
        ok = (input.isFile ? interpreter.runFile(input.text) : interpreter.run(input.text, input.name)) && ok;
    }

    if (inputs.empty() || interactiveAfterwards) {
        const bool replOk = repl(interpreter);
        // Mistakes typed into an interactive session are not failures of
        // the program, but errors in piped input are.
        if (!isatty(STDIN_FILENO)) {
            ok = replOk && ok;
        }
    }
    return ok ? 0 : 1;
}

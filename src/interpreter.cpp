#include "interpreter.hpp"

#include "evaluator.hpp"

#include <fstream>
#include <ostream>
#include <sstream>

namespace mu
{

namespace
{

std::string signature(std::size_t arity)
{
    return arity == 1 ? "N -> N" : "N^" + std::to_string(arity) + " -> N";
}

} // namespace

Interpreter::Interpreter(std::ostream& out, std::ostream& err, Options options)
    : out_(out), err_(err), options_(options)
{
}

bool Interpreter::run(std::string_view source, std::string_view sourceName, std::size_t firstLine)
{
    return run(source, sourceName, firstLine, std::filesystem::current_path());
}

bool Interpreter::runFile(const std::filesystem::path& path)
{
    std::string contents;
    if (!load(path, contents))
    {
        out_.flush();
        err_ << "error: cannot open '" << path.string() << "'\n";
        return false;
    }
    return run(contents, path.string(), 1, path.parent_path());
}

bool Interpreter::load(const std::filesystem::path& path, std::string& contents)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    contents = buffer.str();
    loaded_.insert(std::filesystem::weakly_canonical(path));
    return true;
}

bool Interpreter::run(std::string_view source, std::string_view sourceName, std::size_t firstLine,
                      const std::filesystem::path& directory)
{
    Parser parser(tokenize(source, firstLine), environment_);
    bool ok = true;
    const auto report = [&](const CompileError& error) {
        out_.flush();
        err_ << sourceName << ':' << error.location().line << ':' << error.location().column
             << ": error: " << error.what() << '\n';
        ok = false;
    };
    while (!parser.atEnd())
    {
        Statement statement;
        try
        {
            statement = parser.parseStatement();
        }
        catch (const CompileError& error)
        {
            report(error);
            parser.synchronize();
            continue;
        }
        // The statement is complete here, so errors while executing it must
        // not skip anything.
        try
        {
            ok = execute(statement, directory) && ok;
        }
        catch (const CompileError& error)
        {
            report(error);
        }
        catch (const RuntimeError& error)
        {
            out_.flush();
            err_ << sourceName << ": runtime error: " << error.what() << '\n';
            ok = false;
        }
    }
    out_.flush();
    return ok;
}

bool Interpreter::execute(const Statement& statement, const std::filesystem::path& directory)
{
    if (const auto* import = std::get_if<Import>(&statement))
    {
        const std::filesystem::path path = directory / import->path;
        if (loaded_.contains(std::filesystem::weakly_canonical(path)))
        {
            return true;
        }
        std::string contents;
        if (!load(path, contents))
        {
            throw CompileError(import->location, "cannot open '" + path.string() + "'");
        }
        return run(contents, path.string(), 1, path.parent_path());
    }
    if (const auto* definition = std::get_if<Definition>(&statement))
    {
        environment_.insert_or_assign(definition->name, definition->function);
    }
    else if (const auto* evaluation = std::get_if<Evaluation>(&statement))
    {
        Nat value = 0;
        try
        {
            value = evaluate(*evaluation->function, evaluation->arguments);
        }
        catch (const RuntimeError& error)
        {
            throw RuntimeError(evaluation->text + ": " + error.what());
        }
        if (!options_.quiet)
        {
            out_ << evaluation->text << " = ";
        }
        out_ << value << '\n';
    }
    else if (const auto* query = std::get_if<Signature>(&statement))
    {
        out_ << query->text << " : " << signature(query->function->arity) << '\n';
    }
    return true;
}

} // namespace mu

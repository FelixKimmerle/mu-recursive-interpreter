#pragma once

#include "function.hpp"

#include <atomic>
#include <span>

namespace mu {

// Set (e.g. from a SIGINT handler) to abort a running evaluation; the
// evaluator then throws a RuntimeError and resets the flag.
extern std::atomic<bool> interruptRequested;

// Evaluates `function` at `arguments`. The caller guarantees that
// arguments.size() == function.arity. May not terminate if the function
// contains a minimization that is undefined at this point.
Nat evaluate(const Function& function, std::span<const Nat> arguments);

} // namespace mu

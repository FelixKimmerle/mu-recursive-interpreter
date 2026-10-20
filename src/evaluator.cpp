#include "evaluator.hpp"

#include "diagnostics.hpp"

#include <algorithm>
#include <limits>

namespace mu
{

std::atomic<bool> interruptRequested{false};

namespace
{

constexpr Nat maxNat = std::numeric_limits<Nat>::max();

void checkInterrupt()
{
    if (interruptRequested.exchange(false, std::memory_order_relaxed))
    {
        throw RuntimeError("interrupted");
    }
}

struct Evaluator
{
    std::span<const Nat> x;

    Nat operator()(const Successor&) const
    {
        if (x[0] == maxNat)
        {
            throw RuntimeError("overflow: result exceeds " + std::to_string(maxNat));
        }
        return x[0] + 1;
    }

    Nat operator()(const Constant& constant) const
    {
        return constant.value;
    }

    Nat operator()(const Projection& projection) const
    {
        return x[projection.index];
    }

    Nat operator()(const Composition& composition) const
    {
        std::vector<Nat> values;
        values.reserve(composition.inner.size());
        for (const FunctionPtr& g : composition.inner)
        {
            values.push_back(evaluate(*g, x));
        }
        return evaluate(*composition.outer, values);
    }

    // Computed bottom-up: f(0, x), f(1, x), ..., f(n, x). The argument buffer
    // for h is [f(i, x), i, x_1, ..., x_k] and is reused across iterations.
    Nat operator()(const PrimitiveRecursion& recursion) const
    {
        const Nat n = x[0];
        const std::span<const Nat> rest = x.subspan(1);

        Nat accumulator = evaluate(*recursion.base, rest);
        std::vector<Nat> stepArguments(rest.size() + 2);
        std::ranges::copy(rest, stepArguments.begin() + 2);
        for (Nat i = 0; i < n; ++i)
        {
            checkInterrupt();
            stepArguments[0] = accumulator;
            stepArguments[1] = i;
            accumulator = evaluate(*recursion.step, stepArguments);
        }
        return accumulator;
    }

    // Unbounded search for the least n with f(n, x) = 0. Diverges if there is
    // none, exactly like the mathematical definition says it is undefined.
    Nat operator()(const Minimization& minimization) const
    {
        std::vector<Nat> arguments(x.size() + 1);
        std::ranges::copy(x, arguments.begin() + 1);
        for (Nat n = 0;; ++n)
        {
            checkInterrupt();
            arguments[0] = n;
            if (evaluate(*minimization.predicate, arguments) == 0)
            {
                return n;
            }
            if (n == maxNat)
            {
                throw RuntimeError("overflow: minimization searched all of 0.." + std::to_string(maxNat));
            }
        }
    }
};

} // namespace

Nat evaluate(const Function& function, std::span<const Nat> arguments)
{
    return std::visit(Evaluator{arguments}, function.node);
}

} // namespace mu

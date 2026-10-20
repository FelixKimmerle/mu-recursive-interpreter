#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

namespace mu {

// The natural numbers, as far as a machine word reaches.
using Nat = std::uint64_t;

struct Function;
using FunctionPtr = std::shared_ptr<const Function>;

// s(x) = x + 1
struct Successor {};

// c(k, n)(x_1, ..., x_k) = n
struct Constant {
    Nat value;
};

// pi(k, i)(x_1, ..., x_k) = x_i   (index is stored 0-based)
struct Projection {
    std::size_t index;
};

// f(g_1, ..., g_m)(x) = f(g_1(x), ..., g_m(x))
struct Composition {
    FunctionPtr outer;
    std::vector<FunctionPtr> inner;
};

// rec(g, h)(0,     x) = g(x)
// rec(g, h)(n + 1, x) = h(rec(g, h)(n, x), n, x)
struct PrimitiveRecursion {
    FunctionPtr base;
    FunctionPtr step;
};

// |f(x) = min { n | f(n, x) = 0 }
struct Minimization {
    FunctionPtr predicate;
};

// A mu-recursive function term together with its (statically known) arity.
// Terms are immutable and shared, so a named function used in several
// places is stored only once.
struct Function {
    std::size_t arity;
    std::variant<Successor, Constant, Projection, Composition, PrimitiveRecursion, Minimization> node;
};

} // namespace mu

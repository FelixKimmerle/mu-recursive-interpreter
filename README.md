# mu-recursive-interpreter

An interpreter for **μ-recursive functions** (*μ-rekursive Funktionen*) as introduced in
the lecture [*Theoretische Informatik II*](https://fmi.uni-stuttgart.de/ti/teaching/s20/ti2/)
at the University of Stuttgart. You build functions on the natural numbers from a few
basic functions (successor, constants, projections) and three operators (composition,
primitive recursion, μ-operator), then evaluate them.

```text
add  = rec(pi(1, 1), s(pi(3, 1)));      // add(n, x) = n + x
mult = rec(c(1, 0), add(pi(3, 3), pi(3, 1)));
mult(6, 7);                             // prints: mult(6, 7) = 42
```

This is a C++20 port of an older Java implementation, with bugs fixed and static
arity checking added (see [Changes compared to the Java version](#changes-compared-to-the-java-version)).

> **Educational purpose only.** This project was written while studying for the lecture,
> to experiment with the definitions and check exercises. It is not a practical
> programming tool: arithmetic is built from the successor function and is therefore
> very slow, and it is not affiliated with or endorsed by the University of Stuttgart
> or the lecturers.

## Background

The theory behind this interpreter is covered in units 10 to 12 of the lecture
[Theoretische Informatik II](https://fmi.uni-stuttgart.de/ti/teaching/s20/ti2/):

| Unit | Topic | Used here |
|---|---|---|
| **Einheit 10** | Primitive recursion: basic functions, composition (*Einsetzen*), the recursion scheme; the pairing function $\mathbb{N}^2 \to \mathbb{N}$ and its inverses; bounded operators | `s`, `c`, `pi`, composition, `rec`; [`pairing.pr`](examples/pairing.pr) |
| **Einheit 11** | Primitive recursive = LOOP-computable; the μ-operator and the class of μ-recursive functions | `\|f`; [`minimization.pr`](examples/minimization.pr) (including the lecture example $\mu g(17, 5) = 3$) |
| **Einheit 12** | μ-recursive = WHILE-computable; Kleene's normal form theorem; the Ackermann function | why μ makes functions partial, [Kleene's theorem](#6-the-μ-operator-f) |

The notation in this README and the conventions of the interpreter follow these
units: `rec` recurses over the first argument with
$f(n+1, \vec{x}) = h(f(n, \vec{x}), n, \vec{x})$, and μ searches over the first
argument.

---

## Contents

- [Background](#background)
- [Building](#building)
- [Usage](#usage)
- [The language](#the-language)
- [The building blocks, mathematically](#the-building-blocks-mathematically)
  - [Successor](#1-successor-s)
  - [Constants](#2-constants-ck-n)
  - [Projections](#3-projections-pik-i)
  - [Composition](#4-composition-fg1--gm)
  - [Primitive recursion](#5-primitive-recursion-recg-h)
  - [The μ-operator](#6-the-μ-operator-f)
- [Examples](#examples)
- [Errors](#errors)
- [Limitations](#limitations)
- [Project layout and tests](#project-layout-and-tests)
- [Changes compared to the Java version](#changes-compared-to-the-java-version)

---

## Building

Requirements: CMake ≥ 3.20 and a C++20 compiler (GCC ≥ 11, Clang ≥ 14, MSVC 2022).

```bash
cmake -S . -B build
```

```bash
cmake --build build
```

The executable is `build/mu-interp`. To run the test suite:

```bash
ctest --test-dir build
```

## Usage

```text
mu-interp [options] [file ...]

  -e <code>    run <code> as if it were a file
  -i           start an interactive session after running files / -e
  -q           print only the values of evaluations
  -h, --help   show this help
```

```bash
build/mu-interp examples/minimization.pr
```

A program can load a library with `import "file.pr";` (path relative to the importing
file), e.g. the examples start with `import "prelude.pr";`. Several files given on the
command line are executed in order and **share their definitions**.

Without any file the interpreter starts an interactive session (REPL). Statements may
span several lines; they are executed as soon as a `;` ends the input. Ctrl-C aborts a
running evaluation (useful when a μ-search never ends), Ctrl-D exits.

```text
$ build/mu-interp examples/prelude.pr -i     # load the library, then interact
> fact(5);
fact(5) = 120
> square = mult(id, id);
> square;
square : N -> N
> square(12);
square(12) = 144
```

Programs can also be piped in (`echo 's(1);' | build/mu-interp`) or passed inline:

```bash
build/mu-interp -q -e 'add = rec(pi(1,1), s(pi(3,1))); add(20, 22);'
```

The exit status is `0` on success, `1` if any statement failed, and `2` for bad
command-line usage.

## The language

A program is a sequence of statements, each terminated by `;`:

| Statement                 | Meaning                                                    |
|---------------------------|------------------------------------------------------------|
| `name = expr;`            | define a function                                          |
| `expr(n1, ..., nk);`      | evaluate a k-ary function at numbers, prints `... = value` |
| `expr();`                 | evaluate a 0-ary function                                  |
| `expr;`                   | print the signature (arity) of a function                  |
| `import "file.pr";`       | run another file (once), keeping its definitions           |

Expressions:

| Syntax                  | Function                         | Arity            |
|-------------------------|----------------------------------|------------------|
| `s`                     | successor                        | 1                |
| `c(k, n)`               | k-ary constant with value n      | k                |
| `pi(k, i)`              | k-ary projection on the i-th arg | k (needs 1≤i≤k)  |
| `f(g1, ..., gm)`        | composition                      | arity of the gᵢ  |
| `rec(g, h)`             | primitive recursion              | arity(g) + 1     |
| `\|f`                   | μ-operator (minimization)        | arity(f) − 1     |
| `name`                  | a previously defined function    | its arity        |
| `( expr )`              | grouping                         |                  |

Comments are `// to end of line` and `/* block */`; block comments nest.

Notes:

- `|` binds weaker than composition: `|f(g, h)` is `|(f(g, h))`. Use parentheses to
  compose *after* minimizing: `(|f)(g, h)`.
- A parenthesized list of **numbers** after an expression evaluates it; a list of
  **functions** composes. `s(pi(3, 1))` is a function, `s(5)` evaluates to `6`.
- Names are resolved when a definition is read, so a function can only use functions
  defined before it — there is no hidden general recursion. Redefining a name is
  allowed; earlier definitions keep using the old meaning.
- `import` paths are relative to the directory of the importing file. Each file is
  loaded at most once, so importing the same library twice (or cyclic imports) is fine.
- `s`, `c`, `pi`, `rec` and `import` are reserved. Identifiers consist of letters, digits and `_`.

Formally:

```text
program    = { statement } ;
statement  = "import" STRING ";"
           | IDENT "=" expr ";"
           | expr "(" [ NUMBER { "," NUMBER } ] ")" ";"
           | expr ";" ;
expr       = "|" expr | postfix ;
postfix    = primary { "(" expr { "," expr } ")" } ;
primary    = "s" | "c" "(" NUMBER "," NUMBER ")" | "pi" "(" NUMBER "," NUMBER ")"
           | "rec" "(" expr "," expr ")" | IDENT | "(" expr ")" ;
```

## The building blocks, mathematically

All functions map tuples of natural numbers to a natural number,
$f : \mathbb{N}^k \to \mathbb{N}$, where $k$ is the **arity**. Every expression has a
fixed arity, which the interpreter checks before anything is evaluated. Below,
$\vec{x} = (x_1, \dots, x_k)$.

The *primitive recursive* functions are the smallest class containing the basic
functions (1–3) and closed under composition (4) and primitive recursion (5). Adding
the μ-operator (6) gives the *μ-recursive* functions, which are exactly the
WHILE-computable (= Turing-computable) functions.

### 1. Successor `s`

$$
s : \mathbb{N} \to \mathbb{N}, \qquad s(x) = x + 1
$$

The only basic function that produces something "new": every number is
$s(s(\cdots s(0)))$. Example: `s(41);` → `42`.

### 2. Constants `c(k, n)`

$$
c^k_n : \mathbb{N}^k \to \mathbb{N}, \qquad c^k_n(x_1, \dots, x_k) = n
$$

The constant function with value $n$ that ignores its $k$ arguments. The arity matters
because composition and recursion require matching arities; e.g. the base case of
`mult = rec(c(1, 0), ...)` must be a *1-ary* zero. `c(0, n)` is a 0-ary function,
i.e. simply a number: `c(0, 5)();` → `5`.

(Strictly speaking, constants can be built from $c^0_0$ and $s$, but having them
built-in is convenient.)

### 3. Projections `pi(k, i)`

$$
\pi^k_i : \mathbb{N}^k \to \mathbb{N}, \qquad \pi^k_i(x_1, \dots, x_k) = x_i
\qquad (1 \le i \le k)
$$

Projections select one argument. Together with composition they let you reorder,
duplicate and drop arguments (the lecture's "Vertauschen", "Identifikation" and
"fiktive Variablen"):

```text
swap(x, y)  = f(y, x)      → f(pi(2, 2), pi(2, 1))
double(x)   = f(x, x)      → f(pi(1, 1), pi(1, 1))
drop(x, y)  = f(x)         → f(pi(2, 1))
```

`pi(1, 1)` is the identity.

### 4. Composition `f(g1, ..., gm)`

Given $f : \mathbb{N}^m \to \mathbb{N}$ and $g_1, \dots, g_m : \mathbb{N}^k \to \mathbb{N}$
(*all with the same arity $k$*):

$$
f \circ (g_1, \dots, g_m) : \mathbb{N}^k \to \mathbb{N}, \qquad
\vec{x} \mapsto f\big(g_1(\vec{x}), \dots, g_m(\vec{x})\big)
$$

This is the lecture's *Einsetzen*. Each $g_i$ receives the full argument vector
$\vec{x}$, so the $g_i$ typically are projections or built from projections:

```text
// h(z, n, x) = s(z)  — successor applied to the first of three arguments
s(pi(3, 1))

// absdiff(x, y) = (x -' y) + (y -' x)
absdiff = add(sub(pi(2, 1), pi(2, 2)), sub(pi(2, 2), pi(2, 1)));
```

### 5. Primitive recursion `rec(g, h)`

Given $g : \mathbb{N}^k \to \mathbb{N}$ and $h : \mathbb{N}^{k+2} \to \mathbb{N}$, the
function $f = \mathrm{rec}(g, h) : \mathbb{N}^{k+1} \to \mathbb{N}$ is

$$
\begin{aligned}
f(0, \vec{x})     &= g(\vec{x}) \\
f(n + 1, \vec{x}) &= h\big(f(n, \vec{x}),\ n,\ \vec{x}\big)
\end{aligned}
$$

The recursion runs over the **first** argument. The step function $h$ receives, in
this order: the previous value $z = f(n, \vec{x})$, the counter $n$, and the remaining
arguments $\vec{x}$. So inside `h` of a `rec` with $k$ extra parameters:

| projection          | refers to            |
|---------------------|----------------------|
| `pi(k+2, 1)`        | previous value $z$   |
| `pi(k+2, 2)`        | counter $n$          |
| `pi(k+2, 2+j)`      | parameter $x_j$      |

Primitive recursion corresponds to a `LOOP` (a `for` loop whose number of iterations
is fixed in advance), so primitive recursive functions are always **total**. The
interpreter evaluates it exactly like the LOOP program from the lecture, bottom-up and
without recursion on the call stack:

```text
z := g(x);  FOR i := 0 TO n-1 DO z := h(z, i, x) END;  return z
```

Example — addition, $\mathrm{add}(n, x) = n + x$:

$$
\mathrm{add}(0, x) = x = \pi^1_1(x), \qquad
\mathrm{add}(n+1, x) = s(\mathrm{add}(n, x)) = (s \circ \pi^3_1)(z, n, x)
$$

```text
add = rec(pi(1, 1), s(pi(3, 1)));
add(5, 7);      // add(5,7) = s(add(4,7)) = ... = s(s(s(s(s(7))))) = 12
```

More examples (all in [`examples/prelude.pr`](examples/prelude.pr)):

| function | equations | definition |
|---|---|---|
| predecessor | $p(0)=0,\ p(n+1)=n$ | `rec(c(0, 0), pi(2, 2))` |
| $x \dot{-} n$ | $r(0,x)=x,\ r(n+1,x)=p(r(n,x))$ | `rec(pi(1, 1), pred(pi(3, 1)))` |
| $n \cdot x$ | $m(0,x)=0,\ m(n+1,x)=x+m(n,x)$ | `rec(c(1, 0), add(pi(3, 3), pi(3, 1)))` |
| $n!$ | $0!=1,\ (n+1)! = (n+1)\cdot n!$ | `rec(c(0, 1), mult(s(pi(2, 2)), pi(2, 1)))` |
| $\mathrm{sg}$ | $\mathrm{sg}(0)=0,\ \mathrm{sg}(n+1)=1$ | `rec(c(0, 0), c(2, 1))` |
| if-then-else | $\mathrm{cond}(0,x,y)=y,\ \mathrm{cond}(n+1,x,y)=x$ | `rec(pi(2, 2), pi(4, 3))` |

($\dot{-}$ is the *modified difference* $x \dot{-} y = \max(x - y, 0)$.)

### 6. The μ-operator `|f`

Given $f : \mathbb{N}^{k+1} \to \mathbb{N}$, the function
$\mu f : \mathbb{N}^k \to \mathbb{N}$ is

$$
\mu f(\vec{x}) = \min\big\{\, n \in \mathbb{N} \ \big|\ f(n, \vec{x}) = 0
\ \wedge\ \forall m < n : f(m, \vec{x}) \text{ is defined and } > 0 \,\big\}
$$

and **undefined** if no such $n$ exists. It searches over the **first** argument of
$f$ — like a `WHILE` loop: $n := 0$; while $f(n, \vec x) \ne 0$ do $n := n + 1$.

$\mu f$ is undefined at $\vec{x}$ if

- $f(n, \vec{x}) > 0$ for all $n$ (the search never ends), or
- the search reaches some $n$ where $f(n, \vec{x})$ itself is undefined.

In both cases the interpreter does not terminate — which is precisely the
mathematical meaning of "undefined" here, since in general it is undecidable
whether a search will ever succeed (halting problem). This is the essential
difference to primitive recursion: μ-recursive functions can be **partial**.

A typical pattern is to compute the *inverse* of a monotone function. To find the
least $n$ with property $P(n, \vec{x})$, write a function that is $0$ exactly when $P$
holds. With $\mathrm{leq}(a, b) = 1$ iff $a \le b$ (else 0):

$$
x \mathrm{div} y = \mu n\,\big[\,(n+1)\cdot y > x\,\big]
= \mu\big(\mathrm{leq}((n+1) \cdot y,\ x)\big)
$$

```text
mu_div = |leq(mult(s(pi(3, 1)), pi(3, 3)), pi(3, 2));
mu_div(17, 5);    // 3
mu_div(7, 0);     // never terminates: (n+1)·0 > 7 is never true
```

The example from the lecture: for $g(x, y, z) = y \dot{-} (s(x) \cdot z)$ we get
$g(0,17,5) = 12,\ g(1,17,5) = 7,\ g(2,17,5) = 2,\ g(3,17,5) = 0$, so
$\mu g(17, 5) = 3$:

```text
g = sub(pi(3, 2), mult(s(pi(3, 1)), pi(3, 3)));
|g(17, 5);        // 3
```

And for the 2-ary constant $c^2_2$, $\mu c^2_2$ is the nowhere-defined 1-ary function.

**Kleene's normal form theorem** says that one μ is always enough: every μ-recursive
$f$ can be written as $f(\vec{x}) = p(\vec{x}, \mu q(\vec{x}))$ with primitive recursive
$p$ and $q$.

## Examples

| File | Run with | Shows |
|---|---|---|
| [`basics.pr`](examples/basics.pr) | `mu-interp examples/basics.pr` | each building block on its own |
| [`prelude.pr`](examples/prelude.pr) | (library, imported by the others) | arithmetic, predicates, `cond`, `max`/`min`, `div`/`mod` |
| [`arithmetic.pr`](examples/arithmetic.pr) | `mu-interp examples/arithmetic.pr` | using the prelude |
| [`minimization.pr`](examples/minimization.pr) | `mu-interp examples/minimization.pr` | μ: division, √, log₂, gcd, primes, undefinedness |
| [`pairing.pr`](examples/pairing.pr) | `mu-interp examples/pairing.pr` | Cantor pairing, its inverse, Fibonacci |

A few highlights:

**Integer square root.** $\lfloor\sqrt{x}\rfloor = \mu n\,[(n+1)^2 > x]$

```text
square = mult(id, id);
isqrt  = |leq(square(s(pi(2, 1))), pi(2, 2));
isqrt(99);          // 9
```

**Greatest common divisor.** Search *downwards* from $x + y$: the first
$d = (x+y) \dot{-} k$ that divides both $x$ and $y$ is the gcd.

```text
gcd_miss = or(mod(pi(3, 2), sub(add(pi(3, 2), pi(3, 3)), pi(3, 1))),
              mod(pi(3, 3), sub(add(pi(3, 2), pi(3, 3)), pi(3, 1))));
gcd = sub(add(pi(2, 1), pi(2, 2)), |gcd_miss);
gcd(12, 18);        // 6
```

**Next prime.** Total, even though it uses an unbounded search — there are
infinitely many primes:

```text
count_div  = rec(c(1, 0), add(pi(3, 1), divides(s(pi(3, 2)), pi(3, 3))));
prime      = eq(count_div(id, id), c(1, 2));       // exactly 2 divisors
next_prime = |not(and(gt(pi(2, 1), pi(2, 2)), prime(pi(2, 1))));
next_prime(20);     // 23
```

**Cantor pairing** (TI2, unit 10). The bijection
$\langle x, y\rangle = \binom{x+y+1}{2} + x$ and its inverses:

```text
pair     = add(binom2(s(add(pi(2, 1), pi(2, 2)))), pi(2, 1));
diagonal = |leq(binom2(s(s(pi(2, 1)))), pi(2, 2));   // x + y
unpair1  = sub(id, binom2(s(diagonal)));             // x
unpair2  = sub(diagonal, unpair1);                   // y
pair(3, 4);                     // 31
pair(unpair1, unpair2)(31);     // 31
```

With pairing, primitive recursion can carry two values at once, which gives
Fibonacci: $(a, b) \mapsto (b, a + b)$ packed into a single number.

```text
fib_pair = rec(c(0, 1), pair(unpair2(pi(2, 1)), add(unpair1(pi(2, 1)), unpair2(pi(2, 1)))));
fib = unpair1(fib_pair);
fib(8);             // 21
```

## Errors

All arities are checked when a statement is read, before anything is evaluated.
Errors report file, line and column; the interpreter then continues with the next
statement:

```text
errors.pr:2:1: error: 'add' expects 2 arguments, got 1
errors.pr:6:15: error: in rec(g, h), h must take 2 more arguments than g: g has arity 1, so h needs arity 3, but has arity 2
errors.pr:8:15: error: all functions in a composition must have the same arity: argument 1 has arity 2, argument 2 has arity 3
errors.pr:9:1: error: unknown function 'unknown'
```

Runtime errors are an overflow beyond $2^{64}-1$ and an evaluation aborted with Ctrl-C.

## Limitations

- Numbers are 64-bit unsigned integers; leaving that range is reported as an error.
- Arithmetic is built from the successor, so it is *unary*: computing $n + x$ with
  `add` takes $n$ steps, $n \cdot x$ takes $n \cdot x$ steps. Results in the millions
  are fine, billions take a while. It is worth recursing on the smaller argument —
  e.g. `mult` in the prelude uses `add(pi(3, 3), pi(3, 1))` so `add` loops over the
  fixed factor instead of the growing accumulator.
- A diverging μ-search runs forever (by definition). Abort with Ctrl-C.

## Project layout and tests

```text
CMakeLists.txt
src/
  main.cpp           command line, REPL
  interpreter.*      runs statements, prints results and errors
  lexer.*            source text -> tokens
  parser.*           tokens -> function terms, arity checking
  function.hpp       the term representation (one struct per building block)
  evaluator.*        evaluation
  diagnostics.hpp    error types
examples/            example programs (see above)
tests/               regression tests, run by ctest
build/               build directory (generated)
```

`ctest` runs every example and test program and compares the output with the
`tests/*.expected` files. `tests/legacy_*.pr` are the test programs of the original Java
version, `tests/regression.pr` covers the bugs listed below, `tests/errors.pr` the error
messages and `tests/imports/` the import rules.

## Changes compared to the Java version

The language is the same, so old programs keep working. Bugs fixed:

1. **μ corrupted the caller's arguments.** `visitMue` pushed its search variable onto
   the *caller's* argument list and never removed it. Any argument evaluated after a μ
   inside a composition saw shifted arguments: with $g$ from the lecture example,
   `add(|g, pi(2, 1))(17, 5)` gave `6` instead of `3 + 17 = 20`.
2. **No arity checking.** The `k` in `pi(k, i)` and `c(k, n)` was ignored. Wrong
   argument counts either crashed with an `IndexOutOfBoundsException` or, worse,
   silently computed something (`add(1, 2, 3)` ignored the 3).
3. **Unknown names** printed a message and then continued with `null`, crashing later
   with a `NullPointerException`; **syntax errors** did the same. Now every error has a
   position and the interpreter recovers at the next `;`.
4. **`s` only worked as `s(expr)`.** `s(5)` crashed and `s` could not be used as a
   function on its own. It is now an ordinary 1-ary function.
5. **32-bit `int`** silently overflowed, and number literals ≥ 2³¹ crashed the scanner.
   Now 64-bit with overflow detection.
6. **Nested block comments** could miss a `/*` directly after a `/` (`//*`).
7. The input file name was hard-coded (`muetest.pr`); there was no CLI.

Streamlining:

- Names are resolved at parse time to shared, immutable terms; evaluation is a single
  `std::visit` without the explicit argument stack.
- Primitive recursion and μ run as loops, so large inputs cannot overflow the call stack.
- Composition and evaluation are distinguished by what is in the parentheses (functions
  vs. numbers) instead of parse-and-backtrack.
- `import`, signature queries (`add;`), grouping parentheses, a REPL, multiple input files,
  `-e`, Ctrl-C to abort a search.

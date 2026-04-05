# PicoCAS Specification

PicoCAS is a tiny, AI Agent-generated, symbolic computer algebra system (CAS) inspired by the core architecture of Mathematica. PicoCAS is written in the C programming language. It supports a recursive expression model, pattern matching, rewriting rules, and a small library of built-in functions.

The name, PicoCAS is inspired by David Stoutemyer's PICOMATH-80 tiny computer algebra system. 

## 1. Expression Structure

Everything in PicoCAS is an `Expr`, which can be one of the following types:
- **Integer**: 64-bit signed integers (e.g., `123`, `-45`).
- **Real**: Double-precision floating-point numbers (e.g., `3.14`, `1.2E-3`).
- **Rational**: Exact fractions of integers, represented as `Rational[n, d]`.
- **Complex**: Complex numbers, represented as `Complex[re, im]`. The symbol `I` represents `Complex[0, 1]`.
- **Symbol**: Named identifiers (e.g., `x`, `Plus`, `Pi`, `Infinity`, `E`).
- **String**: Character sequences enclosed in double quotes (e.g., `"hello"`).
- **Function**: Compound expressions consisting of a `head` and zero or more `arguments` (e.g., `f[x, y]`, `{1, 2, 3}`).

## 2. Core Built-in Functions

### Structural Manipulation

#### Part
Extracts subparts of an expression.
- `expr[[i]]` or `Part[expr, i]`: Extracts the $i$-th element.
- `expr[[i, j, ...]]` or `Part[expr, i, j, ...]]`: Extracts nested parts.
- `expr[[{i, j, ...}]]`: Extracts a list of specific parts.
- `expr[[All]]`: Represents all elements at a given level.

**Features**: 
- Supports negative indices to count from the end (`-1` is the last element).
- `expr[[0]]` returns the `Head` of the expression. This is permitted even for atomic expressions.
- Mapping `All` across a dimension allows column extraction from matrices.

```mathematica
In[1]:= {a, b, c, d}[[2]]
Out[1]= b

In[2]:= {a, b, c, d}[[-1]]
Out[2]= d

In[3]:= 123[[0]]
Out[3]= Integer
```

#### Span
- `i;;j`: Represents a span of elements `i` through `j`.
- `i;;`: Represents a span from `i` to the end.
- `;;j`: Represents a span from the beginning to `j`.
- `;;`: Represents a span that includes all elements.
- `i;;j;;k`: Represents a span from `i` through `j` in steps of `k`.
- `i;;;;k`: Represents a span from `i` to the end in steps of `k`.
- `;;j;;k`: Represents a span from the beginning to `j` in steps of `k`.
- `;;;;k`: Represents a span from the beginning to the end in steps of `k`.

**Features**:
- `m[[i;;j;;k]]` is equivalent to `Take[m, {i, j, k}]` but evaluated natively within `Part`.
- `m[[i;;j]] = v` can be used to assign `v` iteratively over a span of elements. If `v` is a list, elements are assigned sequentially. If `v` is a non-list expression, it is assigned uniformly to all elements in the span.
- When used in `Part`, negative `i` and `j` count from the end.
- `i` and `j` can be of the form `UpTo[n]` to restrict endpoints to the actual length of the list.
- Any argument of `Span[...]` can be `All`.

```mathematica
In[1]:= {a, b, c, d, e, f, g, h}[[2;;5]]
Out[1]= {b, c, d, e}

In[2]:= {a, b, c, d, e, f, g, h}[[1;;-1;;3]]
Out[2]= {a, d, g}

In[3]:= t = {a, b, c, d, e, f, g, h}; t[[2;;5]] = x; t
Out[3]= {a, x, x, x, x, f, g, h}

In[4]:= t = {a, b, c, d, e, f, g, h}; t[[2;;5]] = {p, q, r, s}; t
Out[4]= {a, p, q, r, s, f, g, h}

In[5]:= Range[10][[3;;All]]
Out[5]= {3, 4, 5, 6, 7, 8, 9, 10}
```

#### Head
Returns the top-level wrapper of an expression.
- `Head[expr]`

**Features**: 
- For functions, returns the symbol or expression acting as the head.
- For atoms, returns the symbolic type name: `Integer`, `Real`, `Rational`, `Complex`, `Symbol`, or `String`.

```mathematica
In[1]:= Head[f[x]]
Out[1]= f

In[2]:= Head[3/4]
Out[2]= Rational
```

#### Length
Returns the number of arguments in an expression.
- `Length[expr]`

**Features**: 
- Returns the count of top-level arguments for functions.
- Returns `0` for all atomic expressions.

```mathematica
In[1]:= Length[{a, b, c}]
Out[1]= 3
```

#### Dimensions
Returns the list of lengths of levels in a nested rectangular structure.
- `Dimensions[expr]`

```mathematica
In[1]:= Dimensions[{{1, 2}, {3, 4}}]
Out[1]= {2, 2}
```

#### First, Last, Most, Rest
Convenience functions for accessing parts of a sequence.
- `First[expr]`: Equivalent to `expr[[1]]`.
- `Last[expr]`: Equivalent to `expr[[-1]]`.
- `Most[expr]`: Returns all elements except the last.
- `Rest[expr]`: Returns all elements except the first.

#### Reverse
Reverses the order of elements.
- `Reverse[expr]`: Reverses top-level elements.
- `Reverse[expr, n]`: Reverses elements at level `n`.
- `Reverse[expr, {n1, n2, ...}]`: Reverses at levels `n1`, `n2`, etc.

#### RotateLeft, RotateRight
Cycles elements.
- `RotateLeft[expr, n]`: Cycles `n` positions to the left.
- `RotateLeft[expr]`: Cycles 1 position to the left.
- `RotateLeft[expr, {n1, n2, ...}]`: Cycles at successive levels.
- `RotateRight[...]`: Similar, but cycles to the right.

#### Transpose
Transposes levels in a rectangular array.
- `Transpose[list]`: Transposes the first two levels.
- `Transpose[list, {n1, n2, ...}]`: Level `k` becomes level `nk` in result.

**Features**:
- `Protected`.
- Works only on rectangular arrays.
- `Transpose[m, {1, 1}]` extracts the diagonal of a square matrix.

```mathematica
In[1]:= Transpose[{{a, b}, {c, d}}]
Out[1]= {{a, c}, {b, d}}

In[2]:= Transpose[{{a, b}, {c, d}}, {1, 1}]
Out[2]= {a, d}
```

#### Take, Drop

In[1]:= Reverse[{a, b, c}]
Out[1]= {c, b, a}

In[2]:= RotateLeft[{a, b, c}, 1]
Out[2]= {b, c, a}

In[3]:= RotateRight[{a, b, c}, 1]
Out[3]= {c, a, b}
```

#### Take, Drop

In[1]:= Most[{a, b, c}]
Out[1]= {a, b}

In[2]:= Rest[f[x, y]]
Out[2]= f[y]
```

#### Take, Drop
Extract or remove sequences of elements.
- `Take[expr, spec]`: Extracts elements according to `spec`.
- `Drop[expr, spec]`: Removes elements according to `spec`.

#### Flatten
Flattens out nested lists.
- `Flatten[list]`: Flattens all levels.
- `Flatten[list, n]`: Flattens up to level `n`.
- `Flatten[list, n, h]`: Flattens subexpressions with head `h`.

#### Partition
Partitions a list into sublists.
- `Partition[list, n]`: Non-overlapping sublists of length `n`.
- `Partition[list, n, d]`: Sublists with offset `d`.
- `Partition[list, {n1, n2, ...}]`: Multi-level partitioning.
- `Partition[list, spec, dspec]`: Multi-level partitioning with offsets.
- `Partition[list, UpTo[n]]`: Allows shorter final sublist.

**Features**:
- `Protected`.
- Works on any expression with arguments.
- `Partition[list, n, d]` only includes full sublists of length `n` unless `UpTo` is used.

```mathematica
In[1]:= Partition[{a, b, c, d, e}, 2]
Out[1]= {{a, b}, {c, d}}

In[2]:= Partition[{a, b, c, d, e}, 2, 1]
Out[2]= {{a, b}, {b, c}, {c, d}, {d, e}}

In[3]:= Partition[{a, b, c, d, e}, UpTo[2]]
Out[3]= {{a, b}, {c, d}, {e}}

In[4]:= Partition[{{1, 2, 3}, {4, 5, 6}}, {2, 2}]
Out[4]= {{{{1, 2}, {4, 5}}}}
```

#### Split
Splits a list into sublists of identical adjacent elements.
- `Split[list]`
- `Split[list, test]`

**Features**:
- `Protected`.
- Uses `SameQ` as the default test.
- Result has the same head as the input.

```mathematica
In[1]:= Split[{a, a, a, b, b, a, a, c}]
Out[1]= {{a, a, a}, {b, b}, {a, a}, {c}}

In[2]:= Split[{1, 2, 3, 4, 3, 2, 1}, Less]
Out[2]= {{1, 2, 3, 4}, {3}, {2}, {1}}
```

#### Sort
Sorts elements of an expression into canonical order.
- `Sort[list]`
- `Sort[list, p]`

**Features**:
- `Protected`.
- Uses an efficient quicksort algorithm.
- Canonical order:
    - Real numbers by numerical value.
    - Complex numbers by real part, then imaginary part magnitude.
    - Strings in dictionary order (lowercase before uppercase).
    - Symbols by name.
    - Expressions by length, then head, then parts depth-first.
- Polynomial order: `x^n` sorts relative to `x`.
- Custom ordering function `p` can return `1`, `0`, `-1`, `True`, or `False`.

```mathematica
In[1]:= Sort[{d, b, c, a}]
Out[1]= {a, b, c, d}

In[2]:= Sort[{Pi, E, 2, 3, 1, Sqrt[2]}, Less]
Out[2]= {1, Sqrt[2], 2, E, 3, Pi}
```

#### Union
Gives a sorted list of all distinct elements from one or more expressions.
- `Union[list]`
- `Union[list1, list2, ...]`
- `Union[..., SameTest -> test]`

**Features**:
- `Flat`, `OneIdentity`, `Protected`, `ReadProtected`.
- All expressions must have the same head.
- Result has the same head as the inputs.

```mathematica
In[1]:= Union[{1, 2, 1, 3, 6, 2, 2}]
Out[1]= {1, 2, 3, 6}

In[2]:= Union[{a, b, a, c}, {d, a, e, b}, {c, a}]
Out[2]= {a, b, c, d, e}
```

#### DeleteDuplicates
Removes duplicate elements while preserving order.
- `DeleteDuplicates[list]`
- `DeleteDuplicates[list, test]`

**Features**:
- `Protected`.
- Preserves the order of first occurrences.

```mathematica
In[1]:= DeleteDuplicates[{a, a, b, a, c, b, a}]
Out[1]= {a, b, c}
```

#### Tally
Counts occurrences of elements.
- `Tally[list]`
- `Tally[list, test]`

**Features**:
- `Protected`.
- Returns a list of `{element, count}` pairs.
- Elements appear in the order of their first occurrence.

```mathematica
In[1]:= Tally[{a, a, b, a, c, b, a}]
Out[1]= {{a, 4}, {b, 2}, {c, 1}}
```

#### Min, Max
Returns the numerically smallest or largest elements.
- `Min[x1, x2, ...]`
- `Max[x1, x2, ...]`
- `Min[{x1, x2, ...}, {y1, ...}, ...]`

**Features**:
- `Flat`, `NumericFunction`, `OneIdentity`, `Orderless`, `Protected`.
- Flattens `List` arguments.
- `Min[]` returns `Infinity`.
- `Max[]` returns `-Infinity`.
- Handles `Infinity` and `-Infinity`.
- Simplifies numeric arguments to a single value.

```mathematica
In[1]:= Min[9, 2]
Out[1]= 2

In[2]:= Min[{4, 1, 7, 2}]
Out[2]= 1

In[3]:= Max[Infinity, 5]
Out[3]= Infinity
```

#### Append, Prepend

- `Protected`.
- Default head to flatten is `List`.
- $n$ must be a non-negative integer.

```mathematica
In[1]:= Flatten[{{a, b}, {c, {d, e}}}]
Out[1]= {a, b, c, d, e}

In[2]:= Flatten[{{a, b}, {c, {d, e}}}, 1]
Out[2]= {a, b, c, {d, e}}

In[3]:= Flatten[f[f[a], b], -1, f]
Out[3]= f[a, b]
```

#### Append, Prepend

- `n`: first `n` elements.
- `-n`: last `n` elements.
- `{m, n}`: elements from `m` to `n`.
- `{m, n, s}`: elements from `m` to `n` with step `s`.
- `UpTo[n]`: up to `n` elements as available.

```mathematica
In[1]:= Take[{a, b, c, d}, 2]
Out[1]= {a, b}

In[2]:= Drop[{a, b, c, d}, -1]
Out[2]= {a, b, c}
```

#### Append, Prepend
Adds an element to the end or beginning of an expression.
- `Append[expr, elem]`
- `Prepend[expr, elem]`

```mathematica
In[1]:= Append[{a, b}, c]
Out[1]= {a, b, c}
```

#### AppendTo, PrependTo
Updates a variable by appending or prepending an element.
- `AppendTo[symbol, elem]`
- `PrependTo[symbol, elem]`

#### Insert, Delete
Inserts or removes elements at specified positions.
- `Insert[expr, elem, pos]`
- `Delete[expr, pos]`

**Features**:
- `pos` can be a single index, a list (path), or a list of paths.
- `Delete[expr, 0]` replaces the head with `Sequence`.

```mathematica
In[1]:= Insert[{a, b, c}, x, 2]
Out[1]= {a, x, b, c}

In[2]:= Delete[{a, b, c}, 2]
Out[1]= {a, c}
```

#### Depth
Gives the maximum number of indices needed to specify any part of expr, plus 1.
- `Depth[expr]`

**Features**:
- `Protected`.
- Default option: `Heads -> False`.
- Raw objects (atoms) have depth 1.
- Numbers, `Rational`, and `Complex` have depth 1.
- Symbolic constants like `Pi`, `E`, `I` have depth 1.
- Compound expressions have depth `1 + Max(depths of arguments)`.
- With `Heads -> True`, it includes heads of expressions and their parts.

```mathematica
In[1]:= Depth[f[g[h[x]]]]
Out[1]= 4

In[2]:= Depth[1/2]
Out[2]= 1

In[3]:= Depth[h[{{{a}}}][x, y]]
Out[3]= 2

In[4]:= Depth[h[{{{a}}}][x, y], Heads -> True]
Out[4]= 6
```

#### Level
Gives a list of all subexpressions of expr on levels specified by levelspec.
- `Level[expr, levelspec]`
- `Level[expr, levelspec, f]`

**Features**:
- `Protected`.
- Default option: `Heads -> False`.
- Standard level specifications:
  - `n`: levels 1 through `n`.
  - `Infinity`: levels 1 through `Infinity`.
  - `{n}`: level `n` only.
  - `{n1, n2}`: levels `n1` through `n2`.
- Positive level `n` refers to distance from the top (level 0 is the whole expression).
- Negative level `-n` refers to distance from the bottom (depth `n`).
- Level `-1` corresponds to atomic objects.
- Lists subexpressions in post-order (depth-first), resulting in lexicographic ordering of indices.

```mathematica
In[1]:= Level[a + f[x, y^n], {-1}]
Out[1]= {a, x, y, n}

In[2]:= Level[a + f[x, y^n], 2]
Out[2]= {a, x, y^n, f[x, y^n]}

In[3]:= Level[x^2 + y^3, 3, Heads -> True]
Out[3]= {Plus, Power, x, 2, x^2, Power, y, 3, y^3}
```

#### Variables
Gives an ordered list of all independent variables in a polynomial.
- `Variables[poly]`

**Features**:
- `Protected`.
- Looks for variables only inside `Plus`, `Times`, and `Power` with rational exponents.
- Returns a sorted `List` of variables.
- Symbolic constants like `Pi`, `E`, and `I` are not treated as variables.

```mathematica
In[1]:= Variables[(x + y)^2 + 3 z^2 - y z + 7]
Out[1]= {x, y, z}

In[2]:= Variables[Sin[x] + Cos[x]]
Out[2]= {Cos[x], Sin[x]}

In[3]:= Variables[E^x]
Out[3]= {}
```

#### Expand
Expands out products and positive integer powers in an expression.
- `Expand[expr]`
- `Expand[expr, patt]`

**Features**:
- `Protected`.
- Works only on positive integer powers and distributes products over sums.
- Threads over equations, inequalities, and lists.
- Implements an efficient binary-splitting algorithm for distributing products and repeated squaring for powers.
- `Expand[expr, patt]` leaves unexpanded any parts of `expr` that are free of the pattern `patt`.

```mathematica
In[1]:= Expand[(x+3)(x+2)]
Out[1]= 6 + 5 x + x^2

In[2]:= Expand[(x+y)^2 (x-y)^2]
Out[2]= x^4 - 2 x^2 y^2 + y^4

In[3]:= Expand[(x+1)^2 + (y+1)^2, x]
Out[3]= 1 + 2x + x^2 + (1+y)^2
```

#### Coefficient
Gives the coefficient of a specific form in a polynomial.
- `Coefficient[expr, form]`
- `Coefficient[expr, form, n]`

**Features**:
- `Protected`, `Listable`.
- `Coefficient[expr, form, 0]` picks out terms that do NOT contain `form`.
- Works whether or not `expr` is explicitly given in expanded form (it automatically expands internally).
- Treats distinct transcendental powers as algebraically unrelated (e.g., `x^s` is treated as a separate base from `x`).

```mathematica
In[1]:= Coefficient[(x+1)^3, x, 2]
Out[1]= 3

In[2]:= Coefficient[(x+y)^4, x y^3]
Out[2]= 4

In[3]:= Coefficient[x^s x, x^s]
Out[1]= x
```

#### CoefficientList
Gives a list of coefficients of powers of variables in a polynomial.
- `CoefficientList[poly, var]`
- `CoefficientList[poly, {var1, var2, ...}]`

**Features**:
- `Protected`.
- Gives an array of coefficients starting with power 0.
- Returns a full rectangular array for multiple variables. Combinations of powers that do not appear in `poly` give zeros in the array.
- Automatically expands the polynomial internally.

```mathematica
In[1]:= CoefficientList[1 + 6 x - x^4, x]
Out[1]= {1, 6, 0, 0, -1}

In[2]:= CoefficientList[(1 + x)^10, x]
Out[2]= {1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1}

In[3]:= CoefficientList[1 + a x^2 + b x y + c y^2, {x, y}]
Out[3]= {{1, 0, c}, {0, b, 0}, {a, 0, 0}}
```

#### Collect
Collects together terms involving the same powers of objects matching a variable or variables.
- `Collect[expr, x]`
- `Collect[expr, {x1, x2, ...}]`
- `Collect[expr, var, h]`

**Features**:
- `Protected`.
- Automatically threads over lists, equations, inequalities, and logic functions.
- Effectively writes `expr` as a polynomial in `x` or a fractional power of `x`.
- `Collect[expr, var, h]` applies `h` to the expression that forms the coefficient of each term obtained.

```mathematica
In[1]:= Collect[a x + b y + c x + d y, x]
Out[1]= b y + d y + (a + c) x

In[2]:= Collect[a x^4 + b x^4 + 2 a^2 x - 3 b x + x - 7, x]
Out[2]= -7 + (1 + 2 a^2 - 3 b) x + (a + b) x^4

In[3]:= Collect[a Sqrt[x] + Sqrt[x] + x^(2/3) - c x + 3x - 2b x^(2/3) + 5, x]
Out[3]= 5 + (1 + a) Sqrt[x] + (1 - 2b) x^(2/3) + (3 - c) x
```

#### FactorSquareFree
Pulls out any multiple factors in a polynomial using Yun's algorithm.
- `FactorSquareFree[poly]`

**Features**:
- `Listable`, `Protected`.
- Automatically threads over lists, as well as equations, inequalities and logic functions.
- Works on both univariate and multivariate polynomials.

```mathematica
In[1]:= FactorSquareFree[x^5 - x^3 - x^2 + 1]
Out[1]= (-1 + x)^2 (1 + 2 x + 2 x^2 + x^3)

In[2]:= FactorSquareFree[x^4 - 9x^3 + 29x^2 - 39x + 18]
Out[2]= (-3 + x)^2 (2 - 3 x + x^2)

In[3]:= FactorSquareFree[x^5 - x^3 y^2 - x^2 y^3 + y^5]
Out[3]= (x - y)^2 (x^3 + 2 x^2 y + 2 x y^2 + y^3)

In[4]:= FactorSquareFree[{(x^2 - 1)(x - 1), (x^4 - 1)(x^2 - 1)}]
Out[4]= {(-1 + x)^2 (1 + x), (-1 + x^2)^2 (1 + x^2)}
```

#### Factor
Factors a polynomial over the integers.
- `Factor[poly]`

**Features**:
- `Listable`, `Protected`.
- When given a rational expression, first resolves dependencies over `Together` before factoring.
- Uses exact root isolation (Rational Root Theorem limits) and binomial descents structured identically to Zassenhaus recombination, evaluating combinations exact and memory safe.
- Threads natively across lists, logic structures, and numeric groupings perfectly.

```mathematica
In[1]:= Factor[1 + 2x + x^2]
Out[1]= (1 + x)^2

In[2]:= Factor[x^10 - 1]
Out[2]= (-1 + x) (1 + x) (1 - x + x^2 - x^3 + x^4) (1 + x + x^2 + x^3 + x^4)

In[3]:= Factor[x^10 - y^10]
Out[3]= (x - y) (x + y) (x^4 - x^3 y + x^2 y^2 - x y^3 + y^4) (x^4 + x^3 y + x^2 y^2 + x y^3 + y^4)

In[4]:= Factor[2x^3 y - 2a^2 x y - 3a^2 x^2 + 3a^4]
Out[4]= (a - x) (a + x) (3 a^2 - 2 x y)

In[5]:= Factor[(x^3 + 2x^2)/(x^2 - 4y^2) - (x + 2)/(x^2 - 4y^2)]
Out[5]= ((-1 + x) (1 + x) (2 + x)) / ((x - 2 y) (x + 2 y))
```

#### PolynomialGCD
Gives the greatest common divisor of the polynomials.
- `PolynomialGCD[poly1, poly2, ...]`

**Features**:
- `Protected`, `Listable`.
- Handles univariate and multivariate polynomials.
- Treats algebraic numbers (like `I`) as independent variables or constants seamlessly during complex arithmetic evaluations.
- Pre-extracts common factors before falling back to a full primitive Euclidean algorithm computation.

```mathematica
In[1]:= PolynomialGCD[(1+x)^2(2+x)(4+x), (1+x)(2+x)(3+x)]
Out[1]= (1+x) (2+x)

In[2]:= PolynomialGCD[x^2+4x+4, x^2+2x+1]
Out[2]= 1

In[3]:= PolynomialGCD[x^2-1, x^3-1, x^4-1, x^5-1, x^6-1, x^7-1]
Out[3]= -1 + x
```

#### PolynomialLCM
Gives the least common multiple of the polynomials.
- `PolynomialLCM[poly1, poly2, ...]`

**Features**:
- `Protected`, `Listable`.
- Handles univariate and multivariate polynomials.
- Treats algebraic numbers (like `I`) as independent variables or constants seamlessly during complex arithmetic evaluations.
- Preserves explicit factored forms where possible.

```mathematica
In[1]:= PolynomialLCM[(1+x)^2(2+x)(4+x), (1+x)(2+x)(3+x)]
Out[1]= (1+x)^2 (2+x) (3+x) (4+x)

In[2]:= PolynomialLCM[x^4-4, x^4+4 x^2+4]
Out[2]= (-2+x^2) (4+4 x^2+x^4)
```

#### PolynomialQuotient
Gives the quotient of $p$ and $q$, treated as polynomials in $x$, with any remainder dropped.
- `PolynomialQuotient[p, q, x]`

**Features**:
- `Protected`.
- Uses polynomial long division over the field of rational functions in the coefficients.

```mathematica
In[1]:= PolynomialQuotient[x^4+2x+1, x^2+1, x]
Out[1]= -1 + x^2

In[2]:= PolynomialQuotient[x^2+2x+1, x^3, x]
Out[2]= 0

In[3]:= PolynomialQuotient[x^2+x+1, 2x+1, x]
Out[3]= 1/4 + x/2
```

#### PolynomialRemainder
Gives the remainder from dividing $p$ by $q$, treated as polynomials in $x$.
- `PolynomialRemainder[p, q, x]`

**Features**:
- `Protected`.
- The degree of the result in $x$ is guaranteed to be smaller than the degree of $q$.
- If the dividend is a multiple of the divisor, then the remainder is zero.

```mathematica
In[1]:= PolynomialRemainder[x^4+2x+1, x^2+1, x]
Out[1]= 2 + 2 x

In[2]:= PolynomialRemainder[x^3, a x+b, x]
Out[2]= -(b^3/a^3)
```

#### PolynomialQ
Tests whether an expression is a polynomial in one or more variables.
- `PolynomialQ[expr, var]`: Yields `True` if `expr` is a polynomial in `var`.
- `PolynomialQ[expr, {var1, var2, ...}]`: Tests whether `expr` is a polynomial in the set of variables.

**Features**:
- `Protected`.
- Variables can be symbols or compound expressions.
- Constants (expressions free of the specified variables) are polynomials of degree 0.
- `Power[base, exp]` is a polynomial if `exp` is a non-negative integer and `base` is a polynomial.

```mathematica
In[1]:= PolynomialQ[x^3 - 2x/y + 3x z, x]
Out[1]= True

In[2]:= PolynomialQ[x^3 - 2x/y + 3x z, y]
Out[2]= False

In[3]:= PolynomialQ[x^2 + a x y^2 - b Sin[c], {x, y}]
Out[3]= True

In[4]:= PolynomialQ[f[a] + f[a]^2, f[a]]
Out[4]= True
```

#### PolynomialMod
Gives the polynomial reduced modulo `m`.
- `PolynomialMod[poly, m]`
- `PolynomialMod[poly, {m1, m2, ...}]`

**Features**:
- `Protected`, `Listable`.
- Reduces a polynomial modulo an integer, another polynomial, or a list of integers/polynomials.
- Always gives a result with minimal degree and leading coefficients.
- Handles rational division mapping perfectly scaling over exact modulo structures dynamically.

```mathematica
In[1]:= PolynomialMod[3x^2+2x+1,2]
Out[1]= 1 + x^2

In[2]:= PolynomialMod[3x^2+2x+1,x^2+1]
Out[2]= -2 + 2 x

In[3]:= PolynomialMod[35x^3+21x^2 y^2-17x y^3+55z-123,19]
Out[3]= 10 + 2 x y^3 + 16 x^3 + 2 x^2 y^2 + 17 z

In[4]:= PolynomialMod[3x^3+21x^2 y^2-7x y^3+55,{2x^2-7,x y-3, 9}]
Out[4]= 1 + 7 x + x^3 + 4 y^2
```

#### Resultant
Computes the resultant of two polynomials.
- `Resultant[poly1, poly2, var]`

**Features**:
- `Protected`, `Listable`.
- Computes the resultant of polynomials `poly1` and `poly2` with respect to the variable `var`.
- The resultant is independent of common roots and vanishes exactly when the polynomials have roots in common.
- Implemented fundamentally over symbolic systems using Sylvester matrix determinants, ensuring full generic expansion over arbitrary constants.
- Automatically preserves multiplicativity (e.g., $Res(A \cdot B, Q) = Res(A, Q) Res(B, Q)$ and $Res(A^k, Q) = Res(A, Q)^k$).

```mathematica
In[1]:= Resultant[x^2 - 2x + 7, x^3 - x + 5, x]
Out[1]= 265

In[2]:= Resultant[x^3 - 5x^2 - 7x + 14, x^3 - 8x^2 + 9x + 58, x]
Out[2]= 0
```

#### Discriminant
Computes the discriminant of the polynomial with respect to the variable.
- `Discriminant[poly, var]`

**Features**:
- `Protected`, `Listable`.
- Computes the discriminant of polynomial `poly` with respect to `var`.
- The discriminant is zero if and only if the polynomial has multiple roots.
- Derived symbolically utilizing the formula $D = \frac{(-1)^{n(n-1)/2}}{a_n} Resultant(P, P', var)$.

```mathematica
In[1]:= Discriminant[a x^2 + b x + c, x]
Out[1]= b^2 - 4 a c

In[2]:= Discriminant[5 x^4 - 3 x + 9, x]
Out[2]= 23273325

In[3]:= Discriminant[(x-1)(x-2)(x-3), x]
Out[3]= 4

In[4]:= Discriminant[(x-1)(x-2)(x-1), x]
Out[4]= 0
```

#### HornerForm
Puts a polynomial or rational function into Horner form.
- `HornerForm[poly]`
- `HornerForm[poly, vars]`
- `HornerForm[poly1/poly2, vars1, vars2]`

**Features**:
- `Protected`.
- Nests multiplications instead of using powers (e.g., $a + x(b + c x)$ instead of $a + bx + cx^2$).
- Identifies variables using `Variables` if not explicitly specified.
- Issues an error and returns unevaluated if the expression is not a polynomial or rational function in the target variables.

```mathematica
In[1]:= HornerForm[11 x^3 - 4 x^2 + 7 x + 2]
Out[1]= 2 + x (7 + x (-4 + 11 x))

In[2]:= HornerForm[a + b x + c x^2, x]
Out[2]= a + x (b + c x)

In[3]:= HornerForm[(11 x^3 - 4 x^2 + 7 x + 2)/(x^2 - 3 x + 1)]
Out[3]= (2 + x (7 + x (-4 + 11 x))) / (1 + x (-3 + x))

In[4]:= HornerForm[1 + x^a, x]
HornerForm::poly: 1+x^a is not a polynomial.
Out[4]= HornerForm[1 + x^a, x]
```

### Expression Information

#### Attributes
Returns the core evaluation attributes assigned to a symbol.
- `Attributes[symbol]`

**Features**: 
- Common attributes include `Flat` (associativity), `Orderless` (commutativity), `Listable` (automatic threading over lists), `HoldFirst`, `HoldRest`, `HoldAll` (evaluation control), and `Protected`.

```mathematica
In[1]:= Attributes[Plus]
Out[1]= {Flat, Listable, NumericFunction, OneIdentity, Orderless}
```

#### AtomQ, NumberQ, IntegerQ
Predicates for testing expression types.
- `AtomQ[expr]`: `True` if the expression has no parts.
- `NumberQ[expr]`: `True` if the expression is a numeric type (Integer, Real, Rational, Complex).
- `IntegerQ[expr]`: `True` if the expression is an Integer.

#### ListQ, VectorQ, MatrixQ
Predicates for testing lists and their structures.
- `ListQ[expr]`: `True` if the head of `expr` is `List`.
- `VectorQ[expr]`: `True` if `expr` is a list, none of whose elements are themselves lists.
- `VectorQ[expr, test]`: `True` if `expr` is a list and `test` yields `True` for each element.
- `MatrixQ[expr]`: `True` if `expr` is a list of lists of the same length, with no deeper lists.
- `MatrixQ[expr, test]`: `True` if `expr` is a matrix and `test` yields `True` for each element.

#### EvenQ, OddQ
- `EvenQ[n]`: `True` if `n` is an even integer.
- `OddQ[n]`: `True` if `n` is an odd integer.

#### Information
Returns a formatted string containing the syntax and description of a symbol.
- `Information[symbol]`
- `?symbol` (shortcut)

```mathematica
In[1]:= ?Range
Out[1]= "Range[n]
	generates the list {1, 2, 3, ..., n}.
Range[n, m]
	generates the list {n, n + 1, ..., m - 1, m}.
Range[n, m, d]
	uses step d."
```

#### FreeQ
Yields `True` if no subexpression in `expr` matches `form`, and yields `False` otherwise.
- `FreeQ[expr, form]`
- `FreeQ[expr, form, levelspec]`

**Features**:
- `Protected`.
- By default, explores levels `{0, Infinity}` and option `Heads -> True` is enabled.
- `form` can be a structural pattern.

```mathematica
In[1]:= FreeQ[{1, 2, 4, 1, 0}, 0]
Out[1]= False

In[2]:= FreeQ[{a, b, b, a, a, a}, _Integer]
Out[2]= True

In[3]:= f[c_ x_, x_] := c f[x, x] /; FreeQ[c, x]
In[4]:= {f[3 x, x], f[a x, x], f[(1 + x) x, x]}
Out[4]= {3 f[x, x], a f[x, x], f[x (1 + x), x]}
```

#### LeafCount
Gives the total number of indivisible subexpressions in an expression.
- `LeafCount[expr]`

**Features**:
- `Protected`.
- Counts the number of subexpressions in `expr` that correspond to "leaves" on the expression tree.
- By default `Heads -> True` includes the head of expressions and their parts. With `Heads -> False`, it excludes them.
- Evaluates atoms like `Rational` and `Complex` based on their structural representation as functions.

```mathematica
In[1]:= LeafCount[1 + a + b^2]
Out[1]= 6

In[2]:= LeafCount[f[a, b][x, y]]
Out[2]= 5

In[3]:= LeafCount[{1/2, 1 + I}]
Out[3]= 7
```

#### ByteCount
Gives the number of bytes used internally by PicoCAS to store the expression.
- `ByteCount[expr]`

**Features**:
- `Protected`.
- Uses `sizeof()` in C and measures the internal AST memory allocation boundaries, dynamically capturing sizes of individual strings, symbols, allocated blocks, arrays, and expression structs.

### Time and Date

#### Timing
Evaluates `expr` and returns a list of the time in seconds used, together with the result obtained.
- `Timing[expr]`

**Features**:
- `HoldAll`, `Protected`, `SequenceHold`.
- Returns `{timing, result}`.
- Includes only CPU time spent evaluating the expression.

#### RepeatedTiming
Evaluates `expr` repeatedly and returns a list of the average time in seconds used, together with the result obtained.
- `RepeatedTiming[expr]`
- `RepeatedTiming[expr, t]`

**Features**:
- `HoldFirst`, `Protected`, `SequenceHold`.
- Returns `{average_timing, result}`.
- Does repeated evaluation for at least `t` seconds. Default is 1 second.
- Gives a trimmed mean of the timings obtained, discarding lower and upper quartiles.
- Always evaluates `expr` at least four times.

### Linear Algebra

#### Dot (.)
Gives products of vectors, matrices, and tensors.
- `a . b` or `Dot[a, b]`
- `a . b . c` or `Dot[a, b, c]`

**Features**:
- `Flat`, `OneIdentity`, `Protected`.
- Contracts the last index in `a` with the first index in `b`.
- Applying `Dot` to a rank `n` tensor and a rank `m` tensor gives a rank `m+n-2` tensor.
- Scalar product of two vectors yields a scalar.
- Product of a matrix and a vector yields a vector.
- Product of two matrices yields a matrix.
- When arguments are not lists, `Dot` remains unevaluated.
- Gives an error message `Dot::dotsh` if the shapes of the inputs are not compatible.

```mathematica
In[1]:= {a, b, c} . {x, y, z}
Out[1]= a x + b y + c z

In[2]:= {{a, b}, {c, d}} . {x, y}
Out[2]= {a x + b y, c x + d y}

In[3]:= {{a, b}, {c, d}} . {{1, 2}, {3, 4}}
Out[3]= {{a + 3 b, 2 a + 4 b}, {c + 3 d, 2 c + 4 d}}
```

#### Det
Gives the determinant of a square matrix.
- `Det[m]`

**Features**:
- `Protected`.
- Evaluates the determinant of a square matrix symbolically or numerically using Laplace expansion.
- Returns a warning `Det::matsq` if `m` is not a non-empty square matrix.

```mathematica
In[1]:= Det[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}]
Out[1]= 0

In[2]:= Det[{{1.7, 7.1, -2.7}, {2.2, 8.7, 3.2}, {3.2, -9.2, 1.2}}]
Out[2]= 251.572

In[3]:= Det[{{a, b, c}, {d, e, f}, {g, h, i}}]
Out[3]= -c e g + b f g + c d h - a f h - b d i + a e i
```

#### Cross
Gives the vector cross product of its arguments.
- `Cross[v1, v2, ...]`

**Features**:
- `Protected`.
- Returns the cross product or totally antisymmetric product of $n-1$ vectors of length $n$.
- Works for symbolic and numerical inputs.
- Outputs `Cross::nonn1` error message if inputs are not equal-length vectors or if the number of arguments is not one less than their length.

```mathematica
In[1]:= Cross[{1, 2, -1}, {-1, 1, 0}]
Out[1]= {1, 1, 3}

In[2]:= Cross[{1, Sqrt[3]}]
Out[2]= {-Sqrt[3], 1}

In[3]:= Cross[{3.2, 4.2, 5.2}, {0.75, 0.09, 0.06}]
Out[3]= {-0.216, 3.708, -2.862}

In[4]:= Cross[{1.3 + I, 2, 3 - 2 I}, {6. + I, 4, 5 - 7 I}]
Out[4]= {-2 - 6 I, 6.5 - 4.9 I, -6.8 + 2. I}

In[5]:= Cross[{1, 2, 3}, {4, 5}]
Cross::nonn1: The arguments are expected to be vectors of equal length, and the number of arguments is expected to be 1 less than their length.
Out[5]= Cross[{1, 2, 3}, {4, 5}]
```

#### Norm
Gives the norm of a number, vector, or matrix.
- `Norm[expr]`
- `Norm[expr, p]`

**Features**:
- `Protected`.
- For scalars, `Norm[z]` is `Abs[z]`.
- For vectors, `Norm[v]` defaults to the 2-norm: `Sqrt[v . Conjugate[v]]`.
- For vectors, `Norm[v, p]` is `Total[Abs[v]^p]^(1/p)`.
- For vectors, `Norm[v, Infinity]` is the $\infty$-norm given by `Max[Abs[v]]`.
- `Norm[m, "Frobenius"]` gives the Frobenius norm of a matrix `m`.

```mathematica
In[1]:= Norm[{x, y, z}]
Out[1]= Sqrt[Abs[x]^2 + Abs[y]^2 + Abs[z]^2]

In[2]:= Norm[-2 + I]
Out[2]= Sqrt[5]

In[3]:= v = {1, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1}; Norm[v]
Out[3]= Sqrt[5]

In[4]:= Norm[{x, y, z}, p]
Out[4]= (Abs[x]^p + Abs[y]^p + Abs[z]^p)^(1/p)

In[5]:= Norm[{x, y, z}, Infinity]
Out[5]= Max[Abs[x], Abs[y], Abs[z]]

In[6]:= Norm[{{a11, a12}, {a21, a22}}, "Frobenius"]
Out[6]= Sqrt[Abs[a11]^2 + Abs[a12]^2 + Abs[a21]^2 + Abs[a22]^2]
```

#### Tr
Finds the trace of a matrix or tensor.
- `Tr[list]`
- `Tr[list, f]`
- `Tr[list, f, n]`

**Features**:
- `Protected`.
- `Tr[list]` sums the diagonal elements `list[[i, i, ...]]`.
- `Tr[list, f]` applies the function `f` instead of `Plus`.
- `Tr[list, f, n]` considers elements down to level `n`.
- Works for rectangular as well as square matrices and tensors, stopping at the minimum dimension.
- If `n` is omitted, defaults to the depth of the tensor.

```mathematica
In[1]:= Tr[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}]
Out[1]= 15

In[2]:= Tr[{{a, b}, {c, d}}]
Out[2]= a + d

In[3]:= Tr[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}, List]
Out[3]= {1, 5, 9}

In[4]:= Tr[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}, Plus, 1]
Out[4]= {12, 15, 18}
```

#### RowReduce
Gives the row-reduced form of the matrix `m`.
- `RowReduce[m]`

**Features**:
- `Protected`.
- Uses fraction-free division logic to perform exact algorithmic reduction across numerical, rational, and symbolics expressions natively avoiding division errors.

```mathematica
In[1]:= RowReduce[{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}]
Out[1]= {{1, 0, -1}, {0, 1, 2}, {0, 0, 0}}

In[2]:= RowReduce[{{a, b, c}, {d, e, f}, {a+d, b+e, c+f}}]
Out[2]= {{1, 0, (-b f + c e)/(a e - b d)}, {0, 1, (-a f + c d)/(-a e + b d)}, {0, 0, 0}}
```

#### IdentityMatrix
Generates an identity matrix.
- `IdentityMatrix[n]`: Gives the `n x n` identity matrix.
- `IdentityMatrix[{m, n}]`: Gives the `m x n` identity matrix.

**Features**:
- `Protected`.
- Generates exact integer outputs (`1` on main diagonal, `0` elsewhere).
- Will remain unevaluated if arguments are symbolic or negative.

```mathematica
In[1]:= IdentityMatrix[3]
Out[1]= {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}

In[2]:= IdentityMatrix[{2, 3}]
Out[2]= {{1, 0, 0}, {0, 1, 0}}
```

#### DiagonalMatrix
Generates a matrix with specified elements on a diagonal.
- `DiagonalMatrix[list]`: Elements on leading diagonal.
- `DiagonalMatrix[list, k]`: Elements on `k`-th diagonal.
- `DiagonalMatrix[list, k, n]`: Pads with zeros to create an `n x n` matrix.
- `DiagonalMatrix[list, k, {m, n}]`: Creates an `m x n` matrix.

**Features**:
- `Protected`.
- For `k > 0`, places elements `k` positions above the leading diagonal.
- For `k < 0`, places elements `k` positions below the leading diagonal.
- By default, size is optimally bounded to fit the full array cleanly. Extraneous elements are dropped if manual constraints fall short of required lengths.

```mathematica
In[1]:= DiagonalMatrix[{a, b, c}]
Out[1]= {{a, 0, 0}, {0, b, 0}, {0, 0, c}}

In[2]:= DiagonalMatrix[{a, b}, 1]
Out[2]= {{0, a, 0}, {0, 0, b}, {0, 0, 0}}

In[3]:= DiagonalMatrix[{1, 2, 3}, 0, {3, 5}]
Out[3]= {{1, 0, 0, 0, 0}, {0, 2, 0, 0, 0}, {0, 0, 3, 0, 0}}
```

### Statistics

#### Mean
Gives the mean estimate of the elements in data.
- `Mean[data]`

**Features**:
- `Protected`.
- Supports numerical and symbolic data.
- For vectors, computes $(1/n) \sum x_i$.
- For matrices, computes means of elements in each column.

```mathematica
In[1]:= Mean[{1, 2, 3, 4}]
Out[1]= 5/2

In[2]:= Mean[{{a, u}, {b, v}, {c, w}}]
Out[2]= {1/3 (a + b + c), 1/3 (u + v + w)}
```

#### Variance
Gives the unbiased variance estimate of the elements in data.
- `Variance[data]`

**Features**:
- `Protected`.
- For vectors, computes $(1/(n-1)) \sum (x_i - \hat{\mu}) \overline{(x_i - \hat{\mu})}$.
- For matrices, computes variances of elements in each column.

```mathematica
In[1]:= Variance[{1, 2, 3}]
Out[1]= 1

In[2]:= Variance[{{5.2, 7}, {5.3, 8}, {5.4, 9}}]
Out[2]= {0.01, 1}
```

#### StandardDeviation
Gives the standard deviation estimate of the elements in data.
- `StandardDeviation[data]`

**Features**:
- `Protected`.
- Equivalent to `Sqrt[Variance[data]]`.
- For matrices, computes standard deviations of elements in each column.

```mathematica
In[1]:= StandardDeviation[{1, 2, 3}]
Out[1]= 1
```

### Arithmetic and Algebra

#### Plus (+)
Symbolic sum.
- `a + b + ...`

**Features**:
- `Flat`, `Orderless`, `Listable`.
- Combines numeric constants and collects like terms (e.g., `x + 2x` becomes `3x`).
- Returns `0` if no arguments are provided.
- Returns `Overflow[]` if integer addition overflows or if any argument is `Overflow[]`.

```mathematica
In[1]:= 1 + 2 + x + 2*x
Out[1]= 3 + 3*x
```

#### Total
Gives the total of elements in a list.
- `Total[list]`
- `Total[list, n]`
- `Total[list, {n}]`
- `Total[list, {n1, n2}]`

**Features**:
- `Protected`.
- `Total[list]` is equivalent to `Apply[Plus, list]`.
- `Total[list, n]` totals all elements down to level `n`.
- `Total[list, {n}]` totals elements at level `n` only.
- Supports negative levels to count from the bottom (`-1` is the last dimension).
- Handles ragged arrays correctly by summing from the inside out when multiple levels are specified.
- `Total[list, Infinity]` totals all atoms in the expression.

```mathematica
In[1]:= Total[{a, b, c, d}]
Out[1]= a + b + c + d

In[2]:= Total[{{1, 2}, {3, 4}}]
Out[2]= {4, 6}

In[3]:= Total[{{1, 2}, {3, 4}}, 2]
Out[3]= 10

In[4]:= Total[{{1, 2}, {3}}, 2]
Out[4]= 6
```

#### Times (*)
Symbolic product.
- `a * b * ...`

**Features**:
- `Flat`, `Orderless`, `Listable`.
- Combines numeric constants and groups identical bases into `Power` expressions.
- Handles `I` as `Complex[0, 1]`.
- Returns `1` if no arguments are provided.
- Returns `Overflow[]` if integer multiplication overflows or if any argument is `Overflow[]`.

```mathematica
In[1]:= 2 * x * 3 * y
Out[1]= 6*x*y

In[2]:= I * I
Out[2]= -1
```

#### Power (^)
Exponentiation.
- `base ^ exp`

**Features**:
- `Listable`.
- Simplifies integer powers of integers.
- Returns `Overflow[]` if the result exceeds 64-bit integer limits.
- Reduces radicals (e.g., `8^(1/2)` becomes `2*Sqrt[2]`).
- Supports complex results for negative bases (e.g., `(-1)^(1/2)` becomes `I`).
- Distributes power over product if the exponent is an integer.

```mathematica
In[1]:= Sqrt[45]
Out[1]= 3*Sqrt[5]

In[2]:= (a * b)^2
Out[2]= a^2 * b^2
```

#### Sqrt
Square root.
- `Sqrt[z]`: Internally represented as `Power[z, 1/2]`.

#### Numerator
Gives the numerator of an expression.
- `Numerator[expr]`

**Features**:
- `Protected`, `Listable`.
- Picks out terms which do not have superficially negative exponents.
- Can be used on rational and complex numbers.

```mathematica
In[1]:= Numerator[(x-1)(x-2)/(x-3)^2]
Out[1]= (-2 + x) (-1 + x)

In[2]:= Numerator[3/7 + I/11]
Out[2]= 33 + 7 I
```

#### Denominator
Gives the denominator of an expression.
- `Denominator[expr]`

**Features**:
- `Protected`, `Listable`.
- Picks out terms which have superficially negative exponents.
- Can be used on rational and complex numbers.

```mathematica
In[1]:= Denominator[(x-1)(x-2)/(x-3)^2]
Out[1]= (-3 + x)^2

In[2]:= Denominator[3/7 + I/11]
Out[2]= 77
```

#### Cancel
Cancels out common factors in the numerator and denominator of an expression.
- `Cancel[expr]`

**Features**:
- `Protected`, `Listable`.
- Threads over equations, inequalities, logic functions, and sums dynamically.
- Evaluates greatest common divisors via polynomial GCD derivations avoiding extraneous expansions.

```mathematica
In[1]:= Cancel[(x^2 - 1) / (x - 1)]
Out[1]= 1 + x

In[2]:= Cancel[(x - y)/(x^2 - y^2) + (x^3 - 27)/(x^2 - 9)]
Out[2]= (9 + 3 x + x^2)/(3 + x) + 1/(x + y)
```

#### Together
Puts terms in a sum over a common denominator, and cancels factors in the result.
- `Together[expr]`

**Features**:
- `Protected`, `Listable`.
- Makes a sum of terms into a single rational function.
- Computes lowest common multiples (LCM) of denominators securely without unconditionally destroying pre-factored bases unnecessarily.

```mathematica
In[1]:= Together[a/b + c/d]
Out[1]= (b c + a d) / (b d)

In[2]:= Together[x^2/(x^2 - 1) + x/(x^2 - 1)]
Out[2]= x / (-1 + x)

In[3]:= Together[1/x + 1/(x + 1) + 1/(x + 2) + 1/(x + 3)]
Out[3]= (2 (3 + 11 x + 9 x^2 + 2 x^3)) / (x (1 + x) (2 + x) (3 + x))

In[4]:= Together[x^2/(x - y) - x y/(x - y)]
Out[4]= x
```

#### Apart
Gives the partial fraction decomposition of a rational expression.
- `Apart[expr]`
- `Apart[expr, var]`

**Features**:
- `Protected`, `Listable`.
- Writes `expr` as a polynomial in `var` together with a sum of ratios of polynomials with minimal denominators.
- If `var` is not specified, intelligently selects the main polynomial variable natively.
- Implements exact undetermined coefficients algebraically leveraging row-reduced identity expansions over algebraic inputs avoiding recursive fractional losses natively.

```mathematica
In[1]:= Apart[1/((1+x)(5+x))]
Out[1]= 1/(4 (1 + x)) - 1/(4 (5 + x))

In[2]:= Apart[(x^5-2)/((1+x+x^2)(2+x)(1-x))]
Out[2]= 2 - x + (-1 - x/3)/(-1 - x - x^2) + (4 - (11 x)/3)/(2 - x - x^2)

In[3]:= Apart[(x+y)/((x+1)(y+1)(x-y)), x]
Out[3]= 2 y/((-1 - y)^2 (x - y)) - (-1 + y)/((-1 - y)^2 (1 + x))
```

#### Mod, Quotient, QuotientRemainder
- `Mod[n, m]`: Remainder of `n/m`.
- `Quotient[n, m]`: Integer part of `n/m`.
- `QuotientRemainder[n, m]`: Returns `{Quotient[n, m], Mod[n, m]}`.

#### GCD, LCM
- `GCD[n1, n2, ...]`: Greatest common divisor.
- `LCM[n1, n2, ...]`: Least common multiple.

#### PowerMod
Gives modular exponentiations, inverses, and roots.
- `PowerMod[a, b, m]`: Gives $a^b \pmod m$.
- `PowerMod[a, -1, m]`: Finds the modular inverse of `a` modulo `m`.
- `PowerMod[a, 1/r, m]`: Finds a modular `r`-th root of `a`.

**Features**:
- `Protected`, `Listable`.
- Evaluates much more efficiently than `Mod[a^b, m]`.
- Returns unevaluated if the corresponding inverse or root does not exist.
- Allows threading over lists natively.

```mathematica
In[1]:= PowerMod[2, 10, 3]
Out[1]= 1

In[2]:= PowerMod[3, -2, 7]
Out[2]= 4

In[3]:= PowerMod[3, 1/2, 2]
Out[3]= 1

In[4]:= PowerMod[2, {10, 11, 12, 13, 14}, 5]
Out[4]= {4, 3, 1, 2, 4}
```

#### Factorial (!)
Gives the factorial of an integer or half-integer.
- `n!` or `Factorial[n]`

**Features**:
- `Protected`, `Listable`, `NumericFunction`.
- Evaluates exactly for positive integers up to $20!$.
- Yields `ComplexInfinity` for negative integers.
- Supports half-integers utilizing factors of $\sqrt{\pi}$ recursively.
- Supports trailing `!` parsed natively as a postfix operator.

```mathematica
In[1]:= 5!
Out[1]= 120

In[2]:= (1/2)!
Out[2]= Sqrt[Pi]/2

In[3]:= Factorial[0]
Out[3]= 1
```

#### Binomial
Gives the binomial coefficient $\binom{n}{m}$.
- `Binomial[n, m]`

**Features**:
- `Protected`, `Listable`, `NumericFunction`.
- Evaluates exactly for integers, half-integers, and dynamically factors symbolic terms correctly (e.g. `Binomial[n, 4]`).
- Reduces numerical boundaries logically utilizing continuous `Gamma` interpolations.

```mathematica
In[1]:= Binomial[10, 3]
Out[1]= 120

In[2]:= Binomial[8.5, -4.2]
Out[2]= 0.0000604992

In[3]:= Binomial[9/2, 7/2]
Out[3]= 9/2

In[4]:= Binomial[n, 4]
Out[4]= 1/24 (-3 + n) (-2 + n) (-1 + n) n

In[5]:= Binomial[0, 1]
Out[5]= 0
```

#### PrimeQ
- `PrimeQ[n]`: Returns `True` if `n` is a prime number, `False` otherwise.

#### PrimePi
- `PrimePi[x]`: Returns the number of primes less than or equal to `x`.

**Features**:
- `Listable`, `Protected`.
- Uses Meissel-Lehmer algorithm for efficient counting.

```mathematica
In[1]:= PrimePi[10]
Out[1]= 4

In[2]:= PrimePi[100]
Out[2]= 25

In[3]:= PrimePi[{10, 100}]
Out[3]= {4, 25}
```

#### NextPrime
- `NextPrime[x]`: Smallest prime above `x`.
- `NextPrime[x, k]`: $k$-th next prime above `x` (or previous if $k$ is negative).

**Features**:
- `Protected`, `ReadProtected`.
- Supports negative $k$ for finding previous primes.
- Remains unevaluated if no such prime exists (e.g., `NextPrime[2, -1]`).

#### FactorInteger
- `FactorInteger[n]`: Returns a list of prime factors and their exponents.
- `FactorInteger[n, k]`: Partial factorization, at most `k` distinct factors.
- `FactorInteger[n, Automatic]`: Pulls out easy factors using trial division.

**Features**:
- `Listable`, `Protected`.
- Supports negative integers (includes `{-1, 1}`).
- Supports rational numbers (denominator factors have negative exponents).

```mathematica
In[1]:= FactorInteger[12]
Out[1]= {{2, 2}, {3, 1}}

In[2]:= FactorInteger[-12]
Out[2]= {{-1, 1}, {2, 2}, {3, 1}}

In[3]:= FactorInteger[3/4]
Out[3]= {{2, -2}, {3, 1}}

In[4]:= FactorInteger[100, 1]
Out[4]= {{2, 2}, 25}
```

#### EulerPhi
Gives the Euler totient function $\phi(n)$.
- `EulerPhi[n]`

**Features**:
- `Listable`, `Protected`.
- Counts the number of positive integers less than or equal to $n$ that are relatively prime to $n$.
- Returns 0 for $n = 0$, and handles negative integers via $\phi(-n) = \phi(n)$.

```mathematica
In[1]:= EulerPhi[10]
Out[1]= 4
```

#### Re, Im, ReIm, Abs, Conjugate, Arg
Complex number functions.
- `Re[z]`: Real part.
- `Im[z]`: Imaginary part.
- `ReIm[z]`: Returns `{Re[z], Im[z]}`.
- `Abs[z]`: Magnitude.
- `Conjugate[z]`: Complex conjugate.
- `Arg[z]`: Phase angle.

### Elementary Functions

#### Trig Functions
`Sin`, `Cos`, `Tan`, `Cot`, `Sec`, `Csc` and their inverses `ArcSin`, etc.

**Features**:
- `Listable`, `NumericFunction`.
- Exact values for rational multiples of `Pi` with denominators `1, 2, 3, 4, 5, 6, 10, 12`.
- `ArcTan[x, y]` computes the quadrant-aware inverse tangent.

```mathematica
In[1]:= Sin[Pi/6]
Out[1]= 1/2

In[2]:= ArcTan[1, 1]
Out[1]= 1/4*Pi
```

#### Hyperbolic Functions
`Sinh`, `Cosh`, `Tanh`, `Coth`, `Sech`, `Csch` and their inverses.

**Features**:
- `Listable`, `NumericFunction`.
- Special values for `0` and `Infinity`.

#### Exponential and Logarithmic Functions
- `Exp[z]`: Natural exponential. Reduces `Exp[I*q*Pi]` using Euler's formula.
- `Log[z]`: Natural logarithm.
- `Log[b, z]`: Logarithm to base `b`.

```mathematica
In[1]:= Log[2, 8]
Out[1]= 3

In[2]:= Exp[I * Pi]
Out[2]= -1
```

#### Piecewise and Rounding Functions
`Floor`, `Ceiling`, `Round`, `IntegerPart`, `FractionalPart`.

**Features**:
- `Listable`.
- Applied component-wise to `Complex` numbers.
- `Round` rounds to the nearest even integer for ties.
- `Floor[x, a]` returns the greatest multiple of `a` $\le x$.

```mathematica
In[1]:= Round[2.5]
Out[1]= 2

In[2]:= Round[3.5]
Out[2]= 4
```

### Lists and Iteration

#### Table
Generates a list of expressions.
- `Table[expr, n]`
- `Table[expr, {i, imax}]`
- `Table[expr, {i, imin, imax, di}]`
- `Table[expr, {i, {val1, val2, ...}}]`

**Features**:
- `HoldAll`: `expr` is evaluated once for each step.
- Supports nested iterators to create matrices.

```mathematica
In[1]:= Table[i^2, {i, 4}]
Out[1]= {1, 4, 9, 16}
```

#### Range
Generates a list of values.
- `Range[imax]`
- `Range[imin, imax, di]`

#### Array
Generates an array by applying a function to indices.
- `Array[f, n]`
- `Array[f, {n1, n2, ...}]`

### Functional Programming

#### Pure Functions (Function, &)
`Function` is a pure (or "anonymous") function, analogous to $\lambda$ in LISP.
- `Function[body]` or `body &`: A pure function where formal parameters are `#` (or `#1`), `#2`, etc.
- `Function[{u, v, ...}, body]`: A pure function with named local formal parameters.
- `Function[params, body, attrs]`: A pure function treated as having specific evaluation `attrs`.
- `Slot` (`#`): Represents the first argument. `#n` represents the $n$-th argument.
- `SlotSequence` (`##`): Represents all arguments sequence. `##n` represents all arguments starting from the $n$-th.

**Features**:
- Has attribute `HoldAll`, evaluating its body only after arguments are substituted.
- `Slot` parameters are positionally mapped to arguments provided. Remaining arguments are ignored.
- Pure functions can be assigned to variables (e.g., `f = #^2 &`) and applied over lists (e.g., `f /@ {1, 2, 3}`).
- Properly scopes nested `Function` expressions: named inner variables shadow outer ones, and unnamed inner functions establish a new scope for `#` slots.

```mathematica
In[1]:= Function[u, 3 + u][x]
Out[1]= 3 + x

In[2]:= (#1^2 + #2^4)&[x, y]
Out[2]= x^2 + y^4

In[3]:= f[X, ##, Y, ##]&[a, b, c]
Out[3]= f[X, a, b, c, Y, a, b, c]
```

#### Map (/@)
- `f /@ expr` or `Map[f, expr]`

#### Apply (@@, @@@)
- `f @@ expr`: Level 0.
- `f @@@ expr`: Level 1.

#### MapAll (//@)
- `f //@ expr`: Recursive map.

#### Through
Distributes operators that appear inside the heads of expressions.
- `Through[expr]`: Distributes the top-level head.
- `Through[expr, h]`: Performs the transformation wherever `h` occurs in the head of `expr`.

**Features**:
- `Protected`.

```mathematica
In[1]:= Through[{f, g, h}[x]]
Out[1]= {f[x], g[x], h[x]}

In[2]:= Through[(f + g)[x, y]]
Out[2]= f[x, y] + g[x, y]
```

#### Select
Filters elements from an expression matching a criterion.
- `Select[list, crit]`: Returns an expression with the same head as `list`, containing only those elements `e` for which `crit[e]` evaluates to `True`.
- `Select[list, crit, n]`: Returns only the first `n` matching elements.

```mathematica
In[1]:= Select[{1, 2, 4, 7, 6, 2}, EvenQ]
Out[1]= {2, 4, 6, 2}

In[2]:= Select[{1, 2, 4, 7, 6, 2}, # > 2 &, 1]
Out[2]= {4}
```

### Assignment and Rules

#### Set (=), SetDelayed (:=)
- `lhs = rhs`: `Set`. Immediate evaluation of `rhs`.
- `lhs := rhs`: `SetDelayed`. Delayed evaluation of `rhs`.

#### Replace
Applies a rule or list of rules to transform an expression.
- `Replace[expr, rules]`: Applies rules to the entire expression.
- `Replace[expr, rules, levelspec]`: Applies rules to parts specified by `levelspec`.
- `Replace[expr, rules, levelspec, Heads -> True]`: Includes heads of expressions and their parts.

**Features**:
- `Protected`.
- Rules must be of the form `lhs -> rhs` (`Rule`) or `lhs :> rhs` (`RuleDelayed`).
- Tries rules in order. The first one that matches is applied.
- If rules are given in nested lists, `Replace` is mapped onto the inner lists.
- Standard level specifications (`n`, `Infinity`, `All`, `{n}`, `{n1, n2}`) are fully supported with the default being `{0}` (the whole expression).
- Expressions at deeper levels in a subexpression are matched first.
- Replaces parts even inside `Hold` or related wrappers.

```mathematica
In[1]:= Replace[x^2, x^2 -> a + b]
Out[1]= a + b

In[2]:= Replace[1 + x^2, x^2 -> a + b]
Out[2]= 1 + x^2

In[3]:= Replace[x, {{x -> a}, {x -> b}}]
Out[3]= {a, b}

In[4]:= Replace[1 + x^2, x^2 -> a + b, {1}]
Out[4]= 1 + a + b
```

#### ReplaceAll (/.)
Applies a rule or list of rules to transform each subpart of an expression.
- `expr /. lhs -> rhs` or `expr /. {rules}`

**Features**:
- `Protected`.
- Evaluates the entire expression top-down. The first rule that applies to a particular part is used; no further rules are tried on that part or on any of its subparts.
- Applies a rule only once to an expression.
- Returns `expr` unmodified if no rules apply.
- Maps across lists of rules appropriately.

```mathematica
In[1]:= {x, x^2, y, z} /. x -> 1
Out[1]= {1, 1, y, z}

In[2]:= Sin[x] /. Sin -> Cos
Out[2]= Cos[x]

In[3]:= {1, 3, 2, x, 6, Pi} /. _?PrimeQ -> "prime"
Out[3]= {1, "prime", "prime", x, 6, Pi}

In[4]:= {f[2], f[x, y], h[], f[]} /. f[x__] -> "OK"
Out[4]= {"OK", "OK", h[], f[]}
```

#### CompoundExpression (;)
- `expr1; expr2; ...`: Evaluates a sequence of expressions, returning the last one.

### Scoping Constructs

#### Module
Implements lexical scoping by creating unique local variables.
- `Module[{x, y, ...}, expr]`
- `Module[{x = x0, ...}, expr]`

**Features**:
- `HoldAll`, `Protected`.
- Variables are renamed to `name$nnn` using `$ModuleNumber`.
- Created symbols have the `Temporary` attribute.

```mathematica
In[1]:= x = 1; Module[{x = 2}, x + 1]
Out[1]= 3

In[2]:= x
Out[2]= 1
```

#### Block
Implements dynamic scoping by temporarily overriding symbol values.
- `Block[{x, y, ...}, expr]`
- `Block[{x = x0, ...}, expr]`

**Features**:
- `HoldAll`, `Protected`.
- Affects only values, not names.
- Restores original values and attributes after execution.

#### With
Defines local constants by lexical substitution.
- `With[{x = x0, ...}, expr]`
- `With[{x := x0, ...}, expr]`

**Features**:
- `HoldAll`, `Protected`.
- Replaces occurrences of symbols in the body before evaluation.

```mathematica
In[1]:= x = 10; With[{x = 5}, x^2]
Out[1]= 25
```

### Pattern Matching

- `_`: `Blank[]`.
- `__`: `BlankSequence[]`.
- `___`: `BlankNullSequence[]`.
- `x_`: `Pattern[x, _]`.
- `_h`: Matches expression with head `h`.
- `p ? test`: `PatternTest[p, test]`.
- `lhs /; condition`: Conditional matching.

## 3. Operators and Precedence

| Operator | FullForm | Precedence | Association |
|----------|----------|------------|-------------|
| `[[...]]`| `Part` | 100 | Left |
| `f[x]`   | `f[x]` | 1000 | Left |
| `_`, `__`, `___` | `Blank` | 730 | None |
| `?`      | `PatternTest` | 680 | None |
| `@`      | `Prefix` | 620 | Right |
| `//`     | `Postfix` | 70 | Left |
| `&`      | `Function`| 90 | Left |
| `@@`     | `Apply` | 620 | Right |
| `/@`     | `Map`   | 620 | Right |
| `^`      | `Power` | 590 | Right |
| `*`      | `Times` | 400 | Left |
| `/`      | `Divide`| 470 | Left |
| `+`, `-` | `Plus`  | 310 | Left |
| `==`     | `Equal` | 290 | None |
| `===`    | `SameQ` | 290 | None |
| `=`      | `Set`   | 40 | Right |
| `:=`     | `SetDelayed` | 40 | Right |
| `;`      | `CompoundExpression` | 10 | Left |

## 4. Limitations
- Large integer arithmetic is not supported (limited to 64-bit).
- Transcendental simplification is limited.
- Pattern matching for `Flat` and `Orderless` heads is partially implemented.

## 5. Technical Implementation Details

The system is composed of several interdependent subsystems: Expression Representation, Parser, Symbol Table, Evaluator, Pattern Matcher, and Rule Engine.

### 5.1. Expression Representation (`Expr`)

Everything in PicoCAS is an expression, represented by the `Expr` struct. It is implemented as a tagged union.

*   **Types (`ExprType`)**:
    *   `EXPR_INTEGER`: A 64-bit signed integer (`int64_t`).
    *   `EXPR_REAL`: A 64-bit floating-point number (`double`).
    *   `EXPR_SYMBOL`: A named identifier (e.g., `x`, `Plus`, `True`).
    *   `EXPR_STRING`: A string literal.
    *   `EXPR_FUNCTION`: A compound expression consisting of a `head` (which is itself an `Expr*`) and an array of `args` (`Expr**`).

All data structures are immutable-by-convention during evaluation. Functions that manipulate expressions generally return new allocated trees or unmodified references if no changes were made. 

**Memory Management:** Explicit manual memory management is required. `expr_new_*` allocates nodes. `expr_copy` performs a deep copy. `expr_free` performs a deep deallocation. **Crucial Rule:** Built-in functions take ownership of the expression passed to them. If a built-in returns a new expression, it *must* free the input expression (or reuse its nodes). If it returns `NULL` (meaning it remains unevaluated), the evaluator retains ownership.

### 5.2. Symbol Table (`symtab.c`)

The symbol table (`SymbolDef`) stores the global state associated with symbols. Each symbol can have:
*   **Attributes**: Bitflags (e.g., `ATTR_FLAT`, `ATTR_ORDERLESS`, `ATTR_LISTABLE`, `ATTR_PROTECTED`, `ATTR_HOLDALL`, `ATTR_NUMERICFUNCTION`).
*   **OwnValues**: Immediate assignments (e.g., `x = 5`).
*   **DownValues**: Delayed assignments with pattern matching (e.g., `f[x_] := x^2`).
*   **Builtin C Function**: A pointer to a C function (`BuiltinFunc`) that implements native evaluation logic (e.g., `builtin_plus`).

### 5.3. Parser (`parse.c`)

The parser translates raw string input into `Expr` trees.
*   **Lexical Analysis**: Handled inline within the parsing routines by advancing a `ParserState` pointer and skipping whitespace.
*   **Pratt Parsing**: Operator precedence (Infix, Prefix, Postfix) is implemented using a Pratt parser (`parse_expression_prec`). Operator precedence values exactly mirror Mathematica's standard precedences (e.g., `OP_CALL` is 1000, `OP_TIMES` is 400, `OP_PLUS` is 310, `OP_PREFIX` (`@`) is 620, `OP_POSTFIX` (`//`) is 70).

### 5.4. Evaluator (`eval.c`)

The `evaluate(Expr* e)` function is the heart of PicoCAS. It follows Mathematica's infinite evaluation semantics: expressions are repeatedly evaluated until a fixed point is reached (the expression no longer changes).

**Evaluation Sequence for Functions (`f[arg1, arg2]`):**
1.  **Evaluate Head**: The head `f` is evaluated first.
2.  **Check Attributes**: The attributes of the evaluated head are retrieved.
3.  **Evaluate Arguments**: Arguments are evaluated standardly, *unless* the head possesses Hold attributes (`ATTR_HOLDFIRST`, `ATTR_HOLDREST`, `ATTR_HOLDALL`, `ATTR_HOLDALLCOMPLETE`).
4.  **Apply Listable**: If the head is `ATTR_LISTABLE` and any evaluated argument is a `List`, the function automatically threads over the lists (e.g., `Mod[{1, 2}, 3] -> {Mod[1, 3], Mod[2, 3]}`).
5.  **Apply Flat / Orderless**: If `ATTR_FLAT` (associative), nested calls to the same head are flattened. If `ATTR_ORDERLESS` (commutative), arguments are lexically sorted.
6.  **Apply Built-ins**: If a C-level built-in function is registered, it is called.
7.  **Apply User Rules**: If no built-in handles it (or returns `NULL`), the evaluator checks the `DownValues` of the head and applies the first matching pattern replacement.

### 5.5. Pattern Matcher (`match.c`)

Implements structural pattern matching (`MatchQ`).
*   `Blank[]` (`_`): Matches any single expression.
*   `BlankSequence[]` (`__`): Matches 1 or more expressions in a sequence.
*   `BlankNullSequence[]` (`___`): Matches 0 or more expressions in a sequence.
*   `Pattern[name, pattern]` (`name_`): Binds the matched sub-expression to `name` inside a `MatchEnv`.
*   The matcher recursively unifies trees and handles sequence segmenting through backtracking.

### 5.6. Rule Engine (`replace.c`)

Implements expression transformations via rules (`Rule` `->` and `RuleDelayed` `:>`).
*   `ReplaceAll` (`/.`): Traverses the tree top-down, applying rules to sub-expressions.
*   Uses `match.c` to determine if a rule's LHS matches the current expression, and if so, substitutes bindings into the RHS.

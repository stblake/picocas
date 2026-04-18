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

#### Extract
Extracts the part of an expression at the position specified by `pos`.
- `Extract[expr, pos]`
- `Extract[expr, {pos1, pos2, ...}]`
- `Extract[expr, pos, h]`
- `Extract[pos]`

**Features**:
- Position specifications have the same form as those returned by `Position`.
- `Extract[expr, {i, j, ...}]` is equivalent to `Part[expr, i, j, ...]`.
- `pos` can be of the more general form `{part1, part2, ...}` where `parti` are `Part` specifications such as an integer `i`, `All` or `Span`.
- You can use `Extract[expr, ..., Hold]` to extract parts without evaluation.

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

#### Join
Concatenates lists or other expressions that share the same head.
- `Join[list1, list2, ...]`: Concatenates the elements of all lists into a single expression.
- `Join[list1, list2, ..., n]`: Joins the objects at level `n` in each of the lists.

**Features**:
- `Protected`.
- All arguments must share the same head; returns unevaluated if heads differ.
- Works on any head, not just `List` (e.g., `Join[f[a], f[b]]` gives `f[a, b]`).
- `Join[list1, list2, ..., n]` handles ragged arrays by concatenating successive elements at level `n`.

```mathematica
In[1]:= Join[{a, b, c}, {x, y}, {u, v, w}]
Out[1]= {a, b, c, x, y, u, v, w}

In[2]:= Join[{1, 2}, {3, 4}]
Out[2]= {1, 2, 3, 4}

In[3]:= Join[f[a, b], f[c, d]]
Out[3]= f[a, b, c, d]

In[4]:= Join[{{a, b}, {c, d}}, {{1, 2}, {3, 4}}, 2]
Out[4]= {{a, b, 1, 2}, {c, d, 3, 4}}

In[5]:= Join[{{1}, {5, 6}}, {{2, 3}, {7}}, {{4}, {8}}, 2]
Out[5]= {{1, 2, 3, 4}, {5, 6, 7, 8}}

In[6]:= Join[{{x}}, {{1, 2}, {3, 4}}, 2]
Out[6]= {{x, 1, 2}, {3, 4}}
```

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

#### OrderedQ
- `OrderedQ[expr]`: Gives `True` if the elements of `expr` are in canonical order, and `False` otherwise.
- `OrderedQ[expr, p]`: Uses the ordering function `p` to determine whether each pair of elements is in order.

**Features**:
- `Protected`.
- Uses the same internal canonical comparison logic as `Sort` by default.
- Custom ordering function `p` may return `1`, `0`, `-1`, `True`, or `False`.
- `OrderedQ` works with any expression head, not just `List`.
- Automatically handles 0- and 1-element lists.

```mathematica
In[1]:= OrderedQ[{1, 4, 2}]
Out[1]= False

In[2]:= OrderedQ[{"cat", "catfish", "fish"}]
Out[2]= True

In[3]:= OrderedQ[{1, Sqrt[2], 2, E, 3, Pi}, Less]
Out[3]= True

In[4]:= OrderedQ[{{a, 2}, {c, 1}, {d, 3}}, #1[[2]] < #2[[2]] &]
Out[4]= False

In[5]:= OrderedQ[f[b, a, c]]
Out[5]= False
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

#### Commonest
Gives a list of the elements that are the most common in an expression.
- `Commonest[list]`
- `Commonest[list, n]`
- `Commonest[list, UpTo[n]]`

**Features**:
- `Protected`.
- When several elements occur with equal frequency, `Commonest` picks first the ones that occur first in `list`.
- `Commonest[list, n]` returns the `n` commonest elements in the order they appear in `list`.
- `Commonest[list, UpTo[n]]` returns the `n` commonest elements, or as many as are available.
- A message `Commonest::dstlms` is generated if there are fewer distinct elements than requested by an integer `n`.

```mathematica
In[1]:= Commonest[{b, a, c, 2, a, b, 1, 2}]
Out[1]= {b, a, 2}

In[2]:= Commonest[{b, a, c, 2, a, b, 1, 2}, 4]
Out[2]= {b, a, c, 2}

In[3]:= Commonest[{b, a, c, 2, a, b, 1, 2}, UpTo[6]]
Out[3]= {b, a, c, 2, 1}

In[4]:= Commonest[{1, 2, 2, 3, 3, 3, 4}]
Out[4]= {3}

In[5]:= Commonest[{a, E, Sin[y], E, a, 7}]
Out[5]= {a, E}
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

#### PolynomialExtendedGCD
Gives the extended GCD of polynomials.
- `PolynomialExtendedGCD[poly1, poly2, x]`
- `PolynomialExtendedGCD[poly1, poly2, x, Modulus -> p]`

**Features**:
- `Protected`.
- Returns `{d, {a, b}}` such that $a \cdot poly1 + b \cdot poly2 = d$.
- $d$ is the GCD, normalized to be monic.
- Efficiently handles termination when a constant remainder is reached.
- Optimized for cases where the divisor is a constant.

```mathematica
In[1]:= PolynomialExtendedGCD[2x^5-2x, (x^2-1)^2, x]
Out[1]= {-1 + x^2, {x/4, (-4 - 2 x^2)/4}}

In[2]:= PolynomialExtendedGCD[a (x+b)^2, (x+a)(x+b), x]
Out[2]= {b + x, {-(1/(a (a - b))), 1/(a - b)}}
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

#### ClearAttributes
Removes attributes from symbols.
- `ClearAttributes[s, attr]`: Removes `attr` from the list of attributes of the symbol `s`.
- `ClearAttributes["s", attr]`: Removes `attr` from the attributes of the symbol named `"s"` if it exists.
- `ClearAttributes[s, {attr1, attr2, ...}]`: Removes several attributes at a time.
- `ClearAttributes[{s1, s2, ...}, attrs]`: Removes attributes from several symbols at a time.

**Features**:
- `HoldFirst`, `Protected`.
- `ClearAttributes` modifies `Attributes[s]`.
- Cannot clear attributes of a `Locked` symbol.
- Clearing an attribute that is not set is a no-op.

```mathematica
In[1]:= SetAttributes[f, Listable]
In[2]:= f[{1, 2, 3}]
Out[2]= {f[1], f[2], f[3]}
In[3]:= ClearAttributes[f, Listable]
In[4]:= f[{1, 2, 3}]
Out[4]= f[{1, 2, 3}]

In[5]:= SetAttributes[f, {Flat, Orderless, OneIdentity}]
In[6]:= ClearAttributes[f, OneIdentity]
In[7]:= Attributes[f]
Out[7]= {Flat, Orderless}

In[8]:= ClearAttributes[f, {Flat, Orderless}]
In[9]:= Attributes[f]
Out[9]= {}

In[10]:= SetAttributes[{g, h}, Protected]
In[11]:= ClearAttributes[{g, h}, Protected]
In[12]:= Attributes[g]
Out[12]= {}
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

#### Print
- `Print[expr1, expr2, ...]` prints the expressions to stdout and returns `Null`.
- Supports `FullForm` and `InputForm` wrappers.

```mathematica
In[1]:= Print["Result: ", x + y]
Result: x + y
Out[1]= Null

In[2]:= Print[x + y // FullForm]
Plus[x, y]
Out[2]= Null
```

#### FullForm
- `FullForm[expr]` causes `expr` to be printed in its full internal form.
- `expr // FullForm` is a common shortcut.


#### HoldForm
- `HoldForm[expr]` prints as the expression `expr`, with `expr` maintained in an unevaluated form.
- Has attribute `HoldAll` and `Protected`.
- It dissolves prior to printing but preserves the unevaluated `expr` it contains.

```mathematica
In[1]:= HoldForm[1 + 1]
Out[1]= 1 + 1
```
#### Evaluate
Causes `expr` to be evaluated even if it appears as the argument of a function whose attributes specify that it should be held unevaluated.
- `Evaluate[expr]`

**Features**:
- `Protected`.
- `Evaluate` only overrides `HoldFirst`, `HoldRest`, and `HoldAll` attributes when it appears directly as the head of the function argument that would otherwise be held.
- `Evaluate` does not override `HoldAllComplete`.
- `Evaluate` works only on the first level, directly inside a held function. It does not penetrate into deeper subexpressions.
- Outside of held contexts, `Evaluate` acts as identity.

```mathematica
In[1]:= Evaluate[1+1]
Out[1]= 2

In[2]:= Hold[Evaluate[1+1], 2+2]
Out[2]= Hold[2, 2 + 2]

In[3]:= Hold[Evaluate[1+1], Evaluate[2+2], Evaluate[3+3]]
Out[3]= Hold[2, 4, 6]

In[4]:= Hold[f[Evaluate[1+2]]]
Out[4]= Hold[f[Evaluate[1 + 2]]]

In[5]:= Hold[Evaluate[Sin[Pi/6]]]
Out[5]= Hold[1/2]

In[6]:= Hold[Evaluate[2^10]]
Out[6]= Hold[1024]

In[7]:= HoldForm[Evaluate[1+1]]
Out[7]= 2

In[8]:= Hold[Evaluate[Length[{a,b,c}]]]
Out[8]= Hold[3]

In[9]:= Evaluate[Head[{1,2,3}]]
Out[9]= List
```

#### ReleaseHold
Removes `Hold`, `HoldForm`, `HoldPattern`, and `HoldComplete` in `expr`.
- `ReleaseHold[expr]`

**Features**:
- `Protected`.
- `ReleaseHold` removes only one layer of `Hold` etc.; it does not remove inner occurrences in nested `Hold` etc. functions.
- `ReleaseHold` traverses into subexpressions of `expr` and strips any hold wrapper it finds, but does not recurse into the contents of the stripped wrapper.
- When `expr` does not contain any hold wrappers, `ReleaseHold` acts as identity.
- `ReleaseHold` removes all standard unevaluated containers: `Hold`, `HoldForm`, `HoldComplete`, and `HoldPattern`.

```mathematica
In[1]:= Hold[1+1]
Out[1]= Hold[1 + 1]

In[2]:= ReleaseHold[Hold[1+1]]
Out[2]= 2

In[3]:= ReleaseHold /@ {Hold[1+2], HoldForm[2+3], HoldComplete[3+4]}
Out[3]= {3, 5, 7}

In[4]:= ReleaseHold[f[Hold[1+2]]]
Out[4]= f[3]

In[5]:= ReleaseHold[f[Hold[1+g[Hold[2+3]]]]]
Out[5]= f[1 + g[Hold[2 + 3]]]

In[6]:= ReleaseHold[Hold[Hold[1+1]]]
Out[6]= Hold[1 + 1]

In[7]:= ReleaseHold[Hold[Sin[Pi/6]]]
Out[7]= 1/2

In[8]:= ReleaseHold[{f[Hold[1+2]], g[HoldForm[3+4]]}]
Out[8]= {f[3], g[7]}
```

#### InputForm
- `InputForm[expr]` causes `expr` to be printed in a form suitable for input (standard form in PicoCAS).

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

#### MemberQ
- `MemberQ[list, form]`: Returns `True` if an element of list matches form, and `False` otherwise.
- `MemberQ[list, form, levelspec]`: Tests all parts of list specified by levelspec.
- `MemberQ[form]`: Represents an operator form of `MemberQ` that can be applied to an expression.

**Features**:
- `Protected`.
- Default option: `Heads -> False`.
- `form` can be a structural pattern.
- The first argument of `MemberQ` can have any head, not necessarily `List`.
- Returns immediately upon finding the first match.
- Standard level specifications are supported. The default value for `levelspec` in `MemberQ` is `{1}`.

```mathematica
In[1]:= MemberQ[{1, 3, 4, 1, 2}, 2]
Out[1]= True

In[2]:= MemberQ[{x^2, y^2, x^3}, x^_]
Out[2]= True

In[3]:= MemberQ[{{1, 1, 3, 0}, {2, 1, 2, 2}}, 0, 2]
Out[3]= True

In[4]:= MemberQ[{{1, 1, 3, 0}, {2, 1, 2, 2}}, 0]
Out[4]= False
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


#### Inner
A generalization of `Dot` in which `f` plays the role of multiplication and `g` of addition.
- `Inner[f, list1, list2, g]`: computes the "inner `f`" of two lists, with "plus operation" `g`.
- `Inner[f, list1, list2]`: uses `Plus` for `g`.
- `Inner[f, list1, list2, g, n]`: contracts index `n` of the first tensor with the first index of the second tensor.

**Features**:
- `Protected`.
- Like `Dot`, `Inner` effectively contracts the last index of the first tensor with the first index of the second tensor.
- Applying `Inner` to a rank $r$ tensor and a rank $s$ tensor gives a rank $r+s-2$ tensor.

```mathematica
In[1]:= Inner[f, {a, b}, {x, y}, g]
Out[1]= g[f[a, x], f[b, y]]

In[2]:= Inner[Times, {{a, b}, {c, d}}, {x, y}, Plus]
Out[2]= {a x + b y, c x + d y}

In[3]:= Inner[Times, {a1, a2, a3}, {b1, b2, b3}, Plus]
Out[3]= a1 b1 + a2 b2 + a3 b3
```

#### Outer
Gives the generalized outer product of the `listi`, forming all possible combinations of the lowest-level elements in each of them, and feeding them as arguments to `f`.
- `Outer[f, list1, list2, ...]`
- `Outer[f, list1, list2, ..., n]`: treats as separate elements only sublists at level `n`.
- `Outer[f, list1, list2, ..., n1, n2, ...]`: treats as separate elements only sublists at level `ni` in the corresponding `listi`.

**Features**:
- `Protected`.
- Applying `Outer` to two tensors of ranks $r$ and $s$ gives a tensor of rank $r+s$.
- The heads of all `listi` must be the same, but need not necessarily be `List`.

```mathematica
In[1]:= Outer[f, {a, b}, {x, y, z}]
Out[1]= {{f[a, x], f[a, y], f[a, z]}, {f[b, x], f[b, y], f[b, z]}}

In[2]:= Outer[Times, {1, 2, 3, 4}, {a, b, c}]
Out[2]= {{a, b, c}, {2 a, 2 b, 2 c}, {3 a, 3 b, 3 c}, {4 a, 4 b, 4 c}}

In[3]:= Outer[g, f[a, b], f[x, y, z]]
Out[3]= f[f[g[a, x], g[a, y], g[a, z]], f[g[b, x], g[b, y], g[b, z]]]
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

#### Inverse
Gives the inverse of a square matrix.
- `Inverse[m]`

**Features**:
- `Protected`.
- Works on both symbolic and numerical matrices.
- For matrices with approximate real or complex numbers, the inverse is generated to the maximum possible precision given the input.
- Uses fraction-free Gauss-Jordan elimination on the augmented matrix `[A | I]` to compute the inverse exactly for integer, rational, and symbolic matrices.
- Issues `Inverse::sing` warning and returns unevaluated if the matrix is singular.
- Issues `Inverse::matsq` warning and returns unevaluated if the argument is not a non-empty square matrix.
- Satisfies the relation `a . Inverse[a] == Inverse[a] . a == IdentityMatrix[n]`.
- Satisfies the relation `Inverse[a . b] == Inverse[b] . Inverse[a]`.

```mathematica
In[1]:= Inverse[{{1.4,2},{3,-6.7}}]
Out[1]= {{0.435631, 0.130039}, {0.195059, -0.0910273}}

In[2]:= Inverse[{{1,2,3},{4,2,2},{5,1,7}}]
Out[2]= {{-2/7, 11/42, 1/21}, {3/7, 4/21, -5/21}, {1/7, -3/14, 1/7}}

In[3]:= Inverse[{{u,v},{v,u}}]
Out[3]= {{u/(u^2 - v^2), -v/(u^2 - v^2)}, {-v/(u^2 - v^2), u/(u^2 - v^2)}}

In[4]:= Inverse[{{1.2,2.5,-3.2},{0.7,-9.4,5.8},{-0.2,0.3,6.4}}]
Out[4]= {{0.74546, 0.204249, 0.187629}, {0.0679223, -0.0847825, 0.110795}, {0.0201118, 0.010357, 0.15692}}

In[5]:= Inverse[{{2,3,2},{4,9,2},{7,2,4}}]
Out[5]= {{-8/13, 2/13, 3/13}, {1/26, 3/26, -1/13}, {55/52, -17/52, -3/26}}

In[6]:= Inverse[{{a,b},{c,d}}]
Out[6]= {{d/(-b c + a d), -b/(-b c + a d)}, {-c/(-b c + a d), a/(-b c + a d)}}

In[7]:= Inverse[{{1,2},{1,2}}]
Inverse::sing: Matrix {{1, 2}, {1, 2}} is singular.
Out[7]= Inverse[{{1, 2}, {1, 2}}]

In[8]:= a = {{1,2},{3,4}}; a . Inverse[a] == IdentityMatrix[2]
Out[8]= True

In[9]:= a = {{1,1,1},{6,9,7},{8,1,9}}; b = {{0,3,9},{7,9,7},{4,4,1}}; Inverse[a . b] == Inverse[b] . Inverse[a]
Out[9]= True

In[10]:= Inverse[{{1,2},{3,4},{5,6}}]
Inverse::matsq: Argument {{1, 2}, {3, 4}, {5, 6}} at position 1 is not a non-empty square matrix.
Out[10]= Inverse[{{1, 2}, {3, 4}, {5, 6}}]
```

#### MatrixPower
Gives the matrix power of a square matrix.
- `MatrixPower[m, n]`: gives the n-th matrix power of the matrix `m`.
- `MatrixPower[m, n, v]`: gives the n-th matrix power of the matrix `m` applied to the vector `v`.

**Features**:
- `Protected`.
- `MatrixPower[m, n]` effectively evaluates the product of a matrix with itself `n` times.
- When `n` is negative, `MatrixPower` finds powers of the inverse of the matrix `m`.
- `MatrixPower[m, 0]` returns `IdentityMatrix[Length[m]]`.
- `MatrixPower` works only on square matrices.
- Uses binary exponentiation (repeated squaring) for efficient computation.
- Fractional matrix powers are not currently supported and generate a warning.

```mathematica
In[1]:= MatrixPower[{{a, b}, {c, d}}, 2]
Out[1]= {{a^2 + b c, a b + b d}, {a c + c d, b c + d^2}}

In[2]:= MatrixPower[{{a, b}, {c, d}}, 2] == {{a, b}, {c, d}} . {{a, b}, {c, d}}
Out[2]= True

In[3]:= MatrixPower[{{a, b}, {c, d}}, -2] == Inverse[{{a, b}, {c, d}}] . Inverse[{{a, b}, {c, d}}]
Out[3]= True

In[4]:= MatrixPower[{{1, 1}, {1, 2}}, 10]
Out[4]= {{4181, 6765}, {6765, 10946}}

In[5]:= MatrixPower[{{1.2, 2.5, -3.2}, {0.7, -9.4, 5.8}, {-0.2, 0.3, 6.4}}, 5]
Out[5]= {{-1208.61, 19598.2, -12658.4}, {5784.51, -83315.1, 35420.6}, {-559.11, 1960.12, 11511.9}}

In[6]:= MatrixPower[{{2, 3, 0}, {4, 9, 0}, {0, 0, 4}}, 14]
Out[6]= {{25881337259836, 54508871401413, 0}, {72678495201884, 153068703863133, 0}, {0, 0, 268435456}}

In[7]:= MatrixPower[{{a, b}, {0, c}}, 4]
Out[7]= {{a^4, a^2 (a b + b c) + c^2 (a b + b c)}, {0, c^4}}

In[8]:= MatrixPower[{{1, 1}, {1, 2}}, 3, {1, 0}]
Out[8]= {5, 8}

In[9]:= MatrixPower[{{1, 2}, {3, 4}}, 0]
Out[9]= {{1, 0}, {0, 1}}

In[10]:= MatrixPower[{{5}}, -2]
Out[10]= {{1/25}}
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


#### Median
Gives the median estimate of the elements in data.
- `Median[data]`: gives the median estimate $\hat{q}_{1/4}$ of the elements in `data`.
- `Median[dist]`: gives the median of the distribution `dist`.

**Features**:
- `Protected`.
- Median is a robust location estimator, which means it not very sensitive to outliers.
- For `VectorQ` data $\{x_1, \dots, x_n\}$, the median can be thought of as the "middle value". Formally, when data is sorted as $\{x_{(1)}, \dots, x_{(n)}\}$, the median is given by the center element $x_{((n+1)/2)}$ if $n$ is odd and the mean of the two center elements $(x_{(n/2)} + x_{(n/2+1)})/2$ if $n$ is even.
- For `MatrixQ` data, the median is computed for each column vector. `Median` for a tensor gives columnwise medians at the first level.
- `Median` requires numeric values.

```mathematica
In[1]:= Median[{1, 2, 3, 4, 5, 6, 7}]
Out[1]= 4

In[2]:= Median[{1, 2, 3, 4, 5, 6, 7, 8}]
Out[2]= 9/2

In[3]:= Median[{1, 2, 3, 4}]
Out[3]= 5/2

In[4]:= Median[{Pi, E, 2}]
Out[4]= E

In[5]:= Median[{1., 2., 3., 4.}]
Out[5]= 2.5

In[6]:= Median[{{1, 11, 3}, {4, 6, 7}}]
Out[6]= {5/2, 17/2, 5}

In[7]:= Median[{{{3, 7}, {2, 1}}, {{5, 19}, {12, 4}}}]
Out[7]= {{4, 13}, {7, 5/2}}

In[8]:= Median[{a, b, c}]
Median::rectn: Rectangular array of real numbers is expected at position 1 in Median[{a, b, c}].
Out[8]= Median[{a, b, c}]
```
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

#### RootMeanSquare
Gives the root mean square of values in `list`.
- `RootMeanSquare[list]`

**Features**:
- `Protected`.
- Gives the square root of the second sample moment.
- For a list `{x1, x2, ...}`, it computes `Sqrt[1/n Total[{x1^2, x2^2, ...}]]`.
- Handles both numerical and symbolic data.
- Works column-wise on matrices.

```mathematica
In[1]:= RootMeanSquare[{a, b, c, d}]
Out[1]= 1/2 Sqrt[a^2 + b^2 + c^2 + d^2]

In[2]:= RootMeanSquare[{{1, 2}, {5, 10}, {5, 2}, {4, 8}}]
Out[2]= {1/2 Sqrt[67], Sqrt[43]}

In[3]:= RootMeanSquare[{1, 2, 3, 4}]
Out[3]= Sqrt[15/2]

In[4]:= RootMeanSquare[{Pi, E, 2}]
Out[4]= Sqrt[1/3 (4 + E^2 + Pi^2)]

In[5]:= RootMeanSquare[{1., 2., 3., 4.}]
Out[5]= 2.73861
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

### Random Number Generation

#### RandomInteger
Gives pseudorandom integers.
- `RandomInteger[{imin, imax}]`: gives a pseudorandom integer in the range {imin, ..., imax}.
- `RandomInteger[imax]`: gives a pseudorandom integer in the range {0, ..., imax}.
- `RandomInteger[]`: pseudorandomly gives 0 or 1.
- `RandomInteger[range, n]`: gives a list of n pseudorandom integers.
- `RandomInteger[range, {n1, n2, ...}]`: gives an n1 x n2 x ... array of pseudorandom integers.

**Features**:
- `Protected`.
- RandomInteger[{imin, imax}] chooses integers in the range {imin, ..., imax} with equal probability.
- RandomInteger[] gives 0 or 1 with probability 1/2.
- RandomInteger gives a different sequence of pseudorandom integers whenever you run PicoCAS. You can start with a particular seed using SeedRandom.
- Returns bignums when the range exceeds 64-bit integer limits.

```mathematica
In[1]:= SeedRandom[42]; RandomInteger[]
Out[1]= 1

In[2]:= SeedRandom[42]; RandomInteger[10]
Out[2]= 6

In[3]:= SeedRandom[42]; RandomInteger[{1, 6}]
Out[3]= 4

In[4]:= SeedRandom[42]; RandomInteger[{0, 9}, 5]
Out[4]= {6, 9, 4, 1, 3}

In[5]:= SeedRandom[42]; Dimensions[RandomInteger[{0, 1}, {3, 4}]]
Out[5]= {3, 4}

In[6]:= SeedRandom[42]; RandomInteger[{-10, -5}]
Out[6]= -6

In[7]:= IntegerQ[RandomInteger[10^20]]
Out[7]= True
```

#### RandomReal
Gives pseudorandom real numbers.
- `RandomReal[]`: gives a pseudorandom real number in the range 0 to 1.
- `RandomReal[{xmin, xmax}]`: gives a pseudorandom real number in the range xmin to xmax.
- `RandomReal[xmax]`: gives a pseudorandom real number in the range 0 to xmax.
- `RandomReal[range, n]`: gives a list of n pseudorandom reals.
- `RandomReal[range, {n1, n2, ...}]`: gives an n1 x n2 x ... array of pseudorandom reals.

**Features**:
- `Protected`.
- RandomReal[{xmin, xmax}] chooses reals with a uniform probability distribution in the range xmin to xmax.
- RandomReal gives a different sequence of pseudorandom reals whenever you run PicoCAS. You can start with a particular seed using SeedRandom.
- Uses 53 bits of randomness for full double-precision mantissa coverage.
- Accepts integer, real, rational, and bigint range arguments.

```mathematica
In[1]:= SeedRandom[42]; RandomReal[]
Out[1]= 0.376082

In[2]:= SeedRandom[42]; RandomReal[10]
Out[2]= 3.76082

In[3]:= SeedRandom[42]; RandomReal[{-1, 1}]
Out[3]= -0.247836

In[4]:= SeedRandom[42]; Length[RandomReal[{0, 1}, 5]]
Out[4]= 5

In[5]:= SeedRandom[42]; Dimensions[RandomReal[{0, 1}, {3, 4}]]
Out[5]= {3, 4}

In[6]:= SeedRandom[42]; RandomReal[{0, 1}, 0]
Out[6]= {}

In[7]:= RandomReal[x]
Out[7]= RandomReal[x]
```

#### SeedRandom
Resets the pseudorandom generator.
- `SeedRandom[n]`: seeds the generator with integer n.
- `SeedRandom[]`: reseeds from system entropy.

**Features**:
- `Protected`.
- After `SeedRandom[n]`, the sequence of pseudorandom numbers generated will be the same each time.
- Accepts bignums as seeds.

```mathematica
In[1]:= SeedRandom[42]; {RandomInteger[], RandomInteger[], RandomInteger[]}
Out[1]= {1, 1, 0}

In[2]:= SeedRandom[42]; {RandomInteger[], RandomInteger[], RandomInteger[]}
Out[2]= {1, 1, 0}
```

#### RandomComplex
Gives pseudorandom complex numbers.
- `RandomComplex[]`: gives a pseudorandom complex number with real and imaginary parts in the range 0 to 1.
- `RandomComplex[{zmin, zmax}]`: gives a pseudorandom complex number in the rectangle with corners given by the complex numbers zmin and zmax.
- `RandomComplex[zmax]`: gives a pseudorandom complex number in the rectangle whose corners are the origin and zmax.
- `RandomComplex[range, n]`: gives a list of n pseudorandom complex numbers.
- `RandomComplex[range, {n1, n2, ...}]`: gives an n1 x n2 x ... array of pseudorandom complex numbers.

**Features**:
- `Protected`.
- `RandomComplex[{zmin, zmax}]` chooses complex numbers uniformly in the rectangle with corners at `zmin` and `zmax`.
- RandomComplex gives a different sequence of pseudorandom complex numbers whenever you run PicoCAS. You can start with a particular seed using SeedRandom.
- Uses 53 bits of randomness per component for full double-precision mantissa coverage.
- Accepts integer, real, rational, and complex range arguments. When the range has no imaginary component, the result simplifies to a real.

```mathematica
In[1]:= SeedRandom[42]; Head[RandomComplex[]]
Out[1]= Complex

In[2]:= SeedRandom[42]; RandomComplex[2 + 3 I]
Out[2]= 0.752164 + 1.30654 I

In[3]:= SeedRandom[42]; Length[RandomComplex[{0, 1 + I}, 5]]
Out[3]= 5

In[4]:= SeedRandom[42]; Dimensions[RandomComplex[{0, 1 + I}, {3, 4}]]
Out[4]= {3, 4}

In[5]:= RandomComplex[x]
Out[5]= RandomComplex[x]
```

#### RandomChoice
Gives pseudorandom choices from a list of elements.
- `RandomChoice[{e1, e2, ...}]`: gives a pseudorandom choice of one of the ei.
- `RandomChoice[list, n]`: gives a list of n pseudorandom choices.
- `RandomChoice[list, {n1, n2, ...}]`: gives an n1 x n2 x ... array of pseudorandom choices.
- `RandomChoice[{w1, w2, ...} -> {e1, e2, ...}]`: gives a pseudorandom choice weighted by the wi.
- `RandomChoice[wlist -> elist, n]`: gives a list of n weighted choices.
- `RandomChoice[wlist -> elist, {n1, n2, ...}]`: gives an n1 x n2 x ... array of weighted choices.

**Features**:
- `Protected`.
- `RandomChoice[{e1, e2, ...}]` chooses with equal probability between all of the ei.
- RandomChoice gives a different sequence of pseudorandom choices whenever you run PicoCAS. You can start with a particular seed using SeedRandom.
- Weighted selection uses cumulative weight binary search for efficient O(log n) per choice.
- Weights must be non-negative real numbers with a positive total.

```mathematica
In[1]:= SeedRandom[42]; RandomChoice[{a, b, c, d, e}]
Out[1]= c

In[2]:= SeedRandom[42]; RandomChoice[{a, b, c}, 5]
Out[2]= {b, c, b, a, a}

In[3]:= SeedRandom[42]; Dimensions[RandomChoice[{1, 2, 3}, {3, 4}]]
Out[3]= {3, 4}

In[4]:= RandomChoice[{1, 0, 0} -> {a, b, c}]
Out[4]= a

In[5]:= RandomChoice[{1, 0} -> {x, y}, 5]
Out[5]= {x, x, x, x, x}

In[6]:= RandomChoice[x]
Out[6]= RandomChoice[x]
```

#### RandomSample
Gives a pseudorandom sample of elements without replacement.
- `RandomSample[{e1, e2, ...}, n]`: gives a pseudorandom sample of n of the ei.
- `RandomSample[{w1, w2, ...} -> {e1, e2, ...}, n]`: gives a pseudorandom sample of n of the ei chosen using weights wi.
- `RandomSample[{e1, e2, ...}]`: gives a pseudorandom permutation of the ei.
- `RandomSample[list, UpTo[n]]`: gives a sample of n of the ei, or as many as are available.

**Features**:
- `Protected`.
- `RandomSample[{e1, e2, ...}, n]` never samples any of the ei more than once.
- `RandomSample[{e1, e2, ...}, n]` samples each of the ei with equal probability.
- `RandomSample[{e1, e2, ...}, UpTo[n]]` gives a sample of n of the ei, or as many as are available.
- RandomSample gives a different sequence of pseudorandom choices whenever you run PicoCAS. You can start with a particular seed using SeedRandom.
- Requesting n greater than the list length (without UpTo) returns unevaluated.
- Uses the Fisher-Yates shuffle for uniform sampling without replacement.
- Weighted sampling removes selected elements and renormalizes weights.

```mathematica
In[1]:= SeedRandom[42]; RandomSample[{a, b, c, d, e}, 3]
Out[1]= {d, e, b}

In[2]:= Sort[RandomSample[{1, 2, 3, 4, 5}, 5]]
Out[2]= {1, 2, 3, 4, 5}

In[3]:= Length[RandomSample[{a, b, c, d, e}]]
Out[3]= 5

In[4]:= RandomSample[{a, b, c}, 0]
Out[4]= {}

In[5]:= Length[RandomSample[{a, b, c, d, e}, UpTo[10]]]
Out[5]= 5

In[6]:= RandomSample[{1, 0, 0} -> {a, b, c}, 1]
Out[6]= {a}

In[7]:= Sort[RandomSample[{1, 1, 0} -> {a, b, c}, 2]]
Out[7]= {a, b}

In[8]:= Sort[RandomSample[{1, 2, 3} -> {a, b, c}]]
Out[8]= {a, b, c}

In[9]:= RandomSample[{a, b}, 5]
Out[9]= RandomSample[{a, b}, 5]

In[10]:= RandomSample[x]
Out[10]= RandomSample[x]
```

### String Operations

#### StringLength

Gives the number of characters in a string.

- `StringLength["string"]`: Returns the number of characters as an integer.
- Returns unevaluated for non-string arguments.
- **Attributes**: `Listable`, `Protected`.

```mathematica
In[1]:= StringLength["tiger"]
Out[1]= 5

In[2]:= StringLength[""]
Out[2]= 0

In[3]:= StringLength["hello world"]
Out[3]= 11

In[4]:= StringLength[{\"ABC\", \"DE\", \"F\"}]
Out[4]= {3, 2, 1}

In[5]:= StringLength[x]
Out[5]= StringLength[x]
```

#### Characters

Gives a list of the characters in a string.

- `Characters["string"]`: Returns a `List` of single-character strings.
- Each character is given as a length-1 string.
- Returns unevaluated for non-string arguments.
- **Attributes**: `Listable`, `Protected`.

```mathematica
In[1]:= Characters["ABC"]
Out[1]= {"A", "B", "C"}

In[2]:= Characters["A string."]
Out[2]= {"A", " ", "s", "t", "r", "i", "n", "g", "."}

In[3]:= Characters[""]
Out[3]= {}

In[4]:= Characters[{"ABC", "DEF", "XYZ"}]
Out[4]= {{"A", "B", "C"}, {"D", "E", "F"}, {"X", "Y", "Z"}}

In[5]:= Characters[x]
Out[5]= Characters[x]
```

#### StringJoin

Concatenates strings together.

- `StringJoin["s1", "s2", ...]`: Joins all string arguments into a single string.
- `StringJoin[{"s1", "s2", ...}]`: Flattens all lists recursively and joins enclosed strings.
- `StringJoin[]`: Returns the empty string `""`.
- The infix form is `"s1" <> "s2" <> ...`.
- Returns unevaluated if any non-string, non-list leaf is encountered.
- **Attributes**: `Flat`, `OneIdentity`, `Protected`.

```mathematica
In[1]:= StringJoin["abcd", "ABCD", "xyz"]
Out[1]= "abcdABCDxyz"

In[2]:= "abcd" <> "ABCD" <> "xyz"
Out[2]= "abcdABCDxyz"

In[3]:= StringJoin[{{"AB", "CD"}, "XY"}]
Out[3]= "ABCDXY"

In[4]:= StringJoin[]
Out[4]= ""

In[5]:= StringJoin["a", x]
Out[5]= StringJoin["a", x]

In[6]:= StringJoin[Characters["hello"]]
Out[6]= "hello"
```

#### StringPart

Extracts characters from a string by position.

- `StringPart["string", n]`: Gives the nth character in `"string"` as a single-character string.
- `StringPart["string", -n]`: Counts from the end to give the nth-to-last character.
- `StringPart["string", {n1, n2, ...}]`: Gives a list of the specified characters.
- `StringPart["string", m;;n]`: Gives a list of characters from position m through n.
- `StringPart["string", m;;n;;s]`: Gives characters from m through n in steps of s.
- `StringPart[{s1, s2, ...}, spec]`: Gives the list of results for each of the si.
- Negative indices count from the end of the string.
- In `StringPart["string", m;;n;;s]`, m, n, and/or s can be negative.
- Returns unevaluated for invalid arguments or out-of-bounds indices.
- **Attributes**: `Protected`.

```mathematica
In[1]:= StringPart["abcdefghijklm", 6]
Out[1]= "f"

In[2]:= StringPart["abcdefghijklm", {1, 3, 5}]
Out[2]= {"a", "c", "e"}

In[3]:= StringPart["abcdefghijklm", -4]
Out[3]= "j"

In[4]:= StringPart["abcdefghijklm", 1;;6]
Out[4]= {"a", "b", "c", "d", "e", "f"}

In[5]:= StringPart["abcdefghijklm", 1;;-1;;2]
Out[5]= {"a", "c", "e", "g", "i", "k", "m"}

In[6]:= StringPart["abcdefghijklm", -1;;1;;-2]
Out[6]= {"m", "k", "i", "g", "e", "c", "a"}

In[7]:= StringPart[{"abcd", "efgh", "ijklm"}, 1]
Out[7]= {"a", "e", "i"}

In[8]:= StringPart[{"abcd", "efgh", "ijklm"}, {1, -1}]
Out[8]= {{"a", "d"}, {"e", "h"}, {"i", "m"}}

In[9]:= StringPart["abcdef", -3;;-1]
Out[9]= {"d", "e", "f"}

In[10]:= StringPart["abcde", 5;;1;;-1]
Out[10]= {"e", "d", "c", "b", "a"}

In[11]:= StringPart[x, 1]
Out[11]= StringPart[x, 1]
```

#### StringTake

Gives a substring of a string.

- `StringTake["string", n]`: Gives a string containing the first n characters.
- `StringTake["string", -n]`: Gives the last n characters.
- `StringTake["string", {n}]`: Gives the nth character.
- `StringTake["string", {m, n}]`: Gives characters m through n.
- `StringTake["string", {m, n, s}]`: Gives characters m through n in steps of s.
- `StringTake["string", UpTo[n]]`: Gives n characters, or as many as are available.
- `StringTake[{s1, s2, ...}, spec]`: Gives the list of results for each si.
- `StringTake["string", 0]`: Returns the empty string `""`.
- Negative indices count from the end of the string.
- In `StringTake["string", {m, n, s}]`, m, n, and/or s can be negative.
- Returns unevaluated for invalid arguments or out-of-bounds counts.
- **Attributes**: `Protected`.

```mathematica
In[1]:= StringTake["abcdefghijklm", 6]
Out[1]= "abcdef"

In[2]:= StringTake["abcdefghijklm", -4]
Out[2]= "jklm"

In[3]:= StringTake["abcdefghijklm", {5, 10}]
Out[3]= "efghij"

In[4]:= StringTake["abcdefghijklm", {6}]
Out[4]= "f"

In[5]:= StringTake["abcdefghijklm", {1, -1, 2}]
Out[5]= "acegikm"

In[6]:= StringTake[{"abcdef", "stuv", "xyzw"}, -2]
Out[6]= {"ef", "uv", "zw"}

In[7]:= StringTake["abc", UpTo[4]]
Out[7]= "abc"

In[8]:= StringTake["abcdef", {-3, -1}]
Out[8]= "def"

In[9]:= StringTake["abcde", {5, 1, -1}]
Out[9]= "edcba"

In[10]:= StringTake["abc", 0]
Out[10]= ""

In[11]:= StringTake[x, 1]
Out[11]= StringTake[x, 1]
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
Out[2]= 2 - x + (-1 - x/3)/(1 + x + x^2) + 1/(9 (-1 + x)) - 34/(9 (2 + x))

In[3]:= Apart[(x+y)/((x+1)(y+1)(x-y)), x]
Out[3]= 2 y/((1 + y)^2 (x - y)) - (-1 + y)/((1 + x) (1 + y)^2)
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
- `PrimeQ[z]`: For a Gaussian integer `z = a + b I`, returns `True` if `z` is a Gaussian prime.

**Features**:
- `Listable`, `Protected`.
- A Gaussian integer `a + b I` is a Gaussian prime if:
  - Both `a` and `b` are nonzero and `a^2 + b^2` is an ordinary prime, or
  - One of `a`, `b` is zero and the absolute value of the other is a prime congruent to 3 mod 4.
- Returns `False` for non-integer complex numbers, rationals, reals, and strings.

```mathematica
In[1]:= PrimeQ[7]
Out[1]= True

In[2]:= PrimeQ[1 + I]
Out[2]= True

In[3]:= PrimeQ[1 + 2 I]
Out[3]= True

In[4]:= PrimeQ[3 I]
Out[4]= True

In[5]:= PrimeQ[5 I]
Out[5]= False

In[6]:= PrimeQ[2 + 2 I]
Out[6]= False
```

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
- `FactorInteger[n, Method -> method]`: Factors `n` using a specific algorithmic method.

**Options for `Method`**:
- `"Automatic"`: (Default) Attempts factorization by sequentially executing Trial Division, Pollard's Rho, and ECM.
- `"TrialDivision"`: Extracts bounds matching the first 1000 computed primes and halts cleanly. Composite residues are preserved.
- `"PollardRho"`: Executes the Brent cycle-finding variant of Pollard's Rho algorithm targeting GMP bignums.
- `{"BlakeRationalBaseDescent", "Base" -> a/b}`: Executes the Rational Base Descent algorithm specifically targeting semiprimes proximate to powers of `a/b`.
- `"PollardP-1"`: Executes the Pollard $P-1$ algorithm, leveraging GMP and ECM natively.
- `"WilliamsP+1"`: Executes the Williams $P+1$ algorithm via the ECM library natively.
- `"Fermat"`: Explores factors symmetrically close to the square root boundary natively on large integers.
- `"CFRAC"`: Implements the continued fraction integer factorization method natively on GMP bignums.
- `"ShanksSquareForms"`: Implements Shanks's Square Forms Factorization (SQUFOF) natively on large integers. Halts explicitly if factors are not discovered within the loop constraints.
- `"ECM"`: Explicitly triggers Elliptic-Curve Method discovery natively.

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

#### Rational
Represents a rational number.
- `Rational[n, d]`

**Features**:
- Automatically simplifies to lowest terms (e.g. `Rational[15, 5]` evaluates to `3`, `Rational[2, 4]` evaluates to `Rational[1, 2]`).
- Returns `Indeterminate` when `n` and `d` are both `0` (e.g. `Rational[0, 0]`).
- Returns `ComplexInfinity` when `n` is non-zero and `d` is `0` (e.g. `Rational[1, 0]`).

```mathematica
In[1]:= Rational[15, 5]
Out[1]= 3
```

#### Re, Im, ReIm, Abs, Conjugate, Arg
Complex number functions.
- `Re[z]`: Real part.
- `Im[z]`: Imaginary part.
- `ReIm[z]`: Returns `{Re[z], Im[z]}`.
- `Abs[z]`: Magnitude.
- `Conjugate[z]`: Complex conjugate.
- `Arg[z]`: Phase angle.

### Power Series

#### SeriesData
Represents an explicit truncated power series.
- `SeriesData[x, x0, {a0, a1, ..., a_{k-1}}, nmin, nmax, den]`

The `ai` are the coefficients of the series about the expansion point `x0`.
The power of `(x - x0)` attached to `ai` is `(nmin + i)/den`, and a trailing
`O[x - x0]^(nmax/den)` term indicates the order at which higher terms have
been dropped.

**Features**:
- `Protected`.
- `SeriesData` is a pure data head; it has no evaluator and is normally
  produced by `Series`.
- Standard printing renders the series as an ordinary mathematical sum:
  `a0 + a1 (x - x0) + a2 (x - x0)^2 + ... + O[x - x0]^p`. Zero
  coefficients are suppressed, and `x0 == 0` is displayed as simply `x`
  without the subtraction.
- `InputForm[...]` switches to the literal `SeriesData[x, x0, {...},
  nmin, nmax, den]` form, which round-trips through the parser.
- `FullForm[...]` shows the raw tree structure.

```mathematica
In[1]:= SeriesData[x, 0, {1, 1, 1/2, 1/6, 1/24, 1/120}, 0, 6, 1]
Out[1]= 1 + x + 1/2 x^2 + 1/6 x^3 + 1/24 x^4 + 1/120 x^5 + O[x]^6

In[2]:= InputForm[%]
Out[2]= SeriesData[x, 0, {1, 1, 1/2, 1/6, 1/24, 1/120}, 0, 6, 1]

In[3]:= SeriesData[x, 0, Table[i^2, {i, 10}], 0, 10, 1]
Out[3]= 1 + 4 x + 9 x^2 + 16 x^3 + 25 x^4 + 36 x^5 + 49 x^6 + 64 x^7 + 81 x^8 + 100 x^9 + O[x]^10

In[4]:= SeriesData[x, 2, {a, b, c}, 0, 3, 1]
Out[4]= a + b (x - 2) + c (x - 2)^2 + O[x - 2]^3

In[5]:= SeriesData[x, 0, {1, 2, 3}, 1, 7, 2]
Out[5]= Sqrt[x] + 2 x + 3 x^(3/2) + O[x]^(7/2)
```

#### Series
Produces the power-series expansion of an expression about a point.
- `Series[f, {x, x0, n}]` — Taylor/Laurent/Puiseux expansion up to order `(x - x0)^n`.
- `Series[f, x -> x0]` — leading-term form, emitting the first non-trivial part plus `O[x - x0]^2`.
- `Series[f, {x, x0, nx}, {y, y0, ny}, ...]` — iterated multivariate expansion. Each inner coefficient is itself a `SeriesData` in the next variable.
- `Series[f, {x, Infinity, n}]` — expansion at infinity, substituting `x -> 1/u` internally. The emitted `SeriesData` uses `Power[x, -1]` as its variable, so the series prints with `1/x` as the base and an `O[1/x]^(n+1)` term.

**Features**:
- `HoldAll` and `Protected` (so the expansion variable is not evaluated before `Series` has a chance to shield it).
- Threaded over lists: `Series[{f1, f2, ...}, spec]` becomes `{Series[f1, spec], Series[f2, spec], ...}`.
- Handles Taylor expansions for smooth functions, Laurent expansions where the function has a pole at `x0`, Puiseux expansions for fractional-power cases such as `Sqrt[Sin[x]]`, and logarithmic expansions for cases like `x^x` where `Log[x]` survives as a symbolic coefficient.
- Symbolic parameters in exponents are supported: `Series[(1 + x)^n, {x, 0, 4}]` returns the binomial expansion with `n` kept unexpanded.
- Approximate numeric coefficients flow through series arithmetic unchanged.
- For unknown heads (e.g. `f[x]` where `f` has no rules), the engine falls back to naive Taylor via `D` at the expansion point; the coefficients appear as `Derivative[k][f][x0]`.

**Coefficient arithmetic** automatically promotes to BigInt-backed `Rational` when 64-bit numerators or denominators would overflow, so previously-overflowing Laurent/Puiseux cases like `Series[1/Sin[x]^10, {x, 0, 2}]` and `Series[Sqrt[Log[1 + x]], {x, 0, 12}]` now produce exact coefficients (at the cost of slower evaluation for large orders).

Inverse trigonometric and inverse hyperbolic heads (`ArcSin`, `ArcCos`, `ArcTan`, `ArcCot`, `ArcSinh`, `ArcCosh`, `ArcTanh`, `ArcCoth`) are handled by direct series kernels at `u = 0` rather than by naive repeated differentiation, which would blow up expression size exponentially for higher orders. `ArcCosh` uses the principal-branch identity `ArcCosh[u] = I*ArcCos[u]`, so its expansion at `x = 0` has the expected `I*Pi/2` constant term and imaginary coefficients.

Forward reciprocal heads (`Sec`, `Csc`, `Cot`, `Sech`, `Csch`, `Coth`) are rewritten as `1/Cos[x]`, `1/Sin[x]`, `Cos[x]/Sin[x]`, etc., before expansion. Inverse reciprocal heads (`ArcSec`, `ArcCsc`, `ArcSech`, `ArcCsch`) are rewritten via the identities `ArcSec[z] = ArcCos[1/z]`, `ArcCsc[z] = ArcSin[1/z]`, `ArcSech[z] = ArcCosh[1/z]`, `ArcCsch[z] = ArcSinh[1/z]`, so that a blowing-up inner series (e.g. `z = 1/x`) collapses to a convergent kernel case rather than triggering spurious `Power::infy` warnings.

Expansions where the inner series diverges at the expansion point (e.g. `Series[f[1/x], {x, 0, n}]`) are handled via dedicated at-infinity identities:
- `ArcCoth[1/u] = ArcTanh[u]`, `ArcCot[1/u] = ArcTan[u]` (handled at the series level via inner-series inversion).
- `ArcTanh[1/u] = I*Pi/2 + ArcTanh[u]` (principal branch).
- `ArcSinh[1/v] = -Log[v] + Log[1 + Sqrt[1 + v^2]]` and `ArcCosh[1/v] = -Log[v] + Log[1 + Sqrt[1 - v^2]]` (handled by rewriting at the expression level; the symbolic `-Log[x]` term rides the existing `Log[x]` symbolic-coefficient path).

**Internal padding for symbolic expansion points**: The engine computes series at a padded internal order (user order + 12 by default) so that intermediate Laurent/Puiseux operations don't lose accuracy. When the expansion point `x0` is not a literal number, padding is capped at 2 — at a symbolic point the series coefficients are themselves symbolic expressions (e.g. `Cosh[a]`, `Sinh[a]`), and the `O(N^2)` convolution inside `so_inv`/`so_div` would otherwise spin indefinitely on exponentially growing expression trees. This makes cases like `Series[Coth[x], {x, a, 1}]`, `Series[Tanh[x], {x, a, 1}]`, `Series[Sec[x], {x, a, 1}]`, and `Series[1/Cosh[x], {x, a, 1}]` terminate in milliseconds.

**Known limitation**: `Series` does not yet emit Puiseux expansions at square-root-style branch points (e.g. `ArcCosh[x + 1]` near `x = 0`); such inputs are returned unevaluated rather than risking infinite-loop or incorrect output. The naive-Taylor fallback also caps iterations and bails out on `Infinity`/`Indeterminate` derivatives so unknown heads cannot spin the engine.

```mathematica
In[1]:= Series[Exp[x], {x, 0, 10}]
Out[1]= 1 + x + x^2/2 + x^3/6 + x^4/24 + x^5/120 + x^6/720 + x^7/5040 + x^8/40320 + x^9/362880 + x^10/3628800 + O[x]^11

In[2]:= Series[f[x], {x, a, 3}]
Out[2]= f[a] + Derivative[1][f][a] (x - a) + 1/2 Derivative[2][f][a] (x - a)^2 + 1/6 Derivative[3][f][a] (x - a)^3 + O[x - a]^4

In[3]:= Series[Cos[x]/x, {x, 0, 10}]
Out[3]= 1/x - x/2 + x^3/24 - x^5/720 + x^7/40320 - x^9/3628800 + O[x]^11

In[4]:= Series[Sqrt[Sin[x]], {x, 0, 10}]
Out[4]= Sqrt[x] - x^(5/2)/12 + x^(9/2)/1440 - x^(13/2)/24192 - 67 x^(17/2)/29030400 + O[x]^(21/2)

In[5]:= Series[x^x, {x, 0, 4}]
Out[5]= 1 + Log[x] x + Log[x]^2/2 x^2 + Log[x]^3/6 x^3 + Log[x]^4/24 x^4 + O[x]^5

In[6]:= Series[(1 + x)^n, {x, 0, 4}]
Out[6]= 1 + n x + 1/2 n (-1 + n) x^2 + 1/6 n (-2 + n) (-1 + n) x^3 + 1/24 n (-3 + n) (-2 + n) (-1 + n) x^4 + O[x]^5

In[7]:= Series[Sin[1/x], {x, Infinity, 10}]
Out[7]= 1/x - 1/6 (1/x)^3 + 1/120 (1/x)^5 - 1/5040 (1/x)^7 + 1/362880 (1/x)^9 + O[1/x]^11

In[8]:= Series[Sin[x + y], {x, 0, 3}, {y, 0, 3}]
Out[8]= (y - y^3/6 + O[y]^4) + (1 - y^2/2 + O[y]^4) x + (-y/2 + y^3/12 + O[y]^4) x^2 + (-1/6 + y^2/12 + O[y]^4) x^3 + O[x]^4

In[9]:= Series[{Sin[x], Cos[x], Tan[x]}, {x, 0, 5}]
Out[9]= {x - x^3/6 + x^5/120 + O[x]^6, 1 - x^2/2 + x^4/24 + O[x]^6, x + x^3/3 + 2 x^5/15 + O[x]^6}
```

#### Normal
Converts a `SeriesData` back into an ordinary expression by dropping its O-term.
- `Normal[expr]`

**Features**:
- `Protected`.
- Returns the Plus of the coefficient-times-power terms (zero coefficients skipped). For non-`SeriesData` input, `Normal` is the identity.

```mathematica
In[1]:= Normal[Series[Exp[x], {x, 0, 5}]]
Out[1]= 1 + x + x^2/2 + x^3/6 + x^4/24 + x^5/120

In[2]:= Normal[a + b]
Out[2]= a + b
```

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


#### TrigToExp
Converts trigonometric and hyperbolic functions in `expr` to exponentials.
- `TrigToExp[expr]`

**Features**:
- `Listable`, `Protected`.
- Operates on both circular and hyperbolic functions, and their inverses.
- Automatically threads over lists, equations, inequalities, and logic functions.

```mathematica
In[1]:= TrigToExp[Cos[x]]
Out[1]= 1/2 E^(-I x) + 1/2 E^(I x)

In[2]:= TrigToExp[ArcSin[x]]
Out[2]= -I Log[I x + Sqrt[1 - x^2]]

In[3]:= TrigToExp[{Sin[x], Cos[x], Tan[x]}]
Out[3]= {(-1/2*I) E^((I) x) + (1/2*I) E^((-I) x), 1/2 E^((-I) x) + 1/2 E^((I) x), (-I) E^((I) x)/(E^((-I) x) + E^((I) x)) + (I) E^((-I) x)/(E^((-I) x) + E^((I) x))}
```

#### ExpToTrig
Converts exponentials in `expr` to trigonometric or hyperbolic functions.
- `ExpToTrig[expr]`

**Features**:
- `Listable`, `Protected`.
- Tries when possible to give results that do not involve explicit complex numbers.
- ExpToTrig is natively the inverse of `TrigToExp`.
- Automatically threads over lists, equations, inequalities, and logic functions.

```mathematica
In[1]:= ExpToTrig[Exp[I x]]
Out[1]= Cos[x] + I Sin[x]

In[2]:= ExpToTrig[Log[1 + I x] - Log[1 - I x]]
Out[2]= 2 I ArcTan[x]

In[3]:= ExpToTrig[Exp[I x] == -1]
Out[3]= Cos[x] + I Sin[x] == -1
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

#### Tuples
Generates a list of all possible combinations or tuples of elements.
- `Tuples[list, n]`: generates a list of all possible n-tuples of elements from `list`.
- `Tuples[{list1, list2, ...}]`: generates a list of all possible tuples whose *i*-th element is from `listi`.
- `Tuples[list, {n1, n2, ...}]`: generates a list of all possible $n_1 \times n_2 \times \dots$ arrays of elements in `list`.

**Features**:
- `Protected`.
- The elements of `list` are treated as distinct.
- The object `list` need not have head `List`. The head at each level in the arrays generated by `Tuples` will be the same as the head of `list`.

```mathematica
In[1]:= Tuples[{0, 1}, 3]
Out[1]= {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {1, 0, 1}, {1, 1, 0}, {1, 1, 1}}

In[2]:= Tuples[{{a, b}, {1, 2, 3, 4}, {x}}]
Out[2]= {{a, 1, x}, {a, 2, x}, {a, 3, x}, {a, 4, x}, {b, 1, x}, {b, 2, x}, {b, 3, x}, {b, 4, x}}

In[3]:= Tuples[{a, b}, {2, 2}]
Out[3]= {{{{a, a}, {a, a}}, {{a, a}, {a, b}}}, {{{a, a}, {b, a}}, {{a, a}, {b, b}}}, {{{a, b}, {a, a}}, {{a, b}, {a, b}}}, {{{a, b}, {b, a}}, {{a, b}, {b, b}}}, {{{b, a}, {a, a}}, {{b, a}, {a, b}}}, {{{b, a}, {b, a}}, {{b, a}, {b, b}}}, {{{b, b}, {a, a}}, {{b, b}, {a, b}}}, {{{b, b}, {b, a}}, {{b, b}, {b, b}}}}

In[4]:= Tuples[f[x, y, z], 2]
Out[4]= {f[x, x], f[x, y], f[x, z], f[y, x], f[y, y], f[y, z], f[z, x], f[z, y], f[z, z]}
```


#### Permutations
Generates a list of all possible permutations of the elements in `list`.
- `Permutations[list]`: generates a list of all possible permutations of the elements in `list`.
- `Permutations[list, n]`: gives all permutations containing at most `n` elements.
- `Permutations[list, {n}]`: gives all permutations containing exactly `n` elements.

**Features**:
- `Protected`.
- There are $n!$ permutations of a list of $n$ distinct elements.
- Repeated elements are treated as identical.
- The object `list` need not have head `List`.
- `Permutations[list]` is effectively equivalent to `Permutations[list, {Length[list]}]`.
- `Permutations[list, {n_min, n_max}]` gives permutations of `list` between `n_min` and `n_max` elements.
- `Permutations[list, {n_min, n_max, dn}]` uses step `dn`.
- `Permutations[list, All]` is equivalent to `Permutations[list, {0, Length[list]}]`.

```mathematica
In[1]:= Permutations[{a, b, c}]
Out[1]= {{a, b, c}, {a, c, b}, {b, a, c}, {b, c, a}, {c, a, b}, {c, b, a}}

In[2]:= Permutations[{a, b, c, d}, {3}]
Out[2]= {{a, b, c}, {a, b, d}, {a, c, b}, {a, c, d}, {a, d, b}, {a, d, c}, {b, a, c}, {b, a, d}, {b, c, a}, {b, c, d}, {b, d, a}, {b, d, c}, {c, a, b}, {c, a, d}, {c, b, a}, {c, b, d}, {c, d, a}, {c, d, b}, {d, a, b}, {d, a, c}, {d, b, a}, {d, b, c}, {d, c, a}, {d, c, b}}

In[3]:= Permutations[{a, a, b}]
Out[3]= {{a, a, b}, {a, b, a}, {b, a, a}}

In[4]:= Permutations[{x, x^2, x + 1}]
Out[4]= {{x, x^2, 1 + x}, {x, 1 + x, x^2}, {x^2, x, 1 + x}, {x^2, 1 + x, x}, {1 + x, x, x^2}, {1 + x, x^2, x}}

In[5]:= Permutations[Range[3], All]
Out[5]= {{}, {1}, {2}, {3}, {1, 2}, {1, 3}, {2, 1}, {2, 3}, {3, 1}, {3, 2}, {1, 2, 3}, {1, 3, 2}, {2, 1, 3}, {2, 3, 1}, {3, 1, 2}, {3, 2, 1}}

In[6]:= Permutations[Range[4], {4, 0, -2}]
Out[6]= {{1, 2, 3, 4}, {1, 2, 4, 3}, {1, 3, 2, 4}, {1, 3, 4, 2}, {1, 4, 2, 3}, {1, 4, 3, 2}, {2, 1, 3, 4}, {2, 1, 4, 3}, {2, 3, 1, 4}, {2, 3, 4, 1}, {2, 4, 1, 3}, {2, 4, 3, 1}, {3, 1, 2, 4}, {3, 1, 4, 2}, {3, 2, 1, 4}, {3, 2, 4, 1}, {3, 4, 1, 2}, {3, 4, 2, 1}, {4, 1, 2, 3}, {4, 1, 3, 2}, {4, 2, 1, 3}, {4, 2, 3, 1}, {4, 3, 1, 2}, {4, 3, 2, 1}, {1, 2}, {1, 3}, {1, 4}, {2, 1}, {2, 3}, {2, 4}, {3, 1}, {3, 2}, {3, 4}, {4, 1}, {4, 2}, {4, 3}, {}}

In[7]:= Permutations[f[a, b, c]]
Out[7]= {f[a, b, c], f[a, c, b], f[b, a, c], f[b, c, a], f[c, a, b], f[c, b, a]}
```
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

#### Distribute
Implements the distributive law for operators `f` and `g`.
- `Distribute[expr]`: distributes the head of `expr` over `Plus` appearing in any of its arguments.
- `Distribute[expr, g]`: distributes over `g`.
- `Distribute[expr, g, f]`: performs the distribution only if the head of `expr` is `f`.
- `Distribute[expr, g, f, gp, fp]`: gives `gp` and `fp` in place of `g` and `f` respectively in the result of the distribution.

**Features**:
- `Protected`.
- `Distribute` explicitly constructs the complete result of a distribution; `Expand`, on the other hand, builds up results iteratively, simplifying at each stage.
- For pure products, `Distribute` gives the same results as `Expand`.

```mathematica
In[1]:= Distribute[(a + b) . (x + y + z)]
Out[1]= a . x + a . y + a . z + b . x + b . y + b . z

In[2]:= Distribute[f[a + b, c + d + e]]
Out[2]= f[a, c] + f[a, d] + f[a, e] + f[b, c] + f[b, d] + f[b, e]

In[3]:= Distribute[(a + b + c) (u + v), Plus, Times]
Out[3]= a u + a v + b u + b v + c u + c v

In[4]:= Distribute[{{a, b}, {x, y, z}, {s, t}}, List]
Out[4]= {{a, x, s}, {a, x, t}, {a, y, s}, {a, y, t}, {a, z, s}, {a, z, t}, {b, x, s}, {b, x, t}, {b, y, s}, {b, y, t}, {b, z, s}, {b, z, t}}

In[5]:= Distribute[{{}, {a}}, {{}, {b}}, {{}, {c}}, List, List, List, Join]
Out[5]= {{}, {c}, {b}, {b, c}, {a}, {a, c}, {a, b}, {a, b, c}}
```

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

#### MapAt
Applies a function to selected parts of an expression, identified by position specifications of the same form as those returned by `Position`.

- `MapAt[f, expr, n]`: applies `f` to the element at position `n` in `expr`. If `n` is negative, the position is counted from the end. `n = 0` targets the head of `expr`.
- `MapAt[f, expr, {i, j, ...}]`: applies `f` to the part of `expr` at position `{i, j, ...}` (equivalently `expr[[i, j, ...]]`).
- `MapAt[f, expr, {{i1, j1, ...}, {i2, j2, ...}, ...}]`: applies `f` to each of the listed parts of `expr`. If the same position appears more than once, `f` is applied repeatedly to that part.

**Features**:
- `Protected`.
- Path components may be integers (positive or negative), `All` (selects every child at that level), or `Span` expressions such as `i ;; j` or `i ;; j ;; k`.
- Works on expressions with any head (not just `List`); after substitution the evaluator re-applies canonical ordering for `Orderless` heads such as `Plus` and `Times`.
- `MapAt[f, expr, {}]` applies `f` to the entire expression itself.

```mathematica
In[1]:= MapAt[f, {a, b, c, d}, 2]
Out[1]= {a, f[b], c, d}

In[2]:= MapAt[f, {a, b, c, d}, {{1}, {4}}]
Out[2]= {f[a], b, c, f[d]}

In[3]:= MapAt[f, {{a, b, c}, {d, e}}, {2, 1}]
Out[3]= {{a, b, c}, {f[d], e}}

In[4]:= MapAt[f, {{a, b, c}, {d, e}}, {All, 2}]
Out[4]= {{a, f[b], c}, {d, f[e]}}

In[5]:= MapAt[h, {{a, b, c}, {d, e}, f, g}, -3]
Out[5]= {{a, b, c}, h[{d, e}], f, g}

In[6]:= MapAt[h, {{a, b, c}, {d, e}, f, g}, {2, 1}]
Out[6]= {{a, b, c}, {h[d], e}, f, g}

In[7]:= MapAt[h, {{a, b, c}, {d, e}, f, g}, {{2}, {1}}]
Out[7]= {h[{a, b, c}], h[{d, e}], f, g}

In[8]:= MapAt[h, {{a, b, c}, {d, e}, f, g}, {{1, 1}, {2, 2}, {3}}]
Out[8]= {{h[a], b, c}, {d, h[e]}, h[f], g}

In[9]:= MapAt[f, {1, 2, 3, 4, 5, 6}, 3 ;; 4]
Out[9]= {1, 2, f[3], f[4], 5, 6}

In[10]:= MapAt[f, a + b + c + d, 2]
Out[10]= a + c + d + f[b]

In[11]:= MapAt[f, x^2 + y^2, {{1, 1}, {2, 1}}]
Out[11]= f[x]^2 + f[y]^2

In[12]:= MapAt[f, {a, b, c}, 0]
Out[12]= f[List][a, b, c]
```

#### Nest
Applies a function repeatedly.
- `Nest[f, expr, n]`: gives an expression with `f` applied `n` times to `expr`.

**Features**:
- `Protected`.
- `n` must be a non-negative integer; `Nest[f, expr, 0]` returns `expr` unchanged.
- The function `f` may be a symbol, a built-in, or a pure function (`... &`).
- Each iteration evaluates `f[current]` before proceeding, so numeric computations collapse immediately.
- Returns unevaluated if `n` is not a non-negative integer or the argument count is wrong.

**Examples**:
```
In[1]:= Nest[f, x, 3]
Out[1]= f[f[f[x]]]

In[2]:= Nest[(1 + #)^2 &, 1, 3]
Out[2]= 676

In[3]:= Nest[(1 + #)^2 &, x, 5]
Out[3]= (1 + (1 + (1 + (1 + (1 + x)^2)^2)^2)^2)^2

In[4]:= Nest[Sqrt, 100.0, 4]
Out[4]= 1.33352

In[5]:= Nest[1/(1 + #) &, x, 5]
Out[5]= 1/(1 + 1/(1 + 1/(1 + 1/(1 + 1/(1 + x)))))

In[6]:= Nest[x^# &, x, 6]
Out[6]= x^x^x^x^x^x^x

In[7]:= Nest[#(1 + 0.05) &, 1000, 10]
Out[7]= 1628.89

In[8]:= Nest[(# + 2/#)/2 &, 1.0, 5]
Out[8]= 1.41421

In[9]:= Nest[{{1, 1}, {1, 0}} . # &, {0, 1}, 10]
Out[9]= {55, 34}
```

#### NestList
Applies a function repeatedly, collecting every intermediate result.
- `NestList[f, expr, n]`: gives a list of the results of applying `f` to `expr` 0 through `n` times.

**Features**:
- `Protected`.
- Returns a list of length `n + 1` whose first element is `expr` and whose `(k+1)`-th element is `f` applied `k` times to `expr`.
- `n` must be a non-negative integer; `NestList[f, expr, 0]` returns `{expr}`.
- The function `f` may be a symbol, a built-in, or a pure function (`... &`).
- Each iteration evaluates `f[current]` before proceeding, so numeric computations collapse immediately.
- Returns unevaluated if `n` is not a non-negative integer or the argument count is wrong.
- `Last[NestList[f, expr, n]]` is equivalent to `Nest[f, expr, n]`.

**Examples**:
```
In[1]:= NestList[f, x, 4]
Out[1]= {x, f[x], f[f[x]], f[f[f[x]]], f[f[f[f[x]]]]}

In[2]:= NestList[Cos, 1.0, 10]
Out[2]= {1., 0.540302, 0.857553, 0.65429, 0.79348, 0.701369, 0.76396, 0.722102, 0.750418, 0.731404, 0.744237}

In[3]:= NestList[(1 + #)^2 &, x, 3]
Out[3]= {x, (1 + x)^2, (1 + (1 + x)^2)^2, (1 + (1 + (1 + x)^2)^2)^2}

In[4]:= NestList[Sqrt, 100.0, 4]
Out[4]= {100., 10., 3.16228, 1.77828, 1.33352}

In[5]:= NestList[2 # &, 1, 10]
Out[5]= {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024}

In[6]:= NestList[# + 1 &, 0, 10]
Out[6]= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

In[7]:= NestList[#^2 &, 2, 6]
Out[7]= {2, 4, 16, 256, 65536, 4294967296, 18446744073709551616}

In[8]:= NestList[#(1 + 0.05) &, 1000, 10]
Out[8]= {1000, 1050., 1102.5, 1157.63, 1215.51, 1276.28, 1340.1, 1407.1, 1477.46, 1551.33, 1628.89}

In[9]:= NestList[(# + 2/#)/2 &, 1.0, 5]
Out[9]= {1., 1.5, 1.41667, 1.41422, 1.41421, 1.41421}

In[10]:= NestList[If[EvenQ[#], #/2, (3 # + 1)/2] &, 100, 20]
Out[10]= {100, 50, 25, 38, 19, 29, 44, 22, 11, 17, 26, 13, 20, 10, 5, 8, 4, 2, 1, 2, 1}

In[11]:= NestList[Mod[59 #, 101] &, 1, 15]
Out[11]= {1, 59, 47, 46, 88, 41, 96, 8, 68, 73, 65, 98, 25, 61, 64, 39}
```

#### NestWhile
Iteratively applies a function while a predicate continues to yield `True`.
- `NestWhile[f, expr, test]`: Starts with `expr` and keeps applying `f` until `test` no longer yields `True`.
- `NestWhile[f, expr, test, m]`: Supplies the most recent `m` results (not wrapped in a list) as arguments to `test`.
- `NestWhile[f, expr, test, All]`: Supplies every result so far as arguments to `test`.
- `NestWhile[f, expr, test, {mmin, mmax}]`: Defers the first test until `mmin` results exist, then supplies up to `mmax` most-recent results.
- `NestWhile[f, expr, test, m, max]`: Caps the number of `f` applications at `max` (may be `Infinity`).
- `NestWhile[f, expr, test, m, max, n]`: After the loop terminates, applies `f` an additional `n` times.
- `NestWhile[f, expr, test, m, max, -n]`: Returns the result produced `n` applications before the loop ended (i.e. `Part[NestWhileList[...], -n-1]`).

**Features**:
- `Protected`.
- If `test[expr]` does not yield `True` initially, the unchanged `expr` is returned.
- Results passed to `test` are in generation order with the most recent last, so e.g. `# > 1 &` inspects the oldest when more than one result is supplied.
- `NestWhile[f, expr, UnsameQ, 2]` is equivalent to `FixedPoint[f, expr]`.
- `NestWhile[f, expr, UnsameQ, All]` continues until any prior value reappears.
- `m` must be a positive integer, `All`, or a 2-element list `{mmin, mmax}` with `1 <= mmin <= mmax` (or `mmax = Infinity`); `max` must be a non-negative integer or `Infinity`; `n` must be an integer. Malformed specs leave `NestWhile` unevaluated.
- Pure functions (`... &`) are supported for both `f` and `test`.

**Examples**:
```
In[1]:= NestWhile[#/2 &, 123456, EvenQ]
Out[1]= 1929

In[2]:= NestWhile[Log, 100., # > 0 &]
Out[2]= -0.859384

In[3]:= NestWhile[Floor[#/2] &, 10, UnsameQ, 2]
Out[3]= 0

In[4]:= NestWhile[#/2 &, 123456, EvenQ, 1, 4]
Out[4]= 7716

In[5]:= NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity]
Out[5]= 1

In[6]:= NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity, 1]
Out[6]= 0

In[7]:= NestWhile[Floor[#/2] &, 20, # > 1 &, 1, Infinity, -1]
Out[7]= 2

In[8]:= NestWhile[# + 1 &, 888, !PrimeQ[#] &]
Out[8]= 907

In[9]:= NestWhile[# + 1 &, 888, !PrimeQ[#1] || !PrimeQ[#3] &, 3]
Out[9]= 1021

In[10]:= NestWhile[Mod[# + 3, 7] &, 0, UnsameQ, All]
Out[10]= 0

In[11]:= NestWhile[If[EvenQ[#], #/2, 3 # + 1] &, 27, # != 1 &]
Out[11]= 1
```

#### NestWhileList
Like `NestWhile`, but returns the full list of intermediate results.
- `NestWhileList[f, expr, test]`: Generates `{expr, f[expr], f[f[expr]], ...}`, continuing while `test` applied to the most recent result yields `True`.
- `NestWhileList[f, expr, test, m]`: Supplies the most recent `m` results (not wrapped in a list) as arguments to `test`.
- `NestWhileList[f, expr, test, All]`: Supplies every result so far as arguments to `test`.
- `NestWhileList[f, expr, test, {mmin, mmax}]`: Defers the first test until `mmin` results exist, then supplies up to `mmax` most-recent results.
- `NestWhileList[f, expr, test, m, max]`: Caps the number of `f` applications at `max` (may be `Infinity`).
- `NestWhileList[f, expr, test, m, max, n]`: Appends `n` additional applications of `f` to the list after the loop terminates.
- `NestWhileList[f, expr, test, m, max, -n]`: Drops the last `n` elements from the list.

**Features**:
- `Protected`.
- Results are listed in generation order, including the final element on which `test` yielded a non-`True` value (or the last element produced when `max` iterations were reached).
- If `test[expr]` does not yield `True` initially, the result is just `{expr}`.
- `NestWhileList[f, expr, UnsameQ, 2]` is equivalent to `FixedPointList[f, expr]`.
- `NestWhileList[f, expr, test, All]` is equivalent to `NestWhileList[f, expr, test, {1, Infinity}]`.
- `NestWhileList[f, expr, UnsameQ, All]` continues until a previously-seen value reappears, and the repeat is included as the last element of the list.
- `m` must be a positive integer, `All`, or a 2-element list `{mmin, mmax}` with `1 <= mmin <= mmax` (or `mmax = Infinity`); `max` must be a non-negative integer or `Infinity`; `n` must be an integer. Malformed specs leave `NestWhileList` unevaluated.
- Pure functions (`... &`) are supported for both `f` and `test`.

**Examples**:
```
In[1]:= NestWhileList[#/2 &, 123456, EvenQ]
Out[1]= {123456, 61728, 30864, 15432, 7716, 3858, 1929}

In[2]:= NestWhileList[Log, 100., # > 0 &]
Out[2]= {100.0, 4.60517, 1.52718, 0.423423, -0.859384}

In[3]:= NestWhileList[Floor[#/2] &, 20, UnsameQ, 2, 4]
Out[3]= {20, 10, 5, 2, 1}

In[4]:= NestWhileList[Floor[#/2] &, 20, # > 1 &, 1, Infinity]
Out[4]= {20, 10, 5, 2, 1}

In[5]:= NestWhileList[Floor[#/2] &, 20, # > 1 &, 1, Infinity, 1]
Out[5]= {20, 10, 5, 2, 1, 0}

In[6]:= NestWhileList[Floor[#/2] &, 20, # > 1 &, 1, Infinity, -1]
Out[6]= {20, 10, 5, 2}

In[7]:= NestWhileList[# + 1 &, 899, !PrimeQ[#] &]
Out[7]= {899, 900, 901, 902, 903, 904, 905, 906, 907}

In[8]:= NestWhileList[Mod[2 #, 19] &, 2, # != 1 &]
Out[8]= {2, 4, 8, 16, 13, 7, 14, 9, 18, 17, 15, 11, 3, 6, 12, 5, 10, 1}

In[9]:= NestWhileList[Mod[5 #, 7] &, 4, Unequal, All]
Out[9]= {4, 6, 2, 3, 1, 5, 4}

In[10]:= NestWhileList[If[EvenQ[#], #/2, (3 # + 1)/2] &, 400, Unequal, All]
Out[10]= {400, 200, 100, 50, 25, 38, 19, 29, 44, 22, 11, 17, 26, 13, 20, 10, 5, 8, 4, 2, 1, 2}

In[11]:= NestWhileList[If[EvenQ[#], #/2, (3 # + 1)/2] &, 400, Unequal, All, Infinity, -1]
Out[11]= {400, 200, 100, 50, 25, 38, 19, 29, 44, 22, 11, 17, 26, 13, 20, 10, 5, 8, 4, 2, 1}
```

#### FixedPointList
Generates the list of successive iterates of a function until a fixed point is reached.
- `FixedPointList[f, expr]`: Generates `{expr, f[expr], f[f[expr]], ...}`, continuing until two consecutive results are `SameQ`. The returned list always begins with `expr`, and its last two elements are always equal (the fixed point appears twice).
- `FixedPointList[f, expr, n]`: Stops after at most `n` applications of `f`. If `n` is reached before convergence, the last two elements need not be equal.
- `FixedPointList[f, expr, SameTest -> s]`: Uses the binary predicate `s` instead of `SameQ` to test successive pairs of results. Iteration stops when `s[prev, current]` evaluates to `True`.
- `FixedPointList[f, expr, n, SameTest -> s]`: Combines a bounded iteration count with a custom equivalence test.

**Features**:
- `Protected`.
- `FixedPointList[f, expr]` is equivalent to `NestWhileList[f, expr, UnsameQ, 2]`.
- `n` (when given) must be a non-negative integer or `Infinity`. Malformed argument specs leave `FixedPointList` unevaluated.

```mathematica
In[1]:= FixedPointList[1 + Floor[#/2] &, 1000]
Out[1]= {1000, 501, 251, 126, 64, 33, 17, 9, 5, 3, 2, 2}

In[2]:= 1 + Floor[Last[%]/2]
Out[2]= 2

In[3]:= FixedPointList[# /. {a_, b_} /; b != 0 -> {b, Mod[a, b]} &, {28, 21}]
Out[3]= {{28, 21}, {21, 7}, {7, 0}, {7, 0}}

In[4]:= GCD[28, 21]
Out[4]= 7

In[5]:= FixedPointList[1 + Floor[#/2] &, 1000, 5]
Out[5]= {1000, 501, 251, 126, 64, 33}

In[6]:= FixedPointList[(# + 2/#)/2 &, 1.0]
Out[6]= {1.0, 1.5, 1.41667, 1.41422, 1.41421, 1.41421, 1.41421}

In[7]:= FixedPointList[(# + 2/#)/2 &, 1.0, SameTest -> (Abs[#1 - #2] < 0.01 &)]
Out[7]= {1.0, 1.5, 1.41667, 1.41422}
```

#### Fold
Successively applies a binary function to an accumulating seed and the elements of a list.
- `Fold[f, x, list]`: Returns the last element of `FoldList[f, x, list]`, namely `f[...f[f[f[x, list[[1]]], list[[2]]], list[[3]]]..., list[[n]]]`.
- `Fold[f, list]`: Equivalent to `Fold[f, First[list], Rest[list]]`.

**Features**:
- `Protected`.
- The head of the third argument need not be `List` (any compound expression is accepted).
- `Fold[f, x, {}]` returns `x` (the function is never applied); `Fold[f, {a}]` returns `a`.
- `Fold[f, {}]` remains unevaluated (no seed, no elements).
- Each intermediate application is evaluated before the next one.

```mathematica
In[1]:= Fold[f, x, {a, b, c, d}]
Out[1]= f[f[f[f[x, a], b], c], d]

In[2]:= Fold[List, x, {a, b, c, d}]
Out[2]= {{{{x, a}, b}, c}, d}

In[3]:= Fold[Times, 1, {a, b, c, d}]
Out[3]= a b c d

In[4]:= Fold[f, {a, b, c, d}]
Out[4]= f[f[f[a, b], c], d]

In[5]:= Fold[{2 #1, 3 #2} &, x, {a, b, c, d}]
Out[5]= {{{{16 x, 24 a}, 12 b}, 6 c}, 3 d}

In[6]:= Fold[f, x, p[a, b, c, d]]
Out[6]= f[f[f[f[x, a], b], c], d]

(* Horner-form evaluation: 2^3 + 2 + 1 at x = 2 *)
In[7]:= Fold[2 #1 + #2 &, 0, {1, 0, 1, 1}]
Out[7]= 11

(* Form a number from digits *)
In[8]:= Fold[10 #1 + #2 &, 0, {4, 5, 1, 6, 7, 8}]
Out[8]= 451678

(* Form a continued fraction *)
In[9]:= Fold[1/(#2 + #1) &, x, Reverse[{a, b, c, d}]]
Out[9]= 1/(a + 1/(b + 1/(c + 1/(d + x))))

(* Factorial via Times *)
In[10]:= Fold[Times, 1, Range[5]]
Out[10]= 120

(* When the pure function ignores its second argument, Fold coincides with Nest *)
In[11]:= Fold[f[#1] &, x, Range[5]]
Out[11]= f[f[f[f[f[x]]]]]

(* Edge cases *)
In[12]:= Fold[f, x, {}]
Out[12]= x

In[13]:= Fold[f, {a}]
Out[13]= a

In[14]:= Fold[f, {}]
Out[14]= Fold[f, {}]
```

#### FoldList
Produces the list of intermediate fold values.
- `FoldList[f, x, list]`: Gives `{x, f[x, list[[1]]], f[f[x, list[[1]]], list[[2]]], ...}`.
- `FoldList[f, list]`: Gives `{list[[1]], f[list[[1]], list[[2]]], ...}`.

**Features**:
- `Protected`.
- With a length-`n` list, `FoldList` returns a list of length `n + 1` (or `n` in the no-seed form).
- The head of the third argument is preserved in the output: `FoldList[f, x, p[a, b]]` gives `p[x, f[x, a], f[f[x, a], b]]`.
- `FoldList[f, {}]` returns `{}` (an empty list with the input head); `FoldList[f, x, {}]` returns `{x}`.
- `Fold[f, x, list]` is equivalent to `Last[FoldList[f, x, list]]`.

```mathematica
In[1]:= FoldList[f, x, {a, b, c, d}]
Out[1]= {x, f[x, a], f[f[x, a], b], f[f[f[x, a], b], c], f[f[f[f[x, a], b], c], d]}

In[2]:= FoldList[f, {a, b, c, d}]
Out[2]= {a, f[a, b], f[f[a, b], c], f[f[f[a, b], c], d]}

(* Cumulative sums *)
In[3]:= FoldList[Plus, 0, Range[5]]
Out[3]= {0, 1, 3, 6, 10, 15}

(* Head preservation *)
In[4]:= FoldList[f, x, p[a, b, c, d]]
Out[4]= p[x, f[x, a], f[f[x, a], b], f[f[f[x, a], b], c], f[f[f[f[x, a], b], c], d]]

(* Fold to the right *)
In[5]:= FoldList[g[#2, #1] &, x, {a, b, c, d}]
Out[5]= {x, g[a, x], g[b, g[a, x]], g[c, g[b, g[a, x]]], g[d, g[c, g[b, g[a, x]]]]}

(* Successive factorials *)
In[6]:= FoldList[Times, 1, Range[10]]
Out[6]= {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800}

(* Build up a continued fraction *)
In[7]:= FoldList[1/(#2 + #1) &, x, Reverse[{a, b, c}]]
Out[7]= {x, 1/(c + x), 1/(b + 1/(c + x)), 1/(a + 1/(b + 1/(c + x)))}

(* Build up a number from digits *)
In[8]:= FoldList[10 #1 + #2 &, 0, {4, 5, 1, 6, 7, 8}]
Out[8]= {0, 4, 45, 451, 4516, 45167, 451678}

(* Horner-form polynomial evaluation at x = 2, coefficients {1, 0, 1, 1} *)
In[9]:= FoldList[2 #1 + #2 &, 0, {1, 0, 1, 1}]
Out[9]= {0, 1, 2, 5, 11}

(* Edge cases *)
In[10]:= FoldList[f, x, {}]
Out[10]= {x}

In[11]:= FoldList[f, {}]
Out[11]= {}

In[12]:= FoldList[f, p[]]
Out[12]= p[]
```

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

### Control Flow

#### Do
Evaluates an expression sequentially over an iteration range.
- `Do[expr, n]`: Evaluates `expr` `n` times.
- `Do[expr, {i, imax}]`: Evaluates `expr` with `i` from 1 to `imax`.
- `Do[expr, {i, imin, imax, di}]`: Evaluates `expr` with `i` taking values from `imin` to `imax` in steps of `di`.
- `Do[expr, {i, list}]`: Evaluates `expr` with `i` taking values from `list`.
- `Do[expr, spec1, spec2, ...]`: Evaluates `expr` looping over `spec1` internally.

**Features**:
- `HoldAll`, evaluating its body only after arguments are substituted.
- Employs exact dynamic iteration identical to `Table` but discards the evaluated results, returning `Null`.
- Supports explicit break states (`Return`, `Break`, `Continue`, `Throw`, `Abort`, `Quit`).
- Can execute an infinite loop using `Do[expr, Infinity]`.

```mathematica
In[1]:= Do[Print[i], {i, 3}]
Out[1]= Null

In[2]:= Do[If[i == 3, Break[]]; Print[i], {i, 5}]
Out[2]= Null
```

#### For
Executes a loop with an initialization, condition test, increment, and body.
- `For[start, test, incr, body]`: Evaluates `start`, then repeatedly evaluates `body` and `incr` until `test` fails to give `True`.
- `For[start, test, incr]`: Executes the loop with a `Null` body.

**Features**:
- Evaluates its arguments in a nonstandard way (sequence: `test`, `body`, `incr`).
- Has attribute `HoldAll`.
- `Break[]` exits the loop.
- `Continue[]` skips the rest of the body and proceeds to evaluating `incr`.
- Exits as soon as `test` fails.
- Returns `Null` unless an explicit `Return` is evaluated.

```mathematica
In[1]:= For[i=0, i<4, i++, Print[i]]
Out[1]= Null
```

#### While
Evaluates a test expression and, while it yields `True`, repeatedly evaluates a body expression.
- `While[test, body]`: Evaluates `test`, then `body`, repeatedly, until `test` first fails to give `True`.
- `While[test]`: Executes the loop with a `Null` body. Useful when `test` itself has side-effects.

**Features**:
- Has attribute `HoldAll`; both `test` and `body` are re-evaluated each iteration.
- `Break[]` inside `body` exits the loop, yielding `Null`.
- `Continue[]` inside `body` skips the rest of `body` and returns to re-evaluating `test`.
- `Return[v]` inside `body` causes `While` to yield `v`.
- `Throw`, `Abort`, and `Quit` propagate unchanged.
- If the very first evaluation of `test` is not `True`, `body` is never evaluated.
- Returns `Null` unless an explicit `Return` is issued.

```mathematica
In[1]:= n = 1; While[n < 4, n = n + 1]; n
Out[1]= 4

In[2]:= {a, b} = {27, 6}; While[b != 0, {t1, t2} = {b, Mod[a, b]}; a = t1; b = t2]; a
Out[2]= 3

In[3]:= n = 1; While[True, If[n > 10, Break[]]; n = n + 1]; n
Out[3]= 11
```

#### If
Evaluates condition and executes the corresponding branch.
- `If[condition, t, f]`: Gives `t` if `condition` evaluates to `True`, and `f` if it evaluates to `False`.
- `If[condition, t, f, u]`: Gives `u` if `condition` evaluates to neither `True` nor `False`.

**Features**:
- `HoldRest`, evaluating only the chosen branch.
- Remains unevaluated if the condition is undetermined and `u` is not provided.
- `If[condition, t]` returns `Null` if `condition` evaluates to `False`.

```mathematica
In[1]:= If[True, x, y]
Out[1]= x

In[2]:= If[a < b, 1, 0, Indeterminate]
Out[2]= Indeterminate
```

#### TrueQ
Tests whether an expression evaluates explicitly to `True`.
- `TrueQ[expr]`: Yields `True` if `expr` is `True`, and `False` otherwise.

### Assignment and Rules

#### Set (=), SetDelayed (:=)
- `lhs = rhs`: `Set`. Immediate evaluation of `rhs`.
- `lhs := rhs`: `SetDelayed`. Delayed evaluation of `rhs`.
- `lhs := rhs /; condition`: When the RHS of `SetDelayed` is a `Condition`, it is automatically moved to the LHS pattern. This makes `f[x_] := body /; test` equivalent to `f[x_] /; test := body`.

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

#### ReplacePart
Replaces parts of an expression at specified positions.
- `ReplacePart[expr, i -> new]`: Replaces the part at position `i`.
- `ReplacePart[expr, {i, j, ...} -> new]`: Replaces the part at a nested position.
- `ReplacePart[expr, {{i1, ...} -> new1, {i2, ...} -> new2, ...}]`: Multiple replacements.
- `ReplacePart[expr, pattern -> new]`: Replaces parts at all positions matching `pattern` (e.g., `_`, `Except[...]`).

**Features**:
- `Protected`.
- Supports negative indices: `-1` is the last element, `-2` is second-to-last, etc.
- Supports mixed positive/negative indices in nested position specs (e.g., `{2, -1}`).
- Default `Heads -> False`: pattern-based position specs do not match the head (index 0). Use an explicit `0 -> new` rule to replace the head.
- Supports `Rule` (`->`) and `RuleDelayed` (`:>`).

```mathematica
In[1]:= ReplacePart[{a, b, c, d}, 2 -> x]
Out[1]= {a, x, c, d}

In[2]:= ReplacePart[{a, b, c, d, e, f, g}, -3 -> xxx]
Out[2]= {a, b, c, d, xxx, f, g}

In[3]:= ReplacePart[{a, b, c, d, e, f, g}, Except[1 | 3 | 5] -> xxx]
Out[3]= {a, xxx, c, xxx, e, xxx, xxx}

In[4]:= ReplacePart[f[x], 0 -> g]
Out[4]= g[x]
```

#### ReplaceAt
Replaces parts of an expression at specified positions using replacement rules. Unlike `ReplaceAll` (which traverses the whole tree), `ReplaceAt` only attempts the rules at the part(s) named by the position specification.

- `ReplaceAt[expr, rules, n]`: replaces the `n`-th element using `rules`.
- `ReplaceAt[expr, rules, {i, j, ...}]`: replaces the part of `expr` at position `{i, j, ...}` (equivalently `expr[[i, j, ...]]`).
- `ReplaceAt[expr, rules, {{i1, j1, ...}, {i2, j2, ...}, ...}]`: replaces the parts at each of the listed positions. If the same position appears more than once, the rules are applied repeatedly to that part.

**Features**:
- `Protected`.
- `rules` may be a single `Rule` (`->`), `RuleDelayed` (`:>`), or a list of such rules. The rules are tried in order; the first one that applies wins. If no rule matches at a targeted position, the part is left unchanged.
- For `RuleDelayed`, the right-hand side is evaluated separately for each match after substituting bound pattern variables.
- Negative integer indices count from the end. The literal index `0` targets the head of an expression.
- Path components may be integers, the symbol `All` (selects every child at that level), or `Span` expressions such as `i ;; j` or `i ;; j ;; k`.
- Works on expressions with any head (not just `List`); after substitution the evaluator re-applies canonical ordering for `Orderless` heads such as `Plus` and `Times`.
- The position list uses the same form as is returned by `Position`. `ReplaceAt[expr, rules, {}]` applies the rules to the whole expression.

```mathematica
In[1]:= ReplaceAt[{a, a, a, a}, a -> xx, 2]
Out[1]= {a, xx, a, a}

In[2]:= ReplaceAt[{a, a, a, a}, a -> xx, {{1}, {4}}]
Out[2]= {xx, a, a, xx}

In[3]:= ReplaceAt[{{a, a}, {a, a}}, a -> xx, {2, 1}]
Out[3]= {{a, a}, {xx, a}}

In[4]:= ReplaceAt[{a, a, a, a}, a -> xx, -2]
Out[4]= {a, a, xx, a}

In[5]:= ReplaceAt[{{a, a, a}, {a, a, a}}, a -> xx, {-1, -2}]
Out[5]= {{a, a, a}, {a, xx, a}}

In[6]:= ReplaceAt[{1, 2, 3, 4}, x_ :> 2 x - 1, {{2}, {4}}]
Out[6]= {1, 3, 3, 7}

In[7]:= ReplaceAt[{a, b, c, d}, {a -> xx, _ -> yy}, {{1}, {2}, {4}}]
Out[7]= {xx, yy, c, yy}

In[8]:= ReplaceAt[{{a, a}, {a, a}}, a -> xx, {All, 2}]
Out[8]= {{a, xx}, {a, xx}}

In[9]:= ReplaceAt[{{a, b}, {c, d}, e}, x_ :> f[x], 2]
Out[9]= {{a, b}, f[{c, d}], e}

In[10]:= ReplaceAt[{{a, b}, {c, d}, e}, x_ :> f[x], -1]
Out[10]= {{a, b}, {c, d}, f[e]}

In[11]:= ReplaceAt[{{a, b}, {c, d}, e}, x_ :> f[x], {2, 1}]
Out[11]= {{a, b}, {f[c], d}, e}

In[12]:= ReplaceAt[{{a, b}, {c, d}, e}, x_ :> f[x], {{1}, {3}}]
Out[12]= {f[{a, b}], {c, d}, f[e]}

In[13]:= ReplaceAt[{{a, b}, {c, d}, e}, x_ :> f[x], {{1, 2}, {2, 2}, {3}}]
Out[13]= {{a, f[b]}, {c, f[d]}, f[e]}

In[14]:= ReplaceAt[{a, a, a, a, a}, a -> xx, 2 ;; 4]
Out[14]= {a, xx, xx, xx, a}

In[15]:= ReplaceAt[a + b + c + d, _ -> x, 2]
Out[15]= a + c + d + x

In[16]:= ReplaceAt[x^2 + y^2, _ -> z, {{1, 1}, {2, 1}}]
Out[16]= 2 z^2

In[17]:= ReplaceAt[{a, b, c}, _ -> f, 0]
Out[17]= f[a, b, c]
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

#### Cases
Gives a list of elements that match a pattern.
- `Cases[{e1, e2, ...}, pattern]`
- `Cases[{e1, ...}, pattern -> rhs]`
- `Cases[expr, pattern, levelspec]`
- `Cases[expr, pattern -> rhs, levelspec]`
- `Cases[expr, pattern, levelspec, n]`
- `Cases[pattern]`

**Features**:
- Uses standard level specifications.
- Defaults to level `{1}`.
- Option `Heads -> True` includes heads of expressions.
- Operates in depth-first post-order traversal.

#### Position
Gives a list of the positions at which objects matching a pattern appear.
- `Position[expr, pattern]`
- `Position[expr, pattern, levelspec]`
- `Position[expr, pattern, levelspec, n]`
- `Position[pattern]`

**Features**:
- Defaults to levels `{0, Infinity}` with `Heads -> True`.
- Yields lists of indices in lexicographic order.

#### Count
Gives the number of elements or subexpressions that match a pattern.
- `Count[list, pattern]`
- `Count[expr, pattern, levelspec]`
- `Count[pattern]`

**Features**:
- Uses standard level specifications, defaulting to level `{1}`.
- Option `Heads -> True` evaluates heads of expressions.
- Effectively returns the length of the list generated by `Cases` for the same arguments.
- `p:def`: `Optional[p, def]`, matches `p`, but if omitted defaults to `def`.
- `x_.`: `Optional[Pattern[x, _]]`, matches `x_`, but if omitted defaults to globally specified `Default` values.
- `p..`: `Repeated[p]`, matches a sequence of one or more expressions, each matching `p`.
- `p...`: `RepeatedNull[p]`, matches a sequence of zero or more expressions, each matching `p`.
- `Repeated[p, max]`, `Repeated[p, {min, max}]`, `Repeated[p, {n}]`: match sequences of length bounded by `max` or `{min, max}`. Equivalent properties exist for `RepeatedNull`.

#### Default Values
- `Default[f] = val`: Sets the default value for arguments of `f`.
- Built-in default values include `0` for `Plus` and `1` for `Times` and `Power`.
- Used automatically by the pattern matcher when `_.` is present in the pattern.

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
| `<>`     | `StringJoin` | 600 | Left |
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

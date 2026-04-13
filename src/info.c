#include "info.h"
#include "symtab.h"

void info_init(void) {
    // Arithmetic
    symtab_set_docstring("Plus", "x + y + ... or Plus[x, y, ...] represents a sum of terms.");
    symtab_set_docstring("Times", "x * y * ... or Times[x, y, ...] represents a product of terms.");
    symtab_set_docstring("Power", "x ^ y or Power[x, y] represents x to the power y.");
    symtab_set_docstring("Subtract", "x - y or Subtract[x, y] represents x - y.");
    symtab_set_docstring("Divide", "x / y or Divide[x, y] represents x / y.");
    symtab_set_docstring("Sqrt", "Sqrt[z] represents the square root of z.");
    symtab_set_docstring("Complex", "Complex[re, im] represents a complex number with real part re and imaginary part im.");
    symtab_set_docstring("Rational", "Rational[n, d] represents a rational number with numerator n and denominator d. Automatically simplifies if arguments are integers.");
    symtab_set_docstring("GCD", "GCD[n1, n2, ...] gives the greatest common divisor of the integers ni.");
    symtab_set_docstring("LCM", "LCM[n1, n2, ...] gives the least common multiple of the integers ni.");
    symtab_set_docstring("PowerMod", "PowerMod[a, b, m] gives a^b mod m.\nPowerMod[a, -1, m] finds the modular inverse of a modulo m.\nPowerMod[a, 1/r, m] finds a modular r-th root of a.");
    symtab_set_docstring("Factorial", "n! gives the factorial of n.\nFor integers and half integers, Factorial automatically evaluates to exact values.");
    symtab_set_docstring("Binomial", "Binomial[n, m] gives the binomial coefficient.");

    // Structural Manipulation
    symtab_set_docstring("Part", "expr[[i]] or Part[expr, i] gives the i-th part of expr.");
    symtab_set_docstring("Extract", "Extract[expr, pos] extracts the part of expr at the position specified by pos.\nExtract[expr, {pos1, pos2, ...}] extracts a list of parts of expr.\nExtract[expr, pos, h] extracts parts of expr, wrapping each of them with head h before evaluation.\nExtract[pos] represents an operator form of Extract that can be applied to an expression.");
    symtab_set_docstring("Span", "i;;j represents a span of elements i through j. i;;j;;k represents a span in steps of k.");
    
    // Linear Algebra
    symtab_set_docstring("Dot", "a.b.c or Dot[a, b, c] gives products of vectors, matrices, and tensors.");
    symtab_set_docstring("Det", "Det[m] gives the determinant of the square matrix m.");
    symtab_set_docstring("Cross", "Cross[a, b, ...] gives the vector cross product of the arguments.");
    symtab_set_docstring("Norm", "Norm[expr] gives the norm of a number, vector, or matrix.\nNorm[expr, p] gives the p-norm.");
    symtab_set_docstring("Tr", "Tr[list] finds the trace of the matrix or tensor list.\nTr[list, f] finds a generalized trace, combining terms with f instead of Plus.\nTr[list, f, n] goes down to level n in list.");
    symtab_set_docstring("RowReduce", "RowReduce[m] gives the row-reduced form of the matrix m.");
    symtab_set_docstring("IdentityMatrix", "IdentityMatrix[n] gives the n x n identity matrix.\nIdentityMatrix[{m, n}] gives the m x n identity matrix.");
    symtab_set_docstring("DiagonalMatrix", "DiagonalMatrix[list] gives a matrix with the elements of list on the leading diagonal, and zero elsewhere.\nDiagonalMatrix[list, k] gives a matrix with the elements of list on the k-th diagonal.\nDiagonalMatrix[list, k, n] pads with zeros to create an n x n matrix.");
    symtab_set_docstring("FullForm", "FullForm[expr] prints as the full internal structure of expr, without any special formatting.");
    symtab_set_docstring("Head", "Head[expr] gives the head of expr.");
    symtab_set_docstring("Length", "Length[expr] gives the number of elements in expr.");
    symtab_set_docstring("Dimensions", "Dimensions[expr] gives a list of the dimensions of expr.");
    symtab_set_docstring("First", "First[expr] gives the first element of expr.");
    symtab_set_docstring("Last", "Last[expr] gives the last element of expr.");
    symtab_set_docstring("Most", "Most[expr] gives all but the last element of expr.");
    symtab_set_docstring("Rest", "Rest[expr] gives all but the first element of expr.");
    symtab_set_docstring("Append", "Append[expr, elem] adds elem to the end of expr.");
    symtab_set_docstring("Prepend", "Prepend[expr, elem] adds elem to the beginning of expr.");
    symtab_set_docstring("Insert", "Insert[expr, elem, n] inserts elem at position n in expr.");
    symtab_set_docstring("Delete", "Delete[expr, n] deletes the element at position n in expr.");
    symtab_set_docstring("Reverse", "Reverse[expr] reverses the order of elements in expr.");
    symtab_set_docstring("RotateLeft", "RotateLeft[expr, n] rotates the elements of expr n positions to the left.");
    symtab_set_docstring("RotateRight", "RotateRight[expr, n] rotates the elements of expr n positions to the right.");
    symtab_set_docstring("Transpose", "Transpose[list] transposes the first two levels of list.");
    symtab_set_docstring("Flatten", "Flatten[list] flattens all levels of list.");
    symtab_set_docstring("Partition", "Partition[list, n] partitions list into sublists of length n.");

    // List Operations
    symtab_set_docstring("Table", "Table[expr, n]\n\tgenerates a list of n copies of expr.\nTable[expr, {i, imax}]\n\tgenerates a list of the values of expr with i running from 1 to imax.");
    symtab_set_docstring("Range", "Range[n]\n\tgenerates the list {1, 2, 3, ..., n}.\nRange[n, m]\n\tgenerates the list {n, n + 1, ..., m - 1, m}.\nRange[n, m, d]\n\tuses step d.");
    symtab_set_docstring("Array", "Array[f, n] generates a list of length n with elements f[1], f[2], ..., f[n].");
    symtab_set_docstring("Union", "Union[list] gives a sorted list of all distinct elements in list.");
    symtab_set_docstring("Tally", "Tally[list] counts the number of occurrences of each distinct element in list.");
    symtab_set_docstring("Commonest", "Commonest[list] gives a list of the elements that are the most common in list.\nCommonest[list, n] gives a list of the n most common elements in list.");
    symtab_set_docstring("DeleteDuplicates", "DeleteDuplicates[list] removes duplicate elements from list.");
    symtab_set_docstring("Split", "Split[list] splits list into sublists of identical adjacent elements.");

    // Statistics
    symtab_set_docstring("Mean", "Mean[data] gives the mean estimate of the elements in data.");
    symtab_set_docstring("RootMeanSquare", "RootMeanSquare[list] gives the root mean square of values in list.");
    symtab_set_docstring("Variance", "Variance[data] gives the unbiased variance estimate of the elements in data.");
    symtab_set_docstring("StandardDeviation", "StandardDeviation[data] gives the standard deviation estimate of the elements in data.");

    // Functional Programming
    symtab_set_docstring("Map", "f /@ expr or Map[f, expr] applies f to each element of expr.");
    symtab_set_docstring("Apply", "f @@ expr or Apply[f, expr] replaces the head of expr with f.");
    symtab_set_docstring("MapAll", "f //@ expr or MapAll[f, expr] applies f to every subexpression in expr.");
    symtab_set_docstring("Through", "Through[p[f, g][x]] gives p[f[x], g[x]].");
    symtab_set_docstring("Select", "Select[list, crit] selects elements of list that satisfy crit.");
    symtab_set_docstring("FreeQ", "FreeQ[expr, form] yields True if no subexpression in expr matches form, and yields False otherwise.");
    symtab_set_docstring("Function", "body & or Function[body] represents a pure function.");
    symtab_set_docstring("Slot", "# or Slot[n] represents the n-th argument of a pure function.");
    symtab_set_docstring("SlotSequence", "## or SlotSequence[n] represents arguments from the n-th onward.");

    // Predicates
    symtab_set_docstring("AtomQ", "AtomQ[expr] gives True if expr is an atomic object.");
    symtab_set_docstring("NumberQ", "NumberQ[expr] gives True if expr is a number.");
    symtab_set_docstring("NumericQ", "NumericQ[expr] gives True if expr is a numeric quantity, and False otherwise.\nAn expression is considered a numeric quantity if it is either an explicit number or a mathematical constant such as Pi, or is a function that has attribute NumericFunction and all of whose arguments are numeric quantities.");
    symtab_set_docstring("IntegerQ", "IntegerQ[expr] gives True if expr is an integer.");
    symtab_set_docstring("EvenQ", "EvenQ[n] gives True if n is an even integer.");
    symtab_set_docstring("OddQ", "OddQ[n] gives True if n is an odd integer.");
    symtab_set_docstring("PrimeQ", "PrimeQ[n] gives True if n is a prime number.");
    symtab_set_docstring("PolynomialQ", "PolynomialQ[expr, var] gives True if expr is a polynomial in var.");
    symtab_set_docstring("ListQ", "ListQ[expr] gives True if expr is a list.");
    symtab_set_docstring("VectorQ", "VectorQ[expr] gives True if expr is a vector.");
    symtab_set_docstring("MatrixQ", "MatrixQ[expr] gives True if expr is a matrix.");
    symtab_set_docstring("MatchQ", "MatchQ[expr, form] gives True if expr matches form.");

    // Trigonometric
    symtab_set_docstring("Sin", "Sin[z] gives the sine of z.");
    symtab_set_docstring("Cos", "Cos[z] gives the cosine of z.");
    symtab_set_docstring("Tan", "Tan[z] gives the tangent of z.");
    symtab_set_docstring("Cot", "Cot[z] gives the cotangent of z.");
    symtab_set_docstring("Sec", "Sec[z] gives the secant of z.");
    symtab_set_docstring("Csc", "Csc[z] gives the cosecant of z.");
    symtab_set_docstring("ArcSin", "ArcSin[z] gives the inverse sine of z.");
    symtab_set_docstring("ArcCos", "ArcCos[z] gives the inverse cosine of z.");
    symtab_set_docstring("ArcTan", "ArcTan[z] gives the inverse tangent of z.");

    // Hyperbolic
    symtab_set_docstring("Sinh", "Sinh[z] gives the hyperbolic sine of z.");
    symtab_set_docstring("Cosh", "Cosh[z] gives the hyperbolic cosine of z.");
    symtab_set_docstring("Tanh", "Tanh[z] gives the hyperbolic tangent of z.");

    // Log/Exp
    symtab_set_docstring("Log", "Log[z] gives the natural logarithm of z. Log[b, z] gives the logarithm to base b.");
    symtab_set_docstring("Exp", "Exp[z] gives the exponential of z.");

    // Piecewise / Rounding
    symtab_set_docstring("Floor", "Floor[x] gives the greatest integer less than or equal to x.");
    symtab_set_docstring("Ceiling", "Ceiling[x] gives the smallest integer greater than or equal to x.");
    symtab_set_docstring("Round", "Round[x] rounds x to the nearest integer.");

    // Calculus
    symtab_set_docstring("D", "D[f, x] gives the partial derivative of f with respect to x.\nD[f, {x, n}] gives the nth partial derivative.\nD[f, x, y, ...] gives the mixed derivative.");
    symtab_set_docstring("Dt", "Dt[f] gives the total derivative of f.\nDt[f, x] gives the total derivative of f with respect to x.\nDt[f, {x, n}] gives the nth total derivative.");
    symtab_set_docstring("Derivative", "Derivative[n][f][x] represents the nth derivative of a function f evaluated at x.");

    // Control Flow
    symtab_set_docstring("Do", "Do[expr, n] evaluates expr n times.\nDo[expr, {i, imax}] evaluates expr with i successively taking on values 1 through imax.\nDo[expr, {i, imin, imax, di}] uses steps di.");
    symtab_set_docstring("For", "For[start, test, incr, body] executes start, then repeatedly evaluates body and incr until test fails to give True.");
    symtab_set_docstring("If", "If[condition, t, f] gives t if condition evaluates to True, and f if it evaluates to False.\nIf[condition, t, f, u] gives u if condition evaluates to neither True nor False.");
    symtab_set_docstring("TrueQ", "TrueQ[expr] yields True if expr is True, and False otherwise.");
    symtab_set_docstring("Set", "lhs = rhs assigns rhs to lhs.");
    symtab_set_docstring("SetDelayed", "lhs := rhs assigns rhs to lhs, evaluating it only when needed.");
    symtab_set_docstring("Default", "Default[f] gives the default value for arguments of the function f obtained with a _. pattern object.");
    symtab_set_docstring("Optional", "patt:def or Optional[patt,def] is a pattern object that represents an expression of the form patt, which, if omitted, should be replaced by the default value def.");
    symtab_set_docstring("Longest", "Longest[p] is a pattern object that matches the longest sequence consistent with the pattern p.");
    symtab_set_docstring("Shortest", "Shortest[p] is a pattern object that matches the shortest sequence consistent with the pattern p.");
    symtab_set_docstring("Repeated", "p.. or Repeated[p] is a pattern object that represents a sequence of one or more expressions, each matching p.\nRepeated[p, max] represents from 1 to max expressions matching p.\nRepeated[p, {min, max}] represents between min and max expressions matching p.\nRepeated[p, {n}] represents exactly n expressions matching p.");
    symtab_set_docstring("RepeatedNull", "p... or RepeatedNull[p] is a pattern object that represents a sequence of zero or more expressions, each matching p.\nRepeatedNull[p, max] represents from 0 to max expressions matching p.\nRepeatedNull[p, {min, max}] represents between min and max expressions matching p.\nRepeatedNull[p, {n}] represents exactly n expressions matching p.");
    symtab_set_docstring("Blank", "_ or Blank[] represents any single expression.\n_h or Blank[h] represents any single expression with head h.");
    symtab_set_docstring("BlankSequence", "__ or BlankSequence[] represents a sequence of one or more expressions.");
    symtab_set_docstring("BlankNullSequence", "___ or BlankNullSequence[] represents a sequence of zero or more expressions.");
    symtab_set_docstring("Clear", "Clear[x, y, ...] clears the values of symbols.");
    symtab_set_docstring("Flat", "Flat is an attribute that can be assigned to a symbol f to indicate that all expressions involving nested functions f should be flattened out. This property is accounted for in pattern matching.");
    symtab_set_docstring("Orderless", "Orderless is an attribute that can be assigned to a symbol f to indicate that the elements e_i in expressions of the form f[e_1, e_2, ...] should automatically be sorted into canonical order. This property is accounted for in pattern matching.");
    symtab_set_docstring("OneIdentity", "OneIdentity is an attribute that can be assigned to a symbol f to indicate that f[x], f[f[x]], etc. are all equivalent to x for the purpose of pattern matching.");
    symtab_set_docstring("Information", "Information[symbol] or ?symbol returns information on symbol.");
    symtab_set_docstring("OwnValues", "OwnValues[s] gives a list of own-value rules for s.");
    symtab_set_docstring("DownValues", "DownValues[s] gives a list of down-value rules for s.");
    symtab_set_docstring("Attributes", "Attributes[s] gives the list of attributes for s.");
    symtab_set_docstring("SetAttributes", "SetAttributes[s, attr] sets the attributes for s.");
    symtab_set_docstring("CompoundExpression", "expr1; expr2; ... evaluates its arguments in sequence, returning the last result.");
    symtab_set_docstring("Module", "Module[{x, y, ...}, expr] specifies that x, y, ... are local variables.");
    symtab_set_docstring("Block", "Block[{x, y, ...}, expr] evaluates expr with local values for x, y, ....");
    symtab_set_docstring("With", "With[{x = x0, ...}, expr] specifies that x should be replaced by x0 throughout expr.");

    // Rules and Replacements
    symtab_set_docstring("Rule", "lhs -> rhs represents a rule that transforms lhs to rhs.");
    symtab_set_docstring("RuleDelayed", "lhs :> rhs represents a rule that transforms lhs to rhs, evaluating rhs only when the rule is used.");
    symtab_set_docstring("Replace", "Replace[expr, rules] applies rules to the entire expr. Replace[expr, rules, levelspec] applies rules to parts of expr specified by levelspec.");
    symtab_set_docstring("ReplaceAll", "expr /. rules or ReplaceAll[expr, rules] applies rules to transform each subpart of expr.");
    symtab_set_docstring("ReplaceRepeated", "expr //. rules or ReplaceRepeated[expr, rules] repeatedly applies rules until the expression no longer changes.");
    symtab_set_docstring("Print", "Print[expr1, expr2, ...] prints the expressions to stdout and returns Null.");
    symtab_set_docstring("FullForm", "FullForm[expr] is a wrapper that causes expr to be printed in full form.");
    symtab_set_docstring("InputForm", "InputForm[expr] is a wrapper that causes expr to be printed in input form.");
    symtab_set_docstring("HoldForm", "HoldForm[expr] prints as the expression expr, with expr maintained in an unevaluated form.");
    symtab_set_docstring("ReplaceList", "ReplaceList[expr, rules] attempts to transform the entire expression expr by applying a rule or list of rules in all possible ways, and returns a list of the results obtained.\nReplaceList[expr, rules, n] gives a list of at most n results.");
    symtab_set_docstring("Cases", "Cases[{e1, e2, ...}, pattern] gives a list of the ei that match the pattern.\nCases[{e1, ...}, pattern -> rhs] gives a list of the values of rhs corresponding to the ei that match the pattern.\nCases[expr, pattern, levelspec] gives a list of all parts of expr on levels specified by levelspec that match the pattern.\nCases[expr, pattern -> rhs, levelspec] gives the values of rhs that match the pattern.\nCases[expr, pattern, levelspec, n] gives the first n parts in expr that match the pattern.\nCases[pattern] represents an operator form of Cases that can be applied to an expression.");
    symtab_set_docstring("Position", "Position[expr, pattern] gives a list of the positions at which objects matching pattern appear in expr.\nPosition[expr, pattern, levelspec] finds only objects that appear on levels specified by levelspec.\nPosition[expr, pattern, levelspec, n] gives the positions of the first n objects found.\nPosition[pattern] represents an operator form of Position that can be applied to an expression.");
    symtab_set_docstring("OrderedQ", "OrderedQ[h[e1, e2, ...]] gives True if the elements are in canonical order, and False otherwise.\nOrderedQ[expr, p] uses the ordering function p to determine whether each pair of elements is in order.");
    symtab_set_docstring("Sort", "Sort[list] sorts the elements of list into canonical order.\nSort[list, p] sorts using the ordering function p.");
    symtab_set_docstring("MemberQ", "MemberQ[list, form] returns True if an element of list matches form, and False otherwise.\nMemberQ[list, form, levelspec] tests all parts of list specified by levelspec.\nMemberQ[form] represents an operator form of MemberQ that can be applied to an expression.");
    symtab_set_docstring("Count", "Count[list, pattern] gives the number of elements in list that match pattern.\nCount[expr, pattern, levelspec] gives the total number of subexpressions matching pattern that appear at the levels in expr specified by levelspec.\nCount[pattern] represents an operator form of Count that can be applied to an expression.");

    // Polynomials
    symtab_set_docstring("Expand", "Expand[expr] expands out products and powers in expr.\nExpand[expr, patt] leaves unexpanded any parts of expr that are free of the pattern patt.");
    symtab_set_docstring("Coefficient", "Coefficient[expr, form] gives the coefficient of form in expr.\nCoefficient[expr, form, n] gives the coefficient of form^n in expr.");
    symtab_set_docstring("CoefficientList", "CoefficientList[poly, var] gives a list of coefficients of powers of var in poly, starting with power 0.\nCoefficientList[poly, {var1, var2, ...}] gives an array of coefficients of the variables.");
    symtab_set_docstring("PolynomialGCD", "PolynomialGCD[poly1, poly2, ...] gives the greatest common divisor of the polynomials.");
    symtab_set_docstring("PolynomialExtendedGCD", "PolynomialExtendedGCD[poly1, poly2, x] gives the extended GCD of poly1 and poly2 treated as univariate polynomials in x.");
    symtab_set_docstring("PolynomialQuotient", "PolynomialQuotient[p, q, x] gives the quotient of p and q, treated as polynomials in x, with any remainder dropped.");
    symtab_set_docstring("PolynomialRemainder", "PolynomialRemainder[p, q, x] gives the remainder from dividing p by q, treated as polynomials in x.");
    symtab_set_docstring("Collect", "Collect[expr, x] collects together terms involving the same powers of objects matching x.");
    symtab_set_docstring("PolynomialMod", "PolynomialMod[poly,m] gives the polynomial poly reduced modulo m.\nPolynomialMod[poly,{Subscript[m, 1],Subscript[m, 2],...}] reduces modulo all of the Subscript[m, i].");
    symtab_set_docstring("FactorSquareFree", "FactorSquareFree[poly] pulls out any multiple factors in a polynomial.");
    symtab_set_docstring("Factor", "Factor[poly] factors a polynomial over the integers.");
    symtab_set_docstring("Variables", "Variables[poly] gives a list of all independent variables in the polynomial poly.");
    symtab_set_docstring("PolynomialQ", "PolynomialQ[expr, var] yields True if expr is a polynomial in var.");
    symtab_set_docstring("Decompose", "Decompose[poly, x] decomposes a polynomial, if possible, into a composition of simpler polynomials.");
    symtab_set_docstring("HornerForm", "HornerForm[poly] puts the polynomial poly in Horner form.\nHornerForm[poly, vars] puts poly in Horner form with respect to vars.\nHornerForm[poly1/poly2, vars1, vars2] puts the rational function in Horner form.");
    symtab_set_docstring("Resultant", "Resultant[poly1, poly2, var] computes the resultant of the polynomials poly1 and poly2 with respect to the variable var.");
    symtab_set_docstring("Discriminant", "Discriminant[poly, var] computes the discriminant of the polynomial poly with respect to the variable var.");
    symtab_set_docstring("Numerator", "Numerator[expr] gives the numerator of expr.\nNumerator picks out terms which do not have superficially negative exponents.");
    symtab_set_docstring("Denominator", "Denominator[expr] gives the denominator of expr.\nDenominator picks out terms which have superficially negative exponents.");
    symtab_set_docstring("Cancel", "Cancel[expr] cancels out common factors in the numerator and denominator of expr.");
    symtab_set_docstring("Apart", "Apart[expr] rewrites a rational expression as a sum of terms with minimal denominators.\nApart[expr,var] treats all variables other than var as constants.");
    symtab_set_docstring("LeafCount", "LeafCount[expr] gives the total number of indivisible subexpressions in expr.");
    symtab_set_docstring("ByteCount", "ByteCount[expr] gives the number of bytes used internally by PicoCAS to store expr.");

    // Time and Date
    symtab_set_docstring("Timing", "Timing[expr] evaluates expr, and returns a list of the time in seconds used, together with the result obtained.");
    symtab_set_docstring("RepeatedTiming", "RepeatedTiming[expr] evaluates expr repeatedly and returns a list of the average time in seconds used, together with the result obtained.\nRepeatedTiming[expr, t] does repeated evaluation for at least t seconds.");

    // Comparisons
    symtab_set_docstring("SameQ", "lhs === rhs yields True if the expression lhs is identical to rhs, and yields False otherwise.");
    symtab_set_docstring("UnsameQ", "lhs =!= rhs yields True if the expression lhs is not identical to rhs, and yields False otherwise.");
    symtab_set_docstring("Equal", "lhs == rhs yields True if lhs and rhs are equal.");
    symtab_set_docstring("Unequal", "lhs != rhs yields True if lhs and rhs are not equal.");
    symtab_set_docstring("Less", "x < y yields True if x is strictly less than y.");
    symtab_set_docstring("Greater", "x > y yields True if x is strictly greater than y.");
    symtab_set_docstring("LessEqual", "x <= y yields True if x is less than or equal to y.");
    symtab_set_docstring("GreaterEqual", "x >= y yields True if x is greater than or equal to y.");

    // Primes
    symtab_set_docstring("FactorInteger", "FactorInteger[n] gives a list of the prime factors of the integer n, together with their exponents.");
    symtab_set_docstring("EulerPhi", "EulerPhi[n] gives the Euler totient function phi(n).");
    symtab_set_docstring("PrimePi", "PrimePi[x] gives the number of primes less than or equal to x.");
    symtab_set_docstring("NextPrime", "NextPrime[x] gives the next prime after x.");
    symtab_set_docstring("Distribute", "Distribute[f[x1, x2, ...]]\n\tdistributes f over Plus appearing in any of the xi.\nDistribute[expr, g]\n\tdistributes over g.\nDistribute[expr, g, f]\n\tperforms the distribution only if the head of expr is f.\nDistribute[expr, g, f, gp, fp]\n\tgives gp and fp in place of g and f respectively in the result of the distribution.");
    symtab_set_docstring("Distribute", "Distribute[f[x1, x2, ...]]\n\tdistributes f over Plus appearing in any of the xi.\nDistribute[expr, g]\n\tdistributes over g.\nDistribute[expr, g, f]\n\tperforms the distribution only if the head of expr is f.\nDistribute[expr, g, f, gp, fp]\n\tgives gp and fp in place of g and f respectively in the result of the distribution.");
    symtab_set_docstring("Inner", "Inner[f,list1,list2,g]\n\tis a generalization of Dot in which f plays the role of multiplication and g of addition.\nInner[f,list1,list2]\n\tuses Plus for g.\nInner[f,list1,list2,g,n]\n\tcontracts index n of the first tensor with the first index of the second tensor.");
    symtab_set_docstring("Outer", "Outer[f,list1,list2,...]\n\tgives the generalized outer product of the listi, forming all possible combinations of the lowest-level elements in each of them, and feeding them as arguments to f.\nOuter[f,list1,list2,...,n]\n\ttreats as separate elements only sublists at level n in the listi.\nOuter[f,list1,list2,...,n1,n2,...]\n\ttreats as separate elements only sublists at level ni in the corresponding listi.");
    symtab_set_docstring("Tuples", "Tuples[list,n]\n\tgenerates a list of all possible n-tuples of elements from list.\nTuples[{list1,list2,...}]\n\tgenerates a list of all possible tuples whose ith element is from listi.\nTuples[list,{n1,n2,...}]\n\tgenerates a list of all possible n1 x n2 x ... arrays of elements in list.");
    symtab_set_docstring("Permutations", "Permutations[list]\n\tgenerates a list of all possible permutations of the elements in list.\nPermutations[list,n]\n\tgives all permutations containing at most n elements.\nPermutations[list,{n}]\n\tgives all permutations containing exactly n elements.");
    symtab_set_docstring("Min", "Min[x1, x2, ...]\n\tyields the numerically smallest of the xi.\nMin[{x1, x2, ...}, {y1, ...}, ...]\n\tyields the smallest element of any of the lists.");
    symtab_set_docstring("Max", "Max[x1, x2, ...]\n\tyields the numerically largest of the xi.\nMax[{x1, x2, ...}, {y1, ...}, ...]\n\tyields the largest element of any of the lists.");
    symtab_set_docstring("Median", "Median[data]\n\tgives the median estimate of the elements in data.\nMedian[dist]\n\tgives the median of the distribution dist.");

    symtab_set_docstring("Quartiles", "Quartiles[data]\n\tgives the {q_1/4, q_2/4, q_3/4} quantile estimates of the elements in data.\nQuartiles[data,{{a,b},{c,d}}]\n\tuses the quantile definition specified by parameters a, b, c, d.\nQuartiles[dist]\n\tgives the {q_1/4, q_2/4, q_3/4} quantiles of the distribution dist.");
}

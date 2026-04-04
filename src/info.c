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
    symtab_set_docstring("Rational", "Rational[n, d] represents a rational number with numerator n and denominator d.");
    symtab_set_docstring("GCD", "GCD[n1, n2, ...] gives the greatest common divisor of the integers ni.");
    symtab_set_docstring("LCM", "LCM[n1, n2, ...] gives the least common multiple of the integers ni.");

    // Structural Manipulation
    symtab_set_docstring("Part", "expr[[i]] or Part[expr, i] gives the i-th part of expr.");
    symtab_set_docstring("Span", "i;;j represents a span of elements i through j. i;;j;;k represents a span in steps of k.");
    
    // Linear Algebra
    symtab_set_docstring("Dot", "a.b.c or Dot[a, b, c] gives products of vectors, matrices, and tensors.");
    symtab_set_docstring("Det", "Det[m] gives the determinant of the square matrix m.");
    symtab_set_docstring("Cross", "Cross[a, b, ...] gives the vector cross product of the arguments.");
    symtab_set_docstring("Norm", "Norm[expr] gives the norm of a number, vector, or matrix.\nNorm[expr, p] gives the p-norm.");
    symtab_set_docstring("Tr", "Tr[list] finds the trace of the matrix or tensor list.\nTr[list, f] finds a generalized trace, combining terms with f instead of Plus.\nTr[list, f, n] goes down to level n in list.");
    symtab_set_docstring("RowReduce", "RowReduce[m] gives the row-reduced form of the matrix m.");
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
    symtab_set_docstring("DeleteDuplicates", "DeleteDuplicates[list] removes duplicate elements from list.");
    symtab_set_docstring("Split", "Split[list] splits list into sublists of identical adjacent elements.");

    // Statistics
    symtab_set_docstring("Mean", "Mean[data] gives the mean estimate of the elements in data.");
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

    // System / Assignments
    symtab_set_docstring("Set", "lhs = rhs assigns rhs to lhs.");
    symtab_set_docstring("SetDelayed", "lhs := rhs assigns rhs to lhs, evaluating it only when needed.");
    symtab_set_docstring("Clear", "Clear[x, y, ...] clears the values of symbols.");
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

    // Polynomials
    symtab_set_docstring("Expand", "Expand[expr] expands out products and powers in expr.\nExpand[expr, patt] leaves unexpanded any parts of expr that are free of the pattern patt.");
    symtab_set_docstring("Coefficient", "Coefficient[expr, form] gives the coefficient of form in expr.\nCoefficient[expr, form, n] gives the coefficient of form^n in expr.");
    symtab_set_docstring("CoefficientList", "CoefficientList[poly, var] gives a list of coefficients of powers of var in poly, starting with power 0.\nCoefficientList[poly, {var1, var2, ...}] gives an array of coefficients of the variables.");
    symtab_set_docstring("PolynomialGCD", "PolynomialGCD[poly1, poly2, ...] gives the greatest common divisor of the polynomials.");
    symtab_set_docstring("PolynomialLCM", "PolynomialLCM[poly1, poly2, ...] gives the least common multiple of the polynomials.");
    symtab_set_docstring("PolynomialQuotient", "PolynomialQuotient[p, q, x] gives the quotient of p and q, treated as polynomials in x, with any remainder dropped.");
    symtab_set_docstring("PolynomialRemainder", "PolynomialRemainder[p, q, x] gives the remainder from dividing p by q, treated as polynomials in x.");
    symtab_set_docstring("Collect", "Collect[expr, x] collects together terms involving the same powers of objects matching x.");
    symtab_set_docstring("FactorSquareFree", "FactorSquareFree[poly] pulls out any multiple factors in a polynomial.");
    symtab_set_docstring("Variables", "Variables[poly] gives a list of all independent variables in the polynomial poly.");
    symtab_set_docstring("PolynomialQ", "PolynomialQ[expr, var] yields True if expr is a polynomial in var.");
    symtab_set_docstring("Decompose", "Decompose[poly, x] decomposes a polynomial, if possible, into a composition of simpler polynomials.");
    symtab_set_docstring("HornerForm", "HornerForm[poly] puts the polynomial poly in Horner form.\nHornerForm[poly, vars] puts poly in Horner form with respect to vars.\nHornerForm[poly1/poly2, vars1, vars2] puts the rational function in Horner form.");
    symtab_set_docstring("Resultant", "Resultant[poly1, poly2, var] computes the resultant of the polynomials poly1 and poly2 with respect to the variable var.");
    symtab_set_docstring("Discriminant", "Discriminant[poly, var] computes the discriminant of the polynomial poly with respect to the variable var.");
    symtab_set_docstring("Numerator", "Numerator[expr] gives the numerator of expr.\nNumerator picks out terms which do not have superficially negative exponents.");
    symtab_set_docstring("Denominator", "Denominator[expr] gives the denominator of expr.\nDenominator picks out terms which have superficially negative exponents.");

    // Time and Date
    symtab_set_docstring("Timing", "Timing[expr] evaluates expr, and returns a list of the time in seconds used, together with the result obtained.");
    symtab_set_docstring("RepeatedTiming", "RepeatedTiming[expr] evaluates expr repeatedly and returns a list of the average time in seconds used, together with the result obtained.\nRepeatedTiming[expr, t] does repeated evaluation for at least t seconds.");

    // Primes
    symtab_set_docstring("FactorInteger", "FactorInteger[n] gives a list of the prime factors of the integer n, together with their exponents.");
    symtab_set_docstring("PrimePi", "PrimePi[x] gives the number of primes less than or equal to x.");
    symtab_set_docstring("NextPrime", "NextPrime[x] gives the next prime after x.");
}

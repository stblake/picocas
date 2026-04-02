#ifndef PARSE_H
#define PARSE_H

#include "expr.h"

/*
 * Parses Mathematica FullForm expressions into Expr trees
 * Supported syntax:
 * - Symbols:        x, `name`, $var
 * - Numbers:        123, -45, 3.14, 1e5
 * - Strings:        "text"
 * - Functions:      f[x,y], Derivative[1][f][x]
 * - Lists:          {1,2,3}
 * 
 * Returns: New Expr tree on success, NULL on failure
 * Memory: Caller must free result with expr_free()
 */
Expr* parse_expression(const char* input);

#endif // PARSE_H

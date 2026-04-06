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

/*
 * Parses the next expression from the input string pointer,
 * advancing the pointer to the character after the parsed expression.
 * Returns NULL if no more expressions can be parsed.
 */
Expr* parse_next_expression(const char** input_ptr);

#endif // PARSE_H

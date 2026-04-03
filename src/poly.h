#ifndef POLY_H
#define POLY_H

#include "expr.h"

Expr* builtin_polynomialq(Expr* res);
Expr* builtin_variables(Expr* res);
Expr* builtin_coefficient(Expr* res);
Expr* builtin_coefficientlist(Expr* res);
Expr* builtin_polynomialgcd(Expr* res);
Expr* builtin_polynomiallcm(Expr* res);
Expr* builtin_polynomialquotient(Expr* res);
Expr* builtin_polynomialremainder(Expr* res);
Expr* builtin_collect(Expr* res);
Expr* builtin_decompose(Expr* res);
Expr* builtin_hornerform(Expr* res);
Expr* builtin_resultant(Expr* res);
Expr* builtin_discriminant(Expr* res);

// Internal polynomial APIs needed for factorization
bool is_polynomial(Expr* e, Expr** vars, size_t var_count);
bool is_zero_poly(Expr* e);
int get_degree_poly(Expr* e, Expr* var);
Expr* exact_poly_div(Expr* A, Expr* B, Expr** vars, size_t var_count);
Expr* poly_gcd_internal(Expr* A, Expr* B, Expr** vars, size_t var_count);
Expr* poly_content(Expr* A, Expr** vars, size_t var_count);
void collect_variables(Expr* e, Expr*** vars_ptr, size_t* count, size_t* capacity);
int compare_expr_ptrs(const void* a, const void* b);
bool contains_any_symbol_from(Expr* expr, Expr* var);

void poly_init(void);

#endif // POLY_H

#include "poly.h"
#include "expand.h"
#include "eval.h"
#include "parse.h"
#include "print.h"
#include "symtab.h"
#include "core.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declare exact_poly_div
Expr* exact_poly_div(Expr* A, Expr* B, Expr** vars, size_t var_count);
Expr* builtin_variables(Expr* res);

int main() {
    symtab_init(); core_init();
    Expr* A = parse_expression("x^2+2 x y+y^2");
    Expr* B = parse_expression("x+y");
    
    // Get vars from A
    Expr* var_call = evaluate(parse_expression("Variables[x^2+2x y+y^2]"));
    size_t var_count = var_call->data.function.arg_count;
    Expr** vars = malloc(sizeof(Expr*) * var_count);
    for(size_t i=0; i<var_count; i++) vars[i] = expr_copy(var_call->data.function.args[i]);
    
    Expr* q = exact_poly_div(A, B, vars, var_count);
    if (q) {
        char* s = expr_to_string_fullform(q);
        printf("Quotient: %s\n", s);
        free(s); expr_free(q);
    } else {
        printf("Quotient: NULL\n");
    }

    return 0;
}

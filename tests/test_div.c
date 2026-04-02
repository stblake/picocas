#include "poly.h"
#include "expand.h"
#include "eval.h"
#include "parse.h"
#include "print.h"
#include "symtab.h"
#include "core.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    symtab_init(); core_init();
    // Since exact_poly_div is static, we can't call it easily. 
    // We will just test PolynomialLCM with 2 arguments.
    Expr* e = parse_expression("PolynomialLCM[x^2-1, x^3-1]");
    Expr* r = evaluate(e);
    char* s = expr_to_string_fullform(r);
    printf("LCM Result: %s\n", s);
    free(s); expr_free(r); expr_free(e);
    return 0;
}
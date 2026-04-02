#include "parse.h"
#include "print.h"
#include "expr.h"
#include <stdio.h>

int main() {
    const char* tests[] = {"_", "__", "___", "x_", "x__", "x_Integer", "_Integer"};
    for (int i=0; i<7; i++) {
        Expr* e = parse_expression(tests[i]);
        if (e) {
            printf("%s -> ", tests[i]);
            expr_print(e);
            printf("\n");
            expr_free(e);
        }
    }
    return 0;
}

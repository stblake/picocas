#include "eval.h"
#include "parse.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_purefunc_basic() {
    struct {
        const char* input;
        const char* expected;
    } cases[] = {
        {"Function[u, 3+u][x]", "Plus[3, x]"},
        {"Function[3+#][x]", "Plus[3, x]"},
        {"(3+#)&[x]", "Plus[3, x]"},
        {"Function[{u, v}, u^2 + v^4][x, y]", "Plus[Power[x, 2], Power[y, 4]]"},
        {"(#1^2 + #2^4)&[x, y]", "Plus[Power[x, 2], Power[y, 4]]"},
        
        // Unbound slots remain unevaluated
        {"(3+#)&[]", "Plus[3, Slot[1]]"},
        
        // Sequence slots
        {"f[##]&[a, b, c, d]", "f[a, b, c, d]"},
        {"f[X, ##, Y, ##]&[a, b, c, d]", "f[X, a, b, c, d, Y, a, b, c, d]"},
        {"f[##2]&[a, b, c, d]", "f[b, c, d]"}, // SlotSequence[2]
        
        // Mapping a pure function over a list
        {"g[#, #^2]& /@ {x, y, z}", "List[g[x, Power[x, 2]], g[y, Power[y, 2]], g[z, Power[z, 2]]]"},
        
        // Array from a pure function
        {"Array[1+#^2&, 3]", "List[2, 5, 10]"},
        
        // Listable pure functions
        {"Function[{u}, g[u], Listable][{a, b, c}]", "List[g[a], g[b], g[c]]"},
        {"Function[{u}, g[u]][{a, b, c}]", "g[List[a, b, c]]"},
        
        {NULL, NULL}
    };

    for (int i = 0; cases[i].input != NULL; i++) {
        Expr* e = parse_expression(cases[i].input);
        Expr* res = evaluate(e);
        char* s = expr_to_string_fullform(res);
        if (strcmp(s, cases[i].expected) != 0) {
            printf("Forward %s: expected %s, got %s\n", cases[i].input, cases[i].expected, s);
        }
        ASSERT_MSG(strcmp(s, cases[i].expected) == 0, "Forward %s: expected %s, got %s", cases[i].input, cases[i].expected, s);
        free(s);
        expr_free(e);
        expr_free(res);
    }
}

int main() {
    symtab_init();
    core_init();
    
    TEST(test_purefunc_basic);
    
    return 0;
}

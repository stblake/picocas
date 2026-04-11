#include "sort.h"
#include "eval.h"
#include "parse.h"
#include "expr.h"
#include "symtab.h"
#include "core.h"
#include "print.h"
#include "test_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void run_test(const char* input, const char* expected) {
    Expr* e = parse_expression(input);
    Expr* res = evaluate(e);
    char* s = expr_to_string_fullform(res);
    if (strcmp(s, expected) != 0) {
        printf("FAIL: %s\n  expected: %s\n  got:      %s\n", input, expected, s);
    }
    ASSERT_MSG(strcmp(s, expected) == 0, "%s: expected %s, got %s", input, expected, s);
    free(s);
    expr_free(e); expr_free(res);
}

void test_sort_canonical() {
    run_test("Sort[{d, b, c, a}]", "List[a, b, c, d]");
    run_test("Sort[{\"cat\", \"fish\", \"catfish\", \"Cat\"}]", "List[\"cat\", \"Cat\", \"catfish\", \"fish\"]");
    run_test("Sort[{y, x^2, x + y, y^3}]", "List[Power[x, 2], y, Power[y, 3], Plus[x, y]]");
    run_test("Sort[f[b, a, c]]", "f[a, b, c]");
    // Numbers
    run_test("Sort[{3, 1, 2}]", "List[1, 2, 3]");
    run_test("Sort[{1.5, 1, 2}]", "List[1, 1.5, 2]");
}

void test_sort_custom() {
    // Sort by numerical value with Less
    // Note: Pi and E are symbols, Sqrt[2] is Power[2, 1/2]
    // Order: 1, Sqrt[2], 2, E, 3, Pi
    // Numerically: 1, 1.414, 2, 2.718, 3, 3.14159
    // Wait, Less currently works on numbers. If they are symbols it might not evaluate to True/False.
    // I need to make sure Pi and E have numerical values if I want Less to work, 
    // or use N[Pi] etc. But Sort[list, Less] in Mathematica works if they are numeric.
    
    // For now test with explicit numbers
    run_test("Sort[{3, 1, 2, 1.5}, Less]", "List[1, 1.5, 2, 3]");
    run_test("Sort[{3, 1, 2, 1.5}, Greater]", "List[3, 2, 1.5, 1]");
    
    // Sort by comparing second part
    run_test("Sort[{{a, 2}, {c, 1}, {d, 3}}, #1[[2]] < #2[[2]] &]", 
             "List[List[c, 1], List[a, 2], List[d, 3]]");
}


void test_orderedq() {
    run_test("OrderedQ[{e, e}]", "True");
    run_test("OrderedQ[{1, 4, 2}]", "False");
    run_test("OrderedQ[{\"cat\", \"catfish\", \"fish\"}]", "True");
    run_test("OrderedQ[{1, Sqrt[2], 2, E, 3, Pi}]", "False");
    run_test("OrderedQ[{1, Sqrt[2], 2, E, 3, Pi}, Less]", "True");
    run_test("OrderedQ[{{a, 2}, {c, 1}, {d, 3}}, #1[[2]] < #2[[2]] &]", "False");
    run_test("OrderedQ[{x, y, x + y}]", "True");
    run_test("OrderedQ[f[b, a, c]]", "False");
    run_test("OrderedQ[{4, 3, 2, 1}, Greater]", "True");
    run_test("OrderedQ[{4, 3, 3, 1}, Greater]", "False");
    run_test("OrderedQ[{4, 3, 3, 1}, GreaterEqual]", "True");
}

int main() {
    symtab_init();
    core_init();
    
    // Define Pi and E for numerical tests if needed
    // symtab_add_own_value("Pi", expr_new_symbol("Pi"), expr_new_real(3.141592653589793));
    // symtab_get_def("Pi")->attributes |= ATTR_NUMERICFUNCTION;
    
    TEST(test_sort_canonical);
    TEST(test_sort_custom);
    TEST(test_orderedq);
    
    printf("All sort tests passed!\n");
    return 0;
}

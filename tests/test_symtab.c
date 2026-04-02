#include "test_utils.h"
#include "symtab.h"
#include "match.h"
#include "expr.h"
#include "parse.h"

static Expr* parse_or_die(const char* s) {
    Expr* e = parse_expression(s);
    if (!e) {
        fprintf(stderr, "Failed to parse: %s\n", s);
        exit(1);
    }
    return e;
}

void test_own_values() {
    symtab_init();
    
    Expr* px = parse_or_die("x");
    Expr* r42 = parse_or_die("42");
    symtab_add_own_value("x", px, r42);
    
    Expr* test_expr = parse_or_die("x");
    Expr* res = apply_own_values(test_expr);
    
    ASSERT(res != NULL);
    ASSERT(res->type == EXPR_INTEGER && res->data.integer == 42);
    
    expr_free(px); expr_free(r42); expr_free(test_expr); expr_free(res);
    symtab_clear();
}

void test_down_values() {
    symtab_init();
    
    Expr* p_fx = parse_or_die("f[Pattern[x, Blank[]]]");
    Expr* r_xx = parse_or_die("g[x, x]");
    symtab_add_down_value("f", p_fx, r_xx);
    
    Expr* test_expr = parse_or_die("f[42]");
    Expr* res = apply_down_values(test_expr);
    
    ASSERT(res != NULL);
    Expr* expected = parse_or_die("g[42, 42]");
    ASSERT(expr_eq(res, expected));
    
    expr_free(p_fx); expr_free(r_xx); expr_free(test_expr); expr_free(res); expr_free(expected);
    symtab_clear();
}

void test_symtab_stress() {
    symtab_init();
    
    // Create 1000 own values and test retrieval
    for (int i = 0; i < 1000; i++) {
        char sym_name[32];
        snprintf(sym_name, sizeof(sym_name), "sym%d", i);
        
        Expr* px = parse_or_die(sym_name);
        Expr* val = expr_new_integer(i);
        symtab_add_own_value(sym_name, px, val);
        expr_free(px);
        expr_free(val);
    }
    
    // Check some random values
    Expr* t1 = parse_or_die("sym500");
    Expr* r1 = apply_own_values(t1);
    ASSERT(r1 != NULL && r1->type == EXPR_INTEGER && r1->data.integer == 500);
    expr_free(t1); expr_free(r1);

    Expr* t2 = parse_or_die("sym999");
    Expr* r2 = apply_own_values(t2);
    ASSERT(r2 != NULL && r2->type == EXPR_INTEGER && r2->data.integer == 999);
    expr_free(t2); expr_free(r2);
    
    Expr* t3 = parse_or_die("sym1000");
    Expr* r3 = apply_own_values(t3);
    ASSERT(r3 == NULL); // doesn't exist
    expr_free(t3);
    
    // Check down values resolution list stress
    Expr* p_f1 = parse_or_die("f[1]");
    Expr* r_f1 = parse_or_die("100");
    symtab_add_down_value("f", p_f1, r_f1);
    
    Expr* p_f2 = parse_or_die("f[2]");
    Expr* r_f2 = parse_or_die("200");
    symtab_add_down_value("f", p_f2, r_f2);
    
    Expr* p_fx = parse_or_die("f[Pattern[x, Blank[]]]");
    Expr* r_fx = parse_or_die("g[x]");
    symtab_add_down_value("f", p_fx, r_fx);
    
    Expr* tf1 = parse_or_die("f[1]");
    Expr* rf1 = apply_down_values(tf1);
    ASSERT(rf1 != NULL && rf1->type == EXPR_INTEGER && rf1->data.integer == 100);
    expr_free(tf1); expr_free(rf1);
    
    Expr* tf3 = parse_or_die("f[3]");
    Expr* rf3 = apply_down_values(tf3);
    ASSERT(rf3 != NULL);
    Expr* expected_rf3 = parse_or_die("g[3]");
    ASSERT(expr_eq(rf3, expected_rf3));
    expr_free(tf3); expr_free(rf3); expr_free(expected_rf3);

    expr_free(p_f1); expr_free(r_f1);
    expr_free(p_f2); expr_free(r_f2);
    expr_free(p_fx); expr_free(r_fx);
    
    symtab_clear();
}

int main() {
    TEST(test_own_values);
    TEST(test_down_values);
    TEST(test_symtab_stress);
    printf("All symtab tests passed!\n");
    return 0;
}

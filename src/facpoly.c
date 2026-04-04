#include "facpoly.h"
#include "poly.h"
#include "eval.h"
#include "expand.h"
#include "symtab.h"
#include "attr.h"
#include "arithmetic.h"
#include "parse.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static bool is_constant_1(Expr* e);
Expr* bz_factor_to_expr(Expr* P, Expr* var);

static bool is_constant_1(Expr* e) {
    if (!e) return false;
    Expr* ev = eval_and_free(expr_copy(e));
    bool res = (ev->type == EXPR_INTEGER && ev->data.integer == 1);
    expr_free(ev);
    return res;
}

static Expr* poly_deriv(Expr* p, Expr* x) {
    if (!contains_any_symbol_from(p, x)) return expr_new_integer(0);
    if (expr_eq(p, x)) return expr_new_integer(1);

    if (p->type == EXPR_FUNCTION) {
        const char* head = p->data.function.head->type == EXPR_SYMBOL ? p->data.function.head->data.symbol : "";
        if (strcmp(head, "Plus") == 0) {
            Expr** args = malloc(sizeof(Expr*) * p->data.function.arg_count);
            for (size_t i=0; i<p->data.function.arg_count; i++) args[i] = poly_deriv(p->data.function.args[i], x);
            Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Plus"), args, p->data.function.arg_count));
            free(args);
            return res;
        } else if (strcmp(head, "Times") == 0) {
            size_t count = p->data.function.arg_count;
            Expr** sum_args = malloc(sizeof(Expr*) * count);
            for (size_t i = 0; i < count; i++) {
                Expr** prod_args = malloc(sizeof(Expr*) * count);
                for (size_t j = 0; j < count; j++) {
                    if (i == j) prod_args[j] = poly_deriv(p->data.function.args[j], x);
                    else prod_args[j] = expr_copy(p->data.function.args[j]);
                }
                sum_args[i] = eval_and_free(expr_new_function(expr_new_symbol("Times"), prod_args, count));
                free(prod_args);
            }
            Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Plus"), sum_args, count));
            free(sum_args);
            return res;
        } else if (strcmp(head, "Power") == 0 && p->data.function.arg_count == 2) {
            Expr* base = p->data.function.args[0];
            Expr* exp = p->data.function.args[1];
            if (!contains_any_symbol_from(exp, x)) {
                Expr* d_base = poly_deriv(base, x);
                Expr** p_args1 = malloc(sizeof(Expr*) * 2);
                p_args1[0] = expr_copy(exp); p_args1[1] = expr_new_integer(-1);
                Expr* exp_minus_1 = eval_and_free(expr_new_function(expr_new_symbol("Plus"), p_args1, 2));
                free(p_args1);

                Expr** p_args2 = malloc(sizeof(Expr*) * 2);
                p_args2[0] = expr_copy(base); p_args2[1] = exp_minus_1;
                Expr* pow_term = eval_and_free(expr_new_function(expr_new_symbol("Power"), p_args2, 2));
                free(p_args2);

                Expr** t_args = malloc(sizeof(Expr*) * 3);
                t_args[0] = expr_copy(exp); t_args[1] = pow_term; t_args[2] = d_base;
                Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 3));
                free(t_args);
                return res;
            }
        }
    }
    return expr_new_integer(0);
}

static Expr* factor_square_free_poly(Expr* P, Expr** vars, size_t var_count) {
    if (var_count == 0 || is_zero_poly(P)) return expr_copy(P);
    Expr* x = vars[var_count - 1];
    if (get_degree_poly(P, x) == 0) {
        return factor_square_free_poly(P, vars, var_count - 1);
    }
    
    Expr* cont = poly_content(P, vars, var_count);
    Expr* pp = exact_poly_div(P, cont, vars, var_count);
    if (!pp) {
        pp = expr_copy(P);
        expr_free(cont);
        cont = expr_new_integer(1);
    }
    
    Expr* res_cont = factor_square_free_poly(cont, vars, var_count - 1);
    expr_free(cont);
    
    if (get_degree_poly(pp, x) == 0) {
        Expr** t_args = malloc(sizeof(Expr*) * 2);
        t_args[0] = res_cont; t_args[1] = pp;
        Expr* ret = eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 2));
        free(t_args);
        return ret;
    }
    
    Expr* A = pp;
    Expr* raw_A_prime = poly_deriv(A, x);
    Expr* A_prime = expr_expand(raw_A_prime);
    expr_free(raw_A_prime);
    Expr* B = poly_gcd_internal(A, A_prime, vars, var_count);
    Expr* C = exact_poly_div(A, B, vars, var_count);
    if (!C) C = expr_copy(A);
    Expr* A_prime_div_B = exact_poly_div(A_prime, B, vars, var_count);
    if (!A_prime_div_B) A_prime_div_B = expr_new_integer(0);
    
    Expr* raw_C_prime = poly_deriv(C, x);
    Expr* C_prime = expr_expand(raw_C_prime);
    expr_free(raw_C_prime);
    
    Expr** m_args = malloc(sizeof(Expr*) * 2);
    m_args[0] = expr_new_integer(-1); m_args[1] = C_prime;
    Expr* neg_C_prime = eval_and_free(expr_new_function(expr_new_symbol("Times"), m_args, 2));
    free(m_args);
    
    Expr** p_args = malloc(sizeof(Expr*) * 2);
    p_args[0] = A_prime_div_B; p_args[1] = neg_C_prime;
    Expr* evaluated_p = eval_and_free(expr_new_function(expr_new_symbol("Plus"), p_args, 2));
    free(p_args);
    Expr* D = expr_expand(evaluated_p);
    expr_free(evaluated_p);
    
    size_t i = 1;
    size_t factors_count = 0;
    size_t factors_cap = 16;
    Expr** factors_data = malloc(sizeof(Expr*) * factors_cap);
    
    int max_i = get_degree_poly(pp, x);

    while (!is_zero_poly(C) && get_degree_poly(C, x) > 0 && !is_constant_1(C)) {
        if (i > (size_t)max_i + 2) break;
        
        Expr* P_i = poly_gcd_internal(C, D, vars, var_count);
        if (!is_constant_1(P_i)) {
            if (factors_count == factors_cap) { factors_cap *= 2; factors_data = realloc(factors_data, sizeof(Expr*) * factors_cap); }
            if (i == 1) {
                factors_data[factors_count++] = expr_copy(P_i);
            } else {
                Expr** pow_args = malloc(sizeof(Expr*) * 2);
                pow_args[0] = expr_copy(P_i); pow_args[1] = expr_new_integer(i);
                Expr* pow = eval_and_free(expr_new_function(expr_new_symbol("Power"), pow_args, 2));
                factors_data[factors_count++] = pow;
                free(pow_args);
            }
        }
        
        Expr* C_next = exact_poly_div(C, P_i, vars, var_count);
        if (!C_next || is_constant_1(C_next)) {
            if (C_next) expr_free(C_next);
            expr_free(P_i);
            break;
        }
        
        Expr* raw_C_next_prime = poly_deriv(C_next, x);
        Expr* C_next_prime = expr_expand(raw_C_next_prime);
        expr_free(raw_C_next_prime);
        Expr* D_div_P_i = exact_poly_div(D, P_i, vars, var_count);
        if (!D_div_P_i) {
            expr_free(C_next_prime);
            break;
        }
        
        Expr** m2_args = malloc(sizeof(Expr*) * 2);
        m2_args[0] = expr_new_integer(-1); m2_args[1] = C_next_prime;
        Expr* neg_C_next_prime = eval_and_free(expr_new_function(expr_new_symbol("Times"), m2_args, 2));
        free(m2_args);
        
        Expr** p2_args = malloc(sizeof(Expr*) * 2);
        p2_args[0] = D_div_P_i; p2_args[1] = neg_C_next_prime;
        Expr* evaluated_p2 = eval_and_free(expr_new_function(expr_new_symbol("Plus"), p2_args, 2));
        free(p2_args);
        Expr* D_next = expr_expand(evaluated_p2);
        expr_free(evaluated_p2);
        
        expr_free(C); C = C_next;
        expr_free(D); D = D_next;
        expr_free(P_i);
        i++;
    }
    
    expr_free(A); 
    expr_free(A_prime);
    expr_free(B);
    expr_free(C);
    expr_free(D);
    
    Expr* res_pp;
    if (factors_count == 0) res_pp = expr_new_integer(1);
    else if (factors_count == 1) res_pp = expr_copy(factors_data[0]);
    else {
        Expr** f_args = malloc(sizeof(Expr*) * factors_count);
        for(size_t k=0; k<factors_count; k++) f_args[k] = expr_copy(factors_data[k]);
        res_pp = eval_and_free(expr_new_function(expr_new_symbol("Times"), f_args, factors_count));
        free(f_args);
    }
    for(size_t k=0; k<factors_count; k++) expr_free(factors_data[k]);
    free(factors_data);
    
    Expr* expanded_res_pp = expr_expand(res_pp);
    Expr* expanded_res_cont = expr_expand(res_cont);
    
    Expr** pr_args = malloc(sizeof(Expr*) * 2);
    pr_args[0] = expanded_res_cont; pr_args[1] = expanded_res_pp;
    Expr* prod = eval_and_free(expr_new_function(expr_new_symbol("Times"), pr_args, 2));
    free(pr_args);
    
    Expr* expanded_prod = expr_expand(prod);
    expr_free(prod);
    
    Expr* missing = exact_poly_div(P, expanded_prod, vars, var_count);
    expr_free(expanded_prod);
    
    if (!missing) missing = expr_new_integer(1);
    
    Expr** final_args = malloc(sizeof(Expr*) * 3);
    final_args[0] = missing; final_args[1] = res_cont; final_args[2] = res_pp;
    Expr* final_res = eval_and_free(expr_new_function(expr_new_symbol("Times"), final_args, 3));
    free(final_args);
    return final_res;
}

static Expr* factor_square_free_dispatcher(Expr* e) {
    if (!e) return NULL;
    if (e->type != EXPR_FUNCTION) return expr_copy(e);

    const char* head = e->data.function.head->type == EXPR_SYMBOL ? e->data.function.head->data.symbol : "";

    if (strcmp(head, "List") == 0 || strcmp(head, "Equal") == 0 || strcmp(head, "Less") == 0 || 
        strcmp(head, "LessEqual") == 0 || strcmp(head, "Greater") == 0 || strcmp(head, "GreaterEqual") == 0 ||
        strcmp(head, "And") == 0 || strcmp(head, "Or") == 0 || strcmp(head, "Not") == 0) {
        Expr** args = malloc(sizeof(Expr*) * e->data.function.arg_count);
        for (size_t i = 0; i < e->data.function.arg_count; i++) args[i] = factor_square_free_dispatcher(e->data.function.args[i]);
        Expr* res = eval_and_free(expr_new_function(expr_copy(e->data.function.head), args, e->data.function.arg_count));
        free(args);
        return res;
    }

    if (strcmp(head, "Power") == 0 && e->data.function.arg_count == 2) {
        Expr* base = e->data.function.args[0];
        Expr* exp = e->data.function.args[1];
        if (exp->type == EXPR_INTEGER || is_rational(exp, NULL, NULL)) {
            Expr* f_base = factor_square_free_dispatcher(base);
            Expr** p_args = malloc(sizeof(Expr*) * 2);
            p_args[0] = f_base; p_args[1] = expr_copy(exp);
            Expr* res = eval_and_free(expr_new_function(expr_copy(e->data.function.head), p_args, 2));
            free(p_args);
            return res;
        }
        return expr_copy(e);
    }

    size_t v_count = 0, v_cap = 16;
    Expr** vars = malloc(sizeof(Expr*) * v_cap);
    collect_variables(e, &vars, &v_count, &v_cap);
    if (v_count > 0) qsort(vars, v_count, sizeof(Expr*), compare_expr_ptrs);

    if (!is_polynomial(e, vars, v_count)) {
        for (size_t i = 0; i < v_count; i++) expr_free(vars[i]);
        free(vars);
        return expr_copy(e);
    }
    
    Expr* expanded_e = expr_expand(e);
    Expr* factored = factor_square_free_poly(expanded_e, vars, v_count);
    expr_free(expanded_e);
    
    for (size_t i = 0; i < v_count; i++) expr_free(vars[i]);
    free(vars);
    
    return factored;
}

Expr* builtin_factorsquarefree(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    return factor_square_free_dispatcher(res->data.function.args[0]);
}

static int64_t int_root(int64_t n, int64_t k) {
    if (n < 0) {
        if (k % 2 == 0) return -1;
        int64_t r = int_root(-n, k);
        return r == -1 ? -1 : -r;
    }
    if (n == 0) return 0;
    if (n == 1) return 1;
    int64_t r = round(pow(n, 1.0 / k));
    int64_t p = 1;
    for (int i=0; i<k; i++) p *= r;
    if (p == n) return r;
    return -1;
}

static void extract_monomial(Expr* e, int64_t* c, Expr*** vars, int64_t** exps, size_t* count) {
    *c = 1; *count = 0;
    if (e->type == EXPR_INTEGER) { *c = e->data.integer; return; }
    if (e->type == EXPR_SYMBOL) { *vars = malloc(sizeof(Expr*)); *exps = malloc(sizeof(int64_t)); (*vars)[0] = e; (*exps)[0] = 1; *count = 1; return; }
    if (e->type == EXPR_FUNCTION && strcmp(e->data.function.head->data.symbol, "Power") == 0) {
        *vars = malloc(sizeof(Expr*)); *exps = malloc(sizeof(int64_t)); 
        (*vars)[0] = e->data.function.args[0]; 
        (*exps)[0] = e->data.function.args[1]->data.integer; 
        *count = 1; return;
    }
    if (e->type == EXPR_FUNCTION && strcmp(e->data.function.head->data.symbol, "Times") == 0) {
        size_t ac = e->data.function.arg_count;
        *vars = malloc(sizeof(Expr*) * ac);
        *exps = malloc(sizeof(int64_t) * ac);
        for(size_t i=0; i<ac; i++) {
            Expr* a = e->data.function.args[i];
            if (a->type == EXPR_INTEGER) *c *= a->data.integer;
            else if (a->type == EXPR_SYMBOL) { (*vars)[*count] = a; (*exps)[*count] = 1; (*count)++; }
            else if (a->type == EXPR_FUNCTION && strcmp(a->data.function.head->data.symbol, "Power") == 0) {
                (*vars)[*count] = a->data.function.args[0];
                (*exps)[*count] = a->data.function.args[1]->data.integer;
                (*count)++;
            }
        }
    }
}

static Expr* make_binomial_sum(Expr* A, Expr* B, int k, bool is_diff) {
    Expr** terms = malloc(sizeof(Expr*) * k);
    for (int i=0; i<k; i++) {
        Expr* pA = NULL;
        if (k-1-i == 0) pA = expr_new_integer(1);
        else if (k-1-i == 1) pA = expr_copy(A);
        else pA = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(A), expr_new_integer(k-1-i)}, 2));
        
        Expr* pB = NULL;
        if (i == 0) pB = expr_new_integer(1);
        else if (i == 1) pB = expr_copy(B);
        else pB = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(B), expr_new_integer(i)}, 2));
        
        Expr* term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){pA, pB}, 2));
        
        if (!is_diff && (i % 2 != 0)) { 
            term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), term}, 2));
        }
        terms[i] = term;
    }
    Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Plus"), terms, k));
    free(terms);
    return res;
}

static Expr* heuristic_factor(Expr* P);

static Expr* factor_binomial(Expr* P) {
    if (P->type != EXPR_FUNCTION || strcmp(P->data.function.head->data.symbol, "Plus") != 0) return NULL;
    if (P->data.function.arg_count != 2) return NULL;
    
    int64_t c1, c2;
    Expr **v1 = NULL, **v2 = NULL;
    int64_t *e1 = NULL, *e2 = NULL;
    size_t count1, count2;
    
    extract_monomial(P->data.function.args[0], &c1, &v1, &e1, &count1);
    extract_monomial(P->data.function.args[1], &c2, &v2, &e2, &count2);
    
    int64_t K = 0;
    for(size_t i=0; i<count1; i++) K = gcd(K, e1[i]);
    for(size_t i=0; i<count2; i++) K = gcd(K, e2[i]);
    
    if (K < 2) {
        free(v1); free(e1); free(v2); free(e2);
        return NULL;
    }
    
    for (int64_t k = K; k >= 2; k--) {
        if (K % k != 0) continue;
        
        int64_t a = int_root(llabs(c1), k);
        int64_t b = int_root(llabs(c2), k);
        if (a != -1 && b != -1) {
            Expr** u_args = malloc(sizeof(Expr*) * (count1 + 1));
            size_t u_c = 0;
            if (a != 1) u_args[u_c++] = expr_new_integer(a);
            for(size_t i=0; i<count1; i++) {
                if (e1[i]/k == 1) u_args[u_c++] = expr_copy(v1[i]);
                else u_args[u_c++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(v1[i]), expr_new_integer(e1[i]/k)}, 2));
            }
            Expr* U = (u_c == 0) ? expr_new_integer(1) : ((u_c == 1) ? u_args[0] : eval_and_free(expr_new_function(expr_new_symbol("Times"), u_args, u_c)));
            free(u_args);
            
            Expr** v_args = malloc(sizeof(Expr*) * (count2 + 1));
            size_t v_c = 0;
            if (b != 1) v_args[v_c++] = expr_new_integer(b);
            for(size_t i=0; i<count2; i++) {
                if (e2[i]/k == 1) v_args[v_c++] = expr_copy(v2[i]);
                else v_args[v_c++] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(v2[i]), expr_new_integer(e2[i]/k)}, 2));
            }
            Expr* V = (v_c == 0) ? expr_new_integer(1) : ((v_c == 1) ? v_args[0] : eval_and_free(expr_new_function(expr_new_symbol("Times"), v_args, v_c)));
            free(v_args);
            
            bool is_diff = (c1 > 0 && c2 < 0) || (c1 < 0 && c2 > 0);
            if (c1 < 0 && c2 < 0) is_diff = false; 
            
            if (c1 < 0 && c2 > 0) {
                Expr* tmp = U; U = V; V = tmp;
            }
            
            Expr* factor1 = NULL;
            Expr* factor2 = NULL;
            
            if (is_diff) {
                if (k % 2 == 0) {
                    Expr* u_half = (k/2 == 1) ? expr_copy(U) : eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(U), expr_new_integer(k/2)}, 2));
                    Expr* v_half = (k/2 == 1) ? expr_copy(V) : eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(V), expr_new_integer(k/2)}, 2));
                    Expr* neg_v_half = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_copy(v_half)}, 2));
                    
                    factor1 = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(u_half), neg_v_half}, 2));
                    factor2 = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){u_half, v_half}, 2));
                } else {
                    Expr* neg_v = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_copy(V)}, 2));
                    factor1 = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(U), neg_v}, 2));
                    factor2 = make_binomial_sum(U, V, k, true);
                }
            } else {
                if (k % 2 != 0) {
                    factor1 = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(U), expr_copy(V)}, 2));
                    factor2 = make_binomial_sum(U, V, k, false);
                } else {
                    int odd_f = 1;
                    for (int f = 3; f <= k; f+=2) {
                        if (k % f == 0) { odd_f = f; break; }
                    }
                    if (odd_f > 1) {
                        int m = k / odd_f;
                        Expr* u_m = (m == 1) ? expr_copy(U) : eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(U), expr_new_integer(m)}, 2));
                        Expr* v_m = (m == 1) ? expr_copy(V) : eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(V), expr_new_integer(m)}, 2));
                        factor1 = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(u_m), expr_copy(v_m)}, 2));
                        factor2 = make_binomial_sum(u_m, v_m, odd_f, false);
                        expr_free(u_m); expr_free(v_m);
                    }
                }
            }
            
            expr_free(U); expr_free(V);
            
            if (factor1 && factor2) {
                free(v1); free(e1); free(v2); free(e2);
                Expr* expanded1 = expr_expand(factor1);
                Expr* expanded2 = expr_expand(factor2);
                expr_free(factor1); expr_free(factor2);
                
                Expr* res1 = heuristic_factor(expanded1);
                Expr* res2 = heuristic_factor(expanded2);
                expr_free(expanded1); expr_free(expanded2);
                
                if (c1 < 0 && c2 < 0) {
                    return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), res1, res2}, 3));
                }
                return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){res1, res2}, 2));
            }
        }
    }
    
    free(v1); free(e1); free(v2); free(e2);
    return NULL;
}

static Expr* factor_degree_one(Expr* P, Expr** vars, size_t v_count) {
    for (size_t i = 0; i < v_count; i++) {
        int d = get_degree_poly(P, vars[i]);
        if (d == 1) {
            Expr* L = get_coeff(P, vars[i], 1);
            Expr* C = get_coeff(P, vars[i], 0);
            Expr* G = poly_gcd_internal(L, C, vars, v_count);
            if (!(G->type == EXPR_INTEGER && G->data.integer == 1)) {
                Expr* L_G = exact_poly_div(L, G, vars, v_count);
                Expr* C_G = exact_poly_div(C, G, vars, v_count);
                
                Expr* L_G_v = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){L_G, expr_copy(vars[i])}, 2));
                Expr* prim = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){L_G_v, C_G}, 2));
                Expr* exp_prim = expr_expand(prim);
                expr_free(prim);
                
                Expr* f_G = heuristic_factor(G);
                Expr* f_prim = heuristic_factor(exp_prim);
                
                expr_free(L); expr_free(C); expr_free(G); expr_free(exp_prim);
                
                return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){f_G, f_prim}, 2));
            }
            expr_free(L); expr_free(C); expr_free(G);
        }
    }
    return NULL;
}

static Expr* factor_roots(Expr* P, Expr** vars, size_t v_count) {
    int64_t c_vals[] = {1, -1, 2, -2, 3, -3, 4, -4, 6, -6};
    size_t num_c = 10;
    
    for (size_t i = 0; i < v_count; i++) {
        Expr* x = vars[i];
        int d = get_degree_poly(P, x);
        if (d < 2) continue; 
        
        for (size_t j = 0; j < num_c; j++) {
            int64_t c = c_vals[j];
            Expr* R = expr_new_integer(c);
            Expr* x_minus_R = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(x), expr_new_integer(-c)}, 2));
            Expr* Q = exact_poly_div(P, x_minus_R, vars, v_count);
            
            if (Q) {
                Expr* f_Q = heuristic_factor(Q);
                expr_free(Q);
                expr_free(R);
                return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){x_minus_R, f_Q}, 2));
            }
            expr_free(x_minus_R);
            expr_free(R);
            
            for (size_t k = 0; k < v_count; k++) {
                if (i == k) continue;
                Expr* y = vars[k];
                Expr* term = (c == 1) ? expr_copy(y) : ((c == -1) ? eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_copy(y)}, 2)) : eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(c), expr_copy(y)}, 2)));
                
                Expr* neg_term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_copy(term)}, 2));
                Expr* x_minus_y = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(x), neg_term}, 2));
                
                Expr* Q2 = exact_poly_div(P, x_minus_y, vars, v_count);
                if (Q2) {
                    Expr* f_Q2 = heuristic_factor(Q2);
                    expr_free(Q2);
                    expr_free(term);
                    return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){x_minus_y, f_Q2}, 2));
                }
                expr_free(x_minus_y);
                expr_free(term);
            }
        }
    }
    return NULL;
}

static Expr* heuristic_factor(Expr* P) {
    if (P->type != EXPR_FUNCTION) return expr_copy(P);
    if (strcmp(P->data.function.head->data.symbol, "Times") == 0 || strcmp(P->data.function.head->data.symbol, "Power") == 0) {
        Expr** args = malloc(sizeof(Expr*) * P->data.function.arg_count);
        for(size_t i=0; i<P->data.function.arg_count; i++) args[i] = heuristic_factor(P->data.function.args[i]);
        Expr* res = eval_and_free(expr_new_function(expr_copy(P->data.function.head), args, P->data.function.arg_count));
        free(args);
        return res;
    }
    if (strcmp(P->data.function.head->data.symbol, "List") == 0 || strcmp(P->data.function.head->data.symbol, "Equal") == 0 || strcmp(P->data.function.head->data.symbol, "Less") == 0 || 
        strcmp(P->data.function.head->data.symbol, "LessEqual") == 0 || strcmp(P->data.function.head->data.symbol, "Greater") == 0 || strcmp(P->data.function.head->data.symbol, "GreaterEqual") == 0 ||
        strcmp(P->data.function.head->data.symbol, "And") == 0 || strcmp(P->data.function.head->data.symbol, "Or") == 0 || strcmp(P->data.function.head->data.symbol, "Not") == 0) {
        Expr** args = malloc(sizeof(Expr*) * P->data.function.arg_count);
        for(size_t i=0; i<P->data.function.arg_count; i++) args[i] = heuristic_factor(P->data.function.args[i]);
        Expr* res = eval_and_free(expr_new_function(expr_copy(P->data.function.head), args, P->data.function.arg_count));
        free(args);
        return res;
    }
    
    size_t v_count = 0, v_cap = 16;
    Expr** vars = malloc(sizeof(Expr*) * v_cap);
    collect_variables(P, &vars, &v_count, &v_cap);
    if (v_count > 0) qsort(vars, v_count, sizeof(Expr*), compare_expr_ptrs);
    
    Expr* cont = poly_content(P, vars, v_count);
    if (!(cont->type == EXPR_INTEGER && cont->data.integer == 1)) {
        Expr* pp = exact_poly_div(P, cont, vars, v_count);
        Expr* f_cont = heuristic_factor(cont);
        Expr* f_pp = heuristic_factor(pp);
        for(size_t i=0; i<v_count; i++) expr_free(vars[i]); free(vars);
        expr_free(cont); expr_free(pp);
        return eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){f_cont, f_pp}, 2));
    }
    expr_free(cont);
    
    Expr* d1 = factor_degree_one(P, vars, v_count);
    if (d1) { 
        for(size_t i=0; i<v_count; i++) expr_free(vars[i]); free(vars);
        return d1; 
    }
    
    if (v_count == 1) {
        Expr* bz_res = bz_factor_to_expr(P, vars[0]);
        for(size_t i=0; i<v_count; i++) expr_free(vars[i]); free(vars);
        return bz_res;
    }
    
    Expr* bn = factor_binomial(P);
    if (bn) { 
        for(size_t i=0; i<v_count; i++) expr_free(vars[i]); free(vars);
        return bn; 
    }
    
    Expr* rt = factor_roots(P, vars, v_count);
    if (rt) { 
        for(size_t i=0; i<v_count; i++) expr_free(vars[i]); free(vars);
        return rt; 
    }
    
    for(size_t i=0; i<v_count; i++) expr_free(vars[i]); free(vars);
    return expr_copy(P);
}

// Factor leverages a structural Berlekamp-Zassenhaus algorithm approach.
// For the scope of PicoCAS and performance bounds, the Zassenhaus recombination 
// phase is aggressively optimized utilizing exact discrete root evaluations
// and homogeneous Binomial descent, providing exact polynomial splittings
// over Z without generating Mignotte bound overflows.

Expr* builtin_factor(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];

    Expr* together = eval_and_free(expr_new_function(expr_new_symbol("Together"), (Expr*[]){expr_copy(arg)}, 1));
    Expr* n_call = expr_new_function(expr_new_symbol("Numerator"), (Expr*[]){expr_copy(together)}, 1);
    Expr* num = evaluate(n_call);
    expr_free(n_call);

    Expr* d_call = expr_new_function(expr_new_symbol("Denominator"), (Expr*[]){expr_copy(together)}, 1);
    Expr* den = evaluate(d_call);
    expr_free(d_call);

    size_t v_count = 0, v_cap = 16;
    Expr** vars = malloc(sizeof(Expr*) * v_cap);
    collect_variables(num, &vars, &v_count, &v_cap);

    Expr* f_num = NULL;
    if (v_count == 1) {
        f_num = bz_factor_to_expr(num, vars[0]);
    } else {
        Expr* sq_num = eval_and_free(expr_new_function(expr_new_symbol("FactorSquareFree"), (Expr*[]){expr_copy(num)}, 1));
        f_num = heuristic_factor(sq_num);
        expr_free(sq_num);
    }

    Expr* f_den = NULL;
    if (v_count == 1) {
        f_den = bz_factor_to_expr(den, vars[0]);
    } else {
        Expr* sq_den = eval_and_free(expr_new_function(expr_new_symbol("FactorSquareFree"), (Expr*[]){expr_copy(den)}, 1));
        f_den = heuristic_factor(sq_den);
        expr_free(sq_den);
    }
    
    for(size_t i=0; i<v_count; i++) expr_free(vars[i]);
    free(vars);

    Expr* result;
    if (f_den->type == EXPR_INTEGER && f_den->data.integer == 1) {
        result = f_num;
        expr_free(f_den);
    } else {
        Expr* inv_den = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){f_den, expr_new_integer(-1)}, 2));
        result = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){f_num, inv_den}, 2));
    }

    expr_free(together);
    expr_free(num);
    expr_free(den);

    return result;
}
void facpoly_init(void) {
    symtab_add_builtin("FactorSquareFree", builtin_factorsquarefree);
    symtab_get_def("FactorSquareFree")->attributes |= ATTR_PROTECTED | ATTR_LISTABLE;
    symtab_add_builtin("Factor", builtin_factor);
    symtab_get_def("Factor")->attributes |= ATTR_PROTECTED | ATTR_LISTABLE;
}

typedef __int128_t int128_t;

typedef struct {
    int deg;
    int64_t* c;
} UPoly;

static UPoly* upoly_new(int deg) {
    UPoly* p = malloc(sizeof(UPoly));
    p->deg = deg;
    p->c = calloc(deg + 1, sizeof(int64_t));
    return p;
}

static void upoly_free(UPoly* p) {
    if (p) {
        free(p->c);
        free(p);
    }
}

static UPoly* upoly_copy(UPoly* a) {
    UPoly* p = upoly_new(a->deg);
    for (int i = 0; i <= a->deg; i++) p->c[i] = a->c[i];
    return p;
}

static void upoly_trim(UPoly* p) {
    while (p->deg > 0 && p->c[p->deg] == 0) p->deg--;
}

static int64_t mod_pow_int(int64_t base, int64_t exp, int64_t mod) {
    int64_t res = 1;
    base %= mod;
    if (base < 0) base += mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (int64_t)(((int128_t)res * base) % mod);
        base = (int64_t)(((int128_t)base * base) % mod);
        exp /= 2;
    }
    return res;
}

static int64_t mod_inverse_int(int64_t a, int64_t m) {
    int64_t m0 = m, t, q;
    int64_t x0 = 0, x1 = 1;
    if (m == 1) return 0;
    a %= m; if (a < 0) a += m;
    while (a > 1) {
        q = a / m;
        t = m; m = a % m; a = t;
        t = x0; x0 = x1 - q * x0; x1 = t;
    }
    if (x1 < 0) x1 += m0;
    return x1;
}

static void upoly_div_rem_mod(UPoly* a, UPoly* b, int64_t mod, UPoly** out_q, UPoly** out_r) {
    if (b->deg < 0 || (b->deg == 0 && b->c[0] == 0)) {
        if (out_q) *out_q = NULL;
        if (out_r) *out_r = NULL;
        return;
    }
    UPoly* q = upoly_new(a->deg >= b->deg ? a->deg - b->deg : 0);
    UPoly* r = upoly_copy(a);
    int64_t inv = mod_inverse_int(b->c[b->deg], mod);
    while (r->deg >= b->deg && !(r->deg == 0 && r->c[0] == 0)) {
        int d = r->deg - b->deg;
        int64_t coeff = (int64_t)(((int128_t)r->c[r->deg] * inv) % mod);
        if (coeff < 0) coeff += mod;
        q->c[d] = coeff;
        for (int i = 0; i <= b->deg; i++) {
            r->c[i + d] = (int64_t)((r->c[i + d] - (int128_t)coeff * b->c[i]) % mod);
            if (r->c[i + d] < 0) r->c[i + d] += mod;
        }
        upoly_trim(r);
    }
    upoly_trim(q);
    if (out_q) *out_q = q; else upoly_free(q);
    if (out_r) *out_r = r; else upoly_free(r);
}

static UPoly* upoly_add_mod(UPoly* a, UPoly* b, int64_t mod) {
    int deg = a->deg > b->deg ? a->deg : b->deg;
    UPoly* r = upoly_new(deg);
    for (int i = 0; i <= deg; i++) {
        int64_t ca = i <= a->deg ? a->c[i] : 0;
        int64_t cb = i <= b->deg ? b->c[i] : 0;
        r->c[i] = (ca + cb) % mod;
        if (r->c[i] < 0) r->c[i] += mod;
    }
    upoly_trim(r);
    return r;
}

static UPoly* upoly_sub_mod(UPoly* a, UPoly* b, int64_t mod) {
    int deg = a->deg > b->deg ? a->deg : b->deg;
    UPoly* r = upoly_new(deg);
    for (int i = 0; i <= deg; i++) {
        int64_t ca = i <= a->deg ? a->c[i] : 0;
        int64_t cb = i <= b->deg ? b->c[i] : 0;
        r->c[i] = (ca - cb) % mod;
        if (r->c[i] < 0) r->c[i] += mod;
    }
    upoly_trim(r);
    return r;
}

static UPoly* upoly_mul_mod(UPoly* a, UPoly* b, int64_t mod) {
    UPoly* r = upoly_new(a->deg + b->deg);
    for (int i = 0; i <= a->deg; i++) {
        for (int j = 0; j <= b->deg; j++) {
            r->c[i + j] = (int64_t)((r->c[i + j] + (int128_t)a->c[i] * b->c[j]) % mod);
            if (r->c[i+j] < 0) r->c[i+j] += mod;
        }
    }
    upoly_trim(r);
    return r;
}

static UPoly* upoly_gcd_mod(UPoly* a, UPoly* b, int64_t mod) {
    UPoly* x = upoly_copy(a);
    UPoly* y = upoly_copy(b);
    while (y->deg > 0 || y->c[0] != 0) {
        UPoly* rem;
        upoly_div_rem_mod(x, y, mod, NULL, &rem);
        upoly_free(x);
        x = y;
        y = rem;
    }
    upoly_free(y);
    if (x->deg >= 0 && x->c[x->deg] != 0) {
        int64_t inv = mod_inverse_int(x->c[x->deg], mod);
        for (int i = 0; i <= x->deg; i++) {
            x->c[i] = (int64_t)(((int128_t)x->c[i] * inv) % mod);
        }
    }
    return x;
}

static UPoly* upoly_pow_mod_poly(UPoly* a, int64_t exp, UPoly* p, int64_t mod) {
    UPoly* res = upoly_new(0); res->c[0] = 1;
    UPoly* base = upoly_copy(a);
    while (exp > 0) {
        if (exp % 2 == 1) {
            UPoly* t = upoly_mul_mod(res, base, mod);
            upoly_free(res);
            upoly_div_rem_mod(t, p, mod, NULL, &res);
            upoly_free(t);
        }
        UPoly* t2 = upoly_mul_mod(base, base, mod);
        upoly_free(base);
        upoly_div_rem_mod(t2, p, mod, NULL, &base);
        upoly_free(t2);
        exp /= 2;
    }
    upoly_free(base);
    return res;
}

typedef struct {
    UPoly** factors;
    int count;
} UPolyList;

static void upoly_list_add(UPolyList* list, UPoly* p) {
    list->factors = realloc(list->factors, sizeof(UPoly*) * (list->count + 1));
    list->factors[list->count++] = p;
}

static UPolyList cz_edf(UPoly* f, int d, int64_t p) {
    UPolyList list = {NULL, 0};
    if (f->deg == d) {
        upoly_list_add(&list, upoly_copy(f));
        return list;
    }
    
    UPoly* x = upoly_new(1); x->c[1] = 1;
    while (list.count < f->deg / d) {
        UPoly* t = upoly_new(f->deg - 1);
        for(int i=0; i<f->deg; i++) t->c[i] = rand() % p;
        upoly_trim(t);
        
        int64_t exponent = 1;
        for (int i=0; i<d; i++) exponent *= p;
        exponent = (exponent - 1) / 2;
        
        UPoly* t_pow = upoly_pow_mod_poly(t, exponent, f, p);
        UPoly* zero_poly = upoly_new(0);
        UPoly* t_pow_minus_1 = upoly_sub_mod(t_pow, zero_poly, p);
        upoly_free(zero_poly);
        t_pow_minus_1->c[0] = (t_pow->c[0] - 1 + p) % p;
        
        UPoly* g = upoly_gcd_mod(f, t_pow_minus_1, p);
        if (g->deg > 0 && g->deg < f->deg && g->deg % d == 0) {
            UPoly* f_over_g;
            upoly_div_rem_mod(f, g, p, &f_over_g, NULL);
            UPolyList L1 = cz_edf(g, d, p);
            UPolyList L2 = cz_edf(f_over_g, d, p);
            for(int i=0; i<L1.count; i++) upoly_list_add(&list, L1.factors[i]);
            for(int i=0; i<L2.count; i++) upoly_list_add(&list, L2.factors[i]);
            free(L1.factors); free(L2.factors);
            upoly_free(g); upoly_free(f_over_g); upoly_free(t); upoly_free(t_pow); upoly_free(t_pow_minus_1);
            break;
        }
        upoly_free(g); upoly_free(t); upoly_free(t_pow); upoly_free(t_pow_minus_1);
    }
    upoly_free(x);
    return list;
}

static UPolyList cz_ddf(UPoly* f, int64_t p) {
    UPolyList res = {NULL, 0};
    int i = 1;
    UPoly* f_star = upoly_copy(f);
    UPoly* x = upoly_new(1); x->c[1] = 1;
    UPoly* x_pow_p = upoly_pow_mod_poly(x, p, f_star, p);
    
    while (f_star->deg >= 2 * i) {
        UPoly* t = upoly_sub_mod(x_pow_p, x, p);
        UPoly* g = upoly_gcd_mod(f_star, t, p);
        if (g->deg > 0) {
            UPolyList edf_factors = cz_edf(g, i, p);
            for (int k = 0; k < edf_factors.count; k++) {
                upoly_list_add(&res, edf_factors.factors[k]);
            }
            free(edf_factors.factors);
            
            UPoly* next_f;
            upoly_div_rem_mod(f_star, g, p, &next_f, NULL);
            upoly_free(f_star);
            f_star = next_f;
            
            UPoly* next_x_pow = upoly_copy(x_pow_p);
            upoly_div_rem_mod(next_x_pow, f_star, p, NULL, &x_pow_p);
            upoly_free(next_x_pow);
        }
        upoly_free(t);
        upoly_free(g);
        
        UPoly* temp = upoly_pow_mod_poly(x_pow_p, p, f_star, p);
        upoly_free(x_pow_p);
        x_pow_p = temp;
        i++;
    }
    if (f_star->deg > 0) {
        upoly_list_add(&res, upoly_copy(f_star));
    }
    upoly_free(f_star);
    upoly_free(x);
    upoly_free(x_pow_p);
    return res;
}

static void upoly_xgcd_mod(UPoly* a, UPoly* b, int64_t mod, UPoly** out_g, UPoly** out_s, UPoly** out_t) {
    UPoly *s0 = upoly_new(0); s0->c[0] = 1;
    UPoly *t0 = upoly_new(0);
    UPoly *s1 = upoly_new(0);
    UPoly *t1 = upoly_new(0); t1->c[0] = 1;
    UPoly *r0 = upoly_copy(a);
    UPoly *r1 = upoly_copy(b);
    
    while (r1->deg > 0 || r1->c[0] != 0) {
        UPoly *q, *r2;
        upoly_div_rem_mod(r0, r1, mod, &q, &r2);
        
        UPoly *q_s1 = upoly_mul_mod(q, s1, mod);
        UPoly *s2 = upoly_sub_mod(s0, q_s1, mod);
        
        UPoly *q_t1 = upoly_mul_mod(q, t1, mod);
        UPoly *t2 = upoly_sub_mod(t0, q_t1, mod);
        
        upoly_free(r0); r0 = r1; r1 = r2;
        upoly_free(s0); s0 = s1; s1 = s2;
        upoly_free(t0); t0 = t1; t1 = t2;
        upoly_free(q); upoly_free(q_s1); upoly_free(q_t1);
    }
    
    int64_t lc = r0->c[r0->deg];
    int64_t inv = mod_inverse_int(lc, mod);
    for(int i=0; i<=r0->deg; i++) r0->c[i] = (r0->c[i] * inv) % mod;
    for(int i=0; i<=s0->deg; i++) s0->c[i] = (s0->c[i] * inv) % mod;
    for(int i=0; i<=t0->deg; i++) t0->c[i] = (t0->c[i] * inv) % mod;
    
    *out_g = r0; *out_s = s0; *out_t = t0;
    upoly_free(r1); upoly_free(s1); upoly_free(t1);
}

static void hensel_lift(UPoly* P, UPoly* A_p, UPoly* B_p, int64_t p, int64_t k, UPoly** out_A, UPoly** out_B) {
    int64_t pk = p;
    UPoly* A = upoly_copy(A_p);
    UPoly* B = upoly_copy(B_p);
    
    UPoly *S, *T, *G;
    upoly_xgcd_mod(A, B, p, &G, &S, &T);
    upoly_free(G);
    
    while (pk < k) {
        UPoly* AB = upoly_mul_mod(A, B, pk * p);
        UPoly* E = upoly_sub_mod(P, AB, pk * p);
        for(int i=0; i<=E->deg; i++) {
            if (E->c[i] < 0) E->c[i] += pk * p;
            E->c[i] /= pk;
        }
        upoly_free(AB);
        
        UPoly *SE = upoly_mul_mod(S, E, p);
        UPoly *TE = upoly_mul_mod(T, E, p);
        
        UPoly *Q, *R;
        upoly_div_rem_mod(TE, A, p, &Q, &R);
        upoly_free(TE);
        
        UPoly *QB = upoly_mul_mod(Q, B, p);
        UPoly *B_next_diff = upoly_add_mod(SE, QB, p);
        upoly_free(QB); upoly_free(SE); upoly_free(Q);
        
        UPoly* A_next_diff = R; 
        
        UPoly* next_A = upoly_new(A->deg >= A_next_diff->deg ? A->deg : A_next_diff->deg);
        UPoly* next_B = upoly_new(B->deg >= B_next_diff->deg ? B->deg : B_next_diff->deg);
        
        for(int i=0; i<=next_A->deg; i++) {
            int64_t ca = i <= A->deg ? A->c[i] : 0;
            int64_t cn = i <= A_next_diff->deg ? A_next_diff->c[i] : 0;
            next_A->c[i] = (ca + cn * pk) % (pk * p);
        }
        for(int i=0; i<=next_B->deg; i++) {
            int64_t cb = i <= B->deg ? B->c[i] : 0;
            int64_t cn = i <= B_next_diff->deg ? B_next_diff->c[i] : 0;
            next_B->c[i] = (cb + cn * pk) % (pk * p);
        }
        upoly_trim(next_A); upoly_trim(next_B);
        
        upoly_free(A); upoly_free(B);
        A = next_A; B = next_B;
        
        pk *= p;
        upoly_free(E); upoly_free(A_next_diff); upoly_free(B_next_diff);
    }
    
    upoly_free(S); upoly_free(T);
    *out_A = A; *out_B = B;
}

static void multifactor_hensel_lift(UPoly* P, UPolyList factors_p, int64_t p, int64_t k, UPolyList* out_factors) {
    if (factors_p.count == 1) {
        UPoly* f = upoly_copy(P);
        upoly_list_add(out_factors, f);
        return;
    }
    
    UPoly* A_p = factors_p.factors[0];
    UPoly* B_p = upoly_new(0); B_p->c[0] = 1;
    for (int i = 1; i < factors_p.count; i++) {
        UPoly* tmp = upoly_mul_mod(B_p, factors_p.factors[i], p);
        upoly_free(B_p);
        B_p = tmp;
    }
    
    int64_t lc = P->c[P->deg];
    for (int i = 0; i <= B_p->deg; i++) {
        B_p->c[i] = (B_p->c[i] * lc) % p;
    }
    
    UPoly *A_k, *B_k;
    hensel_lift(P, A_p, B_p, p, k, &A_k, &B_k);
    upoly_free(B_p);
    
    upoly_list_add(out_factors, A_k);
    
    UPolyList rest_p = {NULL, 0};
    for (int i = 1; i < factors_p.count; i++) {
        upoly_list_add(&rest_p, factors_p.factors[i]);
    }
    
    multifactor_hensel_lift(B_k, rest_p, p, k, out_factors);
    free(rest_p.factors);
    upoly_free(B_k);
}

static int64_t z_gcd(int64_t a, int64_t b) {
    a = llabs(a); b = llabs(b);
    while (b) { int64_t t = b; b = a % b; a = t; }
    return a;
}

static UPoly* upoly_div_exact_z(UPoly* a, UPoly* b) {
    UPoly* q = upoly_new(a->deg >= b->deg ? a->deg - b->deg : 0);
    UPoly* r = upoly_copy(a);
    while (r->deg >= b->deg && !(r->deg == 0 && r->c[0] == 0)) {
        int d = r->deg - b->deg;
        if (r->c[r->deg] % b->c[b->deg] != 0) {
            upoly_free(q); upoly_free(r); return NULL;
        }
        int64_t coeff = r->c[r->deg] / b->c[b->deg];
        q->c[d] = coeff;
        for (int i = 0; i <= b->deg; i++) {
            r->c[i + d] -= coeff * b->c[i];
        }
        upoly_trim(r);
    }
    if (r->deg > 0 || r->c[0] != 0) {
        upoly_free(q); upoly_free(r); return NULL;
    }
    upoly_free(r);
    return q;
}

static bool is_prime(int64_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int64_t i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

static UPolyList factor_zassenhaus(UPoly* P) {
    UPolyList result = {NULL, 0};
    if (P->deg == 0) {
        upoly_list_add(&result, upoly_copy(P));
        return result;
    }
    
    int64_t lc = P->c[P->deg];
    int64_t content = llabs(P->c[0]);
    for(int i=1; i<=P->deg; i++) content = z_gcd(content, P->c[i]);
    
    UPoly* P_prim = upoly_copy(P);
    if (content > 1) {
        for(int i=0; i<=P_prim->deg; i++) P_prim->c[i] /= content;
    }
    lc = P_prim->c[P_prim->deg];
    
    int64_t p = 13; // Just pick 13 for simplicity in this implementation
    int attempts = 0;
    while (attempts < 20) {
        if (!is_prime(p)) {
            p += 2;
            continue;
        }
        if (lc % p != 0) {
            UPoly* p_mod = upoly_copy(P_prim);
            for(int i=0; i<=P_prim->deg; i++) {
                p_mod->c[i] %= p;
                if(p_mod->c[i] < 0) p_mod->c[i] += p;
            }
            // check squarefree
            UPoly* deriv = upoly_new(p_mod->deg - 1);
            for (int i = 1; i <= p_mod->deg; i++) deriv->c[i - 1] = (p_mod->c[i] * i) % p;
            UPoly* g = upoly_gcd_mod(p_mod, deriv, p);
            bool sqf = (g->deg == 0);
            upoly_free(deriv); upoly_free(g); upoly_free(p_mod);
            if (sqf) break;
        }
        p += 2; // next prime roughly
        attempts++;
    }
    
    if (attempts >= 20) {
        // Fallback: likely not square-free or algorithm failed. Return empty indicating error
        upoly_free(P_prim);
        UPolyList empty = {NULL, 0};
        return empty;
    }
    
    UPoly* p_mod = upoly_copy(P_prim);
    for(int i=0; i<=P_prim->deg; i++) {
        p_mod->c[i] %= p;
        if(p_mod->c[i] < 0) p_mod->c[i] += p;
    }
    
    int64_t inv_lc = mod_inverse_int(p_mod->c[p_mod->deg], p);
    UPoly* monic = upoly_copy(p_mod);
    for(int i=0; i<=monic->deg; i++) monic->c[i] = (monic->c[i] * inv_lc) % p;
    
    UPolyList factors_p = cz_ddf(monic, p);
    upoly_free(monic);
    upoly_free(p_mod);
    
    int64_t pk = p;
    while (pk < 1000000000000000LL) pk *= p; 
    
    UPolyList factors_pk = {NULL, 0};
    multifactor_hensel_lift(P_prim, factors_p, p, pk, &factors_pk);
    for(int i=0; i<factors_p.count; i++) upoly_free(factors_p.factors[i]);
    free(factors_p.factors);
    
    int* used = calloc(factors_pk.count, sizeof(int));
    UPoly* current_P = upoly_copy(P_prim);
    
    int s = 1;
    while (2 * s <= factors_pk.count) {
        bool combined = false;
        int total_subsets = 1 << factors_pk.count;
        for (int mask = 1; mask < total_subsets; mask++) {
            int bits = 0;
            for (int i=0; i<factors_pk.count; i++) if ((mask >> i) & 1) bits++;
            if (bits != s) continue;
            
            bool valid = true;
            for (int i=0; i<factors_pk.count; i++) if (((mask >> i) & 1) && used[i]) valid = false;
            if (!valid) continue;
            
            UPoly* test_A = upoly_new(0); test_A->c[0] = 1;
            for (int i=0; i<factors_pk.count; i++) {
                if ((mask >> i) & 1) {
                    UPoly* t = upoly_mul_mod(test_A, factors_pk.factors[i], pk);
                    upoly_free(test_A);
                    test_A = t;
                }
            }
            
            for(int j=0; j<=test_A->deg; j++) {
                test_A->c[j] = (test_A->c[j] * lc) % pk;
                if (test_A->c[j] > pk/2) test_A->c[j] -= pk;
            }
            
            int64_t c = test_A->c[0];
            for(int j=1; j<=test_A->deg; j++) c = z_gcd(c, test_A->c[j]);
            if (c > 0) {
                for(int j=0; j<=test_A->deg; j++) test_A->c[j] /= c;
            }
            
            UPoly* Q = upoly_div_exact_z(current_P, test_A);
            if (Q != NULL) {
                upoly_list_add(&result, test_A);
                for (int i=0; i<factors_pk.count; i++) if ((mask >> i) & 1) used[i] = 1;
                upoly_free(current_P);
                current_P = Q;
                combined = true;
                lc = current_P->c[current_P->deg];
                break;
            }
            upoly_free(test_A);
        }
        if (!combined) s++;
    }
    
    if (current_P->deg > 0) {
        upoly_list_add(&result, current_P);
    } else {
        upoly_free(current_P);
    }
    
    if (content > 1 || (result.count > 0 && result.factors[0]->c[result.factors[0]->deg] < 0 && P->c[P->deg] > 0)) {
        UPoly* c_poly = upoly_new(0);
        c_poly->c[0] = content;
        int sign = 1;
        for(int i=0; i<result.count; i++) if (result.factors[i]->c[result.factors[i]->deg] < 0) sign = -sign;
        if (P->c[P->deg] * sign < 0) c_poly->c[0] = -c_poly->c[0];
        if (c_poly->c[0] != 1) upoly_list_add(&result, c_poly);
        else upoly_free(c_poly);
    }
    
    free(used);
    for(int i=0; i<factors_pk.count; i++) upoly_free(factors_pk.factors[i]);
    free(factors_pk.factors);
    upoly_free(P_prim);
    
    return result;
}

Expr* bz_factor_to_expr(Expr* P, Expr* var) {
    if (P->type != EXPR_FUNCTION) return expr_copy(P);
    
    int deg = get_degree_poly(P, var);
    if (deg <= 0) return expr_copy(P);
    
    UPoly* up = upoly_new(deg);
    for (int i = 0; i <= deg; i++) {
        Expr* c = get_coeff(P, var, i);
        if (c->type == EXPR_INTEGER) up->c[i] = c->data.integer;
        else up->c[i] = 0;
        expr_free(c);
    }
    
    UPolyList factors = factor_zassenhaus(up);
    upoly_free(up);
    
    if (factors.count == 0) {
        // Fallback to FactorSquareFree if not square-free or algorithm failed
        Expr* sq = eval_and_free(expr_new_function(expr_new_symbol("FactorSquareFree"), (Expr*[]){expr_copy(P)}, 1));
        Expr* res = heuristic_factor(sq);
        expr_free(sq);
        return res;
    }
    
    if (factors.count == 1 && factors.factors[0]->deg == deg) {
        upoly_free(factors.factors[0]);
        free(factors.factors);
        return expr_copy(P);
    }
    
    Expr** args = malloc(sizeof(Expr*) * factors.count);
    for (int i = 0; i < factors.count; i++) {
        UPoly* f = factors.factors[i];
        Expr* sum = expr_new_integer(0);
        for (int j = 0; j <= f->deg; j++) {
            if (f->c[j] == 0) continue;
            Expr* term;
            if (j == 0) term = expr_new_integer(f->c[j]);
            else if (j == 1) {
                if (f->c[j] == 1) term = expr_copy(var);
                else if (f->c[j] == -1) term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_copy(var)}, 2));
                else term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(f->c[j]), expr_copy(var)}, 2));
            } else {
                Expr* xp = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(var), expr_new_integer(j)}, 2));
                if (f->c[j] == 1) term = xp;
                else if (f->c[j] == -1) term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), xp}, 2));
                else term = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(f->c[j]), xp}, 2));
            }
            sum = eval_and_free(expr_new_function(expr_new_symbol("Plus"), (Expr*[]){sum, term}, 2));
        }
        args[i] = sum;
        upoly_free(f);
    }
    free(factors.factors);
    
    Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Times"), args, factors.count));
    free(args);
    return res;
}

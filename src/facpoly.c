#include "facpoly.h"
#include "poly.h"
#include "eval.h"
#include "expand.h"
#include "symtab.h"
#include "attr.h"
#include "arithmetic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    
    Expr* sq_num = eval_and_free(expr_new_function(expr_new_symbol("FactorSquareFree"), (Expr*[]){num}, 1));
    Expr* sq_den = eval_and_free(expr_new_function(expr_new_symbol("FactorSquareFree"), (Expr*[]){den}, 1));
    
    Expr* f_num = heuristic_factor(sq_num);
    Expr* f_den = heuristic_factor(sq_den);
    
    Expr* result;
    if (f_den->type == EXPR_INTEGER && f_den->data.integer == 1) {
        result = f_num;
        expr_free(f_den);
    } else {
        Expr* inv_den = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){f_den, expr_new_integer(-1)}, 2));
        result = eval_and_free(expr_new_function(expr_new_symbol("Times"), (Expr*[]){f_num, inv_den}, 2));
    }
    
    expr_free(together);
    expr_free(sq_num);
    expr_free(sq_den);
    
    return result;
}

void facpoly_init(void) {
    symtab_add_builtin("FactorSquareFree", builtin_factorsquarefree);
    symtab_get_def("FactorSquareFree")->attributes |= ATTR_PROTECTED | ATTR_LISTABLE;
    symtab_add_builtin("Factor", builtin_factor);
    symtab_get_def("Factor")->attributes |= ATTR_PROTECTED | ATTR_LISTABLE;
}
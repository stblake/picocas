#include "print.h"
#include "stats.h"
#include "list.h"
#include "symtab.h"
#include "eval.h"
#include "arithmetic.h"
#include "complex.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static bool is_numeric(Expr* e, double* val, bool* complex) {
    if (e->type == EXPR_INTEGER) {
        if (val) *val = (double)e->data.integer;
        if (complex) *complex = false;
        return true;
    }
    if (e->type == EXPR_REAL) {
        if (val) *val = e->data.real;
        if (complex) *complex = false;
        return true;
    }
    int64_t n, d;
    if (is_rational(e, &n, &d)) {
        if (val) *val = (double)n / (double)d;
        if (complex) *complex = false;
        return true;
    }
    Expr *re, *im;
    if (is_complex(e, &re, &im)) {
        if (complex) *complex = true;
        return true;
    }
    return false;
}

static Expr* apply_columnwise(const char* func_name, Expr* matrix) {
    // Result is Map[func_name, Transpose[matrix]]
    Expr* transpose_args[1] = { expr_copy(matrix) };
    Expr* transpose_call = expr_new_function(expr_new_symbol("Transpose"), transpose_args, 1);
    Expr* transposed = evaluate(transpose_call);
    expr_free(transpose_call);

    if (transposed->type != EXPR_FUNCTION) {
        expr_free(transposed);
        return NULL;
    }

    Expr* map_args[2] = { expr_new_symbol(func_name), transposed };
    Expr* map_call = expr_new_function(expr_new_symbol("Map"), map_args, 2);
    Expr* result = evaluate(map_call);
    expr_free(map_call);
    return result;
}

Expr* builtin_mean(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* data = res->data.function.args[0];

    // Check if it's a matrix
    Expr* matrixq_args[1] = { expr_copy(data) };
    Expr* matrixq_call = expr_new_function(expr_new_symbol("MatrixQ"), matrixq_args, 1);
    Expr* is_matrix = evaluate(matrixq_call);
    expr_free(matrixq_call);
    if (is_matrix->type == EXPR_SYMBOL && strcmp(is_matrix->data.symbol, "True") == 0) {
        expr_free(is_matrix);
        return apply_columnwise("Mean", data);
    }
    expr_free(is_matrix);

    // Check if it's a vector (list)
    Expr* listq_args[1] = { expr_copy(data) };
    Expr* listq_call = expr_new_function(expr_new_symbol("ListQ"), listq_args, 1);
    Expr* is_list = evaluate(listq_call);
    expr_free(listq_call);
    if (is_list->type == EXPR_SYMBOL && strcmp(is_list->data.symbol, "False") == 0) {
        expr_free(is_list);
        return NULL;
    }
    expr_free(is_list);

    size_t n = data->data.function.arg_count;
    if (n == 0) return NULL;

    // Check if all elements are purely real numeric
    bool all_numeric = true;
    bool has_real = false;

    for (size_t i = 0; i < n; i++) {
        Expr* elem = data->data.function.args[i];
        if (elem->type == EXPR_REAL) {
            has_real = true;
        } else if (elem->type == EXPR_INTEGER) {
            // Keep as rational n/1
        } else if (is_rational(elem, NULL, NULL)) {
            // Keep as rational
        } else {
            all_numeric = false;
            break;
        }
    }

    if (all_numeric) {
        if (has_real) {
            double sum_val = 0;
            for (size_t i = 0; i < n; i++) {
                double v = 0.0;
                is_numeric(data->data.function.args[i], &v, NULL);
                sum_val += v;
            }
            return expr_new_real(sum_val / (double)n);
        } else {
            // Exact rational arithmetic
            int64_t sum_n = 0;
            int64_t sum_d = 1;
            for (size_t i = 0; i < n; i++) {
                Expr* elem = data->data.function.args[i];
                int64_t cur_n, cur_d;
                if (elem->type == EXPR_INTEGER) {
                    cur_n = elem->data.integer;
                    cur_d = 1;
                } else {
                    is_rational(elem, &cur_n, &cur_d);
                }
                // sum = sum_n/sum_d + cur_n/cur_d = (sum_n*cur_d + cur_n*sum_d) / (sum_d*cur_d)
                int64_t new_n = sum_n * cur_d + cur_n * sum_d;
                int64_t new_d = sum_d * cur_d;
                int64_t common = gcd(new_n < 0 ? -new_n : new_n, new_d);
                sum_n = new_n / common;
                sum_d = new_d / common;
            }
            // mean = sum / n = sum_n / (sum_d * n)
            return make_rational(sum_n, sum_d * (int64_t)n);
        }
    }

    // Fallback to symbolic: Mean = (1/n) * Plus @@ data
    Expr* sum_call = expr_new_function(expr_new_symbol("Apply"), (Expr*[]){expr_new_symbol("Plus"), expr_copy(data)}, 2);
    Expr* sum = evaluate(sum_call);
    expr_free(sum_call);

    Expr* n_inv = make_rational(1, (int64_t)n);
    Expr* mean_args[2] = { n_inv, sum };
    Expr* mean_call = expr_new_function(expr_new_symbol("Times"), mean_args, 2);
    Expr* result = evaluate(mean_call);
    expr_free(mean_call);
    return result;
}

Expr* builtin_variance(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* data = res->data.function.args[0];

    // Check if it's a matrix
    Expr* matrixq_args[1] = { expr_copy(data) };
    Expr* matrixq_call = expr_new_function(expr_new_symbol("MatrixQ"), matrixq_args, 1);
    Expr* is_matrix = evaluate(matrixq_call);
    expr_free(matrixq_call);
    if (is_matrix->type == EXPR_SYMBOL && strcmp(is_matrix->data.symbol, "True") == 0) {
        expr_free(is_matrix);
        return apply_columnwise("Variance", data);
    }
    expr_free(is_matrix);

    // Check if it's a vector (list)
    Expr* listq_args[1] = { expr_copy(data) };
    Expr* listq_call = expr_new_function(expr_new_symbol("ListQ"), listq_args, 1);
    Expr* is_list = evaluate(listq_call);
    expr_free(listq_call);
    if (is_list->type == EXPR_SYMBOL && strcmp(is_list->data.symbol, "False") == 0) {
        expr_free(is_list);
        return NULL;
    }
    expr_free(is_list);

    size_t n = data->data.function.arg_count;
    if (n <= 1) return NULL;

    // Check if all elements are purely real numeric
    bool all_numeric = true;
    bool has_real = false;
    for (size_t i = 0; i < n; i++) {
        Expr* elem = data->data.function.args[i];
        if (elem->type == EXPR_REAL) {
            has_real = true;
        } else if (elem->type == EXPR_INTEGER || is_rational(elem, NULL, NULL)) {
            // Numeric
        } else {
            all_numeric = false;
            break;
        }
    }

    if (all_numeric) {
        if (has_real) {
            // Welford's algorithm
            double m = 0;
            double s = 0;
            for (size_t i = 0; i < n; i++) {
                double x = 0.0;
                is_numeric(data->data.function.args[i], &x, NULL);
                double old_m = m;
                m += (x - m) / (double)(i + 1);
                s += (x - m) * (x - old_m);
            }
            return expr_new_real(s / (double)(n - 1));
        } else {
            // Exact calculation for Variance
            // Var = (Sum[x^2] - n*Mean[x]^2) / (n-1)
            // Using Sum[(n*x_i - Sum[x_j])^2] / (n^2 * (n-1))
            int64_t sum_n = 0;
            int64_t sum_d = 1;
            for (size_t i = 0; i < n; i++) {
                Expr* elem = data->data.function.args[i];
                int64_t cur_n, cur_d;
                if (elem->type == EXPR_INTEGER) { cur_n = elem->data.integer; cur_d = 1; }
                else is_rational(elem, &cur_n, &cur_d);
                int64_t new_n = sum_n * cur_d + cur_n * sum_d;
                int64_t new_d = sum_d * cur_d;
                int64_t common = gcd(new_n < 0 ? -new_n : new_n, new_d);
                sum_n = new_n / common;
                sum_d = new_d / common;
            }
            // Now sum_n/sum_d is the sum of elements
            int64_t sq_sum_n = 0;
            int64_t sq_sum_d = 1;
            for (size_t i = 0; i < n; i++) {
                Expr* elem = data->data.function.args[i];
                int64_t cur_n, cur_d;
                if (elem->type == EXPR_INTEGER) { cur_n = elem->data.integer; cur_d = 1; }
                else is_rational(elem, &cur_n, &cur_d);
                
                // (x - mean)^2 = (cur_n/cur_d - sum_n/(n*sum_d))^2 
                // = ( (cur_n * n * sum_d - sum_n * cur_d) / (n * sum_d * cur_d) )^2
                int64_t term_n = cur_n * (int64_t)n * sum_d - sum_n * cur_d;
                int64_t term_d = (int64_t)n * sum_d * cur_d;
                int64_t common = gcd(term_n < 0 ? -term_n : term_n, term_d);
                term_n /= common; term_d /= common;
                
                int64_t term_sq_n = term_n * term_n;
                int64_t term_sq_d = term_d * term_d;
                
                int64_t new_sq_sum_n = sq_sum_n * term_sq_d + term_sq_n * sq_sum_d;
                int64_t new_sq_sum_d = sq_sum_d * term_sq_d;
                common = gcd(new_sq_sum_n < 0 ? -new_sq_sum_n : new_sq_sum_n, new_sq_sum_d);
                sq_sum_n = new_sq_sum_n / common;
                sq_sum_d = new_sq_sum_d / common;
            }
            // Variance = sq_sum / (n-1)
            return make_rational(sq_sum_n, sq_sum_d * ((int64_t)n - 1));
        }
    }

    // Fallback to symbolic: Compute Mean
    Expr* mean_args[1] = { expr_copy(data) };
    Expr* mean_call = expr_new_function(expr_new_symbol("Mean"), mean_args, 1);
    Expr* mu = evaluate(mean_call);
    expr_free(mean_call);

    // Sum[(x - mu) * Conjugate[x - mu]]
    Expr** diffs = malloc(sizeof(Expr*) * n);
    for (size_t i = 0; i < n; i++) {
        Expr* x = data->data.function.args[i];
        Expr* sub_args[2] = { expr_copy(x), expr_new_function(expr_new_symbol("Times"), (Expr*[]){expr_new_integer(-1), expr_copy(mu)}, 2) };
        Expr* diff = eval_and_free(expr_new_function(expr_new_symbol("Plus"), sub_args, 2));
        
        Expr* conj_args[1] = { expr_copy(diff) };
        Expr* conj_call = expr_new_function(expr_new_symbol("Conjugate"), conj_args, 1);
        Expr* conj_diff = evaluate(conj_call);
        expr_free(conj_call);

        Expr* prod_args[2] = { diff, conj_diff };
        diffs[i] = eval_and_free(expr_new_function(expr_new_symbol("Times"), prod_args, 2));
    }
    expr_free(mu);

    Expr* sum_diffs = expr_new_function(expr_new_symbol("Plus"), diffs, n);
    Expr* sum = evaluate(sum_diffs);
    expr_free(sum_diffs);
    free(diffs);

    Expr* n_minus_1_inv = make_rational(1, (int64_t)n - 1);
    Expr* var_call = expr_new_function(expr_new_symbol("Times"), (Expr*[]){ n_minus_1_inv, sum }, 2);
    Expr* result = evaluate(var_call);
    expr_free(var_call);
    return result;
}

Expr* builtin_standard_deviation(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* data = res->data.function.args[0];

    // Check if it's a matrix
    Expr* matrixq_args[1] = { expr_copy(data) };
    Expr* matrixq_call = expr_new_function(expr_new_symbol("MatrixQ"), matrixq_args, 1);
    Expr* is_matrix = evaluate(matrixq_call);
    expr_free(matrixq_call);
    if (is_matrix->type == EXPR_SYMBOL && strcmp(is_matrix->data.symbol, "True") == 0) {
        expr_free(is_matrix);
        return apply_columnwise("StandardDeviation", data);
    }
    expr_free(is_matrix);

    // Optimization for real numeric data
    if (data->type == EXPR_FUNCTION) {
        bool all_real = true;
        size_t n = data->data.function.arg_count;
        for (size_t i = 0; i < n; i++) {
            bool complex;
            if (!is_numeric(data->data.function.args[i], NULL, &complex) || complex) {
                all_real = false;
                break;
            }
        }
        if (all_real && n > 1) {
            Expr* var_call = expr_new_function(expr_new_symbol("Variance"), (Expr*[]){ expr_copy(data) }, 1);
            Expr* var_e = evaluate(var_call);
            expr_free(var_call);
            if (var_e->type == EXPR_REAL) {
                double val = sqrt(var_e->data.real);
                expr_free(var_e);
                return expr_new_real(val);
            }
            expr_free(var_e);
        }
    }

    // StandardDeviation[data] = Variance[data]^(1/2)
    Expr* var_call = expr_new_function(expr_new_symbol("Variance"), (Expr*[]){ expr_copy(data) }, 1);
    Expr* var = evaluate(var_call);
    expr_free(var_call);

    if (!var) return NULL;

    Expr* sqrt_call = expr_new_function(expr_new_symbol("Power"), (Expr*[]){ var, make_rational(1, 2) }, 2);
    Expr* result = evaluate(sqrt_call);
    expr_free(sqrt_call);
    return result;
}



/* ------------------- Median ------------------- */

static bool is_real_numeric(Expr* e) {
    Expr* numq = expr_new_function(expr_new_symbol("NumericQ"), (Expr*[]){expr_copy(e)}, 1);
    Expr* numq_eval = evaluate(numq);
    expr_free(numq);
    if (numq_eval->type != EXPR_SYMBOL || strcmp(numq_eval->data.symbol, "True") != 0) {
        expr_free(numq_eval);
        return false;
    }
    expr_free(numq_eval);
    
    Expr* freeq = expr_new_function(expr_new_symbol("FreeQ"), (Expr*[]){expr_copy(e), expr_new_symbol("I")}, 2);
    Expr* freeq_eval = evaluate(freeq);
    expr_free(freeq);
    if (freeq_eval->type != EXPR_SYMBOL || strcmp(freeq_eval->data.symbol, "True") != 0) {
        expr_free(freeq_eval);
        return false;
    }
    expr_free(freeq_eval);
    
    return true;
}

Expr* builtin_median(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* data = res->data.function.args[0];

    // Check if it's a vector or tensor. If it's empty or not a list, return NULL.
    if (data->type != EXPR_FUNCTION || data->data.function.head->type != EXPR_SYMBOL || strcmp(data->data.function.head->data.symbol, "List") != 0) {
        return expr_copy(res);
    }

    size_t n = data->data.function.arg_count;
    if (n == 0) return expr_copy(res);

    // Check if it's a matrix/tensor by checking if the first element is a List.
    if (data->data.function.args[0]->type == EXPR_FUNCTION && 
        data->data.function.args[0]->data.function.head->type == EXPR_SYMBOL && 
        strcmp(data->data.function.args[0]->data.function.head->data.symbol, "List") == 0) {
        // Apply columnwise via Transpose and Map
        return apply_columnwise("Median", data);
    }

    // Now it's a 1D vector. Check if all elements are real numbers.
    bool all_real = true;
    for (size_t i = 0; i < n; i++) {
        Expr* elem = data->data.function.args[i];
        if (!is_real_numeric(elem)) {
            all_real = false;
            break;
        }
    }

    if (!all_real) {
        char* str = expr_to_string(res);
        printf("Median::rectn: Rectangular array of real numbers is expected at position 1 in %s.\n", str);
        free(str);
        return expr_copy(res);
    }

    // Sort the list
    Expr* sort_expr = expr_new_function(expr_new_symbol("Sort"), (Expr*[]){expr_copy(data)}, 1);
    Expr* sorted = evaluate(sort_expr);
    expr_free(sort_expr);

    if (sorted->type != EXPR_FUNCTION || sorted->data.function.arg_count != n) {
        expr_free(sorted);
        return expr_copy(res);
    }

    Expr* result = NULL;
    if (n % 2 == 1) {
        result = expr_copy(sorted->data.function.args[n / 2]);
    } else {
        Expr* m1 = sorted->data.function.args[n / 2 - 1];
        Expr* m2 = sorted->data.function.args[n / 2];
        Expr* sum = expr_new_function(expr_new_symbol("Plus"), (Expr*[]){expr_copy(m1), expr_copy(m2)}, 2);
        Expr* sum_eval = evaluate(sum);
        expr_free(sum);
        
        Expr* div = expr_new_function(expr_new_symbol("Divide"), (Expr*[]){sum_eval, expr_new_integer(2)}, 2);
        result = evaluate(div);
        expr_free(div);
    }

    expr_free(sorted);
    return result;
}

void stats_init(void) {
    symtab_add_builtin("Mean", builtin_mean);
    symtab_add_builtin("Median", builtin_median);
    symtab_get_def("Median")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("Variance", builtin_variance);
    symtab_add_builtin("StandardDeviation", builtin_standard_deviation);

    symtab_get_def("Mean")->attributes |= ATTR_PROTECTED;
    symtab_get_def("Variance")->attributes |= ATTR_PROTECTED;
    symtab_get_def("StandardDeviation")->attributes |= ATTR_PROTECTED;
}

#include "datetime.h"
#include "eval.h"
#include "symtab.h"
#include "attr.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

Expr* builtin_timing(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) {
        return NULL;
    }
    
    Expr* arg = res->data.function.args[0];
    
    clock_t start = clock();
    Expr* evaluated = evaluate(arg);
    clock_t end = clock();
    
    double time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    
    Expr** results = malloc(sizeof(Expr*) * 2);
    results[0] = expr_new_real(time_used);
    results[1] = evaluated;
    
    Expr* final_res = expr_new_function(expr_new_symbol("List"), results, 2);
    free(results);
    return final_res;
}

static int compare_doubles(const void* a, const void* b) {
    double arg1 = *(const double*)a;
    double arg2 = *(const double*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

Expr* builtin_repeated_timing(Expr* res) {
    if (res->type != EXPR_FUNCTION || (res->data.function.arg_count != 1 && res->data.function.arg_count != 2)) {
        return NULL;
    }
    
    Expr* arg = res->data.function.args[0];
    double t_target = 1.0; // Default 1 second
    
    if (res->data.function.arg_count == 2) {
        Expr* t_expr = res->data.function.args[1];
        if (t_expr->type == EXPR_INTEGER) {
            t_target = (double)t_expr->data.integer;
        } else if (t_expr->type == EXPR_REAL) {
            t_target = t_expr->data.real;
        } else {
            Expr* t_eval = evaluate(t_expr);
            if (t_eval->type == EXPR_INTEGER) {
                t_target = (double)t_eval->data.integer;
            } else if (t_eval->type == EXPR_REAL) {
                t_target = t_eval->data.real;
            } else {
                expr_free(t_eval);
                return NULL;
            }
            expr_free(t_eval);
        }
    }
    
    Expr* first_evaluated = NULL;
    
    size_t cap = 16;
    size_t count = 0;
    double* timings = malloc(sizeof(double) * cap);
    
    clock_t total_start = clock();
    double elapsed_total = 0.0;
    
    // Minimum 4 evaluations, or until t_target seconds is reached
    while (count < 4 || elapsed_total < t_target) {
        clock_t start = clock();
        Expr* evaluated = evaluate(arg);
        clock_t end = clock();
        
        if (count == 0) {
            first_evaluated = evaluated;
        } else {
            expr_free(evaluated);
        }
        
        if (count == cap) {
            cap *= 2;
            timings = realloc(timings, sizeof(double) * cap);
        }
        
        double time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        timings[count++] = time_used;
        
        clock_t current_time = clock();
        elapsed_total = ((double) (current_time - total_start)) / CLOCKS_PER_SEC;
    }
    
    qsort(timings, count, sizeof(double), compare_doubles);
    
    // Trimmed mean (discard lower and upper quartiles)
    size_t q_size = count / 4;
    size_t start_idx = q_size;
    size_t end_idx = count - q_size;
    
    double sum = 0.0;
    for (size_t i = start_idx; i < end_idx; i++) {
        sum += timings[i];
    }
    double average_time = sum / (double)(end_idx - start_idx);
    
    free(timings);
    
    Expr** results = malloc(sizeof(Expr*) * 2);
    results[0] = expr_new_real(average_time);
    results[1] = first_evaluated;
    
    Expr* final_res = expr_new_function(expr_new_symbol("List"), results, 2);
    free(results);
    return final_res;
}

void datetime_init(void) {
    symtab_add_builtin("Timing", builtin_timing);
    symtab_add_builtin("RepeatedTiming", builtin_repeated_timing);
}
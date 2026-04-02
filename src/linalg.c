#include "linalg.h"
#include "eval.h"
#include "symtab.h"
#include "attr.h"
#include "print.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int get_tensor_dims(Expr* e, int64_t* dims) {
    if (e->type != EXPR_FUNCTION || e->data.function.head->type != EXPR_SYMBOL || strcmp(e->data.function.head->data.symbol, "List") != 0) {
        return 0; // rank 0
    }
    int64_t len = e->data.function.arg_count;
    dims[0] = len;
    if (len == 0) return 1;

    int sub_rank = get_tensor_dims(e->data.function.args[0], dims + 1);
    for (int64_t i = 1; i < len; i++) {
        int64_t cur_dims[64];
        int cur_rank = get_tensor_dims(e->data.function.args[i], cur_dims);
        if (cur_rank != sub_rank) return -1; // jagged
        for (int j = 0; j < sub_rank; j++) {
            if (cur_dims[j] != dims[j + 1]) return -1; // jagged
        }
    }
    return sub_rank + 1;
}

static void flatten_tensor(Expr* e, Expr** flat, size_t* idx) {
    if (e->type == EXPR_FUNCTION && e->data.function.head->type == EXPR_SYMBOL && strcmp(e->data.function.head->data.symbol, "List") == 0) {
        for (size_t i = 0; i < e->data.function.arg_count; i++) {
            flatten_tensor(e->data.function.args[i], flat, idx);
        }
    } else {
        flat[(*idx)++] = expr_copy(e);
    }
}

static Expr* build_tensor(int64_t* dims, int rank, Expr** flat, size_t* idx) {
    if (rank == 0) {
        return expr_copy(flat[(*idx)++]);
    }
    int64_t len = dims[0];
    Expr** args = NULL;
    if (len > 0) args = malloc(sizeof(Expr*) * len);
    for (int64_t i = 0; i < len; i++) {
        args[i] = build_tensor(dims + 1, rank - 1, flat, idx);
    }
    Expr* res = expr_new_function(expr_new_symbol("List"), args, len);
    if (args) free(args);
    return res;
}

static Expr* dot2(Expr* a, Expr* b, bool* error_printed) {
    int64_t dimsA[64];
    int rankA = get_tensor_dims(a, dimsA);
    if (rankA <= 0) return NULL; // Not a tensor, or jagged

    int64_t dimsB[64];
    int rankB = get_tensor_dims(b, dimsB);
    if (rankB <= 0) return NULL;

    int64_t K = dimsA[rankA - 1];
    if (K != dimsB[0]) {
        if (!*error_printed) {
            char* a_str = expr_to_string_fullform(a);
            char* b_str = expr_to_string_fullform(b);
            fprintf(stderr, "Dot::dotsh: Tensors %s and %s have incompatible shapes.\n", a_str, b_str);
            free(a_str);
            free(b_str);
            *error_printed = true;
        }
        return NULL;
    }

    int64_t N_A = 1; for(int i=0; i<rankA; i++) N_A *= dimsA[i];
    int64_t N_B = 1; for(int i=0; i<rankB; i++) N_B *= dimsB[i];

    Expr** flatA = NULL; if (N_A > 0) flatA = malloc(sizeof(Expr*) * N_A);
    Expr** flatB = NULL; if (N_B > 0) flatB = malloc(sizeof(Expr*) * N_B);
    size_t idxA = 0; if (N_A > 0) flatten_tensor(a, flatA, &idxA);
    size_t idxB = 0; if (N_B > 0) flatten_tensor(b, flatB, &idxB);

    int64_t R = K == 0 ? N_A : N_A / K;
    int64_t S = K == 0 ? N_B : N_B / K;

    Expr** flatC = NULL; 
    if (R * S > 0) flatC = malloc(sizeof(Expr*) * (R * S));

    for (int64_t r = 0; r < R; r++) {
        for (int64_t s = 0; s < S; s++) {
            Expr** sum_args = NULL;
            if (K > 0) sum_args = malloc(sizeof(Expr*) * K);
            for (int64_t k = 0; k < K; k++) {
                Expr* a_elem = flatA[r * K + k];
                Expr* b_elem = flatB[k * S + s];
                Expr* t_args[2] = { expr_copy(a_elem), expr_copy(b_elem) };
                sum_args[k] = eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 2));
            }
            if (K == 0) {
                flatC[r * S + s] = expr_new_integer(0);
            } else if (K == 1) {
                flatC[r * S + s] = sum_args[0];
            } else {
                flatC[r * S + s] = eval_and_free(expr_new_function(expr_new_symbol("Plus"), sum_args, K));
            }
            if (sum_args) free(sum_args);
        }
    }

    if (flatA) { for(size_t i=0; i<idxA; i++) expr_free(flatA[i]); free(flatA); }
    if (flatB) { for(size_t i=0; i<idxB; i++) expr_free(flatB[i]); free(flatB); }

    int64_t dimsC[64];
    int rankC = rankA + rankB - 2;
    for (int i = 0; i < rankA - 1; i++) dimsC[i] = dimsA[i];
    for (int i = 0; i < rankB - 1; i++) dimsC[rankA - 1 + i] = dimsB[i + 1];

    size_t c_idx = 0;
    Expr* result = build_tensor(dimsC, rankC, flatC, &c_idx);
    if (flatC) {
        for (int64_t i = 0; i < R * S; i++) expr_free(flatC[i]);
        free(flatC);
    }

    return result;
}

Expr* builtin_dot(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t count = res->data.function.arg_count;
    if (count == 0) return NULL; 
    if (count == 1) return expr_copy(res->data.function.args[0]);

    Expr** new_args = malloc(sizeof(Expr*) * count);
    for (size_t i=0; i<count; i++) new_args[i] = expr_copy(res->data.function.args[i]);
    size_t new_count = count;

    bool changed = false;
    bool error_printed = false;
    for (size_t i = 0; i < new_count - 1; i++) {
        Expr* a = new_args[i];
        Expr* b = new_args[i+1];
        
        int64_t dA[64], dB[64];
        if (get_tensor_dims(a, dA) > 0 && get_tensor_dims(b, dB) > 0) {
            Expr* d = dot2(a, b, &error_printed);
            if (d) {
                expr_free(a);
                expr_free(b);
                new_args[i] = d;
                for (size_t j = i + 2; j < new_count; j++) {
                    new_args[j - 1] = new_args[j];
                }
                new_count--;
                changed = true;
                i--; // re-check the new element at index i with i+1 (which shifted)
                if (i == (size_t)-1) i = 0; // In case we backed up past 0 (wait, i-- makes i=2^64-1, then i++ in loop makes it 0, correct)
            } else if (error_printed) {
                for (size_t j=0; j<new_count; j++) expr_free(new_args[j]);
                free(new_args);
                return NULL;
            }
        }
    }

    if (!changed) {
        for (size_t j=0; j<new_count; j++) expr_free(new_args[j]);
        free(new_args);
        return NULL;
    }

    Expr* final_res;
    if (new_count == 1) {
        final_res = new_args[0];
    } else {
        final_res = expr_new_function(expr_new_symbol("Dot"), new_args, new_count);
    }
    
    free(new_args);
    return final_res;
}

static Expr* laplace_det(Expr** flat, int original_n, int n, int row, int* cols) {
    if (n == 1) {
        return expr_copy(flat[row * original_n + cols[0]]);
    }
    Expr** sum_args = malloc(sizeof(Expr*) * n);
    for (int i = 0; i < n; i++) {
        int* new_cols = malloc(sizeof(int) * (n - 1));
        for (int j = 0, k = 0; j < n; j++) {
            if (j != i) new_cols[k++] = cols[j];
        }
        Expr* minor_det = laplace_det(flat, original_n, n - 1, row + 1, new_cols);
        free(new_cols);
        Expr* elem = flat[row * original_n + cols[i]];
        if (i % 2 != 0) {
            Expr* t_args[3] = { expr_new_integer(-1), expr_copy(elem), minor_det };
            sum_args[i] = eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 3));
        } else {
            Expr* t_args[2] = { expr_copy(elem), minor_det };
            sum_args[i] = eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 2));
        }
    }
    Expr* res = eval_and_free(expr_new_function(expr_new_symbol("Plus"), sum_args, n));
    free(sum_args);
    return res;
}

Expr* builtin_det(Expr* res) {
    if (res->type != EXPR_FUNCTION || res->data.function.arg_count != 1) return NULL;
    Expr* arg = res->data.function.args[0];

    int64_t dims[64];
    int rank = get_tensor_dims(arg, dims);
    
    if (rank != 2 || dims[0] != dims[1] || dims[0] == 0) {
        char* arg_str = expr_to_string_fullform(arg);
        fprintf(stderr, "Det::matsq: Argument %s at position 1 is not a non-empty square matrix.\n", arg_str);
        free(arg_str);
        return NULL;
    }
    
    int n = (int)dims[0];
    Expr** flat = malloc(sizeof(Expr*) * n * n);
    size_t idx = 0;
    flatten_tensor(arg, flat, &idx);
    
    int* cols = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) cols[i] = i;
    
    Expr* det_val = laplace_det(flat, n, n, 0, cols);
    
    free(cols);
    for (size_t i = 0; i < idx; i++) expr_free(flat[i]);
    free(flat);
    
    return det_val;
}

Expr* builtin_cross(Expr* res) {
    if (res->type != EXPR_FUNCTION) return NULL;
    size_t m = res->data.function.arg_count;
    if (m == 0) return NULL;

    size_t n = m + 1;
    bool valid = true;
    for (size_t i = 0; i < m; i++) {
        Expr* arg = res->data.function.args[i];
        if (arg->type != EXPR_FUNCTION || arg->data.function.head->type != EXPR_SYMBOL || strcmp(arg->data.function.head->data.symbol, "List") != 0) {
            valid = false;
            break;
        }
        if (arg->data.function.arg_count != n) {
            valid = false;
            break;
        }
    }

    if (!valid) {
        fprintf(stderr, "Cross::nonn1: The arguments are expected to be vectors of equal length, and the number of arguments is expected to be 1 less than their length.\n");
        return NULL;
    }

    Expr** result_args = malloc(sizeof(Expr*) * n);

    for (size_t i = 0; i < n; i++) {
        Expr** minor_flat = malloc(sizeof(Expr*) * m * m);
        for (size_t r = 0; r < m; r++) {
            Expr* vec = res->data.function.args[r];
            size_t c_idx = 0;
            for (size_t c = 0; c < n; c++) {
                if (c == i) continue;
                minor_flat[r * m + c_idx] = vec->data.function.args[c];
                c_idx++;
            }
        }

        int* cols = malloc(sizeof(int) * m);
        for (size_t c = 0; c < m; c++) cols[c] = (int)c;

        Expr* det_val = laplace_det(minor_flat, (int)m, (int)m, 0, cols);
        free(cols);
        free(minor_flat);

        int sign = ((m + i) % 2 == 0) ? 1 : -1;
        if (sign == -1) {
            Expr* t_args[2] = { expr_new_integer(-1), det_val };
            result_args[i] = eval_and_free(expr_new_function(expr_new_symbol("Times"), t_args, 2));
        } else {
            result_args[i] = det_val;
        }
    }

    Expr* final_res = expr_new_function(expr_new_symbol("List"), result_args, n);
    free(result_args);
    return final_res;
}

Expr* builtin_norm(Expr* res) {
    if (res->type != EXPR_FUNCTION || (res->data.function.arg_count != 1 && res->data.function.arg_count != 2)) return NULL;
    
    Expr* expr = res->data.function.args[0];
    Expr* p = NULL;
    if (res->data.function.arg_count == 2) {
        p = res->data.function.args[1];
    }
    
    int64_t dims[64];
    int rank = get_tensor_dims(expr, dims);
    if (rank < 0) return NULL; // jagged array
    
    if (rank == 0) {
        // Scalar: Norm[x] -> Abs[x]
        if (!p) {
            Expr* args[1] = { expr_copy(expr) };
            return eval_and_free(expr_new_function(expr_new_symbol("Abs"), args, 1));
        }
        return NULL;
    }
    
    if (rank == 1 || (rank >= 2 && p && p->type == EXPR_STRING && strcmp(p->data.string, "Frobenius") == 0)) {
        int64_t N = 1;
        for (int i = 0; i < rank; i++) N *= dims[i];
        
        Expr** flat = NULL;
        if (N > 0) {
            flat = malloc(sizeof(Expr*) * N);
            size_t idx = 0;
            flatten_tensor(expr, flat, &idx);
        }
        
        Expr* result = NULL;
        
        if (p && p->type == EXPR_SYMBOL && strcmp(p->data.symbol, "Infinity") == 0) {
            if (N == 0) {
                result = expr_new_integer(0);
            } else {
                Expr** max_args = malloc(sizeof(Expr*) * N);
                for (int64_t i = 0; i < N; i++) {
                    max_args[i] = eval_and_free(expr_new_function(expr_new_symbol("Abs"), (Expr*[]){expr_copy(flat[i])}, 1));
                }
                result = eval_and_free(expr_new_function(expr_new_symbol("Max"), max_args, N));
                free(max_args);
            }
        } else {
            Expr* norm_p = NULL;
            if (!p || (p->type == EXPR_STRING && strcmp(p->data.string, "Frobenius") == 0)) {
                norm_p = expr_new_integer(2);
            } else {
                norm_p = expr_copy(p);
            }
            
            if (N == 0) {
                result = expr_new_integer(0);
            } else {
                Expr** plus_args = malloc(sizeof(Expr*) * N);
                for (int64_t i = 0; i < N; i++) {
                    Expr* abs_val = eval_and_free(expr_new_function(expr_new_symbol("Abs"), (Expr*[]){expr_copy(flat[i])}, 1));
                    plus_args[i] = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){abs_val, expr_copy(norm_p)}, 2));
                }
                Expr* sum = NULL;
                if (N == 1) {
                    sum = plus_args[0];
                } else {
                    sum = eval_and_free(expr_new_function(expr_new_symbol("Plus"), plus_args, N));
                }
                free(plus_args);
                
                Expr* inv_p = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){expr_copy(norm_p), expr_new_integer(-1)}, 2));
                result = eval_and_free(expr_new_function(expr_new_symbol("Power"), (Expr*[]){sum, inv_p}, 2));
            }
            expr_free(norm_p);
        }
        
        if (flat) {
            for (int64_t i = 0; i < N; i++) expr_free(flat[i]);
            free(flat);
        }
        return result;
    }
    
    // Fallback for unhandled matrix norm (e.g. SVD max singular value)
    return NULL;
}

void linalg_init(void) {
    symtab_add_builtin("Dot", builtin_dot);
    symtab_get_def("Dot")->attributes |= ATTR_FLAT | ATTR_ONEIDENTITY | ATTR_PROTECTED;
    symtab_add_builtin("Det", builtin_det);
    symtab_get_def("Det")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("Cross", builtin_cross);
    symtab_get_def("Cross")->attributes |= ATTR_PROTECTED;
    symtab_add_builtin("Norm", builtin_norm);
    symtab_get_def("Norm")->attributes |= ATTR_PROTECTED;
}

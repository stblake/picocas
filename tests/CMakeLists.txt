
cmake_minimum_required(VERSION 3.10)
project(picocas_tests)

set(COMMON_SRC
    ../src/expr.c
    ../src/parse.c
    ../src/eval.c
    ../src/internal.c
    ../src/attr.c
    ../src/core.c
    ../src/comparisons.c
    ../src/symtab.c
    ../src/match.c
    ../src/part.c
    ../src/print.c
    ../src/plus.c
    ../src/times.c
    ../src/arithmetic.c
    ../src/facint.c
    ../src/power.c
    ../src/funcprog.c
    ../src/boolean.c
    ../src/list.c
    ../src/replace.c
    ../src/complex.c
    ../src/trig.c
    ../src/hyperbolic.c
    ../src/logexp.c
    ../src/piecewise.c
    ../src/purefunc.c
    ../src/modular.c
    ../src/sort.c
    ../src/stats.c
    ../src/info.c
    ../src/expand.c
    ../src/poly.c
    ../src/rat.c
    ../src/facpoly.c
    ../src/parfrac.c
    ../src/datetime.c
    ../src/linalg.c
    ../src/load.c
    ../src/default_helper.c
)

add_executable(expr_tests
    test_expr.c
    test_part.c
    ${COMMON_SRC}
)
target_include_directories(expr_tests PRIVATE ../src)

add_executable(parse_tests
    test_parse.c
    ${COMMON_SRC}
)
target_include_directories(parse_tests PRIVATE ../src)

add_executable(eval_tests
    test_eval.c
    ${COMMON_SRC}
)
target_include_directories(eval_tests PRIVATE ../src)

add_executable(match_tests
    test_match.c
    ${COMMON_SRC}
)
target_include_directories(match_tests PRIVATE ../src)

add_executable(match_extensive_tests
    test_match_extensive.c
    ${COMMON_SRC}
)
target_include_directories(match_extensive_tests PRIVATE ../src)

add_executable(symtab_tests
    test_symtab.c
    ${COMMON_SRC}
)
target_include_directories(symtab_tests PRIVATE ../src)

add_executable(regression_tests
    test_regression.c
    ${COMMON_SRC}
)
target_include_directories(regression_tests PRIVATE ../src)

add_executable(comparisons_tests
    test_comparisons.c
    ${COMMON_SRC}
)
target_include_directories(comparisons_tests PRIVATE ../src)

add_executable(list_tests
    test_list.c
    ${COMMON_SRC}
)
target_include_directories(list_tests PRIVATE ../src)

add_executable(replace_tests
    test_replace.c
    ${COMMON_SRC}
)
target_include_directories(replace_tests PRIVATE ../src)

add_executable(core_tests
    test_core.c
    ${COMMON_SRC}
)
target_include_directories(core_tests PRIVATE ../src)

add_executable(trig_tests
    test_trig.c
    ${COMMON_SRC}
)
target_include_directories(trig_tests PRIVATE ../src)

add_executable(hyperbolic_tests
    test_hyperbolic.c
    ${COMMON_SRC}
)
target_include_directories(hyperbolic_tests PRIVATE ../src)

add_executable(logexp_tests
    test_logexp.c
    ${COMMON_SRC}
)
target_include_directories(logexp_tests PRIVATE ../src)

add_executable(piecewise_tests
    test_piecewise.c
    ${COMMON_SRC}
)
target_include_directories(piecewise_tests PRIVATE ../src)

add_executable(purefunc_tests
    test_purefunc.c
    ${COMMON_SRC}
)
target_include_directories(purefunc_tests PRIVATE ../src)

add_executable(modular_tests
    test_modular.c
    ${COMMON_SRC}
)
target_include_directories(modular_tests PRIVATE ../src)

add_executable(funcprog_through_tests
    test_funcprog_through.c
    ${COMMON_SRC}
)
target_include_directories(funcprog_through_tests PRIVATE ../src)

add_executable(split_tests
    test_split.c
    ${COMMON_SRC}
)
target_include_directories(split_tests PRIVATE ../src)

add_executable(sort_tests
    test_sort.c
    ${COMMON_SRC}
)
target_include_directories(sort_tests PRIVATE ../src)

add_executable(backtrack_tests test_backtrack.c ${COMMON_SRC})
target_include_directories(backtrack_tests PRIVATE ../src)
add_executable(poly_tests
    test_poly.c
    ${COMMON_SRC}
)
target_include_directories(poly_tests PRIVATE ../src)

add_executable(expand_tests
    test_expand.c
    ${COMMON_SRC}
)
target_include_directories(expand_tests PRIVATE ../src)

add_executable(stats_tests
    test_stats.c
    ${COMMON_SRC}
)
target_include_directories(stats_tests PRIVATE ../src)

add_executable(datetime_tests
    test_datetime.c
    ${COMMON_SRC}
)
target_include_directories(datetime_tests PRIVATE ../src)

add_executable(facpoly_tests
    test_facpoly.c
    ${COMMON_SRC}
)
target_include_directories(facpoly_tests PRIVATE ../src)

add_executable(rat_tests
    test_rat.c
    ${COMMON_SRC}
)
target_include_directories(rat_tests PRIVATE ../src)

add_executable(linalg_tests
    test_linalg.c
    ${COMMON_SRC}
)
target_include_directories(linalg_tests PRIVATE ../src)


add_executable(parfrac_tests
    test_parfrac.c
    ${COMMON_SRC}
)
target_include_directories(parfrac_tests PRIVATE ../src)
add_executable(polymod_tests test_polymod.c ${COMMON_SRC})
target_include_directories(polymod_tests PRIVATE ../src)

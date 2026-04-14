cmake_minimum_required(VERSION 3.10)
project(picocas_tests)

option(USE_ECM "Use ECM library for factorisation" ON)

include_directories(/usr/local/include)
if(USE_ECM)
    include_directories(../src/external/ecm)
endif()

if(EXISTS "/opt/homebrew/include")
    include_directories(/opt/homebrew/include)
endif()
link_directories(/usr/local/lib)
if(EXISTS "/opt/homebrew/lib")
    link_directories(/opt/homebrew/lib)
endif()

if(USE_ECM)
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/../src/external/ecm/.libs/libecm.a
        COMMAND ./build_ecm.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
        COMMENT "Building ECM library"
    )
    add_custom_target(build_ecm DEPENDS ${CMAKE_SOURCE_DIR}/../src/external/ecm/.libs/libecm.a)
    link_libraries(gmp m ${CMAKE_SOURCE_DIR}/../src/external/ecm/.libs/libecm.a)
else()
    add_compile_definitions(NO_ECM)
    link_libraries(gmp m)
endif()

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
    ../src/patterns.c
    ../src/patterns.c
    ../src/replace.c
    ${CMAKE_SOURCE_DIR}/../src/simp.c
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
    ../src/iter.c
    ../src/cond.c
    ../src/default_helper.c
    ../src/random.c
)

add_executable(expr_tests test_expr.c test_part.c ${COMMON_SRC})
target_include_directories(expr_tests PRIVATE ../src)

add_executable(parse_tests test_parse.c ${COMMON_SRC})
target_include_directories(parse_tests PRIVATE ../src)

add_executable(eval_tests test_eval.c ${COMMON_SRC})
target_include_directories(eval_tests PRIVATE ../src)

add_executable(match_tests test_match.c ${COMMON_SRC})
target_include_directories(match_tests PRIVATE ../src)

add_executable(match_extensive_tests test_match_extensive.c ${COMMON_SRC})
target_include_directories(match_extensive_tests PRIVATE ../src)

add_executable(symtab_tests test_symtab.c ${COMMON_SRC})
target_include_directories(symtab_tests PRIVATE ../src)

add_executable(regression_tests test_regression.c ${COMMON_SRC})
target_include_directories(regression_tests PRIVATE ../src)

add_executable(comparisons_tests test_comparisons.c ${COMMON_SRC})
target_include_directories(comparisons_tests PRIVATE ../src)

add_executable(list_tests test_list.c ${COMMON_SRC})
target_include_directories(list_tests PRIVATE ../src)

add_executable(iter_tests test_iter.c ${COMMON_SRC})
target_link_libraries(iter_tests m)
target_include_directories(iter_tests PRIVATE ../src)

add_executable(cond_tests test_cond.c ${COMMON_SRC})
target_link_libraries(cond_tests m)
target_include_directories(cond_tests PRIVATE ../src)

add_executable(patterns_tests test_patterns.c ${COMMON_SRC})
target_link_libraries(patterns_tests m)
target_include_directories(patterns_tests PRIVATE ../src)

add_executable(replace_tests test_replace.c
    ${CMAKE_SOURCE_DIR}/../src/simp.c ${COMMON_SRC})
target_include_directories(replace_tests PRIVATE ../src)

add_executable(core_tests test_core.c ${COMMON_SRC})
target_include_directories(core_tests PRIVATE ../src)

add_executable(trig_tests test_trig.c ${COMMON_SRC})
target_include_directories(trig_tests PRIVATE ../src)

add_executable(hyperbolic_tests test_hyperbolic.c ${COMMON_SRC})
target_include_directories(hyperbolic_tests PRIVATE ../src)

add_executable(logexp_tests test_logexp.c ${COMMON_SRC})
target_include_directories(logexp_tests PRIVATE ../src)

add_executable(piecewise_tests test_piecewise.c ${COMMON_SRC})
target_include_directories(piecewise_tests PRIVATE ../src)

add_executable(purefunc_tests test_purefunc.c ${COMMON_SRC})
target_include_directories(purefunc_tests PRIVATE ../src)

add_executable(modular_tests test_modular.c ${COMMON_SRC})
target_include_directories(modular_tests PRIVATE ../src)

add_executable(funcprog_through_tests test_funcprog_through.c ${COMMON_SRC})
target_include_directories(funcprog_through_tests PRIVATE ../src)

add_executable(split_tests test_split.c ${COMMON_SRC})
target_include_directories(split_tests PRIVATE ../src)

add_executable(sort_tests test_sort.c ${COMMON_SRC})
target_include_directories(sort_tests PRIVATE ../src)

add_executable(backtrack_tests test_backtrack.c ${COMMON_SRC})
target_include_directories(backtrack_tests PRIVATE ../src)

add_executable(poly_tests test_poly.c ${COMMON_SRC})
target_include_directories(poly_tests PRIVATE ../src)

add_executable(expand_tests test_expand.c ${COMMON_SRC})
target_include_directories(expand_tests PRIVATE ../src)

add_executable(stats_tests test_stats.c ${COMMON_SRC})
target_include_directories(stats_tests PRIVATE ../src)

add_executable(datetime_tests test_datetime.c ${COMMON_SRC})
target_include_directories(datetime_tests PRIVATE ../src)

add_executable(facpoly_tests test_facpoly.c ${COMMON_SRC})
target_include_directories(facpoly_tests PRIVATE ../src)

add_executable(rat_tests test_rat.c ${COMMON_SRC})
target_include_directories(rat_tests PRIVATE ../src)

add_executable(linalg_tests test_linalg.c ${COMMON_SRC})
target_include_directories(linalg_tests PRIVATE ../src)

add_executable(parfrac_tests test_parfrac.c ${COMMON_SRC})
target_include_directories(parfrac_tests PRIVATE ../src)

add_executable(polymod_tests test_polymod.c ${COMMON_SRC})
target_include_directories(polymod_tests PRIVATE ../src)

add_executable(bigint_tests test_bigint.c ${COMMON_SRC})
target_include_directories(bigint_tests PRIVATE ../src /usr/local/include)
target_link_directories(bigint_tests PRIVATE /usr/local/lib)
if(USE_ECM)
    target_link_libraries(bigint_tests gmp m ${CMAKE_SOURCE_DIR}/../src/external/ecm/.libs/libecm.a)
else()
    target_link_libraries(bigint_tests gmp m)
endif()

add_executable(integrals_tests test_integrals.c ${COMMON_SRC})
target_include_directories(integrals_tests PRIVATE ../src)
if(USE_ECM)
    target_link_libraries(integrals_tests gmp m ${CMAKE_SOURCE_DIR}/../src/external/ecm/.libs/libecm.a)
else()
    target_link_libraries(integrals_tests gmp m)
endif()

add_executable(print_tests test_print.c ${COMMON_SRC})
target_include_directories(print_tests PRIVATE ../src)

add_executable(random_tests test_random.c ${COMMON_SRC})
target_include_directories(random_tests PRIVATE ../src)

add_executable(evaluate_tests test_evaluate.c ${COMMON_SRC})
target_include_directories(evaluate_tests PRIVATE ../src)

add_executable(releasehold_tests test_releasehold.c ${COMMON_SRC})
target_include_directories(releasehold_tests PRIVATE ../src)

if(USE_ECM)
    add_dependencies(expr_tests build_ecm)
    add_dependencies(parse_tests build_ecm)
    add_dependencies(eval_tests build_ecm)
    add_dependencies(match_tests build_ecm)
    add_dependencies(match_extensive_tests build_ecm)
    add_dependencies(symtab_tests build_ecm)
    add_dependencies(regression_tests build_ecm)
    add_dependencies(comparisons_tests build_ecm)
    add_dependencies(list_tests build_ecm)
    add_dependencies(iter_tests build_ecm)
    add_dependencies(cond_tests build_ecm)
    add_dependencies(patterns_tests build_ecm)
    add_dependencies(replace_tests build_ecm)
    add_dependencies(core_tests build_ecm)
    add_dependencies(trig_tests build_ecm)
    add_dependencies(hyperbolic_tests build_ecm)
    add_dependencies(logexp_tests build_ecm)
    add_dependencies(piecewise_tests build_ecm)
    add_dependencies(purefunc_tests build_ecm)
    add_dependencies(modular_tests build_ecm)
    add_dependencies(funcprog_through_tests build_ecm)
    add_dependencies(split_tests build_ecm)
    add_dependencies(sort_tests build_ecm)
    add_dependencies(backtrack_tests build_ecm)
    add_dependencies(poly_tests build_ecm)
    add_dependencies(expand_tests build_ecm)
    add_dependencies(stats_tests build_ecm)
    add_dependencies(datetime_tests build_ecm)
    add_dependencies(facpoly_tests build_ecm)
    add_dependencies(rat_tests build_ecm)
    add_dependencies(linalg_tests build_ecm)
    add_dependencies(parfrac_tests build_ecm)
    add_dependencies(polymod_tests build_ecm)
    add_dependencies(bigint_tests build_ecm)
    add_dependencies(integrals_tests build_ecm)
    add_dependencies(print_tests build_ecm)
    add_dependencies(random_tests build_ecm)
    add_dependencies(evaluate_tests build_ecm)
    add_dependencies(releasehold_tests build_ecm)
endif()

add_executable(simp_tests test_simp.c ${COMMON_SRC})
target_include_directories(simp_tests PRIVATE ../src)
if(USE_ECM)
    target_link_libraries(simp_tests gmp m ${CMAKE_SOURCE_DIR}/../src/external/ecm/.libs/libecm.a)
else()
    target_link_libraries(simp_tests gmp m)
endif()
add_test(NAME simp_tests COMMAND simp_tests)

add_executable(distribute_tests test_distribute.c ${COMMON_SRC})
target_include_directories(distribute_tests PRIVATE ../src)
if(USE_ECM)
    target_link_libraries(distribute_tests gmp m ${CMAKE_SOURCE_DIR}/../src/external/ecm/.libs/libecm.a)
else()
    target_link_libraries(distribute_tests gmp m)
endif()
add_test(NAME distribute_tests COMMAND distribute_tests)

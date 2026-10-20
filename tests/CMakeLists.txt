function(add_interpreter_test name workdir expected)
    cmake_parse_arguments(ARG "EXPECT_FAILURE" "" "INPUTS" ${ARGN})
    add_test(NAME ${name}
        COMMAND ${CMAKE_COMMAND}
            -DEXE=$<TARGET_FILE:mu-interp>
            -DWORKDIR=${workdir}
            "-DINPUTS=${ARG_INPUTS}"
            -DEXPECTED=${CMAKE_CURRENT_SOURCE_DIR}/${expected}
            -DEXPECT_FAILURE=${ARG_EXPECT_FAILURE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/run_test.cmake)
endfunction()

set(tests ${CMAKE_CURRENT_SOURCE_DIR})
set(examples ${PROJECT_SOURCE_DIR}/examples)

add_interpreter_test(legacy_test ${tests} legacy_test.expected INPUTS legacy_test.pr)
add_interpreter_test(legacy_muetest ${tests} legacy_muetest.expected INPUTS legacy_muetest.pr)
add_interpreter_test(regression ${tests} regression.expected INPUTS regression.pr)
add_interpreter_test(errors ${tests} errors.expected INPUTS errors.pr EXPECT_FAILURE)

add_interpreter_test(example_basics ${examples} basics.expected INPUTS basics.pr)
# Run from the project root, so the imports must resolve relative to the
# importing file, not the working directory.
add_interpreter_test(example_arithmetic ${PROJECT_SOURCE_DIR} arithmetic.expected
    INPUTS examples/arithmetic.pr)
add_interpreter_test(example_minimization ${PROJECT_SOURCE_DIR} minimization.expected
    INPUTS examples/minimization.pr)
add_interpreter_test(example_pairing ${PROJECT_SOURCE_DIR} pairing.expected
    INPUTS examples/pairing.pr)
# Old style still works: the prelude is loaded once, the import is skipped.
add_interpreter_test(example_explicit_prelude ${PROJECT_SOURCE_DIR} arithmetic.expected
    INPUTS examples/prelude.pr examples/arithmetic.pr)
add_interpreter_test(imports ${tests} imports.expected INPUTS imports/main.pr EXPECT_FAILURE)

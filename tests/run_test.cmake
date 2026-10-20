# Runs the interpreter on INPUTS (a ';'-separated list of files relative to
# WORKDIR) and compares stdout+stderr with the file EXPECTED.
#   cmake -DEXE=... -DWORKDIR=... -DINPUTS=... -DEXPECTED=... [-DEXPECT_FAILURE=ON] -P run_test.cmake

execute_process(
    COMMAND ${EXE} ${INPUTS}
    WORKING_DIRECTORY ${WORKDIR}
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output
    RESULT_VARIABLE status
)

if(EXPECT_FAILURE AND status EQUAL 0)
    message(FATAL_ERROR "expected a non-zero exit status")
elseif(NOT EXPECT_FAILURE AND NOT status EQUAL 0)
    message(FATAL_ERROR "exit status ${status}\n${output}")
endif()

file(READ ${EXPECTED} expected)
if(NOT output STREQUAL expected)
    message(FATAL_ERROR "output differs from ${EXPECTED}\n--- expected\n${expected}\n--- actual\n${output}")
endif()

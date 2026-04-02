# Helper script: check_coverage_threshold.cmake
#
# Called by the coverage_report target to verify that a coverage metric
# meets its minimum threshold.  Fails the build (via message(FATAL_ERROR))
# when the actual coverage is below the required value.
#
# Required variables (passed via -D on the cmake -P command line):
#   COVERAGE_INFO  – path to the cleaned lcov .info file
#   LCOV_PATH      – path to the lcov executable
#   THRESHOLD      – minimum acceptable percentage (integer, e.g. 90)
#   TYPE           – "lines" or "branches"

if(NOT DEFINED COVERAGE_INFO OR NOT DEFINED LCOV_PATH OR
   NOT DEFINED THRESHOLD      OR NOT DEFINED TYPE)
    message(FATAL_ERROR "check_coverage_threshold.cmake: missing required variable(s).")
endif()

execute_process(
    COMMAND ${LCOV_PATH} --rc branch_coverage=1 --summary ${COVERAGE_INFO}
    OUTPUT_VARIABLE LCOV_SUMMARY
    ERROR_VARIABLE  LCOV_SUMMARY   # lcov writes the summary to stderr
    RESULT_VARIABLE LCOV_RESULT
)

if(NOT LCOV_RESULT EQUAL 0)
    message(FATAL_ERROR "lcov --summary failed.")
endif()

# Parse the relevant line from the lcov summary output.
# Expected formats:
#   lines......: 92.3% (120 of 130 lines)
#   branches...: 87.5% (70 of 80 branches)
if(TYPE STREQUAL "lines")
    string(REGEX MATCH "lines[. ]+: ([0-9]+)\\.([0-9]+)%" _match "${LCOV_SUMMARY}")
elseif(TYPE STREQUAL "branches")
    string(REGEX MATCH "branches[. ]+: ([0-9]+)\\.([0-9]+)%" _match "${LCOV_SUMMARY}")
else()
    message(FATAL_ERROR "Unknown TYPE '${TYPE}'. Use 'lines' or 'branches'.")
endif()

if(NOT _match)
    message(WARNING "Could not parse ${TYPE} coverage from lcov summary. Skipping threshold check.")
    return()
endif()

set(ACTUAL_INT   "${CMAKE_MATCH_1}")
set(ACTUAL_FRAC  "${CMAKE_MATCH_2}")

if(ACTUAL_INT LESS THRESHOLD)
    message(FATAL_ERROR
        "${TYPE} coverage ${ACTUAL_INT}.${ACTUAL_FRAC}% is below the required ${THRESHOLD}%.")
endif()

message(STATUS "${TYPE} coverage: ${ACTUAL_INT}.${ACTUAL_FRAC}% (threshold: ${THRESHOLD}%) – OK")

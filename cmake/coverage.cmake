# Coverage helpers for stream_chunk
#
# Usage:
#   cmake -DENABLE_COVERAGE=ON ..
#   cmake --build .
#   ctest
#   cmake --build . --target coverage_report
#
# Enforces: 90% line coverage and 85% branch coverage.

option(ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

function(enable_coverage target)
    if(NOT ENABLE_COVERAGE)
        return()
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(WARNING "Coverage is only supported with GCC or Clang.")
        return()
    endif()

    # INTERFACE libraries have no compiled sources; skip silently.
    get_target_property(_target_type ${target} TYPE)
    if(_target_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    target_compile_options(${target} PRIVATE --coverage -O0 -g)
    target_link_options(${target} PRIVATE --coverage)
endfunction()

function(add_coverage_target)
    if(NOT ENABLE_COVERAGE)
        return()
    endif()

    find_program(LCOV_PATH lcov REQUIRED)
    find_program(GENHTML_PATH genhtml REQUIRED)

    set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(COVERAGE_INFO "${COVERAGE_DIR}/coverage.info")
    set(COVERAGE_CLEAN "${COVERAGE_DIR}/coverage_clean.info")
    set(COVERAGE_HTML  "${COVERAGE_DIR}/html")

    set(LINE_THRESHOLD   90)
    set(BRANCH_THRESHOLD 85)

    # VERBATIM prevents CMake / Ninja from shell-expanding glob patterns such
    # as "*/include/stream_chunk/*.hpp" before they reach lcov.
    add_custom_target(coverage_report
        COMMENT "Generating coverage report (line >= ${LINE_THRESHOLD}%, branch >= ${BRANCH_THRESHOLD}%)"

        # Create output directory
        COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}

        # Reset counters
        COMMAND ${LCOV_PATH} --zerocounters --directory ${CMAKE_BINARY_DIR}

        # Run tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure

        # Capture coverage data (including branch coverage)
        COMMAND ${LCOV_PATH}
            --capture
            --branch-coverage
            --directory ${CMAKE_BINARY_DIR}
            --output-file ${COVERAGE_INFO}

        # Extract only our library headers; apply branch/brace/trivial filters to
        # remove compiler-generated branches (e.g. exception handlers) and other
        # noise that cannot be meaningfully tested.
        COMMAND ${LCOV_PATH}
            --extract ${COVERAGE_INFO}
            "*/include/stream_chunk/*.hpp"
            --branch-coverage
            --filter branch,brace,blank,trivial
            --ignore-errors unused
            --output-file ${COVERAGE_CLEAN}

        # Generate HTML report
        COMMAND ${GENHTML_PATH}
            --branch-coverage
            --output-directory ${COVERAGE_HTML}
            ${COVERAGE_CLEAN}

        # Enforce line coverage threshold
        COMMAND ${CMAKE_COMMAND}
            -DCOVERAGE_INFO=${COVERAGE_CLEAN}
            -DLCOV_PATH=${LCOV_PATH}
            -DTHRESHOLD=${LINE_THRESHOLD}
            -DTYPE=lines
            -P ${CMAKE_SOURCE_DIR}/cmake/check_coverage_threshold.cmake

        # Enforce branch coverage threshold
        COMMAND ${CMAKE_COMMAND}
            -DCOVERAGE_INFO=${COVERAGE_CLEAN}
            -DLCOV_PATH=${LCOV_PATH}
            -DTHRESHOLD=${BRANCH_THRESHOLD}
            -DTYPE=branches
            -P ${CMAKE_SOURCE_DIR}/cmake/check_coverage_threshold.cmake

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        VERBATIM
    )
endfunction()

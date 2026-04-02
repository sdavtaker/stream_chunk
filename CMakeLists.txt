cmake_minimum_required(VERSION 3.25)

project(
    stream_chunk
    VERSION 0.1.0
    DESCRIPTION "ostream that outputs to multiple files with an incremental suffix and rolls over at fixed size intervals"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# C++ standard
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Build type default
# ---------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Export compile commands (needed by clang-tidy)
# ---------------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Coverage
# ---------------------------------------------------------------------------
include(cmake/coverage.cmake)

# ---------------------------------------------------------------------------
# clang-tidy (opt-in via -DENABLE_CLANG_TIDY=ON)
# ---------------------------------------------------------------------------
option(ENABLE_CLANG_TIDY "Run clang-tidy on sources during build" OFF)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_PATH NAMES clang-tidy clang-tidy-18 clang-tidy-17 REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PATH}")
    message(STATUS "clang-tidy enabled: ${CLANG_TIDY_PATH}")
endif()

# ---------------------------------------------------------------------------
# Header-only library
# ---------------------------------------------------------------------------
add_library(stream_chunk INTERFACE)
add_library(stream_chunk::stream_chunk ALIAS stream_chunk)

target_include_directories(stream_chunk
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(stream_chunk INTERFACE cxx_std_23)

# ---------------------------------------------------------------------------
# Install rules
# ---------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS stream_chunk
    EXPORT stream_chunk_targets
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT stream_chunk_targets
    FILE        stream_chunkTargets.cmake
    NAMESPACE   stream_chunk::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stream_chunk
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/stream_chunkConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/stream_chunkConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stream_chunk
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/stream_chunkConfigVersion.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/stream_chunkConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/stream_chunkConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/stream_chunk
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
option(BUILD_TESTING "Build unit tests" ON)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
    add_coverage_target()
endif()

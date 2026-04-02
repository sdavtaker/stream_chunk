find_package(Catch2 3 REQUIRED)

add_executable(stream_chunk_tests
    test_stream_chunk.cpp
)

target_link_libraries(stream_chunk_tests
    PRIVATE
        stream_chunk::stream_chunk
        Catch2::Catch2WithMain
)

target_compile_options(stream_chunk_tests
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Werror>
)

# Coverage instrumentation is applied to the test binary; because the library
# is header-only, all library code is compiled into this translation unit and
# will be measured automatically.
enable_coverage(stream_chunk_tests)

include(CTest)
include(Catch)
catch_discover_tests(stream_chunk_tests)

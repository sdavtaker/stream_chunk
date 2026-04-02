# stream_chunk

ostream that output to multiple files with a incremental suffix in the filename and rolls over at fixed size intervals.

`stream_chunk` is a C++23 library that provides a `std::ostream`-compatible interface
for writing to a sequence of files. A new file is opened whenever the current file
reaches a configurable byte-size limit. Files are named `<base_path>.<index>` where
`<index>` starts at `0` and increments on each rotation.

## Requirements

| Tool | Minimum version |
|------|----------------|
| CMake | 3.25 |
| C++ compiler with C++23 support | GCC 13 / Clang 17 / MSVC 19.38 |
| [vcpkg](https://vcpkg.io) | any recent version |
| Catch2 (via vcpkg) | 3.x |
| lcov + genhtml (optional, coverage only) | any recent version |
| clang-tidy (optional) | 17+ |

## Cloning

```bash
git clone --recurse-submodules https://github.com/sdavtaker/stream_chunk.git
cd stream_chunk
```

## Building

### 1. Bootstrap vcpkg (first time only)

```bash
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh      # Linux / macOS
# or
.\vcpkg\bootstrap-vcpkg.bat     # Windows
```

### 2. Configure

```bash
cmake -B build \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
```

**Optional flags**

| Flag | Default | Description |
|------|---------|-------------|
| `-DBUILD_TESTING=ON` | `ON` | Build unit tests |
| `-DENABLE_COVERAGE=ON` | `OFF` | Instrument for code coverage |
| `-DENABLE_CLANG_TIDY=ON` | `OFF` | Run clang-tidy during build |

### 3. Build

```bash
cmake --build build --parallel
```

## Testing

```bash
cd build
ctest --output-on-failure
```

Or run the test binary directly:

```bash
./build/tests/stream_chunk_tests
```

## Code Coverage

Coverage requires a GCC or Clang build with `lcov` and `genhtml` installed.

```bash
cmake -B build \
      -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_COVERAGE=ON \
      -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake

cmake --build build --parallel
cmake --build build --target coverage_report
```

The `coverage_report` target:
1. Runs the full test suite via CTest.
2. Generates an HTML report in `build/coverage/html/index.html`.
3. **Fails the build** if line coverage drops below **90 %** or branch coverage drops below **85 %**.

## Code Style

The project follows the **LLVM** coding style enforced by `clang-format`:

```bash
# Check formatting
clang-format --dry-run --Werror $(find include src tests -name '*.hpp' -o -name '*.cpp')

# Apply formatting
clang-format -i $(find include src tests -name '*.hpp' -o -name '*.cpp')
```

clang-tidy rules are defined in `.clang-tidy`. Enable during build with
`-DENABLE_CLANG_TIDY=ON`.

## Installing

```bash
cmake --install build --prefix /usr/local
```

This installs:
- Headers → `<prefix>/include/stream_chunk/`
- Library → `<prefix>/lib/`
- CMake package config → `<prefix>/lib/cmake/stream_chunk/`

Downstream projects can then use:

```cmake
find_package(stream_chunk REQUIRED)
target_link_libraries(my_target PRIVATE stream_chunk::stream_chunk)
```

## Quick usage example

```cpp
#include <stream_chunk/stream_chunk.hpp>

int main() {
    // Creates log.0, log.1, … each at most 1 MiB.
    stream_chunk::ChunkedFileStream out("log", 1024 * 1024);
    for (int i = 0; i < 1000; ++i) {
        out << "Entry " << i << '\n';
    }
}
```

## License

BSD 2-Clause – see [LICENSE](LICENSE).


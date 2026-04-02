# AGENTS.md – stream_chunk project guide

This file is intended for AI coding agents (and human developers) who need a
quick orientation to the project structure and the standard build / test
workflow.

---

## Project overview

`stream_chunk` is a **header-only** C++23 library that exposes a
`std::ostream`-compatible interface for writing to a *sequence* of files.
Each file is capped at a configurable byte-size limit; a new file is opened
transparently once the limit is reached.

The project deliberately mirrors the `std::ofstream` / `std::filebuf`
interface so that it can be used as a drop-in replacement wherever log
rotation is needed.

---

## Repository layout

```
stream_chunk/
├── CMakeLists.txt               # Top-level build definition
├── vcpkg.json                   # vcpkg manifest (Catch2 dependency)
├── .clang-format                # LLVM style rules
├── .clang-tidy                  # clang-tidy checks (warnings-as-errors)
├── LICENSE                      # BSD-2-Clause
├── README.md                    # End-user documentation
├── AGENTS.md                    # This file
│
├── .github/
│   └── workflows/
│       └── ci.yml               # GitHub Actions CI (ubuntu-latest)
│
├── cmake/
│   ├── coverage.cmake           # Coverage helper functions & targets
│   ├── check_coverage_threshold.cmake  # CMake script: assert coverage %
│   └── stream_chunkConfig.cmake.in     # CMake package config template
│
├── include/
│   └── stream_chunk/
│       └── stream_chunk.hpp     # Full API + inline implementation (SPDX: BSD-2-Clause)
│
└── tests/
    ├── CMakeLists.txt           # Test target (Catch2); coverage instrumentation here
    └── test_stream_chunk.cpp    # Unit tests
```

---

## Key design decisions

| Concern | Decision |
|---------|----------|
| Language standard | C++23 |
| Library type | Header-only (INTERFACE CMake target) |
| Build system | CMake ≥ 3.25 |
| Dependency management | vcpkg (manifest mode) |
| Test framework | Catch2 v3 |
| CI platform | GitHub Actions (ubuntu-latest) |
| Code style | LLVM (clang-format) |
| Static analysis | clang-tidy (warnings-as-errors) |
| Coverage thresholds | Lines ≥ 90 %, Branches ≥ 85 % |
| License | BSD-2-Clause (SPDX identifiers in all headers) |

---

## Build instructions (quick reference)

```bash
# 1. Bootstrap vcpkg (once)
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

# 2. Configure
cmake -B build \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake

# 3. Build
cmake --build build --parallel
```

### Useful CMake flags

| Flag | Default | Purpose |
|------|---------|---------|
| `BUILD_TESTING` | `ON` | Compile unit tests |
| `ENABLE_COVERAGE` | `OFF` | Add `--coverage` to compile/link |
| `ENABLE_CLANG_TIDY` | `OFF` | Integrate clang-tidy into build |

---

## Test instructions (quick reference)

```bash
cd build
ctest --output-on-failure
```

Run the test binary directly for more verbose output:

```bash
./build/tests/stream_chunk_tests --reporter=compact
```

---

## Coverage instructions

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON \
      -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --parallel
cmake --build build --target coverage_report
# HTML report: build/coverage/html/index.html
```

The target fails if line coverage < 90 % or branch coverage < 85 %.

---

## Code style

```bash
# Check (CI mode)
clang-format --dry-run --Werror \
    $(find include tests -name '*.hpp' -o -name '*.cpp')

# Fix in-place
clang-format -i \
    $(find include tests -name '*.hpp' -o -name '*.cpp')
```

clang-tidy is configured in `.clang-tidy` and can be enabled at configure
time with `-DENABLE_CLANG_TIDY=ON`.

---

## Adding new headers

Because the library is header-only, all code lives in `include/stream_chunk/`.

1. Add the new `.hpp` file to `include/stream_chunk/` with the SPDX header:
   ```cpp
   // SPDX-License-Identifier: BSD-2-Clause
   // Copyright (c) 2026, Damian Vicino
   ```
2. Write corresponding tests in `tests/test_stream_chunk.cpp` (or a new
   `tests/test_<feature>.cpp` registered in `tests/CMakeLists.txt`).
3. No changes to `CMakeLists.txt` are needed; INTERFACE libraries expose
   the entire `include/` directory automatically.

// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, Damian Vicino
// All rights reserved.

#include "stream_chunk/stream_chunk.hpp"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helper: read the complete contents of a file into a string.
// ---------------------------------------------------------------------------
static std::string read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    REQUIRE(f.is_open());
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}

// ---------------------------------------------------------------------------
// Helper: RAII temporary directory.
// ---------------------------------------------------------------------------
struct TempDir {
    fs::path path;

    TempDir() {
        static std::atomic<unsigned> counter{0};
        path = fs::temp_directory_path() /
               ("stream_chunk_test_" + std::to_string(counter.fetch_add(1)));
        fs::create_directories(path);
    }

    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("ChunkedFileStream – chunk_size=0 throws", "[ChunkedFileStream]") {
    TempDir tmp;
    REQUIRE_THROWS_AS(
        stream_chunk::ChunkedFileStream(tmp.path / "log", 0),
        std::invalid_argument);
}

TEST_CASE("ChunkedFileStream – single write within one chunk", "[ChunkedFileStream]") {
    TempDir tmp;
    const fs::path base = tmp.path / "log";

    {
        stream_chunk::ChunkedFileStream out(base, 1024);
        out << "Hello, world!";
    }

    // Only file index 0 should exist.
    REQUIRE(fs::exists(base.string() + ".0"));
    REQUIRE_FALSE(fs::exists(base.string() + ".1"));

    REQUIRE(read_file(base.string() + ".0") == "Hello, world!");
}

TEST_CASE("ChunkedFileStream – rotation at exact chunk boundary", "[ChunkedFileStream]") {
    TempDir tmp;
    const fs::path base = tmp.path / "log";

    constexpr std::size_t chunk = 5;
    const std::string data = "ABCDEFGHIJ"; // 10 bytes → 2 chunks of 5

    {
        stream_chunk::ChunkedFileStream out(base, chunk);
        out << data;
    }

    REQUIRE(fs::exists(base.string() + ".0"));
    REQUIRE(fs::exists(base.string() + ".1"));
    REQUIRE_FALSE(fs::exists(base.string() + ".2"));

    REQUIRE(read_file(base.string() + ".0") == "ABCDE");
    REQUIRE(read_file(base.string() + ".1") == "FGHIJ");
}

TEST_CASE("ChunkedFileStream – rotation after chunk boundary", "[ChunkedFileStream]") {
    TempDir tmp;
    const fs::path base = tmp.path / "log";

    constexpr std::size_t chunk = 4;
    // 11 bytes: chunks of 4, 4, 3 → three files
    const std::string data = "ABCDEFGHIJK";

    {
        stream_chunk::ChunkedFileStream out(base, chunk);
        out << data;
    }

    REQUIRE(fs::exists(base.string() + ".0"));
    REQUIRE(fs::exists(base.string() + ".1"));
    REQUIRE(fs::exists(base.string() + ".2"));
    REQUIRE_FALSE(fs::exists(base.string() + ".3"));

    REQUIRE(read_file(base.string() + ".0") == "ABCD");
    REQUIRE(read_file(base.string() + ".1") == "EFGH");
    REQUIRE(read_file(base.string() + ".2") == "IJK");
}

TEST_CASE("ChunkedFileStream – multiple small writes accumulate", "[ChunkedFileStream]") {
    TempDir tmp;
    const fs::path base = tmp.path / "log";

    {
        stream_chunk::ChunkedFileStream out(base, 4);
        out << "AB";
        out << "CD"; // Fills chunk 0 exactly
        out << "EF"; // Goes into chunk 1
    }

    REQUIRE(read_file(base.string() + ".0") == "ABCD");
    REQUIRE(read_file(base.string() + ".1") == "EF");
}

TEST_CASE("ChunkedFileStream – current_chunk_index and bytes_written", "[ChunkedFileStream]") {
    TempDir tmp;
    const fs::path base = tmp.path / "log";

    stream_chunk::ChunkedFileStream out(base, 8);

    REQUIRE(out.current_chunk_index() == 0);
    REQUIRE(out.bytes_written_in_current_chunk() == 0);

    out << "ABCD"; // 4 bytes
    REQUIRE(out.current_chunk_index() == 0);
    REQUIRE(out.bytes_written_in_current_chunk() == 4);

    out << "EFGH"; // fills chunk 0 (8 bytes total)
    REQUIRE(out.current_chunk_index() == 0);
    REQUIRE(out.bytes_written_in_current_chunk() == 8);

    out << "I"; // triggers rotation → now in chunk 1
    REQUIRE(out.current_chunk_index() == 1);
    REQUIRE(out.bytes_written_in_current_chunk() == 1);
}

TEST_CASE("ChunkedFileStream – single-byte chunk size", "[ChunkedFileStream]") {
    TempDir tmp;
    const fs::path base = tmp.path / "log";

    {
        stream_chunk::ChunkedFileStream out(base, 1);
        out << "XYZ";
    }

    REQUIRE(read_file(base.string() + ".0") == "X");
    REQUIRE(read_file(base.string() + ".1") == "Y");
    REQUIRE(read_file(base.string() + ".2") == "Z");
    REQUIRE_FALSE(fs::exists(base.string() + ".3"));
}

TEST_CASE("ChunkedFileStream – overflow reached via single-char writes", "[ChunkedFileBuf]") {
    // out.put(c) → sputc(c) → overflow(c), exercising the overflow() path.
    TempDir tmp;
    const fs::path base = tmp.path / "log";

    {
        stream_chunk::ChunkedFileStream out(base, 2);
        out.put('A'); // overflow('A') – no rotation yet
        out.put('B'); // overflow('B') – fills chunk 0 exactly
        out.put('C'); // overflow('C') – triggers rotation, then writes to chunk 1
    }

    REQUIRE(read_file(base.string() + ".0") == "AB");
    REQUIRE(read_file(base.string() + ".1") == "C");
}

TEST_CASE("ChunkedFileStream – throws on non-existent parent directory", "[ChunkedFileStream]") {
    // Exercises the `!file_buf_.open(...)` error branch in rotate().
    REQUIRE_THROWS_AS(
        stream_chunk::ChunkedFileStream("/nonexistent_dir_xyz/log", 1024),
        std::runtime_error);
}

TEST_CASE("ChunkedFileStream – sync flushes without error", "[ChunkedFileStream]") {
    // Exercises sync() / pubsync() through the explicit flush() call.
    TempDir tmp;
    stream_chunk::ChunkedFileStream out(tmp.path / "log", 1024);
    out << "flush me";
    out.flush();
    REQUIRE(out.good());
}

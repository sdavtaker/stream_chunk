// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, Damian Vicino
// All rights reserved.
//
// stream_chunk – ostream that writes to multiple files with an incremental
// filename suffix and rolls over when a configurable byte-size limit is
// reached.

#pragma once

#include <cstddef>
#include <filesystem>
#include <format>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <streambuf>
#include <string>

namespace stream_chunk {

/// A std::streambuf that transparently rotates the underlying file whenever
/// the number of bytes written to the current file reaches @p chunk_size.
///
/// Files are named  <base_path>.<index>  where @p index starts at 0 and
/// increments by 1 on each rotation.
///
/// Example
/// -------
/// @code
/// stream_chunk::ChunkedFileBuf buf("log", 1024);   // 1 KiB per file
/// std::ostream out(&buf);
/// out << "Hello, world!\n";
/// @endcode
class ChunkedFileBuf : public std::streambuf {
public:
    /// Construct the buffer.
    ///
    /// @param base_path  Path prefix for generated files (no extension needed).
    /// @param chunk_size Maximum number of bytes written to a single file
    ///                   before a new file is opened.  Must be greater than 0.
    explicit ChunkedFileBuf(std::filesystem::path base_path, std::size_t chunk_size)
        : base_path_(std::move(base_path)), chunk_size_(chunk_size) {
        if (chunk_size_ == 0) {
            throw std::invalid_argument("chunk_size must be greater than 0");
        }
        rotate();
    }

    /// Flushes and closes the currently open file.
    ~ChunkedFileBuf() override {
        sync();
        file_buf_.close();
    }

    // Non-copyable, non-movable (owns an open file handle).
    ChunkedFileBuf(const ChunkedFileBuf&) = delete;
    ChunkedFileBuf& operator=(const ChunkedFileBuf&) = delete;
    ChunkedFileBuf(ChunkedFileBuf&&) = delete;
    ChunkedFileBuf& operator=(ChunkedFileBuf&&) = delete;

    /// Returns the index of the file currently being written.
    /// Invariant: rotate() is always called in the constructor, so
    /// next_file_index_ >= 1 and this subtraction is always safe.
    [[nodiscard]] std::size_t current_chunk_index() const noexcept {
        return next_file_index_ - 1;
    }

    /// Returns the number of bytes written to the current file so far.
    [[nodiscard]] std::size_t bytes_written_in_current_chunk() const noexcept {
        return bytes_in_chunk_;
    }

protected:
    // std::streambuf overrides

    int_type overflow(int_type ch) override {
        // LCOV_EXCL_START – overflow(eof) is standard virtual boilerplate;
        // there is no public API path that routes a bare EOF through here.
        if (ch == traits_type::eof()) {
            return traits_type::eof();
        }
        // LCOV_EXCL_STOP

        if (bytes_in_chunk_ >= chunk_size_) {
            rotate();
        }

        const char c = traits_type::to_char_type(ch);
        // LCOV_EXCL_START – sputc() failure requires OS-level write error;
        // cannot be induced without replacing the underlying filebuf.
        if (file_buf_.sputc(c) == traits_type::eof()) {
            return traits_type::eof();
        }
        // LCOV_EXCL_STOP

        ++bytes_in_chunk_;
        return ch;
    }

    std::streamsize xsputn(const char_type* s, std::streamsize count) override {
        std::streamsize written = 0;

        while (written < count) {
            if (bytes_in_chunk_ >= chunk_size_) {
                rotate();
            }

            const std::streamsize remaining_in_chunk =
                static_cast<std::streamsize>(chunk_size_ - bytes_in_chunk_);
            const std::streamsize to_write = std::min(count - written, remaining_in_chunk);

            const std::streamsize result = file_buf_.sputn(s + written, to_write);
            bytes_in_chunk_ += static_cast<std::size_t>(result);
            written += result;

            // LCOV_EXCL_START – sputn() short-write requires OS-level error;
            // cannot be induced without replacing the underlying filebuf.
            if (result < to_write) {
                break;
            }
            // LCOV_EXCL_STOP
        }

        return written;
    }

    int sync() override { return file_buf_.pubsync(); }

private:
    /// Opens the next file in the sequence and resets the byte counter.
    /// After this call next_file_index_ is always >= 1.
    void rotate() {
        if (file_buf_.is_open()) {
            file_buf_.pubsync();
            file_buf_.close();
        }

        const auto path =
            base_path_.parent_path() /
            (base_path_.filename().string() + std::format(".{}", next_file_index_));

        if (!file_buf_.open(path, std::ios::out | std::ios::binary)) {
            throw std::runtime_error(
                std::format("stream_chunk: failed to open file '{}'", path.string()));
        }

        ++next_file_index_;
        bytes_in_chunk_ = 0;
    }

    std::filesystem::path base_path_;
    std::size_t chunk_size_;
    /// Index of the *next* file to open on the following rotation.
    /// Always >= 1 after construction (the constructor calls rotate() once).
    std::size_t next_file_index_{0};
    std::size_t bytes_in_chunk_{0};
    std::filebuf file_buf_;
};

/// A convenience ostream backed by a ChunkedFileBuf.
///
/// Example
/// -------
/// @code
/// stream_chunk::ChunkedFileStream out("log", 1024);
/// out << "Hello, world!\n";
/// @endcode
class ChunkedFileStream : public std::ostream {
public:
    /// @see ChunkedFileBuf::ChunkedFileBuf
    explicit ChunkedFileStream(std::filesystem::path base_path, std::size_t chunk_size)
        : std::ostream(nullptr), buf_(std::move(base_path), chunk_size) {
        rdbuf(&buf_);
    }

    ~ChunkedFileStream() override = default;

    // Non-copyable, non-movable.
    ChunkedFileStream(const ChunkedFileStream&) = delete;
    ChunkedFileStream& operator=(const ChunkedFileStream&) = delete;
    ChunkedFileStream(ChunkedFileStream&&) = delete;
    ChunkedFileStream& operator=(ChunkedFileStream&&) = delete;

    /// @see ChunkedFileBuf::current_chunk_index
    [[nodiscard]] std::size_t current_chunk_index() const noexcept {
        return buf_.current_chunk_index();
    }

    /// @see ChunkedFileBuf::bytes_written_in_current_chunk
    [[nodiscard]] std::size_t bytes_written_in_current_chunk() const noexcept {
        return buf_.bytes_written_in_current_chunk();
    }

private:
    ChunkedFileBuf buf_;
};

} // namespace stream_chunk

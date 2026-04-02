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
#include <fstream>
#include <ostream>
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
    explicit ChunkedFileBuf(std::filesystem::path base_path, std::size_t chunk_size);

    /// Flushes and closes the currently open file.
    ~ChunkedFileBuf() override;

    // Non-copyable, non-movable (owns an open file handle).
    ChunkedFileBuf(const ChunkedFileBuf&) = delete;
    ChunkedFileBuf& operator=(const ChunkedFileBuf&) = delete;
    ChunkedFileBuf(ChunkedFileBuf&&) = delete;
    ChunkedFileBuf& operator=(ChunkedFileBuf&&) = delete;

    /// Returns the index of the file currently being written.
    [[nodiscard]] std::size_t current_chunk_index() const noexcept;

    /// Returns the number of bytes written to the current file so far.
    [[nodiscard]] std::size_t bytes_written_in_current_chunk() const noexcept;

protected:
    // std::streambuf overrides
    int_type overflow(int_type ch) override;
    std::streamsize xsputn(const char_type* s, std::streamsize count) override;
    int sync() override;

private:
    /// Opens the next file in the sequence and resets the byte counter.
    void rotate();

    std::filesystem::path base_path_;
    std::size_t chunk_size_;
    std::size_t current_index_{0};
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
    explicit ChunkedFileStream(std::filesystem::path base_path, std::size_t chunk_size);

    ~ChunkedFileStream() override = default;

    // Non-copyable, non-movable.
    ChunkedFileStream(const ChunkedFileStream&) = delete;
    ChunkedFileStream& operator=(const ChunkedFileStream&) = delete;
    ChunkedFileStream(ChunkedFileStream&&) = delete;
    ChunkedFileStream& operator=(ChunkedFileStream&&) = delete;

    /// @see ChunkedFileBuf::current_chunk_index
    [[nodiscard]] std::size_t current_chunk_index() const noexcept;

    /// @see ChunkedFileBuf::bytes_written_in_current_chunk
    [[nodiscard]] std::size_t bytes_written_in_current_chunk() const noexcept;

private:
    ChunkedFileBuf buf_;
};

} // namespace stream_chunk

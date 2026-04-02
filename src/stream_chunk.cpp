// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, Damian Vicino
// All rights reserved.

#include "stream_chunk/stream_chunk.hpp"

#include <cassert>
#include <format>
#include <stdexcept>

namespace stream_chunk {

// ---------------------------------------------------------------------------
// ChunkedFileBuf
// ---------------------------------------------------------------------------

ChunkedFileBuf::ChunkedFileBuf(std::filesystem::path base_path, std::size_t chunk_size)
    : base_path_(std::move(base_path)), chunk_size_(chunk_size) {
    if (chunk_size_ == 0) {
        throw std::invalid_argument("chunk_size must be greater than 0");
    }
    rotate();
}

ChunkedFileBuf::~ChunkedFileBuf() {
    // Flush any buffered data before closing.
    sync();
    file_buf_.close();
}

std::size_t ChunkedFileBuf::current_chunk_index() const noexcept {
    return current_index_ - 1;
}

std::size_t ChunkedFileBuf::bytes_written_in_current_chunk() const noexcept {
    return bytes_in_chunk_;
}

ChunkedFileBuf::int_type ChunkedFileBuf::overflow(int_type ch) {
    if (ch == traits_type::eof()) {
        return traits_type::eof();
    }

    if (bytes_in_chunk_ >= chunk_size_) {
        rotate();
    }

    char c = traits_type::to_char_type(ch);
    if (file_buf_.sputc(c) == traits_type::eof()) {
        return traits_type::eof();
    }

    ++bytes_in_chunk_;
    return ch;
}

std::streamsize ChunkedFileBuf::xsputn(const char_type* s, std::streamsize count) {
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

        if (result < to_write) {
            // Underlying write failed.
            break;
        }
    }

    return written;
}

int ChunkedFileBuf::sync() {
    return file_buf_.pubsync();
}

void ChunkedFileBuf::rotate() {
    if (file_buf_.is_open()) {
        file_buf_.pubsync();
        file_buf_.close();
    }

    const auto path =
        base_path_.parent_path() /
        (base_path_.filename().string() + std::format(".{}", current_index_));

    if (!file_buf_.open(path, std::ios::out | std::ios::binary)) {
        throw std::runtime_error(
            std::format("stream_chunk: failed to open file '{}'", path.string()));
    }

    ++current_index_;
    bytes_in_chunk_ = 0;
}

// ---------------------------------------------------------------------------
// ChunkedFileStream
// ---------------------------------------------------------------------------

ChunkedFileStream::ChunkedFileStream(std::filesystem::path base_path, std::size_t chunk_size)
    : std::ostream(nullptr), buf_(std::move(base_path), chunk_size) {
    rdbuf(&buf_);
}

std::size_t ChunkedFileStream::current_chunk_index() const noexcept {
    return buf_.current_chunk_index();
}

std::size_t ChunkedFileStream::bytes_written_in_current_chunk() const noexcept {
    return buf_.bytes_written_in_current_chunk();
}

} // namespace stream_chunk

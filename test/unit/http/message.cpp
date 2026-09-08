/**
 * @file message.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Message::read tests — chunked delivery, pipelined leftover handling,
 *        multi-block heads and the per-request O(1) buffer reuse invariant
 * @version 0.1.0
 * @date 2025-09-08
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <gtest/gtest.h>
#include <xsl/asio.h>

#include <cstring>
#include <string>
#include <vector>

using namespace xsl;
using namespace xsl::asio;

namespace {
constexpr std::string_view REQ1 = "GET /hello HTTP/1.1\r\nHost: localhost\r\n\r\n";
constexpr std::string_view REQ2 = "POST /submit HTTP/1.1\r\nHost: localhost\r\n\r\n";

/**
 * @brief mock async reader delivering a fixed byte string in fixed-size reads
 */
struct ChunkReader {
  std::string data;
  std::size_t offset = 0;
  std::size_t chunk = 1;  ///< max bytes per read()
  std::size_t reads = 0;  ///< number of read() calls issued

  constexpr ChunkReader* operator->() noexcept { return this; }

  Task<io::Result> read(byte* dst, std::size_t size) {
    ++reads;
    std::size_t n = std::min({size, chunk, data.size() - offset});
    std::memcpy(dst, data.data() + offset, n);
    offset += n;
    co_return io::Result{n};
  }
};

Request &read_request(ChunkReader &reader, Request &req) {
  errc res = req.read(reader).block();
  EXPECT_EQ(res, errc{}) << "read failed at offset " << reader.offset;
  return req;
}
}  // namespace

TEST(http_message, whole_head_in_one_read) {
  ChunkReader reader{std::string(REQ1), 0, 4096, 0};
  Request req;
  read_request(reader, req);
  EXPECT_EQ(req.method(), Method::GET);
  EXPECT_EQ(req.line.path, "/hello");
  EXPECT_EQ(req.get_header("Host"), std::optional<std::string_view>("localhost"));
  EXPECT_EQ(reader.reads, 1);
  EXPECT_EQ(req.buffer.size(), 1);  // single block, nothing retained
  EXPECT_EQ(req.win_begin, REQ1.size());  // window consumed up to the head end
  EXPECT_EQ(req.win_end, REQ1.size());
}

TEST(http_message, byte_by_byte_delivery) {
  ChunkReader reader{std::string(REQ1), 0, 1, 0};
  Request req;
  read_request(reader, req);
  EXPECT_EQ(req.method(), Method::GET);
  EXPECT_EQ(req.get_header("Host"), std::optional<std::string_view>("localhost"));
  EXPECT_EQ(reader.reads, REQ1.size());  // one read per byte
}

TEST(http_message, split_header_value_across_reads) {
  // the value of Host splits in the middle of "localhost"
  ChunkReader reader{std::string(REQ1), 0, 27, 0};
  Request req;
  read_request(reader, req);
  EXPECT_EQ(req.get_header("Host"), std::optional<std::string_view>("localhost"));
}

TEST(http_message, head_split_exactly_before_terminator) {
  // the final CRLF of the head arrives in its own read
  ChunkReader reader{std::string(REQ1), 0, REQ1.size() - 2, 0};
  Request req;
  read_request(reader, req);
  EXPECT_EQ(req.method(), Method::GET);
  EXPECT_EQ(reader.reads, 2);
}

TEST(http_message, head_exactly_block_sized) {
  // head of exactly HTTP_BUFFER_BLOCK_SIZE bytes (last header line terminated
  // by the first CRLF, empty line by the second) must not trigger rotate
  std::string head = "GET / HTTP/1.1\r\n";
  while (head.size() < HTTP_BUFFER_BLOCK_SIZE - 4) {
    head += "X: 0123456789\r\n";
  }
  head.resize(HTTP_BUFFER_BLOCK_SIZE - 4);
  head += "\r\n\r\n";
  ASSERT_EQ(head.size(), HTTP_BUFFER_BLOCK_SIZE);
  ChunkReader reader{head, 0, 4096, 0};
  Request req;
  read_request(reader, req);
  EXPECT_EQ(req.method(), Method::GET);
  EXPECT_EQ(reader.reads, 1);
  EXPECT_EQ(req.buffer.size(), 1);
}

TEST(http_message, multi_block_head) {
  // 200 headers ≈ 6KB: the head spans several blocks (rotate path)
  std::string head = "GET /big HTTP/1.1\r\n";
  for (int i = 0; i < 200; ++i) {
    head += std::format("X-Header-{:03}: value-{:03}\r\n", i, i);
  }
  head += "\r\n";
  ASSERT_GT(head.size(), HTTP_BUFFER_BLOCK_SIZE);
  ChunkReader reader{head, 0, 512, 0};
  Request req;
  read_request(reader, req);
  EXPECT_EQ(req.line.path, "/big");
  EXPECT_EQ(req.get_header("X-Header-007"), std::optional<std::string_view>("value-007"));
  EXPECT_EQ(req.get_header("X-Header-199"), std::optional<std::string_view>("value-199"));
}

TEST(http_message, single_line_longer_than_block_rejected) {
  // one header line of more than one block cannot be parsed: rejected instead
  // of the previous silent corruption
  std::string head = "GET / HTTP/1.1\r\nX: ";
  head += std::string(HTTP_BUFFER_BLOCK_SIZE * 2, 'a');
  head += "\r\n\r\n";
  ChunkReader reader{head, 0, 4096, 0};
  Request req;
  errc res = req.read(reader).block();
  EXPECT_EQ(res, errc::illegal_byte_sequence);
}

TEST(http_message, pipelined_second_request_from_leftover) {
  // two requests delivered in one read: the second read must consume the
  // leftover WITHOUT issuing another read (the old implementation lost it);
  // the same Request object is reused, as the server does per connection
  ChunkReader reader{std::string(REQ1) + std::string(REQ2), 0, 4096, 0};
  Request req;
  read_request(reader, req);
  EXPECT_EQ(req.method(), Method::GET);
  std::size_t reads_after_first = reader.reads;
  read_request(reader, req);
  EXPECT_EQ(req.method(), Method::POST);
  EXPECT_EQ(req.line.path, "/submit");
  EXPECT_EQ(reader.reads, reads_after_first);  // served entirely from leftover
  EXPECT_EQ(req.buffer.size(), 1);
}

TEST(http_message, pipelined_partial_second_request) {
  // second request arrives split across reads after the first one
  ChunkReader reader{std::string(REQ1) + std::string(REQ2), 0, 30, 0};
  Request req;
  read_request(reader, req);
  EXPECT_EQ(req.method(), Method::GET);
  read_request(reader, req);
  EXPECT_EQ(req.method(), Method::POST);
  EXPECT_EQ(req.get_header("Host"), std::optional<std::string_view>("localhost"));
}

TEST(http_message, leftover_across_multi_block_head) {
  // a multi-block head followed by a pipelined request: the leftover lands in
  // a non-zero block and must be compacted back to buffer[0]
  std::string big = "GET /big HTTP/1.1\r\n";
  for (int i = 0; i < 200; ++i) {
    big += std::format("X-Header-{:03}: value-{:03}\r\n", i, i);
  }
  big += "\r\n";
  ChunkReader reader{big + std::string(REQ2), 0, 700, 0};
  Request req;
  read_request(reader, req);
  EXPECT_EQ(req.line.path, "/big");
  ASSERT_GT(req.buffer.size(), 1);  // multi-block head actually exercised
  read_request(reader, req);
  EXPECT_EQ(req.method(), Method::POST);
  EXPECT_EQ(req.win_block, 0);
  // scratch blocks from the multi-block head are retained but bounded; every
  // subsequent request must keep reusing them without growth
  std::size_t blocks_after_big_head = req.buffer.size();
  for (int i = 0; i < 16; ++i) {
    reader.data += std::format("GET /r{} HTTP/1.1\r\nHost: x\r\n\r\n", i);
    read_request(reader, req);
    ASSERT_EQ(req.line.path, std::format("/r{}", i));
    ASSERT_EQ(req.buffer.size(), blocks_after_big_head) << "growth at request " << i;
  }
}

TEST(http_message, no_buffer_growth_over_many_requests) {
  // regression: the old implementation retained one 4KB block per request
  // (and did O(n) work per request); the block count must stay bounded
  std::string stream;
  for (int i = 0; i < 256; ++i) {
    stream += std::format("GET /r{} HTTP/1.1\r\nHost: x\r\n\r\n", i);
  }
  ChunkReader reader{stream, 0, 4096, 0};
  Request req;
  for (int i = 0; i < 256; ++i) {
    errc res = req.read(reader).block();
    ASSERT_EQ(res, errc{});
    ASSERT_EQ(req.line.path, std::format("/r{}", i));
    ASSERT_LE(req.buffer.size(), 2uz) << "block leak at request " << i;
    ASSERT_EQ(req.win_block, 0uz);
  }
}

int main() {
  testing::InitGoogleTest();
  return RUN_ALL_TESTS();
}

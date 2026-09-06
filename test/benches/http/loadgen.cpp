/**
 * @file loadgen.cpp
 * @author Haixin Pang (kmdr.error@gmail.com)
 * @brief Keep-alive HTTP/1.1 load generator for the HTTP-vs-asio benchmark,
 *        shared by both servers so the comparison is fair
 * @version 0.1.0
 * @date 2025-12-17
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <CLI/CLI.hpp>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

struct Config {
  std::string host = "127.0.0.1";
  int port = 8080;
  std::string path = "/hello";
  int threads = 4;
  int conns = 4;  ///< connections per thread
  double duration = 5.0;
  std::string out;  ///< optional JSON output path
};

using Clock = std::chrono::steady_clock;

struct ThreadStats {
  std::uint64_t requests = 0;
  std::uint64_t errors = 0;
  std::vector<std::uint32_t> latency_us;  ///< per request latency in microseconds
};

int connect_once(const Config& cfg) {
  int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return -1;
  sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(static_cast<std::uint16_t>(cfg.port));
  if (::inet_pton(AF_INET, cfg.host.c_str(), &sa.sin_addr) != 1) {
    // fall back to DNS
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* ais = nullptr;
    if (::getaddrinfo(cfg.host.c_str(), std::to_string(cfg.port).c_str(), &hints, &ais) != 0
        || ais == nullptr) {
      ::close(fd);
      return -1;
    }
    int rc = ::connect(fd, ais->ai_addr, ais->ai_addrlen);
    ::freeaddrinfo(ais);
    if (rc != 0) {
      ::close(fd);
      return -1;
    }
  } else if (::connect(fd, reinterpret_cast<sockaddr*>(&sa), sizeof sa) != 0) {
    ::close(fd);
    return -1;
  }
  int one = 1;
  ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
  timeval tv{5, 0};
  ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
  ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
  return fd;
}

/// @brief write the whole buffer, returns false on error
bool write_all(int fd, std::string_view data) {
  std::size_t sent = 0;
  while (sent < data.size()) {
    ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
    if (n <= 0) return false;
    sent += static_cast<std::size_t>(n);
  }
  return true;
}

/// @brief parsed response head
struct ResponseInfo {
  int status = 0;
  std::size_t head_size = 0;
  bool has_length = false;  ///< Content-Length present
  std::size_t content_length = 0;
  bool close = false;  ///< server closes after this response
};

/// @brief find "\r\n\r\n" inside [begin, end)
const char* find_head_end(const char* begin, const char* end) {
  if (end - begin < 4) return nullptr;
  for (const char* p = begin; p + 4 <= end; ++p) {
    if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n') return p;
  }
  return nullptr;
}

bool parse_response_head(const std::string& buf, ResponseInfo& info) {
  const char* head_end = find_head_end(buf.data(), buf.data() + buf.size());
  if (head_end == nullptr) return false;
  info.head_size = static_cast<std::size_t>(head_end - buf.data()) + 4;
  std::string_view head(buf.data(), info.head_size);
  std::size_t line_end = head.find("\r\n");
  std::string_view status_line = head.substr(0, line_end);
  // HTTP/1.1 200 OK
  std::size_t sp1 = status_line.find(' ');
  if (sp1 == std::string_view::npos) return false;
  std::size_t sp2 = status_line.find(' ', sp1 + 1);
  std::string_view code
      = status_line.substr(sp1 + 1, sp2 == std::string_view::npos ? std::string_view::npos
                                                                  : sp2 - sp1 - 1);
  std::from_chars(code.begin(), code.end(), info.status);
  auto iequals = [](std::string_view a, std::string_view b) {
    return a.size() == b.size()
           && std::equal(a.begin(), a.end(), b.begin(),
                         [](char x, char y) {
                           return std::tolower(static_cast<unsigned char>(x))
                                  == std::tolower(static_cast<unsigned char>(y));
                         });
  };
  std::size_t pos = line_end == std::string_view::npos ? std::string_view::npos : line_end + 2;
  while (pos != std::string_view::npos && pos < head.size()) {
    std::size_t end = head.find("\r\n", pos);
    std::string_view line = head.substr(pos, end == std::string_view::npos ? head.size() - pos
                                                                           : end - pos);
    std::size_t colon = line.find(':');
    if (colon != std::string_view::npos) {
      std::string_view key = line.substr(0, colon);
      std::string_view value = line.substr(colon + 1);
      value.remove_prefix(std::min(value.size(), value.find_first_not_of(" \t")));
      if (iequals(key, "Content-Length")) {
        info.has_length = true;
        std::from_chars(value.begin(), value.end(), info.content_length);
      } else if (iequals(key, "Connection") && iequals(value, "close")) {
        info.close = true;
      }
    }
    pos = end == std::string_view::npos ? std::string_view::npos : end + 2;
  }
  return true;
}

/// @brief drop exactly `n` body bytes (some may already be buffered in `buf`)
bool drop_body(int fd, std::string& buf, std::size_t n) {
  std::size_t buffered = std::min(buf.size(), n);
  buf.erase(0, buffered);
  n -= buffered;
  char sink[8192];
  while (n > 0) {
    ssize_t got = ::recv(fd, sink, std::min(n, sizeof sink), 0);
    if (got <= 0) return false;
    n -= static_cast<std::size_t>(got);
  }
  return true;
}

void worker(const Config& cfg, ThreadStats& stats, const std::atomic<bool>& stop) {
  std::string request = std::format(
      "GET {} HTTP/1.1\r\nHost: {}:{}\r\nUser-Agent: xsl-loadgen/0.1\r\nAccept: */*\r\n\r\n",
      cfg.path, cfg.host, cfg.port);
  auto deadline = Clock::now() + std::chrono::duration_cast<Clock::duration>(
                                      std::chrono::duration<double>(cfg.duration));

  std::vector<int> fds(static_cast<std::size_t>(cfg.conns));
  for (int& fd : fds) fd = connect_once(cfg);

  std::string buf;
  buf.reserve(4096);
  char chunk[8192];

  for (std::size_t idx = 0; !stop.load(std::memory_order_relaxed); ++idx) {
    if (Clock::now() >= deadline) break;
    int& fd = fds[idx % fds.size()];
    if (fd < 0) fd = connect_once(cfg);
    if (fd < 0) {
      ++stats.errors;
      continue;
    }
    auto t0 = Clock::now();
    if (!write_all(fd, request)) {
      ++stats.errors;
      ::close(fd);
      fd = -1;
      continue;
    }
    ResponseInfo info;
    buf.clear();
    bool ok = true;
    while (!parse_response_head(buf, info)) {
      ssize_t got = ::recv(fd, chunk, sizeof chunk, 0);
      if (got <= 0) {
        ok = false;
        break;
      }
      buf.append(chunk, static_cast<std::size_t>(got));
    }
    if (ok) {
      if (info.has_length) {
        ok = drop_body(fd, buf, info.head_size + info.content_length);
      } else if (info.close) {
        // read until EOF
        ssize_t got;
        while ((got = ::recv(fd, chunk, sizeof chunk, 0)) > 0) {}
        ok = got == 0;
      } else {
        ok = false;  // no way to delimit the body, avoid hanging
      }
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - t0);
    if (!ok || info.status != 200) {
      ++stats.errors;
      ::close(fd);
      fd = -1;
      continue;
    }
    if (info.close) {
      ::close(fd);
      fd = -1;
    }
    ++stats.requests;
    stats.latency_us.push_back(static_cast<std::uint32_t>(
        std::min<std::uint64_t>(elapsed.count(), std::numeric_limits<std::uint32_t>::max())));
  }
  for (int fd : fds) {
    if (fd >= 0) ::close(fd);
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  Config cfg;
  CLI::App app{"Keep-alive HTTP/1.1 load generator (HTTP vs asio benchmark)"};
  app.add_option("-H,--host", cfg.host, "Server host")->capture_default_str();
  app.add_option("-p,--port", cfg.port, "Server port")->check(CLI::Range(1, 65535))->capture_default_str();
  app.add_option("-P,--path", cfg.path, "Request path")->capture_default_str();
  app.add_option("-t,--threads", cfg.threads, "Worker threads")->check(CLI::Range(1, 256))->capture_default_str();
  app.add_option("-c,--conns", cfg.conns, "Connections per thread")->check(CLI::Range(1, 1024))->capture_default_str();
  app.add_option("-d,--duration", cfg.duration, "Test duration in seconds")->check(CLI::Range(0.1, 3600.0))->capture_default_str();
  app.add_option("-o,--out", cfg.out, "Write results as JSON to this file");
  CLI11_PARSE(app, argc, argv);

  std::atomic<bool> stop{false};
  std::vector<ThreadStats> stats(static_cast<std::size_t>(cfg.threads));
  std::vector<std::thread> workers;
  workers.reserve(static_cast<std::size_t>(cfg.threads));
  auto t0 = Clock::now();
  for (auto& one : stats) {
    workers.emplace_back([&cfg, &one, &stop] { worker(cfg, one, stop); });
  }
  std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::duration<double>(cfg.duration)));
  stop.store(true);
  for (auto& w : workers) w.join();
  double elapsed
      = std::chrono::duration<double>(Clock::now() - t0).count();

  std::uint64_t requests = 0, errors = 0;
  std::vector<std::uint32_t> latencies;
  for (auto& one : stats) {
    requests += one.requests;
    errors += one.errors;
    latencies.insert(latencies.end(), one.latency_us.begin(), one.latency_us.end());
  }
  std::sort(latencies.begin(), latencies.end());
  auto percentile = [&](double p) -> std::uint32_t {
    if (latencies.empty()) return 0;
    auto idx = static_cast<std::size_t>(std::ceil(p / 100.0 * latencies.size())) - 1;
    return latencies[std::min(idx, latencies.size() - 1)];
  };
  double rps = requests / elapsed;

  std::printf("requests=%llu errors=%llu duration=%.2fs rps=%.1f\n",
              static_cast<unsigned long long>(requests),
              static_cast<unsigned long long>(errors), elapsed, rps);
  std::printf("latency_us p50=%u p90=%u p99=%u p999=%u max=%u\n", percentile(50),
              percentile(90), percentile(99), percentile(99.9), percentile(100));

  if (!cfg.out.empty()) {
    std::ofstream out(cfg.out);
    out << std::format(
        "{{\"requests\": {}, \"errors\": {}, \"duration_s\": {:.3f}, \"rps\": {:.1f}, "
        "\"latency_us\": {{\"p50\": {}, \"p90\": {}, \"p99\": {}, \"p999\": {}, \"max\": {}}}}}\n",
        requests, errors, elapsed, rps, percentile(50), percentile(90), percentile(99),
        percentile(99.9), percentile(100));
  }
  return 0;
}

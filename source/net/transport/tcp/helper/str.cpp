#include "xsl/net/transport/tcp/context.h"
#include "xsl/net/transport/tcp/helper/def.h"
#include "xsl/net/transport/tcp/helper/str.h"
#include "xsl/wheel.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

TCP_HELPER_NAMESPACE_BEGIN

SendString::SendString(string&& data) : data_buffer() {
  this->data_buffer.emplace_back(xsl::move(data));
}
SendString::~SendString() {}
tcp::SendResult SendString::exec(tcp::SendContext& ctx) {
  SPDLOG_TRACE("start send");
  while (!this->data_buffer.empty()) {
    auto& data = this->data_buffer.front();
    ssize_t n = write(ctx.sfd, data.c_str(), data.size());
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SPDLOG_DEBUG("[write] send over");
      } else {
        SPDLOG_ERROR("[write] Failed to send data");
        return {false};
      }
    } else if (static_cast<size_t>(n) < data.size()) {
      data.erase(0, n);
      return {false};
    }
    SPDLOG_DEBUG("send {} bytes", n);
    this->data_buffer.pop_front();
  }
  return {true};
}
RecvString::RecvString(string& data) : data_buffer(data) {}
RecvString::~RecvString() {}

tcp::RecvResult RecvString::exec(tcp::RecvContext& ctx) {
  SPDLOG_TRACE("start recv string");
  vector<string> data;
  char buf[MAX_SINGLE_RECV_SIZE];
  ssize_t n;
  do {
    n = ::recv(ctx.sfd, buf, sizeof(buf), 0);
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        if (data.empty()) {
          SPDLOG_DEBUG("recv eof");
          return {tcp::RecvError::RECV_EOF};
        }
        SPDLOG_DEBUG("recv over");
        break;
      } else {
        SPDLOG_ERROR("Failed to recv data, err : {}", strerror(errno));
        // TODO: handle recv error
        return {tcp::RecvError::UNKNOWN};
      }
    } else if (n == 0) {
      SPDLOG_DEBUG("recv eof");
      return {tcp::RecvError::RECV_EOF};
      break;
    }
    data.emplace_back(buf, n);
  } while (n == sizeof(buf));
  this->data_buffer = accumulate(data.begin(), data.end(), string());
  SPDLOG_DEBUG("data size: {}", this->data_buffer.size());
  return {false};
}

TCP_HELPER_NAMESPACE_END

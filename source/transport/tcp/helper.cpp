#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstddef>
#include <numeric>

#include "xsl/transport/tcp/helper.h"
#include "xsl/transport/tcp/tcp.h"
#include "xsl/utils/wheel/wheel.h"
TCP_NAMESPACE_BEGIN
// TODO: add FileHeaderGenerator
// FileInfor::FileInfor(size_t size) : size(size) {}
//  class ReadySendFile {
//    public:
//      ReadySendFile(wheel::string&& path, FileHeaderGenerator&& generator);
//      ~ReadySendFile();
//      bool exec(SendContext& ctx);

//   private:
//     wheel::string path;
//     FileHeaderGenerator header_gen;
// };

SendFile::SendFile(wheel::string&& path) : path_buffer({wheel::move(path)}) {}
SendFile::~SendFile() {}
bool SendFile::exec(SendContext& ctx) {
  while (true) {
    int ffd = open(this->path_buffer.front().c_str(), O_RDONLY);
    if (ffd == -1) {
      spdlog::error("open file failed");
    }
    struct stat st;
    if (fstat(ffd, &st) == -1) {
      close(ffd);
      spdlog::error("fstat failed");
    }
    off_t offset = 0;
    ssize_t n = sendfile(ctx.sfd, ffd, &offset, st.st_size);
    // TODO: handle sendfile error
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        spdlog::debug("[sendfile] send over");
      } else {
        spdlog::error("[sendfile] Failed to send file");
        close(ffd);
        return false;
      }
    } else if (n < st.st_size) {
      spdlog::debug("[sendfile] send {} bytes", n);
      close(ffd);
      return false;
    }
    close(ffd);
    this->path_buffer.pop_front();
    if (this->path_buffer.empty()) {
      break;
    }
  }
  return true;
}
wheel::unique_ptr<SendTaskNode> SendString::create(wheel::string&& data) {
  return wheel::make_unique<SendString>(wheel::move(data));
}
SendString::SendString(wheel::string&& data) { this->data_buffer.emplace_back(wheel::move(data)); }
SendString::~SendString() {}
bool SendString::exec(SendContext& ctx) {
  spdlog::trace("[SendString::exec]");
  while (!this->data_buffer.empty()) {
    auto& data = this->data_buffer.front();
    ssize_t n = write(ctx.sfd, data.c_str(), data.size());
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        spdlog::debug("[write] send over");
      } else {
        spdlog::error("[write] Failed to send data");
        return false;
      }
    } else if (static_cast<size_t>(n) < data.size()) {
      data.erase(0, n);
      return false;
    }
    spdlog::debug("[SendString::exec] send {} bytes", n);
    this->data_buffer.pop_front();
  }
  return true;
}
wheel::unique_ptr<RecvTaskNode> RecvString::create(wheel::string& data) {
  return wheel::make_unique<RecvString>(data);
}

RecvString::RecvString(wheel::string& data) : data_buffer(data) {}
RecvString::~RecvString() {}

bool RecvString::exec(RecvContext& ctx) {
  spdlog::trace("[RecvString::exec]");
  wheel::vector<wheel::string> data;
  char buf[MAX_SINGLE_RECV_SIZE];
  ssize_t n;
  do {
    n = ::recv(ctx.sfd, buf, sizeof(buf), 0);
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        spdlog::debug("[RecvString::exec] recv over");
        break;
      } else {
        spdlog::error("[RecvString::exec] Failed to recv data, err : {}", strerror(errno));
        // TODO: handle recv error
        return false;
      }
    } else if (n == 0) {
      spdlog::debug("[RecvString::exec] recv over");
      break;
    }
    data.emplace_back(buf, n);
  } while (n == sizeof(buf));
  this->data_buffer = std::accumulate(data.begin(), data.end(), wheel::string());
  spdlog::debug("[RecvString::exec] data size: {}", this->data_buffer.size());
  return true;
}

TCP_NAMESPACE_END

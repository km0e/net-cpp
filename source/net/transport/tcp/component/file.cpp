#include "xsl/net/transport/tcp/component/def.h"
#include "xsl/net/transport/tcp/component/file.h"
#include "xsl/wheel.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

TCP_COMPONENTS_NAMESPACE_BEGIN

SendFile::SendFile(string&& path) : path_buffer({xsl::move(path)}) {}
SendFile::~SendFile() {}
SendResult SendFile::exec(SendContext& ctx) {
  while (true) {
    int ffd = open(this->path_buffer.front().c_str(), O_RDONLY);
    if (ffd == -1) {
      SPDLOG_ERROR("open file failed");
    }
    struct stat st;
    if (fstat(ffd, &st) == -1) {
      close(ffd);
      SPDLOG_ERROR("fstat failed");
    }
    off_t offset = 0;
    ssize_t n = sendfile(ctx.sfd, ffd, &offset, st.st_size);
    // TODO: handle sendfile error
    if (n == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SPDLOG_DEBUG("[sendfile] send over");
      } else {
        SPDLOG_ERROR("[sendfile] Failed to send file");
        close(ffd);
        return {SendError::Unknown};
      }
    } else if (n < st.st_size) {
      SPDLOG_DEBUG("[sendfile] send {} bytes", n);
      close(ffd);
      return {false};
    }
    close(ffd);
    this->path_buffer.pop_front();
    if (this->path_buffer.empty()) {
      break;
    }
  }
  return {true};
}
TCP_COMPONENTS_NAMESPACE_END

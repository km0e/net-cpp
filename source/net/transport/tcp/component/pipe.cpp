// #include "xsl/net/transport/tcp/context.h"
// #include "xsl/net/transport/tcp/helper/def.h"
// #include "xsl/net/transport/tcp/helper/pipe.h"

// #include <fcntl.h>
// #include <spdlog/spdlog.h>
// #include <sys/sendfile.h>
// #include <sys/socket.h>
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <unistd.h>

// TCP_HELPER_NAMESPACE_BEGIN

// SendPipe::SendPipe(int fd) : fd(fd) {}
// SendPipe::~SendPipe() {}
// // using splice to send data
// tcp::SendResult SendPipe::exec(tcp::SendContext& ctx) {
//   while (true) {
//     ssize_t n = splice(fd, nullptr, ctx.sfd, nullptr, MAX_SINGLE_PIPE_SIZE,
//                        SPLICE_F_MORE | SPLICE_F_MOVE);
//     if (n < 0) {
//       if (errno == EAGAIN) {
//         // TODO handle EAGAIN
//         return {false};
//       }
//       return {tcp::SendError::UNKNOWN};
//     } else if (n == 0) {
//       break;
//     }
//   }
//   return {true};
// }
// RecvPipe::RecvPipe(int fd) : fd(fd) {}
// RecvPipe::~RecvPipe() {}
// // using splice to receive data
// tcp::RecvResult RecvPipe::exec(tcp::RecvContext& ctx) {
//   while (true) {
//     ssize_t n = splice(ctx.sfd, nullptr, fd, nullptr, MAX_SINGLE_PIPE_SIZE,
//                        SPLICE_F_MORE | SPLICE_F_MOVE);
//     if (n < 0) {
//       if (errno == EAGAIN) {
//         // TODO handle EAGAIN
//         return {true};
//       }
//       return {tcp::RecvError::UNKNOWN};
//     } else if (n == 0) {
//       break;
//     }
//   }
//   return {false};
// }
// TCP_HELPER_NAMESPACE_END

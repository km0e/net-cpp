// #include "xsl/net/sync/poller.h"
// #include "xsl/net/transport/tcp/conn.h"
// #include "xsl/net/transport/tcp/context.h"
// #include "xsl/net/transport/tcp/def.h"
// #include "xsl/wheel.h"

// TCP_NAMESPACE_BEGIN
// TcpConn::TcpConn(int fd, RecvTasks&& recv_tasks, SendTasks&& send_tasks)
//     : fd(fd), recv_tasks(xsl::move(recv_tasks)), send_tasks(xsl::move(send_tasks)) {}

// TcpConn::~TcpConn() {
//   if (this->fd != -1) {
//     close(this->fd);
//   }
// }
// sync::PollHandleHint TcpConn::operator()(int fd, IOM_EVENTS events) {
//   if (events == IOM_EVENTS::IN) {
//     this->recv(fd, events);
//   } else if (events == IOM_EVENTS::OUT) {
//     this->send(fd, events);
//   }
//   return sync::PollHandleHint::NONE;
// }
// void TcpConn::send(int fd, IOM_EVENTS events) {
//   (void)events;
//   SPDLOG_TRACE("start send");
//   SendTasks hl;
//   auto state = this->handler.send(hl);
//   // move new tasks to this->tasks tail
//   hl.splice_after(hl.before_begin(), this->send_tasks);
//   this->send_tasks = std::move(hl);
//   SendContext ctx(fd, this->send_tasks);
//   while (!this->send_tasks.empty()) {
//     auto res = this->send_tasks.front()->exec(ctx);
//     if (res.is_ok()) {
//       if (!res.unwrap()) {
//         break;
//       }
//       this->send_tasks.pop_front();
//     } else {
//       // TODO: handle send error
//       SPDLOG_ERROR("send error");
//     }
//   }
//   if (state.events != this->events) {
//     this->poller->modify(fd, state.events);
//   }
//   this->events = state.events;
// }
// void TcpConn::recv(int fd, IOM_EVENTS events) {
//   (void)events;
//   SPDLOG_TRACE("start recv");
//   if (this->recv_tasks.empty()) {
//     SPDLOG_ERROR("No recv task found");
//     // this->events = IOM_EVENTS::NONE;
//     // this->poller->unregister(fd);
//     return;
//   }
//   RecvContext ctx(fd, this->recv_tasks);
//   while (true) {
//     auto res = ctx.iter->get()->exec(ctx);
//     if (res.is_ok()) {
//       if (!res.unwrap()) {
//         break;
//       }
//       ctx.iter++;
//       if (ctx.iter == this->recv_tasks.end()) {
//         ctx.iter = this->recv_tasks.begin();
//       }
//     } else {
//       switch (res.unwrap_err()) {
//         case RecvError::RECV_EOF:
//           SPDLOG_DEBUG("recv eof");
//           this->poller->unregister(fd);
//           this->events = IOM_EVENTS::NONE;
//           close(fd);
//           fd = -1;
//           return;
//         case RecvError::UNKNOWN:
//           SPDLOG_ERROR("recv error");
//           this->poller->unregister(fd);
//           this->events = IOM_EVENTS::NONE;
//           close(fd);
//           fd = -1;
//           return;
//         default:
//           break;
//       }
//     }
//   }
//   SPDLOG_DEBUG("recv all data");
//   auto state = this->handler.recv(this->recv_tasks);
//   if ((state.events & IOM_EVENTS::OUT) == IOM_EVENTS::OUT) {
//     this->send(fd, IOM_EVENTS::OUT);
//   } else if (state.events != this->events) {
//     this->poller->modify(fd, state.events);
//     this->events = state.events;
//   }
//   SPDLOG_TRACE("recv done");
// }
// TCP_NAMESPACE_END

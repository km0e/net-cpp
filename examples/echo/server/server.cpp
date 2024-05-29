// #include "xsl/transport/tcp/server.h"
// #include "xsl/wheel/mutex.h"
// #include "xsl/wheel/wheel.h"
// use absl for threadpool
// #include <absl/synchronization/internal/thread_pool.h>

// auto echo_buffer = xsl::wheel::make_shared<xsl::wheel::queue<xsl::wheel::string>>();
// auto buffer_mutex = xsl::wheel::make_shared<xsl::wheel::Mutex>();
// auto stop = xsl::wheel::atomic_flag();

// bool server_read(wheel::shared_ptr<Poller> poller, int fd, IOM_EVENTS events) {
//   if(events & IOM_EVENTS::IN) {
//     wheel::string buffer;
//     buffer.resize(1024);
//     int pos = 0;
//     while(true) {
//       ssize_t bytes = read(fd, buffer.data() + pos, buffer.size() - pos);
//       if(bytes == -1) {
//         if(errno == EAGAIN || errno == EWOULDBLOCK) {
//           break;
//         }
//         stop.test_and_set();
//         return false;
//       } else if(bytes == 0) {
//         break;
//       }
//       pos += bytes;
//       buffer.resize(pos + 1024);
//     }
//     buffer.resize(pos);
//     buffer_mutex->lock();
//     echo_buffer->push(buffer);
//     buffer_mutex->unlock();
//   }
//   return true;
// }

// bool server_write(wheel::shared_ptr<Poller> poller, int fd, IOM_EVENTS events) {
//   if(events & IOM_EVENTS::OUT) {
//     buffer_mutex->lock();
//     while(true) {
//       if(echo_buffer->empty()) {
//         break;
//       }
//       wheel::string buffer = echo_buffer->front();
//       echo_buffer->pop();
//       int pos = 0;
//       while(pos < buffer.size()) {
//         ssize_t bytes = write(fd, buffer.data() + pos, buffer.size() - pos);
//         if(bytes == -1) {
//           if(errno == EAGAIN || errno == EWOULDBLOCK) {
//             break;
//           }
//           stop.test_and_set();
//           buffer_mutex->unlock();
//           return false;
//         }
//         pos += bytes;
//       }
//     }
//   }
//   return true;
// }

int main() {
  // auto thread_pool = absl::synchronization_internal::ThreadPool(10);
  // wheel::shared_ptr<Poller> poller = wheel::make_shared<Poller>();
  // TcpServer server("127.0.0.1", 8080);
  // server.set_handler([](wheel::shared_ptr<Poller> poller, int fd, IOM_EVENTS events) {
  //   poller->register_handler(fd, IOM_EVENTS::IN, [fd](int in_fd, IOM_EVENTS events) -> bool {
  //     if(events & IOM_EVENTS::IN) {
  //     }
  //     return true;
  //   });
  // });
}

#include <poller.h>
#include <sys/signal.h>
Poller::Poller() {
  this->fd = epoll_create(1);
}
bool Poller::valid() {
  return this->fd != -1;
}
bool Poller::register_handler(int fd, IOM_EVENTS events, PollHandler handler) {
  epoll_event event;
  event.events = events;
  event.data.fd = fd;
  if(epoll_ctl(this->fd, EPOLL_CTL_ADD, fd, &event) == -1) {
    return false;
  }
  return true;
}
void Poller::poll() {
  epoll_event events[10];
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGQUIT);
  int n = epoll_pwait(this->fd, events, 10, -1, &mask);
  for(int i = 0; i < n; i++) {
    if(!this->handlers[events[i].data.fd](events[i].data.fd, (IOM_EVENTS)events[i].events)) {
      this->unregister(events[i].data.fd);
    }
  }
}
void Poller::unregister(int fd) {
  epoll_ctl(this->fd, EPOLL_CTL_DEL, fd, nullptr);
}

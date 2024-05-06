#pragma once
#ifndef _XSL_NET_H_
#  define _XSL_NET_H_
#  include <netinet/in.h>
#  include <sys/epoll.h>
#  include <sys/signal.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <wheel.h>
#  define USE_EPOLL
class Url {};

class Header {};

class Request {
public:
  Url url;
  Header header;
};

#endif  // _XSL_NET_H_

#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  define _XSL_NET_TRANSPORT_TCP_SERVER_H_
#  include <xsl/sync/poller.h>
#  include <xsl/transport/transport.h>
#  include <xsl/transport/utils.h>
#  include <xsl/utils/wheel/wheel.h>
TRANSPORT_NAMESPACE_BEGIN

enum class HandleHint {
  NONE = 0,
  // Hint that the param data is a pointer to a string that should be sent
  WRITE = 2,
};

class HandleState {
public:
  HandleState();
  HandleState(sync::IOM_EVENTS events, HandleHint hint);
  ~HandleState();
  sync::IOM_EVENTS events;
  HandleHint hint;
};

using Handler = wheel::function<HandleState(int read_write, wheel::string& data)>;
class TcpConn {
public:
  TcpConn(int fd, wheel::shared_ptr<sync::Poller> poller, Handler&& handler);
  TcpConn(TcpConn&&) = default;
  ~TcpConn();
  sync::IOM_EVENTS send();
  sync::IOM_EVENTS recv();

private:
  int fd;
  wheel::shared_ptr<sync::Poller> poller;
  Handler handler;
  wheel::string send_buffer;
};

class HandlerGenerator {
public:
  virtual Handler operator()() = 0;
};

class TcpServer {
public:
  TcpServer(TcpConfig config = {});
  ~TcpServer();
  bool serve(const char* ip, int port);
  bool valid();
  void set_max_connections(int max_connections);
  // Handler is a function that takes a shared pointer to a Poller, an int, and an IOM_EVENTS enum
  // and returns a bool The handler is called when the server receives a connection
  void set_handler_generator(wheel::shared_ptr<HandlerGenerator> handler_generator);
  void set_poller(wheel::shared_ptr<sync::Poller> poller);

private:
  sync::IOM_EVENTS proxy_handler(int fd, sync::IOM_EVENTS events);
  sync::IOM_EVENTS accept_handler(int fd, sync::IOM_EVENTS events);

  int server_fd;
  TcpConfig config;
  // Handler is a function that takes a shared pointer to a Poller, an int, and an IOM_EVENTS enum
  // and returns a bool The handler is called when the server receives a connection
  wheel::shared_ptr<HandlerGenerator> handler_generator;
  wheel::unordered_map<int, TcpConn> handlers;
  wheel::shared_ptr<sync::Poller> poller;
};
TRANSPORT_NAMESPACE_END
#endif

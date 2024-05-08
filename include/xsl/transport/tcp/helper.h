#pragma once
#include "xsl/utils/wheel/wheel.h"
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  include <xsl/transport/tcp/tcp.h>
TCP_NAMESPACE_BEGIN
class SendFile : public TaskNode {
public:
  SendFile(wheel::string&& path);
  ~SendFile();
  bool exec(int fd) override;

private:
  wheel::list<wheel::string> path_buffer;
};

class SendString : public TaskNode {
public:
  static wheel::unique_ptr<TaskNode> create(wheel::string&& data);
  SendString(wheel::string&& data);
  SendString(SendString&&) = default;
  ~SendString();
  bool exec(int fd) override;

private:
  wheel::list<wheel::string> data_buffer;
};
const size_t MAX_SINGLE_RECV_SIZE = 1024;
class RecvString : public TaskNode {
public:
  static wheel::unique_ptr<TaskNode> create(wheel::string& data);
  RecvString(RecvString&&) = default;
  RecvString(wheel::string& data);
  ~RecvString();
  bool exec(int fd) override;

  wheel::string& data_buffer;
};
TCP_NAMESPACE_END
#endif

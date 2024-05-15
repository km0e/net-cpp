#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  include "xsl/net/transport/tcp/context.h"
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/wheel.h"
TCP_NAMESPACE_BEGIN
// TODO: add FileHeaderGenerator
// class FileInfor {
// public:
//   FileInfor(size_t size);
//   size_t size;
// };
// using FileHeaderGenerator = wheel::function<wheel::string(const FileInfor&)>;

class SendFile : public SendTaskNode {
public:
  SendFile(string&& path);
  ~SendFile();
  bool exec(SendContext& ctx) override;

protected:
  list<string> path_buffer;
};

class SendString : public SendTaskNode {
public:
  SendString(string&& data);
  SendString(SendString&&) = default;
  ~SendString();
  bool exec(SendContext& ctx) override;

private:
  list<string> data_buffer;
};
const size_t MAX_SINGLE_RECV_SIZE = 1024;
class RecvString : public RecvTaskNode {
public:
  static unique_ptr<RecvString> create(string& data);
  RecvString(RecvString&&) = default;
  RecvString(string& data);
  ~RecvString();
  RecvResult exec(RecvContext& ctx) override;

private:
  string& data_buffer;
};
TCP_NAMESPACE_END
#endif

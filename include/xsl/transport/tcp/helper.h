#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  include <xsl/transport/tcp/context.h>
#  include <xsl/transport/tcp/def.h>

#  include "xsl/utils/wheel/wheel.h"
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
  SendFile(
      wheel::string&& path);
  ~SendFile();
  bool exec(SendContext& ctx) override;

protected:
  wheel::list<wheel::string> path_buffer;
};

class SendString : public SendTaskNode {
public:
  static wheel::unique_ptr<SendTaskNode> create(wheel::string&& data);
  SendString(wheel::string&& data);
  SendString(SendString&&) = default;
  ~SendString();
  bool exec(SendContext& ctx) override;

private:
  wheel::list<wheel::string> data_buffer;
};
const size_t MAX_SINGLE_RECV_SIZE = 1024;
class RecvString : public RecvTaskNode {
public:
  static wheel::unique_ptr<RecvTaskNode> create(wheel::string& data);
  RecvString(RecvString&&) = default;
  RecvString(wheel::string& data);
  ~RecvString();
  bool exec(RecvContext& ctx) override;

private:
  wheel::string& data_buffer;
};
TCP_NAMESPACE_END
#endif

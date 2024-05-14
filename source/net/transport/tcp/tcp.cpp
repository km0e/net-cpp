#include "xsl/net/transport/tcp/conn.h"
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
TCP_NAMESPACE_BEGIN
HandleConfig::HandleConfig() : recv_tasks() {}
HandleConfig::~HandleConfig() {}
TCP_NAMESPACE_END

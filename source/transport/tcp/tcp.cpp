#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xsl/transport/tcp/conn.h>

#include "xsl/transport/tcp/tcp.h"
TCP_NAMESPACE_BEGIN
HandleConfig::HandleConfig() {}
HandleConfig::~HandleConfig() {}
TCP_NAMESPACE_END

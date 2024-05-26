#pragma once
#ifndef _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  define _XSL_NET_TRANSPORT_TCP_HELPER_H_
#  include "xsl/net/transport/tcp/def.h"
#  include "xsl/net/transport/tcp/helper/file.h"
#  include "xsl/net/transport/tcp/helper/str.h"
TCP_NAMESPACE_BEGIN
using helper::RecvString;
using helper::SendFile;
using helper::SendString;
TCP_NAMESPACE_END
#endif

#pragma once
#ifndef _XSL_SYNC_H_
#  define _XSL_SYNC_H_
#  include "xsl/net/def.h"
#  include "xsl/net/sync/poller.h"
NET_NAMESPACE_BEGIN
using sync::detail::DefaultPoller;
using sync::detail::IOM_EVENTS;
using sync::detail::Poller;
NET_NAMESPACE_END
#endif

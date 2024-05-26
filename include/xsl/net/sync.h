#pragma once
#ifndef _XSL_SYNC_H_
#  define _XSL_SYNC_H_
#  include "xsl/net/def.h"
#  include "xsl/net/sync/poller.h"
NET_NAMESPACE_BEGIN
using sync::DefaultPoller;
using sync::IOM_EVENTS;
using sync::Poller;
using sync::PollHandler;
using sync::sub_shared;
using sync::sub_unique;
NET_NAMESPACE_END
#endif

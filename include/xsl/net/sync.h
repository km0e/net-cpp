#pragma once
#ifndef _XSL_SYNC_H_
#  define _XSL_SYNC_H_
#  include "xsl/net/def.h"
#  include "xsl/net/sync/poller.h"
NET_NAMESPACE_BEGIN
using sync::Poller;
using sync::IOM_EVENTS;
using sync::poll_add_shared;
using sync::poll_add_unique;
using sync::Poller;
using sync::PollHandleHint;
using sync::PollHandleHintTag;
using sync::PollHandler;
NET_NAMESPACE_END
#endif

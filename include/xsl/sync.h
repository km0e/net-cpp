#pragma once
#ifndef XSL_SYNC
#  define XSL_SYNC
#  include "xsl/def.h"
#  include "xsl/sync/mutex.h"
#  include "xsl/sync/poller.h"
XSL_NB
using sync::IOM_EVENTS;
using sync::LockGuard;
using sync::poll_add_shared;
using sync::poll_add_unique;
using sync::Poller;
using sync::PollHandleHint;
using sync::PollHandleHintTag;
using sync::PollHandler;
using sync::ShrdGuard;
using sync::ShrdRes;
XSL_NE
#endif

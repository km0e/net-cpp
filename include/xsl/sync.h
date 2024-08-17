#pragma once
#ifndef XSL_SYNC
#  define XSL_SYNC
#  include "xsl/def.h"
#  include "xsl/sync/mutex.h"
#  include "xsl/sync/spsc.h"
XSL_NB
using sync::LockGuard;
using sync::ShardGuard;
using sync::ShardRes;
using sync::SPSC;
XSL_NE
#endif

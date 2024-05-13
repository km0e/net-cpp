#pragma once
#ifndef _XSL_SYNC_H_
#  define _XSL_SYNC_H_
#include "xsl/sync/poller.h"
namespace xsl::sync {
  using detail::DefaultPoller;
  using detail::Poller;
  using detail::IOM_EVENTS;
}  // namespace xsl::sync
#endif

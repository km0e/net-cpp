#include "xsl/net/sync/poller.h"
#include "xsl/net/transport/tcp/conn.h"

#include <spdlog/spdlog.h>

#define MAX_CONNECTIONS 10

TCP_NAMESPACE_BEGIN

HandleState::HandleState(IOM_EVENTS events, HandleHint hint) : events(events), hint(hint) {}
HandleState::HandleState() : events(IOM_EVENTS::NONE), hint(HandleHint::NONE) {}
HandleState::~HandleState() {}

TCP_NAMESPACE_END

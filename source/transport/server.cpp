#include <spdlog/spdlog.h>
#include <xsl/sync/poller.h>
#include <xsl/transport/server.h>
#include <xsl/transport/transport.h>
#include <xsl/transport/utils.h>
#include <xsl/utils/utils.h>

#define MAX_CONNECTIONS 10

TRANSPORT_NAMESPACE_BEGIN

HandleState::HandleState(sync::IOM_EVENTS events, HandleHint hint) : events(events), hint(hint) {}
HandleState::HandleState() : events(sync::IOM_EVENTS::NONE), hint(HandleHint::NONE) {}
HandleState::~HandleState() {}


TRANSPORT_NAMESPACE_END

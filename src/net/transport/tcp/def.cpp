#include <xsl/net/transport/tcp/def.h>
TCP_NAMESPACE_BEGIN
Socket::Socket(int sfd) : sfd(sfd) {}
Socket::~Socket() { close(sfd); }
int Socket::raw_fd() const { return sfd; }

TCP_NAMESPACE_END
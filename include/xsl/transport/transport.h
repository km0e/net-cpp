#pragma once
#ifndef _XSL_NET_TRANSPORT_H_
#define _XSL_NET_TRANSPORT_H_
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#define TRANSPORT_NAMESPACE_BEGIN namespace transport {
#define TRANSPORT_NAMESPACE_END }

#define MAX_CONNECTIONS 10

#endif  // _XSL_NET_TRANSPORT_H_
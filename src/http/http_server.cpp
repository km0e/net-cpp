#include <xsl/config.h>
#include <xsl/http/http_server.h>
XSL_NAMESPACE_BEGIN
HttpServer::HttpServer() {
}
HttpServer::~HttpServer() {
}
void HttpServer::serve(const char *ip, int port) {
  tcpServer.serve(ip, port);
}
void HttpServer::setMaxConnections(int maxConnections) {
  // tcpServer.setMaxConnections(maxConnections);
}
void HttpServer::setMaxThreads(int maxThreads) {
  // tcpServer.setMaxThreads(maxThreads);
}
XSL_NAMESPACE_END
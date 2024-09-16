import socketserver
import threading

default_port = 12345


def start_echo_server(host):
    port = default_port

    class TcpEchoHandler(socketserver.BaseRequestHandler):

        def handle(self):
            while True:
                data = self.request.recv(1024)
                if not data:
                    break
                print(f"Tcp Received: {data}")
                self.request.sendall(data)

    class UdpEchoHandler(socketserver.BaseRequestHandler):

        def handle(self):
            data, socket = self.request
            print(f"Udp Received: {data}")
            socket.sendto(data, self.client_address)

    while True:
        try:
            tcp_server = socketserver.TCPServer((host, port), TcpEchoHandler)
            udp_server = socketserver.UDPServer((host, port), UdpEchoHandler)
        except OSError:
            port += 1
        else:
            break
    tcp_server_thread = threading.Thread(target=tcp_server.serve_forever)
    tcp_server_thread.daemon = True
    tcp_server_thread.start()
    udp_server_thread = threading.Thread(target=udp_server.serve_forever)
    udp_server_thread.daemon = True
    udp_server_thread.start()
    return str(port), tcp_server_thread, udp_server_thread

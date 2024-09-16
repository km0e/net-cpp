from echo import start_echo_server

port, tcp, udp = start_echo_server('127.0.0.1')

print(port)

tcp.join()
udp.join()

import os
import sys
import subprocess
import time

test_server = sys.argv[1]
echo_client = sys.argv[2]
test_ip = sys.argv[3]
test_port = sys.argv[4]

# Start the echo server
print("Starting echo server: %s -i %s -p %s" %
      (test_server, test_ip, test_port))
server_res = subprocess.Popen([test_server, "-i", test_ip, "-p", test_port],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
# Sleep 100ms to give the echo server time to start
time.sleep(0.1)
# Run the test program
client_res = subprocess.Popen([echo_client, "-i", test_ip, "-p", test_port],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)

# Wait for the test program to finish
client_res.wait()

# Kill the echo server

server_res.terminate()
server_res.wait()

# Check the test program's output

if client_res.returncode != 0 or server_res.returncode != 0:
    print("Test failed: client", client_res.returncode, "server",
          server_res.returncode)
    print("Client output:")
    print(str(client_res.stdout.read(), 'utf-8'))
    print("Client error:")
    print(str(client_res.stderr.read(), 'utf-8'))
    print("Server output:")
    print(str(server_res.stdout.read(), 'utf-8'))
    print("Server error:")
    print(str(server_res.stderr.read(), 'utf-8'))
    sys.exit(1)
else:
    print("Test passed")
    sys.exit(0)

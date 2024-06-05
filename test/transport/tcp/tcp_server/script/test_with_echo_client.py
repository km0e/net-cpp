import os
import sys
import subprocess
import time

ip = sys.argv[1]
port = sys.argv[2]

client = sys.argv[3]
test_programs = sys.argv[4:]

for test_program in test_programs:
    if not os.path.exists(test_program):
        print("Test program %s does not exist" % test_program)
        sys.exit(1)
    server = subprocess.Popen([test_program, "-i", ip, "-p", port],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
    time.sleep(0.1)
    client = subprocess.Popen([client, "-i", ip, "-p", port],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
    client.wait()
    server.terminate()
    server.wait()
    if client.returncode != 0 or server.returncode != 0:
        print("Test failed")
        print("Test Server status: %d, output:" % server.returncode)
        print(str(server.stdout.read(), 'utf-8'))
        print(str(server.stderr.read(), 'utf-8'))
        print("Test Client output: %d, output:" % client.returncode)
        print(str(client.stdout.read(), 'utf-8'))
        print(str(client.stderr.read(), 'utf-8'))
        sys.exit(1)

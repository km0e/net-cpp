import os
import sys
import subprocess
import time

ip = sys.argv[1]
port = sys.argv[2]
ncat = sys.argv[3]
cat = sys.argv[4]

test_programs = sys.argv[5:]

for test_program in test_programs:
    if not os.path.exists(test_program):
        print("Test program %s does not exist" % test_program)
        sys.exit(1)
    pipe = os.pipe()
    server = subprocess.Popen([ncat, '-l', port, '-c', cat],
                              stdin=pipe[0],
                              stdout=pipe[1])
    time.sleep(0.1)
    client = subprocess.Popen([test_program, "-i", ip, "-p", port],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
    client.wait()
    server.terminate()
    server.wait()
    if client.returncode != 0:
        print("Test failed")
        print("Test output:")
        print(str(client.stdout.read(), 'utf-8'))
        print(str(client.stderr.read(), 'utf-8'))
        sys.exit(0)

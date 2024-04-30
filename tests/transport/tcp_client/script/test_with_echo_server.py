import os
import sys
import subprocess
import time

test_program = sys.argv[1]
test_host = sys.argv[2]
test_port = sys.argv[3]
ncat = sys.argv[4]
cat = sys.argv[5]

# Start the echo server
print("Starting echo server: ncat -l %s -c %s" % (test_port, cat))
echo_server = subprocess.Popen([ncat, '-l', test_port, '-c', cat],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)
# Sleep 100ms to give the echo server time to start
time.sleep(0.1)
# Run the test program
test = subprocess.Popen([test_program, "-h", test_host, "-p", test_port],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE)

# Wait for the test program to finish
test.wait()

# Kill the echo server

echo_server.terminate()
echo_server.wait()

# Check the test program's output

if test.returncode != 0:
    print("Test failed")
    print("Test output:")
    print(str(test.stdout.read(), 'utf-8'))
    print(str(test.stderr.read(), 'utf-8'))
    sys.exit(1)
else:
    print("Test passed")
    sys.exit(0)

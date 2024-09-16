import os
import sys
import subprocess
import argparse
from echo import start_echo_server


def parse_args():
    parser = argparse.ArgumentParser(
        description='Run a test program with an echo server')
    parser.add_argument('test_programs',
                        nargs='+',
                        help='Test programs to run')
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()

    port, _, _ = start_echo_server('127.0.0.1')

    for test_program in args.test_programs:
        if not os.path.exists(test_program):
            print("Test program %s does not exist" % test_program)
            continue
        print("Run %s -i 127.0.0.1 -p %s" % (test_program, port))
        client = subprocess.Popen(
            [test_program, "-i", "127.0.0.1", "-p", port],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
        client.wait()
        if client.returncode != 0:
            print("Test failed",file=sys.stderr)
            print("Test output:",file=sys.stderr)
            print(str(client.stdout.read(), 'utf-8'),file=sys.stderr)
            print(str(client.stderr.read(), 'utf-8'),file=sys.stderr)

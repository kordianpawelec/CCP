import socket
import subprocess
import time


HOST = "127.0.0.1"
PORT = 19000


def main():
    server = subprocess.Popen(
        ["./build/cci-server", str(PORT)]
    )

    try:
        time.sleep(0.2)

        alice = socket.create_connection((HOST, PORT))
        bob = socket.create_connection((HOST, PORT))

        try:
            #
            # TODO:
            #
            # register Alice
            # register Bob
            #
            # Alice SEND Bob
            #
            # Bob FETCH
            #
            # Bob disconnect before ACK
            #
            # reconnect Bob
            #
            # Bob FETCH
            #
            # verify SAME message
            #
            # ACK
            #

            pass

        finally:
            alice.close()
            bob.close()

    finally:
        server.terminate()
        server.wait(timeout=2)


if __name__ == "__main__":
    main()
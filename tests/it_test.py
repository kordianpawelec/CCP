import socket
import subprocess
import time
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parent.parent

sys.path.insert(0, str(ROOT))

from client.cci_protoco import *


HOST = "127.0.0.1"
PORT = 19000


def request(sock, message_type, payload=b""):
    send_frame(sock, message_type, payload)
    return recv_frame(sock)

def main():
    server = subprocess.Popen([str(ROOT / "build" / "cci-server"), str(PORT)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    try:
        time.sleep(0.3)
        alice = socket.create_connection((HOST, PORT), timeout=2)
        bob = socket.create_connection((HOST, PORT), timeout=2)


        # Register Alice
        msg_type, payload = request(alice, CCI_REGISTER, build_register("alice"))
        assert msg_type == CCI_OK


        # Register Bob
        msg_type, payload = request(bob, CCI_REGISTER, build_register("bob"))
        assert msg_type == CCI_OK


        # Alice sends to Bob        
        msg_type, payload = request(alice, CCI_SEND, build_send("bob", b"hello bob",))
        assert msg_type == CCI_OK
        message_id = int.from_bytes(payload[2:10], "big")


        # Bob fetches
        msg_type, payload = request(bob, CCI_FETCH)
        assert msg_type == CCI_DELIVERY
        first_id = int.from_bytes(payload[0:8], "big")
        assert first_id == message_id


        # Bob disconnects without ACK.
        bob.close()
        time.sleep(0.1)


        # Bob reconnects.
        bob = socket.create_connection((HOST, PORT), timeout=2)
        msg_type, payload = request(bob, CCI_REGISTER, build_register("bob"))
        assert msg_type == CCI_OK


        # Fetch should redeliver same message.
        msg_type, payload = request(bob, CCI_FETCH)
        assert msg_type == CCI_DELIVERY
        second_id = int.from_bytes(payload[0:8], "big")
        assert second_id == first_id


        # ACK.
        msg_type, payload = request(bob, CCI_ACK, build_ack(second_id),)
        assert msg_type == CCI_OK


        print("Integration test passed")
        alice.close()
        bob.close()

    finally:
        server.terminate()
        try:
            server.wait(timeout=2)
        except subprocess.TimeoutExpired:
            server.kill()


if __name__ == "__main__":
    main()
import socket
import sys

from cci_protoco import *


def print_response(message_type, payload):
    if message_type == CCI_OK:
        operation = int.from_bytes(payload[0:2], "big")
        value = int.from_bytes(payload[2:10], "big")
        print(f"OK operation={operation} value={value}")


    elif message_type == CCI_ERROR:
        error = int.from_bytes(payload[0:2],"big")
        print(f"ERROR code={error}")


    elif message_type == CCI_DELIVERY:
        message_id = int.from_bytes(payload[0:8],"big")
        sender_length = payload[8]
        sender = payload[9:9 + sender_length].decode("utf-8")
        body = payload[9 + sender_length:]
        print(f"MESSAGE id={message_id} " f"from={sender}: " f"{body.decode('utf-8', errors='replace')}")


def handle_command(sock: socket.socket, command: str) -> bool:

    parts = command.split(" ", 2)
    if not parts:
        return True

    if parts[0] == "register":
        if len(parts) != 2:
            print("usage: register <username>")
            return True

        send_frame(sock, CCI_REGISTER, build_register(parts[1]))


    elif parts[0] == "send":
        if len(parts) != 3:
            print("usage: send <user> <message>")
            return True

        send_frame(sock, CCI_SEND, build_send(parts[1], parts[2].encode("utf-8")))

    elif parts[0] == "fetch":
        send_frame(sock, CCI_FETCH, build_fetch())

    elif parts[0] == "ack":

        if len(parts) != 2:
            print("usage: ack <message_id>")
            return True

        send_frame(sock, CCI_ACK, build_ack(int(parts[1])),)


    elif parts[0] == "quit":
        return False

    else:
        print("commands: ""register, send, fetch, ack, quit")
        return True


    message_type, payload = recv_frame(sock)

    print_response(message_type, payload)

    return True


def main():
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} HOST PORT")
        raise SystemExit(1)

    host = sys.argv[1]
    port = int(sys.argv[2])

    with socket.create_connection((host, port)) as sock:

        print(f"Connected to {host}:{port}")

        while True:
            command = input("> ").strip()

            if not handle_command(sock, command):
                break


if __name__ == "__main__":
    main()
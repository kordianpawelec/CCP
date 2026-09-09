import socket
import sys

from cci_protocol import (
    CCI_ACK,
    CCI_FETCH,
    CCI_REGISTER,
    CCI_SEND,
    build_ack,
    build_fetch,
    build_register,
    build_send,
    parse_response,
    recv_frame,
    send_frame,
)


def handle_command(sock: socket.socket, command: str) -> bool:
    """
    TODO:

    Commands:

        register alice
        send bob hello
        fetch
        ack 123
        quit
    """

    return True


def main() -> None:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} HOST PORT")
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
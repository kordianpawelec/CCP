import socket
import struct


CCI_MAGIC = 0xCC
CCI_VERSION = 1

CCI_REGISTER = 1
CCI_SEND = 2
CCI_FETCH = 3
CCI_ACK = 4

CCI_OK = 100
CCI_DELIVERY = 101
CCI_ERROR = 102

HEADER_FORMAT = "!BBHI"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)


def recv_exact(sock: socket.socket, size: int) -> bytes:
    data = b""

    while len(data) < size:
        chunk = sock.recv(size - len(data))

        if not chunk:
            raise ConnectionError("Server disconnected")

        data += chunk

    return data


def send_frame(
    sock: socket.socket,
    message_type: int,
    payload: bytes = b"",
) -> None:
    header = struct.pack(
        HEADER_FORMAT,
        CCI_MAGIC,
        CCI_VERSION,
        message_type,
        len(payload),
    )

    sock.sendall(header + payload)


def recv_frame(sock: socket.socket):
    raw_header = recv_exact(sock, HEADER_SIZE)

    magic, version, message_type, payload_length = \
        struct.unpack(HEADER_FORMAT, raw_header)

    if magic != CCI_MAGIC:
        raise ValueError("Invalid CCI magic")

    if version != CCI_VERSION:
        raise ValueError("Unsupported CCI version")

    payload = recv_exact(sock, payload_length) \
        if payload_length > 0 else b""

    return message_type, payload


def build_register(username: str) -> bytes:
    return username.encode("utf-8")


def build_send(destination: str, body: bytes) -> bytes:
    destination_bytes = destination.encode("utf-8")

    if len(destination_bytes) > 255:
        raise ValueError("Destination too long")

    return (
        bytes([len(destination_bytes)])
        + destination_bytes
        + body
    )


def build_fetch() -> bytes:
    return b""


def build_ack(message_id: int) -> bytes:
    return message_id.to_bytes(
        8,
        byteorder="big",
        signed=False,
    )
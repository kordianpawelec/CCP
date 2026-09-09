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
    """Receive exactly size bytes or raise an error."""
    raise NotImplementedError


def send_frame(
    sock: socket.socket,
    message_type: int,
    payload: bytes = b"",
) -> None:
    """Serialize and send one CCI frame."""
    raise NotImplementedError


def recv_frame(sock: socket.socket):
    """Receive and decode one CCI frame."""
    raise NotImplementedError


def build_register(username: str) -> bytes:
    raise NotImplementedError


def build_send(destination: str, body: bytes) -> bytes:
    raise NotImplementedError


def build_fetch() -> bytes:
    raise NotImplementedError


def build_ack(message_id: int) -> bytes:
    raise NotImplementedError


def parse_response(message_type: int, payload: bytes):
    raise NotImplementedError
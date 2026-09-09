# CCI Relay — Approach

## Architecture
The solution is a single C TCP server using POSIX threads.
Each client connection is handled by a worker thread. The server keeps registered clients and their mailboxes in memory.

A small Python client is included to exercise the protocol and run integration tests.

## Protocol
CCI uses a custom binary protocol over TCP.
Each frame contains an 8-byte header:

- 1 byte magic
- 1 byte version
- 2 byte message type
- 4 byte payload length
Multi-byte values use network byte order.

Supported operations are:
- `REGISTER`
- `SEND`
- `FETCH`
- `ACK`

Server responses are:
- `OK`
- `DELIVERY`
- `ERROR`

Because TCP is a byte stream, `send_exact()` and `recv_exact()` are used to handle partial reads and writes.

## Message Delivery
Each registered client has a bounded FIFO mailbox.

Messages have two states:
```text
QUEUED
IN_FLIGHT
```


FETCH marks the oldest queued message as IN_FLIGHT.

A message is removed only after the recipient sends the correct ACK.

If the client disconnects before acknowledging, the message is changed back to QUEUED and is redelivered after reconnect.

This provides at-least-once delivery.

## Concurrency
The server uses:

- one mutex for shared client registry state
- one mutex per mailbox
- one worker thread per connection

Network I/O is not performed while mailbox locks are held.

Delivery is pull-based using FETCH, so a slow recipient does not block another client's SEND operation.

## Resource Limits
The implementation is intentionally bounded:

- 10 active TCP connections
- 32 registered clients
- 128 messages per mailbox
- 1024 byte message bodies
- 2048 byte protocol payloads

## Testing
The project includes:
state tests for registration, sending, ACKs, reconnect and redelivery
protocol tests for encoding, decoding and socket framing
an integration test using the real TCP server

Run all tests with:
`make test`

## Trade-offs / Limitations
For simplicity and time-boxing:

all state is stored in memory
there is no authentication or encryption
there is no durable persistence
only one message per mailbox may be in flight
worker shutdown is not fully coordinated

A production version would add persistent storage, coordinated shutdown, stronger validation and client idempotency support.

AI Usage

AI was used as a design and code-review aid for protocol framing, concurrency, mailbox semantics, testing and debugging. The implementation was compiled and tested incrementally rather than accepting generated suggestions without verification.
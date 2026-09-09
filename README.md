# CCI Relay Server

CCI (Cisco Chat Interview) is a small TCP client/server message relay
implemented in C.

The project demonstrates:

- a custom length-prefixed binary protocol over TCP
- concurrent client handling using POSIX threads
- bounded per-client mailboxes
- FIFO message delivery
- explicit acknowledgement
- offline message retention
- reconnect and redelivery of unacknowledged messages
- bounded active connections and message sizes

A small Python reference client is provided for interacting with the server.

---

## Requirements

### C server

- GCC or Clang
- POSIX threads
- Linux or macOS
- Make

### Client and integration tests

- Python 3

---

## Build

The recommended build method is:

```bash
make clean
make test
make run
```

## Client 
client:
```bash
python3 client/cci_client.py 127.0.0.1 9000
```

Available commands:
```
register <username>
send <recipient> <message>
fetch
ack <message_id>
quit
```

Example:
```
> register alice
OK operation=1 value=0

> send bob hello bob
OK operation=2 value=1

A second client can register as Bob:

> register bob
OK operation=1 value=0

> fetch
MESSAGE id=1 from=alice: hello bob

> ack 1
OK operation=4 value=1
```


## Docker
Docker Build:
```
docker build -t cci-relay .
```
Run:
```
docker run --rm -p 9000:9000 cci-relay
```
Then connect with:
```
python3 client/cci_client.py 127.0.0.1 9000
```
Decision is to create a server that will be able to handle upto 10 connection at the same time.
This is to arbitrary constrain.

The job of a server will be to handle connection with a client. the server only response on clients requests.

I am going to develope a simple TCP protocol Cisco Chat Interview CCI.

This protocol will be structured as follow:

+----------------+ 1 bytes
| MAGIC          |
+----------------+ 1 byte
| VERSION        |
+----------------+ 4 bytes
| TYPE           |
+----------------+ 4 bytes
| PAYLOAD_LENGTH |
+----------------+
| PAYLOAD        | N bytes
+----------------+

We will hard limit our payload to 2024 bytes. This is so our locacl machine can handle the payload storeded in memory.

For the messages we will handle them as follow.

On a sccessful registration of a client:
- server will validate wheather the client:  exists / is connected to the server / returns error.

- when the client succesfully connects it will then send a FETCH and request server to send all the messages to the client one by one.

- server will send them in FIFO order 

- to ensure mssages are not lost the client will have to send ACK after reciving them and on then the server will free up the memory and delete the messages from itself

- server will also be responsible for the UUID of the message and it will tag each message with appropiate ID.


The server operations for each of the type I chose are:

REGISTER
    payload: username

SEND
    payload: destination + body
    server will generate uint64 msg ID

FETCH
    payload: empty
    oldest FIFO msg
    marks message IN_FLIGHT

ACK
    payload: message ID
    remove msg


The responses the client can accept are:
OK
ERROR
DELIVERY

if client disconnect and the msg wont be delivered to them the IN_FLIGHT will retry once the client reconnects

reconnect + REGISTER + FETCH -> same message is redeliveredinc



# CCI Relay — Approach

## 1. Acceptance Criteria

TODO

## 2. Architecture

TODO

## 3. CCI Protocol

### Header

| Field | Size |
|---|---:|
| Magic | 1 byte |
| Version | 1 byte |
| Type | 2 bytes |
| Payload length | 4 bytes |

All multi-byte integer fields use network byte order.

### REGISTER

TODO

### SEND

TODO

### FETCH

TODO

### ACK

TODO

### OK

TODO

### DELIVERY

TODO

### ERROR

TODO

## 4. Connection Lifecycle

TODO

## 5. State Model

TODO

## 6. Mailbox Model

FIFO bounded ring buffer.

TODO

## 7. Delivery Semantics

Messages remain stored until acknowledged.

TODO

## 8. Ordering

TODO

## 9. Duplicate Behaviour

TODO

## 10. Concurrency Model

TODO

## 11. Resource Limits

TODO

## 12. Shutdown Behaviour

TODO

## 13. Testing Strategy

TODO

## 14. Trade-offs

TODO

## 15. Known Limitations

TODO

## 16. Next Steps

TODO

## 17. AI Usage

TODO
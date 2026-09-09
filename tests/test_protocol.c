#include "cci_protocol.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


static void test_build_frame(void)
{
    CCIFrame frame;

    const uint8_t payload[] = {'a', 'l', 'i', 'c', 'e'};

    assert(cci_build_frame(&frame, CCI_REGISTER, payload, sizeof(payload)) == 0);
    assert(frame.header.magic == CCI_MAGIC);
    assert(frame.header.version == CCI_VERSION);
    assert(frame.header.type == CCI_REGISTER);
    assert(frame.header.payload_length == sizeof(payload));
    assert(memcmp(frame.payload, payload, sizeof(payload)) == 0);
}


static void test_parse_register(void)
{
    CCIFrame frame;

    const uint8_t payload[] = {'a', 'l', 'i', 'c', 'e'};

    assert(cci_build_frame(&frame, CCI_REGISTER, payload, sizeof(payload)) == 0);

    char username[32];

    assert(cci_parse_register(&frame, username, sizeof(username)) == 0);
    assert(strcmp(username, "alice") == 0);
}


static void test_parse_send(void)
{
    CCIFrame frame;

    /*
     * destination = bob
     * body = hello
     */
    uint8_t payload[] = {3, 'b', 'o', 'b', 'h', 'e', 'l', 'l', 'o'};

    assert(cci_build_frame(&frame, CCI_SEND, payload, sizeof(payload)) == 0);

    char destination[32];

    uint8_t body[1024];

    size_t body_length = 0;

    assert(cci_parse_send(&frame, destination, sizeof(destination), body, sizeof(body), &body_length) == 0);
    assert(strcmp(destination, "bob") == 0);
    assert(body_length == 5);
    assert(memcmp(body, "hello", 5) == 0);
}


static void test_ack(void)
{
    CCIFrame frame;

    uint64_t expected_id = 0x0102030405060708ULL;

    uint8_t payload[8];

    for (int i = 0; i < 8; i++) {
        payload[i] = (uint8_t)(expected_id >> (56 - i * 8));
    }

    assert(cci_build_frame(&frame, CCI_ACK, payload, sizeof(payload)) == 0);

    uint64_t actual_id = 0;

    assert(cci_parse_ack(&frame, &actual_id) == 0);
    assert(actual_id == expected_id);
}


static void test_socket_round_trip(void)
{
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);

    CCIFrame outgoing;

    const uint8_t payload[] = {'a', 'l', 'i', 'c', 'e'};

    assert(cci_build_frame(&outgoing, CCI_REGISTER, payload, sizeof(payload)) == 0);
    assert(cci_write_frame(sockets[0], &outgoing) == 0);

    CCIFrame incoming;

    assert(cci_read_frame(sockets[1], &incoming) == 0);
    assert(incoming.header.magic == CCI_MAGIC);
    assert(incoming.header.version == CCI_VERSION);
    assert(incoming.header.type == CCI_REGISTER);
    assert(incoming.header.payload_length == sizeof(payload));
    assert(memcmp(incoming.payload, payload, sizeof(payload)) == 0);

    close(sockets[0]);
    close(sockets[1]);
}


int main(void)
{
    test_build_frame();
    test_parse_register();
    test_parse_send();
    test_ack();
    test_socket_round_trip();

    printf("All protocol tests passed\n");

    return 0;
}

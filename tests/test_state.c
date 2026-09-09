#include "cci_state.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


static void test_register(void)
{
    ServerState state;
    assert(state_init(&state) == 0);

    Client *alice = NULL;
    assert(state_register(&state, "alice", 10, &alice) == STATE_OK);
    assert(alice != NULL);
    assert(strcmp(alice->name, "alice") == 0);
    assert(alice->connection_state == CLIENT_CONNECTED);
    assert(alice->socket_fd == 10);
    assert(state.client_count == 1);

    state_destroy(&state);
}


static void test_duplicate_registration(void)
{
    ServerState state;
    assert(state_init(&state) == 0);

    Client *alice = NULL;
    Client *duplicate = NULL;
    assert(state_register(&state, "alice", 10, &alice) == STATE_OK);
    assert(state_register(&state, "alice", 11, &duplicate) == STATE_CLIENT_ALREADY_CONNECTED);
    assert(state.client_count == 1);
    state_destroy(&state);
}


static void test_send_fetch_ack(void)
{
    ServerState state;
    assert(state_init(&state) == 0);

    Client *alice = NULL;
    Client *bob = NULL;
    assert(state_register(&state, "alice", 10, &alice) == STATE_OK);
    assert(state_register(&state, "bob", 11, &bob) == STATE_OK);

    const uint8_t body[] = "hello bob";
    uint64_t message_id = 0;
    assert(state_send(&state, alice, "bob", body, sizeof(body) - 1, &message_id) == STATE_OK);
    assert(message_id == 1);
    assert(bob->mailbox.count == 1);

    Message message;
    assert(state_fetch(bob, &message) == STATE_OK);
    assert(message.id == message_id);
    assert(message.state == MESSAGE_IN_FLIGHT);
    assert(message.body_length == sizeof(body) - 1);
    assert(memcmp(message.body, body, sizeof(body) - 1) == 0);
    assert(state_ack(bob, message_id) == STATE_OK);
    assert(bob->mailbox.count == 0);
    
    state_destroy(&state);
}


static void test_invalid_ack(void)
{
    ServerState state;
    assert(state_init(&state) == 0);
    Client *alice = NULL;
    Client *bob = NULL;
    assert(state_register(&state, "alice", 10, &alice) == STATE_OK);
    assert(state_register(&state, "bob", 11, &bob) == STATE_OK);
    const uint8_t body[] = "hello";

    uint64_t message_id = 0;
    assert(state_send(&state, alice, "bob", body, sizeof(body) - 1, &message_id) == STATE_OK);

    /*
     * ACK before FETCH must fail.
     */
    assert(state_ack(bob, message_id) == STATE_INVALID_ACK);
    Message message;
    assert(state_fetch(bob, &message) == STATE_OK);

    /*
     * Wrong message ID must fail.
     */
    assert(state_ack(bob, message_id + 1) == STATE_INVALID_ACK);

    /*
     * Correct ACK succeeds.
     */
    assert(state_ack(bob, message_id) == STATE_OK);

    /*
     * Repeated ACK must fail.
     */
    assert(state_ack(bob, message_id) == STATE_INVALID_ACK);

    state_destroy(&state);
}


static void test_fetch_requires_ack(void)
{
    ServerState state;
    assert(state_init(&state) == 0);

    Client *alice = NULL;
    Client *bob = NULL;

    assert(state_register(&state, "alice", 10, &alice) == STATE_OK);
    assert(state_register(&state, "bob", 11, &bob) == STATE_OK);

    const uint8_t body[] = "message";
    uint64_t message_id = 0;
    assert(state_send(&state, alice, "bob", body, sizeof(body) - 1, &message_id) == STATE_OK);

    Message message;
    assert(state_fetch(bob, &message) == STATE_OK);
    assert(state_fetch(bob, &message) == STATE_MESSAGE_AWAITING_ACK);
    state_destroy(&state);
}


static void test_disconnect_redelivery(void)
{
    ServerState state;
    assert(state_init(&state) == 0);

    Client *alice = NULL;
    Client *bob = NULL;
    assert(state_register(&state, "alice", 10, &alice) == STATE_OK);
    assert(state_register(&state, "bob", 11, &bob) == STATE_OK);

    const uint8_t body[] = "important message";
    uint64_t message_id = 0;
    assert(state_send(&state, alice, "bob", body, sizeof(body) - 1, &message_id) == STATE_OK);

    Message first_delivery;
    assert(state_fetch(bob, &first_delivery) == STATE_OK);
    assert(first_delivery.id == message_id);

    /*
     * Disconnect before ACK.
     */
    assert(state_disconnect(&state, bob) == STATE_OK);
    assert(bob->connection_state == CLIENT_DISCONNECTED);
    assert(bob->mailbox.count == 1);

    /*
     * Reconnect same logical identity.
     */
    Client *reconnected_bob = NULL;
    assert(state_register(&state, "bob", 20, &reconnected_bob) == STATE_OK);
    assert(reconnected_bob == bob);
    Message second_delivery;
    assert(state_fetch(reconnected_bob, &second_delivery) == STATE_OK);

    /*
     * Must be the same message ID.
     */
    assert(second_delivery.id == first_delivery.id);
    assert(state_ack(reconnected_bob, second_delivery.id) == STATE_OK);
    assert(reconnected_bob->mailbox.count == 0);
    state_destroy(&state);
}


static void test_offline_delivery(void)
{
    ServerState state;
    assert(state_init(&state) == 0);

    Client *alice = NULL;
    Client *bob = NULL;
    assert(state_register(&state, "alice", 10, &alice) == STATE_OK);
    assert(state_register(&state, "bob", 11, &bob) == STATE_OK);
    assert(state_disconnect(&state, bob) == STATE_OK);

    const uint8_t body[] = "offline message";
    uint64_t message_id = 0;
    assert(state_send(&state, alice, "bob", body, sizeof(body) - 1, &message_id) == STATE_OK);
    assert(bob->mailbox.count == 1);

    Client *reconnected_bob = NULL;
    assert(state_register(&state, "bob", 20, &reconnected_bob) == STATE_OK);

    Message message;
    assert(state_fetch(reconnected_bob, &message) == STATE_OK);
    assert(message.id == message_id);
    state_destroy(&state);
}


int main(void)
{
    test_register();
    test_duplicate_registration();
    test_send_fetch_ack();
    test_invalid_ack();
    test_fetch_requires_ack();
    test_disconnect_redelivery();
    test_offline_delivery();

    printf("All state tests passed\n");

    return 0;
}

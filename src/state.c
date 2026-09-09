#include "cci_state.h"
#include <string.h>
#include <stdlib.h>

int state_init(ServerState *state)
{
    if (state == NULL) return -1;
    memset(state, 0, sizeof(*state));

    state->client_count = 0;
    state->next_message_id = 1;

    if (pthread_mutex_init(&state->mutex, NULL) != 0) return -1;

    for (size_t i = 0; i < MAX_REGISTERED_CLIENTS; i++) {
        state->clients[i].connection_state = CLIENT_DISCONNECTED;
        state->clients[i].socket_fd = -1;
        state->clients[i].mailbox.head = 0;
        state->clients[i].mailbox.tail = 0;
        state->clients[i].mailbox.count = 0;

        if (pthread_mutex_init(&state->clients[i].mailbox.mutex, NULL) != 0) return -1;
    }

    return 0;
}

void state_destroy(ServerState *state)
{
    if (state == NULL) {
        return;
    }
        

    for (size_t i =0; i < MAX_REGISTERED_CLIENTS; i++) {
        pthread_mutex_destroy(&state->clients[i].mailbox.mutex);
    }
    pthread_mutex_destroy(&state->mutex);
}


static Client *find_client_unlocked(ServerState *state, const char *username)
{
    for (size_t i = 0; i < state->client_count; i++) {
        char* name = state->clients[i].name;
        if (strcmp(username, name) == 0) return &state->clients[i];
    }

    return NULL;
}

StateResult state_register(ServerState *state, const char *username, int socket_fd, Client **out_client)
{
    if (state == NULL || username == NULL || out_client == NULL) {
        return STATE_INVALID_ARGUMENT;
    }
    *out_client = NULL;

    if (strlen(username) > MAX_USERNAME_LEN - 1 || strlen(username) == 0) {
        return STATE_INVALID_ARGUMENT;
    }

    pthread_mutex_lock(&state->mutex);
   
    Client* client = find_client_unlocked(state, username);


    if (client != NULL) {
        if (client->connection_state == CLIENT_CONNECTED) {
            pthread_mutex_unlock(&state->mutex);
            return STATE_CLIENT_ALREADY_CONNECTED;
        }

        if (client->connection_state == CLIENT_DISCONNECTED) {
            client->connection_state = CLIENT_CONNECTED;
            client->socket_fd = socket_fd;
            *out_client = client;
            pthread_mutex_unlock(&state->mutex);
            return STATE_OK;
        }
    }

    if (state->client_count >= MAX_REGISTERED_CLIENTS) {
        pthread_mutex_unlock(&state->mutex);
        return STATE_CLIENT_LIMIT_REACHED;
    }

    client = &state->clients[state->client_count];

    client->connection_state = CLIENT_CONNECTED;

    for (size_t i = 0; i < strlen(username); i++) {
        client->name[i] = *(username + i);
    }
    
    client->name[strlen(username)] = '\0';

    client->socket_fd = socket_fd;
    state->client_count++;
    *out_client = client;
    pthread_mutex_unlock(&state->mutex);

    return STATE_OK;
}


StateResult state_disconnect(ServerState *state, Client *client)
{
    if (state == NULL || client == NULL) {
        return STATE_INVALID_ARGUMENT;
    }

    pthread_mutex_lock(&state->mutex);
    pthread_mutex_lock(&client->mailbox.mutex);

    if (client->connection_state == CLIENT_DISCONNECTED) {
        pthread_mutex_unlock(&client->mailbox.mutex);
        pthread_mutex_unlock(&state->mutex);
        return STATE_INVALID_ARGUMENT;
    }

    client->socket_fd = -1;
    client->connection_state = CLIENT_DISCONNECTED;

    if (client->mailbox.count > 0) {
        Message *msg = &client->mailbox.messages[client->mailbox.head];

        if (msg->state == MESSAGE_IN_FLIGHT) {
            msg->state = MESSAGE_QUEUED;
        }

    }
    pthread_mutex_unlock(&client->mailbox.mutex);
    pthread_mutex_unlock(&state->mutex);

    return STATE_OK;
}


StateResult state_send(ServerState *state, Client *sender, const char* dest_user, const uint8_t *body, size_t body_size, uint64_t *out_message_id)
{
    if (state == NULL || sender == NULL || dest_user == NULL || body == NULL || out_message_id == NULL || body_size == 0) { 
        return STATE_INVALID_ARGUMENT;
    }

    if (body_size > MAX_MESSAGE_SIZE) {
        return STATE_MESSAGE_TOO_LONG;
    }

    pthread_mutex_lock(&state->mutex);
    
    
    Client* recipient = find_client_unlocked(state, dest_user);
    
    if (recipient == NULL) {
        pthread_mutex_unlock(&state->mutex);

        return STATE_CLIENT_NOT_FOUND;
    }

    pthread_mutex_lock(&recipient->mailbox.mutex);

    if (recipient->mailbox.count >= MAX_MAILBOX_MESSAGES) {

        pthread_mutex_unlock(&recipient->mailbox.mutex);
        pthread_mutex_unlock(&state->mutex);

        return STATE_MAILBOX_FULL;
    }


    Message *new_msg = &recipient->mailbox.messages[recipient->mailbox.tail];
    new_msg->id = state->next_message_id++;
    new_msg->state = MESSAGE_QUEUED;
    new_msg->body_length = body_size;

    for (size_t i = 0; i < strlen(sender->name); i++) {
        new_msg->sender[i] = sender->name[i];
    }
    new_msg->sender[strlen(sender->name)] = '\0';
    

    for (size_t i = 0; i < body_size; i++) {
        new_msg->body[i] = body[i];
    }

    *out_message_id = new_msg->id;

    recipient->mailbox.tail = (recipient->mailbox.tail + 1) % MAX_MAILBOX_MESSAGES;
    recipient->mailbox.count++;

    pthread_mutex_unlock(&recipient->mailbox.mutex);
    pthread_mutex_unlock(&state->mutex);

    return STATE_OK;
}


StateResult state_fetch(Client *client, Message *out_message)
{
    if (client == NULL || out_message == NULL) {
        return STATE_INVALID_ARGUMENT;
    }

    pthread_mutex_lock(&client->mailbox.mutex);

    if (client->mailbox.count == 0) {
        pthread_mutex_unlock(&client->mailbox.mutex);
        return STATE_MAILBOX_EMPTY;
    }

    Message *msg = &client->mailbox.messages[client->mailbox.head];

    if (msg->state == MESSAGE_IN_FLIGHT) {
        pthread_mutex_unlock(&client->mailbox.mutex);
        return STATE_MESSAGE_AWAITING_ACK;
    }

    msg->state = MESSAGE_IN_FLIGHT;

    *out_message = *msg;
    pthread_mutex_unlock(&client->mailbox.mutex);

    return STATE_OK;
}


StateResult state_ack(Client *client, uint64_t message_id)
{
    if (client == NULL) {
        return STATE_INVALID_ARGUMENT;
    }

    pthread_mutex_lock(&client->mailbox.mutex);

    if (client->mailbox.count == 0) {

            pthread_mutex_unlock(&client->mailbox.mutex);

        return STATE_INVALID_ACK;
    }

    Message *msg = &client->mailbox.messages[client->mailbox.head];

    if(msg->id != message_id || msg->state != MESSAGE_IN_FLIGHT) {
        pthread_mutex_unlock(&client->mailbox.mutex);

        return STATE_INVALID_ACK;
    }

    client->mailbox.head = (client->mailbox.head + 1) % MAX_MAILBOX_MESSAGES;
    client->mailbox.count--;
    
    pthread_mutex_unlock(&client->mailbox.mutex);

    return STATE_OK;



}


const char *state_result_string(StateResult result)
{
    switch (result)
    {
    case STATE_OK:
        return "STATE_OK";
    case STATE_INVALID_ARGUMENT:
        return "STATE_INVALID_ARGUMENT";
    case STATE_CLIENT_NOT_FOUND:
        return "STATE_CLIENT_NOT_FOUND";
    case STATE_CLIENT_ALREADY_CONNECTED:
        return "STATE_CLIENT_ALREADY_CONNECTED";
    case STATE_CLIENT_LIMIT_REACHED:
        return "STATE_CLIENT_LIMIT_REACHED";
    case STATE_MAILBOX_FULL:
        return "STATE_MAILBOX_FULL";
    case STATE_MAILBOX_EMPTY:
        return "STATE_MAILBOX_EMPTY";
    case STATE_MESSAGE_TOO_LONG:
        return "STATE_MESSAGE_TOO_LONG";
    case STATE_MESSAGE_AWAITING_ACK:
        return "STATE_MESSAGE_AWAITING_ACK";
    case STATE_INVALID_ACK:
        return "STATE_INVALID_ACK";
    default:
        return "UNKNOWN";
    }
}

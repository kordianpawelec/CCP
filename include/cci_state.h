#ifndef CCI_STATE_H
#define CCI_STATE_H

#include <stdint.h>
#include <stddef.h>

#include <pthread.h>

#define MAX_ACTIVE_CONNECTIONS   10
#define MAX_REGISTERED_CLIENTS   32
#define MAX_USERNAME_LEN         32
#define MAX_MAILBOX_MESSAGES     128
#define MAX_MESSAGE_SIZE         1024


typedef enum
{
    MESSAGE_QUEUED,
    MESSAGE_IN_FLIGHT
} MessageState;


typedef enum
{
    CLIENT_DISCONNECTED,
    CLIENT_CONNECTED,
} ClientConnectionState;


typedef enum
{
    STATE_OK,
    STATE_INVALID_ARGUMENT,
    STATE_CLIENT_NOT_FOUND,
    STATE_CLIENT_ALREADY_CONNECTED,
    STATE_CLIENT_LIMIT_REACHED,
    STATE_MAILBOX_FULL,
    STATE_MAILBOX_EMPTY,
    STATE_MESSAGE_TOO_LONG,
    STATE_MESSAGE_AWAITING_ACK,
    STATE_INVALID_ACK
} StateResult;


typedef struct
{
    uint64_t id;
    char sender[MAX_USERNAME_LEN];
    uint8_t body[MAX_MESSAGE_SIZE];
    size_t body_length;
    MessageState state;
} Message;


typedef struct
{
    Message messages[MAX_MAILBOX_MESSAGES];

    size_t head;
    size_t tail;
    size_t count;

    pthread_mutex_t mutex;
} Mailbox;


typedef struct
{
    char name[MAX_USERNAME_LEN];
    ClientConnectionState connection_state;
    int socket_fd;
    Mailbox mailbox;
} Client;


typedef struct
{
    Client clients[MAX_REGISTERED_CLIENTS];
    size_t client_count;
    uint64_t next_message_id;
    pthread_mutex_t mutex;
} ServerState;




int state_init(ServerState *state);
void state_destroy(ServerState *state);

StateResult state_register(ServerState *state, const char* username, int socket_fd, Client **out_client);
StateResult state_disconnect(ServerState *state, Client *client);

StateResult state_send(ServerState *state, Client *sender, const char *dest_user, const uint8_t *body, size_t body_size, uint64_t *msg_id);
StateResult state_fetch(Client *client, Message *out_msg);
StateResult state_ack(Client* client, uint64_t msg_id);

const char* state_result_string(StateResult result);


#endif

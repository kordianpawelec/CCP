#include "cci_server.h"
#include "cci_protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

typedef struct
{
    Server *server;
    int socket_fd;

} ClientThreadArgs;


static void *client_worker(void *arg);
static int send_protocol_error(int socket_fd, CCIErrorCode error);


static CCIErrorCode state_error_to_cci(StateResult result)
{
    switch (result)
    {
        case STATE_INVALID_ARGUMENT:
            return CCI_ERROR_INVALID_PAYLOAD;

        case STATE_CLIENT_ALREADY_CONNECTED:
            return CCI_ERROR_CLIENT_EXISTS;

        case STATE_CLIENT_NOT_FOUND:
            return CCI_ERROR_CLIENT_NOT_FOUND;

        case STATE_CLIENT_LIMIT_REACHED:
            return CCI_ERROR_CLIENT_LIMIT_REACHED;

        case STATE_MAILBOX_FULL:
            return CCI_ERROR_MAILBOX_FULL;

        case STATE_MAILBOX_EMPTY:
            return CCI_ERROR_MAILBOX_EMPTY;

        case STATE_MESSAGE_TOO_LONG:
            return CCI_ERROR_MESSAGE_TOO_LONG;

        case STATE_MESSAGE_AWAITING_ACK:
            return CCI_ERROR_MESSAGE_AWAITING_ACK;

        case STATE_INVALID_ACK:
            return CCI_ERROR_INVALID_ACK;

        default:
            return CCI_ERROR_INTERNAL;
    }
}


static int send_state_error(int socket_fd, StateResult result)
{
    CCIFrame response;

    if (cci_build_error(&response, state_error_to_cci(result)) != 0) {
        return -1;
    }

    return cci_write_frame(socket_fd, &response);
}


static int handle_register(Server *server, Client **registered_client, int socket_fd,const CCIFrame *request)
{
    char username[MAX_USERNAME_LEN];
    int rc = cci_parse_register(request, username, MAX_USERNAME_LEN);

    if (rc < 0) {
        CCIFrame response;

        if (cci_build_error(&response, CCI_ERROR_INVALID_PAYLOAD) != 0) {
            return -1;
        }

        return cci_write_frame(socket_fd, &response);
    }

    if (*registered_client != NULL) {
        CCIFrame response;

        if (cci_build_error(&response, CCI_ERROR_CLIENT_EXISTS) != 0) {
            return -1;
        }

        return cci_write_frame(socket_fd, &response
);
    }

    StateResult result = state_register(&server->state, username, socket_fd, registered_client);

    if (result != STATE_OK) {
        return send_state_error(socket_fd, result);
    }

    CCIFrame response;

    if (cci_build_ok(&response, CCI_REGISTER, 0) != 0) {
        return -1;
    }

    return cci_write_frame(socket_fd, &response);
}


static int handle_send(Server *server, Client *client, int socket_fd, const CCIFrame *request)
{
    if (client == NULL) {
        return send_protocol_error(socket_fd, CCI_ERROR_INVALID_PAYLOAD);
    }

    char destination[MAX_USERNAME_LEN];

    uint8_t body[MAX_MESSAGE_SIZE];

    size_t body_length = 0;

    uint64_t message_id = 0;


    int rc = cci_parse_send(request, destination, MAX_USERNAME_LEN, body, MAX_MESSAGE_SIZE, &body_length);

    if (rc < 0) {
        return send_protocol_error(socket_fd, CCI_ERROR_INVALID_PAYLOAD);
    }


    StateResult result = state_send(&server->state, client, destination, body, body_length, &message_id);

    if (result != STATE_OK) {
        return send_state_error(socket_fd, result);
    }


    CCIFrame response;

    if (cci_build_ok(&response, CCI_SEND, message_id) != 0) {
        return -1;
    }


    return cci_write_frame(socket_fd, &response);
}

static int handle_fetch(Client *client, int socket_fd, const CCIFrame *request)
{
    if (client == NULL) {
        return send_protocol_error(socket_fd, CCI_ERROR_INVALID_PAYLOAD);
    }

    if (request->header.payload_length != 0) {
        return send_protocol_error(socket_fd, CCI_ERROR_INVALID_PAYLOAD);
    }

    Message message;

    StateResult result = state_fetch(client, &message);

    if (result != STATE_OK) {
        return send_state_error(socket_fd, result);
    }

    CCIFrame response;

    if (cci_build_delivery(&response, message.id, message.sender, message.body, message.body_length) != 0) {
        return -1;
    }

    return cci_write_frame(socket_fd, &response);
}


static int handle_ack(Client *client, int socket_fd, const CCIFrame *request)
{
    if (client == NULL) {
        return send_protocol_error(socket_fd, CCI_ERROR_INVALID_PAYLOAD);
    }


    uint64_t message_id = 0;


    int rc = cci_parse_ack(request, &message_id);

    if (rc < 0) {
        return send_protocol_error(socket_fd, CCI_ERROR_INVALID_PAYLOAD);
    }


    StateResult result = state_ack(client, message_id);

    if (result != STATE_OK) {
        return send_state_error(socket_fd, result);
    }

    CCIFrame response;

    if (cci_build_ok(&response, CCI_ACK, message_id) != 0) {
        return -1;
    }


    return cci_write_frame(socket_fd, &response);
}

static int handle_frame(Server *server, Client **registered_client, int socket_fd, const CCIFrame *request)
{
    if (server == NULL || registered_client == NULL || request == NULL) {
        return -1;
    }


    switch (request->header.type)
    {
        case CCI_REGISTER:
            return handle_register(server, registered_client, socket_fd, request);

        case CCI_SEND:
            if (*registered_client == NULL) {
                return send_protocol_error(socket_fd, CCI_ERROR_INVALID_PAYLOAD);
            }
            return handle_send(server, *registered_client, socket_fd, request);

        case CCI_FETCH:
            if (*registered_client == NULL) {
                return send_protocol_error(socket_fd, CCI_ERROR_INVALID_PAYLOAD);
            }
            return handle_fetch(*registered_client, socket_fd, request);

        case CCI_ACK:
            if (*registered_client == NULL) {
                return send_protocol_error(socket_fd, CCI_ERROR_INVALID_PAYLOAD);
            }
            return handle_ack(*registered_client, socket_fd, request);

        default:
            return send_protocol_error(socket_fd, CCI_ERROR_INVALID_FRAME);
    }
}

int server_init(Server *server, uint16_t port)
{
    if (server == NULL) {
        return -1;
    }


    memset(server, 0, sizeof(*server));

    server->listen_fd = -1;
    server->port = port;

    signal(SIGPIPE, SIG_IGN);

    if (state_init(&server->state) != 0) {
        return -1;
    }


    if (pthread_mutex_init(&server->connection_mutex,NULL) != 0) {
        state_destroy(&server->state);
        return -1;
    }


    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server->listen_fd < 0) {
        pthread_mutex_destroy(&server->connection_mutex);
        state_destroy(&server->state);
        return -1;
    }

    int reuse = 1;

    if (setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        close(server->listen_fd);
        pthread_mutex_destroy(&server->connection_mutex);
        state_destroy(&server->state);
        return -1;
    }

    struct sockaddr_in address;

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr =htonl(INADDR_ANY);
    address.sin_port = htons(port);


    if (bind(server->listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(server->listen_fd);
        pthread_mutex_destroy(&server->connection_mutex);
        state_destroy(&server->state);
        return -1;
    }


    if (listen(server->listen_fd, MAX_ACTIVE_CONNECTIONS) < 0) {
        close(server->listen_fd);
        pthread_mutex_destroy(&server->connection_mutex);
        state_destroy(&server->state);
        return -1;
    }


    server->active_connections = 0;
    atomic_store(&server->running, true);
    return 0;
}


int server_run(Server *server)
{
    if (server == NULL) {
        return -1;
    }


    while (atomic_load(&server->running))
    {
        int client_fd = accept(server->listen_fd, NULL, NULL);


        if (client_fd < 0)
        {
            if (errno == EINTR) {
                continue;
            }

            if (!atomic_load(&server->running)) {
                break;
            }

            return -1;
        }

        pthread_mutex_lock(&server->connection_mutex);


        if (server->active_connections >= MAX_ACTIVE_CONNECTIONS) {
            pthread_mutex_unlock(&server->connection_mutex);
            send_protocol_error(client_fd, CCI_ERROR_CLIENT_LIMIT_REACHED);
            close(client_fd);
            continue;
        }

        server->active_connections++;

        pthread_mutex_unlock(&server->connection_mutex);

        ClientThreadArgs *args = malloc(sizeof(*args));

        if (args == NULL)
        {
            close(client_fd);
            pthread_mutex_lock(&server->connection_mutex);
            server->active_connections--;
            pthread_mutex_unlock(&server->connection_mutex);
            continue;
        }


        args->server = server;
        args->socket_fd = client_fd;

        pthread_t thread;

        if (pthread_create(&thread, NULL, client_worker, args) != 0) {
            free(args);
            close(client_fd);
            pthread_mutex_lock(&server->connection_mutex);
            server->active_connections--;
            pthread_mutex_unlock(&server->connection_mutex);
            continue;
        }
        pthread_detach(thread);
    }
    return 0;
}




static void *client_worker(void *arg)
{
    ClientThreadArgs *args = (ClientThreadArgs *)arg;

    Server *server = args->server;

    int socket_fd = args->socket_fd;

    free(args);


    Client *registered_client = NULL;


    while (atomic_load(&server->running))
    {
        CCIFrame request;

        int rc = cci_read_frame(socket_fd, &request);

        if (rc < 0) {
            break;
        }

        rc = handle_frame(server, &registered_client, socket_fd, &request);

        if (rc < 0) {
            break;
        }
    }


    if (registered_client != NULL) {
        state_disconnect(&server->state, registered_client);
    }

    close(socket_fd);

    pthread_mutex_lock(&server->connection_mutex);

    if (server->active_connections > 0) {
        server->active_connections--;
    }

    pthread_mutex_unlock(&server->connection_mutex);


    return NULL;
}

void server_request_stop(Server *server)
{
    if (server == NULL) {
        return;
    }

    atomic_store(&server->running, false);

    if (server->listen_fd >= 0)
    {
        shutdown(server->listen_fd, SHUT_RDWR);
    }
}


void server_destroy(Server *server)
{
    if (server == NULL) {
        return;
    }

    server_request_stop(server);

    if (server->listen_fd >= 0)
    {
        close(server->listen_fd);
        server->listen_fd = -1;
    }

    state_destroy(&server->state);

    pthread_mutex_destroy(&server->connection_mutex);
}

static int send_protocol_error(int socket_fd, CCIErrorCode error)
{
    CCIFrame response;

    if (cci_build_error(&response, error) != 0) {
        return -1;
    }

    return cci_write_frame(socket_fd, &response);
}

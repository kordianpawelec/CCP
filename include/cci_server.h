#ifndef CCI_SERVER_H
#define CCI_SERVER_H

#include "cci_state.h"

#include <stddef.h>
#include <stdint.h>
#include <pthread.h>


typedef struct {
    int listen_fd;

    uint16_t port;

    ServerState state;

    size_t active_connections;
    pthread_mutex_t connection_mutex;

    int running;
} Server;


int server_init(
    Server *server,
    uint16_t port
);

int server_run(
    Server *server
);

void server_request_stop(
    Server *server
);

void server_destroy(
    Server *server
);


#endif
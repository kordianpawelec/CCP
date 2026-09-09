#include "cci_server.h"
#include "cci_protocol.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


typedef struct {
    Server *server;
    int socket_fd;
} ClientThreadArgs;


static void *client_worker(void *arg);


static int handle_frame(
    Server *server,
    Client **registered_client,
    int socket_fd,
    const CCIFrame *request)
{
    /*
     * TODO:
     *
     * switch(request->header.type)
     *
     * REGISTER
     * SEND
     * FETCH
     * ACK
     *
     * Dispatch to state functions.
     */

    return 0;
}


static int handle_register(
    Server *server,
    Client **registered_client,
    int socket_fd,
    const CCIFrame *request)
{
    /*
     * TODO
     */

    return 0;
}


static int handle_send(
    Server *server,
    Client *client,
    int socket_fd,
    const CCIFrame *request)
{
    /*
     * TODO
     */

    return 0;
}


static int handle_fetch(
    Client *client,
    int socket_fd)
{
    /*
     * TODO
     */

    return 0;
}


static int handle_ack(
    Client *client,
    int socket_fd,
    const CCIFrame *request)
{
    /*
     * TODO
     */

    return 0;
}


int server_init(
    Server *server,
    uint16_t port)
{
    /*
     * TODO:
     *
     * state_init()
     *
     * socket()
     * setsockopt()
     * bind()
     * listen()
     *
     * initialise connection mutex
     */

    return -1;
}


int server_run(Server *server)
{
    /*
     * TODO:
     *
     * while running:
     *
     *     accept()
     *
     *     enforce MAX_CLIENTS active connections
     *
     *     allocate ClientThreadArgs
     *
     *     pthread_create()
     *
     * Decide whether threads are detached
     * or joined during shutdown.
     */

    return -1;
}


static void *client_worker(void *arg)
{
    /*
     * TODO:
     *
     * ClientThreadArgs args
     *
     * Client *registered_client = NULL;
     *
     * loop:
     *
     *     cci_read_frame()
     *
     *     handle_frame()
     *
     * disconnect:
     *
     *     if registered:
     *         state_disconnect()
     *
     *     close socket
     *
     *     decrement active connection count
     */

    return NULL;
}


void server_request_stop(Server *server)
{
    /*
     * TODO:
     *
     * Trigger predictable shutdown.
     */

    (void) server;
}


void server_destroy(Server *server)
{
    /*
     * TODO:
     *
     * close listener
     * state_destroy()
     * destroy connection mutex
     */

    (void) server;
}
#include "cci_server.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define DEFAULT_PORT 9000


int main(int argc, char **argv)
{
    uint16_t port = DEFAULT_PORT;

    if (argc > 1) {
        /*
         * TODO:
         *
         * Parse and validate port properly.
         */
        port = (uint16_t) atoi(argv[1]);
    }


    Server server;

    if (server_init(&server, port) != 0) {
        fprintf(stderr, "Failed to initialise server\n");
        return EXIT_FAILURE;
    }


    printf("CCI server listening on port %u\n", port);

    int result = server_run(&server);

    server_destroy(&server);

    return result == 0
        ? EXIT_SUCCESS
        : EXIT_FAILURE;
}
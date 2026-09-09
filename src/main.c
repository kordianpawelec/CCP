#include "cci_server.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_PORT 9000

int main(int argc, char **argv)
{
    uint16_t port = DEFAULT_PORT;

    if (argc > 1) {
        long parsed_port = strtol(argv[1], NULL, 10);

        if (parsed_port <= 0 || parsed_port > 65535) {
            fprintf(stderr, "Invalid port\n");
            return EXIT_FAILURE;
        }

        port = (uint16_t)parsed_port;
    }

    Server server;

    if (server_init(&server, port) != 0) {
        fprintf(stderr, "Failed to initialize server\n");
        return EXIT_FAILURE;
    }

    printf("CCI server listening on port %u\n", port);

    int rc = server_run(&server);

    server_destroy(&server);

    return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

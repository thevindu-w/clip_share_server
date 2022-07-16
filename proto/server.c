#include <stdio.h>
#include <stdlib.h>

#include "../utils/net_utils.h"
#include "versions.h"
#include "server.h"

// protocol version status
#define PROTOCOL_SUPPORTED 1
#define PROTOCOL_OBSOLETE 2
#define PROTOCOL_UNKNOWN 3

void server(int socket)
{
    unsigned char version;
    if (read_sock(socket, (char *)&version, 1) == EXIT_FAILURE)
    {
        return;
    }
    if (version < PROTOCOL_MIN)
    { /* the protocol version used by the client is obsolete
        and not supported by the server */
        if (write_sock(socket, &(char){PROTOCOL_OBSOLETE}, 1) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version status failed\n");
#endif
        }
        return;
    }
    else if (version <= PROTOCOL_MAX)
    { /* the protocol version used by the client is supported by the server */
        if (write_sock(socket, &(char){PROTOCOL_SUPPORTED}, 1) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version status failed\n");
#endif
            return;
        }
    }
    else
    { /* the protocol version used by the client is newer than the
        latest protocol version supported by the server */
        if (write_sock(socket, &(char){PROTOCOL_UNKNOWN}, 1) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version status failed\n");
#endif
            return;
        }
        if (write_sock(socket, &(char){PROTOCOL_MAX}, 1) == EXIT_FAILURE)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "send protocol version failed\n");
#endif
            return;
        }
        if (read_sock(socket, (char *)&version, 1) == EXIT_FAILURE)
        {
            return;
        }
        if (version != PROTOCOL_MAX)
        { /* client is not going to continue with a supported version */
            return;
        }
    }
    switch (version)
    {
    case 0: // version 0 is for testing purposes
        break;
    case 1:
    {
        version_1(socket);
        break;
    }
    default: // invalid or unknown version
        break;
    }
}
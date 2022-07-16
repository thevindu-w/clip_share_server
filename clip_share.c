/*
 *  clip_share.c - server which communicates with client app to share data
 *  Copyright (C) 2022 H. Thevindu J. Wijesekera
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "utils/utils.h"
#include "utils/net_utils.h"
#include "proto/server.h"

int clip_share(const int port)
{
    int listener_d = open_listener_socket();
    bind_port(listener_d, port);
    if (listen(listener_d, 3) == -1)
        error("Can\'t listen");
    signal(SIGCHLD, SIG_IGN);
    while (1)
    {
        int connect_d = get_connection(listener_d);
#ifndef DEBUG_MODE
        pid_t p1 = fork();
        if (p1)
        {
            close(connect_d);
        }
        else
        {
            close(listener_d);
#endif
            server(connect_d);
            close(connect_d);
#ifndef DEBUG_MODE
            break;
        }
#endif
    }
    return 0;
}

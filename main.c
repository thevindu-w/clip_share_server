/*
 *  main.c - server of clip_share to share data copied to clipboard
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/wait.h>
#endif
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "servers.h"
#include "utils/utils.h"
#include "utils/net_utils.h"

// tcp and udp
#define APP_PORT 4337
// tcp
#define APP_PORT_SECURE 4338
// tcp
#define WEB_PORT 443
// tcp
#define WEB_PORT_NO_ROOT 4339

#ifdef __linux__

static int kill_other_processes(const char *);
static void print_usage(void);

static void print_usage()
{
#ifdef NO_WEB
    fprintf(stderr, "Usage: %s [-s] [-p app_port]\n", PROGRAM_NAME);
#else
    fprintf(stderr, "Usage: %s [-s] [-p app_port] [-w web_port]\n", PROGRAM_NAME);
#endif
}

static int kill_other_processes(const char *prog_name)
{
    DIR *dir;
    struct dirent *dir_ptr;
    FILE *fp;
    char filepath[269];
    char cur_task_name[48];
    char buf[128];
    buf[127] = 0;
    dir = opendir("/proc");
    int cnt = 0;
    const pid_t this_pid = getpid();
    if (dir == NULL)
    {
#ifdef DEBUG_MODE
        fprintf(stderr, "Error opening /proc. dir = NULL\n");
#endif
        return -1;
    }
    while ((dir_ptr = readdir(dir)) != NULL)
    {
        if ((strcmp(dir_ptr->d_name, ".") == 0) || (strcmp(dir_ptr->d_name, "..") == 0))
            continue;
        if (DT_DIR != dir_ptr->d_type)
            continue;
        for (char *dname = dir_ptr->d_name; *dname; dname++)
        {
            if (!isdigit(*dname))
            {
                goto LOOP_END;
            }
        }
        sprintf(filepath, "/proc/%s/status", dir_ptr->d_name);
        fp = fopen(filepath, "r"); // Open the file
        if (fp == NULL)
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "Error opening %s. file ptr = NULL\n", filepath);
#endif
            continue;
        }
        if (fgets(buf, 127, fp) == NULL)
        {
            fclose(fp);
            continue;
        }
        sscanf(buf, "%*s %s", cur_task_name);
        if (!strncmp(prog_name, cur_task_name, 15)) // /proc/<pid>/status truncates executable name to 15 chars
        {
            pid_t pid = (pid_t)strtoul(dir_ptr->d_name, NULL, 10);
            if (pid != this_pid)
            {
#ifdef DEBUG_MODE
                fprintf(stderr, "killed %s\n", dir_ptr->d_name);
#endif
                kill(pid, SIGTERM);
                cnt++;
            }
        }
        fclose(fp);
    LOOP_END:;
    }
    closedir(dir);
    return cnt;
}

int main(int argc, char **argv)
{
    /*program must be called as PROGRAM_NAME*/
    {
        char *prog_name = strrchr(argv[0], '/');
        if (!prog_name)
        {
            prog_name = argv[0];
        }
        else
        {
            prog_name++; // don't want the '/' before the program name
        }
        if (strcmp(prog_name, PROGRAM_NAME))
        { // prog_name is not equal to PROGRAM_NAME
            fprintf(stderr, "This program must be called as %s\nBut it is called as %s\nAborting\n", PROGRAM_NAME, prog_name);
            return 1;
        }
    }

    // Parse args
    int stop = 0;
    int app_port = APP_PORT;
#ifndef NO_WEB
    int web_port = geteuid() ? WEB_PORT_NO_ROOT : WEB_PORT;
#endif
    {
        int opt;
        while ((opt = getopt(argc, argv, "sp:w:")) != -1) // TODO : implement quiet mode
        {
            switch (opt)
            {
            case 's':
            {
                stop = 1;
                break;
            }
            case 'p':
            {
                char *endptr;
                app_port = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || endptr == optarg)
                {
                    fprintf(stderr, "Invalid app port %s\n", optarg);
                    print_usage();
                    exit(EXIT_FAILURE);
                }
                break;
            }
#ifndef NO_WEB
            case 'w':
            {
                char *endptr;
                web_port = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || endptr == optarg)
                {
                    fprintf(stderr, "Invalid web port %s\n", optarg);
                    print_usage();
                    exit(EXIT_FAILURE);
                }
                break;
            }
#endif
            default: /* '?' */
                print_usage();
                exit(EXIT_FAILURE);
            }
        }
    }
    /*stop other instances of this process if any and stop this process*/
    if (stop)
    {
        int cnt = kill_other_processes(PROGRAM_NAME);
        if (cnt > 0)
        {
            puts("Server Stopped");
        }
        exit(EXIT_SUCCESS);
    }

#ifndef DEBUG_MODE
    pid_t p_clip = fork();
    if (p_clip == 0)
    {
#endif
        return clip_share(app_port);
#ifndef DEBUG_MODE
    }
#ifndef NO_WEB
    pid_t p_web = fork();
    if (p_web == 0)
    {
        return web_server(web_port);
    }
#endif
    puts("Server Started");
    pid_t p_scan = fork();
    if (p_scan == 0)
    {
        udp_server(app_port);
        return 0;
    }
#endif
    exit(EXIT_SUCCESS);
}

#elif _WIN32

static DWORD WINAPI udpThreadFn(void *arg)
{
    (void)arg;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        error("failed WSAStartup for UDP");
        return EXIT_FAILURE;
    }
    udp_server(APP_PORT);
    return EXIT_SUCCESS;
}

int main()
{
    HANDLE udpThread = CreateThread(NULL, 0, udpThreadFn, NULL, 0, NULL);
    if (udpThread == NULL)
    {
#ifdef DEBUG_MODE
        error("UDP thread creation failed");
#endif
    }
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        error("failed WSAStartup");
        return EXIT_FAILURE;
    }

    clip_share(APP_PORT);

    return EXIT_SUCCESS;
}

#endif
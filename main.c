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

#include "config.h"
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

extern char blob_cert[];
extern char blob_key[];
extern char blob_ca_cert[];

static int kill_other_processes(const char *);
static void print_usage(const char *);

static void print_usage(const char *prog_name)
{
#ifdef NO_WEB
    fprintf(stderr, "Usage: %s [-h] [-s] [-p app_port]\n", prog_name);
#else
    fprintf(stderr, "Usage: %s [-h] [-s] [-p app_port] [-w web_port]\n", prog_name);
#endif
}

static int kill_other_processes(const char *prog_name)
{
    DIR *dir;
    struct dirent *dir_ptr;
    FILE *fp;
    char filepath[269];
    char cur_task_name[16];
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
        sscanf(buf, "%*s %15s", cur_task_name);
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
    // Get basename of the program
    char *prog_name = strrchr(argv[0], '/');
    if (!prog_name)
    {
        prog_name = argv[0];
    }
    else
    {
        prog_name++; // don't want the '/' before the program name
    }

    config cfg = parse_conf("clipshare.conf");

    // Parse args
    int stop = 0;
    int restart = 0;
    int app_port = cfg.app_port > 0 ? cfg.app_port : APP_PORT;
    int app_port_secure = cfg.app_port_secure > 0 ? cfg.app_port_secure : APP_PORT_SECURE;
#ifndef NO_WEB
    int web_port = geteuid() ? WEB_PORT_NO_ROOT : WEB_PORT;
    if (cfg.web_port > 0)
    {
        web_port = cfg.web_port;
    }
#endif

    char *priv_key = cfg.priv_key ? cfg.priv_key : blob_key;
    cfg.priv_key = priv_key;
    char *server_cert = cfg.server_cert ? cfg.server_cert : blob_cert;
    cfg.server_cert = server_cert;
    char *ca_cert = cfg.ca_cert ? cfg.ca_cert : blob_ca_cert;
    cfg.ca_cert = ca_cert;

    {
        int opt;
        while ((opt = getopt(argc, argv, "hsrp:w:")) != -1)
        {
            switch (opt)
            {
            case 'h': // help
            {
                print_usage(prog_name);
                exit(EXIT_SUCCESS);
                break;
            }
            case 's': // stop
            {
                stop = 1;
                break;
            }
            case 'r': // restart
            {
                restart = 1;
                break;
            }
            case 'p': // app port
            {
                char *endptr;
                app_port = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || endptr == optarg)
                {
                    fprintf(stderr, "Invalid app port %s\n", optarg);
                    print_usage(prog_name);
                    exit(EXIT_FAILURE);
                }
                break;
            }
#ifndef NO_WEB
            case 'w': // web port
            {
                char *endptr;
                web_port = strtol(optarg, &endptr, 10);
                if (*endptr != '\0' || endptr == optarg)
                {
                    fprintf(stderr, "Invalid web port %s\n", optarg);
                    print_usage(prog_name);
                    exit(EXIT_FAILURE);
                }
                break;
            }
#endif
            default:
                print_usage(prog_name);
                exit(EXIT_FAILURE);
            }
        }
    }
    /* stop other instances of this process if any.
    Stop this process if stop flag is set */
    if (stop || restart)
    {
        int cnt = kill_other_processes(prog_name);
        if (cnt > 0)
        {
            const char *msg = stop ? "Server Stopped" : "Server Restarting...";
            puts(msg);
        }
        if (stop)
            exit(EXIT_SUCCESS);
    }

    pid_t p_clip = fork();
    if (p_clip == 0)
    {
        return clip_share(app_port, 0, cfg);
    }
    pid_t p_clip_ssl = fork();
    if (p_clip_ssl == 0)
    {
        return clip_share(app_port_secure, 1, cfg);
    }
#ifndef NO_WEB
    pid_t p_web = fork();
    if (p_web == 0)
    {
        return web_server(web_port, cfg);
    }
#endif
    puts("Server Started");
    pid_t p_scan = fork();
    if (p_scan == 0)
    {
        udp_server(app_port);
        return 0;
    }
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
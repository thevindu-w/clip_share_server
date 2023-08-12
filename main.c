/*
 *  main.c - server of clip_share to share data copied to clipboard
 *  Copyright (C) 2022-2023 H. Thevindu J. Wijesekera
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
#include <string.h>
#include <fcntl.h>

#include "config.h"
#include "globals.h"
#include "servers.h"
#include "utils/utils.h"
#include "utils/net_utils.h"

#ifdef __linux__
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <ctype.h>
#elif _WIN32
#include <tlhelp32.h>
#include "win_getopt/getopt.h"
#endif

// tcp and udp
#define APP_PORT 4337
// tcp
#define APP_PORT_SECURE 4338
// tcp
#define WEB_PORT 4339

config configuration;

static void print_usage(const char *);
static void kill_other_processes(const char *);

static void print_usage(const char *prog_name)
{
#ifdef NO_WEB
    fprintf(stderr, "Usage: %s [-h] [-s] [-r] [-R] [-p app_port]\n", prog_name);
#else
    fprintf(stderr, "Usage: %s [-h] [-s] [-r] [-R] [-p app_port] [-w web_port]\n", prog_name);
#endif
}

#ifdef __linux__
/*
 * Attempts to kill all other processes with the name, prog_name, using SIGTERM.
 * This does not kill itself.
 * It is not guaranteed to kill all processes with the given name.
 * Some processes may still survive if the effective user of this process has
 * no permission to kill that process, or if the process ignores SIGTERM.
 */
static void kill_other_processes(const char *prog_name)
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
        return;
    }
    while ((dir_ptr = readdir(dir)) != NULL)
    {
        if ((strcmp(dir_ptr->d_name, ".") == 0) || (strcmp(dir_ptr->d_name, "..") == 0))
            continue;
        if (DT_DIR != dir_ptr->d_type)
            continue;
        for (const char *dname = dir_ptr->d_name; *dname; dname++)
        {
            if (!isdigit(*dname))
            {
                goto LOOP_END;
            }
        }
        if (snprintf_check(filepath, 269, "/proc/%s/status", dir_ptr->d_name))
        {
#ifdef DEBUG_MODE
            fprintf(stderr, "Error writing file name\n");
#endif
            continue;
        }
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
    (void)closedir(dir);
    return;
}

#elif _WIN32

/*
 * Attempts to kill all other processes with the name, prog_name.
 */
static void kill_other_processes(const char *prog_name)
{
    DWORD this_pid = GetCurrentProcessId();
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if ((stricmp(entry.szExeFile, prog_name) == 0) && (entry.th32ProcessID != this_pid))
            {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }
    }
    CloseHandle(snapshot);
}

static DWORD WINAPI udpThreadFn(void *arg)
{
    (void)arg;
    udp_server(configuration.app_port);
    return EXIT_SUCCESS;
}

static DWORD WINAPI appThreadFn(void *arg)
{
    (void)arg;
    clip_share(INSECURE);
    return EXIT_SUCCESS;
}

static DWORD WINAPI appSecureThreadFn(void *arg)
{
    (void)arg;
    clip_share(SECURE);
    return EXIT_SUCCESS;
}

#ifndef NO_WEB
static DWORD WINAPI webThreadFn(void *arg)
{
    (void)arg;
    web_server();
    return EXIT_SUCCESS;
}
#endif

#endif

/*
 * The main entrypoint of the application
 */
int main(int argc, char **argv)
{
    // Get basename of the program
    const char *prog_name = strrchr(argv[0], PATH_SEP);
    if (!prog_name)
    {
        prog_name = argv[0];
    }
    else
    {
        prog_name++; // don't want the '/' before the program name
    }
#ifdef DEBUG_MODE
    printf("prog_name=%s\n", prog_name);
#endif

    configuration = parse_conf("clipshare.conf");

    // Parse args
    int stop = 0;
    int restart = configuration.restart >= 0 ? configuration.restart : 1;
    unsigned short app_port = configuration.app_port > 0 ? configuration.app_port : APP_PORT;
    unsigned short app_port_secure = configuration.app_port_secure > 0 ? configuration.app_port_secure : APP_PORT_SECURE;
#ifndef NO_WEB
    unsigned short web_port = configuration.web_port > 0 ? configuration.web_port : WEB_PORT;
#endif

    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "hsrRp:w:")) != -1)
    {
        switch (opt)
        {
        case 'h': // help
        {
            print_usage(prog_name);
            clear_config(&configuration);
            exit(EXIT_SUCCESS);
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
        case 'R': // no-restart
        {
            restart = 0;
            break;
        }
        case 'p': // app port
        {
            char *endptr;
            app_port = (unsigned short)strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || endptr == optarg)
            {
                fprintf(stderr, "Invalid app port %s\n", optarg);
                print_usage(prog_name);
                clear_config(&configuration);
                exit(EXIT_FAILURE);
            }
            break;
        }
#ifndef NO_WEB
        case 'w': // web port
        {
            char *endptr;
            web_port = (unsigned short)strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || endptr == optarg)
            {
                fprintf(stderr, "Invalid web port %s\n", optarg);
                print_usage(prog_name);
                clear_config(&configuration);
                exit(EXIT_FAILURE);
            }
            break;
        }
#endif
        default:
        {
            print_usage(prog_name);
            clear_config(&configuration);
            exit(EXIT_FAILURE);
        }
        }
    }

    /* stop other instances of this process if any.
    Stop this process if stop flag is set */
    if (stop || restart)
    {
        kill_other_processes(prog_name);
        const char *msg = stop ? "Server Stopped" : "Server Restarting...";
        puts(msg);
        if (stop)
        {
            clear_config(&configuration);
            exit(EXIT_SUCCESS);
        }
    }

    configuration.app_port = app_port;
    configuration.insecure_mode_enabled = configuration.insecure_mode_enabled >= 0 ? configuration.insecure_mode_enabled : 1;
    configuration.app_port_secure = app_port_secure;
    configuration.secure_mode_enabled = configuration.secure_mode_enabled >= 0 ? configuration.secure_mode_enabled : 0;
#ifndef NO_WEB
    configuration.web_port = web_port;
    configuration.web_mode_enabled = configuration.web_mode_enabled >= 0 ? configuration.web_mode_enabled : 0;
#endif

    if (configuration.working_dir)
    {
        if (!is_directory(configuration.working_dir, 1))
        {
            char err[3072];
            snprintf_check(err, 3072, "Not existing working directory \'%s\'", configuration.working_dir);
            fprintf(stderr, "%s\n", err);
            error_exit(err);
        }
        char *old_work_dir = getcwd(NULL, 0);
        if (chdir(configuration.working_dir))
        {
            char err[3072];
            snprintf_check(err, 3072, "Failed changing working directory to \'%s\'", configuration.working_dir);
            fprintf(stderr, "%s\n", err);
            error_exit(err);
            clear_config(&configuration);
            return EXIT_FAILURE;
        }
        char *new_work_dir = getcwd(NULL, 0);
        if (old_work_dir == NULL || new_work_dir == NULL)
        {
            char *err = "Error occured during changing working directory.";
            fprintf(stderr, "%s\n", err);
            error_exit(err);
            clear_config(&configuration);
            return EXIT_FAILURE;
        }
        // if the working directory did not change, set configuration.working_dir to NULL
        if (!strcmp(old_work_dir, new_work_dir))
        {
            configuration.working_dir = NULL;
        }
        free(old_work_dir);
        free(new_work_dir);
    }

#ifdef __linux__
    if (configuration.insecure_mode_enabled)
    {
        pid_t p_clip = fork();
        if (p_clip == 0)
        {
            return clip_share(INSECURE);
        }
    }
    if (configuration.secure_mode_enabled)
    {
        pid_t p_clip_ssl = fork();
        if (p_clip_ssl == 0)
        {
            return clip_share(SECURE);
        }
    }
#ifndef NO_WEB
    if (configuration.web_mode_enabled)
    {
        pid_t p_web = fork();
        if (p_web == 0)
        {
            return web_server();
        }
    }
#endif
    puts("Server Started");
    pid_t p_scan = fork();
    if (p_scan == 0)
    {
        udp_server(app_port);
        return 0;
    }

#elif _WIN32

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        error("failed WSAStartup");
        return EXIT_FAILURE;
    }
    HANDLE insecureThread = NULL, secureThread = NULL;
#ifndef NO_WEB
    HANDLE webThread = NULL;
#endif
    if (configuration.insecure_mode_enabled)
    {
        insecureThread = CreateThread(NULL, 0, appThreadFn, NULL, 0, NULL);
#ifdef DEBUG_MODE
        if (insecureThread == NULL)
        {
            error("App thread creation failed");
        }
#endif
    }

    if (configuration.secure_mode_enabled)
    {
        secureThread = CreateThread(NULL, 0, appSecureThreadFn, NULL, 0, NULL);
#ifdef DEBUG_MODE
        if (secureThread == NULL)
        {
            error("App Secure thread creation failed");
        }
#endif
    }

#ifndef NO_WEB
    if (configuration.web_mode_enabled)
    {
        webThread = CreateThread(NULL, 0, webThreadFn, NULL, 0, NULL);
#ifdef DEBUG_MODE
        if (webThread == NULL)
        {
            error("Web thread creation failed");
        }
#endif
    }
#endif

    HANDLE udpThread = CreateThread(NULL, 0, udpThreadFn, NULL, 0, NULL);
    if (udpThread == NULL)
    {
#ifdef DEBUG_MODE
        error("UDP thread creation failed");
#endif
    }

    if (insecureThread != NULL)
        WaitForSingleObject(insecureThread, INFINITE);
    if (secureThread != NULL)
        WaitForSingleObject(secureThread, INFINITE);
#ifndef NO_WEB
    if (webThread != NULL)
        WaitForSingleObject(webThread, INFINITE);
#endif
#endif
    clear_config(&configuration);
    return EXIT_SUCCESS;
}

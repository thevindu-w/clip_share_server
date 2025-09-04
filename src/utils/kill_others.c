/*
 * utils/kill_others.c - kill other instances of the application
 * Copyright (C) 2022-2024 H. Thevindu J. Wijesekera
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
#include <string.h>
#include <unistd.h>
#include <utils/kill_others.h>
#include <utils/utils.h>
#ifdef __linux__
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#elif defined(__APPLE__)
#include <errno.h>
#include <libproc.h>
#include <signal.h>
#include <sys/sysctl.h>
#elif defined(_WIN32)
#include <processthreadsapi.h>
#include <tlhelp32.h>
#include <userenv.h>
#endif

#ifdef __linux__

/*
 * Attempts to kill all other processes with the name, prog_name, using SIGTERM.
 * This does not kill itself.
 * It is not guaranteed to kill all processes with the given name.
 * Some processes may still survive if the effective user of this process has
 * no permission to kill that process, or if the process ignores SIGTERM.
 */
void kill_other_processes(const char *prog_name) {
    DIR *dir;
    struct dirent *dir_ptr;
    FILE *fp;
    char filepath[269];
    char cur_task_name[16];
    char buf[128];
    buf[127] = 0;
    dir = opendir("/proc");
    const pid_t this_pid = getpid();
    if (dir == NULL) {
#ifdef DEBUG_MODE
        fprintf(stderr, "Error opening /proc. dir = NULL\n");
#endif
        return;
    }
    int terminated = 0;
    while ((dir_ptr = readdir(dir)) != NULL) {
        if ((strcmp(dir_ptr->d_name, ".") == 0) || (strcmp(dir_ptr->d_name, "..") == 0)) continue;
        if (DT_DIR != dir_ptr->d_type) continue;
        int is_invalid = 0;
        for (const char *dname = dir_ptr->d_name; *dname; dname++) {
            if (!isdigit(*dname)) {
                is_invalid = 1;
                break;
            }
        }
        if (is_invalid) continue;
        if (snprintf_check(filepath, 269, "/proc/%s/status", dir_ptr->d_name)) {
#ifdef DEBUG_MODE
            fprintf(stderr, "Error writing file name\n");
#endif
            continue;
        }
        fp = fopen(filepath, "r");  // Open the file
        if (fp == NULL) {
#ifdef DEBUG_MODE
            fprintf(stderr, "Error opening %s. file ptr = NULL\n", filepath);
#endif
            continue;
        }
        if (fgets(buf, 127, fp) == NULL) {
            fclose(fp);
            continue;
        }
        sscanf(buf, "%*s %15[^\n]", cur_task_name);
        fclose(fp);
        if (strncmp(prog_name, cur_task_name, 15))  // /proc/<pid>/status truncates executable name to 15 chars
            continue;
        pid_t pid = (pid_t)strtoul(dir_ptr->d_name, NULL, 10);
        if (pid != this_pid) {
#ifdef DEBUG_MODE
            fprintf(stderr, "terminated %s\n", dir_ptr->d_name);
#endif
            kill(pid, SIGTERM);
            terminated = 1;
        }
    }
    (void)closedir(dir);
    if (terminated) {  // wait for terminated processes to exit
        struct timespec interval = {.tv_sec = 0, .tv_nsec = 5000000L};
        nanosleep(&interval, NULL);
    }
    return;
}

#elif defined(__APPLE__)

void kill_other_processes(const char *prog_name) {
    struct kinfo_proc *procs = NULL;
    size_t count = 0;
    int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    int done = 0;
    size_t length;
    int err;
    do {
        length = 0;
        err = sysctl(name, (sizeof(name) / sizeof(*name)) - 1, NULL, &length, NULL, 0);
        if (err == -1) err = errno;
        if (err == 0) {
            length += 4096;
            procs = malloc(length);
            if (procs == NULL) {
                err = ENOMEM;
            }
        }
        if (err == 0) {
            err = sysctl(name, (sizeof(name) / sizeof(*name)) - 1, procs, &length, NULL, 0);
            if (err == -1) {
                err = errno;
            }
            if (err == 0) {
                done = 1;
            } else if (err == ENOMEM) {
                if (procs) free(procs);
                procs = NULL;
                err = 0;
            }
        }
    } while (err == 0 && !done);
    if (err == 0) {
        count = length / sizeof(struct kinfo_proc);
    } else {
        if (procs) free(procs);
        return;
    }

    pid_t this_pid = getpid();
    char pname[32];
    int terminated = 0;
    for (size_t i = 0; i < count; i++) {
        pid_t pid = procs[i].kp_proc.p_pid;
        pname[0] = 0;
        proc_name(pid, pname, sizeof(pname));
        if (!strncmp(prog_name, pname, 32)) {
            if (pid != this_pid) {
#ifdef DEBUG_MODE
                fprintf(stderr, "terminated %i\n", pid);
#endif
                kill(pid, SIGTERM);
                terminated = 1;
            }
        }
    }
    free(procs);
    if (terminated) {  // wait for terminated processes to exit
        struct timespec interval = {.tv_sec = 0, .tv_nsec = 5000000L};
        nanosleep(&interval, NULL);
    }
}

#elif defined(_WIN32)

/*
 * Attempts to kill all other processes with the name, prog_name.
 * This does not kill itself.
 * It is not guaranteed to kill all processes with the given name.
 */
void kill_other_processes(const char *prog_name) {
    DWORD this_pid = GetCurrentProcessId();
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &entry) == TRUE) {
        while (Process32Next(snapshot, &entry) == TRUE) {
            if ((stricmp(entry.szExeFile, prog_name) == 0) && (entry.th32ProcessID != this_pid)) {
                FreeConsole();
                AttachConsole(entry.th32ProcessID);
                SetConsoleCtrlHandler(NULL, TRUE);
                GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
                FreeConsole();
                Sleep(10);
                SetConsoleCtrlHandler(NULL, FALSE);
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                if (hProcess != INVALID_HANDLE_VALUE) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        }
    }
    FreeConsole();
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
    CloseHandle(snapshot);
}

#endif

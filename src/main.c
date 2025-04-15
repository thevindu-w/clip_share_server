/*
 * main.c - main entrypoint of clip_share
 * Copyright (C) 2022-2025 H. Thevindu J. Wijesekera
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

#include <fcntl.h>
#include <globals.h>
#include <servers/servers.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/config.h>
#include <utils/kill_others.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#ifdef __linux__
#include <pwd.h>
#include <sys/wait.h>
#elif defined(_WIN32)
#include <res/win/resource.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <tlhelp32.h>
#include <userenv.h>
#elif defined(__APPLE__)
#include <pwd.h>
#include <utils/mac_menu.h>
#endif

// tcp and udp
#define APP_PORT 4337
// tcp
#define APP_PORT_SECURE 4338
#ifdef WEB_ENABLED
// tcp
#define WEB_PORT 4339
#endif

// maximum transfer sizes
#define MAX_TEXT_LENGTH 4194304L     // 4 MiB
#define MAX_FILE_SIZE 68719476736LL  // 64 GiB

#define ERROR_LOG_FILE "server_err.log"

config configuration;
char *error_log_file = NULL;
char *cwd = NULL;
size_t cwd_len = 0;

#ifdef __APPLE__
const char *global_prog_name = NULL;
#endif

static void print_usage(const char *);
static char *get_user_home(void);
static char *get_conf_file(void);

static void print_usage(const char *prog_name) {
#ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stderr);
    }
#endif
    fprintf(stderr, "Usage: %s [-h] [-s] [-r] [-R] [-d] [-D]\n", prog_name);
}

/*
 * Parse command line arguments and set corresponding variables
 */
static inline void _parse_args(int argc, char **argv, int *stop_p, int *daemonize_p) {
    int opt;
    while ((opt = getopt(argc, argv, "hvsrRdD")) != -1) {
        switch (opt) {
            case 'h': {  // help
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            }
            case 'v': {  // version
                puts("ClipShare version " VERSION);
                exit(EXIT_SUCCESS);
            }
            case 's': {  // stop
                *stop_p = 1;
                break;
            }
            case 'r': {  // restart
                configuration.restart = 1;
                break;
            }
            case 'R': {  // no-restart
                configuration.restart = 0;
                break;
            }
            case 'd': {  // stop
                *daemonize_p = 1;
                break;
            }
            case 'D': {  // stop
                *daemonize_p = 0;
                break;
            }
            default: {
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }
}

/*
 * Set the error_log_file absolute path
 */
static inline void _set_error_log_file(const char *path) {
    char *working_dir = getcwd_wrapper(2050);
    if (!working_dir) exit(EXIT_FAILURE);
    working_dir[2049] = 0;
    size_t working_dir_len = strnlen(working_dir, 2048);
    if (working_dir_len == 0 || working_dir_len >= 2048) {
        free(working_dir);
        exit(EXIT_FAILURE);
    }
    size_t buf_sz = working_dir_len + strlen(path) + 1;  // +1 for terminating \0
    if (working_dir[working_dir_len - 1] != PATH_SEP) {
        buf_sz++;  // 1 more char for PATH_SEP
    }
    error_log_file = malloc(buf_sz);
    if (!error_log_file) {
        free(working_dir);
        exit(EXIT_FAILURE);
    }
    if (working_dir[working_dir_len - 1] == PATH_SEP) {
        snprintf_check(error_log_file, buf_sz, "%s%s", working_dir, ERROR_LOG_FILE);
    } else {
        snprintf_check(error_log_file, buf_sz, "%s/%s", working_dir, ERROR_LOG_FILE);
    }
    free(working_dir);
}

/*
 * Change working directory to the directory specified in the configuration
 */
static inline void _change_working_dir(void) {
    if (!configuration.working_dir || !*configuration.working_dir) error_exit("Invalid working directory");
    char *ptr;
    for (ptr = configuration.working_dir; *ptr; ptr++) {
        if (*ptr == PATH_SEP) *ptr = '/';
    }
    ptr--;
    while (*ptr == '/' && ptr > configuration.working_dir) {  // do not remove / from root
        *ptr = 0;
        ptr--;
    }
    if (!is_directory(configuration.working_dir, 1)) {
        char err[3072];
        snprintf_check(err, 3072, "Error: Not existing working directory \'%s\'", configuration.working_dir);
        fprintf(stderr, "%s\n", err);
        error_exit(err);
    }
    char *old_work_dir = getcwd_wrapper(0);
    if (chdir_wrapper(configuration.working_dir)) {
        char err[3072];
        snprintf_check(err, 3072, "Error: Failed changing working directory to \'%s\'", configuration.working_dir);
        fprintf(stderr, "%s\n", err);
        if (old_work_dir) free(old_work_dir);
        error_exit(err);
    }
    char *new_work_dir = getcwd_wrapper(0);
    if (old_work_dir == NULL || new_work_dir == NULL) {
        const char *err = "Error occurred during changing working directory.";
        fprintf(stderr, "%s\n", err);
        if (old_work_dir) free(old_work_dir);
        if (new_work_dir) free(new_work_dir);
        error_exit(err);
    }
    // if the working directory did not change, set configuration.working_dir to NULL
    if (!strcmp(old_work_dir, new_work_dir)) {
        configuration.working_dir = NULL;
    }
    free(old_work_dir);
    free(new_work_dir);
}

/*
 * Apply default values to the configuration options that are not specified in conf file.
 */
static inline void _apply_default_conf(void) {
    if (configuration.restart < 0) configuration.restart = 1;
    if (configuration.app_port <= 0) configuration.app_port = APP_PORT;
    if (configuration.insecure_mode_enabled < 0) configuration.insecure_mode_enabled = 1;
    if (configuration.app_port_secure <= 0) configuration.app_port_secure = APP_PORT_SECURE;
    if (configuration.secure_mode_enabled < 0) configuration.secure_mode_enabled = 0;
    if (configuration.udp_port <= 0) configuration.udp_port = APP_PORT;

    if (configuration.max_text_length <= 0) configuration.max_text_length = MAX_TEXT_LENGTH;
    if (configuration.max_file_size <= 0) configuration.max_file_size = MAX_FILE_SIZE;
    if (configuration.cut_sent_files < 0) configuration.cut_sent_files = 0;
    if (configuration.client_selects_display < 0) configuration.client_selects_display = 0;
    if (configuration.display <= 0) configuration.display = 1;

    if (configuration.min_proto_version < PROTOCOL_MIN) configuration.min_proto_version = PROTOCOL_MIN;
    if (configuration.min_proto_version > PROTOCOL_MAX) configuration.min_proto_version = PROTOCOL_MAX;
    if (configuration.max_proto_version < configuration.min_proto_version ||
        configuration.max_proto_version > PROTOCOL_MAX)
        configuration.max_proto_version = PROTOCOL_MAX;

    if (configuration.method_enabled.get_text < 0) configuration.method_enabled.get_text = 1;
    if (configuration.method_enabled.send_text < 0) configuration.method_enabled.send_text = 1;
    if (configuration.method_enabled.get_files < 0) configuration.method_enabled.get_files = 1;
    if (configuration.method_enabled.send_files < 0) configuration.method_enabled.send_files = 1;
    if (configuration.method_enabled.get_image < 0) configuration.method_enabled.get_image = 1;
    if (configuration.method_enabled.get_copied_image < 0) configuration.method_enabled.get_copied_image = 1;
    if (configuration.method_enabled.get_screenshot < 0) configuration.method_enabled.get_screenshot = 1;
    if (configuration.method_enabled.info < 0) configuration.method_enabled.info = 1;

#ifdef WEB_ENABLED
    if (configuration.web_port <= 0) configuration.web_port = WEB_PORT;
    if (configuration.web_mode_enabled < 0) configuration.web_mode_enabled = 0;
#endif
#if defined(_WIN32) || defined(__APPLE__)
    if (configuration.tray_icon < 0) configuration.tray_icon = 1;
#endif
#ifdef __APPLE__
    if (configuration.bind_addr_udp.af == AF_INET)
        configuration.bind_addr_udp.addr.addr4.s_addr = INADDR_ANY;
    else if (configuration.bind_addr_udp.af == AF_INET6)
        configuration.bind_addr_udp.addr.addr6 = in6addr_any;
#endif
}

#ifdef _WIN32

#define TRAY_CB_MSG (WM_USER + 0x100)

static volatile HINSTANCE instance = NULL;
static volatile HWND hWnd = NULL;
static volatile GUID guid = {0};
static volatile char running = 1;

static DWORD WINAPI udpThreadFn(void *arg) {
    (void)arg;
    udp_server();
    return EXIT_SUCCESS;
}

static DWORD WINAPI appThreadFn(void *arg) {
    (void)arg;
    clip_share(INSECURE);
    return EXIT_SUCCESS;
}

static DWORD WINAPI appSecureThreadFn(void *arg) {
    (void)arg;
    clip_share(SECURE);
    return EXIT_SUCCESS;
}

#ifdef WEB_ENABLED
static DWORD WINAPI webThreadFn(void *arg) {
    (void)arg;
    web_server();
    return EXIT_SUCCESS;
}
#endif

static inline void setGUID(void) {
    char file_path[2048];
    GetModuleFileName(NULL, (char *)file_path, 2048);
    size_t size = strnlen(file_path, 2048);
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < size; i++) {
        h ^= (unsigned char)file_path[i];
        h *= 0x100000001B3ULL;
    }
    guid.Data1 = (unsigned long)h;
    h >>= 32;
    guid.Data2 = (unsigned short)h;
    h >>= 16;
    guid.Data3 = (unsigned short)h;

    h = 0x312bdf6556d47ffdUL;
    for (size_t i = 0; i < size; i++) {
        h ^= (unsigned char)file_path[i];
        h *= 0x100000001B3ULL;
    }
    for (unsigned i = 0; i < 8; i++) {
        guid.Data4[i] = (unsigned char)h;
        h >>= 8;
    }
}

static inline void show_tray_icon(void) {
    NOTIFYICONDATA notifyIconData;
    if (configuration.tray_icon) {
        notifyIconData = (NOTIFYICONDATA){.hWnd = hWnd,
                                          .uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE | NIF_GUID,
                                          .uCallbackMessage = TRAY_CB_MSG,
                                          .uVersion = NOTIFYICON_VERSION_4,
                                          .hIcon = LoadIcon(instance, MAKEINTRESOURCE(APP_ICON)),
                                          .guidItem = guid};
        notifyIconData.cbSize = sizeof(notifyIconData);
        snprintf_check(notifyIconData.szTip, 64, "ClipShare");
        Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
        Shell_NotifyIcon(NIM_ADD, &notifyIconData);
        Shell_NotifyIcon(NIM_SETVERSION, &notifyIconData);
    }
}

static inline void remove_tray_icon(void) {
    NOTIFYICONDATA notifyIconData = {
        .cbSize = sizeof(NOTIFYICONDATA), .hWnd = NULL, .uFlags = NIF_GUID, .guidItem = guid};
    Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
    if (hWnd) DestroyWindow(hWnd);
}

static LRESULT CALLBACK WindowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_QUERYENDSESSION:
        case WM_COMMAND: {
            remove_tray_icon();
            running = 0;
            break;
        }
        case TRAY_CB_MSG: {
            switch (LOWORD(lParam)) {
                case NIN_SELECT:
                case NIN_KEYSELECT:
                case WM_CONTEXTMENU: {
                    POINT pt;
                    GetCursorPos(&pt);
                    HMENU hmenu = CreatePopupMenu();
                    InsertMenu(hmenu, 0, MF_BYPOSITION | MF_STRING, 100, TEXT("Stop"));
                    SetForegroundWindow(window);
                    TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, window,
                                   NULL);
                    PostMessage(window, WM_NULL, 0, 50);
                    return 0;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    return DefWindowProc(window, msg, wParam, lParam);
}

static char *get_user_home(void) {
    DWORD pid = GetCurrentProcessId();
    HANDLE procHndl = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    HANDLE token;
    if (!OpenProcessToken(procHndl, TOKEN_QUERY, &token)) {
        CloseHandle(procHndl);
        return NULL;
    }
    DWORD wlen;
    GetUserProfileDirectoryW(token, NULL, &wlen);
    CloseHandle(token);
    if (wlen >= 512) return NULL;
    wchar_t whome[wlen];
    if (!OpenProcessToken(procHndl, TOKEN_QUERY, &token)) {
        CloseHandle(procHndl);
        return NULL;
    }
    if (!GetUserProfileDirectoryW(token, whome, &wlen)) {
        CloseHandle(procHndl);
        CloseHandle(token);
        return NULL;
    }
    CloseHandle(token);
    CloseHandle(procHndl);
    char *home = NULL;
    uint32_t len;
    if (!wchar_to_utf8_str(whome, &home, &len) == EXIT_SUCCESS) return NULL;
    if (len >= 512 && home) {
        free(home);
        return NULL;
    }
    return home;
}

#endif

#if defined(__linux__) || defined(__APPLE__)

static char *get_user_home(void) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd pw;
        struct passwd *result = NULL;
        const size_t buf_sz = 2048;
        char buf[buf_sz];
        if (getpwuid_r(getuid(), &pw, buf, buf_sz, &result) || result == NULL) return NULL;
        home = result->pw_dir;
    }
    if (home) return strndup(home, 512);
    return NULL;
}

__attribute__((noreturn)) static void exit_on_signal_handler(int sig) {
    (void)sig;
    exit(0);
}

#endif

static char *get_conf_file(void) {
    if (file_exists(CONFIG_FILE)) return strdup(CONFIG_FILE);

#if defined(__linux__) || defined(__APPLE__)
    const char *xdg_conf = getenv("XDG_CONFIG_HOME");
    if (xdg_conf && *xdg_conf) {
        size_t xdg_len = strnlen(xdg_conf, 512);
        char *conf_path = malloc(xdg_len + sizeof(CONFIG_FILE) + 3);
        if (conf_path) {
            snprintf(conf_path, xdg_len + sizeof(CONFIG_FILE) + 2, "%s%c%s", xdg_conf, PATH_SEP, CONFIG_FILE);
            if (file_exists(conf_path)) return conf_path;
            free(conf_path);
        }
    }
#endif

    char *home = get_user_home();
    if (!home) return NULL;
    size_t home_len = strnlen(home, 512);
    char *conf_path = realloc(home, home_len + sizeof(CONFIG_FILE) + 3);
    if (!conf_path) {
        free(home);
        return NULL;
    }
    snprintf(conf_path + home_len, sizeof(CONFIG_FILE) + 2, "%c%s", PATH_SEP, CONFIG_FILE);
    return conf_path;
}

/*
 * The main entrypoint of the application
 */
int main(int argc, char **argv) {
    // Get basename of the program
    const char *prog_name = strrchr(argv[0], PATH_SEP);
    if (!prog_name) {
        prog_name = argv[0];
    } else {
        prog_name++;  // don't want the '/' before the program name
    }
#ifdef __APPLE__
    global_prog_name = prog_name;
#endif

    atexit(cleanup);

#if defined(__linux__) || defined(__APPLE__)
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, &exit_on_signal_handler);
    signal(SIGTERM, &exit_on_signal_handler);
    signal(SIGSEGV, &exit_on_signal_handler);
    signal(SIGABRT, &exit_on_signal_handler);
    signal(SIGQUIT, &exit_on_signal_handler);
    signal(SIGSYS, &exit_on_signal_handler);
    signal(SIGHUP, &exit_on_signal_handler);
    signal(SIGBUS, &exit_on_signal_handler);
#endif

    _set_error_log_file(ERROR_LOG_FILE);

#ifdef _WIN32
    // initialize instance and guid
    instance = GetModuleHandle(NULL);
    setGUID();
#endif

    char *conf_path = get_conf_file();
    if (!conf_path) {
        exit(EXIT_FAILURE);
    }
    parse_conf(&configuration, conf_path);
    free(conf_path);
    _apply_default_conf();

    int stop = 0;
    int daemonize = 1;
    // Parse command line arguments
    _parse_args(argc, argv, &stop, &daemonize);

    /* stop other instances of this process if any.
    Stop this process if stop flag is set */
    if (stop || configuration.restart) {
#ifdef _WIN32
        remove_tray_icon();
#endif
        kill_other_processes(prog_name);
        const char *msg = stop ? "Server Stopped" : "Server Restarting...";
        puts(msg);
        if (stop) {
            exit(EXIT_SUCCESS);
        }
    }
#if defined(_WIN32) && defined(DEBUG_MODE)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

    if (configuration.working_dir) _change_working_dir();
    cwd = getcwd_wrapper(0);
    cwd_len = strnlen(cwd, 2048);

#ifdef NO_SSL
    if (configuration.secure_mode_enabled) {
        const char *message =
            "Secure mode is enabled in the configuration. This program version does not support TLS/SSL.";
        fprintf(stderr,
                "%s If the secure mode is not needed, please set secure_mode_enabled=false in the configuration.\n",
                message);
        error_exit(message);
    }
#endif

#if defined(__linux__) || defined(__APPLE__)
    pid_t p_clip = 0;
    pid_t p_clip_ssl = 0;
#ifdef WEB_ENABLED
    pid_t p_web = 0;
#endif
    if (configuration.insecure_mode_enabled) {
        fflush(stdout);
        fflush(stderr);
        p_clip = fork();
        if (p_clip == 0) {
            int status = clip_share(INSECURE);
            exit(status);
        }
    }
    if (configuration.secure_mode_enabled) {
        fflush(stdout);
        fflush(stderr);
        p_clip_ssl = fork();
        if (p_clip_ssl == 0) {
            int status = clip_share(SECURE);
            exit(status);
        }
    }
#ifdef WEB_ENABLED
    if (configuration.web_mode_enabled) {
        fflush(stdout);
        fflush(stderr);
        p_web = fork();
        if (p_web == 0) {
            int status = web_server();
            exit(status);
        }
    }
#endif
    puts("Server Started");
    fflush(stdout);
    fflush(stderr);
    pid_t p_scan = fork();
    if (p_scan == 0) {
        udp_server();
        exit(EXIT_SUCCESS);
    }

#ifdef __APPLE__
    if (configuration.tray_icon) {
        fflush(stdout);
        fflush(stderr);
        pid_t p_menu = fork();
        if (p_menu == 0) {
            show_menu_icon();
            exit(EXIT_SUCCESS);
        }
    }
#endif

    if (!daemonize) {
        if (p_clip > 0) waitpid(p_clip, NULL, 0);
        if (p_clip_ssl > 0) waitpid(p_clip_ssl, NULL, 0);
        if (p_scan > 0) waitpid(p_scan, NULL, 0);
#ifdef WEB_ENABLED
        if (p_web > 0) waitpid(p_web, NULL, 0);
#endif
    }

#elif defined(_WIN32)

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error_exit("Failed WSAStartup");
    }
    HANDLE insecureThread = NULL;
    HANDLE secureThread = NULL;
#ifdef WEB_ENABLED
    HANDLE webThread = NULL;
#endif
    if (configuration.insecure_mode_enabled) {
        insecureThread = CreateThread(NULL, 0, appThreadFn, NULL, 0, NULL);
#ifdef DEBUG_MODE
        if (insecureThread == NULL) {
            error("App thread creation failed");
        }
#endif
    }

    if (configuration.secure_mode_enabled) {
        secureThread = CreateThread(NULL, 0, appSecureThreadFn, NULL, 0, NULL);
#ifdef DEBUG_MODE
        if (secureThread == NULL) {
            error("App Secure thread creation failed");
        }
#endif
    }

#ifdef WEB_ENABLED
    if (configuration.web_mode_enabled) {
        webThread = CreateThread(NULL, 0, webThreadFn, NULL, 0, NULL);
#ifdef DEBUG_MODE
        if (webThread == NULL) {
            error("Web thread creation failed");
        }
#endif
    }
#endif

    if (configuration.tray_icon) {
        char CLASSNAME[] = "clip";
        WNDCLASS wc = {.lpfnWndProc = (WNDPROC)WindowProc, .hInstance = instance, .lpszClassName = CLASSNAME};
        RegisterClass(&wc);
        hWnd = CreateWindowEx(0, CLASSNAME, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, instance, NULL);
        show_tray_icon();
    }

    HANDLE udpThread = CreateThread(NULL, 0, udpThreadFn, NULL, 0, NULL);
    if (udpThread == NULL) {
#ifdef DEBUG_MODE
        error("UDP thread creation failed");
#endif
    }

    if (configuration.tray_icon) {
        MSG msg = {};
        while (running && GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (insecureThread != NULL) TerminateThread(insecureThread, 0);
        if (secureThread != NULL) TerminateThread(secureThread, 0);
#ifdef WEB_ENABLED
        if (webThread != NULL) TerminateThread(webThread, 0);
#endif
    }

    if (insecureThread != NULL) WaitForSingleObject(insecureThread, INFINITE);
    if (secureThread != NULL) WaitForSingleObject(secureThread, INFINITE);
#ifdef WEB_ENABLED
    if (webThread != NULL) WaitForSingleObject(webThread, INFINITE);
#endif

    remove_tray_icon();
    CloseHandle(instance);
#endif
    return 0;
}

/*
 * utils/win_load_lib.c - load dynamic libraries at runtime on Windows
 * Copyright (C) 2025 H. Thevindu J. Wijesekera
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

#ifdef _WIN64

// clang-format off
#include <windows.h>
// clang-format on
#include <shellapi.h>
#include <stdio.h>
#include <utils/win_load_lib.h>

extern FARPROC __imp_GetUserProfileDirectoryW;

extern FARPROC __imp_Shell_NotifyIconA;
extern FARPROC __imp_DragQueryFileW;

extern FARPROC __imp_OpenProcessToken;

#ifndef NO_SSL
extern FARPROC __imp_CryptAcquireContextW;
extern FARPROC __imp_CryptCreateHash;
extern FARPROC __imp_CryptDecrypt;
extern FARPROC __imp_CryptDestroyHash;
extern FARPROC __imp_CryptDestroyKey;
extern FARPROC __imp_CryptEnumProvidersW;
extern FARPROC __imp_CryptExportKey;
extern FARPROC __imp_CryptGenRandom;
extern FARPROC __imp_CryptGetProvParam;
extern FARPROC __imp_CryptGetUserKey;
extern FARPROC __imp_CryptReleaseContext;
extern FARPROC __imp_CryptSetHashParam;
extern FARPROC __imp_CryptSignHashW;
extern FARPROC __imp_DeregisterEventSource;
extern FARPROC __imp_RegisterEventSourceW;
extern FARPROC __imp_ReportEventW;
#endif

static HMODULE module_userenv = NULL;
static HMODULE module_shell32 = NULL;
static HMODULE module_advapi32 = NULL;

FARPROC __imp_GetUserProfileDirectoryW = NULL;
FARPROC __imp_Shell_NotifyIconA = NULL;
FARPROC __imp_DragQueryFileW = NULL;

FARPROC __imp_OpenProcessToken = NULL;

#ifndef NO_SSL
FARPROC __imp_CryptAcquireContextW = NULL;
FARPROC __imp_CryptCreateHash = NULL;
FARPROC __imp_CryptDecrypt = NULL;
FARPROC __imp_CryptDestroyHash = NULL;
FARPROC __imp_CryptDestroyKey = NULL;
FARPROC __imp_CryptEnumProvidersW = NULL;
FARPROC __imp_CryptExportKey = NULL;
FARPROC __imp_CryptGenRandom = NULL;
FARPROC __imp_CryptGetProvParam = NULL;
FARPROC __imp_CryptGetUserKey = NULL;
FARPROC __imp_CryptReleaseContext = NULL;
FARPROC __imp_CryptSetHashParam = NULL;
FARPROC __imp_CryptSignHashW = NULL;
FARPROC __imp_DeregisterEventSource = NULL;
FARPROC __imp_RegisterEventSourceW = NULL;
FARPROC __imp_ReportEventW = NULL;
#endif

static inline HMODULE LoadLibWrapper(const char *dll) {
    HMODULE module = LoadLibraryA(dll);
    if (!module) {
        fprintf(stderr, "Failed to load DLL %s\n", dll);
        return NULL;
    }
    return module;
}

static inline FARPROC GetProcAddressWrapper(HMODULE module, const char *funcName) {
    FARPROC func = GetProcAddress(module, funcName);
    if (!func) {
        fprintf(stderr, "Failed to load function %s\n", funcName);
        return NULL;
    }
    return func;
}

static int _load_libs(void) {
    module_userenv = LoadLibWrapper("USERENV.DLL");
    if (!module_userenv) return EXIT_FAILURE;

    __imp_GetUserProfileDirectoryW = GetProcAddressWrapper(module_userenv, "GetUserProfileDirectoryW");
    if (!__imp_GetUserProfileDirectoryW) return EXIT_FAILURE;

    module_shell32 = LoadLibWrapper("SHELL32.DLL");
    if (!module_shell32) return EXIT_FAILURE;

    __imp_Shell_NotifyIconA = GetProcAddressWrapper(module_shell32, "Shell_NotifyIconA");
    if (!__imp_Shell_NotifyIconA) return EXIT_FAILURE;

    __imp_DragQueryFileW = GetProcAddressWrapper(module_shell32, "DragQueryFileW");
    if (!__imp_DragQueryFileW) return EXIT_FAILURE;

    module_advapi32 = LoadLibWrapper("ADVAPI32.DLL");
    if (!module_advapi32) return EXIT_FAILURE;

    __imp_OpenProcessToken = GetProcAddressWrapper(module_advapi32, "OpenProcessToken");
    if (!__imp_OpenProcessToken) return EXIT_FAILURE;

#ifndef NO_SSL
    __imp_CryptAcquireContextW = GetProcAddressWrapper(module_advapi32, "CryptAcquireContextW");
    if (!__imp_CryptAcquireContextW) return EXIT_FAILURE;

    __imp_CryptCreateHash = GetProcAddressWrapper(module_advapi32, "CryptCreateHash");
    if (!__imp_CryptCreateHash) return EXIT_FAILURE;

    __imp_CryptDecrypt = GetProcAddressWrapper(module_advapi32, "CryptDecrypt");
    if (!__imp_CryptDecrypt) return EXIT_FAILURE;

    __imp_CryptDestroyHash = GetProcAddressWrapper(module_advapi32, "CryptDestroyHash");
    if (!__imp_CryptDestroyHash) return EXIT_FAILURE;

    __imp_CryptDestroyKey = GetProcAddressWrapper(module_advapi32, "CryptDestroyKey");
    if (!__imp_CryptDestroyKey) return EXIT_FAILURE;

    __imp_CryptEnumProvidersW = GetProcAddressWrapper(module_advapi32, "CryptEnumProvidersW");
    if (!__imp_CryptEnumProvidersW) return EXIT_FAILURE;

    __imp_CryptExportKey = GetProcAddressWrapper(module_advapi32, "CryptExportKey");
    if (!__imp_CryptExportKey) return EXIT_FAILURE;

    __imp_CryptGenRandom = GetProcAddressWrapper(module_advapi32, "CryptGenRandom");
    if (!__imp_CryptGenRandom) return EXIT_FAILURE;

    __imp_CryptGetProvParam = GetProcAddressWrapper(module_advapi32, "CryptGetProvParam");
    if (!__imp_CryptGetProvParam) return EXIT_FAILURE;

    __imp_CryptGetUserKey = GetProcAddressWrapper(module_advapi32, "CryptGetUserKey");
    if (!__imp_CryptGetUserKey) return EXIT_FAILURE;

    __imp_CryptReleaseContext = GetProcAddressWrapper(module_advapi32, "CryptReleaseContext");
    if (!__imp_CryptReleaseContext) return EXIT_FAILURE;

    __imp_CryptSetHashParam = GetProcAddressWrapper(module_advapi32, "CryptSetHashParam");
    if (!__imp_CryptSetHashParam) return EXIT_FAILURE;

    __imp_CryptSignHashW = GetProcAddressWrapper(module_advapi32, "CryptSignHashW");
    if (!__imp_CryptSignHashW) return EXIT_FAILURE;

    __imp_DeregisterEventSource = GetProcAddressWrapper(module_advapi32, "DeregisterEventSource");
    if (!__imp_DeregisterEventSource) return EXIT_FAILURE;

    __imp_RegisterEventSourceW = GetProcAddressWrapper(module_advapi32, "RegisterEventSourceW");
    if (!__imp_RegisterEventSourceW) return EXIT_FAILURE;

    __imp_ReportEventW = GetProcAddressWrapper(module_advapi32, "ReportEventW");
    if (!__imp_ReportEventW) return EXIT_FAILURE;
#endif

    return EXIT_SUCCESS;
}

int load_libs(void) {
    SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (_load_libs() != EXIT_SUCCESS) {
        cleanup_libs();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static inline void clean_lib(HMODULE *module_p) {
    if (*module_p) {
        FreeLibrary(*module_p);
        *module_p = NULL;
    }
}

void cleanup_libs(void) {
    clean_lib(&module_userenv);
    clean_lib(&module_shell32);
    clean_lib(&module_advapi32);
}

#endif

/*
 *  conf_parse.c - parse config file for application
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

#include <config.h>
#include <globals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/list_utils.h>
#include <utils/net_utils.h>
#include <utils/utils.h>

#define LINE_MAX_LEN 2047

/*
 * Trims all charactors in the range \x01 to \x20 inclusive from both ends of
 * the string in-place.
 * The string must point to a valid and null-terminated string.
 * This does not re-allocate memory to shrink.
 */
static inline void trim(char *str) {
    const char *ptr = str;
    while (0 < *ptr && *ptr <= ' ') {
        ptr++;
    }
    char *p1 = str;
    while (*ptr) {
        *p1 = *ptr;
        p1++;
        ptr++;
    }
    *p1 = 0;
    if (*str == 0) return;
    p1--;
    while (0 < *p1 && *p1 <= ' ') {
        *p1 = 0;
        p1--;
    }
}

/*
 * Reads the list of client names from the file given by the filename.
 * Allowed client names must not exceed 511 characters.
 * Returns a list2* of null terminated strings as elements on success.
 * Returns null on error.
 */
static inline list2 *get_client_list(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;
    list2 *client_list = init_list(1);
    char client[512];
    while (fscanf(f, "%511[^\n]%*c", client) != EOF) {
        client[511] = 0;
        trim(client);
        size_t len = strnlen(client, 512);
        if (len < 1) continue;
        if (client[0] == '#') continue;
#ifdef DEBUG_MODE
        printf("Client : %s\n", client);
#endif
        append(client_list, strdup(client));
    }
    fclose(f);
    return client_list;
}

/*
 * Loads the content of a file given by the file_name and set the buffer to the address pointed by ptr.
 * File must not be empty and its size must be less than 64 KiB.
 * ptr must be a valid pointer to a char*.
 * If the char* pointed by ptr is not null. This will call free() on that char*
 * Sets the malloced block of memory containing a null-terminated file content to the address pointed by ptr.
 * Does not modify the ptr if an error occured.
 * Note that if the file contained null byte in it, the length of the string may be smaller than the allocated memory
 * block.
 */
static inline void load_file(const char *file_name, char **ptr) {
    if (!file_name) return;
    FILE *file_ptr = fopen(file_name, "rb");
    if (!file_ptr) return;
    ssize_t len = get_file_size(file_ptr);
    if (len <= 0 || 65536 < len) {
        fclose(file_ptr);
        return;
    }
    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(file_ptr);
        return;
    }
    ssize_t sz = (ssize_t)fread(buf, 1, (size_t)len, file_ptr);
    if (sz < len) {
        fclose(file_ptr);
        free(buf);
        return;
    }
    buf[len] = 0;
    if (*ptr) free(*ptr);
    *ptr = buf;
    fclose(file_ptr);
}

/*
 * str must be a valid and null-terminated string
 * conf_ptr must be a valid pointer to a char
 * Sets the value pointed by conf_ptr to 1 if the string is "true" or "1".
 * Sets the value pointed by conf_ptr to 0 if the string is "false" or "0".
 * Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_is_true(const char *str, char *conf_ptr) {
    if (!strcasecmp("true", str) || !strcmp("1", str)) {
        *conf_ptr = 1;
    } else if (!strcasecmp("false", str) || !strcmp("0", str)) {
        *conf_ptr = 0;
    }
}

/*
 * str must be a valid and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned int
 * Sets the value pointed by conf_ptr to the unsigned int given as a string in str if that is a valid value between 1
 * and 2^32-1 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_uint(const char *str, unsigned int *conf_ptr) {
    long long value = strtoll(str, NULL, 10);
    if (0 < value && value <= 4294967295L) {
        *conf_ptr = (unsigned int)value;
    }
}

/*
 * str must be a valid and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned short
 * Sets the value pointed by conf_ptr to the unsigned short given as a string in str if that is a valid value between 1
 * and 65535 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_ushort(const char *str, unsigned short *conf_ptr) {
    long value = strtol(str, NULL, 10);
    if (0 < value && value < 65536) {
        *conf_ptr = (unsigned short)value;
    }
}

/*
 * Parse a single line in the config file and update the config if the line
 * contained a valid configuration.
 */
static void parse_line(char *line, config *cfg) {
    char *eq = strchr(line, '=');
    if (!eq) {
        return;
    }
    *eq = 0;
    char *key = line;
    char *value = eq + 1;

    trim(key);
    trim(value);

    if (key[0] == '#') return;

    const size_t key_len = strnlen(key, LINE_MAX_LEN);
    if (key_len <= 0 || key_len >= LINE_MAX_LEN) return;

    const size_t value_len = strnlen(value, LINE_MAX_LEN);
    if (value_len <= 0 || value_len >= LINE_MAX_LEN) return;

#ifdef DEBUG_MODE
    printf("Key=%s : Value=%s\n", key, value);
#endif

    if (!strcmp("app_port", key)) {
        set_ushort(value, &(cfg->app_port));
    } else if (!strcmp("insecure_mode_enabled", key)) {
        set_is_true(value, &(cfg->insecure_mode_enabled));
    } else if (!strcmp("app_port_secure", key)) {
        set_ushort(value, &(cfg->app_port_secure));
    } else if (!strcmp("secure_mode_enabled", key)) {
        set_is_true(value, &(cfg->secure_mode_enabled));
    } else if (!strcmp("udp_port", key)) {
        set_ushort(value, &(cfg->udp_port));
#ifndef NO_WEB
    } else if (!strcmp("web_port", key)) {
        set_ushort(value, &(cfg->web_port));
    } else if (!strcmp("web_mode_enabled", key)) {
        set_is_true(value, &(cfg->web_mode_enabled));
#endif
    } else if (!strcmp("server_key", key)) {
        load_file(value, &(cfg->priv_key));
    } else if (!strcmp("server_cert", key)) {
        load_file(value, &(cfg->server_cert));
    } else if (!strcmp("ca_cert", key)) {
        load_file(value, &(cfg->ca_cert));
    } else if (!strcmp("allowed_clients", key)) {
        list2 *client_list = get_client_list(value);
        if (client_list == NULL) return;
        if (cfg->allowed_clients) free_list(cfg->allowed_clients);
        cfg->allowed_clients = client_list;
    } else if (!strcmp("working_dir", key)) {
        if (cfg->working_dir) free(cfg->working_dir);
        cfg->working_dir = strdup(value);
    } else if (!strcmp("bind_address", key) && ipv4_aton(value, &(cfg->bind_addr)) != EXIT_SUCCESS) {
        char msg[48];
        snprintf_check(msg, 48, "Invalid bind address %s", value);
        error_exit(msg);
    } else if (!strcmp("restart", key)) {
        set_is_true(value, &(cfg->restart));
    } else if (!strcmp("max_text_length", key)) {
        set_uint(value, &(cfg->max_text_length));
#ifdef _WIN32
    } else if (!strcmp("tray_icon", key)) {
        set_is_true(value, &(cfg->tray_icon));
    } else if (!strcmp("display", key)) {
        set_ushort(value, &(cfg->display));
#endif
#ifdef DEBUG_MODE
    } else {
        printf("Unknown key \"%s\"\n", key);
#endif
    }
}

void parse_conf(config *cfg, const char *file_name) {
    cfg->app_port = 0;
    cfg->insecure_mode_enabled = -1;

    cfg->app_port_secure = 0;
    cfg->secure_mode_enabled = -1;

    cfg->udp_port = 0;

#ifndef NO_WEB
    cfg->web_port = 0;
    cfg->web_mode_enabled = -1;
#endif

    cfg->priv_key = NULL;
    cfg->server_cert = NULL;
    cfg->ca_cert = NULL;
    cfg->allowed_clients = NULL;
    cfg->working_dir = NULL;
    cfg->restart = -1;
    cfg->max_text_length = 0;
#ifdef _WIN32
    cfg->tray_icon = -1;
    cfg->display = 0;
#endif
    if (ipv4_aton(NULL, &(cfg->bind_addr)) != EXIT_SUCCESS) error_exit("Error initializing bind address");

    if (!file_name) {
#ifdef DEBUG_MODE
        printf("File name is null\n");
#endif
        puts("Using default configurations");
        return;
    }

    FILE *f = fopen(file_name, "r");
    if (!f) {
#ifdef DEBUG_MODE
        printf("Error opening conf file\n");
#endif
        puts("Using default configurations");
        return;
    }

    char line[LINE_MAX_LEN + 1];
    while (fscanf(f, "%2047[^\n]%*c", line) != EOF) {
        line[LINE_MAX_LEN] = 0;
        parse_line(line, cfg);
    }
    fclose(f);

    return;
}

void clear_config(config *cfg) {
    if (cfg->priv_key) {
        size_t len = strnlen(cfg->priv_key, 65536);
        memset(cfg->priv_key, 0, len);
        free(cfg->priv_key);
    }
    if (cfg->server_cert) {
        free(cfg->server_cert);
    }
    if (cfg->ca_cert) {
        free(cfg->ca_cert);
    }
    if (cfg->working_dir) {
        free(cfg->working_dir);
    }
    if (cfg->allowed_clients) {
        free_list(cfg->allowed_clients);
    }
}

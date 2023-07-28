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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/utils.h"
#include "utils/net_utils.h"
#include "utils/list_utils.h"
#include "config.h"

/*
 * Trims all charactors in the range \x01 to \x20 inclusive from both ends of
 * the string in-place.
 * The string must point to a valid and null-terminated string.
 * This does not re-allocate memory to shrink.
 */
static void trim(char *string)
{
    const char *ptr = string;
    while (0 < *ptr && *ptr <= ' ')
    {
        ptr++;
    }
    char *p1 = string;
    while (*ptr)
    {
        *p1 = *ptr;
        p1++;
        ptr++;
    }
    *p1 = 0;
    if (*string == 0)
        return;
    p1--;
    while (0 < *p1 && *p1 <= ' ')
    {
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
static list2 *get_client_list(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
        return NULL;
    list2 *client_list = init_list(1);
    char client[512];
    while (fscanf(f, "%511[^\n]%*c", client) != EOF)
    {
        trim(client);
        size_t len = strlen(client);
        if (len < 1)
            continue;
        if (client[0] == '#')
            continue;
#ifdef DEBUG_MODE
        printf("Client : %s\n", client);
#endif
        append(client_list, strdup(client));
    }
    fclose(f);
    return client_list;
}

/*
 * Loads the content of a file given by the file_name.
 * File must not be empty and its size must be less than 64 KiB
 * Returns the malloced block of memory containing a null-terminated file content.
 * Returns null on error.
 * Note that if the file contained null byte in it, the length of the returned
 * string may be smaller than the allocated memory block.
 */
static char *load_file(const char *file_name)
{
    if (!file_name)
        return NULL;
    FILE *file_ptr = fopen(file_name, "rb");
    if (!file_ptr)
        return NULL;
    ssize_t len = get_file_size(file_ptr);
    if (len <= 0 || 65536 < len)
    {
        fclose(file_ptr);
        return NULL;
    }
    char *buf = (char *)malloc(len + 1);
    if (!buf)
    {
        fclose(file_ptr);
        return NULL;
    }
    ssize_t sz = (ssize_t)fread(buf, 1, len, file_ptr);
    if (sz < len)
    {
        fclose(file_ptr);
        free(buf);
        return NULL;
    }
    buf[len] = 0;
    fclose(file_ptr);
    return buf;
}

/*
 * str must be a valid and null-terminated string
 * Returns 1 if the string is "true" or "1".
 * Returns 0 if the string is "false" or "0".
 * Returns -1 otherwise.
 */
static inline char is_true_str(const char *str)
{
    if (!strcasecmp("true", str) || !strcmp("1", str))
    {
        return 1;
    }
    else if (!strcasecmp("false", str) || !strcmp("0", str))
    {
        return 0;
    }
    return -1;
}

/*
 * Parse a single line in the config file and update the config if the line
 * contained a valid configuration.
 */
static void parse_line(char *line, config *cfg)
{
    char *eq = strchr(line, '=');
    if (!eq)
    {
        return;
    }
    *eq = 0;
    char *key = line;
    char *value = eq + 1;

    trim(key);
    trim(value);

#ifdef DEBUG_MODE
    printf("Key=%s : Value=%s\n", key, value);
#endif

    if (!strcmp("app_port", key))
    {
        long port = strtol(value, NULL, 10);
        if (0 < port && port < 65536)
        {
            cfg->app_port = (unsigned short)port;
        }
    }
    else if (!strcmp("insecure_mode_enabled", key))
    {
        char is_true = is_true_str(value);
        if (is_true >= 0)
        {
            cfg->insecure_mode_enabled = is_true;
        }
    }
    else if (!strcmp("app_port_secure", key))
    {
        long port = strtol(value, NULL, 10);
        if (0 < port && port < 65536)
        {
            cfg->app_port_secure = (unsigned short)port;
        }
    }
    else if (!strcmp("secure_mode_enabled", key))
    {
        char is_true = is_true_str(value);
        if (is_true >= 0)
        {
            cfg->secure_mode_enabled = is_true;
        }
    }
#ifndef NO_WEB
    else if (!strcmp("web_port", key))
    {
        long port = strtol(value, NULL, 10);
        if (0 < port && port < 65536)
        {
            cfg->web_port = (unsigned short)port;
        }
    }
    else if (!strcmp("web_mode_enabled", key))
    {
        char is_true = is_true_str(value);
        if (is_true >= 0)
        {
            cfg->web_mode_enabled = is_true;
        }
    }
#endif
    else if (!strcmp("server_key", key))
    {
        char *buf = load_file(value);
        if (cfg->priv_key)
            free(cfg->priv_key);
        cfg->priv_key = buf;
    }
    else if (!strcmp("server_cert", key))
    {
        char *buf = load_file(value);
        if (cfg->server_cert)
            free(cfg->server_cert);
        cfg->server_cert = buf;
    }
    else if (!strcmp("ca_cert", key))
    {
        char *buf = load_file(value);
        if (cfg->ca_cert)
            free(cfg->ca_cert);
        cfg->ca_cert = buf;
    }
    else if (!strcmp("allowed_clients", key))
    {
        list2 *client_list = get_client_list(value);
        if (client_list)
        {
            if (cfg->allowed_clients)
                free_list(cfg->allowed_clients);
            cfg->allowed_clients = client_list;
        }
    }
    else if (!strcmp("working_dir", key))
    {
        if (strlen(value) > 0)
        {
            if (cfg->working_dir)
                free(cfg->working_dir);
            cfg->working_dir = strdup(value);
        }
    }
    else if (!strcmp("bind_address", key))
    {
        if (strlen(value) > 0 && ipv4_aton(value, &(cfg->bind_addr)) != EXIT_SUCCESS)
        {
            char msg[48];
            snprintf_check(msg, 48, "Invalid bind address %s", value);
            error_exit(msg);
        }
    }
#ifdef DEBUG_MODE
    else
    {
        printf("Unknown key \"%s\"\n", key);
    }
#endif
}

config parse_conf(const char *file_name)
{
    config cfg;
    cfg.app_port = 0;
    cfg.insecure_mode_enabled = -1;

    cfg.app_port_secure = 0;
    cfg.secure_mode_enabled = -1;

#ifndef NO_WEB
    cfg.web_port = 0;
    cfg.web_mode_enabled = -1;
#endif

    cfg.priv_key = NULL;
    cfg.server_cert = NULL;
    cfg.ca_cert = NULL;
    cfg.allowed_clients = NULL;

    cfg.working_dir = NULL;
    if (ipv4_aton(NULL, &(cfg.bind_addr)) != EXIT_SUCCESS)
    {
        error_exit("Error initializing bind address");
    }

    if (!file_name)
    {
#ifdef DEBUG_MODE
        printf("File name is null\n");
#endif
        puts("Using default configurations");
        return cfg;
    }

    FILE *f = fopen(file_name, "r");
    if (!f)
    {
#ifdef DEBUG_MODE
        printf("Error opening conf file\n");
#endif
        puts("Using default configurations");
        return cfg;
    }

    char line[2048];
    line[2047] = 0;
    while (fscanf(f, "%2047[^\n]%*c", line) != EOF)
    {
        parse_line(line, &cfg);
    }
    fclose(f);

    return cfg;
}

void clear_config(config *cfg)
{
    if (cfg->priv_key)
    {
        size_t len = strlen(cfg->priv_key);
        memset(cfg->priv_key, 0, len);
        free(cfg->priv_key);
    }
    if (cfg->server_cert)
    {
        free(cfg->server_cert);
    }
    if (cfg->ca_cert)
    {
        free(cfg->ca_cert);
    }
    if (cfg->working_dir)
    {
        free(cfg->working_dir);
    }
    if (cfg->allowed_clients)
    {
        free_list(cfg->allowed_clients);
    }
}
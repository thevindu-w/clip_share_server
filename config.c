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

static void trim(char *string)
{
    char *ptr = string;
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

static char *load_file(FILE *f)
{
    ssize_t len = get_file_size(f);
    if (len <= 0 || 65536 < len)
    {
        fclose(f);
        return NULL;
    }
    char *buf = (char *)malloc(len + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    ssize_t sz = (ssize_t)fread(buf, 1, len, f);
    if (sz < len)
    {
        fclose(f);
        free(buf);
        return NULL;
    }
    buf[len] = 0;
    fclose(f);
    return buf;
}

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
        FILE *f = fopen(value, "rb");
        if (!f)
            return;
        char *buf = load_file(f);
        if (cfg->priv_key)
            free(cfg->priv_key);
        cfg->priv_key = buf;
    }
    else if (!strcmp("server_cert", key))
    {
        FILE *f = fopen(value, "rb");
        if (!f)
            return;
        char *buf = load_file(f);
        if (cfg->server_cert)
            free(cfg->server_cert);
        cfg->server_cert = buf;
    }
    else if (!strcmp("ca_cert", key))
    {
        FILE *f = fopen(value, "rb");
        if (!f)
            return;
        char *buf = load_file(f);
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
            if (cfg->working_dir)
                free(cfg->working_dir);
        cfg->working_dir = strdup(value);
    }
    else if (!strcmp("bind_address", key))
    {
        if (strlen(value) > 0)
        {
            if (ipv4_aton(value, &(cfg->bind_addr)) != EXIT_SUCCESS)
            {
                char msg[48];
                snprintf_check(msg, 48, "Invalid bind address %s", value);
                error_exit(msg);
            }
        }
    }
#ifdef DEBUG_MODE
    else
    {
        printf("Unknown key \"%s\"\n", key);
    }
#endif
}

config parse_conf(const char *conf_file)
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

    FILE *f = fopen(conf_file, "r");
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
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
    char *ptr;
    ptr = string;
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
    char *buf = (char *)malloc(len);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }
    ssize_t sz = (ssize_t)fread(buf, 1, len, f);
    if (sz < len)
    {
        free(buf);
        return NULL;
    }
    fclose(f);
    return buf;
}

static void parse_line(char *line, config *cfg)
{
    char *eq = strchr(line, '=');
    if (!eq)
    {
        return;
    }
    *eq = 0;
    char key[256];
    sscanf(line, "%255[^\n]", key);
    char value[2048];
    strcpy(value, eq + 1);

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
        if (!strcasecmp("true", value) || !strcmp("1", value))
        {
            cfg->insecure_mode_enabled = 1;
        }
        else if (!strcasecmp("false", value) || !strcmp("0", value))
        {
            cfg->insecure_mode_enabled = 0;
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
        if (!strcasecmp("true", value) || !strcmp("1", value))
        {
            cfg->secure_mode_enabled = 1;
        }
        else if (!strcasecmp("false", value) || !strcmp("0", value))
        {
            cfg->secure_mode_enabled = 0;
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
        if (!strcasecmp("true", value) || !strcmp("1", value))
        {
            cfg->web_mode_enabled = 1;
        }
        else if (!strcasecmp("false", value) || !strcmp("0", value))
        {
            cfg->web_mode_enabled = 0;
        }
    }
#endif
    else if (!strcmp("server_key", key))
    {
        FILE *f = fopen(value, "rb");
        if (!f)
            return;
        char *buf = load_file(f);
        cfg->priv_key = buf;
    }
    else if (!strcmp("server_cert", key))
    {
        FILE *f = fopen(value, "rb");
        if (!f)
            return;
        char *buf = load_file(f);
        cfg->server_cert = buf;
    }
    else if (!strcmp("ca_cert", key))
    {
        FILE *f = fopen(value, "rb");
        if (!f)
            return;
        char *buf = load_file(f);
        cfg->ca_cert = buf;
    }
    else if (!strcmp("allowed_clients", key))
    {
        list2 *client_list = get_client_list(value);
        if (client_list)
            cfg->allowed_clients = client_list;
    }
    else if (!strcmp("working_dir", key))
    {
        if (strlen(value) > 0)
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
                error(msg);
                exit(1);
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
        error("Error initializing bind address");
        exit(1);
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

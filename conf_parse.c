/*
 *  conf_parse.c - parse config file for application
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
#include <string.h>

#include "conf_parse.h"

static void parse_line(char *line, config *cfg)
{
    char *eq = strchr(line, '=');
    if (!eq)
    {
        return;
    }
    *eq = 0;
    char key[256];
    sscanf(line, "%255s", key);
    char value[1024];
    strcpy(value, eq + 1);

    char *ptr;
    ptr = value;
    while (0 < *ptr && *ptr <= ' ')
    {
        ptr++;
    }
    char *p1 = value;
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

    if (!strcmp("app_port", key))
    {
        long port = strtol(value, NULL, 10);
        if (0 < port && port < 65536)
        {
            cfg->app_port = (unsigned short)port;
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
    else if (!strcmp("web_port", key))
    {
        long port = strtol(value, NULL, 10);
        if (0 < port && port < 65536)
        {
            cfg->web_port = (unsigned short)port;
        }
    }
    else if (!strcmp("server_key", key))
    {
        FILE *f = fopen(value, "r");
        if (!f) return;
        fseek(f, 0, SEEK_END);
        ssize_t len = ftell(f);
        rewind(f);
        if (len<=0 || 65536<len){
            fclose(f);
            return;
        }
        char *buf = (char *)malloc(len);
        ssize_t sz = (ssize_t)fread(buf, 1, len, f);
        if (sz < len){
            free(buf);
            return;
        }
        fclose(f);
        cfg->priv_key = buf;
    }
    else if (!strcmp("server_cert", key))
    {
        FILE *f = fopen(value, "r");
        if (!f) return;
        fseek(f, 0, SEEK_END);
        ssize_t len = ftell(f);
        rewind(f);
        if (len<=0 || 65536<len){
            fclose(f);
            return;
        }
        char *buf = (char *)malloc(len);
        ssize_t sz = (ssize_t)fread(buf, 1, len, f);
        if (sz < len){
            free(buf);
            return;
        }
        fclose(f);
        cfg->server_cert = buf;
    }
    else if (!strcmp("ca_cert", key))
    {
        FILE *f = fopen(value, "r");
        if (!f) return;
        fseek(f, 0, SEEK_END);
        ssize_t len = ftell(f);
        rewind(f);
        if (len<=0 || 65536<len){
            fclose(f);
            return;
        }
        char *buf = (char *)malloc(len);
        ssize_t sz = (ssize_t)fread(buf, 1, len, f);
        if (sz < len){
            free(buf);
            return;
        }
        fclose(f);
        cfg->ca_cert = buf;
    }
}

config parse_conf(const char *conf_file)
{
    config cfg;
    cfg.app_port = -1;
    cfg.app_port_secure = -1;
    cfg.web_port = -1;
    cfg.priv_key = NULL;
    cfg.server_cert = NULL;
    cfg.ca_cert = NULL;

    FILE *f = fopen(conf_file, "r");
    if (!f)
    {
        return cfg;
    }

    char line[1024];
    while (fscanf(f, "%1023[^\n]%*c", line) != EOF)
    {
        parse_line(line, &cfg);
    }
    fclose(f);

    return cfg;
}
/*
 * utils/conf_parse.c - parse config file for application
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

#include <globals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils/config.h>
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

#ifndef NO_SSL
/*
 * Reads the list of client names from the file given by the filename.
 * Allowed client names must not exceed 511 characters.
 * Returns a list2* of null terminated strings as elements on success.
 * Returns null on error.
 */
static inline list2 *get_client_list(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) error_exit("Error: allowed client list file not found");
    list2 *client_list = init_list(1);
    char client[512];
    while (fscanf(f, "%511[^\n]%*c", client) != EOF) {
        client[511] = 0;
        trim(client);
        size_t len = strnlen(client, 512);
        if (len < 1) continue;
        if (client[0] == '#') continue;
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
 * Does not modify the ptr if an error occurred.
 * Note that if the file contained null byte in it, the length of the string may be smaller than the allocated memory
 * block.
 */
static inline void load_file(const char *file_name, data_buffer *buf_ptr) {
    if (!file_name) error_exit("Error: invalid filename");
    FILE *file_ptr = fopen(file_name, "rb");
    if (!file_ptr) error_exit("Error: certificate file not found");
    int64_t len = get_file_size(file_ptr);
    if (len <= 0 || 65536L < len) {
        fclose(file_ptr);
        error_exit("Error: invalid certificate file size");
    }
    char *buf = (char *)malloc((size_t)len);
    if (!buf) {
        fclose(file_ptr);
        error_exit("Error: malloc failed for certificate file");
    }
    ssize_t sz = (ssize_t)fread(buf, 1, (size_t)len, file_ptr);
    fclose(file_ptr);
    if (sz != len) {
        free(buf);
        error_exit("Error: certificate file reading failed");
    }
    if (buf_ptr->data) free(buf_ptr->data);
    buf_ptr->data = buf;
    buf_ptr->len = (int32_t)len;
}
#endif

/*
 * str must be a valid and null-terminated string
 * conf_ptr must be a valid pointer to a char
 * Sets the value pointed by conf_ptr to 1 if the string is "true" or "1".
 * Sets the value pointed by conf_ptr to 0 if the string is "false" or "0".
 * Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_is_true(const char *str, int8_t *conf_ptr) {
    if (!strcasecmp("true", str) || !strcmp("1", str)) {
        *conf_ptr = 1;
    } else if (!strcasecmp("false", str) || !strcmp("0", str)) {
        *conf_ptr = 0;
    } else {
        error_exit("Error: invalid boolean config value");
    }
}

/*
 * str must be a valid, non-empty, and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned 64-bit long integer
 * Sets the value pointed by conf_ptr to the unsigned 64-bit value given as a string in str if that is a valid value
 * between 1 and 2^61-1 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_int64(const char *str, int64_t *conf_ptr) {
    char *end_ptr;
    int64_t value = (int64_t)strtoull(str, &end_ptr, 10);
    switch (*end_ptr) {
        case '\0':
            break;
        case 'k':
        case 'K': {
            if (value > 2305843009213693LL) error_exit("Error: config value too large");
            value *= 1000;
            break;
        }
        case 'm':
        case 'M': {
            if (value > 2305843009213LL) error_exit("Error: config value too large");
            value *= 1000000L;
            break;
        }
        case 'g':
        case 'G': {
            if (value > 2305843009LL) error_exit("Error: config value too large");
            value *= 1000000000L;
            break;
        }
        case 't':
        case 'T': {
            if (value > 2305843L) error_exit("Error: config value too large");
            value *= 1000000000000LL;
            break;
        }
        default: {
            error_exit("Error: config value has invalid suffix");
        }
    }
    if (*end_ptr && *(end_ptr + 1)) error_exit("Error: config value has invalid suffix");
    if (value <= 0) error_exit("Error: invalid config value");
    *conf_ptr = value;
}

/*
 * str must be a valid, non-empty, and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned int
 * Sets the value pointed by conf_ptr to the unsigned int given as a string in str if that is a valid value between 1
 * and 2^32-2 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_uint32(const char *str, uint32_t *conf_ptr) {
    char *end_ptr;
    int64_t value = (int64_t)strtoull(str, &end_ptr, 10);
    if (value < 0 || value > 4294967294LL) error_exit("Error: config value not in range 0-4294967294");
    switch (*end_ptr) {
        case '\0':
            break;
        case 'k':
        case 'K': {
            if (value > 4294967L) error_exit("Error: config value too large");
            value *= 1000;
            break;
        }
        case 'm':
        case 'M': {
            if (value > 4294) error_exit("Error: config value too large");
            value *= 1000000L;
            break;
        }
        case 'g':
        case 'G': {
            if (value > 4) error_exit("Error: config value too large");
            value *= 1000000000L;
            break;
        }
        default: {
            error_exit("Error: config value has invalid suffix");
        }
    }
    if (*end_ptr && *(end_ptr + 1)) error_exit("Error: config value has invalid suffix");
    if (value <= 0 || 4294967295LL <= value) error_exit("Error: invalid config value");
    *conf_ptr = (uint32_t)value;
}

/*
 * str must be a valid, non-empty, and null-terminated string
 * conf_ptr must be a valid pointer to an unsigned short
 * Sets the value pointed by conf_ptr to the unsigned short given as a string in str if that is a valid value between 1
 * and 65535 inclusive. Otherwise, does not change the value pointed by conf_ptr
 */
static inline void set_uint16(const char *str, uint16_t *conf_ptr) {
    char *end_ptr;
    long value = strtol(str, &end_ptr, 10);
    if (value < 0 || value > 65535L) error_exit("Error: config value not in range 0-65535");
    if (*end_ptr) error_exit("Error: invalid config value");
    *conf_ptr = (uint16_t)value;
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
    if (key_len <= 0 || key_len >= LINE_MAX_LEN) error_exit("Error: invalid config key");

    const size_t value_len = strnlen(value, LINE_MAX_LEN);
    if (value_len <= 0 || value_len >= LINE_MAX_LEN) error_exit("Error: invalid config value");

    if (!strcmp("app_port", key)) {
        set_uint16(value, &(cfg->app_port));
    } else if (!strcmp("insecure_mode_enabled", key)) {
        set_is_true(value, &(cfg->insecure_mode_enabled));
    } else if (!strcmp("app_port_secure", key)) {
        set_uint16(value, &(cfg->app_port_secure));
    } else if (!strcmp("secure_mode_enabled", key)) {
        set_is_true(value, &(cfg->secure_mode_enabled));
    } else if (!strcmp("udp_port", key)) {
        set_uint16(value, &(cfg->udp_port));
    } else if (!strcmp("udp_server_enabled", key)) {
        set_is_true(value, &(cfg->udp_server_enabled));
#ifdef WEB_ENABLED
    } else if (!strcmp("web_port", key)) {
        set_uint16(value, &(cfg->web_port));
    } else if (!strcmp("web_mode_enabled", key)) {
        set_is_true(value, &(cfg->web_mode_enabled));
#endif
#ifndef NO_SSL
    } else if (!strcmp("server_cert", key)) {
        load_file(value, &(cfg->server_cert));
    } else if (!strcmp("ca_cert", key)) {
        load_file(value, &(cfg->ca_cert));
    } else if (!strcmp("allowed_clients", key)) {
        list2 *client_list = get_client_list(value);
        if (client_list == NULL) return;
        if (cfg->allowed_clients) free_list(cfg->allowed_clients);
        cfg->allowed_clients = client_list;
#endif
    } else if (!strcmp("working_dir", key)) {
        if (cfg->working_dir) free(cfg->working_dir);
        cfg->working_dir = strdup(value);
    } else if (!strcmp("bind_address", key)) {
        if (parse_ip(value, &(cfg->bind_addr)) != EXIT_SUCCESS) {
            char msg[64];
            if (snprintf_check(msg, 64, "Error: Invalid bind address %s", value)) msg[0] = 0;
            error_exit(msg);
        }
    } else if (!strcmp("bind_address_udp", key)) {
        if (parse_ip(value, &(cfg->bind_addr_udp)) != EXIT_SUCCESS) {
            char msg[64];
            if (snprintf_check(msg, 64, "Error: Invalid UDP bind address %s", value)) msg[0] = 0;
            error_exit(msg);
        }
    } else if (!strcmp("restart", key)) {
        set_is_true(value, &(cfg->restart));
    } else if (!strcmp("max_text_length", key)) {
        set_uint32(value, &(cfg->max_text_length));
    } else if (!strcmp("max_file_size", key)) {
        set_int64(value, &(cfg->max_file_size));
    } else if (!strcmp("cut_sent_files", key)) {
        set_is_true(value, &(cfg->cut_sent_files));
    } else if (!strcmp("client_selects_display", key)) {
        set_is_true(value, &(cfg->client_selects_display));
    } else if (!strcmp("display", key)) {
        set_uint16(value, &(cfg->display));
    } else if (!strcmp("min_proto_version", key)) {
        set_uint16(value, &(cfg->min_proto_version));
    } else if (!strcmp("max_proto_version", key)) {
        set_uint16(value, &(cfg->max_proto_version));
    } else if (!strcmp("method_get_text_enabled", key)) {
        set_is_true(value, &(cfg->method_enabled.get_text));
    } else if (!strcmp("method_send_text_enabled", key)) {
        set_is_true(value, &(cfg->method_enabled.send_text));
    } else if (!strcmp("method_get_files_enabled", key)) {
        set_is_true(value, &(cfg->method_enabled.get_files));
    } else if (!strcmp("method_send_files_enabled", key)) {
        set_is_true(value, &(cfg->method_enabled.send_files));
    } else if (!strcmp("method_get_image_enabled", key)) {
        set_is_true(value, &(cfg->method_enabled.get_image));
    } else if (!strcmp("method_get_copied_image_enabled", key)) {
        set_is_true(value, &(cfg->method_enabled.get_copied_image));
    } else if (!strcmp("method_get_screenshot_enabled", key)) {
        set_is_true(value, &(cfg->method_enabled.get_screenshot));
    } else if (!strcmp("method_info_enabled", key)) {
        set_is_true(value, &(cfg->method_enabled.info));
#if defined(_WIN32) || defined(__APPLE__)
    } else if (!strcmp("tray_icon", key)) {
        set_is_true(value, &(cfg->tray_icon));
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
    cfg->udp_server_enabled = -1;

#ifdef WEB_ENABLED
    cfg->web_port = 0;
    cfg->web_mode_enabled = -1;
#endif

    cfg->server_cert.data = NULL;
    cfg->server_cert.len = -1;
    cfg->ca_cert.data = NULL;
    cfg->ca_cert.len = -1;
    cfg->allowed_clients = NULL;

    cfg->working_dir = NULL;
    cfg->restart = -1;
    cfg->max_text_length = 0;
    cfg->max_file_size = 0;
    cfg->cut_sent_files = -1;
    cfg->client_selects_display = -1;
    cfg->display = 0;

    cfg->min_proto_version = 0;
    cfg->max_proto_version = 0;

    cfg->method_enabled.get_text = -1;
    cfg->method_enabled.send_text = -1;
    cfg->method_enabled.get_files = -1;
    cfg->method_enabled.send_files = -1;
    cfg->method_enabled.get_image = -1;
    cfg->method_enabled.get_copied_image = -1;
    cfg->method_enabled.get_screenshot = -1;
    cfg->method_enabled.info = -1;
#if defined(_WIN32) || defined(__APPLE__)
    cfg->tray_icon = -1;
#endif
    if (parse_ip(NULL, &(cfg->bind_addr)) != EXIT_SUCCESS) error_exit("Error initializing bind address");
    if (parse_ip(NULL, &(cfg->bind_addr_udp)) != EXIT_SUCCESS) error_exit("Error initializing bind address UDP");

    if (!file_name) {
#ifdef DEBUG_MODE
        printf("File name is null\n");
#endif
        puts("Using default configurations");
        return;
    }

    FILE *f = fopen(file_name, "r");
    if (!f) {
        puts("Using default configurations");
        return;
    }

    char line[LINE_MAX_LEN + 1];
    while (fgets(line, LINE_MAX_LEN, f)) {
        line[LINE_MAX_LEN] = 0;
        parse_line(line, cfg);
    }
    fclose(f);

    return;
}

#ifndef NO_SSL
void clear_config_key_cert(config *cfg) {
    if (cfg->server_cert.data) {
        if (cfg->server_cert.len > 0) memset(cfg->server_cert.data, 0, (size_t)cfg->server_cert.len);
        free(cfg->server_cert.data);
        cfg->server_cert.data = NULL;
    }
    if (cfg->ca_cert.data) {
        free(cfg->ca_cert.data);
        cfg->ca_cert.data = NULL;
    }
    if (cfg->allowed_clients) {
        free_list(cfg->allowed_clients);
        cfg->allowed_clients = NULL;
    }
}
#endif

void clear_config(config *cfg) {
#ifndef NO_SSL
    clear_config_key_cert(cfg);
#endif
    if (cfg->working_dir) {
        free(cfg->working_dir);
        cfg->working_dir = NULL;
    }
}

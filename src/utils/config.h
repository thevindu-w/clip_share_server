/*
 * utils/config.h - header for conf_parse.c
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

#ifndef UTILS_CONFIG_H_
#define UTILS_CONFIG_H_

#include <stdint.h>
#include <sys/types.h>
#include <utils/list_utils.h>

typedef struct _data_buffer {
    int32_t len;
    char *data;
} data_buffer;

typedef struct _config {
    uint16_t app_port;
    uint16_t app_port_secure;
    uint16_t udp_port;
    int8_t insecure_mode_enabled;
    int8_t secure_mode_enabled;
#ifdef WEB_ENABLED
    uint16_t web_port;
    int8_t web_mode_enabled;
#endif

    data_buffer server_cert;
    data_buffer ca_cert;
    list2 *allowed_clients;

    char *working_dir;
    uint32_t bind_addr;
    uint32_t bind_addr_udp;
    int8_t restart;

    uint32_t max_text_length;
    int64_t max_file_size;

    int8_t cut_sent_files;
    int8_t client_selects_display;
    uint16_t display;

    uint16_t min_proto_version;
    uint16_t max_proto_version;

    struct {
        int8_t get_text;
        int8_t send_text;
        int8_t get_files;
        int8_t send_files;
        int8_t get_image;
        int8_t get_copied_image;
        int8_t get_screenshot;
        int8_t info;
    } method_enabled;

#if defined(_WIN32) || defined(__APPLE__)
    int8_t tray_icon;
#endif
} config;

/*
 * Parse the config file given by the file_name.
 * Sets the default configuration on error.
 */
extern void parse_conf(config *cfg, const char *file_name);

/*
 * Clears the memory allocated for TLS keys, certificates, and allowed_clients list in the configuration.
 * This does not clear the config *conf or other config entries.
 */
extern void clear_config_key_cert(config *conf);

/*
 * Clears all the memory allocated in the array type or list2* type elements in the configuration.
 * This does not clear the memory block pointed by the config *conf.
 */
extern void clear_config(config *conf);

#endif  // UTILS_CONFIG_H_

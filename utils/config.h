/*
 * config.h - header for conf_parse.c
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

typedef struct _config {
    unsigned short app_port;
    unsigned short app_port_secure;
    unsigned short udp_port;
    int8_t insecure_mode_enabled;
    int8_t secure_mode_enabled;
#ifdef WEB_ENABLED
    unsigned short web_port;
    int8_t web_mode_enabled;
#endif

    char *priv_key;
    char *server_cert;
    char *ca_cert;
    list2 *allowed_clients;

    char *working_dir;
    uint32_t bind_addr;
    int8_t restart;

    uint32_t max_text_length;
    int64_t max_file_size;

    int8_t cut_sent_files;
    int8_t client_selects_display;
    unsigned short display;

    unsigned short min_proto_version;
    unsigned short max_proto_version;

    int8_t method_get_text_enabled;
    int8_t method_send_text_enabled;
    int8_t method_get_files_enabled;
    int8_t method_send_files_enabled;
    int8_t method_get_image_enabled;
    int8_t method_get_copied_image_enabled;
    int8_t method_get_screenshot_enabled;
    int8_t method_info_enabled;

#ifdef _WIN32
    int8_t tray_icon;
#endif
} config;

/*
 * Parse the config file given by the file_name.
 * Sets the default configuration on error.
 */
extern void parse_conf(config *cfg, const char *file_name);

/*
 * Clears all the memory allocated in the array type or list2* type elements in the configuration.
 * This does not clear the memory block pointed by the config *conf.
 */
extern void clear_config(config *conf);

#endif  // UTILS_CONFIG_H_

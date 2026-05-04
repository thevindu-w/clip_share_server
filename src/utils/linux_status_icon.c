/*
 * utils/linux_status_icon.c - display a status icon on Linux
 * Copyright (C) 2026 H. Thevindu J. Wijesekera
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

#ifdef __linux__

#include <dlfcn.h>
#include <globals.h>
#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils/kill_others.h>
#include <utils/linux_status_icon.h>

extern const char icon_png[];
extern const unsigned int icon_png_len;

static void *gtk_handle = NULL;
static void *appind_handle = NULL;

static void (*ptr_gtk_init)(int *, char ***);
static void (*ptr_gtk_main_quit)(void);
static void (*ptr_gtk_main)(void);
static GtkWidget *(*ptr_gtk_menu_new)(void);
static GtkWidget *(*ptr_gtk_menu_item_new_with_label)(const gchar *);
static void (*ptr_gtk_menu_shell_append)(GtkMenuShell *, GtkWidget *);
static GTypeInstance *(*ptr_g_type_check_instance_cast)(GTypeInstance *, GType);
static GType (*ptr_gtk_menu_shell_get_type)(void);
static GType (*ptr_gtk_menu_get_type)(void);
static void (*ptr_gtk_widget_show_all)(GtkWidget *);
static void (*ptr_gtk_menu_popup_at_pointer)(GtkMenu *, const GdkEvent *);
static unsigned long (*ptr_g_signal_connect_data)(void *, const char *, GCallback, void *, void *, int);
static GdkPixbufLoader *(*ptr_gdk_pixbuf_loader_new)(void);
static gboolean (*ptr_gdk_pixbuf_loader_write)(GdkPixbufLoader *, const guchar *, gsize, GError **);
static gboolean (*ptr_gdk_pixbuf_loader_close)(GdkPixbufLoader *, GError **);
static GdkPixbuf *(*ptr_gdk_pixbuf_loader_get_pixbuf)(GdkPixbufLoader *);
static GtkStatusIcon *(*ptr_gtk_status_icon_new_from_pixbuf)(GdkPixbuf *);
static void (*ptr_gtk_status_icon_set_tooltip_text)(GtkStatusIcon *, const gchar *);
static void (*ptr_gtk_status_icon_set_visible)(GtkStatusIcon *, gboolean);
static void (*ptr_g_object_unref)(gpointer);

static AppIndicator *(*ptr_app_indicator_new)(const char *, const char *, AppIndicatorCategory);
static void (*ptr_app_indicator_set_status)(AppIndicator *, AppIndicatorStatus);
static void (*ptr_app_indicator_set_menu)(AppIndicator *, GtkMenu *);
static void (*ptr_app_indicator_set_title)(AppIndicator *, const char *);

static char icon_path[] = "/tmp/clipshare-server-iconXXXXXX.png";
static int is_running;

#define APP_NAME "ClipShare Server"

static void on_quit(void *widget, gpointer data) {
    (void)widget;
    (void)data;

    if (ptr_gtk_main_quit) {
        ptr_gtk_main_quit();
        is_running = 0;
    }
    kill_other_processes(global_prog_name);
}

static inline GtkMenu *create_menu(void) {
    GtkWidget *menu = ptr_gtk_menu_new();
    GtkWidget *quit_item = ptr_gtk_menu_item_new_with_label("Quit");

    ptr_g_signal_connect_data(quit_item, "activate", G_CALLBACK(on_quit), NULL, NULL, 0);

    ptr_gtk_menu_shell_append(
        (GtkMenuShell *)ptr_g_type_check_instance_cast((GTypeInstance *)menu, ptr_gtk_menu_shell_get_type()),
        quit_item);
    ptr_gtk_widget_show_all(menu);
    return (GtkMenu *)ptr_g_type_check_instance_cast((GTypeInstance *)menu, ptr_gtk_menu_get_type());
}

static void on_popup(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data) {
    (void)status_icon;
    (void)button;
    (void)activate_time;
    (void)user_data;

    GtkMenu *menu = create_menu();

    ptr_gtk_menu_popup_at_pointer(menu, NULL);
}

static int show_icon_gtk(void) {
    *(void **)&ptr_gtk_init = dlsym(gtk_handle, "gtk_init");
    *(void **)&ptr_gtk_main = dlsym(gtk_handle, "gtk_main");
    *(void **)&ptr_gtk_main_quit = dlsym(gtk_handle, "gtk_main_quit");
    *(void **)&ptr_gtk_menu_new = dlsym(gtk_handle, "gtk_menu_new");
    *(void **)&ptr_gtk_menu_shell_append = dlsym(gtk_handle, "gtk_menu_shell_append");
    *(void **)&ptr_gtk_menu_item_new_with_label = dlsym(gtk_handle, "gtk_menu_item_new_with_label");
    *(void **)&ptr_g_type_check_instance_cast = dlsym(gtk_handle, "g_type_check_instance_cast");
    *(void **)&ptr_gtk_menu_shell_get_type = dlsym(gtk_handle, "gtk_menu_shell_get_type");
    *(void **)&ptr_gtk_menu_get_type = dlsym(gtk_handle, "gtk_menu_get_type");
    *(void **)&ptr_gtk_widget_show_all = dlsym(gtk_handle, "gtk_widget_show_all");
    *(void **)&ptr_gtk_menu_popup_at_pointer = dlsym(gtk_handle, "gtk_menu_popup_at_pointer");
    *(void **)&ptr_gtk_status_icon_new_from_pixbuf = dlsym(gtk_handle, "gtk_status_icon_new_from_pixbuf");
    *(void **)&ptr_gtk_status_icon_set_tooltip_text = dlsym(gtk_handle, "gtk_status_icon_set_tooltip_text");
    *(void **)&ptr_gtk_status_icon_set_visible = dlsym(gtk_handle, "gtk_status_icon_set_visible");
    *(void **)&ptr_gdk_pixbuf_loader_new = dlsym(gtk_handle, "gdk_pixbuf_loader_new");
    *(void **)&ptr_gdk_pixbuf_loader_write = dlsym(gtk_handle, "gdk_pixbuf_loader_write");
    *(void **)&ptr_gdk_pixbuf_loader_close = dlsym(gtk_handle, "gdk_pixbuf_loader_close");
    *(void **)&ptr_gdk_pixbuf_loader_get_pixbuf = dlsym(gtk_handle, "gdk_pixbuf_loader_get_pixbuf");
    *(void **)&ptr_g_object_unref = dlsym(gtk_handle, "g_object_unref");
    *(void **)&ptr_g_signal_connect_data = dlsym(gtk_handle, "g_signal_connect_data");

    if (!ptr_gtk_init || !ptr_gtk_main || !ptr_gtk_main_quit || !ptr_gtk_menu_new || !ptr_gtk_menu_shell_append ||
        !ptr_gtk_menu_item_new_with_label || !ptr_g_type_check_instance_cast || !ptr_gtk_menu_shell_get_type ||
        !ptr_gtk_menu_get_type || !ptr_gtk_widget_show_all || !ptr_gtk_menu_popup_at_pointer ||
        !ptr_gdk_pixbuf_loader_new || !ptr_gdk_pixbuf_loader_write || !ptr_gdk_pixbuf_loader_close ||
        !ptr_gdk_pixbuf_loader_get_pixbuf || !ptr_gtk_status_icon_new_from_pixbuf ||
        !ptr_gtk_status_icon_set_tooltip_text || !ptr_gtk_status_icon_set_visible || !ptr_g_signal_connect_data ||
        !ptr_g_object_unref) {
        return EXIT_FAILURE;
    }

    ptr_gtk_init(NULL, NULL);

    GdkPixbufLoader *loader = ptr_gdk_pixbuf_loader_new();
    if (!ptr_gdk_pixbuf_loader_write(loader, (const guchar *)icon_png, icon_png_len, NULL)) {
        ptr_g_object_unref(loader);
        return EXIT_FAILURE;
    }
    if (!ptr_gdk_pixbuf_loader_close(loader, NULL)) {
        ptr_g_object_unref(loader);
        return EXIT_FAILURE;
    }

    GdkPixbuf *pixbuf = ptr_gdk_pixbuf_loader_get_pixbuf(loader);
    if (!pixbuf) {
        ptr_g_object_unref(loader);
        return EXIT_FAILURE;
    }
    GtkStatusIcon *status_icon = ptr_gtk_status_icon_new_from_pixbuf(pixbuf);
    ptr_g_object_unref(loader);
    if (!status_icon) {
        return EXIT_FAILURE;
    }
    ptr_gtk_status_icon_set_tooltip_text(status_icon, APP_NAME);
    ptr_gtk_status_icon_set_visible(status_icon, TRUE);

    ptr_g_signal_connect_data(status_icon, "activate", G_CALLBACK(on_popup), NULL, NULL, 0);

    ptr_gtk_main();
    return EXIT_SUCCESS;
}

static int show_icon_appindicator(void) {
    *(void **)&ptr_gtk_init = dlsym(gtk_handle, "gtk_init");
    *(void **)&ptr_gtk_main = dlsym(gtk_handle, "gtk_main");
    *(void **)&ptr_gtk_main_quit = dlsym(gtk_handle, "gtk_main_quit");
    *(void **)&ptr_gtk_menu_new = dlsym(gtk_handle, "gtk_menu_new");
    *(void **)&ptr_gtk_menu_shell_append = dlsym(gtk_handle, "gtk_menu_shell_append");
    *(void **)&ptr_gtk_menu_item_new_with_label = dlsym(gtk_handle, "gtk_menu_item_new_with_label");
    *(void **)&ptr_g_type_check_instance_cast = dlsym(gtk_handle, "g_type_check_instance_cast");
    *(void **)&ptr_gtk_menu_shell_get_type = dlsym(gtk_handle, "gtk_menu_shell_get_type");
    *(void **)&ptr_gtk_menu_get_type = dlsym(gtk_handle, "gtk_menu_get_type");
    *(void **)&ptr_gtk_widget_show_all = dlsym(gtk_handle, "gtk_widget_show_all");
    *(void **)&ptr_g_signal_connect_data = dlsym(gtk_handle, "g_signal_connect_data");

    *(void **)&ptr_app_indicator_new = dlsym(appind_handle, "app_indicator_new");
    *(void **)&ptr_app_indicator_set_status = dlsym(appind_handle, "app_indicator_set_status");
    *(void **)&ptr_app_indicator_set_menu = dlsym(appind_handle, "app_indicator_set_menu");
    *(void **)&ptr_app_indicator_set_title = dlsym(appind_handle, "app_indicator_set_title");

    if (!ptr_gtk_init || !ptr_gtk_main || !ptr_gtk_main_quit || !ptr_gtk_menu_new || !ptr_gtk_menu_shell_append ||
        !ptr_gtk_menu_item_new_with_label || !ptr_g_type_check_instance_cast || !ptr_gtk_menu_shell_get_type ||
        !ptr_gtk_menu_get_type || !ptr_gtk_widget_show_all || !ptr_g_signal_connect_data || !ptr_app_indicator_new ||
        !ptr_app_indicator_set_status || !ptr_app_indicator_set_menu || !ptr_app_indicator_set_title) {
        return EXIT_FAILURE;
    }

    ptr_gtk_init(NULL, NULL);

    int icon_fd = mkstemps(icon_path, 4);
    if (icon_fd < 0) {
#ifdef DEBUG_MODE
        puts("Error creating temp file for app icon");
#endif
        return EXIT_FAILURE;
    }
#ifdef DEBUG_MODE
    printf("Temp icon file created at %s\n", icon_path);
#endif
    ssize_t write_len = write(icon_fd, icon_png, icon_png_len);
    close(icon_fd);
    if (write_len != (ssize_t)icon_png_len) {
        return EXIT_FAILURE;
    }

    AppIndicator *indicator =
        ptr_app_indicator_new("clipshare-server", icon_path, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    ptr_app_indicator_set_title(indicator, APP_NAME);
    ptr_app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);

    GtkMenu *menu = create_menu();

    ptr_app_indicator_set_menu(indicator, menu);
    ptr_gtk_main();

    return EXIT_SUCCESS;
}

static inline void *get_appindicator_so(void) {
    void *handle = dlopen("libayatana-appindicator3.so.1", RTLD_NOW);
    if (handle) {
        return handle;
    }
    handle = dlopen("libayatana-appindicator3.so", RTLD_NOW);
    if (handle) {
        return handle;
    }
    handle = dlopen("libappindicator3.so.1", RTLD_NOW);
    if (handle) {
        return handle;
    }
    handle = dlopen("libappindicator3.so", RTLD_NOW);
    if (handle) {
        return handle;
    }
    return NULL;
}

void show_status_icon(void) {
    is_running = 1;
    gtk_handle = dlopen("libgtk-3.so.0", RTLD_NOW);
    if (!gtk_handle) {
        gtk_handle = dlopen("libgtk-3.so", RTLD_NOW);
        if (!gtk_handle) {
            return;
        }
    }

    appind_handle = get_appindicator_so();
    if (appind_handle) {
        show_icon_appindicator();
    } else {
        show_icon_gtk();
    }

    cleanup_status_icon();
    return;
}

void cleanup_status_icon(void) {
    if (ptr_gtk_main_quit) {
        if (is_running) {
            ptr_gtk_main_quit();
            is_running = 0;
        }
        ptr_gtk_main_quit = NULL;
    }
    if (appind_handle) {
        dlclose(appind_handle);
        appind_handle = NULL;
    }
    if (gtk_handle) {
        dlclose(gtk_handle);
        gtk_handle = NULL;
    }
    remove(icon_path);
}

#endif

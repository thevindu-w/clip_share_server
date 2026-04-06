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
#include <stdlib.h>
#include <utils/kill_others.h>
#include <utils/linux_status_icon.h>

extern const char icon_png[];
extern const unsigned int icon_png_len;

static void *gtk_handle = NULL;

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

static void on_quit(GtkWidget *widget, gpointer data) {
    (void)widget;
    (void)data;

    if (ptr_gtk_main_quit) {
        ptr_gtk_main_quit();
    }
    kill_other_processes(global_prog_name);
}

static void on_popup(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data) {
    (void)status_icon;
    (void)button;
    (void)activate_time;
    (void)user_data;

    GtkWidget *menu = ptr_gtk_menu_new();
    GtkWidget *quit_item = ptr_gtk_menu_item_new_with_label("Quit");

    ptr_g_signal_connect_data(quit_item, "activate", G_CALLBACK(on_quit), NULL, NULL, 0);

    ptr_gtk_menu_shell_append(
        (GtkMenuShell *)ptr_g_type_check_instance_cast((GTypeInstance *)menu, ptr_gtk_menu_shell_get_type()),
        quit_item);
    ptr_gtk_widget_show_all(menu);

    ptr_gtk_menu_popup_at_pointer(
        (GtkMenu *)ptr_g_type_check_instance_cast((GTypeInstance *)menu, ptr_gtk_menu_get_type()), NULL);
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
    ptr_gtk_status_icon_set_tooltip_text(status_icon, "ClipShare Server");
    ptr_gtk_status_icon_set_visible(status_icon, TRUE);

    ptr_g_signal_connect_data(status_icon, "activate", G_CALLBACK(on_popup), NULL, NULL, 0);

    ptr_gtk_main();
    return EXIT_SUCCESS;
}

void show_status_icon(void) {
    gtk_handle = dlopen("libgtk-3.so.0", RTLD_NOW);
    if (!gtk_handle) {
        gtk_handle = dlopen("libgtk-3.so", RTLD_NOW);
        if (!gtk_handle) {
            return;
        }
    }

    show_icon_gtk();
    ptr_gtk_main_quit = NULL;
    dlclose(gtk_handle);
    gtk_handle = NULL;
    return;
}

void cleanup_status_icon(void) {
    if (ptr_gtk_main_quit) {
        ptr_gtk_main_quit();
    }
    if (gtk_handle) {
        dlclose(gtk_handle);
        gtk_handle = NULL;
    }
}

#endif

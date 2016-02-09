/*****************************************************************************

 Copyright (c) 2008-2016  Monavacon Limited <http://www.monavacon.com/>
 Copyright (c) 2001-2008  OpenSS7 Corporation <http://www.openss7.com/>
 Copyright (c) 1997-2001  Brian F. G. Bidulock <bidulock@openss7.org>

 All Rights Reserved.

 This program is free software: you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation, version 3 of the license.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 details.

 You should have received a copy of the GNU General Public License along with
 this program.  If not, see <http://www.gnu.org/licenses/>, or write to the
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 -----------------------------------------------------------------------------

 U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on
 behalf of the U.S. Government ("Government"), the following provisions apply
 to you.  If the Software is supplied by the Department of Defense ("DoD"), it
 is classified as "Commercial Computer Software" under paragraph 252.227-7014
 of the DoD Supplement to the Federal Acquisition Regulations ("DFARS") (or any
 successor regulations) and the Government is acquiring only the license rights
 granted herein (the license rights customarily provided to non-Government
 users).  If the Software is supplied to any unit or agency of the Government
 other than DoD, it is classified as "Restricted Computer Software" and the
 Government's rights in the Software are defined in paragraph 52.227-19 of the
 Federal Acquisition Regulations ("FAR") (or any successor regulations) or, in
 the cases of NASA, in paragraph 18.52.227-86 of the NASA Supplement to the FAR
 (or any successor regulations).

 -----------------------------------------------------------------------------

 Commercial licensing and support of this software is available from OpenSS7
 Corporation at a fee.  See http://www.openss7.com/

 *****************************************************************************/

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <syslog.h>
#include <sys/utsname.h>

#include <assert.h>
#include <locale.h>
#include <stdarg.h>
#include <strings.h>
#include <regex.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#ifdef XRANDR
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/randr.h>
#endif
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#ifdef STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#endif
#include <X11/SM/SMlib.h>
#include <gio/gio.h>
#include <glib.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <cairo.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <pwd.h>

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

#define GTK_EVENT_STOP		TRUE
#define GTK_EVENT_PROPAGATE	FALSE

#define XPRINTF(args...) do { } while (0)
#define OPRINTF(args...) do { if (options.output > 1) { \
	fprintf(stdout, "I: "); \
	fprintf(stdout, args); \
	fflush(stdout); } } while (0)
#define DPRINTF(args...) do { if (options.debug) { \
	fprintf(stderr, "D: %s +%d %s(): ", __FILE__, __LINE__, __func__); \
	fprintf(stderr, args); \
	fflush(stderr); } } while (0)
#define EPRINTF(args...) do { \
	fprintf(stderr, "E: %s +%d %s(): ", __FILE__, __LINE__, __func__); \
	fprintf(stderr, args); \
	fflush(stderr);   } while (0)
#define DPRINT() do { if (options.debug) { \
	fprintf(stderr, "D: %s +%d %s()\n", __FILE__, __LINE__, __func__); \
	fflush(stderr); } } while (0)

#undef EXIT_SUCCESS
#undef EXIT_FAILURE
#undef EXIT_SYNTAXERR

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1
#define EXIT_SYNTAXERR  2

static Atom _XA_WM_STATE;
static Atom _XA_XDE_THEME_NAME;
static Atom _XA_GTK_READ_RCFILES;
static Atom _XA_NET_WM_ICON_GEOMETRY;
// static Atom _XA_NET_DESKTOP_LAYOUT;
// static Atom _XA_NET_NUMBER_OF_DESKTOPS;
// static Atom _XA_NET_CURRENT_DESKTOP;
// static Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
// static Atom _XA_WIN_WORKSPACE_COUNT;
// static Atom _XA_WIN_WORKSPACE;
// static Atom _XA_XROOTPMAP_ID;
// static Atom _XA_ESETROOT_PMAP_ID;

// static Atom _XA_WIN_AREA;
// static Atom _XA_WIN_AREA_COUNT;

typedef enum {
	UseScreenDefault,               /* default screen by button */
	UseScreenActive,                /* screen with active window */
	UseScreenFocused,               /* screen with focused window */
	UseScreenPointer,               /* screen with pointer */
	UseScreenSpecified,             /* specified screen */
} UseScreen;

typedef enum {
	PositionDefault,                /* default position */
	PositionPointer,                /* position at pointer */
	PositionCenter,                 /* center of monitor */
	PositionTopLeft,                /* top left of work area */
	PositionBottomRight,		/* bottom right of work area */
	PositionSpecified,		/* specified position (X geometry) */
} MenuPosition;

typedef enum {
	CommandDefault,
	CommandPopup,
	CommandHelp,
	CommandVersion,
	CommandCopying,
} Command;

typedef enum {
	WindowOrderDefault,
	WindowOrderClient,
	WindowOrderStacking,
} WindowOrder;

typedef struct {
	int debug;
	int output;
	char *display;
	int screen;
	int button;
	Time timestamp;
	UseScreen which;
	MenuPosition where;
	struct {
		int value;
		int sign;
	} x, y;
	unsigned int w, h;
	WindowOrder order;
	Command command;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.button = 0,
	.timestamp = CurrentTime,
	.which = UseScreenDefault,
	.where = PositionDefault,
	.x = {
	      .value = 0,
	      .sign = 1,
	      }
	,
	.y = {
	      .value = 0,
	      .sign = 1,
	      }
	,
	.w = 0,
	.h = 0,
	.order = WindowOrderDefault,
	.command = CommandDefault,
};

void
selection_done(GtkMenuShell *menushell, gpointer user_data)
{
	OPRINTF("Selection done: exiting\n");
	if (!gtk_menu_get_tearoff_state(GTK_MENU(menushell)))
		gtk_main_quit();
}

void
workspace_activate(GtkMenuItem *item, gpointer user_data)
{
	OPRINTF("Menu item [%s] activated\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	wnck_workspace_activate(user_data, gtk_get_current_event_time());
}

void
workspace_activate_item(GtkMenuItem *item, gpointer user_data)
{
	OPRINTF("Menu item [%s] activate item\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	wnck_workspace_activate(user_data, gtk_get_current_event_time());
}

gboolean
workspace_button_press(GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	GdkEventButton *ev = (typeof(ev)) event;

	OPRINTF("Menu item [%s] button press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (ev->button != 1)
		return GTK_EVENT_PROPAGATE;
	// workspace_activate(GTK_MENU_ITEM(item), user_data);
	wnck_workspace_activate(user_data, ev->time);
	gtk_menu_shell_activate_item(GTK_MENU_SHELL(gtk_widget_get_parent(item)), item, TRUE);
	return GTK_EVENT_STOP;
}

WnckWorkspace *selected_workspace = NULL;
GtkItem *selected_item = NULL;

void
workspace_select(GtkItem *item, gpointer user_data)
{
	OPRINTF("Menu item [%s] selected\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	selected_workspace = user_data;
	selected_item = item;
}

void
workspace_deselect(GtkItem *item, gpointer user_data)
{
	OPRINTF("Menu item [%s] deselected\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (selected_workspace == user_data)
		selected_workspace = NULL;
	if (selected_item == item)
		selected_item = NULL;
}

gboolean
workspace_key_press(GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	GdkEventKey *ev = (typeof(ev)) event;

	OPRINTF("Menu item [%s] key press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (ev->keyval != GDK_KEY_Return)
		return GTK_EVENT_PROPAGATE;
	// workspace_activate(GTK_MENU_ITEM(item), user_data);
	wnck_workspace_activate(user_data, ev->time);
	gtk_menu_shell_activate_item(GTK_MENU_SHELL(gtk_widget_get_parent(item)), item, TRUE);
	return GTK_EVENT_STOP;
}

gboolean
workspace_menu_key_press(GtkWidget *menu, GdkEvent *event, gpointer user_data)
{
	GdkEventKey *ev = (typeof(ev)) event;
	GtkWidget *item;

	OPRINTF("Menu key press\n");
	item = gtk_container_get_focus_child(GTK_CONTAINER(menu));
	if (GTK_IS_MENU_ITEM(item))
		OPRINTF("Menu item [%s] key press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (ev->keyval == GDK_KEY_Return) {
		OPRINTF("Menu key press [Return]\n");
		if (selected_workspace)
			wnck_workspace_activate(selected_workspace, ev->time);
		if (selected_item)
			gtk_menu_shell_activate_item(GTK_MENU_SHELL(menu), GTK_WIDGET(selected_item), TRUE);
		return GTK_EVENT_STOP;
	}
	return GTK_EVENT_PROPAGATE;
}

void
window_activate(GtkMenuItem *item, gpointer user_data)
{
	WnckWindow *win = user_data;
	WnckWorkspace *work = wnck_window_get_workspace(win);

	OPRINTF("Menu item [%s] activated\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	wnck_workspace_activate(work, gtk_get_current_event_time());
	wnck_window_activate(win, gtk_get_current_event_time());
}

gboolean
window_menu(GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	GdkEventButton *ev = (typeof(ev)) event;
	WnckWindow *win = user_data;
	GtkWidget *menu, *parent;

	if (ev->button != 3)
		return GTK_EVENT_PROPAGATE;
#if 0
	menu = wnck_action_menu_new(win);
	g_signal_connect(G_OBJECT(menu), "selection_done", G_CALLBACK(selection_done), NULL);
	parent = gtk_widget_get_parent(item);
	/* FIXME: need a menu position function like menu cascading. */
	gtk_menu_popup(GTK_MENU(menu), parent, item, 
			NULL, NULL, ev->button, ev->time);
	return GTK_EVENT_STOP;
#else
	(void) parent;
	if (!gtk_menu_item_get_submenu(GTK_MENU_ITEM(item))) {
		menu = wnck_action_menu_new(win);
		g_signal_connect(G_OBJECT(menu), "selection_done", G_CALLBACK(selection_done), NULL);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
	}
	return GTK_EVENT_PROPAGATE;
#endif
}

WnckWindow *selected_window = NULL;
GtkItem *selected_witem = NULL;

void
window_select(GtkItem *item, gpointer user_data)
{
	OPRINTF("Menu item [%s] selected\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	selected_window = user_data;
	selected_witem = item;
}

void
window_deselect(GtkItem *item, gpointer user_data)
{
	OPRINTF("Menu item [%s] deselected\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (selected_window == user_data)
		selected_window = NULL;
	if (selected_witem == item)
		selected_witem = NULL;
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), NULL);
}

gboolean
window_key_press(GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	GdkEventKey *ev = (typeof(ev)) event;
	WnckWindow *win = user_data;
	WnckWorkspace *work;

	OPRINTF("Menu item [%s] key press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (ev->keyval != GDK_KEY_Return)
		return GTK_EVENT_PROPAGATE;
	work = wnck_window_get_workspace(win);
	wnck_workspace_activate(work, ev->time);
	wnck_window_activate(win, ev->time);
	gtk_menu_shell_activate_item(GTK_MENU_SHELL(gtk_widget_get_parent(item)), item, TRUE);
	return GTK_EVENT_STOP;
}

gboolean
window_menu_key_press(GtkWidget *menu, GdkEvent *event, gpointer user_data)
{
	GdkEventKey *ev = (typeof(ev)) event;
	GtkWidget *item;

	item = gtk_container_get_focus_child(GTK_CONTAINER(menu));
	if (GTK_IS_MENU_ITEM(item))
		OPRINTF("Menu item [%s] key press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (ev->keyval == GDK_KEY_Return || ev->keyval == GDK_KEY_space)  {
		OPRINTF("Menu key press [Return]\n");
		if (selected_window)
			wnck_window_activate(selected_window, ev->time);
		if (selected_witem)
			gtk_menu_shell_activate_item(GTK_MENU_SHELL(menu),
						     GTK_WIDGET(selected_witem), TRUE);
		return GTK_EVENT_STOP;
	} else if (ev->keyval == GDK_KEY_Right) {
		OPRINTF("Menu key press [Right]\n");
		if (selected_window && selected_witem
		    && !gtk_menu_item_get_submenu(GTK_MENU_ITEM(selected_witem))) {
			GtkWidget *menu = wnck_action_menu_new(selected_window);
			g_signal_connect(G_OBJECT(menu), "selection_done", G_CALLBACK(selection_done), NULL);
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(selected_witem), menu);
			return GTK_EVENT_PROPAGATE;
		}
	} else if (ev->keyval == GDK_KEY_Escape) {
		OPRINTF("Menu key press [Escape]\n");
		if (!gtk_menu_get_tearoff_state(GTK_MENU(menu)))
			gtk_main_quit();
		return GTK_EVENT_PROPAGATE;
	}
	return GTK_EVENT_PROPAGATE;
}

void
add_workspace(GtkMenuItem *item, gpointer user_data)
{
	WnckScreen *scrn = user_data;
	int count;

	if ((count = wnck_screen_get_workspace_count(scrn)) && count < 32)
		wnck_screen_change_workspace_count(scrn, count + 1);
}

void
del_workspace(GtkMenuItem *item, gpointer user_data)
{
	WnckScreen *scrn = user_data;
	int count;

	if ((count = wnck_screen_get_workspace_count(scrn)) && count > 1)
		wnck_screen_change_workspace_count(scrn, count - 1);
}

void
show_child(GtkWidget *child, gpointer user_data)
{
	OPRINTF("Type of child %p is %zu\n", child, G_OBJECT_TYPE(child));
}

static GtkWidget *
popup_menu_new(WnckScreen *scrn)
{
	GtkWidget *menu, *sep;
	GList *workspaces, *workspace;
	GList *windows, *window;
	GSList *group = NULL;
	WnckWorkspace *active;
	int anum;

	menu = gtk_menu_new();
	g_signal_connect(G_OBJECT(menu), "key_press_event", G_CALLBACK(workspace_menu_key_press),
			 NULL);
	g_signal_connect(G_OBJECT(menu), "selection_done", G_CALLBACK(selection_done), NULL);
	workspaces = wnck_screen_get_workspaces(scrn);
	switch (options.order) {
	default:
	case WindowOrderDefault:
	case WindowOrderClient:
		windows = wnck_screen_get_windows(scrn);
		break;
	case WindowOrderStacking:
		windows = wnck_screen_get_windows_stacked(scrn);
		break;
	}
	active = wnck_screen_get_active_workspace(scrn);
	anum = wnck_workspace_get_number(active);
	{
		GtkWidget *item, *submenu, *icon;
		int window_count = 0;

		icon =
		    gtk_image_new_from_icon_name("preferences-system-windows", GTK_ICON_SIZE_MENU);
		item = gtk_image_menu_item_new_with_label("Iconified Windows");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
		gtk_menu_append(menu, item);
		gtk_widget_show(item);

		submenu = gtk_menu_new();
		g_signal_connect(G_OBJECT(submenu), "key_press_event", G_CALLBACK(window_menu_key_press), NULL);
		g_signal_connect(G_OBJECT(submenu), "selection_done", G_CALLBACK(selection_done), NULL);

		for (window = windows; window; window = window->next) {
			GdkPixbuf *pixbuf;
			const char *wname;
			char *label, *p;
			WnckWindow *win;
			GtkWidget *witem, *image;
			gboolean need_tooltip = FALSE;

			win = window->data;
			if (wnck_window_is_skip_tasklist(win))
				continue;
			if (wnck_window_is_pinned(win))
				continue;
			if (!wnck_window_is_minimized(win))
				continue;
			wname = wnck_window_get_name(win);
			witem = gtk_image_menu_item_new();
			label = g_strdup(wname);
			if ((p = strstr(label, " - GVIM"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			if ((p = strstr(label, " - VIM"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			if ((p = strstr(label, " - Mozilla Firefox"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			p = label;
			label = g_strdup_printf(" ○ %s", p);
			g_free(p);
			gtk_menu_item_set_label(GTK_MENU_ITEM(witem), label);
			if (strlen(label) > 44) {
				if (GTK_IS_BIN(witem)
				    && GTK_IS_LABEL(gtk_bin_get_child(GTK_BIN(witem)))) {
					GtkWidget *child = gtk_bin_get_child(GTK_BIN(witem));

					gtk_label_set_ellipsize(GTK_LABEL(child),
								PANGO_ELLIPSIZE_MIDDLE);
					gtk_label_set_max_width_chars(GTK_LABEL(child), 40);
					need_tooltip = TRUE;
				} else {
					strcpy(label + 41, "...");
					need_tooltip = TRUE;
				}
			}
			g_free(label);
			if (need_tooltip)
				gtk_widget_set_tooltip_text(witem, wname);
			pixbuf = wnck_window_get_mini_icon(win);
			if ((image = gtk_image_new_from_pixbuf(pixbuf)))
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(witem), image);
			gtk_menu_append(submenu, witem);
			gtk_widget_show(witem);
			g_signal_connect(G_OBJECT(witem), "activate", G_CALLBACK(window_activate), win);
			g_signal_connect(G_OBJECT(witem), "button_press_event", G_CALLBACK(window_menu), win);
			g_signal_connect(G_OBJECT(witem), "select", G_CALLBACK(window_select), win);
			g_signal_connect(G_OBJECT(witem), "deselect", G_CALLBACK(window_deselect), win);
#if 0
			g_object_set(gtk_widget_get_settings(GTK_WIDGET(witem)),
					"gtk-menu-popup-delay", (gint) 5000000,
					"gtk-menu-popdown-delay", (gint) 5000000,
					NULL);
#endif
			window_count++;
		}
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
		gtk_widget_show(submenu);
		gtk_widget_set_sensitive(item, window_count ? TRUE : FALSE);
	}
	sep = gtk_separator_menu_item_new();
	gtk_menu_append(menu, sep);
	gtk_widget_show(sep);
	for (workspace = workspaces; workspace; workspace = workspace->next) {
		int wnum, len;
		const char *name;
		WnckWorkspace *work;
		GtkWidget *item, *submenu, *title, *icon;
		char *label, *wkname, *p;
		int window_count = 0;

		work = workspace->data;
		wnum = wnck_workspace_get_number(work);
		name = wnck_workspace_get_name(work);
		wkname = strdup(name);
		while ((p = strrchr(wkname, ' ')) && p[1] == '\0')
			*p = '\0';
		for (p = wkname; *p == ' '; p++) ;
		len = strlen(p);
		if (len < 6 || strspn(p, " 0123456789") == len)
			label = g_strdup_printf("Workspace %s", p);
		else
			label = g_strdup_printf("[%d] %s", wnum + 1, p);
		free(wkname);
#if 1
		(void) group;
		(void) anum;
		icon =
		    gtk_image_new_from_icon_name("preferences-system-windows", GTK_ICON_SIZE_MENU);
		item = gtk_image_menu_item_new_with_label(label);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
#if 0
		if (GTK_IS_BIN(item)) {
			GtkWidget *child;

			child = gtk_bin_get_child(GTK_BIN(item));
			if (GTK_IS_LABEL(child)) {
				gtk_misc_set_alignment(GTK_MISC(child), 1.0, 0.5);
			}
		}
#endif
#else
		item = gtk_radio_menu_item_new_with_label(group, label);
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
		if (wnum == anum)
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		else
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), FALSE);
#endif
		gtk_menu_append(menu, item);
		gtk_widget_show(item);

		submenu = gtk_menu_new();
		g_signal_connect(G_OBJECT(submenu), "key_press_event", G_CALLBACK(window_menu_key_press), NULL);
		g_signal_connect(G_OBJECT(submenu), "selection_done", G_CALLBACK(selection_done), NULL);

#if 1
		icon =
		    gtk_image_new_from_icon_name("preferences-desktop-display", GTK_ICON_SIZE_MENU);
#if 0
		p = label;
		label = g_strdup_printf("%s ——", p);
		g_free(p);
#endif
		title = gtk_image_menu_item_new_with_label(label);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(title), icon);
		if (GTK_IS_BIN(title)) {
			GtkWidget *child;

			child = gtk_bin_get_child(GTK_BIN(title));
			if (GTK_IS_LABEL(child)) {
				gchar *markup;
				gint xpad = 0, ypad = 0;

				markup = g_markup_printf_escaped("<b>%s</b>", label);
				gtk_label_set_markup(GTK_LABEL(child), markup);
				g_free(markup);
				gtk_misc_set_alignment(GTK_MISC(child), 0.5, 0.5);
				gtk_misc_get_padding(GTK_MISC(child), &xpad, &ypad);
				if (ypad < 3)
					ypad = 3;
				gtk_misc_set_padding(GTK_MISC(child), xpad, ypad);
			}
		}
		gtk_menu_append(submenu, title);
		gtk_widget_show(title);
		g_signal_connect(G_OBJECT(title), "activate", G_CALLBACK(workspace_activate), work);
		sep = gtk_separator_menu_item_new();
		gtk_menu_append(submenu, sep);
		gtk_widget_show(sep);
		sep = gtk_separator_menu_item_new();
		gtk_menu_append(submenu, sep);
		gtk_widget_show(sep);
		window_count++;
#endif
		g_free(label);

		for (window = windows; window; window = window->next) {
			GdkPixbuf *pixbuf;
			const char *wname;
			char *label;
			WnckWindow *win;
			GtkWidget *witem, *image;
			gboolean need_tooltip = FALSE;

			win = window->data;
			if (!wnck_window_is_on_workspace(win, work))
				continue;
			if (wnck_window_is_skip_tasklist(win))
				continue;
			if (wnck_window_is_pinned(win))
				continue;
			if (wnck_window_is_minimized(win))
				continue;
			wname = wnck_window_get_name(win);
			witem = gtk_image_menu_item_new();
			label = g_strdup(wname);
			if ((p = strstr(label, " - GVIM"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			if ((p = strstr(label, " - VIM"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			if ((p = strstr(label, " - Geeqie"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			if ((p = strstr(label, " - Mozilla Firefox"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			p = label;
			label = g_strdup_printf(" ● %s", p);
			g_free(p);
			gtk_menu_item_set_label(GTK_MENU_ITEM(witem), label);
			if (strlen(label) > 44) {
				if (GTK_IS_BIN(witem)
				    && GTK_IS_LABEL(gtk_bin_get_child(GTK_BIN(witem)))) {
					GtkWidget *child = gtk_bin_get_child(GTK_BIN(witem));

					gtk_label_set_ellipsize(GTK_LABEL(child),
								PANGO_ELLIPSIZE_MIDDLE);
					gtk_label_set_max_width_chars(GTK_LABEL(child), 40);
					need_tooltip = TRUE;
				} else {
					strcpy(label + 41, "...");
					need_tooltip = TRUE;
				}
			}
			g_free(label);
			if (need_tooltip)
				gtk_widget_set_tooltip_text(witem, wname);
			pixbuf = wnck_window_get_mini_icon(win);
			if ((image = gtk_image_new_from_pixbuf(pixbuf)))
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(witem), image);
			gtk_menu_append(submenu, witem);
			gtk_widget_show(witem);
			g_signal_connect(G_OBJECT(witem), "activate", G_CALLBACK(window_activate), win);
			g_signal_connect(G_OBJECT(witem), "button_press_event", G_CALLBACK(window_menu), win);
			g_signal_connect(G_OBJECT(witem), "select", G_CALLBACK(window_select), win);
			g_signal_connect(G_OBJECT(witem), "deselect", G_CALLBACK(window_deselect), win);
#if 0
			g_object_set(gtk_widget_get_settings(GTK_WIDGET(witem)),
					"gtk-menu-popup-delay", (gint) 5000000,
					"gtk-menu-popdown-delay", (gint) 5000000,
					NULL);
#endif
			window_count++;
		}
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
		gtk_widget_show(submenu);
		gtk_widget_set_sensitive(item, window_count ? TRUE : FALSE);
		gtk_widget_add_events(item, GDK_ALL_EVENTS_MASK);
		g_signal_connect(G_OBJECT(item), "button_press_event", G_CALLBACK(workspace_button_press), work);
		g_signal_connect(G_OBJECT(item), "key_press_event", G_CALLBACK(workspace_key_press), work);
		g_signal_connect(G_OBJECT(item), "activate_item", G_CALLBACK(workspace_activate_item), work);
		g_signal_connect(G_OBJECT(item), "select", G_CALLBACK(workspace_select), work);
		g_signal_connect(G_OBJECT(item), "deselect", G_CALLBACK(workspace_deselect), work);
#if 0
		g_object_set(gtk_widget_get_settings(GTK_WIDGET(item)),
				"gtk-menu-popup-delay", (gint) 5000000,
				"gtk-menu-popdown-delay", (gint) 5000000,
				NULL);
#endif
	}
	sep = gtk_separator_menu_item_new();
	gtk_menu_append(menu, sep);
	gtk_widget_show(sep);
	{
		GtkWidget *item, *submenu, *icon;
		int window_count = 0;

		icon =
		    gtk_image_new_from_icon_name("preferences-system-windows", GTK_ICON_SIZE_MENU);
		item = gtk_image_menu_item_new_with_label("All Workspaces");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
		gtk_menu_append(menu, item);
		gtk_widget_show(item);

		submenu = gtk_menu_new();
		g_signal_connect(G_OBJECT(submenu), "key_press_event", G_CALLBACK(window_menu_key_press), NULL);
		g_signal_connect(G_OBJECT(submenu), "selection_done", G_CALLBACK(selection_done), NULL);

		for (window = windows; window; window = window->next) {
			GdkPixbuf *pixbuf;
			const char *wname;
			WnckWindow *win;
			GtkWidget *witem, *image;

			win = window->data;
			if (wnck_window_is_skip_tasklist(win))
				continue;
			if (!wnck_window_is_pinned(win))
				continue;
			if (wnck_window_is_minimized(win))
				continue;
			wname = wnck_window_get_name(win);
			witem = gtk_image_menu_item_new_with_label(wname);
			pixbuf = wnck_window_get_mini_icon(win);
			if ((image = gtk_image_new_from_pixbuf(pixbuf)))
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(witem), image);
			gtk_menu_append(submenu, witem);
			gtk_widget_show(witem);
			g_signal_connect(G_OBJECT(witem), "activate", G_CALLBACK(window_activate), win);
			g_signal_connect(G_OBJECT(witem), "button_press_event", G_CALLBACK(window_menu), win);
			g_signal_connect(G_OBJECT(witem), "select", G_CALLBACK(window_select), win);
			g_signal_connect(G_OBJECT(witem), "deselect", G_CALLBACK(window_deselect), win);
#if 0
			g_object_set(gtk_widget_get_settings(GTK_WIDGET(witem)),
					"gtk-menu-popup-delay", (gint) 5000000,
					"gtk-menu-popdown-delay", (gint) 5000000,
					NULL);
#endif
			window_count++;
		}
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
		gtk_widget_show(submenu);
		gtk_widget_set_sensitive(item, window_count ? TRUE : FALSE);
	}
	sep = gtk_separator_menu_item_new();
	gtk_menu_append(menu, sep);
	gtk_widget_show(sep);
	{
		GtkWidget *item, *icon;
		int count;

		icon =
		    gtk_image_new_from_icon_name("preferences-desktop-display", GTK_ICON_SIZE_MENU);
		item = gtk_image_menu_item_new_with_label("Append a workspace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
		gtk_menu_append(menu, item);
		gtk_widget_show(item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(add_workspace), scrn);
		gtk_widget_set_sensitive(item,
					 ((count = wnck_screen_get_workspace_count(scrn))
					  && count < 32));

		icon = gtk_image_new_from_icon_name("preferences-desktop-display", GTK_ICON_SIZE_MENU);
		item = gtk_image_menu_item_new_with_label("Remove last workspace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
		gtk_menu_append(menu, item);
		gtk_widget_show(item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(del_workspace), scrn);
		gtk_widget_set_sensitive(item, ((count = wnck_screen_get_workspace_count(scrn))
					  && count > 1));
	}

	gtk_widget_show_all(menu);
	return (menu);
}

/** @brief find the specified screen
  * 
  * Either specified with options.screen, or if the DISPLAY environment variable
  * specifies a screen, use that screen; otherwise, return NULL.
  */
static WnckScreen *
find_specific_screen(GdkDisplay *disp)
{
	WnckScreen *scrn = NULL;
	int nscr = gdk_display_get_n_screens(disp);

	if (0 <= options.screen && options.screen < nscr)
		/* user specified a valid screen number */
		scrn = wnck_screen_get(options.screen);
	return (scrn);
}

/** @brief find the screen of window with the focus
  */
static WnckScreen *
find_focus_screen(GdkDisplay *disp)
{
	WnckScreen *scrn = NULL;
	Window focus = None, froot = None;
	int di;
	unsigned int du;
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);

	XGetInputFocus(dpy, &focus, &di);
	if (focus != PointerRoot && focus != None) {
		XGetGeometry(dpy, focus, &froot, &di, &di, &du, &du, &du, &du);
		if (froot)
			scrn = wnck_screen_get_for_root(froot);
	}
	return (scrn);
}

static WnckScreen *
find_pointer_screen(GdkDisplay *disp)
{
	WnckScreen *scrn = NULL;
	GdkScreen *screen = NULL;

	gdk_display_get_pointer(disp, &screen, NULL, NULL, NULL);
	if (screen)
		scrn = wnck_screen_get(gdk_screen_get_number(screen));
	return (scrn);
}

static WnckScreen *
find_screen(GdkDisplay *disp)
{
	WnckScreen *scrn = NULL;

	if ((scrn = find_specific_screen(disp)))
		return (scrn);
	switch (options.which) {
	case UseScreenDefault:
		if (options.button) {
			if ((scrn = find_pointer_screen(disp)))
				return (scrn);
			if ((scrn = find_focus_screen(disp)))
				return (scrn);
		} else {
			if ((scrn = find_focus_screen(disp)))
				return (scrn);
			if ((scrn = find_pointer_screen(disp)))
				return (scrn);
		}
		break;
	case UseScreenActive:
		break;
	case UseScreenFocused:
		if ((scrn = find_focus_screen(disp)))
			return (scrn);
		break;
	case UseScreenPointer:
		if ((scrn = find_pointer_screen(disp)))
			return (scrn);
		break;
	case UseScreenSpecified:
		break;
	}

	if (!scrn)
		scrn = wnck_screen_get_default();
	return (scrn);
}

static gboolean
position_pointer(GtkMenu *menu, WnckScreen *scrn, gint *x, gint *y)
{
	GdkDisplay *disp;
	
	DPRINT();
	disp = gtk_widget_get_display(GTK_WIDGET(menu));
	gdk_display_get_pointer(disp, NULL, x, y, NULL);
	return TRUE;
}

static gboolean
position_center_monitor(GtkMenu *menu, WnckScreen *scrn, gint *x, gint *y)
{
	GdkDisplay *disp;
	GdkScreen *scr;
	GdkRectangle rect;
	gint px, py, nmon;
	GtkRequisition req;

	DPRINT();
	disp = gtk_widget_get_display(GTK_WIDGET(menu));
	gdk_display_get_pointer(disp, &scr, &px, &py, NULL);
	nmon = gdk_screen_get_monitor_at_point(scr, px, py);
	gdk_screen_get_monitor_geometry(scr, nmon, &rect);
	gtk_widget_get_requisition(GTK_WIDGET(menu), &req);

	*x = rect.x + (rect.width - req.width) / 2;
	*y = rect.y + (rect.height - req.height) / 2;

	return TRUE;
}

static gboolean
position_topleft_workarea(GtkMenu *menu, WnckScreen *scrn, gint *x, gint *y)
{
#if 1
	WnckWorkspace *wkspc;

	wkspc = wnck_screen_get_active_workspace(scrn);
	*x = wnck_workspace_get_viewport_x(wkspc);
	*y = wnck_workspace_get_viewport_y(wkspc);
#else
	GdkDisplay *disp;
	GdkScreen *scr;
	GdkRectangle rect;
	gint px, py, nmon;

	DPRINT();
	disp = gtk_widget_get_display(GTK_WIDGET(menu));
	gdk_display_get_pointer(disp, &scr, &px, &py, NULL);
	nmon = gdk_screen_get_monitor_at_point(scr, px, py);
	gdk_screen_get_monitor_geometry(scr, nmon, &rect);

	*x = rect.x;
	*y = rect.y;
#endif

	return TRUE;
}

static gboolean
position_bottomright_workarea(GtkMenu *menu, WnckScreen *scrn, gint *x, gint *y)
{
#if 1
	WnckWorkspace *wkspc;
	GtkRequisition req;

	wkspc = wnck_screen_get_active_workspace(scrn);
	gtk_widget_get_requisition(GTK_WIDGET(menu), &req);
	*x = wnck_workspace_get_viewport_x(wkspc) +
		wnck_workspace_get_width(wkspc) - req.width;
	*y = wnck_workspace_get_viewport_y(wkspc) +
		wnck_workspace_get_height(wkspc) - req.height;
#else
	GdkDisplay *disp;
	GdkScreen *scrn;
	GdkRectangle rect;
	gint px, py, nmon;
	GtkRequisition req;

	DPRINT();
	disp = gtk_widget_get_display(GTK_WIDGET(menu));
	gdk_display_get_pointer(disp, &scrn, &px, &py, NULL);
	nmon = gdk_screen_get_monitor_at_point(scrn, px, py);
	gdk_screen_get_monitor_geometry(scrn, nmon, &rect);
	gtk_widget_get_requisition(GTK_WIDGET(menu), &req);

	*x = rect.x + rect.width - req.width;
	*y = rect.y + rect.height - req.height;
#endif

	return TRUE;
}

static gboolean
position_specified(GtkMenu *menu, WnckScreen *scrn, gint *x, gint *y)
{
	int x1, y1, sw, sh;

	sw = wnck_screen_get_width(scrn);
	sh = wnck_screen_get_height(scrn);

	x1 = (options.x.sign < 0) ? sw - options.x.value : options.x.value;
	y1 = (options.y.sign < 0) ? sh - options.y.value : options.y.value;

	if (!options.w && !options.h) {
		*x = x1;
		*y = y1;
	} else {
		GtkRequisition req;
		int x2, y2;

		gtk_widget_size_request(GTK_WIDGET(menu), &req);
		x2 = x1 + options.w;
		y2 = y1 + options.h;

		if (x1 + req.width < sw)
			*x = x1;
		else if (x2 - req.width > 0)
			*x = x2 - req.width;
		else
			*x = 0;

		if (y2 + req.height < sh)
			*y = y2;
		else if (y1 - req.height > 0)
			*y = y1 - req.height;
		else
			*y = 0;
	}
	return TRUE;
}

static void
position_menu(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	WnckScreen *scrn = user_data;

	*push_in = FALSE;
	if (options.button) {
		position_pointer(menu, scrn, x, y);
		return;
	}
	switch (options.where) {
	case PositionDefault:
		position_center_monitor(menu, scrn, x, y);
		break;
	case PositionPointer:
		position_pointer(menu, scrn, x, y);
		break;
	case PositionCenter:
		position_center_monitor(menu, scrn, x, y);
		break;
	case PositionTopLeft:
		position_topleft_workarea(menu, scrn, x, y);
		break;
	case PositionBottomRight:
		position_bottomright_workarea(menu, scrn, x, y);
		break;
	case PositionSpecified:
		position_specified(menu, scrn, x, y);
		break;
	}
}

static void
do_popup(int argc, char *argv[])
{
	GdkDisplay *disp;
	WnckScreen *scrn;
	GtkWidget *menu;

	if (!(disp = gdk_display_get_default())) {
		EPRINTF("cannot get default display\n");
		exit(EXIT_FAILURE);
	}
	if (!(scrn = find_screen(disp))) {
		EPRINTF("cannot find screen\n");
		exit(EXIT_FAILURE);
	}
	wnck_screen_force_update(scrn);
	if (!(menu = popup_menu_new(scrn))) {
		EPRINTF("cannot get menu\n");
		exit(EXIT_FAILURE);
	}
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, position_menu, scrn,
		       options.button, options.timestamp);
	gtk_main();
}

static void
reparse(Display *dpy, Window root)
{
	XTextProperty xtp = { NULL, };
	char **list = NULL;
	int strings = 0;

	gtk_rc_reparse_all();
	if (XGetTextProperty(dpy, root, &xtp, _XA_XDE_THEME_NAME)) {
		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				char *rc_string;

				rc_string = g_strdup_printf("gtk-theme-name=\"%s\"", list[0]);
				gtk_rc_parse_string(rc_string);
				g_free(rc_string);
			}
			if (list)
				XFreeStringList(list);
		} else
			EPRINTF("could not get text list for property\n");
		if (xtp.value)
			XFree(xtp.value);
	} else
		DPRINTF("could not get _XDE_THEME_NAME for root 0x%lx\n", root);
}

static GdkFilterReturn
event_handler_PropertyNotify(Display *dpy, XEvent *xev)
{
	if (options.debug > 1) {
		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n",
			(xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
	}
	if (xev->xproperty.atom == _XA_XDE_THEME_NAME
	    && xev->xproperty.state == PropertyNewValue) {
		reparse(dpy, xev->xproperty.window);
		return GDK_FILTER_REMOVE;       /* event handled */
	}
	return GDK_FILTER_CONTINUE;     /* event not handled */
}

static GdkFilterReturn
event_handler_ClientMessage(Display *dpy, XEvent *xev)
{
	if (options.debug > 1) {
		fprintf(stderr, "==> ClientMessage:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xclient.window);
		fprintf(stderr, "    --> message_type = %s\n",
			XGetAtomName(dpy, xev->xclient.message_type));
		fprintf(stderr, "    --> format = %d\n", xev->xclient.format);
		switch (xev->xclient.format) {
			int i;

		case 8:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 20; i++)
				fprintf(stderr, " %02x", xev->xclient.data.b[i]);
			fprintf(stderr, "\n");
			break;
		case 16:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 10; i++)
				fprintf(stderr, " %04x", xev->xclient.data.s[i]);
			fprintf(stderr, "\n");
			break;
		case 32:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 5; i++)
				fprintf(stderr, " %08lx", xev->xclient.data.l[i]);
			fprintf(stderr, "\n");
			break;
		}
		fprintf(stderr, "<== ClientMessage:\n");
	}
	if (xev->xclient.message_type == _XA_GTK_READ_RCFILES) {
		reparse(dpy, xev->xclient.window);
		return GDK_FILTER_REMOVE;       /* event handled */
	}
	return GDK_FILTER_CONTINUE;     /* event not handled */
}

static GdkFilterReturn
handle_event(Display *dpy, XEvent *xev)
{
	switch (xev->type) {
	case PropertyNotify:
		return event_handler_PropertyNotify(dpy, xev);
	case ClientMessage:
		return event_handler_ClientMessage(dpy, xev);
	}
	return GDK_FILTER_CONTINUE;     /* event not handled, continue processing */
}

static GdkFilterReturn
filter_handler(GdkXEvent * xevent, GdkEvent * event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	Display *dpy = data;

	return handle_event(dpy, xev);
}

static void
startup(int argc, char *argv[])
{
	GdkAtom atom;
	GdkEventMask mask;
	GdkDisplay *disp;
	GdkScreen *scrn;
	GdkWindow *root;
	Display *dpy;
	char *file;
	int nscr;

	file = g_strdup_printf("%s/.gtkrc-2.0.xde", g_get_home_dir());
	gtk_rc_add_default_file(file);
	g_free(file);

	gtk_init(&argc, &argv);

	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);

	if (options.screen >= 0 && options.screen >= nscr) {
		EPRINTF("bad screen specified: %d\n", options.screen);
		exit(EXIT_FAILURE);
	}

	dpy = GDK_DISPLAY_XDISPLAY(disp);
	gdk_window_add_filter(NULL, filter_handler, dpy);

	atom = gdk_atom_intern_static_string("WM_STATE");
	_XA_WM_STATE = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_XA_XDE_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_GTK_READ_RCFILES");
	_XA_GTK_READ_RCFILES = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_WM_ICON_GEOMETRY");
	_XA_NET_WM_ICON_GEOMETRY = gdk_x11_atom_to_xatom_for_display(disp, atom);

	scrn = gdk_display_get_default_screen(disp);
	root = gdk_screen_get_root_window(scrn);
	mask = gdk_window_get_events(root);
	mask |= GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK | GDK_SUBSTRUCTURE_MASK;
	gdk_window_set_events(root, mask);

	reparse(dpy, GDK_WINDOW_XID(root));

	wnck_set_client_type(WNCK_CLIENT_TYPE_PAGER);
}

static void
copying(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stdout, "\
--------------------------------------------------------------------------------\n\
%1$s\n\
--------------------------------------------------------------------------------\n\
Copyright (c) 2008-2016  Monavacon Limited <http://www.monavacon.com/>\n\
Copyright (c) 2001-2008  OpenSS7 Corporation <http://www.openss7.com/>\n\
Copyright (c) 1997-2001  Brian F. G. Bidulock <bidulock@openss7.org>\n\
\n\
All Rights Reserved.\n\
--------------------------------------------------------------------------------\n\
This program is free software: you can  redistribute it  and/or modify  it under\n\
the terms of the  GNU Affero  General  Public  License  as published by the Free\n\
Software Foundation, version 3 of the license.\n\
\n\
This program is distributed in the hope that it will  be useful, but WITHOUT ANY\n\
WARRANTY; without even  the implied warranty of MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.\n\
\n\
You should have received a copy of the  GNU Affero General Public License  along\n\
with this program.   If not, see <http://www.gnu.org/licenses/>, or write to the\n\
Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\
--------------------------------------------------------------------------------\n\
U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on behalf\n\
of the U.S. Government (\"Government\"), the following provisions apply to you. If\n\
the Software is supplied by the Department of Defense (\"DoD\"), it is classified\n\
as \"Commercial  Computer  Software\"  under  paragraph  252.227-7014  of the  DoD\n\
Supplement  to the  Federal Acquisition Regulations  (\"DFARS\") (or any successor\n\
regulations) and the  Government  is acquiring  only the  license rights granted\n\
herein (the license rights customarily provided to non-Government users). If the\n\
Software is supplied to any unit or agency of the Government  other than DoD, it\n\
is  classified as  \"Restricted Computer Software\" and the Government's rights in\n\
the Software  are defined  in  paragraph 52.227-19  of the  Federal  Acquisition\n\
Regulations (\"FAR\")  (or any successor regulations) or, in the cases of NASA, in\n\
paragraph  18.52.227-86 of  the  NASA  Supplement  to the FAR (or any  successor\n\
regulations).\n\
--------------------------------------------------------------------------------\n\
", NAME " " VERSION);
}

static void
version(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stdout, "\
%1$s (OpenSS7 %2$s) %3$s\n\
Written by Brian Bidulock.\n\
\n\
Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016  Monavacon Limited.\n\
Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008  OpenSS7 Corporation.\n\
Copyright (c) 1997, 1998, 1999, 2000, 2001  Brian F. G. Bidulock.\n\
This is free software; see the source for copying conditions.  There is NO\n\
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\
\n\
Distributed by OpenSS7 under GNU Affero General Public License Version 3,\n\
with conditions, incorporated herein by reference.\n\
\n\
See `%1$s --copying' for copying permissions.\n\
", NAME, PACKAGE, VERSION);
}

static void
usage(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stderr, "\
Usage:\n\
    %1$s [-p|--popup] [options]\n\
    %1$s {-h|--help}\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
", argv[0]);
}

static const char *
show_order(WindowOrder order)
{
	switch (order) {
	case WindowOrderDefault:
	case WindowOrderClient:
		return ("client");
	case WindowOrderStacking:
		return ("stacking");
	}
	return NULL;
}

static const char *
show_screen(int snum)
{
	static char screen[64] = { 0, };

	if (options.screen == -1)
		return ("None");
	snprintf(screen, sizeof(screen), "%d", options.screen);
	return (screen);
}

static const char *
show_which(UseScreen which)
{
	switch (which) {
	case UseScreenDefault:
		return ("auto");
	case UseScreenActive:
		return ("active");
	case UseScreenFocused:
		return ("focused");
	case UseScreenPointer:
		return ("pointer");
	case UseScreenSpecified:
		return show_screen(options.screen);
	}
	return NULL;
}

static const char *
show_where(MenuPosition where)
{
	static char position[128] = { 0, };

	switch (where) {
	case PositionDefault:
		return ("auto");
	case PositionPointer:
		return ("pointer");
	case PositionCenter:
		return ("center");
	case PositionTopLeft:
		return ("topleft");
	case PositionBottomRight:
		return ("bottomright");
	case PositionSpecified:
		snprintf(position, sizeof(position), "%ux%u%c%d%c%d", options.w, options.h,
			 (options.x.sign < 0) ? '-' : '+', options.x.value,
			 (options.y.sign < 0) ? '-' : '+', options.y.value);
		return (position);
	}
	return NULL;
}

static void
help(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	/* *INDENT-OFF* */
	(void) fprintf(stdout, "\
Usage:\n\
    %1$s [-p|--popup] [options]\n\
    %1$s {-h|--help} [options]\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
Command options:\n\
   [-p, --popup]\n\
        pop up the menu\n\
    -h, --help, -?, --?\n\
        print this usage information and exit\n\
    -V, --version\n\
        print version and exit\n\
    -C, --copying\n\
        print copying permission and exit\n\
Options:\n\
    -d, --display DISPLAY\n\
        specify the X display, DISPLAY, to use [default: %4$s]\n\
    -s, --screen SCREEN\n\
        specify the screen number, SCREEN, to use [default: %5$d]\n\
    -b, --button BUTTON\n\
        specify the mouse button number, BUTTON, for popup [default: %6$d]\n\
    -T, --timestamp TIMESTAMP\n\
        use the time, TIMESTAMP, for button/keyboard event [default: %7$lu]\n\
    -w, --which {active|focused|pointer|select}\n\
        specify the screen for which to pop the menu [default: %8$s]\n\
        \"active\"   - the screen with EWMH/NetWM active client\n\
        \"focused\"  - the screen with EWMH/NetWM focused client\n\
        \"pointer\"  - the screen with EWMH/NetWM pointer\n\
        \"SCREEN\"   - the specified screen number\n\
    -W, --where {pointer|center|topleft|GEOMETRY}\n\
        specify where to place the menu [default: %9$s]\n\
        \"pointer\"  - northwest corner under the pointer\n\
        \"center\"   - center of associated monitor\n\
        \"topleft\"  - northwest corner of work area\n\
        POSITION   - postion on screen as X geometry\n\
    -O, --order {client|stacking}\n\
        specify the order of windows [default: %10$s]\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %2$d]\n\
        this option may be repeated.\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %3$d]\n\
        this option may be repeated.\n\
", argv[0]
	, options.debug
	, options.output
	, options.display
	, options.screen
	, options.button
	, options.timestamp
	, show_which(options.which)
	, show_where(options.where)
	, show_order(options.order)
);
	/* *INDENT-ON* */
}

static void
set_defaults(void)
{
	const char *env;

	if ((env = getenv("DISPLAY")))
		options.display = strdup(env);
}

static void
get_defaults(void)
{
	const char *p;
	int n;

	if (!options.display) {
		EPRINTF("No DISPLAY environment variable nor --display option\n");
		exit(EXIT_FAILURE);
	}
	if (options.screen < 0 && (p = strrchr(options.display, '.'))
	    && (n = strspn(++p, "0123456789")) && *(p + n) == '\0')
		options.screen = atoi(p);
	if (options.command == CommandDefault)
		options.command = CommandPopup;
}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;

	setlocale(LC_ALL, "");

	set_defaults();

	while (1) {
		int c, val, len;
		char *endptr = NULL;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"display",		required_argument,	NULL,	'd'},
			{"screen",		required_argument,	NULL,	's'},
			{"popup",		no_argument,		NULL,	'p'},
			{"button",		required_argument,	NULL,	'b'},
			{"timestamp",		required_argument,	NULL,	'T'},
			{"which",		required_argument,	NULL,	'w'},
			{"where",		required_argument,	NULL,	'W'},

			{"order",		required_argument,	NULL,	'O'},
			{"debug",		optional_argument,	NULL,	'D'},
			{"verbose",		optional_argument,	NULL,	'v'},
			{"help",		no_argument,		NULL,	'h'},
			{"version",		no_argument,		NULL,	'V'},
			{"copying",		no_argument,		NULL,	'C'},
			{"?",			no_argument,		NULL,	'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "d:s:pb:T:w:W:O:D::v::hVCH?",
				     long_options, &option_index);
#else                           /* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:pb:T:w:W:O:D:vhVCH?");
#endif                          /* _GNU_SOURCE */
		if (c == -1) {
			if (options.debug)
				fprintf(stderr, "%s: done options processing\n", argv[0]);
			break;
		}
		switch (c) {
		case 0:
			goto bad_usage;

		case 'd':       /* -d, --display DISPLAY */
			setenv("DISPLAY", optarg, TRUE);
			free(options.display);
			options.display = strdup(optarg);
			break;
		case 's':       /* -s, --screen SCREEN */
			options.screen = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			break;
		case 'p':       /* -p, --popup */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandPopup;
			options.command = CommandPopup;
			break;

		case 'b':       /* -b, --button BUTTON */
			options.button = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			break;
		case 'T':       /* -T, --timestamp TIMESTAMP */
			options.timestamp = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			break;

		case 'w':	/* -w, --which WHICH */
			if (options.which != UseScreenDefault)
				goto bad_option;
			if (!(len = strlen(optarg)))
				goto bad_option;
			if (!strncasecmp("active", optarg, len))
				options.which = UseScreenActive;
			else if (!strncasecmp("focused", optarg, len))
				options.which = UseScreenFocused;
			else if (!strncasecmp("pointer", optarg, len))
				options.which = UseScreenPointer;
			else {
				options.screen = strtoul(optarg, &endptr, 0);
				if (endptr && *endptr)
					goto bad_option;
				options.which = UseScreenSpecified;
			}
			break;
		case 'W':       /* -W, --where WHERE */
			if (options.where != PositionDefault)
				goto bad_option;
			if (!(len = strlen(optarg)))
				goto bad_option;
			if (!strncasecmp("pointer", optarg, len))
				options.where = PositionPointer;
			else if (!strncasecmp("center", optarg, len))
				options.where = PositionCenter;
			else if (!strncasecmp("topleft", optarg, len))
				options.where = PositionTopLeft;
			else if (!strncasecmp("bottomright", optarg, len))
				options.where = PositionBottomRight;
			else {
				int mask, x = 0, y = 0;
				unsigned int w = 0, h = 0;

				mask = XParseGeometry(optarg, &x, &y, &w, &h);
				if (!(mask & XValue) || !(mask & YValue))
					goto bad_option;
				options.where = PositionSpecified;
				options.x.value = x;
				options.x.sign = (mask & XNegative) ? -1 : 1;
				options.y.value = y;
				options.y.sign = (mask & YNegative) ? -1 : 1;
				options.w = w;
				options.h = h;
			}
			break;

		case 'O':	/* -O, --order ORDERTYPE */
			if (options.order != WindowOrderDefault)
				goto bad_option;
			len = strlen(optarg);
			if (!strncasecmp("client", optarg, len))
				options.order = WindowOrderClient;
			else
			if (!strncasecmp("stacking", optarg, len))
				options.order = WindowOrderStacking;
			else
				goto bad_option;
			break;

		case 'D':       /* -D, --debug [LEVEL] */
			if (options.debug)
				fprintf(stderr, "%s: increasing debug verbosity\n", argv[0]);
			if (optarg == NULL) {
				options.debug++;
				break;
			}
			if ((val = strtol(optarg, &endptr, 0)) < 0)
				goto bad_option;
			if (endptr && *endptr)
				goto bad_option;
			options.debug = val;
			break;
		case 'v':       /* -v, --verbose [LEVEL] */
			if (options.debug)
				fprintf(stderr, "%s: increasing output verbosity\n", argv[0]);
			if (optarg == NULL) {
				options.output++;
				break;
			}
			if ((val = strtol(optarg, &endptr, 0)) < 0)
				goto bad_option;
			if (endptr && *endptr)
				goto bad_option;
			options.output = val;
			break;
		case 'h':       /* -h, --help */
		case 'H':       /* -H, --? */
			command = CommandHelp;
			break;
		case 'V':       /* -V, --version */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandVersion;
			options.command = CommandVersion;
			break;
		case 'C':       /* -C, --copying */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandCopying;
			options.command = CommandCopying;
			break;
		case '?':
		default:
		      bad_option:
			optind--;
			goto bad_nonopt;
		      bad_nonopt:
			if (options.output || options.debug) {
				if (optind < argc) {
					fprintf(stderr, "%s: syntax error near '", argv[0]);
					while (optind < argc) {
						fprintf(stderr, "%s", argv[optind++]);
						fprintf(stderr, "%s", (optind < argc) ? " " : "");
					}
					fprintf(stderr, "'\n");
				} else {
					fprintf(stderr, "%s: missing option or argument", argv[0]);
					fprintf(stderr, "\n");
				}
				fflush(stderr);
			      bad_usage:
				usage(argc, argv);
			}
			exit(EXIT_SYNTAXERR);
		}
	}
	if (options.debug) {
		fprintf(stderr, "%s: option index = %d\n", argv[0], optind);
		fprintf(stderr, "%s: option count = %d\n", argv[0], argc);
	}
	if (optind < argc) {
		fprintf(stderr, "%s: excess non-option arguments near '", argv[0]);
		while (optind < argc) {
			fprintf(stderr, "%s", argv[optind++]);
			fprintf(stderr, "%s", (optind < argc) ? " " : "");
		}
		fprintf(stderr, "'\n");
		usage(argc, argv);
		exit(EXIT_SYNTAXERR);
	}
	get_defaults();
	startup(argc, argv);
	switch (command) {
	default:
	case CommandDefault:
	case CommandPopup:
		DPRINTF("%s: popping the menu\n", argv[0]);
		do_popup(argc, argv);
		break;
	case CommandHelp:
		DPRINTF("%s: printing help message\n", argv[0]);
		help(argc, argv);
		break;
	case CommandVersion:
		DPRINTF("%s: printing version message\n", argv[0]);
		version(argc, argv);
		break;
	case CommandCopying:
		DPRINTF("%s: printing copying message\n", argv[0]);
		copying(argc, argv);
		break;
	}
	exit(EXIT_SUCCESS);
}

// vim: tw=100 com=sr0\:/**,mb\:*,ex\:*/,sr0\:/*,mb\:*,ex\:*/,b\:TRANS formatoptions+=tcqlor

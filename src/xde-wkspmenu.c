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

/** @section Headers
  * @{ */

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
#include <sys/timerfd.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/poll.h>
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
#include <wordexp.h>
#include <execinfo.h>
#include <math.h>

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
#include <glib-unix.h>
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

/** @} */

/** @section Preamble
  * @{ */

#define XPRINTF(_args...) do { } while (0)

#define DPRINTF(_num, _args...) do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": D: %12s: +%4d : %s() : ", __FILE__, __LINE__, __func__); \
		fprintf(stderr, _args); fflush(stderr); } } while (0)

#define EPRINTF(_args...) do { \
		fprintf(stderr, NAME ": E: %12s +%4d : %s() : ", __FILE__, __LINE__, __func__); \
		fprintf(stderr, _args); fflush(stderr); } while (0)

#define OPRINTF(_num, _args...) do { if (options.debug >= _num || options.output > _num) { \
		fprintf(stdout, NAME ": I: "); \
		fprintf(stdout, _args); fflush(stdout); } } while (0)

#define PTRACE(_num) do { if (options.debug >= _num || options.output >= _num) { \
		fprintf(stderr, NAME ": T: %12s +%4d : %s()\n", __FILE__, __LINE__, __func__); \
		fflush(stderr); } } while (0)

void
dumpstack(const char *file, const int line, const char *func)
{
	void *buffer[32];
	int nptr;
	char **strings;
	int i;

	if ((nptr = backtrace(buffer, 32)) && (strings = backtrace_symbols(buffer, nptr)))
		for (i = 0; i < nptr; i++)
			fprintf(stderr, NAME ": E: %12s +%4d : %s() : \t%s\n", file, line, func, strings[i]);
}

#undef EXIT_SUCCESS
#undef EXIT_FAILURE
#undef EXIT_SYNTAXERR

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1
#define EXIT_SYNTAXERR  2

#define GTK_EVENT_STOP		TRUE
#define GTK_EVENT_PROPAGATE	FALSE

const char *program = NAME;

#define XA_PREFIX               "_XDE_WKSPMENU"
#define XA_SELECTION_NAME	XA_PREFIX "_S%d"
#define XA_NET_DESKTOP_LAYOUT	"_NET_DESKTOP_LAYOUT_S%d"
#define LOGO_NAME		"metacity"

static int saveArgc;
static char **saveArgv;

#define RESNAME "xde-wkspmenu"
#define RESCLAS "XDE-WkspMenu"
#define RESTITL "XDG Workspace Menu"

#define USRDFLT "%s/.config/" RESNAME "/rc"
#define APPDFLT "/usr/share/X11/app-defaults/" RESCLAS

/** @} */

/** @section Globals and Structures
  * @{ */

static Atom _XA_XDE_THEME_NAME;
static Atom _XA_GTK_READ_RCFILES;
// static Atom _XA_NET_DESKTOP_LAYOUT;
static Atom _XA_NET_CURRENT_DESKTOP;
static Atom _XA_WIN_WORKSPACE;
static Atom _XA_WM_DESKTOP;
// static Atom _XA_XROOTPMAP_ID;
// static Atom _XA_ESETROOT_PMAP_ID;
static Atom _XA_NET_DESKTOP_NAMES;
static Atom _XA_NET_NUMBER_OF_DESKTOPS;
static Atom _XA_WIN_WORKSPACE_COUNT;
// static Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
static Atom _XA_NET_WM_ICON_GEOMETRY;
#if 0
static Atom _XA_WIN_AREA;
static Atom _XA_WIN_AREA_COUNT;
#endif
static Atom _XA_NET_ACTIVE_WINDOW;
static Atom _XA_NET_CLIENT_LIST;
static Atom _XA_NET_CLIENT_LIST_STACKING;
static Atom _XA_WIN_FOCUS;
static Atom _XA_WIN_CLIENT_LIST;
#ifdef STARTUP_NOTIFICATION
// static Atom _XA_NET_STARTUP_INFO;
// static Atom _XA_NET_STARTUP_INFO_BEGIN;
#endif				/* STARTUP_NOTIFICATION */


typedef enum {
	CommandDefault,
	CommandRun,
	CommandHelp,
	CommandVersion,
	CommandCopying,
} Command;

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
	WindowOrderDefault,
	WindowOrderClient,
	WindowOrderStacking,
} WindowOrder;

typedef struct {
	int mask, x, y;
	unsigned int w, h;
} XdeGeometry;

typedef struct {
	int debug;
	int output;
	char *display;
	int screen;
	int monitor;
	Bool keyboard;
	Bool pointer;
	int button;
	Time timestamp;
	UseScreen which;
	MenuPosition where;
	XdeGeometry geom;
	WindowOrder order;
	char *keys;
	Bool cycle;
	Bool hidden;
	Bool minimized;
	Bool normal;
	Bool monitors;
	Bool workspaces;
	Bool activate;
	Bool raise;
	Bool restore;
	Command command;
	Bool dryrun;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.keyboard = False,
	.pointer = False,
	.button = 0,
	.timestamp = CurrentTime,
	.which = UseScreenDefault,
	.where = PositionDefault,
	.geom = {
		.mask = 0,
		.x = 0,
		.y = 0,
		.w = 0,
		.h = 0,
	},
	.order = WindowOrderDefault,
	.keys = NULL,
	.cycle = False,
	.hidden = False,
	.minimized = False,
	.normal = False,
	.monitors = False,
	.workspaces = False,
	.activate = True,
	.raise = False,
	.restore = True,
	.command = CommandDefault,
	.dryrun = False,
};

Display *dpy = NULL;
GdkDisplay *disp = NULL;

/** @} */

/** @section Menu Handling
  * @{ */

void
selection_done(GtkMenuShell *menushell, gpointer user_data)
{
	OPRINTF(1, "Selection done: exiting\n");
	if (!gtk_menu_get_tearoff_state(GTK_MENU(menushell)))
		gtk_main_quit();
}

void
workspace_activate(GtkMenuItem *item, gpointer user_data)
{
	OPRINTF(1, "Menu item [%s] activated\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	wnck_workspace_activate(user_data, gtk_get_current_event_time());
}

void
workspace_activate_item(GtkMenuItem *item, gpointer user_data)
{
	OPRINTF(1, "Menu item [%s] activate item\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	wnck_workspace_activate(user_data, gtk_get_current_event_time());
}

gboolean
workspace_button_press(GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	GdkEventButton *ev = (typeof(ev)) event;

	OPRINTF(1, "Menu item [%s] button press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (ev->button != 1)
		return GTK_EVENT_PROPAGATE;
	// workspace_activate(GTK_MENU_ITEM(item), user_data);
	wnck_workspace_activate(user_data, ev->time);
	gtk_menu_shell_activate_item(GTK_MENU_SHELL(gtk_widget_get_parent(item)), item, TRUE);
	return GTK_EVENT_STOP;
}

void
workspace_select(GtkItem *item, gpointer user_data)
{
	GtkWidget *menu;

	OPRINTF(1, "Menu item [%s] selected\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	g_object_set_data(G_OBJECT(item), "workspace", user_data);
	if ((menu = gtk_widget_get_parent(GTK_WIDGET(item))))
		g_object_set_data(G_OBJECT(menu), "selected-item", item);
}

void
workspace_deselect(GtkItem *item, gpointer user_data)
{
	GtkWidget *menu;

	OPRINTF(1, "Menu item [%s] deselected\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if ((menu = gtk_widget_get_parent(GTK_WIDGET(item))))
		g_object_set_data(G_OBJECT(menu), "selected-item", NULL);
}

gboolean
workspace_key_press(GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	GdkEventKey *ev = (typeof(ev)) event;

	OPRINTF(1, "Menu item [%s] key press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
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
	WnckWorkspace *work;

	OPRINTF(1, "Window menu key press\n");
	if (!(item = g_object_get_data(G_OBJECT(menu), "selected-item"))) {
		OPRINTF(1, "No selected item!\n");
		return GTK_EVENT_PROPAGATE;
	}
	if (GTK_IS_MENU_ITEM(item))
		OPRINTF(1, "Menu item [%s] key press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (!(work = g_object_get_data(G_OBJECT(item), "workspace"))) {
		OPRINTF(1, "No selected workspace!\n");
		return GTK_EVENT_PROPAGATE;
	}
	if (ev->keyval == GDK_KEY_Return || ev->keyval == GDK_KEY_space) {
		OPRINTF(1, "Menu key press [Return]\n");
		wnck_workspace_activate(work, ev->time);
		gtk_menu_shell_activate_item(GTK_MENU_SHELL(menu), item, TRUE);
		return GTK_EVENT_STOP;
	}
	return GTK_EVENT_PROPAGATE;
}

void
window_activate(GtkMenuItem *item, gpointer user_data)
{
	WnckWindow *win = user_data;
	WnckWorkspace *work;

	OPRINTF(1, "Menu item [%s] activated\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if ((work = wnck_window_get_workspace(win)))
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

void
window_select(GtkItem *item, gpointer user_data)
{
	GtkWidget *menu;

	OPRINTF(1, "Menu item [%s] selected\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	g_object_set_data(G_OBJECT(item), "window", user_data);
	if ((menu = gtk_widget_get_parent(GTK_WIDGET(item))))
		g_object_set_data(G_OBJECT(menu), "selected-item", item);
}

void
window_deselect(GtkItem *item, gpointer user_data)
{
	GtkWidget *menu;

	OPRINTF(1, "Menu item [%s] deselected\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), NULL);
	if ((menu = gtk_widget_get_parent(GTK_WIDGET(item))))
		g_object_set_data(G_OBJECT(menu), "selected-item", NULL);
}

gboolean
window_key_press(GtkWidget *item, GdkEvent *event, gpointer user_data)
{
	GdkEventKey *ev = (typeof(ev)) event;
	WnckWindow *win = user_data;
	WnckWorkspace *work;

	OPRINTF(1, "Menu item [%s] key press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
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
	GtkWidget *item, *submenu;
	WnckWindow *win;

	OPRINTF(1, "Workspace menu key press\n");
	if (!(item = g_object_get_data(G_OBJECT(menu), "selected-item"))) {
		OPRINTF(1, "No selected item!\n");
		return GTK_EVENT_PROPAGATE;
	}
	if (GTK_IS_MENU_ITEM(item))
		OPRINTF(1, "Menu item [%s] key press\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	if (!(win = g_object_get_data(G_OBJECT(item), "window"))) {
		OPRINTF(1, "No selected window!\n");
		return GTK_EVENT_PROPAGATE;
	}
	if (ev->keyval == GDK_KEY_Return || ev->keyval == GDK_KEY_space)  {
		OPRINTF(1, "Menu key press [Return]\n");
		wnck_window_activate(win, ev->time);
		gtk_menu_shell_activate_item(GTK_MENU_SHELL(menu), item, TRUE);
		return GTK_EVENT_STOP;
	} else if (ev->keyval == GDK_KEY_Right) {
		OPRINTF(1, "Menu key press [Right]\n");
		if (gtk_menu_item_get_submenu(GTK_MENU_ITEM(item)))
			return GTK_EVENT_PROPAGATE;
		submenu = wnck_action_menu_new(win);
		g_signal_connect(G_OBJECT(submenu), "selection_done", G_CALLBACK(selection_done), NULL);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
		return GTK_EVENT_PROPAGATE;
	} else if (ev->keyval == GDK_KEY_Escape) {
		OPRINTF(1, "Menu key press [Escape]\n");
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
	OPRINTF(1, "Type of child %p is %zu\n", child, G_OBJECT_TYPE(child));
}

static GtkWidget *
popup_menu_new(WnckScreen *scrn)
{
	GtkWidget *menu, *sep;
	GList *workspaces, *workspace;
	GList *windows, *window;
	WnckWorkspace *active;
	int anum;
	GSList *group = NULL;

	menu = gtk_menu_new();
	g_signal_connect(G_OBJECT(menu), "key_press_event", G_CALLBACK(workspace_menu_key_press), NULL);
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
			g_object_set_data(G_OBJECT(witem), "window", win);
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
		WnckWorkspace *work;
		const char *name;
		GtkWidget *item, *submenu, *title, *icon;
		char *label, *wkname, *p;
		int window_count = 0;

		work = workspace->data;
		wnum = wnck_workspace_get_number(work);
		if  ((name = wnck_workspace_get_name(work)))
			wkname = g_strdup(name);
		else
			wkname = g_strdup_printf("Workspace %d", wnum + 1);
		while ((p = strrchr(wkname, ' ')) && p[1] == '\0')
			*p = '\0';
		for (p = wkname; *p == ' '; p++) ;
		len = strlen(p);
		if (len < 6 || strspn(p, " 0123456789") == len)
			label = g_strdup_printf("Workspace %s", p);
		else
			label = g_strdup_printf("[%d] %s", wnum + 1, p);
		g_free(wkname);
#if 1
#if 1
		(void) group;
		(void) anum;
		icon = gtk_image_new_from_icon_name("preferences-system-windows", GTK_ICON_SIZE_MENU);
		item = gtk_image_menu_item_new_with_label(label);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
#if 1
		if (wnum == anum && GTK_IS_BIN(item)) {
			GtkWidget *child = gtk_bin_get_child(GTK_BIN(item));

			if (GTK_IS_LABEL(child)) {
				gchar *markup;

				markup = g_markup_printf_escaped("<u>%s</u>", label);
				gtk_label_set_markup(GTK_LABEL(child), markup);
				g_free(markup);
			}
		}
#endif
#else
#if 1
		item = gtk_radio_menu_item_new_with_label(group, label);
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
#else
		(void) group;
		item = gtk_check_menu_item_new_with_label(label);
		gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(item), TRUE);
#endif
		if (wnum == anum) {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
		} else {
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), FALSE);
		}
#endif
#if 0
		if (GTK_IS_BIN(item)) {
			GtkWidget *child;

			child = gtk_bin_get_child(GTK_BIN(item));
			if (GTK_IS_LABEL(child)) {
				gchar *markup;
				gint xpad = 0, ypad = 0;

				if (wnum == anum)
					markup = g_markup_printf_escaped("<b><u>%s</u></b>", label);
				else
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
#endif
#endif
		gtk_menu_append(menu, item);
		gtk_widget_show(item);

		submenu = gtk_menu_new();
		g_signal_connect(G_OBJECT(submenu), "key_press_event", G_CALLBACK(window_menu_key_press), NULL);
		g_signal_connect(G_OBJECT(submenu), "selection_done", G_CALLBACK(selection_done), NULL);

#if 1
		icon = gtk_image_new_from_icon_name("preferences-desktop-display", GTK_ICON_SIZE_MENU);
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
			WnckWindowState state;
			const char *wname;
			char *dname;
			WnckWindow *win;
			GtkWidget *witem, *image;
			gboolean hidden = FALSE, iconic = FALSE;
			gboolean need_tooltip = FALSE;

			win = window->data;
			if (!wnck_window_is_on_workspace(win, work))
				continue;
			state = wnck_window_get_state(win);
			if (state & WNCK_WINDOW_STATE_SKIP_TASKLIST)
				continue;
			if (wnck_window_is_pinned(win))
				continue;
			if (state & WNCK_WINDOW_STATE_HIDDEN) {
				hidden = TRUE;
			} else if (state & WNCK_WINDOW_STATE_MINIMIZED) {
				iconic = TRUE;
			}
			wname = wnck_window_get_name(win);
			witem = gtk_image_menu_item_new();
			dname = g_strdup(wname);
			if ((p = strstr(dname, " - GVIM"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			if ((p = strstr(dname, " - VIM"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			if ((p = strstr(dname, " - Geeqie"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			if ((p = strstr(dname, " - Mozilla Firefox"))) {
				*p = '\0';
				need_tooltip = TRUE;
			}
			p = dname;
			if (hidden || iconic) {
				if (wnck_window_is_shaded(win))
					dname = g_strdup_printf(" ▬ %s", p);
				else
					dname = g_strdup_printf(" ○ %s", p);
				// dname = g_strdup_printf(" ▼ %s", p);
			} else
				dname = g_strdup_printf(" ● %s", p);
			g_free(p);
			gtk_menu_item_set_label(GTK_MENU_ITEM(witem), dname);
			if (strlen(dname) > 44) {
				if (GTK_IS_BIN(witem)
				    && GTK_IS_LABEL(gtk_bin_get_child(GTK_BIN(witem)))) {
					GtkWidget *child = gtk_bin_get_child(GTK_BIN(witem));

					gtk_label_set_ellipsize(GTK_LABEL(child),
								PANGO_ELLIPSIZE_MIDDLE);
					gtk_label_set_max_width_chars(GTK_LABEL(child), 40);
					need_tooltip = TRUE;
				} else {
					strcpy(dname + 41, "...");
					need_tooltip = TRUE;
				}
			}
			g_free(dname);
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
			g_object_set_data(G_OBJECT(witem), "window", win);
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
		g_object_set_data(G_OBJECT(item), "workspace", work);
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
			g_object_set_data(G_OBJECT(witem), "window", win);
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

/** @} */

/** @section Finding Screens and Monitors
  * @{ */

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

/** @} */

/** @section Menu Positioning
  * @{ */

static gboolean
position_pointer(GtkMenu *menu, WnckScreen *scrn, gint *x, gint *y)
{
	GdkDisplay *disp;
	
	PTRACE(5);
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

	PTRACE(5);
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

	PTRACE(5);
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

	PTRACE(5);
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

	x1 = (options.geom.mask & XNegative) ? sw - options.geom.x : options.geom.x;
	y1 = (options.geom.mask & YNegative) ? sh - options.geom.y : options.geom.y;

	if (!(options.geom.mask & (WidthValue | HeightValue))) {
		*x = x1;
		*y = y1;
	} else {
		GtkRequisition req;
		int x2, y2;

		gtk_widget_size_request(GTK_WIDGET(menu), &req);
		x2 = x1 + options.geom.w;
		y2 = y1 + options.geom.h;

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

/** @} */

/** @section X Resources
  * @{ */

/** @section Putting X Resources
  * @{ */

void
put_nc_resource(XrmDatabase xrdb, const char *prefix, const char *resource, const char *value)
{
	static char specifier[64];

	snprintf(specifier, sizeof(specifier), "%s.%s", prefix, resource);
	XrmPutStringResource(&xrdb, specifier, value);
}

void
put_resource(XrmDatabase xrdb, const char *resource, char *value)
{
	put_nc_resource(xrdb, RESCLAS, resource, value);
	g_free(value);
}

char *
putXrmInt(int integer)
{
	return g_strdup_printf("%d", integer);
}

char *
putXrmUint(unsigned int integer)
{
	return g_strdup_printf("%u", integer);
}

char *
putXrmTime(Time time)
{
	return g_strdup_printf("%lu", time);
}

char *
putXrmBlanking(unsigned int integer)
{
	switch (integer) {
	case DontPreferBlanking:
		return g_strdup("DontPreferBlanking");
	case PreferBlanking:
		return g_strdup("PreferBlanking");
	case DefaultBlanking:
		return g_strdup("DefaultBlanking");
	default:
		return g_strdup_printf("%u", integer);
	}
}

char *
putXrmExposures(unsigned int integer)
{
	switch (integer) {
	case DontAllowExposures:
		return g_strdup("DontAllowExposures");
	case AllowExposures:
		return g_strdup("AllowExposures");
	case DefaultExposures:
		return g_strdup("DefaultExposures");
	default:
		return g_strdup_printf("%u", integer);
	}
}

char *
putXrmButton(unsigned int integer)
{
	switch (integer) {
	case Button1:
		return g_strdup("Button1");
	case Button2:
		return g_strdup("Button2");
	case Button3:
		return g_strdup("Button3");
	case Button4:
		return g_strdup("Button4");
	case Button5:
		return g_strdup("Button5");
	default:
		return g_strdup_printf("%u", integer);
	}
}

#if 0
char *
putXrmPowerLevel(unsigned integer)
{
	switch (integer) {
	case DPMSModeOn:
		return g_strdup("DPMSModeOn");
	case DPMSModeStandby:
		return g_strdup("DPMSModeStandby");
	case DPMSModeSuspend:
		return g_strdup("DPMSModeSuspend");
	case DPMSModeOff:
		return g_strdup("DPMSModeOff");
	default:
		return putXrmUint(integer);
	}
}
#endif

char *
putXrmDouble(double floating)
{
	return g_strdup_printf("%f", floating);
}

char *
putXrmBool(Bool boolean)
{
	return g_strdup(boolean ? "true" : "false");
}

char *
putXrmString(const char *string)
{
	return g_strdup(string);
}

char *
putXrmWhich(UseScreen which, int screen)
{
	switch (which) {
	case UseScreenDefault:
		return g_strdup("default");
	case UseScreenActive:
		return g_strdup("active");
	case UseScreenFocused:
		return g_strdup("focused");
	case UseScreenPointer:
		return g_strdup("pointer");
	case UseScreenSpecified:
		return putXrmInt(screen);
	}
	return (NULL);
}

char *
putXrmXGeometry(XdeGeometry *geom)
{
	char *wh = NULL, *xy = NULL, *result = NULL;

	if (geom->mask & (WidthValue | HeightValue))
		wh = g_strdup_printf("%ux%u", geom->w, geom->h);
	if (geom->mask & (XValue | YValue))
		xy = g_strdup_printf("%c%d%c%d",
				     (geom->mask & XNegative) ? '-' : '+', geom->x,
				     (geom->mask & YNegative) ? '-' : '+', geom->y);
	if (wh && xy) {
		result = g_strconcat(wh, xy, NULL);
		g_free(wh);
		g_free(xy);
	} else if (wh) {
		result = wh;
	} else if (xy) {
		result = xy;
	}
	return (result);
}

char *
putXrmWhere(MenuPosition where, XdeGeometry *geom)
{
	switch (where) {
	case PositionDefault:
		return g_strdup("default");
	case PositionCenter:
		return g_strdup("center");
	case PositionPointer:
		return g_strdup("pointer");
	case PositionTopLeft:
		return g_strdup("topleft");
	case PositionBottomRight:
		return g_strdup("bottomright");
	case PositionSpecified:
		return putXrmXGeometry(geom);
	}
	return (NULL);
}

char *
putXrmOrder(WindowOrder order)
{
	switch (order) {
	case WindowOrderDefault:
		return g_strdup("default");
	case WindowOrderClient:
		return g_strdup("client");
	case WindowOrderStacking:
		return g_strdup("stacking");
	}
	return (NULL);
}

void
put_resources(void)
{
	XrmDatabase rdb;
	char *val, *usrdb;

	usrdb = g_strdup_printf("%s/.config/xde/inputrc", getenv("HOME"));

	rdb = XrmGetStringDatabase("");
	if (!rdb) {
		DPRINTF(1, "no resource manager database allocated\n");
		return;
	}
	if ((val = putXrmInt(options.debug)))
		put_resource(rdb, "debug", val);
	if ((val = putXrmInt(options.output)))
		put_resource(rdb, "verbose", val);
	/* put a bunch of resources */
	if ((val = putXrmWhich(options.which, options.screen)))
		put_resource(rdb, "which", val);
	if ((val = putXrmWhere(options.where, &options.geom)))
		put_resource(rdb, "where", val);
	if ((val = putXrmOrder(options.order)))
		put_resource(rdb, "order", val);
	if ((val = putXrmString(options.keys)))
		put_resource(rdb, "keys", val);
	if ((val = putXrmBool(options.cycle)))
		put_resource(rdb, "cycle", val);
	if ((val = putXrmBool(options.normal)))
		put_resource(rdb, "normal", val);
	if ((val = putXrmBool(options.minimized)))
		put_resource(rdb, "minimized", val);
	if ((val = putXrmBool(options.monitors)))
		put_resource(rdb, "allMonitors", val);
	if ((val = putXrmBool(options.workspaces)))
		put_resource(rdb, "allWorkspaces", val);
	if ((val = putXrmBool(options.activate)))
		put_resource(rdb, "activate", val);
	if ((val = putXrmBool(options.raise)))
		put_resource(rdb, "raise", val);
	if ((val = putXrmBool(options.restore)))
		put_resource(rdb, "restore", val);

	XrmPutFileDatabase(rdb, usrdb);
	XrmDestroyDatabase(rdb);
	return;
}

/** @} */

/** @section Getting X Resources
  * @{ */

const char *
get_nc_resource(XrmDatabase xrdb, const char *res_name, const char *res_class, const char *resource)
{
	char *type;
	static char name[64];
	static char clas[64];
	XrmValue value = { 0, NULL };

	snprintf(name, sizeof(name), "%s.%s", res_name, resource);
	snprintf(clas, sizeof(clas), "%s.%s", res_class, resource);
	if (XrmGetResource(xrdb, name, clas, &type, &value)) {
		if (value.addr && *(char *) value.addr) {
			DPRINTF(1, "%s:\t\t%s\n", clas, value.addr);
			return (const char *) value.addr;
		} else
			DPRINTF(1, "%s:\t\t%s\n", clas, value.addr);
	} else
		DPRINTF(1, "%s:\t\t%s\n", clas, "ERROR!");
	return (NULL);
}

const char *
get_resource(XrmDatabase xrdb, const char *resource, const char *dflt)
{
	const char *value;

	if (!(value = get_nc_resource(xrdb, RESNAME, RESCLAS, resource)))
		value = dflt;
	return (value);
}

Bool
getXrmInt(const char *val, int *integer)
{
	char *endptr = NULL;
	int value;

	value = strtol(val, &endptr, 0);
	if (endptr && !*endptr) {
		*integer = value;
		return True;
	}
	return False;
}

Bool
getXrmUint(const char *val, unsigned int *integer)
{
	char *endptr = NULL;
	unsigned int value;

	value = strtoul(val, &endptr, 0);
	if (endptr && !*endptr) {
		*integer = value;
		return True;
	}
	return False;
}

Bool
getXrmTime(const char *val, Time *time)
{
	char *endptr = NULL;
	unsigned int value;

	value = strtoul(val, &endptr, 0);
	if (endptr && !*endptr) {
		*time = value;
		return True;
	}
	return False;
}

Bool
getXrmBlanking(const char *val, unsigned int *integer)
{
	if (!strcasecmp(val, "DontPreferBlanking")) {
		*integer = DontPreferBlanking;
		return True;
	}
	if (!strcasecmp(val, "PreferBlanking")) {
		*integer = PreferBlanking;
		return True;
	}
	if (!strcasecmp(val, "DefaultBlanking")) {
		*integer = DefaultBlanking;
		return True;
	}
	return getXrmUint(val, integer);
}

Bool
getXrmExposures(const char *val, unsigned int *integer)
{
	if (!strcasecmp(val, "DontAllowExposures")) {
		*integer = DontAllowExposures;
		return True;
	}
	if (!strcasecmp(val, "AllowExposures")) {
		*integer = AllowExposures;
		return True;
	}
	if (!strcasecmp(val, "DefaultExposures")) {
		*integer = DefaultExposures;
		return True;
	}
	return getXrmUint(val, integer);
}

Bool
getXrmButton(const char *val, unsigned int *integer)
{
	if (!strcasecmp(val, "Button1")) {
		*integer = 1;
		return True;
	}
	if (!strcasecmp(val, "Button2")) {
		*integer = 2;
		return True;
	}
	if (!strcasecmp(val, "Button3")) {
		*integer = 3;
		return True;
	}
	if (!strcasecmp(val, "Button4")) {
		*integer = 4;
		return True;
	}
	if (!strcasecmp(val, "Button5")) {
		*integer = 5;
		return True;
	}
	return getXrmUint(val, integer);
}

#if 0
Bool
getXrmPowerLevel(const char *val, unsigned int *integer)
{
	if (!strcasecmp(val, "DPMSModeOn")) {
		*integer = DPMSModeOn;
		return True;
	}
	if (!strcasecmp(val, "DPMSModeStandby")) {
		*integer = DPMSModeStandby;
		return True;
	}
	if (!strcasecmp(val, "DPMSModeSuspend")) {
		*integer = DPMSModeSuspend;
		return True;
	}
	if (!strcasecmp(val, "DPMSModeOff")) {
		*integer = DPMSModeOff;
		return True;
	}
	return getXrmUint(val, integer);
}
#endif

Bool
getXrmDouble(const char *val, double *floating)
{
	const struct lconv *lc = localeconv();
	char radix, *copy = strdup(val);

	if ((radix = lc->decimal_point[0]) != '.' && strchr(copy, '.'))
		*strchr(copy, '.') = radix;

	*floating = strtod(copy, NULL);
	DPRINTF(1, "Got decimal value %s, translates to %f\n", val, *floating);
	free(copy);
	return True;
}

Bool
getXrmBool(const char *val, Bool *boolean)
{
	int len;

	if ((len = strlen(val))) {
		if (!strncasecmp(val, "true", len)) {
			*boolean = True;
			return True;
		}
		if (!strncasecmp(val, "false", len)) {
			*boolean = False;
			return True;
		}
	}
	EPRINTF("could not parse boolean'%s'\n", val);
	return False;
}

Bool
getXrmString(const char *val, char **string)
{
	char *tmp;

	if ((tmp = strdup(val))) {
		free(*string);
		*string = tmp;
		return True;
	}
	return False;
}

Bool
getXrmWhich(const char *val, UseScreen *which, int *screen)
{
	if (!strcasecmp(val, "default")) {
		*which = UseScreenDefault;
		return True;
	}
	if (!strcasecmp(val, "active")) {
		*which = UseScreenActive;
		return True;
	}
	if (!strcasecmp(val, "focused")) {
		*which = UseScreenFocused;
		return True;
	}
	if (!strcasecmp(val, "pointer")) {
		*which = UseScreenPointer;
		return True;
	}
	if (getXrmInt(val, screen)) {
		*which = UseScreenSpecified;
		return True;
	}
	return False;
}

Bool
getXrmXGeometry(const char *val, XdeGeometry *geom)
{
	int mask, x = 0, y = 0;
	unsigned int w = 0, h = 0;

	mask = XParseGeometry(val, &x, &y, &w, &h);
	if (!(mask & XValue) || !(mask & YValue))
		return False;
	geom->mask = mask;
	geom->x = x;
	geom->y = y;
	geom->w = w;
	geom->h = h;
	return True;
}

Bool
getXrmWhere(const char *val, MenuPosition *where, XdeGeometry *geom)
{
	if (!strcasecmp(val, "default")) {
		*where = PositionDefault;
		return True;
	}
	if (!strcasecmp(val, "center")) {
		*where = PositionCenter;
		return True;
	}
	if (!strcasecmp(val, "topleft")) {
		*where = PositionTopLeft;
		return True;
	}
	if (!strcasecmp(val, "bottomright")) {
		*where = PositionBottomRight;
		return True;
	}
	if (getXrmXGeometry(val, geom)) {
		*where = PositionSpecified;
		return True;
	}
	return False;
}

Bool
getXrmOrder(const char *val, WindowOrder *order)
{
	if (!strcasecmp(val, "default")) {
		*order = WindowOrderDefault;
		return True;
	}
	if (!strcasecmp(val, "client")) {
		*order = WindowOrderClient;
		return True;
	}
	if (!strcasecmp(val, "stacking")) {
		*order = WindowOrderStacking;
		return True;
	}
	return False;
}

void
get_resources(int argc, char *argv[])
{
	XrmDatabase rdb;
	Display *dpy;
	const char *val;
	char *usrdflt;

	PTRACE(5);
	XrmInitialize();
	if (getenv("DISPLAY")) {
		if (!(dpy = XOpenDisplay(NULL))) {
			EPRINTF("could not open display %s\n", getenv("DISPLAY"));
			exit(EXIT_FAILURE);
		}
		rdb = XrmGetDatabase(dpy);
		if (!rdb)
			DPRINTF(1, "no resource manager database allocated\n");
		XCloseDisplay(dpy);
	}
	usrdflt = g_strdup_printf(USRDFLT, getenv("HOME"));
	if (!XrmCombineFileDatabase(usrdflt, &rdb, False))
		DPRINTF(1, "could not open rcfile %s\n", usrdflt);
	g_free(usrdflt);
	if (!XrmCombineFileDatabase(APPDFLT, &rdb, False))
		DPRINTF(1, "could not open rcfile %s\n", APPDFLT);
	if (!rdb) {
		DPRINTF(1, "no resource manager database found\n");
		rdb = XrmGetStringDatabase("");
	}
	if ((val = get_resource(rdb, "debug", "0")))
		getXrmInt(val, &options.debug);
	if ((val = get_resource(rdb, "verbose", "1")))
		getXrmInt(val, &options.output);
	/* get a bunch of resources */
	if ((val = get_resource(rdb, "which", "default")))
		getXrmWhich(val, &options.which, &options.screen);
	if ((val = get_resource(rdb, "where", "default")))
		getXrmWhere(val, &options.where, &options.geom);
	if ((val = get_resource(rdb, "order", "default")))
		getXrmOrder(val, &options.order);
	if ((val = get_resource(rdb, "keys", NULL)))
		getXrmString(val, &options.keys);
	if ((val = get_resource(rdb, "cycle", "false")))
		getXrmBool(val, &options.cycle);
	if ((val = get_resource(rdb, "normal", "false")))
		getXrmBool(val, &options.normal);
	if ((val = get_resource(rdb, "minimized", "false")))
		getXrmBool(val, &options.minimized);
	if ((val = get_resource(rdb, "allMonitors", "false")))
		getXrmBool(val, &options.monitors);
	if ((val = get_resource(rdb, "allWorkspaces", "false")))
		getXrmBool(val, &options.workspaces);
	if ((val = get_resource(rdb, "activate", "true")))
		getXrmBool(val, &options.activate);
	if ((val = get_resource(rdb, "raise", "false")))
		getXrmBool(val, &options.raise);
	if ((val = get_resource(rdb, "restore", "false")))
		getXrmBool(val, &options.restore);

	XrmDestroyDatabase(rdb);
}

/** @} */

/** @} */

/** @section Event handlers
  * @{ */

static void
do_run(int argc, char *argv[])
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
#if 0
	/* mucks up dynamic cascading menus launched with pointer */
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, position_menu, scrn,
		       options.button, options.timestamp);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, position_menu, scrn,
		       0, options.timestamp);
#endif
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
		DPRINTF(1, "could not get _XDE_THEME_NAME for root 0x%lx\n", root);
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
client_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;

	PTRACE(5);
	switch (xev->type) {
	case ClientMessage:
		return event_handler_ClientMessage(dpy, xev);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;	/* event not handled, continue processing */
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

/** @} */

/** @section Initialization
  * @{ */

static void
startup(int argc, char *argv[])
{
	GdkAtom atom;
	GdkEventMask mask;
	GdkScreen *scrn;
	GdkWindow *root;
	char *file;
	int nscr;

	file = g_build_filename(g_get_home_dir(), ".gtkrc-2.0.xde", NULL);
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

	atom = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_XA_XDE_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_GTK_READ_RCFILES");
	_XA_GTK_READ_RCFILES = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);

	atom = gdk_atom_intern_static_string("_NET_CURRENT_DESKTOP");
	_XA_NET_CURRENT_DESKTOP = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKSPACE");
	_XA_WIN_WORKSPACE = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("WM_DESKTOP");
	_XA_WM_DESKTOP = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_DESKTOP_NAMES");
	_XA_NET_DESKTOP_NAMES = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_NUMBER_OF_DESKTOPS");
	_XA_NET_NUMBER_OF_DESKTOPS = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKSPACE_COUNT");
	_XA_WIN_WORKSPACE_COUNT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_WM_ICON_GEOMETRY");
	_XA_NET_WM_ICON_GEOMETRY = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_ACTIVE_WINDOW");
	_XA_NET_ACTIVE_WINDOW = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_CLIENT_LIST");
	_XA_NET_CLIENT_LIST = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_CLIENT_LIST_STACKING");
	_XA_NET_CLIENT_LIST_STACKING = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_FOCUS");
	_XA_WIN_FOCUS = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_CLIENT_LIST");
	_XA_WIN_CLIENT_LIST = gdk_x11_atom_to_xatom_for_display(disp, atom);

	scrn = gdk_display_get_default_screen(disp);
	root = gdk_screen_get_root_window(scrn);
	mask = gdk_window_get_events(root);
	mask |= GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK | GDK_SUBSTRUCTURE_MASK;
	gdk_window_set_events(root, mask);

	reparse(dpy, GDK_WINDOW_XID(root));

	wnck_set_client_type(WNCK_CLIENT_TYPE_PAGER);
}

/** @} */

/** @section Main
  * @{ */

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
    %1$s {-h|--help} [options]\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
", argv[0]);
}

static const char *
show_bool(Bool value)
{
	if (value)
		return ("true");
	return ("false");
}

static const char *
show_order(WindowOrder order)
{
	switch (order) {
	case WindowOrderDefault:
		return ("default");
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
		return ("default");
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
		return ("default");
	case PositionPointer:
		return ("pointer");
	case PositionCenter:
		return ("center");
	case PositionTopLeft:
		return ("topleft");
	case PositionBottomRight:
		return ("bottomright");
	case PositionSpecified:
		snprintf(position, sizeof(position), "%ux%u%c%d%c%d",
			 options.geom.w, options.geom.h,
			 (options.geom.mask & XNegative) ? '-' : '+', options.geom.x,
			 (options.geom.mask & YNegative) ? '-' : '+', options.geom.y);
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
        specify the screen number, SCREEN, to use [default: %5$s]\n\
    -b, --button BUTTON\n\
        specify the mouse button number, BUTTON, for popup [default: %6$d]\n\
    -T, --timestamp TIMESTAMP\n\
        use the time, TIMESTAMP, for button/keyboard event [default: %7$lu]\n\
    -w, --which {active|focused|pointer|SCREEN}\n\
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
        GEOMETRY   - postion on screen as X geometry\n\
    -O, --order {client|stacking}\n\
        specify the order of windows [default: %10$s]\n\
    -c, --cycle\n\
        show a window cycle list [default: %11$s]\n\
    --normal\n\
        list normal windows as well [default: %12$s]\n\
    --hidden\n\
        list hidden windows as well [default: %13$s]\n\
    --minimized\n\
        list minimized windows as well [default: %14$s]\n\
    --all-monitors\n\
        list windows on all monitors [deefault: %15$s]\n\
    --all-workspaces\n\
        list windows on all workspaces [default: %16$s]\n\
    -n, --noactivate\n\
        do not activate windows [default: %17$s]\n\
    -r, --raise\n\
        raise windows when selected/cycling [default: %18$s]\n\
    -R, --restore\n\
        restore previous windows when cycling [default: %19$s]\n\
    -k, --keys FORWARD:REVERSE\n\
        specify keys for cycling [default: %20$s]\n\
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
	, show_screen(options.screen)
	, options.button
	, options.timestamp
	, show_which(options.which)
	, show_where(options.where)
	, show_order(options.order)
	, show_bool(options.cycle)
	, show_bool(options.normal)
	, show_bool(options.hidden)
	, show_bool(options.minimized)
	, show_bool(options.monitors)
	, show_bool(options.workspaces)
	, show_bool(!options.activate)
	, show_bool(options.raise)
	, show_bool(options.restore)
	, options.keys ?: ""
);
	/* *INDENT-ON* */
}

static void
set_defaults(void)
{
	const char *env, *p;
	int n;

	if ((env = getenv("DISPLAY"))) {
		options.display = strdup(env);
		if (options.screen < 0 && (p = strrchr(options.display, '.'))
		    && (n = strspn(++p, "0123456789")) && *(p + n) == '\0')
			options.screen = atoi(p);
	}
	if ((p = getenv("XDE_DEBUG")))
		options.debug = atoi(p);
}

#if 0
static int
find_pointer_screen(void)
{
	return 0;		/* FIXME: write this */
}

static int
find_focus_screen(void)
{
	return 0;		/* FIXME: write this */
}
#endif

static void
get_defaults(void)
{
	const char *p;
	int n;

	if (options.command == CommandDefault)
		options.command = CommandRun;
	if (!options.display) {
		EPRINTF("No DISPLAY environment variable nor --display option\n");
		return;
	}
	if (options.screen < 0 && (p = strrchr(options.display, '.'))
	    && (n = strspn(++p, "0123456789")) && *(p + n) == '\0')
		options.screen = atoi(p);
	if (!options.hidden && !options.minimized)
		options.normal = True;
#if 0
	if (options.screen < 0 && (options.editor || options.trayicon)) {
		if (options.button)
			options.screen = find_pointer_screen();
		else
			options.screen = find_focus_screen();
	}
#endif
}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;

	setlocale(LC_ALL, "");

	set_defaults();

	saveArgc = argc;
	saveArgv = g_strdupv(argv);

	get_resources(argc, argv);

	while (1) {
		int c, val, len;
		char *endptr = NULL;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"display",		required_argument,	NULL,	'd'},
			{"screen",		required_argument,	NULL,	's'},
			{"monitor",		required_argument,	NULL,	'M'},

			{"popup",		no_argument,		NULL,	'p'},
			{"pointer",		no_argument,		NULL,	'P'},
			{"keyboard",		no_argument,		NULL,	'K'},
			{"button",		required_argument,	NULL,	'b'},
			{"which",		required_argument,	NULL,	'w'},
			{"where",		required_argument,	NULL,	'W'},
			{"order",		optional_argument,	NULL,	'O'},

			{"timestamp",		required_argument,	NULL,	'T'},
			{"key",			optional_argument,	NULL,	'k'},

			{"cycle",		no_argument,		NULL,	'c'},
			{"normal",		no_argument,		NULL,	'4'},
			{"hidden",		no_argument,		NULL,	'1'},
			{"minimized",		no_argument,		NULL,	'm'},
			{"all-monitors",	no_argument,		NULL,	'2'},
			{"all-workspaces",	no_argument,		NULL,	'3'},
			{"noactivate",		no_argument,		NULL,	'n'},
			{"raise",		no_argument,		NULL,	'r'},
			{"restore",		no_argument,		NULL,	'R'},

			{"dry-run",		no_argument,		NULL,	'N'},
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
			val = strtol(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			options.screen = val;
			break;
		case 'M':	/* -M, --monitor MONITOR */
			val = strtol(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			options.monitor = val;
			break;

		case 'p':       /* -p, --popup */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandRun;
			options.command = CommandRun;
			break;
		case 'K':	/* -K, --keyboard */
			options.keyboard = True;
			options.pointer = False;
			options.button = 0;
			break;
		case 'P':	/* -P, --pointer */
			options.pointer = True;
			options.keyboard = False;
			if (!options.button)
				options.button = 1;
			break;

		case 'b':       /* -b, --button BUTTON */
			val = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			if (val < 0 || val > 8)
				goto bad_option;
			if (val) {
				options.keyboard = False;
				options.pointer = True;
			} else {
				options.keyboard = True;
				options.pointer = False;
			}
			options.button = val;
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
				options.geom.mask = mask;
				options.geom.x = x;
				options.geom.y = y;
				options.geom.w = w;
				options.geom.h = h;
			}
			break;
		case 'O':	/* -O, --order ORDERTYPE */
			if (options.order != WindowOrderDefault)
				goto bad_option;
			if (!(len = strlen(optarg)))
				goto bad_option;
			if (!strncasecmp("client", optarg, len))
				options.order = WindowOrderClient;
			else if (!strncasecmp("stacking", optarg, len))
				options.order = WindowOrderStacking;
			else
				goto bad_option;
			break;

		case 'T':       /* -T, --timestamp TIMESTAMP */
			options.timestamp = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			break;
		case 'k':	/* -k, --key [KEY1:KEY2] */
			if (optarg) {
				free(options.keys);
				options.keys = strdup(optarg);
			}
			break;
		case 'c':	/* -c, --cycle */
			options.cycle = True;
			break;
		case '4':	/* --normal */
			options.normal = True;
			break;
		case '1':	/* --hidden */
			options.hidden = True;
			break;
		case 'm':	/* -m, --minimized */
			options.minimized = True;
			break;
		case '2':	/* --all-monitors */
			options.monitors = True;
			break;
		case '3':	/* --all-workspaces */
			options.workspaces = True;
			break;
		case 'n':	/* -n, --noactivate */
			options.activate = False;
			break;
		case 'r':	/* -r, --raise */
			options.raise = True;
			break;
		case 'R':	/* -R, --restore */
			options.restore = True;
			break;

		case 'N':	/* -N, --dry-run */
			options.dryrun = True;
			break;
		case 'D':       /* -D, --debug [LEVEL] */
			if (options.debug > 0)
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
			if (options.debug > 0)
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
				goto bad_command;
			if (command == CommandDefault)
				command = CommandVersion;
			options.command = CommandVersion;
			break;
		case 'C':       /* -C, --copying */
			if (options.command != CommandDefault)
				goto bad_command;
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
		      bad_command:
			fprintf(stderr, "%s: only one command option allowed\n", argv[0]);
			goto bad_usage;
		}
	}
	DPRINTF(1, "%s: option index = %d\n", argv[0], optind);
	DPRINTF(1, "%s: option count = %d\n", argv[0], argc);
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
	switch (command) {
	case CommandDefault:
	case CommandRun:
		DPRINTF(1, "%s: popping the menu\n", argv[0]);
		startup(argc, argv);
		do_run(argc, argv);
		break;
	case CommandHelp:
		DPRINTF(1, "%s: printing help message\n", argv[0]);
		help(argc, argv);
		break;
	case CommandVersion:
		DPRINTF(1, "%s: printing version message\n", argv[0]);
		version(argc, argv);
		break;
	case CommandCopying:
		DPRINTF(1, "%s: printing copying message\n", argv[0]);
		copying(argc, argv);
		break;
	default:
		usage(argc, argv);
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

/** @} */

// vim: set sw=8 tw=100 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS fo+=tcqlorn foldmarker=@{,@} foldmethod=marker:

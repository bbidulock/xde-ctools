/*****************************************************************************

 Copyright (c) 2008-2015  Monavacon Limited <http://www.monavacon.com/>
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
#include <gtk/gtk.h>
#include <cairo.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <pwd.h>

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

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
	PositionSpecified,		/* specified position (X geometry) */
} MenuPosition;

typedef enum {
	CommandDefault,
	CommandPopup,
	CommandHelp,
	CommandVersion,
	CommandCopying,
} Command;

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
	Command command;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.button = 0,
	.timestamp = CurrentTime,
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
	.command = CommandDefault,
};

typedef struct {
	char *label;
	char *place;
	char *cmd;
	char *icon;
	char *tooltip;
} Place;

static void
xde_list_free(gpointer data)
{
	Place *place = data;

	free(place->label);
	free(place->place);
	free(place->cmd);
	free(place->icon);
	free(place->tooltip);
	free(place);
}

static GList *
get_xbel_bookmarks(GList *list, const char *file)
{
	GBookmarkFile *bookmark;
	gchar **uris, **uri;
	GError *error = NULL;

	DPRINTF("getting bookmark file %s\n", file);
	if (!(bookmark = g_bookmark_file_new()))
		return (list);
	if (!g_bookmark_file_load_from_file(bookmark, file, &error)) {
		DPRINTF("could not load bookmark file %s: %s\n", file, error->message);
		g_error_free(error);
		g_bookmark_file_free(bookmark);
		return (list);
	}
	uris = g_bookmark_file_get_uris(bookmark, NULL);
	for (uri = uris; uri && *uri; uri++) {
		Place *place;
		GList *node;
		gchar *value;

		DPRINTF("processing uri %s\n", *uri);
		if (g_bookmark_file_get_is_private(bookmark, *uri, NULL)) {
			DPRINTF("uri is private: %s\n", *uri);
			continue;
		}

		for (node = list; node; node = node->next) {
			if (!(place = node->data) || !place->place || strcmp(place->place, *uri))
				continue;

			if ((value = g_bookmark_file_get_title(bookmark, *uri, NULL))) {
				g_free(place->label);
				place->label = value;
			}
			if ((value = g_bookmark_file_get_description(bookmark, *uri, NULL))) {
				g_free(place->tooltip);
				place->tooltip = value;
			}
			if (g_bookmark_file_get_icon(bookmark, *uri, &value, NULL, NULL)) {
				g_free(place->icon);
				place->icon = value;
			}
			break;
		}
		if (node)
			continue;
		DPRINTF("adding uri to places: %s\n", *uri);
		place = calloc(1, sizeof(*place));
		place->label = g_bookmark_file_get_title(bookmark, *uri, NULL);
		place->place = g_strdup(*uri);
		place->cmd = g_strdup_printf("xdg-open '%s'", *uri);
		g_bookmark_file_get_icon(bookmark, *uri, &place->icon, NULL, NULL);
		place->tooltip = g_bookmark_file_get_description(bookmark, *uri, NULL);
		DPRINTF("label   = %s\n", place->label);
		DPRINTF("place   = %s\n", place->place);
		DPRINTF("cmd     = %s\n", place->cmd);
		DPRINTF("icon    = %s\n", place->icon);
		DPRINTF("tooltip = %s\n", place->tooltip);
		list = g_list_append(list, place);
	}
	if (uris)
		g_strfreev(uris);
	g_bookmark_file_free(bookmark);
	return (list);
}

static GList *
get_simple_bookmarks(GList *list, const char *file)
{
	FILE *f;
	char *b;

	if (!(f = fopen(file, "r"))) {
		DPRINTF("%s: %s\n", NAME, strerror(errno));
		return (list);
	}
	b = calloc(BUFSIZ, sizeof(*b));
	while (fgets(b, BUFSIZ, f)) {
		Place *place;
		GList *node;
		char *p;

		if ((p = strchr(b, '\n')))
			*p = '\0';
		if (!(p = strchr(b, ' ')) || !p[1])
			continue;
		*p = '\0';
		p++;
		for (node = list; node; node = node->next) {
			if ((place = node->data) && place->place && !strcmp(place->place, b)) {
				g_free(place->label);
				place->label = g_strdup(p);
				break;
			}
		}
		if (node)
			continue;
		place = calloc(1, sizeof(*place));
		place->label = g_strdup(p);
		place->place = g_strdup(b);
		place->cmd = g_strdup_printf("xdg-open '%s'", b);
		place->icon = g_strdup("folder");
		place->tooltip = g_strdup(place->place);
		list = g_list_append(list, place);
	}
	free(b);
	fclose(f);
	return (list);
}

void
xde_entry_activated(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *menu;
	char *cmd, *exec;

	if ((cmd = user_data)) {
		pid_t pid;

		exec = g_strdup_printf("%s &", cmd);
		if ((pid = fork()) == -1) {
			/* error */
			EPRINTF("%s: %s\n", NAME, strerror(errno));
			exit(EXIT_FAILURE);
			return;
		} else if (pid == 0) {
			/* we are the child */
			execl("/bin/sh", "sh", "-c", exec, NULL);
			exit(EXIT_FAILURE);
			return;
		}
		g_free(exec);
		/* we are the parent */
	}
	if (!(menu = gtk_widget_get_parent(GTK_WIDGET(menuitem))) ||
	    !gtk_menu_get_tearoff_state(GTK_MENU(menu)))
		gtk_main_quit();
}

static GtkWidget *
popup_menu_new(WnckScreen *scrn)
{
	GtkWidget *menu, *item, *image;
	GtkIconTheme *itheme;
	GtkIconInfo *info;
	GdkPixbuf *pixbuf = NULL;
	gchar *file;
	const gchar *dir;
	GList *list = NULL, *node;
	Place *place;
	char *esc;

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), "Places");
#if 0
	item = gtk_tearoff_menu_item_new();
	gtk_widget_show(item);
	gtk_menu_append(menu, item);
#endif

	itheme = gtk_icon_theme_get_default();

	place = calloc(1, sizeof(*place));
	place->label = g_strdup("Home");
	esc = g_uri_escape_string(g_get_home_dir(), "/", FALSE);
	place->place = g_strdup_printf("file://%s", esc);
	g_free(esc);
	place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
	place->icon = g_strdup("user-home");
	place->tooltip = g_strdup(place->place);
	list = g_list_append(list, place);

	place = calloc(1, sizeof(*place));
	place->label = g_strdup("Root");
	place->place = g_strdup("file:///");
	place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
	place->icon = g_strdup("folder");
	place->tooltip = g_strdup(place->place);
	list = g_list_append(list, place);

	place = calloc(1, sizeof(*place));
	list = g_list_append(list, place);

	if ((dir = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP))) {
		place = calloc(1, sizeof(*place));
		place->label = g_strdup("Desktop");
		esc = g_uri_escape_string(dir, "/", FALSE);
		place->place = g_strdup_printf("file://%s", esc);
		g_free(esc);
		place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
		place->icon = g_strdup("user-desktop");
		place->tooltip = g_strdup(place->place);
		list = g_list_append(list, place);
	}
	if ((dir = g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD))) {
		place = calloc(1, sizeof(*place));
		place->label = g_strdup("Download");
		esc = g_uri_escape_string(dir, "/", FALSE);
		place->place = g_strdup_printf("file://%s", esc);
		g_free(esc);
		place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
		place->icon = g_strdup("folder-download");
		place->tooltip = g_strdup(place->place);
		list = g_list_append(list, place);
	}
	if ((dir = g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES))) {
		place = calloc(1, sizeof(*place));
		place->label = g_strdup("Templates");
		esc = g_uri_escape_string(dir, "/", FALSE);
		place->place = g_strdup_printf("file://%s", esc);
		g_free(esc);
		place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
		place->icon = g_strdup("folder-templates");
		place->tooltip = g_strdup(place->place);
		list = g_list_append(list, place);
	}
	if ((dir = g_get_user_special_dir(G_USER_DIRECTORY_PUBLIC_SHARE))) {
		place = calloc(1, sizeof(*place));
		place->label = g_strdup("Public Share");
		esc = g_uri_escape_string(dir, "/", FALSE);
		place->place = g_strdup_printf("file://%s", esc);
		g_free(esc);
		place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
		place->icon = g_strdup("folder-publicshare");
		place->tooltip = g_strdup(place->place);
		list = g_list_append(list, place);
	}
	if ((dir = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS))) {
		place = calloc(1, sizeof(*place));
		place->label = g_strdup("Documents");
		esc = g_uri_escape_string(dir, "/", FALSE);
		place->place = g_strdup_printf("file://%s", esc);
		g_free(esc);
		place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
		place->icon = g_strdup("folder-documents");
		place->tooltip = g_strdup(place->place);
		list = g_list_append(list, place);
	}
	if ((dir = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC))) {
		place = calloc(1, sizeof(*place));
		place->label = g_strdup("Music");
		esc = g_uri_escape_string(dir, "/", FALSE);
		place->place = g_strdup_printf("file://%s", esc);
		g_free(esc);
		place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
		place->icon = g_strdup("folder-music");
		place->tooltip = g_strdup(place->place);
		list = g_list_append(list, place);
	}
	if ((dir = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES))) {
		place = calloc(1, sizeof(*place));
		place->label = g_strdup("Pictures");
		esc = g_uri_escape_string(dir, "/", FALSE);
		place->place = g_strdup_printf("file://%s", esc);
		g_free(esc);
		place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
		place->icon = g_strdup("folder-pictures");
		place->tooltip = g_strdup(place->place);
		list = g_list_append(list, place);
	}
	if ((dir = g_get_user_special_dir(G_USER_DIRECTORY_VIDEOS))) {
		place = calloc(1, sizeof(*place));
		place->label = g_strdup("Videos");
		esc = g_uri_escape_string(dir, "/", FALSE);
		place->place = g_strdup_printf("file://%s", esc);
		g_free(esc);
		place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
		place->icon = g_strdup("folder-videos");
		place->tooltip = g_strdup(place->place);
		list = g_list_append(list, place);
	}
	place = calloc(1, sizeof(*place));
	list = g_list_append(list, place);

	place = calloc(1, sizeof(*place));
	place->label = g_strdup("Computer");
	place->place = g_strdup("computer:///");
	place->cmd = g_strdup_printf("pcmanfm '%s'", place->place);
	place->icon = g_strdup("computer");
	place->tooltip = g_strdup(place->place);
	list = g_list_append(list, place);

	place = calloc(1, sizeof(*place));
	place->label = g_strdup("Network");
	place->place = g_strdup("network:///");
	place->cmd = g_strdup_printf("pcmanfm '%s'", place->place);
	place->icon = g_strdup("network");
	place->tooltip = g_strdup(place->place);
	list = g_list_append(list, place);

	place = calloc(1, sizeof(*place));
	place->label = g_strdup("Trash");
	place->place = g_strdup("trash:///");
	place->cmd = g_strdup_printf("xdg-open '%s'", place->place);
	place->icon = g_strdup("user-trash");
	place->tooltip = g_strdup(place->place);
	list = g_list_append(list, place);

	place = calloc(1, sizeof(*place));
	list = g_list_append(list, place);

	file = g_strdup_printf("%s/gtk-3.0/bookmarks", g_get_user_config_dir());
	list = get_simple_bookmarks(list, file);
	g_free(file);

	file = g_strdup_printf("%s/spacefm/bookmarks", g_get_user_config_dir());
	list = get_simple_bookmarks(list, file);
	g_free(file);

	file = g_strdup_printf("%s/.gtk-bookmarks", g_get_home_dir());
	list = get_simple_bookmarks(list, file);
	g_free(file);

	file = g_strdup_printf("%s/user-places.xbel", g_get_user_data_dir());
	list = get_xbel_bookmarks(list, file);
	g_free(file);

	for (node = list; node; node = node->next) {
		place = node->data;

		if (!place->label && !place->place && !place->cmd && !place->icon) {
			item = gtk_separator_menu_item_new();
		} else {
			item = gtk_image_menu_item_new();
			if (place->label)
				gtk_menu_item_set_label(GTK_MENU_ITEM(item), place->label);
			if (place->icon) {
				if ((info = gtk_icon_theme_lookup_icon(itheme, place->icon, 16,
								       GTK_ICON_LOOKUP_FORCE_SIZE |
								       GTK_ICON_LOOKUP_GENERIC_FALLBACK))
				    && (pixbuf = gtk_icon_info_load_icon(info, NULL))
				    && (image = gtk_image_new_from_pixbuf(pixbuf)))
					gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
								      image);
				if (pixbuf) {
					g_object_unref(pixbuf);
					pixbuf = NULL;
				}
			}
			if (place->tooltip)
				gtk_widget_set_tooltip_text(item, place->tooltip);
			if (place->cmd)
				g_signal_connect(G_OBJECT(item), "activate",
						 G_CALLBACK(xde_entry_activated), g_strdup(place->cmd));
		}
		gtk_menu_append(menu, item);
		gtk_widget_show(item);
	}
	g_list_free_full(list, &xde_list_free);

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
	WnckWorkspace *wkspc;

	wkspc = wnck_screen_get_active_workspace(scrn);
	*x = wnck_workspace_get_viewport_x(wkspc);
	*y = wnck_workspace_get_viewport_y(wkspc);

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
	case PositionSpecified:
		position_specified(menu, scrn, x, y);
		break;
	}
}

static void
on_selection_done(GtkMenuShell *menushell, gpointer user_data)
{
	if (!gtk_menu_get_tearoff_state(GTK_MENU(menushell)))
		gtk_main_quit();
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
	g_signal_connect(G_OBJECT(menu), "selection-done", G_CALLBACK(on_selection_done), NULL);
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
Copyright (c) 2008-2015  Monavacon Limited <http://www.monavacon.com/>\n\
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
Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015  Monavacon Limited.\n\
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
    -W, --where {pointer|center|topleft|POSITION}\n\
        specify where to place the menu [default: %9$s]\n\
        \"pointer\"  - northwest corner under the pointer\n\
        \"center\"   - center of associated monitor\n\
        \"topleft\"  - northwest corner of work area\n\
        POSITION   - postion on screen as X geometry\n\
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

			{"debug",		optional_argument,	NULL,	'D'},
			{"verbose",		optional_argument,	NULL,	'v'},
			{"help",		no_argument,		NULL,	'h'},
			{"version",		no_argument,		NULL,	'V'},
			{"copying",		no_argument,		NULL,	'C'},
			{"?",			no_argument,		NULL,	'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "d:s:pb:T:w:W:D::v::hVCH?",
				long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:pb:T:w:W:D:vhVCH?");
#endif				/* _GNU_SOURCE */
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
				options.where = UseScreenPointer;
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

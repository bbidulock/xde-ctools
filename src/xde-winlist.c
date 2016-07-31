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

#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
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
#include <langinfo.h>
#include <stdarg.h>
#include <strings.h>
#include <regex.h>
#include <wordexp.h>
#include <execinfo.h>
#include <math.h>
#include <dlfcn.h>

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
#include <gio/gdesktopappinfo.h>
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

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1
#define EXIT_SYNTAXERR	2

#define GTK_EVENT_STOP		TRUE
#define GTK_EVENT_PROPAGATE	FALSE

const char *program = NAME;

#define XA_PREFIX		"_XDE_WINLIST"
#define XA_SELECTION_NAME	XA_PREFIX "_S%d"
#define XA_NET_DESKTOP_LAYOUT	"_NET_DESKTOP_LAYOUT_S%d"
#define LOGO_NAME		"metacity"

static int saveArgc;
static char **saveArgv;

#define RESNAME "xde-winlist"
#define RESCLAS "XDE-WinList"
#define RESTITL "XDG Window List Menu"

#define USRDFLT "%s/.config/" RESNAME "/rc"
#define APPDFLT "/usr/share/X11/app-defaults/" RESCLAS

/** @} */

/** @section Globals and Structures
  * @{ */

static Atom _XA_GTK_READ_RCFILES;

static Atom _XA_NET_ACTIVE_WINDOW;
static Atom _XA_NET_CLIENT_LIST;
static Atom _XA_NET_CLIENT_LIST_STACKING;
static Atom _XA_NET_CURRENT_DESKTOP;
static Atom _XA_NET_DESKTOP_LAYOUT;
static Atom _XA_NET_DESKTOP_NAMES;
static Atom _XA_NET_NUMBER_OF_DESKTOPS;

static Atom _XA_WIN_AREA;
static Atom _XA_WIN_AREA_COUNT;
static Atom _XA_WIN_CLIENT_LIST;
static Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
static Atom _XA_WIN_FOCUS;
static Atom _XA_WIN_WORKSPACE;
static Atom _XA_WIN_WORKSPACE_COUNT;

static Atom _XA_WM_DESKTOP;

static Atom _XA_ESETROOT_PMAP_ID;
static Atom _XA_XROOTPMAP_ID;

static Atom _XA_XDE_ICON_THEME_NAME;		/* XXX */
static Atom _XA_XDE_THEME_NAME;
static Atom _XA_XDE_WM_CLASS;
static Atom _XA_XDE_WM_CMDLINE;
static Atom _XA_XDE_WM_COMMAND;
static Atom _XA_XDE_WM_ETCDIR;
static Atom _XA_XDE_WM_HOST;
static Atom _XA_XDE_WM_HOSTNAME;
static Atom _XA_XDE_WM_ICCCM_SUPPORT;
static Atom _XA_XDE_WM_ICON;
static Atom _XA_XDE_WM_ICONTHEME;		/* XXX */
static Atom _XA_XDE_WM_INFO;
static Atom _XA_XDE_WM_MENU;
static Atom _XA_XDE_WM_NAME;
static Atom _XA_XDE_WM_NETWM_SUPPORT;
static Atom _XA_XDE_WM_PID;
static Atom _XA_XDE_WM_PRVDIR;
static Atom _XA_XDE_WM_RCFILE;
static Atom _XA_XDE_WM_REDIR_SUPPORT;
static Atom _XA_XDE_WM_STYLE;
static Atom _XA_XDE_WM_STYLENAME;
static Atom _XA_XDE_WM_SYSDIR;
static Atom _XA_XDE_WM_THEME;
static Atom _XA_XDE_WM_THEMEFILE;
static Atom _XA_XDE_WM_USRDIR;
static Atom _XA_XDE_WM_VERSION;

static Atom _XA_PREFIX_REFRESH;
static Atom _XA_PREFIX_RESTART;
static Atom _XA_PREFIX_EDITOR;

#ifdef STARTUP_NOTIFICATION
static Atom _XA_NET_STARTUP_INFO;
static Atom _XA_NET_STARTUP_INFO_BEGIN;
#endif				/* STARTUP_NOTIFICATION */

typedef enum {
	CommandDefault,
	CommandRun,
	CommandQuit,			/* ask running instance to quit */
	CommandHelp,			/* print usage info and exit */
	CommandVersion,			/* print version info and exit */
	CommandCopying,			/* print copying info and exit */
} Command;

typedef enum {
	StyleFullmenu,
	StyleAppmenu,
	StyleSubmenu,
	StyleEntries,
} Style;

typedef enum {
	XdeStyleSystem,
	XdeStyleUser,
	XdeStyleMixed,
} Which;

typedef enum {
	UseScreenDefault,		/* default screen by button */
	UseScreenActive,		/* screen with active window */
	UseScreenFocused,		/* screen with focused window */
	UseScreenPointer,		/* screen with pointer */
	UseScreenSpecified,		/* specified screen */
} UseScreen;

typedef enum {
	PositionDefault,		/* default position */
	PositionPointer,		/* position at pointer */
	PositionCenter,			/* center of monitor */
	PositionTopLeft,		/* top left of work area */
	PositionBottomRight,		/* bottom right of work area */
	PositionSpecified,		/* specified position (X geometry) */
} MenuPosition;

typedef enum {
	WindowOrderDefault,
	WindowOrderClient,
	WindowOrderStacking,
} WindowOrder;

typedef enum {
	SortByDefault,			/* default sorting */
	SortByRecent,			/* sort from most recent */
	SortByFavorite,			/* sort from most frequent */
} Sorting;

typedef enum {
	IncludeDefault,			/* default things */
	IncludeDocs,			/* documents (files) only */
	IncludeApps,			/* applications only */
	IncludeBoth,			/* both documents and applications */
} Include;

typedef enum {
	OrganizeDefault,
	OrganizeNone,
	OrganizeDate,
	OrganizeFreq,
	OrganizeGroup,
	OrganizeContent,
	OrganizeApp,
} Organize;

typedef enum {
	PopupPager,			/* desktop pager feedback */
	PopupTasks,			/* task list feedback */
	PopupCycle,			/* window cycling feedback */
	PopupSetBG,			/* workspace background feedback */
	PopupStart,			/* startup notification feedback */
	PopupLast,
} PopupType;

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
	char *filename;
	Bool replace;
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
	Bool tooltips;
	Bool dryrun;
	char *format;
	char *desktop;
	char *charset;
	char *language;
	char *locale;
	char *wmname;
	char *theme;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.monitor = 0,
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
	.filename = NULL,
	.replace = False,
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
	.tooltips = False,
	.dryrun = False,
};

Display *dpy = NULL;
GdkDisplay *disp = NULL;

struct XdeScreen;
typedef struct XdeScreen XdeScreen;
struct XdeMonitor;
typedef struct XdeMonitor XdeMonitor;

struct XdeMonitor {
	int index;			/* monitor number */
	XdeScreen *xscr;		/* screen */
	int current;			/* current desktop for this monitor */
	GdkRectangle geom;		/* geometry of the monitor */
	struct {
		GList *oldlist;		/* clients for this monitor (old gnome) */
		GList *clients;		/* clients for this monitor */
		GList *stacked;		/* clients for this monitor (stacked) */
		struct {
			GdkWindow *old;	/* active window (previous) */
			GdkWindow *now;	/* active window (current) */
		} active;
	};
};

struct XdeScreen {
	int index;			/* index */
	GdkScreen *scrn;		/* screen */
	GdkWindow *root;
	WnckScreen *wnck;
	gint nmon;			/* number of monitors */
	XdeMonitor *mons;		/* monitors for this screen */
	Bool mhaware;			/* multi-head aware NetWM */
	char *theme;			/* XDE theme name */
	char *itheme;			/* XDE icon theme name */
	Window selwin;			/* selection owner window */
	Atom atom;			/* selection atom for this screen */
	int width, height;		/* dimensions of screen */
	guint timer;			/* timer source of running timer */
	int rows;			/* number of rows in layout */
	int cols;			/* number of cols in layout */
	int desks;			/* number of desks in layout */
	int ndsk;			/* number of desktops */
	int current;			/* current desktop for this screen */
	char *wmname;			/* window manager name (adjusted) */
	Bool goodwm;			/* is the window manager usable? */
	struct {
		GList *oldlist;		/* clients for this screen (old gnome) */
		GList *clients;		/* clients for this screen */
		GList *stacked;		/* clients for this screen (stacked) */
		struct {
			GdkWindow *old;	/* active window (previous) */
			GdkWindow *now;	/* active window (current) */
		} active;
	};
};

XdeScreen *screens = NULL;		/* array of screens */

/** @} */

/** @section Menu Handling
  * @{ */

static void
selection_done(GtkMenuShell *menushell, gpointer user_data)
{
	OPRINTF(1, "Selection done: exiting\n");
	if (!gtk_menu_get_tearoff_state(GTK_MENU(menushell)))
		gtk_main_quit();
}

void
showdesk_activate(GtkMenuItem *item, gpointer user_data)
{
	OPRINTF(1, "Menu item [%s] activated\n", gtk_menu_item_get_label(GTK_MENU_ITEM(item)));
	wnck_screen_toggle_showing_desktop(user_data, !wnck_screen_get_showing_desktop(user_data));
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

/* FIXME: convert to consider monitors */
static GtkWidget *
popup_menu_new(XdeMonitor *xmon)
{
	WnckScreen *scrn = xmon->xscr->wnck;
	GtkWidget *menu, *sep, *icon, *item;
	GList *workspaces, *workspace;
	GList *windows, *window;
	WnckWorkspace *active;
	int anum, count;
	GSList *group = NULL;

	menu = gtk_menu_new();
	g_signal_connect(G_OBJECT(menu), "key_press_event", G_CALLBACK(window_menu_key_press), NULL);
	g_signal_connect(G_OBJECT(menu), "selection_done", G_CALLBACK(selection_done), NULL);
	workspaces = wnck_screen_get_workspaces(scrn);
	if (wnck_screen_get_showing_desktop(scrn)) {
		/* might want a different icon here */
		icon = gtk_image_new_from_icon_name("view-restore", GTK_ICON_SIZE_MENU);
		item = gtk_image_menu_item_new_with_label("Restore Desktop");
	} else {
		icon = gtk_image_new_from_icon_name("perlpanel-applet-showdesktop", GTK_ICON_SIZE_MENU);
		item = gtk_image_menu_item_new_with_label("Show Desktop");
	}
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
	gtk_menu_append(menu, item);
	gtk_widget_show(item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(showdesk_activate), scrn);

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
	for (workspace = workspaces; workspace; workspace = workspace->next) {
		int wnum, len;
		WnckWorkspace *work;
		const char *name;
		char *label, *wkname, *p;

		work = workspace->data;
		wnum = wnck_workspace_get_number(work);
		if (options.workspaces) {
			if ((name = wnck_workspace_get_name(work)))
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
			icon = gtk_image_new_from_icon_name("preferences-desktop-display", GTK_ICON_SIZE_MENU);
			item = gtk_image_menu_item_new_with_label(label);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), icon);
#if 0
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
#if 1
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
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(workspace_activate), work);
			sep = gtk_separator_menu_item_new();
			gtk_menu_append(menu, sep);
			gtk_widget_show(sep);
			sep = gtk_separator_menu_item_new();
			gtk_menu_append(menu, sep);
			gtk_widget_show(sep);
			g_free(label);
		} else if (wnum != anum)
			continue;

		for (window = windows, count = 0; window; window = window->next, count++) {
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
			if (state & WNCK_WINDOW_STATE_HIDDEN) {
				hidden = TRUE;
				if (!options.hidden)
					continue;
			} else if (state & WNCK_WINDOW_STATE_MINIMIZED) {
				iconic = TRUE;
				if (!options.minimized)
					continue;
			} else {
				if (!options.normal)
					continue;
			}
			if (!count) {
				sep = gtk_separator_menu_item_new();
				gtk_menu_append(menu, sep);
				gtk_widget_show(sep);
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
			gtk_menu_append(menu, witem);
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
		}

		if (workspace->next) {
			sep = gtk_separator_menu_item_new();
			gtk_menu_append(menu, sep);
			gtk_widget_show(sep);
		}
	}
	gtk_widget_show_all(menu);
	return (menu);
}

/** @} */

/** @section Finding Screens and Monitors
  * @{ */

/** @brief find the specified monitor
  *
  * Either specified with options.screen and options.monitor, or if the DISPLAY
  * environment variable specifies a screen, use that screen; otherwise, return
  * NULL.
  */
static XdeMonitor *
find_specific_monitor(void)
{
	XdeMonitor *xmon = NULL;
	int nscr = gdk_display_get_n_screens(disp);
	XdeScreen *xscr;

	if (0 <= options.screen && options.screen < nscr) {
		/* user specified a valid screen number */
		xscr = screens + options.screen;
		if (0 <= options.monitor && options.monitor < xscr->nmon)
			xmon = xscr->mons + options.monitor;
	}
	return (xmon);
}

/** @brief find the screen of window with the focus
  */
static XdeMonitor *
find_focus_monitor(void)
{
	XdeMonitor *xmon = NULL;
	XdeScreen *xscr;
	GdkScreen *scrn;
	GdkWindow *win;
	Window focus = None;
	int revert_to, m;

	XGetInputFocus(dpy, &focus, &revert_to);
	if (focus != PointerRoot && focus != None) {
		win = gdk_x11_window_foreign_new_for_display(disp, focus);
		if (win) {
			scrn = gdk_window_get_screen(win);
			xscr = screens + gdk_screen_get_number(scrn);
			m = gdk_screen_get_monitor_at_window(scrn, win);
			g_object_unref(win);
			xmon = xscr->mons + m;
		}
	}
	return (xmon);
}

static XdeMonitor *
find_pointer_monitor(void)
{
	XdeMonitor *xmon = NULL;
	XdeScreen *xscr = NULL;
	GdkScreen *scrn = NULL;
	int m, x = 0, y = 0;

	gdk_display_get_pointer(disp, &scrn, &x, &y, NULL);
	if (scrn) {
		xscr = screens + gdk_screen_get_number(scrn);
		m = gdk_screen_get_monitor_at_point(scrn, x, y);
		xmon = xscr->mons + m;
	}
	return (xmon);
}

static XdeMonitor *
find_monitor(void)
{
	XdeMonitor *xmon = NULL;

	if ((xmon = find_specific_monitor()))
		return (xmon);
	switch (options.which) {
	case UseScreenDefault:
		if (options.button) {
			if ((xmon = find_pointer_monitor()))
				return (xmon);
			if ((xmon = find_focus_monitor()))
				return (xmon);
		} else {
			if ((xmon = find_focus_monitor()))
				return (xmon);
			if ((xmon = find_pointer_monitor()))
				return (xmon);
		}
		break;
	case UseScreenActive:
		break;
	case UseScreenFocused:
		if ((xmon = find_focus_monitor()))
			return (xmon);
		break;
	case UseScreenPointer:
		if ((xmon = find_pointer_monitor()))
			return (xmon);
		break;
	case UseScreenSpecified:
		break;
	}

	if (!xmon)
		xmon = screens->mons;
	return (xmon);
}

/** @} */

/** @section Menu Positioning
  * @{ */

static gboolean
position_pointer(GtkMenu *menu, XdeMonitor *xmon, gint *x, gint *y)
{
	PTRACE(5);
	gdk_display_get_pointer(disp, NULL, x, y, NULL);
	return TRUE;
}

static gboolean
position_center_monitor(GtkMenu *menu, XdeMonitor *xmon, gint *x, gint *y)
{
	GtkRequisition req;

	PTRACE(5);
	gtk_widget_get_requisition(GTK_WIDGET(menu), &req);

	*x = xmon->geom.x + (xmon->geom.width - req.width) / 2;
	*y = xmon->geom.y + (xmon->geom.height - req.height) / 2;

	return TRUE;
}

static gboolean
position_topleft_workarea(GtkMenu *menu, XdeMonitor *xmon, gint *x, gint *y)
{
#if 1
	WnckWorkspace *wkspc;

	/* XXX: not sure this is what we want... */
	wkspc = wnck_screen_get_active_workspace(xmon->xscr->wnck);
	*x = wnck_workspace_get_viewport_x(wkspc);
	*y = wnck_workspace_get_viewport_y(wkspc);
#else
	GdkScreen *scr;
	GdkRectangle rect;
	gint px, py, nmon;

	PTRACE(5);
	gdk_display_get_pointer(disp, &scr, &px, &py, NULL);
	nmon = gdk_screen_get_monitor_at_point(scr, px, py);
	gdk_screen_get_monitor_geometry(scr, nmon, &rect);

	*x = rect.x;
	*y = rect.y;
#endif

	return TRUE;
}

static gboolean
position_bottomright_workarea(GtkMenu *menu, XdeMonitor *xmon, gint *x, gint *y)
{
#if 1
	WnckWorkspace *wkspc;
	GtkRequisition req;

	/* XXX: not sure this is what we want... */
	wkspc = wnck_screen_get_active_workspace(xmon->xscr->wnck);
	gtk_widget_get_requisition(GTK_WIDGET(menu), &req);
	*x = wnck_workspace_get_viewport_x(wkspc) + wnck_workspace_get_width(wkspc) - req.width;
	*y = wnck_workspace_get_viewport_y(wkspc) + wnck_workspace_get_height(wkspc) - req.height;
#else
	GdkScreen *scrn;
	GdkRectangle rect;
	gint px, py, nmon;
	GtkRequisition req;

	PTRACE(5);
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
position_specified(GtkMenu *menu, XdeMonitor *xmon, gint *x, gint *y)
{
	int x1, y1, sw, sh;

	sw = xmon->xscr->width;
	sh = xmon->xscr->height;
	DPRINTF(1, "screen width = %d\n", sw);
	DPRINTF(1, "screen height = %d\n", sh);

	x1 = (options.geom.mask & XNegative) ? sw - options.geom.x : options.geom.x;
	y1 = (options.geom.mask & YNegative) ? sh - options.geom.y : options.geom.y;

	DPRINTF(1, "geometry x1 = %d\n", x1);
	DPRINTF(1, "geometry y1 = %d\n", y1);
	DPRINTF(1, "geometry w = %d\n", options.geom.w);
	DPRINTF(1, "geometry h = %d\n", options.geom.h);

	if (!(options.geom.mask & (WidthValue | HeightValue))) {
		*x = x1;
		*y = y1;
	} else {
		GtkRequisition req;
		int x2, y2;

		gtk_widget_size_request(GTK_WIDGET(menu), &req);
		x2 = x1 + options.geom.w;
		y2 = y1 + options.geom.h;
		DPRINTF(1, "geometry x2 = %d\n", x2);
		DPRINTF(1, "geometry y2 = %d\n", y2);

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
	DPRINTF(1, "placing menu at +%d+%d\n", *x, *y);
	return TRUE;
}

void
position_menu(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	XdeMonitor *xmon = user_data;

	*push_in = FALSE;
	if (options.button) {
		position_pointer(menu, xmon, x, y);
		return;
	}
	switch (options.where) {
	case PositionDefault:
	default:
		position_center_monitor(menu, xmon, x, y);
		break;
	case PositionPointer:
		position_pointer(menu, xmon, x, y);
		break;
	case PositionCenter:
		position_center_monitor(menu, xmon, x, y);
		break;
	case PositionTopLeft:
		position_topleft_workarea(menu, xmon, x, y);
		break;
	case PositionBottomRight:
		position_bottomright_workarea(menu, xmon, x, y);
		break;
	case PositionSpecified:
		position_specified(menu, xmon, x, y);
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
get_resources(void)
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
	if (options.filename)
		if (!XrmCombineFileDatabase(options.filename, &rdb, False))
			DPRINTF(1, "could not open rcfile %s\n", options.filename);
	usrdflt = g_strdup_printf(USRDFLT, getenv("HOME"));
	if (!options.filename || strcmp(options.filename, usrdflt))
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

/** @section X Event Handlers
  * @{ */

static void
update_client_list(XdeScreen *xscr, Atom prop)
{
}

static void
update_screen_active_window(XdeScreen *xscr)
{
}

static void
update_monitor_active_window(XdeMonitor *xmon)
{
}

static void
update_active_window(XdeScreen *xscr, Atom prop)
{
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;
	int i, j = 0, *x;
	Window *active;
	GdkWindow **window;
	XdeMonitor *xmon;

	PTRACE(5);
	active = calloc(xscr->nmon + 1, sizeof(*active));
	window = calloc(xscr->nmon + 1, sizeof(*window));

	if (prop == None || prop == _XA_WIN_FOCUS) {
		if (XGetWindowProperty(dpy, root, _XA_WIN_FOCUS,
				       0, 64, False, XA_CARDINAL, &actual, &format,
				       &nitems, &after, (unsigned char **) &data) == Success &&
		    format == 32 && nitems >= 1 && data) {
			active[0] = data[0];
			if (nitems > 1 && nitems == xscr->nmon) {
				xscr->mhaware = True;
				x = &i;
			} else
				x = &j;
			for (i = 0; i < nitems; i++)
				active[i + 1] = data[*x];
			XFree(data);
		}
	}
	if (prop == None || prop == _XA_NET_ACTIVE_WINDOW) {
		if (XGetWindowProperty(dpy, root, _XA_NET_ACTIVE_WINDOW,
				       0, 64, False, XA_WINDOW, &actual, &format,
				       &nitems, &after, (unsigned char **) &data) == Success &&
		    format == 32 && nitems >= 1 && data) {
			active[0] = data[0];
			if (nitems > 1 && nitems == xscr->nmon) {
				xscr->mhaware = True;
				x = &i;
			} else
				x = &j;
			for (i = 0; i < nitems; i++)
				active[i + 1] = data[*x];
			XFree(data);
		}
	}
	window[0] = gdk_x11_window_foreign_new_for_display(disp, active[0]);
	if (xscr->active.now != window[0]) {
		if (xscr->active.old)
			g_object_unref(xscr->active.old);
		xscr->active.old = xscr->active.now;
		xscr->active.now = window[0];
		update_screen_active_window(xscr);
	}
	for (i = 0, xmon = xscr->mons; i < xscr->nmon; i++, xmon++) {
		if ((window[i + 1] = gdk_x11_window_foreign_new_for_display(disp, active[i + 1]))) {
			if ((i != gdk_screen_get_monitor_at_window(xscr->scrn, window[i + 1]))) {
				g_object_unref(G_OBJECT(window[i + 1]));
				window[i + 1] = NULL;
				continue;
			}
			if (xmon->active.old)
				g_object_unref(xscr->active.old);
			xscr->active.old = xscr->active.now;
			xscr->active.now = window[i + 1];
			update_monitor_active_window(xmon);
		}
	}
	free(active);
	free(window);
}

#if 1
static void
update_root_pixmap(XdeScreen *xscr, Atom prop)
{
}
#endif

static void
update_current_desktop(XdeScreen *xscr, Atom prop)
{
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0, i, j = 0, *x;
	unsigned long *data = NULL;
	XdeMonitor *xmon;
	unsigned long *current;

	PTRACE(5);
	current = calloc(xscr->nmon + 1, sizeof(*current));

	if (prop == None || prop == _XA_WM_DESKTOP) {
		if (XGetWindowProperty(dpy, root, _XA_WM_DESKTOP, 0, 64, False,
				       XA_CARDINAL, &actual, &format, &nitems, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && nitems >= 1 && data) {
			current[0] = data[0];
			x = (xscr->mhaware = (nitems >= xscr->nmon)) ? &i : &j;
			for (i = 0; i < xscr->nmon; i++)
				current[i + 1] = data[*x];
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (prop == None || prop == _XA_WIN_WORKSPACE) {
		if (XGetWindowProperty(dpy, root, _XA_WIN_WORKSPACE, 0, 64, False,
				       XA_CARDINAL, &actual, &format, &nitems, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && nitems >= 1 && data) {
			current[0] = data[0];
			x = (xscr->mhaware = (nitems >= xscr->nmon)) ? &i : &j;
			for (i = 0; i < xscr->nmon; i++)
				current[i + 1] = data[*x];
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (prop == None || prop == _XA_NET_CURRENT_DESKTOP) {
		if (XGetWindowProperty(dpy, root, _XA_NET_CURRENT_DESKTOP, 0, 64, False,
				       XA_CARDINAL, &actual, &format, &nitems, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && nitems >= 1 && data) {
			current[0] = data[0];
			x = (xscr->mhaware = (nitems >= xscr->nmon)) ? &i : &j;
			for (i = 0; i < xscr->nmon; i++)
				current[i + 1] = data[*x];
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (xscr->current != current[0]) {
		xscr->current = current[0];
		DPRINTF(1, "Current desktop for screen %d changed.\n", xscr->index);
	}
	for (i = 0, xmon = xscr->mons; i < xscr->nmon; i++, xmon++) {
		if (xmon->current != current[i + 1]) {
			xmon->current = current[i + 1];
			DPRINTF(1, "Current view for monitor %d chaged.\n", xmon->index);
		}
	}
	free(current);
}

static void
update_layout(XdeScreen *xscr, Atom prop)
{
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0, num, desks = xscr->desks;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;
	Bool propok = False, layout_changed = False, number_changed = False;

	PTRACE(5);
	if (prop == None || prop == _XA_NET_DESKTOP_LAYOUT) {
		if (XGetWindowProperty
		    (dpy, root, _XA_NET_DESKTOP_LAYOUT, 0, 4, False, AnyPropertyType, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 4 && data) {
			if (xscr->cols != (int) data[1] || xscr->rows != (int) data[2]) {
				xscr->cols = data[1];
				xscr->rows = data[2];
				layout_changed = True;
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
		propok = True;
	}
	if (prop == None || prop == _XA_WIN_WORKSPACE_COUNT) {
		if (XGetWindowProperty
		    (dpy, root, _XA_WIN_WORKSPACE_COUNT, 0, 1, False, XA_CARDINAL, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 1 && data) {
			if (xscr->desks != (int) data[0]) {
				xscr->desks = data[0];
				number_changed = True;
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
		propok = True;
	}
	if (prop == None || prop == _XA_NET_NUMBER_OF_DESKTOPS) {
		if (XGetWindowProperty
		    (dpy, root, _XA_NET_NUMBER_OF_DESKTOPS, 0, 1, False, XA_CARDINAL, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 1 && data) {
			if (xscr->desks != (int) data[0]) {
				xscr->desks = data[0];
				number_changed = True;
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
		propok = True;
	}
	if (!propok)
		EPRINTF("wrong property passed\n");

	if (number_changed) {
		if (xscr->desks <= 0)
			xscr->desks = 1;
		if (xscr->desks > 64)
			xscr->desks = 64;
		if (xscr->desks == desks)
			number_changed = False;
	}
	if (layout_changed || number_changed) {
		if (xscr->rows <= 0 && xscr->cols <= 0) {
			xscr->rows = 1;
			xscr->cols = 0;
		}
		if (xscr->cols > xscr->desks) {
			xscr->cols = xscr->desks;
			xscr->rows = 1;
		}
		if (xscr->rows > xscr->desks) {
			xscr->rows = xscr->desks;
			xscr->cols = 1;
		}
		if (xscr->cols == 0)
			for (num = xscr->desks; num > 0; xscr->cols++, num -= xscr->rows) ;
		if (xscr->rows == 0)
			for (num = xscr->desks; num > 0; xscr->rows++, num -= xscr->cols) ;

		// refresh_layout(xscr); /* XXX: should be deferred */
	}
}

static void
update_icon_theme(XdeScreen *xscr, Atom prop)
{
	Window root = RootWindow(dpy, xscr->index);
	XTextProperty xtp = { NULL, };
	Bool changed = False;
	GtkSettings *set;

	gtk_rc_reparse_all();
	if (!prop || prop == _XA_GTK_READ_RCFILES)
		prop = _XA_XDE_ICON_THEME_NAME;
	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				char *rc_string;

				rc_string = g_strdup_printf("gtk-icon-theme-name=\"%s\"", list[0]);
				gtk_rc_parse_string(rc_string);
				g_free(rc_string);
				if (!xscr->itheme || strcmp(xscr->itheme, list[0])) {
					free(xscr->itheme);
					xscr->itheme = strdup(list[0]);
					changed = True;
				}
			}
			if (list)
				XFreeStringList(list);
		} else
			DPRINTF(1, "could not get text list for property\n");
		if (xtp.value)
			XFree(xtp.value);
	} else
		DPRINTF(1, "could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
	if ((set = gtk_settings_get_for_screen(xscr->scrn))) {
		GValue theme_v = G_VALUE_INIT;
		const char *itheme;

		g_value_init(&theme_v, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(set), "gtk-icon-theme-name", &theme_v);
		itheme = g_value_get_string(&theme_v);
		if (itheme && (!xscr->itheme || strcmp(xscr->itheme, itheme))) {
			free(xscr->itheme);
			xscr->itheme = strdup(itheme);
			changed = True;
		}
		g_value_unset(&theme_v);
	}
	if (changed) {
		DPRINTF(1, "New icon theme is %s\n", xscr->itheme);
		/* FIXME: do something more about it. */
	}
}

static void
update_theme(XdeScreen *xscr, Atom prop)
{
	Window root = RootWindow(dpy, xscr->index);
	XTextProperty xtp = { NULL, };
	Bool changed = False;
	GtkSettings *set;

	PTRACE(5);
	gtk_rc_reparse_all();
	if (!prop || prop == _XA_GTK_READ_RCFILES)
		prop = _XA_XDE_THEME_NAME;
	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				char *rc_string;

				rc_string = g_strdup_printf("gtk-theme-name=\"%s\"", list[0]);
				gtk_rc_parse_string(rc_string);
				g_free(rc_string);
				if (!xscr->theme || strcmp(xscr->theme, list[0])) {
					free(xscr->theme);
					xscr->theme = strdup(list[0]);
					changed = True;
				}
			}
			if (list)
				XFreeStringList(list);
		} else
			DPRINTF(1, "could not get text list for property\n");
		if (xtp.value)
			XFree(xtp.value);
	} else
		DPRINTF(1, "could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
	if ((set = gtk_settings_get_for_screen(xscr->scrn))) {
		GValue theme_v = G_VALUE_INIT;
		const char *theme;

		g_value_init(&theme_v, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(set), "gtk-theme-name", &theme_v);
		theme = g_value_get_string(&theme_v);
		if (theme && (!xscr->theme || strcmp(xscr->theme, theme))) {
			free(xscr->theme);
			xscr->theme = strdup(theme);
			changed = True;
		}
		g_value_unset(&theme_v);
	}
	if (changed) {
		DPRINTF(1, "New theme is %s\n", xscr->theme);
		/* FIXME: do something more about it. */
#if 0
		if (options.show.setbg)
			read_theme(xscr);
#endif
	} else
		DPRINTF(1, "No change in current theme %s\n", xscr->theme);
}

static GdkFilterReturn
event_handler_ClientMessage(XEvent *xev)
{
	XdeScreen *xscr = NULL;
	int s, nscr = ScreenCount(dpy);
	Atom type;

	PTRACE(5);
	for (s = 0; s < nscr; s++)
		if (xev->xclient.window == RootWindow(dpy, s)) {
			xscr = screens + s;
			break;
		}
	if (!xscr) {
		EPRINTF("could not find screen for client message with window 0%08lx\n", xev->xclient.window);
		xscr = screens;
	}
	if (options.debug > 1) {
		fprintf(stderr, "==> ClientMessage: %p\n", xscr);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xclient.window);
		fprintf(stderr, "    --> message_type = %s\n", XGetAtomName(dpy, xev->xclient.message_type));
		fprintf(stderr, "    --> format = %d\n", xev->xclient.format);
		switch (xev->xclient.format) {
			int i;

		case 8:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 20; i++)
				fprintf(stderr, " %02x", (int) xev->xclient.data.b[i]);
			fprintf(stderr, "\n");
			break;
		case 16:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 10; i++)
				fprintf(stderr, " %04x", (int) xev->xclient.data.s[i]);
			fprintf(stderr, "\n");
			break;
		case 32:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 5; i++)
				fprintf(stderr, " %08lx", xev->xclient.data.l[i]);
			fprintf(stderr, "\n");
			break;
		}
		fprintf(stderr, "<== ClientMessage: %p\n", xscr);
	}
	type = xev->xclient.message_type;
	if (type == _XA_GTK_READ_RCFILES) {
		update_theme(xscr, type);
		update_icon_theme(xscr, type);
		return GDK_FILTER_REMOVE;	/* event handled */
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
}

static GdkFilterReturn
client_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;

	PTRACE(5);
	switch (xev->type) {
	case ClientMessage:
		return event_handler_ClientMessage(xev);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;	/* event not handled, continue processing */
}

static GdkFilterReturn
event_handler_PropertyNotify(XEvent *xev, XdeScreen *xscr)
{
	Atom atom;

	PTRACE(5);
	if (options.debug > 2) {
		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n",
			(xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
	}
	atom = xev->xproperty.atom;
	if (xev->xproperty.state == PropertyNewValue) {
		if (atom == _XA_XDE_THEME_NAME || atom == _XA_XDE_WM_THEME) {
			update_theme(xscr, atom);
			return GDK_FILTER_REMOVE;	/* event handled */
		} else if (atom == _XA_XDE_ICON_THEME_NAME || atom == _XA_XDE_WM_ICONTHEME) {
			update_icon_theme(xscr, atom);
			return GDK_FILTER_REMOVE;	/* event handled */
#if 1
		} else if (atom == _XA_NET_DESKTOP_LAYOUT) {
			update_layout(xscr, atom);
		} else if (atom == _XA_NET_NUMBER_OF_DESKTOPS) {
			update_layout(xscr, atom);
		} else if (atom == _XA_WIN_WORKSPACE_COUNT) {
			update_layout(xscr, atom);
#endif
#if 1
		} else if (atom == _XA_NET_CURRENT_DESKTOP) {
			update_current_desktop(xscr, atom);
		} else if (atom == _XA_WIN_WORKSPACE) {
			update_current_desktop(xscr, atom);
		} else if (atom == _XA_WM_DESKTOP) {
			update_current_desktop(xscr, atom);
#endif
#if 1
		} else if (atom == _XA_XROOTPMAP_ID) {
			update_root_pixmap(xscr, atom);
		} else if (atom == _XA_ESETROOT_PMAP_ID) {
			update_root_pixmap(xscr, atom);
#endif
		} else if (atom == _XA_NET_ACTIVE_WINDOW) {
			update_active_window(xscr, atom);
#if 1
		} else if (atom == _XA_NET_CLIENT_LIST) {
			update_client_list(xscr, atom);
		} else if (atom == _XA_NET_CLIENT_LIST_STACKING) {
			update_client_list(xscr, atom);
#endif
		} else if (atom == _XA_WIN_FOCUS) {
			update_active_window(xscr, atom);
#if 1
		} else if (atom == _XA_WIN_CLIENT_LIST) {
			update_client_list(xscr, atom);
#endif
		}
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
}

static GdkFilterReturn
root_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = data;

	PTRACE(5);
	switch (xev->type) {
	case PropertyNotify:
		return event_handler_PropertyNotify(xev, xscr);
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
events_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	return GDK_FILTER_CONTINUE;
}

int
handler(Display *display, XErrorEvent *xev)
{
	if (options.debug) {
		char msg[80], req[80], num[80], def[80];

		snprintf(num, sizeof(num), "%d", xev->request_code);
		snprintf(def, sizeof(def), "[request_code=%d]", xev->request_code);
		XGetErrorDatabaseText(dpy, NAME, num, def, req, sizeof(req));
		if (XGetErrorText(dpy, xev->error_code, msg, sizeof(msg)) != Success)
			msg[0] = '\0';
		fprintf(stderr, "X error %s(0x%08lx): %s\n", req, xev->resourceid, msg);
		dumpstack(__FILE__, __LINE__, __func__);
	}
	return (0);
}

int
iohandler(Display *display)
{
	dumpstack(__FILE__, __LINE__, __func__);
	exit(EXIT_FAILURE);
}

int (*oldhandler) (Display *, XErrorEvent *) = NULL;
int (*oldiohandler) (Display *) = NULL;

gboolean
hup_signal_handler(gpointer data)
{
	/* perform reload */
	return G_SOURCE_CONTINUE;
}

gboolean
int_signal_handler(gpointer data)
{
	exit(EXIT_SUCCESS);
	return G_SOURCE_CONTINUE;
}

gboolean
term_signal_handler(gpointer data)
{
	gtk_main_quit();
	return G_SOURCE_CONTINUE;
}

/** @} */

/** @section Initialization
  * @{ */

static void
init_monitors(XdeScreen *xscr)
{
	XdeMonitor *xmon;
	int m;

	xscr->nmon = gdk_screen_get_n_monitors(xscr->scrn);
	xscr->mons = calloc(xscr->nmon, sizeof(*xscr->mons));
	for (m = 0, xmon = xscr->mons; m < xscr->nmon; m++, xmon++) {
		xmon->index = m;
		xmon->xscr = xscr;
		gdk_screen_get_monitor_geometry(xscr->scrn, m, &xmon->geom);
	}
}

static void
init_wnck(XdeScreen *xscr)
{
	WnckScreen *wnck = xscr->wnck = wnck_screen_get(xscr->index);

	wnck_screen_force_update(wnck);
}

/*
 *  This startup function starts up the X11 protocol connection and initializes GTK+.  Note that the
 *  program can still be run from a console, in which case the "DISPLAY" environment variables should
 *  not be defined: in which case, we will not start up X11 at all.
 */
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

#if 0
	/* We can start session management without a display; however, we then need to
	   run a GLIB event loop instead of a GTK event loop.  */
	init_smclient();

	/* do not start up X11 connection unless DISPLAY is defined */
	if (!options.display) {
		loop = g_main_loop_new(NULL, FALSE);
		return;
	}
#endif

	gtk_init(&argc, &argv);

	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);
	dpy = GDK_DISPLAY_XDISPLAY(disp);

	if (options.screen >= 0 && options.screen >= nscr) {
		EPRINTF("bad screen specified: %d\n", options.screen);
		exit(EXIT_FAILURE);
	}


	atom = gdk_atom_intern_static_string("_XDE_ICON_THEME_NAME");
	_XA_XDE_ICON_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_XA_XDE_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_CLASS");
	_XA_XDE_WM_CLASS = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_CMDLINE");
	_XA_XDE_WM_CMDLINE = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_COMMAND");
	_XA_XDE_WM_COMMAND = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_ETCDIR");
	_XA_XDE_WM_ETCDIR = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_HOST");
	_XA_XDE_WM_HOST = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_HOSTNAME");
	_XA_XDE_WM_HOSTNAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_ICCCM_SUPPORT");
	_XA_XDE_WM_ICCCM_SUPPORT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_ICON");
	_XA_XDE_WM_ICON = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_ICONTHEME");
	_XA_XDE_WM_ICONTHEME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_INFO");
	_XA_XDE_WM_INFO = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_MENU");
	_XA_XDE_WM_MENU = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_NAME");
	_XA_XDE_WM_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_NETWM_SUPPORT");
	_XA_XDE_WM_NETWM_SUPPORT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_PID");
	_XA_XDE_WM_PID = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_PRVDIR");
	_XA_XDE_WM_PRVDIR = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_RCFILE");
	_XA_XDE_WM_RCFILE = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_REDIR_SUPPORT");
	_XA_XDE_WM_REDIR_SUPPORT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_STYLE");
	_XA_XDE_WM_STYLE = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_STYLENAME");
	_XA_XDE_WM_STYLENAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_SYSDIR");
	_XA_XDE_WM_SYSDIR = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_THEME");
	_XA_XDE_WM_THEME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_THEMEFILE");
	_XA_XDE_WM_THEMEFILE = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_USRDIR");
	_XA_XDE_WM_USRDIR = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_VERSION");
	_XA_XDE_WM_VERSION = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_GTK_READ_RCFILES");
	_XA_GTK_READ_RCFILES = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, NULL);
#if 1
	atom = gdk_atom_intern_static_string("_NET_DESKTOP_LAYOUT");
	_XA_NET_DESKTOP_LAYOUT = gdk_x11_atom_to_xatom_for_display(disp, atom);

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

	atom = gdk_atom_intern_static_string("_WIN_DESKTOP_BUTTON_PROXY");
	_XA_WIN_DESKTOP_BUTTON_PROXY = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKSPACE_COUNT");
	_XA_WIN_WORKSPACE_COUNT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XROOTPMAP_ID");
	_XA_XROOTPMAP_ID = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("ESETROOT_PMAP_ID");
	_XA_ESETROOT_PMAP_ID = gdk_x11_atom_to_xatom_for_display(disp, atom);

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
#endif
#ifdef STARTUP_NOTIFICATION
	atom = gdk_atom_intern_static_string("_NET_STARTUP_INFO");
	_XA_NET_STARTUP_INFO = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, NULL);

	atom = gdk_atom_intern_static_string("_NET_STARTUP_INFO_BEGIN");
	_XA_NET_STARTUP_INFO_BEGIN = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, NULL);
#endif				/* STARTUP_NOTIFICATION */
	atom = gdk_atom_intern_static_string("_WIN_AREA");
	_XA_WIN_AREA = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_AREA_COUNT");
	_XA_WIN_AREA_COUNT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string(XA_PREFIX "_REFRESH");
	_XA_PREFIX_REFRESH = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, NULL);

	atom = gdk_atom_intern_static_string(XA_PREFIX "_RESTART");
	_XA_PREFIX_RESTART = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, NULL);

	atom = gdk_atom_intern_static_string(XA_PREFIX "_EDITOR");
	_XA_PREFIX_EDITOR = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, NULL);

	scrn = gdk_display_get_default_screen(disp);
	root = gdk_screen_get_root_window(scrn);
	mask = gdk_window_get_events(root);
	mask |= GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK | GDK_SUBSTRUCTURE_MASK;
	gdk_window_set_events(root, mask);

	wnck_set_client_type(WNCK_CLIENT_TYPE_PAGER);
}

XdeMonitor *
init_screens(void)
{
	XdeMonitor *xmon = NULL;
	XdeScreen *xscr;
	int s, nscr;

	nscr = gdk_display_get_n_screens(disp);
	screens = calloc(nscr, sizeof(*screens));

	gdk_window_add_filter(NULL, events_handler, NULL);

	for (s = 0, xscr = screens; s < nscr; s++, xscr++) {
		xscr->index = s;
		xscr->scrn = gdk_display_get_screen(disp, s);
		xscr->root = gdk_screen_get_root_window(xscr->scrn);
		xscr->width = gdk_screen_get_width(xscr->scrn);
		xscr->height = gdk_screen_get_height(xscr->scrn);
		gdk_window_add_filter(xscr->root, root_handler, xscr);
		init_wnck(xscr);
		init_monitors(xscr);
#if 1
		update_layout(xscr, None);
		update_current_desktop(xscr, None);
#endif
		update_theme(xscr, None);
		update_icon_theme(xscr, None);
#if 1
		update_active_window(xscr, None);
		update_client_list(xscr, None);
#endif
	}
	xmon = find_monitor();
	return (xmon);
}

static void
do_run(int argc, char *argv[], Bool replace)
{
	XdeMonitor *xmon;
	GtkWidget *menu;

	oldhandler = XSetErrorHandler(handler);
	oldiohandler = XSetIOErrorHandler(iohandler);

	xmon = init_screens();

	g_unix_signal_add(SIGTERM, &term_signal_handler, NULL);
	g_unix_signal_add(SIGINT, &int_signal_handler, NULL);
	g_unix_signal_add(SIGHUP, &hup_signal_handler, NULL);

	if (!(menu = popup_menu_new(xmon))) {
		EPRINTF("cannot get menu\n");
		exit(EXIT_FAILURE);
	}
	g_signal_connect(G_OBJECT(menu), "selection-done", G_CALLBACK(selection_done), NULL);
#if 0
	/* mucks up dynamic cascading menus launched with pointer */
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, position_menu, xmon,
		       options.button, options.timestamp);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, position_menu, xmon,
		       0, options.timestamp);
#endif
	gtk_main();
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

#if 1
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
#endif

static const char *
show_screen(int snum)
{
	static char screen[64] = { 0, };

	if (snum == -1)
		return ("None");
	snprintf(screen, sizeof(screen), "%d", snum);
	return (screen);
}

const char *
show_style(Style style)
{
	switch (style) {
	case StyleFullmenu:
		return ("fullmenu");
	case StyleAppmenu:
		return ("appmenu");
	case StyleEntries:
		return ("entries");
	case StyleSubmenu:
		return ("submenu");
	}
	return ("(unknown)");
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
    -t, --tooltips\n\
        provide detailed tooltips on menu items [default: %10$s]\n\
    -O, --order {client|stacking}\n\
        specify the order of windows [default: %11$s]\n\
    -c, --cycle\n\
        show a window cycle list [default: %12$s]\n\
    --normal\n\
        list normal windows as well [default: %13$s]\n\
    --hidden\n\
        list hidden windows as well [default: %14$s]\n\
    --minimized\n\
        list minimized windows as well [default: %15$s]\n\
    --all-monitors\n\
        list windows on all monitors [deefault: %16$s]\n\
    --all-workspaces\n\
        list windows on all workspaces [default: %17$s]\n\
    -n, --noactivate\n\
        do not activate windows [default: %18$s]\n\
    -r, --raise\n\
        raise windows when selected/cycling [default: %19$s]\n\
    -R, --restore\n\
        restore previous windows when cycling [default: %20$s]\n\
    -k, --keys FORWARD:REVERSE\n\
        specify keys for cycling [default: %21$s]\n\
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
	, show_bool(options.tooltips)
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

/*
 * Set options in the "options" structure.  The defaults are determined by preset defaults,
 * environment variables and other startup information, but not information from the X Server.  All
 * options are set in this way, only the ones that depend on environment variables or other startup
 * information.
 */
static void
set_defaults(void)
{
	const char *env, *p, *q;
	char *file, *endptr = NULL;
	int n, monitor;
	Time timestamp;

	if ((env = getenv("DISPLAY"))) {
		free(options.display);
		options.display = strdup(env);
		if (options.screen < 0 && (p = strrchr(options.display, '.'))
		    && (n = strspn(++p, "0123456789")) && *(p + n) == '\0')
			options.screen = atoi(p);
	}
	if ((env = getenv("XDG_CURRENT_DESKTOP"))) {
		free(options.desktop);
		options.desktop = strdup(env);
	}
	if ((env = getenv("DESKTOP_STARTUP_ID"))) {
		/* we can get the timestamp from the startup id */
		if ((p = strstr(env, "_TIME"))) {
			timestamp = strtoul(p + 5, &endptr, 10);
			if (endptr && *endptr)
				options.timestamp = timestamp;
		}
		/* we can get the monitor number from the startup id */
		if ((p = strstr(env, "xdg-launch/")) == env &&
		    (p = strchr(p + 11, '/')) && (p = strchr(p + 1, '-')) && (q = strchr(p + 1, '-'))) {
			monitor = strtoul(p, &endptr, 10);
			if (endptr == q)
				options.monitor = monitor;
		}
	}
	if ((env = getenv("XDE_DEBUG")))
		options.debug = atoi(env);
	file = g_build_filename(g_get_home_dir(), ".config", RESNAME, "rc", NULL);
	free(options.filename);
	options.filename = strdup(file);
	g_free(file);
}

static void
get_default_locale(void)
{
	char *val;
	int len, set;

	set = (options.charset || options.language);

	if (!options.charset && (val = nl_langinfo(CODESET)))
		options.charset = strdup(val);
	if (!options.language && options.locale) {
		options.language = strdup(options.locale);
		*strchrnul(options.language, '.') = '\0';
	}
	if (set && options.language && options.charset) {
		len = strlen(options.language) + 1 + strlen(options.charset);
		val = calloc(len, sizeof(*val));
		strcpy(val, options.language);
		strcat(val, ".");
		strcat(val, options.charset);
		DPRINTF(1, "setting locale to: '%s'\n", val);
		if (!setlocale(LC_ALL, val))
			EPRINTF("cannot set locale to '%s'\n", val);
		free(val);
	}
	DPRINTF(1, "locale is '%s'\n", options.locale);
	DPRINTF(1, "charset is '%s'\n", options.charset);
	DPRINTF(1, "language is '%s'\n", options.language);
}

static Bool
get_text_property(Window root, Atom prop, char ***listp, int *stringsp)
{
	XTextProperty xtp = { NULL, };

	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		*listp = NULL;
		*stringsp = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, listp, stringsp) == Success)
			return True;
		else
			DPRINTF(1, "could not get text list for %s property\n", XGetAtomName(dpy, prop));
	} else
		DPRINTF(1, "could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
	return False;
}

static void
get_default_wmname(void)
{
	if (options.wmname) {
		DPRINTF(1, "option wmname is set to '%s'\n", options.wmname);
		return;
	}

	if (options.format) {
		char *p;

		free(options.wmname);
		options.wmname = strdup(options.format);
		if ((p = strstr(options.wmname, "old")) && !p[3])
			p[0] = '\0';
		for (p = options.wmname; *p; p++)
			*p = tolower(*p);
	}
	if (options.display) {
		Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
		GdkScreen *scrn = gdk_display_get_default_screen(disp);
		GdkWindow *wind = gdk_screen_get_root_window(scrn);
		Window root = GDK_WINDOW_XID(wind);
		Atom prop = _XA_XDE_WM_NAME;
		char **list = NULL;
		int strings = 0;

		if (get_text_property(root, prop, &list, &strings)) {
			if (!options.wmname) {
				free(options.wmname);
				options.wmname = strdup(list[0]);
			} else if (strcmp(options.wmname, list[0]))
				DPRINTF(1, "default wmname %s different from actual %s\n",
					options.wmname, list[0]);
			if (list)
				XFreeStringList(list);
		} else
			DPRINTF(1, "could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
	} else
		EPRINTF("cannot determine wmname without DISPLAY\n");

	if (options.wmname)
		DPRINTF(1, "assigned wmname as '%s'\n", options.wmname);
}

static void
get_default_format(void)
{
	if (options.format) {
		DPRINTF(1, "option format is set to '%s'\n", options.format);
		return;
	}

	if (options.wmname) {
		char *p;

		free(options.format);
		options.format = strdup(options.wmname);
		for (p = options.format; *p; p++)
			*p = tolower(*p);
	}
	if (options.display) {
		GdkScreen *scrn = gdk_display_get_default_screen(disp);
		GdkWindow *wind = gdk_screen_get_root_window(scrn);
		Window root = GDK_WINDOW_XID(wind);
		Atom prop = _XA_XDE_WM_NAME;
		char **list = NULL;
		int strings = 0;

		if (get_text_property(root, prop, &list, &strings)) {
			if (!options.format) {
				free(options.format);
				options.format = strdup(list[0]);
			} else if (strcmp(options.format, list[0]))
				DPRINTF(1, "default format %s different from actual %s\n",
					options.format, list[0]);
			if (list)
				XFreeStringList(list);
		} else
			DPRINTF(1, "could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
	} else
		EPRINTF("cannot determine format without DISPLAY\n");

	if (options.format)
		DPRINTF(1, "assigned format as '%s'\n", options.format);
}

static void
get_default_desktop(void)
{
	XdeScreen *xscr = screens;
	const char *env;
	char *p;

	if (!options.desktop || !strcmp(options.desktop, "XDE") || !options.wmname
	    || strcasecmp(options.wmname, options.desktop)) {
		if ((env = getenv("XDG_CURRENT_DESKTOP"))) {
			free(options.desktop);
			options.desktop = strdup(env);
		} else if (options.format) {
			free(options.desktop);
			options.desktop = strdup(options.format);
			for (p = options.desktop; *p; p++)
				*p = toupper(*p);
		} else if (xscr && xscr->wmname) {
			free(options.desktop);
			options.desktop = strdup(xscr->wmname);
			for (p = options.desktop; *p; p++)
				*p = toupper(*p);
		} else if (!options.desktop) {
			options.desktop = strdup("XDE");
		}
	}
	if (options.desktop)
		DPRINTF(1, "assigned desktop as '%s'\n", options.desktop);
}

static void
get_default_theme(void)
{
#if 1
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	GdkWindow *wind = gdk_screen_get_root_window(scrn);
	Window root = GDK_WINDOW_XID(wind);
	XTextProperty xtp = { NULL, };
	Atom prop = _XA_XDE_ICON_THEME_NAME;
	GtkSettings *set;

	gtk_rc_reparse_all();

	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				static const char *prefix = "gtk-icon-theme-name=\"";
				static const char *suffix = "\"";
				char *rc_string;
				int len;

				len = strlen(prefix) + strlen(list[0]) + strlen(suffix);
				rc_string = calloc(len + 1, sizeof(*rc_string));
				strncpy(rc_string, prefix, len);
				strncat(rc_string, list[0], len);
				strncat(rc_string, suffix, len);
				gtk_rc_parse_string(rc_string);
				free(rc_string);
				if (!options.theme || strcmp(options.theme, list[0])) {
					free(options.theme);
					options.theme = strdup(list[0]);
				}
			}
			if (list)
				XFreeStringList(list);
		} else
			EPRINTF("could not get text list for %s property\n", XGetAtomName(dpy, prop));
		if (xtp.value)
			XFree(xtp.value);
	} else
		DPRINTF(1, "could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
	if ((set = gtk_settings_get_for_screen(scrn))) {
		GValue theme_v = G_VALUE_INIT;
		const char *itheme;

		g_value_init(&theme_v, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(set), "gtk-icon-theme-name", &theme_v);
		itheme = g_value_get_string(&theme_v);
		if (itheme && (!options.theme || strcmp(options.theme, itheme))) {
			free(options.theme);
			options.theme = strdup(itheme);
		}
		g_value_unset(&theme_v);
	}
#else

	static const char *suffix1 = "/etc/gtk-2.0/gtkrc";
	static const char *suffix2 = "/.gtkrc-2.0";
	static const char *suffix3 = "/.gtkrc-2.0.xde";
	static const char *prefix = "gtk-icon-theme-name=\"";
	char *env;
	int len;

	if (options.theme)
		return;
	if ((env = getenv("XDG_ICON_THEME"))) {
		free(options.theme);
		options.theme = strdup(env);
		return;
	} else {
		char *path, *d, *e, *buf;

		/* need to go looking for the theme name */

		if ((env = getenv("GTK_RC_FILES"))) {
			path = strdup(env);
		} else {
			env = getenv("HOME") ? : "~";
			len = 1 + strlen(suffix1) + 1 + strlen(env) + strlen(suffix2) +
			    1 + strlen(env) + strlen(suffix3);
			path = calloc(len, sizeof(*path));
			strcpy(path, suffix1);
			strcat(path, ":");
			strcat(path, env);
			strcat(path, suffix2);
			strcat(path, ":");
			strcat(path, env);
			strcat(path, suffix3);
		}
		buf = calloc(BUFSIZ, sizeof(*buf));
		d = path;
		e = d + strlen(d);
		while ((d = strchrnul(d, ':')) < e)
			*d++ = '\0';
		for (d = path; d < e; d += strlen(d) + 1) {
			FILE *f;

			if (!(f = fopen(d, "r")))
				continue;
			while (fgets(buf, BUFSIZ, f)) {
				if (!strncmp(buf, prefix, strlen(prefix))) {
					char *p;

					p = buf + strlen(prefix);
					*strchrnul(p, '"') = '\0';
					free(options.theme);
					options.theme = strdup(p);
				}
				/* FIXME: this does not traverse "include" statements */
			}
			fclose(f);
		}
		free(buf);
	}
	if (!options.theme)
		options.theme = strdup("hicolor");
#endif
}

static void
get_defaults(void)
{
	const char *p;
	int n;

	if (options.command == CommandDefault)
		options.command = CommandRun;
	if (options.display) {
		if (options.screen < 0 && (p = strrchr(options.display, '.'))
		    && (n = strspn(++p, "0123456789")) && *(p + n) == '\0')
			options.screen = atoi(p);
	}
	if (!options.hidden && !options.minimized)
		options.normal = True;
	get_default_locale();
	get_default_wmname();
	get_default_format();
	get_default_desktop();
	get_default_theme();
}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;
	char *loc;

	saveArgc = argc;
	saveArgv = g_strdupv(argv);

	if ((loc = setlocale(LC_ALL, ""))) {
		free(options.locale);
		options.locale = strdup(loc);
	}

	set_defaults();

	get_resources();

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
			{"filename",		required_argument,	NULL,	'f'},
			{"timestamp",		required_argument,	NULL,	'T'},
			{"pointer",		no_argument,		NULL,	'P'},
			{"keyboard",		no_argument,		NULL,	'K'},
			{"button",		required_argument,	NULL,	'b'},
			{"which",		required_argument,	NULL,	'w'},
			{"where",		required_argument,	NULL,	'W'},

			{"wmname",		required_argument,	NULL,	'w'},
			{"desktop",		required_argument,	NULL,	'd'},
			{"charset",		required_argument,	NULL,	'c'},
			{"language",		required_argument,	NULL,	'l'},
			{"theme",		required_argument,	NULL,	't'},

			{"key",			required_argument,	NULL,	'k'},
			{"order",		required_argument,	NULL,	'O'},
			{"cycle",		no_argument,		NULL,	'c'},
			{"normal",		no_argument,		NULL,	'4'},
			{"hidden",		no_argument,		NULL,	'1'},
			{"minimized",		no_argument,		NULL,	'm'},
			{"all-monitors",	no_argument,		NULL,	'2'},
			{"all-workspaces",	no_argument,		NULL,	'3'},
			{"noactivate",		no_argument,		NULL,	'n'},
			{"raise",		no_argument,		NULL,	'r'},
			{"restore",		no_argument,		NULL,	'R'},

			{"tooltips",		no_argument,		NULL,	't'},
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

		c = getopt_long_only(argc, argv, "d:s:M:pf:PKb:w:W:T:k:O:cmnrRtND::v::hVCH?",
				     long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:M:pf:PKb:w:W:T:k:O:cmnrRtND:vhVCH?");
#endif				/* _GNU_SOURCE */
		if (c == -1) {
			if (options.debug > 0)
				fprintf(stderr, "%s: done options processing\n", argv[0]);
			break;
		}
		switch (c) {
		case 0:
			goto bad_usage;

		case 'd':	/* -d, --display DISPLAY */
			setenv("DISPLAY", optarg, TRUE);
			free(options.display);
			options.display = strdup(optarg);
			break;
		case 's':	/* -s, --screen SCREEN */
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

		case 'p':	/* -p, --popup */
			if (options.command != CommandDefault)
				goto bad_command;
			if (command == CommandDefault)
				command = CommandRun;
			options.command = CommandRun;
			break;
		case 'f':	/* -f, --filename FILENAME */
			free(options.filename);
			options.filename = strdup(optarg);
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

		case 'b':	/* -b, --button BUTTON */
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
		case 'W':	/* -W, --where WHERE */
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

		case 'T':	/* -T, --timestamp TIMESTAMP */
			options.timestamp = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			break;

		case 'k':	/* -k, --key [KEY1:KEY2] */
			free(options.keys);
			options.keys = strdup(optarg);
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

		case '7':	/* --replace */
			options.replace = True;
			break;

		case '6':	/* --tooltips */
			options.tooltips = True;
			break;
		case 'N':	/* -N, --dry-run */
			options.dryrun = True;
			break;

		case 'D':	/* -D, --debug [LEVEL] */
			DPRINTF(1, "increasing debug verbosity\n");
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
		case 'v':	/* -v, --verbose [LEVEL] */
			DPRINTF(1, "increasing output verbosity\n");
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
		case 'h':	/* -h, --help */
		case 'H':	/* -H, --? */
			DPRINTF(1, "Setting command to CommandHelp\n");
			command = CommandHelp;
			break;
		case 'V':	/* -V, --version */
			if (options.command != CommandDefault)
				goto bad_command;
			if (command == CommandDefault)
				command = CommandVersion;
			options.command = CommandVersion;
			break;
		case 'C':	/* -C, --copying */
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
					EPRINTF("syntax error near '");
					while (optind < argc) {
						fprintf(stderr, "%s", argv[optind++]);
						fprintf(stderr, "%s", (optind < argc) ? " " : "");
					}
					fprintf(stderr, "'\n");
				} else {
					EPRINTF("missing option or argument");
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
		EPRINTF("excess non-option arguments near '");
		while (optind < argc) {
			fprintf(stderr, "%s", argv[optind++]);
			fprintf(stderr, "%s", (optind < argc) ? " " : "");
		}
		fprintf(stderr, "'\n");
		usage(argc, argv);
		exit(EXIT_SYNTAXERR);
	}
	startup(argc, argv);
	get_defaults();

	switch (command) {
	case CommandDefault:
	case CommandRun:
		DPRINTF(1, "%s: popping the menu\n", argv[0]);
		do_run(argc, argv, options.replace);
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

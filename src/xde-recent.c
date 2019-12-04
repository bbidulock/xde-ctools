/*****************************************************************************

 Copyright (c) 2010-2019  Monavacon Limited <http://www.monavacon.com/>
 Copyright (c) 2002-2009  OpenSS7 Corporation <http://www.openss7.com/>
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

#undef STARTUP_NOTIFICATION

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
#ifdef VNC_SUPPORTED
#include <X11/extensions/Xvnc.h>
#endif
#include <X11/extensions/scrnsaver.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/xf86misc.h>
#include <X11/XKBlib.h>
#ifdef STARTUP_NOTIFICATION
#define SN_API_NOT_YET_FROZEN
#include <libsn/sn.h>
#endif
#include <X11/Xdmcp.h>
#include <X11/Xauth.h>
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
#include <fontconfig/fontconfig.h>
#include <pango/pangofc-fontmap.h>

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

/** @} */

/** @section Preamble
  * @{ */

static const char *
_timestamp(void)
{
	static struct timeval tv = { 0, 0 };
	static char buf[BUFSIZ];
	double stamp;

	gettimeofday(&tv, NULL);
	stamp = (double)tv.tv_sec + (double)((double)tv.tv_usec/1000000.0);
	snprintf(buf, BUFSIZ-1, "%f", stamp);
	return buf;
}

#define XPRINTF(_args...) do { } while (0)

#define DPRINTF(_num, _args...) do { if (options.debug >= _num) { \
		fprintf(stderr, NAME ": D: [%s] %12s +%4d %s(): ", _timestamp(), __FILE__, __LINE__, __func__); \
		fprintf(stderr, _args); fflush(stderr); } } while (0)

#define EPRINTF(_args...) do { \
		fprintf(stderr, NAME ": E: [%s] %12s +%4d %s(): ", _timestamp(), __FILE__, __LINE__, __func__); \
		fprintf(stderr, _args); fflush(stderr);   } while (0)

#define OPRINTF(_num, _args...) do { if (options.debug >= _num || options.output > _num) { \
		fprintf(stdout, NAME ": I: "); \
		fprintf(stdout, _args); fflush(stdout); } } while (0)

#define PTRACE(_num) do { if (options.debug >= _num || options.output >= _num) { \
		fprintf(stderr, NAME ": T: [%s] %12s +%4d %s()\n", _timestamp(), __FILE__, __LINE__, __func__); \
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

#define XA_PREFIX		"_XDE_RECENT"
#define XA_SELECTION_NAME	XA_PREFIX "_S%d"
#define XA_NET_DESKTOP_LAYOUT	"_NET_DESKTOP_LAYOUT_S%d"
#define LOGO_NAME		"metacity"
#define XDE_DESCRIP		"An XDG compliant recent menu."

static int saveArgc;
static char **saveArgv;

#define RESNAME "xde-recent"
#define RESCLAS "XDE-Recent"
#define RESTITL "XDG Recent Menu"

#define APPDFLT "/usr/share/X11/app-defaults/" RESCLAS

/** @} */

/** @section Globals and Structures
  * @{ */

static Atom _XA_MANAGER;
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
static Atom _XA_PREFIX_POPMENU;
static Atom _XA_PREFIX_EDITOR;

static Atom _XA_NET_STARTUP_INFO;
static Atom _XA_NET_STARTUP_INFO_BEGIN;

typedef enum {
	CommandDefault,
	CommandRun,
	CommandPopMenu,			/* ask running instance to pop menu */
	CommandEditor,			/* ask running instance to pop editor */
	CommandRestart,			/* ask running instance to restart */
	CommandRefresh,			/* ask running instance to refresh menu */
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
	UseScreenSelect,		/* manually select screen */
} UseScreen;

typedef enum {
	UseWindowDefault,		/* default window by button */
	UseWindowActive,		/* active window */
	UseWindowFocused,		/* focused window */
	UseWindowPointer,		/* window under pointer */
	UseWindowSpecified,		/* specified window */
	UseWindowSelect,		/* manually select window */
} UseWindow;

typedef enum {
	PositionDefault,		/* default position */
	PositionPointer,		/* position at pointer */
	PositionCenter,			/* center of monitor */
	PositionTopLeft,		/* top left of work area */
	PositionBottomRight,		/* bottom right of work area */
	PositionSpecified,		/* specified position (X geometry) */
	PositionIconGeom,		/* icon geometry position */
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
	PopupWinds,			/* window selection feedback */
	PopupPager,			/* desktop pager feedback */
	PopupTasks,			/* task list feedback */
	PopupCycle,			/* window cycling feedback */
	PopupSetBG,			/* workspace background feedback */
	PopupStart,			/* startup notification feedback */
	PopupInput,			/* desktop input manager */
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
	char *startup_id;
	char *wmname;
	char *desktop;
	char *theme;
	char *itheme;
	char *runhist;
	char *recapps;
	char *recently;
	char *recent;
	int maximum;
	char *keypress;
	Bool keyboard;
	Bool pointer;
	int button;
	Time timestamp;
	UseScreen which;
	MenuPosition where;
	XdeGeometry geom;
	char *filename;
	Sorting sorting;
	Include include;
	Organize organize;
	gboolean groups;
	Command command;
	Bool tooltips;
	Bool exit;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.monitor = 0,
	.startup_id = NULL,
	.wmname = NULL,
	.desktop = NULL,
	.theme = NULL,
	.itheme = NULL,
	.runhist = NULL,
	.recapps = NULL,
	.recently = NULL,
	.recent = NULL,
	.maximum = 50,
	.keypress = NULL,
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
	.filename = NULL,
	.sorting = SortByDefault,
	.include = IncludeDefault,
	.organize = OrganizeDefault,
	.groups = FALSE,
	.command = CommandDefault,
	.tooltips = False,
	.exit = False,
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
	Window laywin;			/* desktop layout selection owner */
	Atom prop;			/* dekstop layout selection atom */
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
#ifdef STARTUP_NOTIFICATION
	SnMonitorContext *ctx;		/* monitor context for this screen */
	GList *sequences;		/* startup notification sequences */
#endif
};

XdeScreen *screens = NULL;		/* array of screens */

/** @} */

/** @section Menu Handling
  * @{ */

static void mainloop_quit(void);

static void
selection_done(GtkMenuShell *menushell, gpointer user_data)
{
	OPRINTF(1, "Selection done: exiting\n");
	if (!gtk_menu_get_tearoff_state(GTK_MENU(menushell)))
		mainloop_quit();
}

GList *items = NULL;

void
items_free(gpointer data)
{
	gtk_recent_info_unref((GtkRecentInfo *) data);
}

void
items_print(gpointer data, gpointer user_data)
{
	FILE *f = (typeof(f)) user_data;
	GtkRecentInfo *i = (typeof(i)) data;
	gchar **apps = NULL;
	gsize length = 0, n;
	gchar *name, *path;

	fprintf(f, "----------------------------------\n");
	if ((name = gtk_recent_info_get_short_name(i))) {
		fprintf(f, "Name:\t\t%s\n", name);
		g_free(name);
	}
	if ((path = gtk_recent_info_get_uri_display(i))) {
		fprintf(f, "Path:\t\t%s\n", path);
		g_free(path);
	}
	fprintf(f, "Local:\t\t%s\n", gtk_recent_info_is_local(i) ? "yes" : "no");
	fprintf(f, "Exists:\t\t%s\n", gtk_recent_info_exists(i) ? "yes" : "no");
	fprintf(f, "Age:\t\t%d\n", gtk_recent_info_get_age(i));
	fprintf(f, "URI:\t\t%s\n", gtk_recent_info_get_uri(i));
	fprintf(f, "DisplayName:\t%s\n", gtk_recent_info_get_display_name(i));
	fprintf(f, "Description:\t%s\n", gtk_recent_info_get_description(i));
	fprintf(f, "MimeType:\t%s\n", gtk_recent_info_get_mime_type(i));
	fprintf(f, "Added:\t\t%lu\n", gtk_recent_info_get_added(i));
	fprintf(f, "Modified:\t%lu\n", gtk_recent_info_get_modified(i));
	fprintf(f, "Visited:\t%lu\n", gtk_recent_info_get_visited(i));
	fprintf(f, "Private:\t%s\n", gtk_recent_info_get_private_hint(i) ? "yes" : "no");
	if ((apps = gtk_recent_info_get_applications(i, &length))) {
		fprintf(f, "Applications:\t\n");
		for (n = 0; n < length; n++) {
			const gchar *app_exec = NULL;
			guint count;
			time_t time_;

			fprintf(f, "\t\t%s\n", apps[n]);
			if (gtk_recent_info_get_application_info
			    (i, apps[n], &app_exec, &count, &time_)) {
				fprintf(f, "\t\tName:\t\t%s\n", apps[n]);
				fprintf(f, "\t\tExec:\t\t%s\n", app_exec);
				fprintf(f, "\t\tCount:\t\t%u\n", count);
				fprintf(f, "\t\tTime:\t\t%lu\n", time_);
			}
		}
		g_strfreev(apps);
		apps = NULL;
	}
	if ((apps = gtk_recent_info_get_groups(i, &length))) {
		fprintf(f, "Groups:\t\t\n");
		for (n = 0; n < length; n++) {
			fprintf(f, "\t\t%s\n", apps[n]);
		}
		g_strfreev(apps);
		apps = NULL;
	}
}

typedef struct {
	gchar *uri;
	gchar *mime;
	time_t stamp;
	gboolean private;
	GSList *groups;
} RecentItem;

typedef struct {
	GList *recent;
	RecentItem *current;
} RecentContext;

static void
xml_start_element(GMarkupParseContext *context, const gchar *element_name,
		  const gchar **attribute_names, const gchar **attribute_values, gpointer user_data,
		  GError **error)
{
	RecentContext *rc = user_data;

	if (!strcmp(element_name, "RecentFiles")) {
		/* don't care */
	} else if (!strcmp(element_name, "RecentItem")) {
		if (!rc->current && !(rc->current = calloc(1, sizeof(*rc->current)))) {
			EPRINTF("could not allocate element\n");
			exit(1);
		}
	} else if (!strcmp(element_name, "URI")) {
		/* don't care */
	} else if (!strcmp(element_name, "Mime-Type")) {
		/* don't care */
	} else if (!strcmp(element_name, "Timestamp")) {
		/* don't care */
	} else if (!strcmp(element_name, "Private")) {
		rc->current->private = TRUE;
	} else if (!strcmp(element_name, "Groups")) {
		/* don't care */
	} else if (!strcmp(element_name, "Group")) {
		/* don't care */
	} else {
		DPRINTF(1, "bad element name '%s'\n", element_name);
		return;
	}
}

static void
xml_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data,
		GError **error)
{
	RecentContext *rc = user_data;

	if (!strcmp(element_name, "RecentFiles")) {
		/* don't care */
	} else if (!strcmp(element_name, "RecentItem")) {
		rc->recent = g_list_append(rc->recent, rc->current);
		rc->current = NULL;
	} else if (!strcmp(element_name, "URI")) {
		/* don't care */
	} else if (!strcmp(element_name, "Mime-Type")) {
		/* don't care */
	} else if (!strcmp(element_name, "Timestamp")) {
		/* don't care */
	} else if (!strcmp(element_name, "Private")) {
		/* don't care */
	} else if (!strcmp(element_name, "Groups")) {
		/* don't care */
	} else if (!strcmp(element_name, "Group")) {
		/* don't care */
	} else {
		DPRINTF(1, "bad element name '%s'\n", element_name);
		return;
	}
}

static void
xml_character_data(GMarkupParseContext *context, const gchar *text, gsize text_len,
		   gpointer user_data, GError **error)
{
	RecentContext *rc = user_data;
	const gchar *element_name;

	element_name = g_markup_parse_context_get_element(context);
	if (!strcmp(element_name, "RecentFiles")) {
		/* don't care */
	} else if (!strcmp(element_name, "RecentItem")) {
		/* don't care */
	} else if (!strcmp(element_name, "URI")) {
		free(rc->current->uri);
		rc->current->uri = calloc(1, text_len + 1);
		memcpy(rc->current->uri, text, text_len);
	} else if (!strcmp(element_name, "Mime-Type")) {
		free(rc->current->mime);
		rc->current->mime = calloc(1, text_len + 1);
		memcpy(rc->current->mime, text, text_len);
	} else if (!strcmp(element_name, "Timestamp")) {
		char *buf, *end = NULL;
		unsigned long int val;

		rc->current->stamp = 0;
		buf = calloc(1, text_len + 1);
		memcpy(buf, text, text_len);
		val = strtoul(buf, &end, 0);
		if (end && *end == '\0')
			rc->current->stamp = val;
	} else if (!strcmp(element_name, "Private")) {
		/* don't care */
	} else if (!strcmp(element_name, "Groups")) {
		/* don't care */
	} else if (!strcmp(element_name, "Group")) {
		char *buf;

		buf = calloc(1, text_len + 1);
		memcpy(buf, text, text_len);
		rc->current->groups = g_slist_append(rc->current->groups, (gpointer) buf);
	} else {
		DPRINTF(1, "bad element name '%s'\n", element_name);
		return;
	}
}

static void
xml_passthrough(GMarkupParseContext *context, const gchar *passthrough_text, gsize text_len,
		gpointer user_data, GError **error)
{
	/* don't care */
}

static void
xml_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
	EPRINTF("got an error during parsing\n");
	exit(1);
}

GMarkupParser xml_parser = {
	.start_element = xml_start_element,
	.end_element = xml_end_element,
	.text = xml_character_data,
	.passthrough = xml_passthrough,
	.error = xml_error,
};

void
group_free(gpointer data)
{
	free((char *) data);
}

void
recent_free(gpointer data)
{
	RecentItem *r = (typeof(r)) data;

	free(r->uri);
	free(r->mime);
	g_slist_free_full(r->groups, group_free);
	free(r);
}

typedef struct {
	char *label;
	char *place;
	char *cmd;
	char *icon;
	GdkPixbuf *pixbuf;
	char *tooltip;
	char *mime;
	time_t time;
	int count;
} Place;

static void
xde_list_free(gpointer data)
{
	Place *place = data;

	free(place->label);
	free(place->place);
	free(place->cmd);
	free(place->icon);
	if (place->pixbuf)
		g_object_unref(place->pixbuf);
	free(place->tooltip);
	free(place->mime);
	free(place);
}

static void
manager_free(gpointer info)
{
	gtk_recent_info_unref(info);
}

static GList *
get_gtk2_recent_list(GList *list, const char *file)
{
	GtkRecentManager *mgr;
	GList *items, *item;

	if (!(mgr = g_object_new(GTK_TYPE_RECENT_MANAGER, "filename", file, NULL))) {
		EPRINTF("could not get recement manager for %s\n", file);
		return (list);
	}
	items = gtk_recent_manager_get_items(mgr);
	for (item = items; item; item = item->next) {
		GtkRecentInfo *info;
		const gchar *uri;
		Place *place;
		GList *node;
		const gchar *value, *mime;
		time_t time;
		gchar **names, **name;
		GdkPixbuf *pixbuf;
		gboolean isapp = FALSE;

		if (!(info = item->data))
			continue;
		uri = gtk_recent_info_get_uri(info);

		DPRINTF(1, "processing gtk2 uri %s\n", uri);

#if 0
		if (gtk_recent_info_get_private_hint(info)) {
			DPRINTF(1, "uri is private: %s\n", uri);
			continue;
		}
#endif
		if ((mime = gtk_recent_info_get_mime_type(info))) {
			isapp = !strcmp(mime, "application/x-desktop");
			switch (options.include) {
			case IncludeDefault:
			case IncludeDocs:
				if (isapp) {
					DPRINTF(1, "not including apps\n");
					continue;
				}
				break;
			case IncludeApps:
				if (!isapp) {
					DPRINTF(1, "not including docs\n");
					continue;
				}
				break;
			case IncludeBoth:
				break;
			}
		}
		for (node = list; node; node = node->next)
			if ((place = node->data) && place->place && !strcmp(place->place, uri))
				break;
		if (node) {
			place = node->data;
			DPRINTF(1, "using gtk2 uri %s\n", uri);
		} else {
			DPRINTF(1, "adding gtk2 uri %s\n", uri);
			place = calloc(1, sizeof(*place));
			place->place = g_strdup(uri);
			list = g_list_append(list, place);
		}
		if ((value = gtk_recent_info_get_display_name(info))
		    || (value = gtk_recent_info_get_short_name(info))) {
			g_free(place->label);
			place->label = g_strdup(value);
		}
		if ((value = gtk_recent_info_get_description(info))) {
			g_free(place->tooltip);
			place->tooltip = g_strdup(value);
		}
		if ((pixbuf = gtk_recent_info_get_icon(info, 16))) {
			if (place->pixbuf)
				g_object_unref(place->pixbuf);
			place->pixbuf = pixbuf;
		}
		if (mime) {
			g_free(place->mime);
			place->mime = g_strdup(mime);
		}
		time = gtk_recent_info_get_added(info);
		if (time != -1 && time > place->time)
			place->time = time;
		time = gtk_recent_info_get_modified(info);
		if (time != -1 && time > place->time)
			place->time = time;
		time = gtk_recent_info_get_visited(info);
		if (time != -1 && time > place->time)
			place->time = time;
		names = gtk_recent_info_get_applications(info, NULL);
		for (name = names; name && *name; name++) {
			const gchar *exec = NULL;
			guint count = 0;
			time_t stamp = -1;

			if (!(gtk_recent_info_get_application_info(info, *name, &exec, &count,
								   &stamp)))
				continue;
			if (exec) {
				g_free(place->cmd);
				place->cmd = g_strdup(exec);
			}
			if (stamp != -1 && stamp > place->time)
				place->time = stamp;
			place->count += count;
		}
		if (names)
			g_strfreev(names);
	}
	g_list_free_full(items, &manager_free);
	g_object_unref(mgr);
	return (list);
}

static GList *
get_xbel_recent_list(GList *list, const char *file)
{
	GBookmarkFile *bookmark;
	gchar **uris, **uri;
	GError *error = NULL;

	DPRINTF(1, "getting bookmark file %s\n", file);
	if (!(bookmark = g_bookmark_file_new())) {
		EPRINTF("could not obtain bookmark file!\n");
		return (list);
	}
	if (!g_bookmark_file_load_from_file(bookmark, file, &error)) {
		DPRINTF(1, "could not load bookmark file %s: %s\n", file, error->message);
		g_error_free(error);
		error = NULL;
		g_bookmark_file_free(bookmark);
		return (list);
	}
	DPRINTF(1, "processing xbel file %s\n", file);
	uris = g_bookmark_file_get_uris(bookmark, NULL);
	for (uri = uris; uri && *uri; uri++) {
		Place *place;
		GList *node;
		gchar *value, *mime;
		time_t time;
		gchar **names, **name;

		DPRINTF(1, "processing xbel uri %s\n", *uri);
#if 0
		if (g_bookmark_file_get_is_private(bookmark, *uri, NULL)) {
			DPRINTF(1, "uri is private: %s\n", *uri);
			continue;
		}
#endif
		if ((mime = g_bookmark_file_get_mime_type(bookmark, *uri, NULL))) {
			switch (options.include) {
			case IncludeDefault:
			case IncludeDocs:
				if (!strcmp(mime, "application/x-desktop")) {
					DPRINTF(1, "not including apps\n");
					continue;
				}
				break;
			case IncludeApps:
				if (strcmp(mime, "application/x-desktop")) {
					DPRINTF(1, "not including docs\n");
					continue;
				}
				break;
			case IncludeBoth:
				break;
			}
		}
		for (node = list; node; node = node->next)
			if ((place = node->data) && place->place && !strcmp(place->place, *uri))
				break;

		if (node) {
			place = node->data;
			DPRINTF(1, "using xbel uri %s\n", *uri);
		} else {
			DPRINTF(1, "adding xbel uri %s\n", *uri);
			place = calloc(1, sizeof(*place));
			place->place = g_strdup(*uri);
			list = g_list_append(list, place);
		}
		if ((value = g_bookmark_file_get_title(bookmark, *uri, NULL))) {
			g_free(place->label);
			place->label = value;
		}
		if ((value = g_bookmark_file_get_description(bookmark, *uri, NULL))) {
			g_free(place->tooltip);
			place->tooltip = value;
		}
		value = NULL;
		if ((g_bookmark_file_get_icon(bookmark, *uri, &value, &mime, &error))) {
			g_free(place->icon);
			place->icon = value;
			DPRINTF(1, "GOT ICON %s\n", value);
		} else if (error) {
			DPRINTF(1, "could not get icon for %s: %s\n", *uri, error->message);
			g_error_free(error);
			error = NULL;
		} else {
			DPRINTF(1, "could not get icon for %s: %s: %s\n", *uri, value, mime);
		}
		if (mime) {
			g_free(place->mime);
			place->mime = mime;
		}
		time = g_bookmark_file_get_added(bookmark, *uri, NULL);
		if (time != -1 && time > place->time)
			place->time = time;
		time = g_bookmark_file_get_modified(bookmark, *uri, NULL);
		if (time != -1 && time > place->time)
			place->time = time;
		time = g_bookmark_file_get_visited(bookmark, *uri, NULL);
		if (time != -1 && time > place->time)
			place->time = time;
		names = g_bookmark_file_get_applications(bookmark, *uri, NULL, NULL);
		for (name = names; name && *name; name++) {
			gchar *exec = NULL;
			guint count = 0;
			time_t stamp = -1;

			if (!(g_bookmark_file_get_app_info(bookmark, *uri, *name, &exec,
							   &count, &stamp, NULL)))
				continue;
			if (exec) {
				g_free(place->cmd);
				place->cmd = exec;
			}
			if (stamp != -1 && stamp > place->time)
				place->time = stamp;
			place->count += count;
		}
		if (names)
			g_strfreev(names);
	}
	if (uris)
		g_strfreev(uris);
	return (list);
}

static GList *
get_xml_recent_list(GList *list, const char *file)
{
	GMarkupParseContext *context;
	GError *error = NULL;
	gchar buffer[BUFSIZ];
	gsize got;
	FILE *f;
	int dummy;
	RecentContext rc = { NULL, NULL };
	GList *recent;

	if (!(f = fopen(file, "r"))) {
		DPRINTF(1, "file %s: %s\n", file, strerror(errno));
		return (list);
	}
	DPRINTF(1, "processing xml file %s\n", file);
	dummy = lockf(fileno(f), F_LOCK, 0);
	if (!(context = g_markup_parse_context_new(&xml_parser,
						   G_MARKUP_TREAT_CDATA_AS_TEXT |
						   G_MARKUP_PREFIX_ERROR_POSITION |
						   G_MARKUP_IGNORE_QUALIFIED, &rc, NULL))) {
		EPRINTF("cannot create XML parser\n");
		dummy = lockf(fileno(f), F_ULOCK, 0);
		fclose(f);
		return (list);
	}
	while ((got = fread(buffer, 1, BUFSIZ, f)) > 0) {
		if (!g_markup_parse_context_parse(context, buffer, got, &error)) {
			EPRINTF("cannot parse buffer: %s\n", error->message);
			g_error_free(error);
			error = NULL;
			g_markup_parse_context_unref(context);
			dummy = lockf(fileno(f), F_ULOCK, 0);
			fclose(f);
			return (list);
		}
	}
	if (!g_markup_parse_context_end_parse(context, &error)) {
		EPRINTF("could not end parsing: %s\n", error->message);
		g_error_free(error);
		error = NULL;
		g_markup_parse_context_unref(context);
		dummy = lockf(fileno(f), F_ULOCK, 0);
		fclose(f);
		return (list);
	}
	g_markup_parse_context_unref(context);
	dummy = lockf(fileno(f), F_ULOCK, 0);
	fclose(f);
	for (recent = rc.recent; recent; recent = recent->next) {
		RecentItem *item;
		Place *place;
		GList *node;

		if (!(item = recent->data)) {
			EPRINTF("no data!\n");
			continue;
		}
		DPRINTF(1, "processing xml uri %s\n", item->uri);
#if 0
		if (item->private) {
			DPRINTF(1, "uri is private: %s\n", item->uri);
			continue;
		}
#endif
		if (item->mime) {
			switch (options.include) {
			case IncludeDefault:
			case IncludeDocs:
				if (!strcmp(item->mime, "application/x-desktop")) {
					DPRINTF(1, "not including apps\n");
					continue;
				}
				break;
			case IncludeApps:
				if (strcmp(item->mime, "application/x-desktop")) {
					DPRINTF(1, "not including docs\n");
					continue;
				}
				break;
			case IncludeBoth:
				break;
			}
		}
		for (node = list; node; node = node->next)
			if ((place = node->data) && place->place && !strcmp(place->place, item->uri))
				break;
		if (node) {
			DPRINTF(1, "using xml uri %s\n", item->uri);
			place = node->data;
		} else {
			DPRINTF(1, "adding xml uri %s\n", item->uri);
			place = calloc(1, sizeof(*place));
			place->place = g_strdup(item->uri);
			list = g_list_append(list, place);
		}
		if (item->stamp && item->stamp != -1 && item->stamp > place->time)
			place->time = item->stamp;
		if (!place->mime && item->mime)
			place->mime = g_strdup(item->mime);
		place->count++;
	}
	g_list_free_full(rc.recent, recent_free);
	(void) dummy;
	return (list);
}

static GList *
get_simple_recent_list(GList *list, const char *filename)
{
	return (list);
}

static gint
sort_recent(gconstpointer a, gconstpointer b)
{
	const Place *A = a;
	const Place *B = b;

	if (options.groups) {
		gboolean aisapp = (A->mime && !strcmp(A->mime, "application/x-desktop"));
		gboolean bisapp = (B->mime && !strcmp(B->mime, "application/x-desktop"));

		if (!aisapp && bisapp)
			return (1);
		if (aisapp && !bisapp)
			return (-1);
	}
	if (A->time < B->time)
		return (1);
	if (A->time > B->time)
		return (-1);
	if (A->count < B->count)
		return (1);
	if (A->count > B->count)
		return (-1);
	return (0);
}

static gint
sort_favorite(gconstpointer a, gconstpointer b)
{
	const Place *A = a;
	const Place *B = b;

	if (options.groups) {
		gboolean aisapp = (A->mime && !strcmp(A->mime, "application/x-desktop"));
		gboolean bisapp = (B->mime && !strcmp(B->mime, "application/x-desktop"));

		if (!aisapp && bisapp)
			return (1);
		if (aisapp && !bisapp)
			return (-1);
	}
	if (A->count < B->count)
		return (1);
	if (A->count > B->count)
		return (-1);
	if (A->time < B->time)
		return (1);
	if (A->time > B->time)
		return (-1);
	return (0);
}

static GList *
do_list_sorting(GList *list)
{
	switch (options.sorting) {
	case SortByDefault:
	case SortByRecent:
		list = g_list_sort(list, &sort_recent);
		break;
	case SortByFavorite:
		list = g_list_sort(list, &sort_favorite);
		break;
	}
	return (list);
}

void
xde_entry_activated(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *menu;
	char *cmd;

	if ((cmd = user_data)) {
		pid_t pid;
		wordexp_t we = { 0, };
		int status;

		if ((status = wordexp(cmd, &we, 0)) != 0 || we.we_wordc < 1) {
			switch(status) {
			case WRDE_BADCHAR:
				EPRINTF("adwm: bad character in command string: %s\n", cmd);
				break;
			case WRDE_BADVAL:
				EPRINTF("adwm: undefined variable substitution in command string: %s\n", cmd);
				break;
			case WRDE_CMDSUB:
				EPRINTF("adwm: command substitution in command string: %s\n", cmd);
				break;
			case WRDE_NOSPACE:
				EPRINTF("adwm: out of memory processing command string: %s\n", cmd);
				break;
			case WRDE_SYNTAX:
				EPRINTF("adwm: syntax error in command string: %s\n", cmd);
				break;
			default:
				EPRINTF("adwm: unknown error processing command string: %s\n", cmd);
				break;
			}
			wordfree(&we); /* necessary ??? */
			return;
		}
		if ((pid = fork()) == -1) {
			/* error */
			EPRINTF("%s: %s\n", NAME, strerror(errno));
			wordfree(&we);
			exit(EXIT_FAILURE);
			return;
		} else if (pid == 0) {
			/* we are the child */
			setsid();
			execvp(we.we_wordv[0], we.we_wordv);
			wordfree(&we);
			exit(127);
			return;
		}
		wordfree(&we);
		/* we are the parent */
	}
	if (!(menu = gtk_widget_get_parent(GTK_WIDGET(menuitem))) ||
	    !gtk_menu_get_tearoff_state(GTK_MENU(menu)))
		mainloop_quit();
}

GHashTable *mime_icons = NULL;
GHashTable *mime_subcl = NULL;
GHashTable *mime_alias = NULL;

void
get_mime_databases(void)
{
	const gchar *const *dirs, *const *dir;
	const gchar *user;
	GList *paths = NULL, *path;
	char *b;
	int dummy;

	(void) dummy;
	user = g_get_user_data_dir();
	DPRINTF(1, "adding path: %s\n", user);
	paths = g_list_prepend(paths, g_strdup(user));
	dirs = g_get_system_data_dirs();
	for (dir = dirs; dir && *dir; dir++) {
		DPRINTF(1, "adding path: %s\n", *dir);
		paths = g_list_prepend(paths, g_strdup(*dir));
	}

	b = calloc(BUFSIZ, sizeof(*b));

	if (mime_icons)
		g_hash_table_destroy(mime_icons);
	mime_icons = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	for (path = paths; path; path = path->next) {
		gchar *file;
		FILE *f;

		file = g_build_filename(path->data, "mime", "generic-icons", NULL);
		if (!(f = fopen(file, "r"))) {
			DPRINTF(1, "cannot open %s: %s:\n", file, strerror(errno));
			g_free(file);
			continue;
		}
		DPRINTF(1, "processing %s\n", file);
		dummy = lockf(fileno(f), F_LOCK, 0);
		while (fgets(b, BUFSIZ, f)) {
			char *p;

			if (b[0] == '#')
				continue;
			if ((p = strrchr(b, '\n')))
				*p = '\0';
			if (!(p = strchr(b, ':')) || !p[1])
				continue;
			*p = '\0';
			p++;
			/* field 1 is b; field 2 is p */
			g_hash_table_replace(mime_icons, g_strdup(b), g_strdup(p));
		}
		dummy = lockf(fileno(f), F_ULOCK, 0);
		fclose(f);
		g_free(file);
	}
	for (path = paths; path; path = path->next) {
		gchar *file;
		FILE *f;

		file = g_build_filename(path->data, "mime", "icons", NULL);
		if (!(f = fopen(file, "r"))) {
			DPRINTF(1, "cannot open %s: %s:\n", file, strerror(errno));
			g_free(file);
			continue;
		}
		DPRINTF(1, "processing %s\n", file);
		dummy = lockf(fileno(f), F_LOCK, 0);
		while (fgets(b, BUFSIZ, f)) {
			char *p;

			if (b[0] == '#')
				continue;
			if ((p = strrchr(b, '\n')))
				*p = '\0';
			if (!(p = strchr(b, ':')) || !p[1])
				continue;
			*p = '\0';
			p++;
			/* field 1 is b; field 2 is p */
			g_hash_table_replace(mime_icons, g_strdup(b), g_strdup(p));
		}
		dummy = lockf(fileno(f), F_ULOCK, 0);
		fclose(f);
		g_free(file);
	}

	if (mime_subcl)
		g_hash_table_destroy(mime_subcl);
	mime_subcl = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	for (path = paths; path; path = path->next) {
		gchar *file;
		FILE *f;

		file = g_build_filename(path->data, "mime", "subclasses", NULL);
		if (!(f = fopen(file, "r"))) {
			DPRINTF(1, "cannot open %s: %s:\n", file, strerror(errno));
			g_free(file);
			continue;
		}
		DPRINTF(1, "processing %s\n", file);
		dummy = lockf(fileno(f), F_LOCK, 0);
		while (fgets(b, BUFSIZ, f)) {
			char *p;

			if (b[0] == '#')
				continue;
			if ((p = strrchr(b, '\n')))
				*p = '\0';
			if (!(p = strchr(b, ' ')) || !p[1])
				continue;
			*p = '\0';
			p++;
			/* field 1 is b; field 2 is p */
			g_hash_table_replace(mime_subcl, g_strdup(b), g_strdup(p));
		}
		dummy = lockf(fileno(f), F_ULOCK, 0);
		fclose(f);
		g_free(file);
	}

	if (mime_alias)
		g_hash_table_destroy(mime_alias);
	mime_alias = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	for (path = paths; path; path = path->next) {
		gchar *file;
		FILE *f;

		file = g_build_filename(path->data, "mime", "aliases", NULL);
		if (!(f = fopen(file, "r"))) {
			DPRINTF(1, "cannot open %s: %s:\n", file, strerror(errno));
			g_free(file);
			continue;
		}
		DPRINTF(1, "processing %s\n", file);
		dummy = lockf(fileno(f), F_LOCK, 0);
		while (fgets(b, BUFSIZ, f)) {
			char *p;

			if (b[0] == '#')
				continue;
			if ((p = strrchr(b, '\n')))
				*p = '\0';
			if (!(p = strchr(b, ' ')) || !p[1])
				continue;
			*p = '\0';
			p++;
			/* field 1 is b; field 2 is p */
			g_hash_table_replace(mime_alias, g_strdup(b), g_strdup(p));
		}
		dummy = lockf(fileno(f), F_ULOCK, 0);
		fclose(f);
		g_free(file);
	}
	g_list_free_full(paths, g_free);
	free(b);
}

/* FIXME: convert to consider monitors */
static GtkWidget *
popup_menu_new(XdeMonitor *xmon)
{
	GtkWidget *menu, *item, *image;
	GtkIconTheme *itheme;
	gchar *file;
	GList *list = NULL, *node;
	unsigned int i, max = options.maximum;

	get_mime_databases();

	menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(menu), "Recent");
#if 0
	item = gtk_tearoff_menu_item_new();
	gtk_widget_show(item);
	gtk_menu_append(menu, item);
#endif

	itheme = gtk_icon_theme_get_default();
#if 0
	{
		gchar **paths = NULL, **path;

		gtk_icon_theme_get_search_path(itheme, &paths, NULL);
		DPRINTF(1, "default icon search path is:s\n");
		for (path = paths; path && *path; path++)
			DPRINTF(1, "\t%s\n", *path);
		g_strfreev(paths);
	}
#endif

	switch (options.include) {
	default:
	case IncludeDefault:
	case IncludeDocs:
	case IncludeBoth:
		file = g_build_filename(g_get_home_dir(), ".recently-used", NULL);
		list = get_xml_recent_list(list, file);
		g_free(file);

		file = g_build_filename(g_get_user_data_dir(), "recently-used", NULL);
		list = get_xml_recent_list(list, file);
		g_free(file);

		file = g_build_filename(g_get_user_data_dir(), "recently-used.xbel", NULL);
		list = get_xbel_recent_list(list, file);
		list = get_gtk2_recent_list(list, file);
		g_free(file);
		break;
	case IncludeApps:
		break;
	}
	switch (options.include) {
	default:
	case IncludeDefault:
	case IncludeDocs:
		break;
	case IncludeApps:
	case IncludeBoth:
		file =
		    g_build_filename(g_get_user_config_dir(), "xde", "recent-applications", NULL);
		list = get_simple_recent_list(list, file);
		g_free(file);

		file = g_build_filename(g_get_home_dir(), ".recent-applications", NULL);
		list = get_xml_recent_list(list, file);
		g_free(file);

		file = g_build_filename(g_get_user_data_dir(), "recent-applications", NULL);
		list = get_xml_recent_list(list, file);
		g_free(file);

		file = g_build_filename(g_get_user_data_dir(), "recent-applications.xbel", NULL);
		list = get_xbel_recent_list(list, file);
		list = get_gtk2_recent_list(list, file);
		g_free(file);
		break;
	}

	list = do_list_sorting(list);

	for (i = 0, node = list; node; node = node->next) {
		Place *place = node->data;
		gchar *filename, *appid = NULL;
		GDesktopAppInfo *app = NULL;
		GIcon *gicon = NULL;
		GdkPixbuf *pixbuf = NULL;
		GtkIconInfo *info = NULL;

		filename = g_filename_from_uri(place->place, NULL, NULL);
		if (filename && access(filename, R_OK)) {
			g_free(filename);
			continue;
		}

		DPRINTF(1, "creating uri %s\n", place->place);
		DPRINTF(1, "filename is '%s'\n", filename);
		DPRINTF(1, "place->place is '%s'\n", place->place);

		item = gtk_image_menu_item_new();

		if (place->mime && !strcmp(place->mime, "application/x-desktop") && filename) {
			if ((appid = strrchr(filename, '/')))
				appid++;
			if (!access(filename, R_OK))
				app = g_desktop_app_info_new_from_filename(filename);
			else if (appid)
				app = g_desktop_app_info_new(appid);
			if (!app) {
				DPRINTF(1, "cannot get app for %s\n", appid ? : filename);
				g_free(filename);
				continue;
			}
		}
#if 1
		if (!app && (!filename || access(filename, R_OK))) {
			DPRINTF(1, "file %s does not exist or is not readable\n", filename);
			g_free(filename);
			continue;
		}
#endif
		if (!place->label && app) {
			const char *label;

			if ((label = g_app_info_get_name(G_APP_INFO(app))) ||
			    (label = g_app_info_get_display_name(G_APP_INFO(app))) ||
			    (label = g_desktop_app_info_get_generic_name(app)))
				place->label = g_strdup(label);
		}

		if (place->label)
			gtk_menu_item_set_label(GTK_MENU_ITEM(item), place->label);
		else {
			char *p;

			if ((p = strrchr(place->place, '/')))
				p++;
			else
				p = place->place;
			gtk_menu_item_set_label(GTK_MENU_ITEM(item), p);
		}
		if (place->mime && !place->icon) {
			char *icon = NULL, *mime = place->mime;

			while (!icon) {
				if ((icon = g_hash_table_lookup(mime_icons, place->mime))) {
					place->icon = g_strdup(icon);
					break;
				}
				if ((mime = g_hash_table_lookup(mime_alias, place->mime))) {
					g_free(place->mime);
					place->mime = g_strdup(mime);
					continue;
				}
				if ((mime = g_hash_table_lookup(mime_subcl, place->mime))) {
					g_free(place->mime);
					place->mime = g_strdup(mime);
					continue;
				}
				break;
			}
		}
		if (place->icon) {
			if ((pixbuf = gtk_icon_theme_load_icon(itheme, place->icon, 16,
							       GTK_ICON_LOOKUP_USE_BUILTIN |
							       GTK_ICON_LOOKUP_FORCE_SIZE |
							       GTK_ICON_LOOKUP_GENERIC_FALLBACK,
							       NULL))
			    && (image = gtk_image_new_from_pixbuf(pixbuf)))
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
			if (pixbuf) {
				g_object_unref(pixbuf);
				pixbuf = NULL;
			}
		}
		if (place->pixbuf) {
			if ((image = gtk_image_new_from_pixbuf(place->pixbuf)))
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), image);
		}
		if (app) {
			if ((gicon = g_app_info_get_icon(G_APP_INFO(app)))) {
				gchar *icon = g_icon_to_string(gicon);

				DPRINTF(1, "got gicon with name %s\n", icon);
				if ((info =
				     gtk_icon_theme_lookup_by_gicon(itheme, gicon, 16,
								    GTK_ICON_LOOKUP_USE_BUILTIN |
								    GTK_ICON_LOOKUP_FORCE_SIZE |
								    GTK_ICON_LOOKUP_GENERIC_FALLBACK))
				    && (pixbuf = gtk_icon_info_load_icon(info, NULL))
				    && (image = gtk_image_new_from_pixbuf(pixbuf)))
					gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM
								      (item), image);
				else {
					DPRINTF(1, "could not get image for gicon %s\n", icon);
				}
				if (info) {
					gtk_icon_info_free(info);
					info = NULL;
				}
				if (pixbuf) {
					g_object_unref(pixbuf);
					pixbuf = NULL;
				}
				g_free(icon);
			} else
				DPRINTF(1, "cannot get gicon from desktop app %s\n", filename);
#if 0
			if ((icon =
			     g_desktop_app_info_get_string(app, G_KEY_FILE_DESKTOP_KEY_ICON))) {
				char *name, *base, *p;

				if (icon[0] == '/' && !access(icon, R_OK)) {
					DPRINTF(1, "going with full icon path %s\n", icon);
					name = g_strdup(icon);
				} else {
					base = icon;
					*strchrnul(base, ' ') = '\0';
					if ((p = strrchr(base, '/')))
						base = p + 1;
					if ((p = strrchr(base, '.')))
						*p = '\0';
					name = g_strdup(base);
				}
				g_free(icon);
			}
#endif
		}
		if (!place->tooltip && app) {
			const char *tooltip;

			if ((tooltip = g_app_info_get_description(G_APP_INFO(app))))
				place->tooltip = g_strdup(tooltip);
			else
				place->tooltip = g_desktop_app_info_get_string(app,
									       G_KEY_FILE_DESKTOP_KEY_COMMENT);
		}
		if (!place->tooltip) {
			char *tooltip, *p, *r;

			if ((tooltip = g_uri_unescape_string(place->place, "/"))) {
				r = tooltip;
				if ((p = strstr(r, "://"))) {
					r = p + 3;
					if ((p = strchr(r, '/')))
						r = p;
				}
				place->tooltip = g_strdup(r);
				g_free(tooltip);
			}
		}
		if (!place->cmd) {
			g_free(place->cmd);
			if (app)
				place->cmd = g_strdup_printf("xdg-launch '%s'", appid ? : (filename ? : place->place));
			else
				place->cmd = g_strdup_printf("xdg-open '%s'", (filename ? : place->place));
		}
		{
			gchar *p, *q;;

			/* strip file:// out cause some appls don't like it (e.g. inkscape) */
			if ((p = strstr(place->cmd, "file://"))) {
				q = place->cmd;
				*p = '\0';
				p += 7;
				place->cmd = g_strdup_printf("%s%s", q, p);
				g_free(q);
			}
		}
		if (options.debug || options.tooltips) {
			gchar *markup;

			markup =
			    g_markup_printf_escaped("<b>URI:</b> %s\n" "<b>Icon:</b> %s\n"
						    "<b>Label:</b> %s\n" "<b>Tooltip:</b> %s\n"
						    "<b>Mime-type:</b> %s\n" "<b>Command:</b> %s\n"
						    "<b>Time:</b> %s" "<b>Count:</b> %d",
						    place->place, place->icon, place->label,
						    place->tooltip, place->mime, place->cmd,
						    ctime(&place->time), place->count);
			gtk_widget_set_tooltip_markup(item, markup);
			g_free(markup);
		} else {
			if (place->tooltip)
				gtk_widget_set_tooltip_text(item, place->tooltip);
		}
		DPRINTF(1, "command is '%s'\n", place->cmd);

		if (place->cmd)
			g_signal_connect(G_OBJECT(item), "activate",
					 G_CALLBACK(xde_entry_activated), g_strdup(place->cmd));
		gtk_menu_append(menu, item);
		gtk_widget_show(item);
		if (app)
			g_object_unref(app);
		g_free(filename);
		if (max && i++ >= max)
			break;
	}
	g_list_free_full(list, &xde_list_free);

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

	switch (options.which) {
	case UseScreenSelect:
		/* FIXME */
		__attribute((fallthrough));
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
		if ((xmon = find_specific_monitor()))
			return (xmon);
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
position_pointer(GtkWidget *widget, XdeMonitor *xmon, gint *x, gint *y)
{
	PTRACE(5);
	gdk_display_get_pointer(disp, NULL, x, y, NULL);
	return TRUE;
}

static gboolean
position_center_monitor(GtkWidget *widget, XdeMonitor *xmon, gint *x, gint *y)
{
	GtkRequisition req;

	PTRACE(5);
	gtk_widget_get_requisition(widget, &req);

	*x = xmon->geom.x + (xmon->geom.width - req.width) / 2;
	*y = xmon->geom.y + (xmon->geom.height - req.height) / 2;

	return TRUE;
}

static gboolean
position_topleft_workarea(GtkWidget *widget, XdeMonitor *xmon, gint *x, gint *y)
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
position_bottomright_workarea(GtkWidget *widget, XdeMonitor *xmon, gint *x, gint *y)
{
#if 1
	WnckWorkspace *wkspc;
	GtkRequisition req;

	/* XXX: not sure this is what we want... */
	wkspc = wnck_screen_get_active_workspace(xmon->xscr->wnck);
	gtk_widget_get_requisition(widget, &req);
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
	gtk_widget_get_requisition(widget, &req);

	*x = rect.x + rect.width - req.width;
	*y = rect.y + rect.height - req.height;
#endif

	return TRUE;
}

static gboolean
position_specified(GtkWidget *widget, XdeMonitor *xmon, gint *x, gint *y)
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
		GtkAllocation alloc = { 0, };
		int x2, y2;

		gtk_widget_realize(widget);
		gtk_widget_get_allocation(widget, &alloc);
		x2 = x1 + options.geom.w;
		y2 = y1 + options.geom.h;
		DPRINTF(1, "geometry x2 = %d\n", x2);
		DPRINTF(1, "geometry y2 = %d\n", y2);

		if (x1 + alloc.width < sw)
			*x = x1;
		else if (x2 - alloc.width > 0)
			*x = x2 - alloc.width;
		else
			*x = 0;

		if (y2 + alloc.height < sh)
			*y = y2;
		else if (y1 - alloc.height > 0)
			*y = y1 - alloc.height;
		else
			*y = 0;
	}
	DPRINTF(1, "placing window at +%d+%d\n", *x, *y);
	return TRUE;
}

void
position_widget(GtkWidget *widget, gint *x, gint *y, XdeMonitor *xmon)
{
	if (options.button) {
		position_pointer(widget, xmon, x, y);
		return;
	}
	switch (options.where) {
	case PositionDefault:
	default:
		position_center_monitor(widget, xmon, x, y);
		break;
	case PositionPointer:
		position_pointer(widget, xmon, x, y);
		break;
	case PositionCenter:
		position_center_monitor(widget, xmon, x, y);
		break;
	case PositionTopLeft:
		position_topleft_workarea(widget, xmon, x, y);
		break;
	case PositionBottomRight:
		position_bottomright_workarea(widget, xmon, x, y);
		break;
	case PositionSpecified:
		position_specified(widget, xmon, x, y);
		break;
	}
}

void
position_menu(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	XdeMonitor *xmon = user_data;

	if (push_in)
		*push_in = FALSE;
	position_widget(GTK_WIDGET(menu), x, y, xmon);
}

/** @} */

/** @section Setting and Getting Arguments
  * @{ */

/** @section Setting Arguments
  * @{ */

#if 0
static const char *show_bool(Bool value);
static const char *show_which(UseScreen which);
static const char *show_where(MenuPosition where);

static void
set_scmon(long scmon)
{
	options.monitor = (short) ((scmon >> 0) & 0xffff);
	options.screen = (short) ((scmon >> 16) & 0xffff);
	DPRINTF(1, "options.monitor = %d\n", options.monitor);
	DPRINTF(1, "options.screen = %d\n", options.screen);
}

static void
set_flags(long flags)
{
#if 1
	options.fileout = (flags & XDE_MENU_FLAG_FILEOUT) ? True : False;
	DPRINTF(1, "options.fileout = %s\n", show_bool(options.fileout));
	options.noicons = (flags & XDE_MENU_FLAG_NOICONS) ? True : False;
	DPRINTF(1, "options.noicons = %s\n", show_bool(options.noicons));
	options.launch = (flags & XDE_MENU_FLAG_LAUNCH) ? True : False;
	DPRINTF(1, "options.launch = %s\n", show_bool(options.launch));
#endif
	options.systray = (flags & XDE_MENU_FLAG_TRAY) ? True : False;
	DPRINTF(1, "options.systray = %s\n", show_bool(options.systray));
#if 1
	options.generate = (flags & XDE_MENU_FLAG_GENERATE) ? True : False;
	DPRINTF(1, "options.generate = %s\n", show_bool(options.generate));
#endif
	options.tooltips = (flags & XDE_MENU_FLAG_TOOLTIPS) ? True : False;
	DPRINTF(1, "options.tooltips = %s\n", show_bool(options.tooltips));
#if 1
	options.actions = (flags & XDE_MENU_FLAG_ACTIONS) ? True : False;
	DPRINTF(1, "options.actions = %s\n", show_bool(options.actions));
#endif
#if 0
	if (flags & XDE_MENU_FLAG_EXCLUDED)
		options.treeflags |= GMENU_TREE_FLAGS_INCLUDE_EXCLUDED;
	else
		options.treeflags &= ~GMENU_TREE_FLAGS_INCLUDE_EXCLUDED;
	if (flags & XDE_MENU_FLAG_NODISPLAY)
		options.treeflags |= GMENU_TREE_FLAGS_INCLUDE_NODISPLAY;
	else
		options.treeflags &= ~GMENU_TREE_FLAGS_INCLUDE_NODISPLAY;
	if (flags & XDE_MENU_FLAG_UNALLOCATED)
		options.treeflags |= GMENU_TREE_FLAGS_INCLUDE_UNALLOCATED;
	else
		options.treeflags &= ~GMENU_TREE_FLAGS_INCLUDE_UNALLOCATED;
	if (flags & XDE_MENU_FLAG_EMPTY)
		options.treeflags |= GMENU_TREE_FLAGS_SHOW_EMPTY;
	else
		options.treeflags &= ~GMENU_TREE_FLAGS_SHOW_EMPTY;
	if (flags & XDE_MENU_FLAG_SEPARATORS)
		options.treeflags |= GMENU_TREE_FLAGS_SHOW_ALL_SEPARATORS;
	else
		options.treeflags &= ~GMENU_TREE_FLAGS_SHOW_ALL_SEPARATORS;
	if (flags & XDE_MENU_FLAG_SORT)
		options.treeflags |= GMENU_TREE_FLAGS_SORT_DISPLAY_NAME;
	else
		options.treeflags &= ~GMENU_TREE_FLAGS_SORT_DISPLAY_NAME;
#endif
	options.button = (flags >> 16) & 0x0f;
	DPRINTF(1, "options.button = %d\n", options.button);
	options.which = (flags >> 20) & 0x0f;
	DPRINTF(1, "options.which = %s\n", show_which(options.which));
	options.screen = (flags >> 24) & 0x0f;
	DPRINTF(1, "options.screen = %d\n", options.screen);
	options.where = (flags >> 28) & 0x0f;
}

static void
set_word1(long word1)
{
	options.geom.mask &= ~(WidthValue|HeightValue);
	options.geom.w = (word1 >> 0) & 0x07fff;
	options.geom.mask |= ((word1 >> 15) & 0x01) ? WidthValue : 0;
	options.geom.h = (word1 >> 16) & 0x07fff;
	options.geom.mask |= ((word1 >> 31) & 0x01) ? HeightValue : 0;
}

static void
set_word2(long word2)
{
	options.geom.mask &= ~(XValue|YValue|XNegative|YNegative);
	options.geom.x = (word2 >> 0) & 0x03fff;
	options.geom.mask |= ((word2 >> 14) & 0x01) ? XValue : 0;
	options.geom.mask |= ((word2 >> 15) & 0x01) ? XNegative : 0;
	options.geom.y = (word2 >> 16) & 0x03fff;
	options.geom.mask |= ((word2 >> 30) & 0x01) ? YValue : 0;
	options.geom.mask |= ((word2 >> 31) & 0x01) ? YNegative : 0;
	DPRINTF(1, "options.where = %s\n", show_where(options.where));
}
#endif

/** @} */

/** @section Getting Arguments
  * @{ */

#if 0
static long
get_scmon(void)
{
	long scmon = 0;

	scmon |= ((long) (options.monitor & 0xffff) << 0);
	scmon |= ((long) (options.screen & 0xffff) << 16);
	DPRINTF(1, "options.monitor = %d\n", options.monitor);
	DPRINTF(1, "options.screen = %d\n", options.screen);
	return (scmon);
}

static long
get_flags(void)
{
	long flags = 0;

#if 1
	DPRINTF(1, "options.fileout = %s\n", show_bool(options.fileout));
	if (options.fileout)
		flags |= XDE_MENU_FLAG_FILEOUT;
	DPRINTF(1, "options.noicons = %s\n", show_bool(options.noicons));
	if (options.noicons)
		flags |= XDE_MENU_FLAG_NOICONS;
	DPRINTF(1, "options.launch = %s\n", show_bool(options.launch));
	if (options.launch)
		flags |= XDE_MENU_FLAG_LAUNCH;
#endif
	DPRINTF(1, "options.systray = %s\n", show_bool(options.systray));
	if (options.systray)
		flags |= XDE_MENU_FLAG_TRAY;
#if 1
	DPRINTF(1, "options.generate = %s\n", show_bool(options.generate));
	if (options.generate)
		flags |= XDE_MENU_FLAG_GENERATE;
#endif
	DPRINTF(1, "options.tooltips = %s\n", show_bool(options.tooltips));
	if (options.tooltips)
		flags |= XDE_MENU_FLAG_TOOLTIPS;
#if 1
	DPRINTF(1, "options.actions = %s\n", show_bool(options.actions));
	if (options.actions)
		flags |= XDE_MENU_FLAG_ACTIONS;
#endif
#if 0
	if (options.treeflags & GMENU_TREE_FLAGS_INCLUDE_EXCLUDED)
		flags |= XDE_MENU_FLAG_EXCLUDED;
	if (options.treeflags & GMENU_TREE_FLAGS_INCLUDE_NODISPLAY)
		flags |= XDE_MENU_FLAG_NODISPLAY;
	if (options.treeflags & GMENU_TREE_FLAGS_INCLUDE_UNALLOCATED)
		flags |= XDE_MENU_FLAG_UNALLOCATED;
	if (options.treeflags & GMENU_TREE_FLAGS_SHOW_EMPTY)
		flags |= XDE_MENU_FLAG_EMPTY;
	if (options.treeflags & GMENU_TREE_FLAGS_SHOW_ALL_SEPARATORS)
		flags |= XDE_MENU_FLAG_SEPARATORS;
	if (options.treeflags & GMENU_TREE_FLAGS_SORT_DISPLAY_NAME)
		flags |= XDE_MENU_FLAG_SORT;
#endif
	DPRINTF(1, "options.button = %d\n", options.button);
	flags |= ((long) (options.button & 0x0f) << 16);
	DPRINTF(1, "options.which = %s\n", show_which(options.which));
	flags |= ((long) (options.which & 0x0f) << 20);
	DPRINTF(1, "options.screen = %d\n", options.screen);
	flags |= ((long) (options.screen & 0x0f) << 24);
	DPRINTF(1, "options.where = %s\n", show_where(options.where));
	flags |= ((long) (options.where & 0x0f) << 28);
	return (flags);
}

static long
get_word1(void)
{
	long word1 = 0;

	word1 |= ((long) (options.geom.w & 0x07fff) << 0);
	word1 |= ((long) ((options.geom.mask & WidthValue) ? 1 : 0) << 15);
	word1 |= ((long) (options.geom.h & 0x07fff) << 16);
	word1 |= ((long) ((options.geom.mask & HeightValue) ? 1 : 0) << 31);
	return (word1);
}

static long
get_word2(void)
{
	long word2 = 0;

	word2 |= ((long) (options.geom.x & 0x03fff) << 0);
	word2 |= ((long) ((options.geom.mask & XValue) ? 1 : 0) << 14);
	word2 |= ((long) ((options.geom.mask & XNegative) ? 1 : 0) << 15);
	word2 |= ((long) (options.geom.y & 0x03fff) << 16);
	word2 |= ((long) ((options.geom.mask & YValue) ? 1 : 0) << 30);
	word2 |= ((long) ((options.geom.mask & YNegative) ? 1 : 0) << 31);
	return (word2);
}
#endif

/** @} */

GMainLoop *loop = NULL;

static void
mainloop(void)
{
	if (options.display)
		gtk_main();
	else
		g_main_loop_run(loop);
}

static void
mainloop_quit(void)
{
	if (options.display)
		gtk_main_quit();
	else
		g_main_loop_quit(loop);
}

/** @} */

/** @section Background Theme Handling
  * @{ */

#if 0
void
xde_pixmap_ref(XdePixmap *pixmap)
{
	if (pixmap)
		pixmap->refs++;
}

void
xde_pixmap_delete(XdePixmap *pixmap)
{
	if (pixmap) {
		if (pixmap->pprev) {
			if ((*(pixmap->pprev) = pixmap->next))
				pixmap->next->pprev = pixmap->pprev;
		}
		free(pixmap);
	}
}

void
xde_pixmap_unref(XdePixmap **pixmapp)
{
	if (pixmapp && *pixmapp) {
		if (--(*pixmapp)->refs <= 0) {
			xde_pixmap_delete(*pixmapp);
			*pixmapp = NULL;
		}
	}
}

void
xde_image_ref(XdeImage *image)
{
	if (image)
		image->refs++;
}

void
xde_image_delete(XdeImage *image)
{
	XdePixmap *pixmap;

	while ((pixmap = image->pixmaps))
		xde_pixmap_delete(pixmap);
	if (image->file)
		free(image->file);
	if (image->pixbuf)
		g_object_unref(image->pixbuf);
	free(image);
}

void
xde_image_unref(XdeImage **imagep)
{
	if (imagep && *imagep) {
		(*imagep)->refs -= 1;
		if ((*imagep)->refs <= 0) {
			xde_image_delete(*imagep);
			*imagep = NULL;
		} else
			DPRINTF(1, "There are %d refs left for %p\n", (*imagep)->refs, *imagep);
	}
}

static char **
get_data_dirs(int *np)
{
	char *home, *xhome, *xdata, *dirs, *pos, *end, **xdg_dirs;
	int len, n;

	home = getenv("HOME") ? : ".";
	xhome = getenv("XDG_DATA_HOME");
	xdata = getenv("XDG_DATA_DIRS") ? : "/usr/local/share:/usr/share";

	len = (xhome ? strlen(xhome) : strlen(home) + strlen("/.local/share")) + strlen(xdata) + 2;
	dirs = calloc(len, sizeof(*dirs));
	if (xhome)
		strcpy(dirs, xhome);
	else {
		strcpy(dirs, home);
		strcat(dirs, "/.local/share");
	}
	strcat(dirs, ":");
	strcat(dirs, xdata);
	end = dirs + strlen(dirs);
	for (n = 0, pos = dirs; pos < end; n++, *strchrnul(pos, ':') = '\0', pos += strlen(pos) + 1) ;
	xdg_dirs = calloc(n + 1, sizeof(*xdg_dirs));
	for (n = 0, pos = dirs; pos < end; n++, pos += strlen(pos) + 1)
		xdg_dirs[n] = strdup(pos);
	free(dirs);
	if (np)
		*np = n;
	return (xdg_dirs);
}

static char *
find_theme_file(XdeScreen *xscr)
{
	char *buf, *file = NULL;
	char **xdg_dirs, **dirs;
	int i, n = 0;

	if (!xscr->theme)
		return (file);

	if (!(xdg_dirs = get_data_dirs(&n)) || !n)
		return (file);

	buf = calloc(PATH_MAX + 1, sizeof(*buf));

	for (i = 0, dirs = &xdg_dirs[i]; i < n; i++, dirs++) {
		strncpy(buf, *dirs, PATH_MAX);
		strncat(buf, "/themes/", PATH_MAX);
		strncat(buf, xscr->theme, PATH_MAX);
		strncat(buf, "/xde/theme.ini", PATH_MAX);

		if (!access(buf, R_OK)) {
			file = strdup(buf);
			break;
		}
	}

	free(buf);

	for (i = 0; i < n; i++)
		free(xdg_dirs[i]);
	free(xdg_dirs);

	return (file);
}

char *
find_image_file(char *name, int dirc, char *dirv[])
{
	char *buf, *file = NULL;
	char **xdg_dirs, **dirs, **d;
	int i, j, n = 0;

	if (!name)
		return (file);

	if (!(xdg_dirs = get_data_dirs(&n)) || !n)
		return (file);

	buf = calloc(PATH_MAX + 1, sizeof(*buf));

	for (j = 0, d = &dirv[j]; j < dirc; j++, d++) {
		for (i = 0, dirs = &xdg_dirs[i]; i < n; i++, dirs++) {
			strncpy(buf, *dirs, PATH_MAX);
			strncat(buf, *d, PATH_MAX);
			strncat(buf, "/", PATH_MAX);
			strncat(buf, name, PATH_MAX);

			if (!access(buf, R_OK)) {
				file = strdup(buf);
				break;
			}
		}
		if (file)
			break;
	}

	free(buf);

	return (file);
}

static void
set_workspaces(XdeScreen *xscr, gint count)
{
	Window root = RootWindow(dpy, xscr->index);
	XEvent ev;

	DPRINTF(1, "Setting the workspace count to %d\n", count);
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = False;
	ev.xclient.display = dpy;
	ev.xclient.window = root;
	ev.xclient.message_type = _XA_NET_NUMBER_OF_DESKTOPS;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = count;
	ev.xclient.data.l[1] = 0;
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = 0;

	XSendEvent(dpy, root, False, StructureNotifyMask | SubstructureNotifyMask | SubstructureRedirectMask, &ev);
	XFlush(dpy);
}

#define _NET_WM_ORIENTATION_HORZ	0
#define _NET_WM_ORIENTATION_VERT	1

#define _NET_WM_TOPLEFT		0
#define _NET_WM_TOPRIGHT	1
#define _NET_WM_BOTTOMRIGHT	2
#define _NET_WM_BOTTOMLEFT	3

Window get_desktop_layout_selection(XdeScreen *xscr);

static void
set_workspace_layout(XdeScreen *xscr, gint *array, gsize num)
{
	Window root = RootWindow(dpy, xscr->index);
	long data[4] = { 0, };

	if (!array)
		num = 0;

	if (options.debug) {
		gsize i;

		DPRINTF(1, "Setting the workspace layout to: ");
		for (i = 0; i < num; i++)
			fprintf(stderr, ";%d", array[i]);
		fprintf(stderr, ";\n");
	}
	switch (num) {
	default:
		EPRINTF("wrong number of integers\n");
		break;
	case 0:
		break;
	case 1:
		/* assume it is just rows */
		data[0] = _NET_WM_ORIENTATION_HORZ;
		data[1] = 0;
		data[2] = array[0];
		data[3] = _NET_WM_TOPLEFT;
		break;
	case 2:
		/* assume it is columns and rows */
		data[0] = _NET_WM_ORIENTATION_HORZ;
		data[1] = array[0];
		data[2] = array[1];
		data[3] = _NET_WM_TOPLEFT;
		break;
	case 3:
		/* assume it is orientation, columns and rows */
		data[0] = array[0];
		data[1] = array[1];
		data[2] = array[2];
		data[3] = _NET_WM_TOPLEFT;
		break;
	case 4:
		/* assume it is fully specified */
		data[0] = array[0];
		data[1] = array[1];
		data[2] = array[2];
		data[3] = array[3];
		break;
	}

	if (!xscr->laywin)
		get_desktop_layout_selection(xscr);
	if (num)
		XChangeProperty(dpy, root, _XA_NET_DESKTOP_LAYOUT, XA_CARDINAL, 32, PropModeReplace,
				(unsigned char *) data, 4);
	XFlush(dpy);
}

static void
set_workspace_names(XdeScreen *xscr, gchar **names, gsize num)
{
	Window root = RootWindow(dpy, xscr->index);
	gchar **alloc = NULL;
	XTextProperty xtp;

	if (!names)
		num = 0;

	if (num == 1) {
		gchar *p, *q, *str;

		str = names[0];
		for (q = str, num = 0; *q; num++, p = strchrnul(q, ','), q = *p ? p + 1 : p) ;
		alloc = names = calloc(num, sizeof(*names));
		for (q = str, num = 0; *q;
		     names[num] = q, num++, p = strchrnul(q, ','), q = *p ? p + 1 : p, *p = '\0') ;
	}
	if (options.debug) {
		gsize i;

		DPRINTF(1, "Setting the workspace names to: ");
		for (i = 0; i < num; i++)
			fprintf(stderr, ";%s", names[i]);
		fprintf(stderr, ";\n");
	}
	Xutf8TextListToTextProperty(dpy, names, num, XUTF8StringStyle, &xtp);
	XSetTextProperty(dpy, root, &xtp, _XA_NET_DESKTOP_NAMES);
	free(alloc);
}

static void
set_workspace_images(XdeScreen *xscr, gchar **images, gsize num, gboolean center, gboolean scaled,
		     gboolean tiled, gboolean full)
{
	char *file;
	char *dirv[] = { "/images" };
	int dirc = 1, n;
	gsize i;
	GdkPixbuf *pb;
	XdeImage *im;

	if (!images)
		num = 0;
	if (options.debug) {
		DPRINTF(1, "Setting the workspace images to: ");
		for (i = 0; i < num; i++)
			fprintf(stderr, ";%s", images[i]);
		fprintf(stderr, ";\n");
	}
	DPRINTF(1, "There are %d images\n", xscr->nimg);
	for (n = 0; n < xscr->nimg; n++) {
		DPRINTF(1, "Attempting to unref image %d (sources is %p)\n", n, xscr->sources + n);
		xde_image_unref(xscr->sources + n);
	}
	DPRINTF(1, "Reallocating %d sources\n", (int) num);
	xscr->sources = realloc(xscr->sources, num * sizeof(*xscr->sources));
	memset(xscr->sources, 0, num * sizeof(*xscr->sources));
	for (i = 0, n = 0; i < num; i++) {
		if ((file = find_image_file(images[i], dirc, dirv))) {
			DPRINTF(1, "Found file for %s: %s\n", images[i], file);
			if ((pb = gdk_pixbuf_new_from_file(file, NULL))) {
				/* save resident memory, get it again when we need it */
				g_object_unref(pb);
				pb = NULL;
				im = calloc(1, sizeof(*im));
				im->refs = 1;
				im->index = n;
				im->file = file;
				im->pixbuf = pb;
				xscr->sources[n] = im;
				n++;
			} else {
				DPRINTF(1, "Could not read file %s\n", file);
				free(file);
			}
		} else
			DPRINTF(1, "Could not find file for %s\n", images[i]);
	}
	xscr->nimg = n;
	add_deferred_refresh_layout(xscr);
}

static void
set_workspace_image(XdeScreen *xscr, gchar *image, gboolean center, gboolean scaled, gboolean tiled, gboolean full)
{
	DPRINTF(1, "Setting workspace image to: %s\n", image);
}

static void
set_workspace_color(XdeScreen *xscf, gchar *color)
{
	DPRINTF(1, "Setting workspace color to: %s\n", color);
}

static void
read_theme(XdeScreen *xscr)
{
	GKeyFile *entry = NULL;
	char *file;
	gint count, *layout;
	gchar **list, **images, *image;
	gsize len;
	gboolean center, scaled, tiled, full;
	gchar *color;

	if (!(file = find_theme_file(xscr)))
		return;

	if (!(entry = g_key_file_new())) {
		EPRINTF("%s: could not allocate key file\n", file);
		return;
	}
	if (!g_key_file_load_from_file(entry, file, G_KEY_FILE_NONE, NULL)) {
		EPRINTF("%s: could not load keyfile\n", file);
		g_key_file_unref(entry);
		return;
	}
	if (!g_key_file_has_group(entry, "Theme")) {
		EPRINTF("%s: has no [%s] section\n", file, "Theme");
		g_key_file_free(entry);
		return;
	}
	if (!g_key_file_has_key(entry, "Theme", "Name", NULL)) {
		EPRINTF("%s: has no %s= entry\n", file, "Name");
		g_key_file_free(entry);
		return;
	}
	if (xscr->entry) {
		g_key_file_free(xscr->entry);
		xscr->entry = NULL;
	}
	xscr->entry = entry;
	DPRINTF(1, "got theme file: %s (%s)\n", xscr->theme, file);

	if (!(count = g_key_file_get_integer(entry, xscr->wmname, "Workspaces", NULL)))
		count = g_key_file_get_integer(entry, "Theme", "Workskspaces", NULL);
	if (1 <= count && count <= 64)
		set_workspaces(xscr, count);
	else
		count = xscr->desks;

	if (!(layout = g_key_file_get_integer_list(entry, xscr->wmname, "WorkspaceLayout", &len, NULL)))
		layout = g_key_file_get_integer_list(entry, "Theme", "WorkspaceLayout", &len, NULL);
	if (layout && len >= 3)
		set_workspace_layout(xscr, layout, len);
	if (layout)
		g_free(layout);

	if (!(list = g_key_file_get_string_list(entry, xscr->wmname, "WorkspaceNames", &len, NULL)))
		list = g_key_file_get_string_list(entry, "Theme", "WorkspaceNames", &len, NULL);
	if (len)
		set_workspace_names(xscr, list, len);
	if (list)
		g_strfreev(list);

	if (!(center = g_key_file_get_boolean(entry, xscr->wmname, "WorkspaceCenter", NULL)))
		center = g_key_file_get_boolean(entry, "Theme", "WorkspaceCenter", NULL);
	if (!(scaled = g_key_file_get_boolean(entry, xscr->wmname, "WorkspaceScaled", NULL)))
		scaled = g_key_file_get_boolean(entry, "Theme", "WorkspaceScaled", NULL);
	if (!(tiled = g_key_file_get_boolean(entry, xscr->wmname, "WorkspaceTiled", NULL)))
		tiled = g_key_file_get_boolean(entry, "Theme", "WorkspaceTiled", NULL);
	if (!(full = g_key_file_get_boolean(entry, xscr->wmname, "WorkspaceFull", NULL)))
		full = g_key_file_get_boolean(entry, "Theme", "WorkspaceFull", NULL);

	if (!(color = g_key_file_get_string(entry, xscr->wmname, "WorkspaceColor", NULL)))
		color = g_key_file_get_string(entry, "Theme", "WorkspaceColor", NULL);

	if (!(image = g_key_file_get_string(entry, xscr->wmname, "WorkspaceImage", NULL)))
		image = g_key_file_get_string(entry, "Theme", "WorkspaceImage", NULL);

	if (!(images = g_key_file_get_string_list(entry, xscr->wmname, "WorkspaceImages", &len, NULL)))
		images = g_key_file_get_string_list(entry, "Theme", "WorkspaceImages", &len, NULL);
	if (!images) {
		char buf[64] = { 0, };

		images = calloc(64, sizeof(*images));
		for (len = 0; len < 64; len++) {
			snprintf(buf, sizeof(buf), "Workspace%dImage", (int) len);
			if (!(images[len] = g_key_file_get_string(entry, xscr->wmname, buf, NULL)))
				break;
		}
		if (len == 0) {
			for (len = 0; len < 64; len++) {
				snprintf(buf, sizeof(buf), "Workspace%dImage", (int) len);
				if (!(images[len] = g_key_file_get_string(entry, "Theme", buf, NULL)))
					break;
			}
		}
		if (len == 0) {
			free(images);
			images = NULL;
		}
	}
	set_workspace_images(xscr, images, len, center, scaled, tiled, full);
	if (images)
		g_strfreev(images);
	if (!images || !len)
		set_workspace_image(xscr, image, center, scaled, tiled, full);
	if (image)
		g_free(image);
	if (color) {
		set_workspace_color(xscr, color);
		g_free(color);
	}
}

static Pixmap
get_temporary_pixmap(XdeScreen *xscr)
{
#if 0
	Display *dpy;
#endif
	Pixmap pmap;
	int s;

#if 0
	if (!(dpy = XOpenDisplay(NULL))) {
		DPRINTF(1, "cannot open display %s\n", getenv("DISPLAY"));
		return (None);
	}
	XSetCloseDownMode(dpy, RetainTemporary);
#endif
	s = xscr->index;
	pmap = XCreatePixmap(dpy, RootWindow(dpy, s), xscr->width, xscr->height, DefaultDepth(dpy, s));
#if 0
	XFlush(dpy);
	XSync(dpy, True);
	XCloseDisplay(dpy);
#endif
	return (pmap);
}

gboolean
test_icon_ext(const char *icon)
{
	const char *p;

	if ((p = strrchr(icon, '.'))) {
		if (strcmp(p, ".xpm") == 0)
			return TRUE;
		if (strcmp(p, ".png") == 0)
			return TRUE;
		if (strcmp(p, ".svg") == 0)
			return TRUE;
		if (strcmp(p, ".jpg") == 0 || strcmp(p, ".jpeg") == 0)
			return TRUE;
		if (strcmp(p, ".gif") == 0)
			return TRUE;
		if (strcmp(p, ".tif") == 0 || strcmp(p, ".tiff") == 0)
			return TRUE;
	}
	return FALSE;
}

GdkPixbuf *
get_icons(GIcon *gicon, const char *const *inames)
{
	GtkIconTheme *theme;
	GtkIconInfo *info;
	const char *const *iname;

	if ((theme = gtk_icon_theme_get_default())) {
		if (gicon && (info = gtk_icon_theme_lookup_by_gicon(theme, gicon, options.iconsize,
								    GTK_ICON_LOOKUP_USE_BUILTIN |
								    GTK_ICON_LOOKUP_GENERIC_FALLBACK |
								    GTK_ICON_LOOKUP_FORCE_SIZE))) {
			if (gtk_icon_info_get_filename(info))
				return gtk_icon_info_load_icon(info, NULL);
			return gtk_icon_info_get_builtin_pixbuf(info);
		}
		for (iname = inames; iname && *iname; iname++) {
			DPRINTF(2, "Testing for icon name: %s\n", *iname);
			if ((info = gtk_icon_theme_lookup_icon(theme, *iname, options.iconsize,
							       GTK_ICON_LOOKUP_USE_BUILTIN |
							       GTK_ICON_LOOKUP_GENERIC_FALLBACK |
							       GTK_ICON_LOOKUP_FORCE_SIZE))) {
				if (gtk_icon_info_get_filename(info))
					return gtk_icon_info_load_icon(info, NULL);
				return gtk_icon_info_get_builtin_pixbuf(info);
			}
		}
	}
	return (NULL);
}

#ifdef STARTUP_NOTIFICATION
GdkPixbuf *
get_sequence_pixbuf(Sequence *seq)
{
	GdkPixbuf *pixbuf = NULL;
	GFile *file;
	GIcon *gicon = NULL;
	char **inames;
	char *icon, *base, *p;
	const char *name;
	int i = 0;

	inames = calloc(16, sizeof(*inames));

	/* FIXME: look up entry file too */
	if ((name = sn_startup_sequence_get_icon_name(seq->seq))) {
		icon = strdup(name);
		if (icon[0] == '/' && !access(icon, R_OK) && test_icon_ext(icon)) {
			DPRINTF(2, "going with full icon path %s\n", icon);
			if ((file = g_file_new_for_path(icon)))
				gicon = g_file_icon_new(file);
		}
		base = icon;
		*strchrnul(base, ' ') = '\0';
		if ((p = strrchr(base, '/')))
			base = p + 1;
		if ((p = strrchr(base, '.')))
			*p = '\0';
		inames[i++] = strdup(base);
		DPRINTF(2, "Choice %d for icon name: %s\n", i, base);
		free(icon);
	} else {
		/* FIXME: look up entry file too */
		/* try both mixed- and lower-case WMCLASS= */
		if ((name = sn_startup_sequence_get_wmclass(seq->seq))) {
			base = strdup(name);
			inames[i++] = base;
			DPRINTF(2, "Choice %d for icon name: %s\n", i, base);
			base = strdup(name);
			for (p = base; *p; p++)
				*p = tolower(*p);
			inames[i++] = base;
			DPRINTF(2, "Choice %d for icon name: %s\n", i, base);
		}
		/* FIXME: look up entry file too */
		/* try BIN= or LAUNCHEE in its absence COMMAND= */
		if ((name = sn_startup_sequence_get_binary_name(seq->seq)) || (name = seq->launchee)) {
			icon = strdup(name);
			base = icon;
			*strchrnul(base, ' ') = '\0';
			if ((p = strrchr(base, '/')))
				base = p + 1;
			if ((p = strrchr(base, '.')))
				*p = '\0';
			inames[i++] = strdup(base);
			DPRINTF(2, "Choice %d for icon name: %s\n", i, base);
			free(icon);
		}
	}
	base = strdup("unknown");
	inames[i++] = base;
	DPRINTF(2, "Choice %d for icon name: %s\n", i, base);
	pixbuf = get_icons(gicon, (const char *const *) inames);
	g_strfreev((gchar **) inames);
	if (gicon)
		g_object_unref(G_OBJECT(gicon));
	return (pixbuf);
}
#endif				/* STARTUP_NOTIFICATION */

#endif

/** @} */

/** @section Popup Window Event Handlers
  * @{ */

#if 0
static gboolean
stop_popup_timer(XdePopup *xpop)
{
	PTRACE(5);
	if (xpop->timer) {
		DPRINTF(1, "stopping popup timer\n");
		g_source_remove(xpop->timer);
		xpop->timer = 0;
		return TRUE;
	}
	return FALSE;
}

static void
release_grabs(XdePopup *xpop)
{
	PTRACE(5);
	if (xpop->pointer) {
#if 0
		/* will be broken when window unmaps */
		DPRINTF(1, "ungrabbing pointer\n");
		gdk_display_pointer_ungrab(disp, GDK_CURRENT_TIME);
#endif
		xpop->pointer = False;
	}
	if (xpop->keyboard) {
#if 0
		/* will be broken when window unmaps */
		DPRINTF(1, "ungrabbing keyboard\n");
		gdk_display_keyboard_ungrab(disp, GDK_CURRENT_TIME);
#endif
		xpop->keyboard = False;
	}
}

static void
drop_popup(XdePopup *xpop)
{
	PTRACE(5);
	if (xpop->popped) {
		stop_popup_timer(xpop);
		release_grabs(xpop);
		gtk_widget_hide(xpop->popup);
		xpop->popped = False;
	}
}

static gboolean
workspace_timeout(gpointer user)
{
	XdePopup *xpop = user;

	DPRINTF(1, "popup timeout!\n");
	drop_popup(xpop);
	xpop->timer = 0;
	return G_SOURCE_REMOVE;
}

static gboolean
start_popup_timer(XdePopup *xpop)
{
	PTRACE(5);
	if (xpop->timer)
		return FALSE;
	DPRINTF(1, "starting popup timer\n");
	xpop->timer = g_timeout_add(options.timeout, workspace_timeout, xpop);
	return TRUE;
}

void
restart_popup_timer(XdePopup *xpop)
{
	DPRINTF(1, "restarting popup timer\n");
	stop_popup_timer(xpop);
	start_popup_timer(xpop);
}

#if 1
static void
show_popup(XdeScreen *xscr, XdePopup *xpop, gboolean grab_p, gboolean grab_k)
{
	GdkGrabStatus status;
	// Window win;

	if (!xpop->popup)
		return;
	DPRINTF(1, "popping the window\n");
	gdk_display_get_pointer(disp, NULL, NULL, NULL, &xpop->mask);
	stop_popup_timer(xpop);
	if (xpop->popped) {
#if 0
		gtk_window_reshow_with_initial_size(GTK_WINDOW(xpop->popup));
#endif
	} else {
		if (xpop->type == PopupStart) {
			// gtk_window_set_default_size(GTK_WINDOW(xpop->popup), -1, -1);
			gtk_widget_set_size_request(GTK_WIDGET(xpop->popup), -1, -1);
		}
		if (xpop->type == PopupTasks) {
			GtkWidget *tasks = xpop->content;
			GList *children;
			int n;

			children = gtk_container_get_children(GTK_CONTAINER(tasks));
			n = g_list_length(children);
			g_list_free(children);

			// gtk_window_set_default_size(GTK_WINDOW(xpop->popup), 300, n * 24);
			gtk_widget_set_size_request(GTK_WIDGET(xpop->popup), 300, n * 24);
		}
		gtk_window_set_screen(GTK_WINDOW(xpop->popup), gdk_display_get_screen(disp, xscr->index));
		gtk_window_set_position(GTK_WINDOW(xpop->popup), GTK_WIN_POS_CENTER_ALWAYS);
		gtk_window_present(GTK_WINDOW(xpop->popup));
		gtk_widget_show_now(GTK_WIDGET(xpop->popup));
		xpop->popped = True;
	}
	// win = GDK_WINDOW_XID(xpop->popup->window);

	if (grab_p && !xpop->pointer) {
		GdkEventMask mask =
		    GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
		// XSetInputFocus(dpy, win, RevertToPointerRoot, options.timestamp);
		status = gdk_pointer_grab(xpop->popup->window, TRUE, mask, NULL, NULL, GDK_CURRENT_TIME);
		switch (status) {
		case GDK_GRAB_SUCCESS:
			DPRINTF(1, "pointer grabbed\n");
			// xpop->pointer = True;
			break;
		case GDK_GRAB_ALREADY_GRABBED:
			DPRINTF(1, "pointer already grabbed\n");
			break;
		case GDK_GRAB_INVALID_TIME:
			EPRINTF("pointer grab invalid time\n");
			break;
		case GDK_GRAB_NOT_VIEWABLE:
			EPRINTF("pointer grab on unviewable window\n");
			break;
		case GDK_GRAB_FROZEN:
			EPRINTF("pointer grab on frozen pointer\n");
			break;
		}
	}
	if (grab_k && !xpop->keyboard) {
		// XSetInputFocus(dpy, win, RevertToPointerRoot, options.timestamp);
		status = gdk_keyboard_grab(xpop->popup->window, TRUE, GDK_CURRENT_TIME);
		switch (status) {
		case GDK_GRAB_SUCCESS:
			DPRINTF(1, "keyboard grabbed\n");
			// xpop->keyboard = True;
			break;
		case GDK_GRAB_ALREADY_GRABBED:
			DPRINTF(1, "keyboard already grabbed\n");
			break;
		case GDK_GRAB_INVALID_TIME:
			EPRINTF("keyboard grab invalid time\n");
			break;
		case GDK_GRAB_NOT_VIEWABLE:
			EPRINTF("keyboard grab on unviewable window\n");
			break;
		case GDK_GRAB_FROZEN:
			EPRINTF("keyboard grab on frozen keyboard\n");
			break;
		}
	}
	// if (!xpop->keyboard || !xpop->pointer)
	if (!(xpop->mask & ~(GDK_LOCK_MASK | GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
		if (!xpop->inside)
			restart_popup_timer(xpop);
}

/** @section Popup Window GDK Events
  * @{ */

#define PASSED_EVENT_MASK (KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|SubstructureNotifyMask|SubstructureRedirectMask)

static GdkFilterReturn
event_handler_KeyPress(Display *dpy, XEvent *xev, XdePopup *xpop)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> KeyPress: %p\n", xpop);
		fprintf(stderr, "    --> send_event = %s\n", xev->xkey.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xkey.window);
		fprintf(stderr, "    --> root = 0x%lx\n", xev->xkey.root);
		fprintf(stderr, "    --> subwindow = 0x%lx\n", xev->xkey.subwindow);
		fprintf(stderr, "    --> time = %lu\n", xev->xkey.time);
		fprintf(stderr, "    --> x = %d\n", xev->xkey.x);
		fprintf(stderr, "    --> y = %d\n", xev->xkey.y);
		fprintf(stderr, "    --> x_root = %d\n", xev->xkey.x_root);
		fprintf(stderr, "    --> y_root = %d\n", xev->xkey.y_root);
		fprintf(stderr, "    --> state = 0x%08x\n", xev->xkey.state);
		fprintf(stderr, "    --> keycode = %u\n", xev->xkey.keycode);
		fprintf(stderr, "    --> same_screen = %s\n", xev->xkey.same_screen ? "true" : "false");
		fprintf(stderr, "<== KeyPress: %p\n", xpop);
	}
	if (!xev->xkey.send_event) {
		XEvent ev = *xev;

		restart_popup_timer(xpop);
		ev.xkey.window = ev.xkey.root;
		XSendEvent(dpy, ev.xkey.root, True, PASSED_EVENT_MASK, &ev);
		XFlush(dpy);
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
}

static GdkFilterReturn
event_handler_KeyRelease(Display *dpy, XEvent *xev, XdePopup *xpop)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> KeyRelease: %p\n", xpop);
		fprintf(stderr, "    --> send_event = %s\n", xev->xkey.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xkey.window);
		fprintf(stderr, "    --> root = 0x%lx\n", xev->xkey.root);
		fprintf(stderr, "    --> subwindow = 0x%lx\n", xev->xkey.subwindow);
		fprintf(stderr, "    --> time = %lu\n", xev->xkey.time);
		fprintf(stderr, "    --> x = %d\n", xev->xkey.x);
		fprintf(stderr, "    --> y = %d\n", xev->xkey.y);
		fprintf(stderr, "    --> x_root = %d\n", xev->xkey.x_root);
		fprintf(stderr, "    --> y_root = %d\n", xev->xkey.y_root);
		fprintf(stderr, "    --> state = 0x%08x\n", xev->xkey.state);
		fprintf(stderr, "    --> keycode = %u\n", xev->xkey.keycode);
		fprintf(stderr, "    --> same_screen = %s\n", xev->xkey.same_screen ? "true" : "false");
		fprintf(stderr, "<== KeyRelease: %p\n", xpop);
	}
	if (!xev->xkey.send_event) {
		XEvent ev = *xev;

		// restart_popup_timer(xpop);
		ev.xkey.window = ev.xkey.root;
		XSendEvent(dpy, ev.xkey.root, True, PASSED_EVENT_MASK, &ev);
		XFlush(dpy);
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
}

static GdkFilterReturn
event_handler_ButtonPress(Display *dpy, XEvent *xev, XdePopup *xpop)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> ButtonPress: %p\n", xpop);
		fprintf(stderr, "    --> send_event = %s\n", xev->xbutton.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xbutton.window);
		fprintf(stderr, "    --> root = 0x%lx\n", xev->xbutton.root);
		fprintf(stderr, "    --> subwindow = 0x%lx\n", xev->xbutton.subwindow);
		fprintf(stderr, "    --> time = %lu\n", xev->xbutton.time);
		fprintf(stderr, "    --> x = %d\n", xev->xbutton.x);
		fprintf(stderr, "    --> y = %d\n", xev->xbutton.y);
		fprintf(stderr, "    --> x_root = %d\n", xev->xbutton.x_root);
		fprintf(stderr, "    --> y_root = %d\n", xev->xbutton.y_root);
		fprintf(stderr, "    --> state = 0x%08x\n", xev->xbutton.state);
		fprintf(stderr, "    --> button = %u\n", xev->xbutton.button);
		fprintf(stderr, "    --> same_screen = %s\n", xev->xbutton.same_screen ? "true" : "false");
		fprintf(stderr, "<== ButtonPress: %p\n", xpop);
	}
	if (!xev->xbutton.send_event) {
		XEvent ev = *xev;

		if (ev.xbutton.button == 4 || ev.xbutton.button == 5) {
			if (!xpop->inside)
				restart_popup_timer(xpop);
			DPRINTF(1, "ButtonPress = %d passing to root window\n", ev.xbutton.button);
			ev.xbutton.window = ev.xbutton.root;
			XSendEvent(dpy, ev.xbutton.root, True, PASSED_EVENT_MASK, &ev);
			XFlush(dpy);
		}
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
}

static GdkFilterReturn
event_handler_ButtonRelease(Display *dpy, XEvent *xev, XdePopup *xpop)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> ButtonRelease: %p\n", xpop);
		fprintf(stderr, "    --> send_event = %s\n", xev->xbutton.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xbutton.window);
		fprintf(stderr, "    --> root = 0x%lx\n", xev->xbutton.root);
		fprintf(stderr, "    --> subwindow = 0x%lx\n", xev->xbutton.subwindow);
		fprintf(stderr, "    --> time = %lu\n", xev->xbutton.time);
		fprintf(stderr, "    --> x = %d\n", xev->xbutton.x);
		fprintf(stderr, "    --> y = %d\n", xev->xbutton.y);
		fprintf(stderr, "    --> x_root = %d\n", xev->xbutton.x_root);
		fprintf(stderr, "    --> y_root = %d\n", xev->xbutton.y_root);
		fprintf(stderr, "    --> state = 0x%08x\n", xev->xbutton.state);
		fprintf(stderr, "    --> button = %u\n", xev->xbutton.button);
		fprintf(stderr, "    --> same_screen = %s\n", xev->xbutton.same_screen ? "true" : "false");
		fprintf(stderr, "<== ButtonRelease: %p\n", xpop);
	}
	if (!xev->xbutton.send_event) {
		XEvent ev = *xev;

		if (ev.xbutton.button == 4 || ev.xbutton.button == 5) {
			// restart_popup_timer(xpop);
			DPRINTF(1, "ButtonRelease = %d passing to root window\n", ev.xbutton.button);
			ev.xbutton.window = ev.xbutton.root;
			XSendEvent(dpy, ev.xbutton.root, True, PASSED_EVENT_MASK, &ev);
			XFlush(dpy);
		}
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
}

static const char *
show_mode(int mode)
{
	PTRACE(5);
	switch (mode) {
	case NotifyNormal:
		return ("NotifyNormal");
	case NotifyGrab:
		return ("NotifyGrab");
	case NotifyUngrab:
		return ("NotifyUngrab");
	case NotifyWhileGrabbed:
		return ("NotifyWhileGrabbed");
	}
	return NULL;
}

static const char *
show_detail(int detail)
{
	PTRACE(5);
	switch (detail) {
	case NotifyAncestor:
		return ("NotifyAncestor");
	case NotifyVirtual:
		return ("NotifyVirtual");
	case NotifyInferior:
		return ("NotifyInferior");
	case NotifyNonlinear:
		return ("NotifyNonlinear");
	case NotifyNonlinearVirtual:
		return ("NotifyNonlinearVirtual");
	case NotifyPointer:
		return ("NotifyPointer");
	case NotifyPointerRoot:
		return ("NotifyPointerRoot");
	case NotifyDetailNone:
		return ("NotifyDetailNone");
	}
	return NULL;
}

static GdkFilterReturn
event_handler_EnterNotify(Display *dpy, XEvent *xev, XdePopup *xpop)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> EnterNotify: %p\n", xpop);
		fprintf(stderr, "    --> send_event = %s\n", xev->xcrossing.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xcrossing.window);
		fprintf(stderr, "    --> root = 0x%lx\n", xev->xcrossing.root);
		fprintf(stderr, "    --> subwindow = 0x%lx\n", xev->xcrossing.subwindow);
		fprintf(stderr, "    --> time = %lu\n", xev->xcrossing.time);
		fprintf(stderr, "    --> x = %d\n", xev->xcrossing.x);
		fprintf(stderr, "    --> y = %d\n", xev->xcrossing.y);
		fprintf(stderr, "    --> x_root = %d\n", xev->xcrossing.x_root);
		fprintf(stderr, "    --> y_root = %d\n", xev->xcrossing.y_root);
		fprintf(stderr, "    --> mode = %s\n", show_mode(xev->xcrossing.mode));
		fprintf(stderr, "    --> detail = %s\n", show_detail(xev->xcrossing.detail));
		fprintf(stderr, "    --> same_screen = %s\n", xev->xcrossing.same_screen ? "true" : "false");
		fprintf(stderr, "    --> focus = %s\n", xev->xcrossing.focus ? "true" : "false");
		fprintf(stderr, "    --> state = 0x%08x\n", xev->xcrossing.state);
		fprintf(stderr, "<== EnterNotify: %p\n", xpop);
	}
	if (xev->xcrossing.mode == NotifyNormal) {
		if (!xpop->inside) {
			DPRINTF(1, "entered popup\n");
			stop_popup_timer(xpop);
			xpop->inside = True;
		}
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_LeaveNotify(Display *dpy, XEvent *xev, XdePopup *xpop)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> LeaveNotify: %p\n", xpop);
		fprintf(stderr, "    --> send_event = %s\n", xev->xcrossing.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xcrossing.window);
		fprintf(stderr, "    --> root = 0x%lx\n", xev->xcrossing.root);
		fprintf(stderr, "    --> subwindow = 0x%lx\n", xev->xcrossing.subwindow);
		fprintf(stderr, "    --> time = %lu\n", xev->xcrossing.time);
		fprintf(stderr, "    --> x = %d\n", xev->xcrossing.x);
		fprintf(stderr, "    --> y = %d\n", xev->xcrossing.y);
		fprintf(stderr, "    --> x_root = %d\n", xev->xcrossing.x_root);
		fprintf(stderr, "    --> y_root = %d\n", xev->xcrossing.y_root);
		fprintf(stderr, "    --> mode = %s\n", show_mode(xev->xcrossing.mode));
		fprintf(stderr, "    --> detail = %s\n", show_detail(xev->xcrossing.detail));
		fprintf(stderr, "    --> same_screen = %s\n", xev->xcrossing.same_screen ? "true" : "false");
		fprintf(stderr, "    --> focus = %s\n", xev->xcrossing.focus ? "true" : "false");
		fprintf(stderr, "    --> state = 0x%08x\n", xev->xcrossing.state);
		fprintf(stderr, "<== LeaveNotify: %p\n", xpop);
	}
	if (xev->xcrossing.mode == NotifyNormal) {
		if (xpop->inside) {
			DPRINTF(1, "left popup\n");
			restart_popup_timer(xpop);
			xpop->inside = False;
		}
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_FocusIn(Display *dpy, XEvent *xev, XdePopup *xpop)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> FocusIn: %p\n", xpop);
		fprintf(stderr, "    --> send_event = %s\n", xev->xfocus.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xfocus.window);
		fprintf(stderr, "    --> mode = %s\n", show_mode(xev->xfocus.mode));
		fprintf(stderr, "    --> detail = %s\n", show_detail(xev->xfocus.detail));
		fprintf(stderr, "<== FocusIn: %p\n", xpop);
	}
	switch (xev->xfocus.mode) {
	case NotifyNormal:
	case NotifyUngrab:
		DPRINTF(1, "focused popup\n");
		break;
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_FocusOut(Display *dpy, XEvent *xev, XdePopup *xpop)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> FocusOut: %p\n", xpop);
		fprintf(stderr, "    --> send_event = %s\n", xev->xfocus.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xfocus.window);
		fprintf(stderr, "    --> mode = %s\n", show_mode(xev->xfocus.mode));
		fprintf(stderr, "    --> detail = %s\n", show_detail(xev->xfocus.detail));
		fprintf(stderr, "<== FocusOut: %p\n", xpop);
	}
	switch (xev->xfocus.mode) {
	case NotifyNormal:
	case NotifyGrab:
	case NotifyWhileGrabbed:
		DPRINTF(1, "unfocused popup\n");
#if 0
		if (!xpop->keyboard) {
			DPRINTF(1, "no grab or focus\n");
			restart_popup_timer(xpop);
		}
#endif
		break;
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
popup_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdePopup *xpop = data;

	PTRACE(5);
	switch (xev->type) {
	case KeyPress:
		return event_handler_KeyPress(dpy, xev, xpop);
	case KeyRelease:
		return event_handler_KeyRelease(dpy, xev, xpop);
	case ButtonPress:
		return event_handler_ButtonPress(dpy, xev, xpop);
	case ButtonRelease:
		return event_handler_ButtonRelease(dpy, xev, xpop);
	case EnterNotify:
		return event_handler_EnterNotify(dpy, xev, xpop);
	case LeaveNotify:
		return event_handler_LeaveNotify(dpy, xev, xpop);
	case FocusIn:
		return event_handler_FocusIn(dpy, xev, xpop);
	case FocusOut:
		return event_handler_FocusOut(dpy, xev, xpop);
	}
	return GDK_FILTER_CONTINUE;
}

void
set_current_desktop(XdeScreen *xscr, int index, Time timestamp)
{
	Window root = RootWindow(dpy, xscr->index);
	XEvent ev;

	PTRACE(5);
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = False;
	ev.xclient.display = dpy;
	ev.xclient.window = root;
	ev.xclient.message_type = _XA_NET_CURRENT_DESKTOP;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = index;
	ev.xclient.data.l[1] = timestamp;
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = 0;

	XSendEvent(dpy, root, False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
}
#endif

/** @} */

/** @section Popup Window GTK Events
  * @{ */

#if 0
/** @brief grab broken event handler
  *
  * Generated when a pointer or keyboard grab is broken.  On X11, this happens
  * when a grab window becomes unviewable (i.e. it or one of its ancestors is
  * unmapped), or if the same application grabs the pointer or keyboard again.
  * Note that implicity grabs (which are initiated by button presses (or
  * grabbed key presses?)) can also cause GdkEventGrabBroken events.
  */
static gboolean
grab_broken_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdePopup *xpop = user;
	GdkEventGrabBroken *ev = (typeof(ev)) event;

	PTRACE(5);
	if (ev->keyboard) {
		DPRINTF(1, "keyboard grab was broken\n");
	} else {
		DPRINTF(1, "pointer grab was broken\n");
	}
	if (ev->implicit) {
		DPRINTF(1, "broken grab was implicit\n");
	} else {
		DPRINTF(1, "broken grab was explicit\n");
	}
	if (ev->grab_window) {
		DPRINTF(1, "we broke the grab\n");
	} else {
		DPRINTF(1, "another application broke the grab\n");
	}
	if (ev->keyboard) {
		// xpop->keyboard = False;
		/* IF we lost a keyboard grab, it is because another hot-key was pressed,
		   either doing something else or moving to another desktop.  Start the
		   timeout in this case. */
		if (ev->grab_window)
			stop_popup_timer(xpop);
		else
			restart_popup_timer(xpop);
	} else {
		// xpop->pointer = False;
		/* If we lost a pointer grab, it is because somebody clicked on another
		   window.  In this case we want to drop the popup altogether.  This will
		   break the keyboard grab if any. */
		if (!ev->grab_window)
			drop_popup(xpop);
	}
	return TRUE;		/* event handled */
}

static void
window_realize(GtkWidget *popup, gpointer xpop)
{
	gdk_window_add_filter(popup->window, popup_handler, xpop);
	// gdk_window_set_override_redirect(popup->window, TRUE);
	// gdk_window_set_accept_focus(popup->window, FALSE);
	// gdk_window_set_focus_on_map(popup->window, FALSE);
}

static gboolean
button_press_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
button_release_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
enter_notify_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
#if 0
	/* currently done by event handler, but considering grab */
	stop_popup_timer(xpop);
	xpop->inside = True;
#endif
	return GTK_EVENT_PROPAGATE;
}

static gboolean
focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
focus_out_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
	return GTK_EVENT_PROPAGATE;
}

static void
grab_focus(GtkWidget *widget, gpointer xpop)
{
}

static gboolean
key_press_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
key_release_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
	GdkEventKey *ev = (typeof(ev)) event;

	if (ev->is_modifier) {
		DPRINTF(1, "released key is modifier: dropping popup\n");
		drop_popup(xpop);
	} else
		DPRINTF(1, "released key is not a modifier: not dropping popup\n");
	return GTK_EVENT_PROPAGATE;
}

static gboolean
leave_notify_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
#if 0
	/* currently done by event handler, but considering grab */
	restart_popup_timer(xpop);
	xpop->inside = False;
#endif
	return GTK_EVENT_PROPAGATE;
}

static gboolean
map_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
scroll_event(GtkWidget *widget, GdkEvent *event, gpointer xpop)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
visibility_notify_event(GtkWidget *popup, GdkEvent *event, gpointer xpop)
{
	GdkEventVisibility *ev = (typeof(ev)) event;

	switch (ev->state) {
	case GDK_VISIBILITY_FULLY_OBSCURED:
	case GDK_VISIBILITY_PARTIAL:
		gdk_window_raise(popup->window);
		break;
	case GDK_VISIBILITY_UNOBSCURED:
		/* good */
		break;
	}
	return GTK_EVENT_PROPAGATE;	/* event not fully handled */
}
#endif

/** @} */

#endif

/** @} */

/** @section Startup Notification Handling
  * @{ */

#ifdef STARTUP_NOTIFICATION

static Bool
parse_startup_id(const char *id, char **launcher_p, char **launchee_p, char **hostname_p,
		 pid_t *pid_p, long *sequence_p, long *timestamp_p)
{
	long sequence = 0, timestamp = 0;
	pid_t pid = 0;
	const char *p;
	char *endptr = NULL, *s;

	do {
		const char *q;
		char *tmp;

		p = q = id;
		if (!(p = strchr(q, '/')))
			break;
		if (launcher_p)
			*launcher_p = strndup(q, p - q);
		q = p + 1;
		if (!(p = strchr(q, '/')))
			break;
		if (launchee_p) {
			*launchee_p = strndup(q, p - q);
			for (s = *launchee_p; *s; s++)
				if (*s == '|')
					*s = '/';
		}
		q = p + 1;
		if (!(p = strchr(q, '-')))
			break;
		tmp = strndup(q, p - q);
		pid = strtoul(tmp, &endptr, 10);
		free(tmp);
		if (*endptr)
			break;
		if (pid_p)
			*pid_p = pid;
		q = p + 1;
		if (!(p = strchr(q, '-')))
			break;
		tmp = strndup(q, p - q);
		sequence = strtoul(tmp, &endptr, 10);
		free(tmp);
		if (*endptr)
			break;
		if (sequence_p)
			*sequence_p = sequence;
		q = p + 1;
		if (!(p = strstr(q, "_TIME")))
			break;
		if (hostname_p)
			*hostname_p = strndup(q, p - q);
		q = p + 5;
		timestamp = strtoul(q, &endptr, 10);
		if (*endptr)
			break;
		if (timestamp_p)
			*timestamp_p = timestamp;
		return (True);
	}
	while (0);

	if (!timestamp && (p = strstr(id, "_TIME"))) {
		timestamp = strtoul(p + 5, &endptr, 10);
		if (!*endptr)
			if (timestamp_p)
				*timestamp_p = timestamp;
	}
	return (False);
}

static Sequence *
find_sequence(XdeScreen *xscr, const char *id)
{
	GList *list;

	for (list = xscr->sequences; list; list = list->next) {
		Sequence *seq = list->data;

		if (!strcmp(id, sn_startup_sequence_get_id(seq->seq)))
			return (seq);
	}
	return (NULL);
}

void
del_sequence(XdePopup *xpop, Sequence *seq)
{
	gtk_list_store_remove(xpop->model, &seq->iter);
	*seq->list = g_list_remove(*seq->list, seq);
	free(seq->launcher);
	free(seq->launchee);
	free(seq->hostname);
	sn_startup_sequence_unref(seq->seq);
	seq->seq = NULL;
	if (seq->info) {
		g_object_unref(G_OBJECT(seq->info));
		seq->info = NULL;
	}
	free(seq);
	xpop->seqcount--;
	if (xpop->seqcount <= 0)
		drop_popup(xpop);
}

static gboolean
seq_timeout(gpointer data)
{
	Sequence *seq = data;
	XdePopup *xpop = seq->xpop;

	del_sequence(xpop, seq);
	return G_SOURCE_REMOVE;	/* remove event source */
}

void
rem_sequence(XdeScreen *xscr, Sequence *seq)
{
	g_timeout_add(options.timeout, seq_timeout, seq);
}

void
cha_sequence(XdeScreen *xscr, Sequence *seq)
{
	GdkPixbuf *pixbuf = NULL;
	const char *appid, *name = NULL, *desc = NULL, *tip;
	char *markup, *aid, *p;
	XdePopup *xpop = seq->xpop;

	if ((appid = sn_startup_sequence_get_application_id(seq->seq))) {
		if (!(p = strrchr(appid, '.')) || strcmp(p, ".desktop"))
			aid = g_strdup_printf("%s.desktop", appid);
		else
			aid = g_strdup(appid);
		seq->info = g_desktop_app_info_new(aid);
		g_free(aid);
	}
	if (!pixbuf)
		pixbuf = get_sequence_pixbuf(seq);
	if (!pixbuf)
		pixbuf = get_icons(g_app_info_get_icon(G_APP_INFO(seq->info)), NULL);
	if (!name)
		name = sn_startup_sequence_get_name(seq->seq);
	if (!name && seq->info)
		name = g_app_info_get_display_name(G_APP_INFO(seq->info));
	if (!name && seq->info)
		name = g_app_info_get_name(G_APP_INFO(seq->info));
	if (!name && seq->info)
		name = g_desktop_app_info_get_generic_name(seq->info);
	if (!name)
		name = sn_startup_sequence_get_wmclass(seq->seq);
	if (!name && seq->info)
		name = g_desktop_app_info_get_startup_wm_class(seq->info);
	if (!name)
		if ((name = sn_startup_sequence_get_binary_name(seq->seq)))
			if (strrchr(name, '/'))
				name = strrchr(name, '/');
	if (!name)
		if ((name = seq->launchee))
			if (strrchr(name, '/'))
				name = strrchr(name, '/');
	if (!desc)
		desc = sn_startup_sequence_get_description(seq->seq);
	if (!desc && seq->info)
		desc = g_app_info_get_description(G_APP_INFO(seq->info));
	markup = g_markup_printf_escaped("<span font=\"%.1f\"><b>%s</b>\n%s</span>", options.fontsize, name ? : "", desc ? : "");
	/* for now, ellipsize later */
	tip = desc;
	/* *INDENT-OFF* */
	gtk_list_store_set(xpop->model, &seq->iter,
			0, pixbuf,
			1, name,
			2, desc,
			3, markup,
			4, tip,
			5, seq,
			-1);
	/* *INDENT-ON* */
	g_object_unref(pixbuf);
	g_free(markup);
}

Sequence *
add_sequence(XdeScreen *xscr, const char *id, SnStartupSequence *sn_seq)
{
	Sequence *seq;
	Time timestamp;
	XdeMonitor *xmon;
	XdePopup *xpop;
	int screen;

	if ((seq = calloc(1, sizeof(*seq)))) {
		seq->screen = xscr->index;
		seq->monitor = 0;
		seq->seq = sn_seq;
		sn_startup_sequence_ref(sn_seq);
		seq->sequence = -1;
		seq->timestamp = -1;
		parse_startup_id(id, &seq->launcher, &seq->launchee, &seq->hostname,
				 &seq->pid, &seq->sequence, &seq->timestamp);
		if (seq->launcher && !strcmp(seq->launcher, "xdg-launch") && seq->sequence != -1)
			seq->monitor = seq->sequence;
		if (!seq->timestamp)
			if ((timestamp = sn_startup_sequence_get_timestamp(sn_seq)) != -1)
				seq->timestamp = timestamp;
		if ((screen = sn_startup_sequence_get_screen(sn_seq)) != -1) {
			seq->screen = screen;
			xscr = screens + screen;
		}
		seq->list = &xscr->sequences;
		xscr->sequences = g_list_append(xscr->sequences, seq);
		xmon = xscr->mons + seq->monitor;
		xpop = &xmon->start;
		gtk_list_store_append(xpop->model, &seq->iter);
		xpop->seqcount++;
		seq->xpop = xpop;

		cha_sequence(xscr, seq);

		show_popup(xscr, xpop, FALSE, FALSE);
	}
	return (seq);
}

#endif				/* STARTUP_NOTIFICATION */

/** @} */

/** @section Session Management
  * @{ */

#if 0
static void
clientSetProperties(SmcConn smcConn, SmPointer data)
{
	char userID[20];
	int i, j, argc = saveArgc;
	char **argv = saveArgv;
	char *cwd = NULL;
	char hint;
	struct passwd *pw;
	SmPropValue *penv = NULL, *prst = NULL, *pcln = NULL;
	SmPropValue propval[11];
	SmProp prop[11];

	SmProp *props[11] = {
		&prop[0], &prop[1], &prop[2], &prop[3], &prop[4],
		&prop[5], &prop[6], &prop[7], &prop[8], &prop[9],
		&prop[10]
	};

	j = 0;

	/* CloneCommand: This is like the RestartCommand except it restarts a copy of the
	   application.  The only difference is that the application doesn't supply its
	   client id at register time.  On POSIX systems the type should be a
	   LISTofARRAY8. */
	prop[j].name = SmCloneCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = pcln = calloc(argc, sizeof(*pcln));
	prop[j].num_vals = 0;
	props[j] = &prop[j];
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-clientId") || !strcmp(argv[i], "-restore"))
			i++;
		else {
			prop[j].vals[prop[j].num_vals].value = (SmPointer) argv[i];
			prop[j].vals[prop[j].num_vals++].length = strlen(argv[i]);
		}
	}
	j++;

#if 1
	/* CurrentDirectory: On POSIX-based systems, specifies the value of the current
	   directory that needs to be set up prior to starting the program and should be
	   of type ARRAY8. */
	prop[j].name = SmCurrentDirectory;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	propval[j].value = NULL;
	propval[j].length = 0;
	cwd = calloc(PATH_MAX + 1, sizeof(propval[j].value[0]));
	if (getcwd(cwd, PATH_MAX)) {
		propval[j].value = cwd;
		propval[j].length = strlen(propval[j].value);
		j++;
	} else {
		free(cwd);
		cwd = NULL;
	}
#endif

#if 0
	/* DiscardCommand: The discard command contains a command that when delivered to
	   the host that the client is running on (determined from the connection), will
	   cause it to discard any information about the current state.  If this command
	   is not specified, the SM will assume that all of the client's state is encoded
	   in the RestartCommand [and properties].  On POSIX systems the type should be
	   LISTofARRAY8. */
	prop[j].name = SmDiscardCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	propval[j].value = "/bin/true";
	propval[j].length = strlen("/bin/true");
	j++;
#endif

#if 1
	char **env;

	/* Environment: On POSIX based systems, this will be of type LISTofARRAY8 where
	   the ARRAY8s alternate between environment variable name and environment
	   variable value. */
	/* XXX: we might want to filter a few out */
	for (i = 0, env = environ; *env; i += 2, env++) ;
	prop[j].name = SmEnvironment;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = penv = calloc(i, sizeof(*penv));
	prop[j].num_vals = i;
	props[j] = &prop[j];
	for (i = 0, env = environ; *env; i += 2, env++) {
		char *equal;
		int len;

		equal = strchrnul(*env, '=');
		len = (int) (*env - equal);
		if (*equal)
			equal++;
		prop[j].vals[i].value = *env;
		prop[j].vals[i].length = len;
		prop[j].vals[i + 1].value = equal;
		prop[j].vals[i + 1].length = strlen(equal);
	}
	j++;
#endif

#if 1
	char procID[20];

	/* ProcessID: This specifies an OS-specific identifier for the process. On POSIX
	   systems this should be of type ARRAY8 and contain the return of getpid()
	   turned into a Latin-1 (decimal) string. */
	prop[j].name = SmProcessID;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	snprintf(procID, sizeof(procID), "%ld", (long) getpid());
	propval[j].value = procID;
	propval[j].length = strlen(procID);
	j++;
#endif

	/* Program: The name of the program that is running.  On POSIX systems, this
	   should eb the first parameter passed to execve(3) and should be of type
	   ARRAY8. */
	prop[j].name = SmProgram;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	propval[j].value = argv[0];
	propval[j].length = strlen(argv[0]);
	j++;

	/* RestartCommand: The restart command contains a command that when delivered to
	   the host that the client is running on (determined from the connection), will
	   cause the client to restart in its current state.  On POSIX-based systems this
	   if of type LISTofARRAY8 and each of the elements in the array represents an
	   element in the argv[] array.  This restart command should ensure that the
	   client restarts with the specified client-ID.  */
	prop[j].name = SmRestartCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = prst = calloc(argc + 4, sizeof(*prst));
	prop[j].num_vals = 0;
	props[j] = &prop[j];
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-clientId") || !strcmp(argv[i], "-restore"))
			i++;
		else {
			prop[j].vals[prop[j].num_vals].value = (SmPointer) argv[i];
			prop[j].vals[prop[j].num_vals++].length = strlen(argv[i]);
		}
	}
	prop[j].vals[prop[j].num_vals].value = (SmPointer) "-clientId";
	prop[j].vals[prop[j].num_vals++].length = 9;
	prop[j].vals[prop[j].num_vals].value = (SmPointer) options.clientId;
	prop[j].vals[prop[j].num_vals++].length = strlen(options.clientId);

	prop[j].vals[prop[j].num_vals].value = (SmPointer) "-restore";
	prop[j].vals[prop[j].num_vals++].length = 9;
	prop[j].vals[prop[j].num_vals].value = (SmPointer) options.saveFile;
	prop[j].vals[prop[j].num_vals++].length = strlen(options.saveFile);
	j++;

	/* ResignCommand: A client that sets the RestartStyleHint to RestartAnyway uses
	   this property to specify a command that undoes the effect of the client and
	   removes any saved state. */
	prop[j].name = SmResignCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = calloc(2, sizeof(*prop[j].vals));
	prop[j].num_vals = 2;
	props[j] = &prop[j];
	prop[j].vals[0].value = "/usr/bin/" RESNAME;
	prop[j].vals[0].length = strlen("/usr/bin/" RESNAME);
	prop[j].vals[1].value = "-quit";
	prop[j].vals[1].length = strlen("-quit");
	j++;

	/* RestartStyleHint: If the RestartStyleHint property is present, it will contain
	   the style of restarting the client prefers.  If this flag is not specified,
	   RestartIfRunning is assumed.  The possible values are as follows:
	   RestartIfRunning(0), RestartAnyway(1), RestartImmediately(2), RestartNever(3).
	   The RestartIfRunning(0) style is used in the usual case.  The client should be
	   restarted in the next session if it is connected to the session manager at the
	   end of the current session. The RestartAnyway(1) style is used to tell the SM
	   that the application should be restarted in the next session even if it exits
	   before the current session is terminated. It should be noted that this is only
	   a hint and the SM will follow the policies specified by its users in
	   determining what applications to restart.  A client that uses RestartAnyway(1)
	   should also set the ResignCommand and ShutdownCommand properties to the
	   commands that undo the state of the client after it exits.  The
	   RestartImmediately(2) style is like RestartAnyway(1) but in addition, the
	   client is meant to run continuously.  If the client exits, the SM should try to
	   restart it in the current session.  The RestartNever(3) style specifies that the
	   client does not wish to be restarted in the next session. */
	prop[j].name = SmRestartStyleHint;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[0];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	hint = SmRestartImmediately;	/* <--- */
	propval[j].value = &hint;
	propval[j].length = 1;
	j++;

	/* ShutdownCommand: This command is executed at shutdown time to clean up after a
	   client that is no longer running but retained its state by setting
	   RestartStyleHint to RestartAnyway(1).  The command must not remove any saved
	   state as the client is still part of the session. */
	prop[j].name = SmShutdownCommand;
	prop[j].type = SmLISTofARRAY8;
	prop[j].vals = calloc(2, sizeof(*prop[j].vals));
	prop[j].num_vals = 2;
	props[j] = &prop[j];
	prop[j].vals[0].value = "/usr/bin/" RESNAME;
	prop[j].vals[0].length = strlen("/usr/bin/" RESNAME);
	prop[j].vals[1].value = "-quit";
	prop[j].vals[1].length = strlen("-quit");
	j++;

	/* UserID: Specifies the user's ID.  On POSIX-based systems this will contain the
	   user's name (the pw_name field of struct passwd).  */
	errno = 0;
	prop[j].name = SmUserID;
	prop[j].type = SmARRAY8;
	prop[j].vals = &propval[j];
	prop[j].num_vals = 1;
	props[j] = &prop[j];
	if ((pw = getpwuid(getuid())))
		strncpy(userID, pw->pw_name, sizeof(userID) - 1);
	else {
		EPRINTF("%s: %s\n", "getpwuid()", strerror(errno));
		snprintf(userID, sizeof(userID), "%ld", (long) getuid());
	}
	propval[j].value = userID;
	propval[j].length = strlen(userID);
	j++;

	SmcSetProperties(smcConn, j, props);

	free(cwd);
	free(pcln);
	free(prst);
	free(penv);
}

static Bool saving_yourself;
static Bool shutting_down;

static void
clientSaveYourselfPhase2CB(SmcConn smcConn, SmPointer data)
{
	clientSetProperties(smcConn, data);
	SmcSaveYourselfDone(smcConn, True);
}

/** @brief save yourself
  *
  * The session manager sends a "Save Yourself" message to a client either to
  * check-point it or just before termination so that it can save its state.
  * The client responds with zero or more calls to SmcSetProperties to update
  * the properties indicating how to restart the client.  When all the
  * properties have been set, the client calls SmcSaveYourselfDone.
  *
  * If interact_type is SmcInteractStyleNone, the client must not interact with
  * the user while saving state.  If interact_style is SmInteractStyleErrors,
  * the client may interact with the user only if an error condition arises.  If
  * interact_style is  SmInteractStyleAny then the client may interact with the
  * user for any purpose.  Because only one client can interact with the user at
  * a time, the client must call SmcInteractRequest and wait for an "Interact"
  * message from the session maanger.  When the client is done interacting with
  * the user, it calls SmcInteractDone.  The client may only call
  * SmcInteractRequest() after it receives a "Save Yourself" message and before
  * it calls SmcSaveYourSelfDone().
  */
static void
clientSaveYourselfCB(SmcConn smcConn, SmPointer data, int saveType, Bool shutdown, int interactStyle, Bool fast)
{
	if (!(shutting_down = shutdown)) {
		if (!SmcRequestSaveYourselfPhase2(smcConn, clientSaveYourselfPhase2CB, data))
			SmcSaveYourselfDone(smcConn, False);
		return;
	}
	clientSetProperties(smcConn, data);
	SmcSaveYourselfDone(smcConn, True);
}

/** @brief die
  *
  * The session manager sends a "Die" message to a client when it wants it to
  * die.  The client should respond by calling SmcCloseConnection.  A session
  * manager that behaves properly will send a "Save Yourself" message before the
  * "Die" message.
  */
static void
clientDieCB(SmcConn smcConn, SmPointer data)
{
	SmcCloseConnection(smcConn, 0, NULL);
	shutting_down = False;
	mainloop_quit();
}

static void
clientSaveCompleteCB(SmcConn smcConn, SmPointer data)
{
	if (saving_yourself) {
		saving_yourself = False;
		mainloop_quit();
	}

}

/** @brief shutdown cancelled
  *
  * The session manager sends a "Shutdown Cancelled" message when the user
  * cancelled the shutdown during an interaction (see Section 5.5, "Interacting
  * With the User").  The client can now continue as if the shutdown had never
  * happended.  If the client has not called SmcSaveYourselfDone() yet, it can
  * either abort the save and then send SmcSaveYourselfDone() with the success
  * argument set to False or it can continue with the save and then call
  * SmcSaveYourselfDone() with the success argument set to reflect the outcome
  * of the save.
  */
static void
clientShutdownCancelledCB(SmcConn smcConn, SmPointer data)
{
	shutting_down = False;
	mainloop_quit();
}

static unsigned long clientCBMask = SmcSaveYourselfProcMask |
    SmcDieProcMask | SmcSaveCompleteProcMask | SmcShutdownCancelledProcMask;

static SmcCallbacks clientCBs = {
	.save_yourself = {.callback = &clientSaveYourselfCB,.client_data = NULL,},
	.die = {.callback = &clientDieCB,.client_data = NULL,},
	.save_complete = {.callback = &clientSaveCompleteCB,.client_data = NULL,},
	.shutdown_cancelled = {.callback = &clientShutdownCancelledCB,.client_data = NULL,},
};
#endif

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
putXrmColor(const GdkColor * color)
{
	return gdk_color_to_string(color);
}

char *
putXrmFont(const PangoFontDescription * font)
{
	return pango_font_description_to_string(font);
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
	case UseScreenSelect:
		return g_strdup("select");
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
	case PositionTopLeft:
		return g_strdup("topleft");
	case PositionPointer:
		return g_strdup("pointer");
	case PositionBottomRight:
		return g_strdup("bottomright");
	case PositionSpecified:
		return putXrmXGeometry(geom);
	case PositionIconGeom:
		return g_strdup("icongeom");
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
	char *usrdb;
	XrmDatabase rdb = NULL;
	Display *dpy;
	char *val;

	if (!options.display)
		return;

	usrdb = g_build_filename(g_get_user_config_dir(), RESNAME, "rc", NULL);

	if (!(dpy = XOpenDisplay(NULL))) {
		EPRINTF("could not open display %s\n", options.display);
		exit(EXIT_FAILURE);
	}
	rdb = XrmGetDatabase(dpy);
	if (!rdb) {
		DPRINTF(1, "no resource manager database allocated\n");
		return;
	}
	if ((val = putXrmInt(options.debug)))
		put_resource(rdb, "debug", val);
	if ((val = putXrmInt(options.output)))
		put_resource(rdb, "verbose", val);
	/* put a bunch of resources */
	if ((val = putXrmString(options.theme)))
		put_resource(rdb, "theme", val);
	if ((val = putXrmString(options.itheme)))
		put_resource(rdb, "icontheme", val);
	if ((val = putXrmWhich(options.which, options.screen)))
		put_resource(rdb, "which", val);
	if ((val = putXrmWhere(options.where, &options.geom)))
		put_resource(rdb, "where", val);
	/* FIXME: more */
	if ((val = putXrmBool(options.tooltips)))
		put_resource(rdb, "tooltips", val);
	XrmPutFileDatabase(rdb, usrdb);
	XrmSetDatabase(dpy, rdb);
	XrmDestroyDatabase(rdb);
	XCloseDisplay(dpy);
	g_free(usrdb);
}

/** @} */

/** @section Putting Key File
  * @{ */

void
put_keyfile(void)
{
#if 0
	char *val, buf[256] = { 0, };

	if (!file) {
		EPRINTF("no key file!\n");
		return;
	}
	if (support.Keyboard) {
		int i, j;

		g_key_file_set_integer(file, KFG_Keyboard,
				       KFK_Keyboard_KeyClickPercent, state.Keyboard.key_click_percent);
		g_key_file_set_integer(file, KFG_Keyboard, KFK_Keyboard_BellPercent, state.Keyboard.bell_percent);
		g_key_file_set_integer(file, KFG_Keyboard, KFK_Keyboard_BellPitch, state.Keyboard.bell_pitch);
		g_key_file_set_integer(file, KFG_Keyboard,
				       KFK_Keyboard_BellDuration, state.Keyboard.bell_duration);
		g_key_file_set_integer(file, KFG_Keyboard, KFK_Keyboard_LEDMask, state.Keyboard.led_mask);
		g_key_file_set_boolean(file, KFG_Keyboard,
				       KFK_Keyboard_GlobalAutoRepeat, state.Keyboard.global_auto_repeat);
		for (i = 0, j = 0; i < 32; i++, j += 2)
			snprintf(buf + j, sizeof(buf) - j, "%02X", state.Keyboard.auto_repeats[i]);
		g_key_file_set_value(file, KFG_Keyboard, KFK_Keyboard_AutoRepeats, buf);
	}
	if (support.Pointer) {
		g_key_file_set_integer(file, KFG_Pointer,
				       KFK_Pointer_AccelerationDenominator, state.Pointer.accel_denominator);
		g_key_file_set_integer(file, KFG_Pointer,
				       KFK_Pointer_AccelerationNumerator, state.Pointer.accel_numerator);
		g_key_file_set_integer(file, KFG_Pointer, KFK_Pointer_Threshold, state.Pointer.threshold);
	}
	if (support.ScreenSaver) {
		if ((val = putXrmExposures(state.ScreenSaver.allow_exposures))) {
			g_key_file_set_value(file, KFG_ScreenSaver, KFK_ScreenSaver_AllowExposures, val);
			g_free(val);
		}
		g_key_file_set_integer(file, KFG_ScreenSaver,
				       KFK_ScreenSaver_Interval, state.ScreenSaver.interval);
		if ((val = putXrmBlanking(state.ScreenSaver.prefer_blanking))) {
			g_key_file_set_value(file, KFG_ScreenSaver, KFK_ScreenSaver_PreferBlanking, val);
			g_free(val);
		}
		g_key_file_set_integer(file, KFG_ScreenSaver, KFK_ScreenSaver_Timeout, state.ScreenSaver.timeout);
	}
	if (support.DPMS) {
		if ((val = putXrmPowerLevel(state.DPMS.power_level))) {
			g_key_file_set_value(file, KFG_DPMS, KFK_DPMS_PowerLevel, val);
			g_free(val);
		}
		g_key_file_set_boolean(file, KFG_DPMS, KFK_DPMS_State, state.DPMS.state ? TRUE : FALSE);
		g_key_file_set_integer(file, KFG_DPMS, KFK_DPMS_StandbyTimeout, state.DPMS.standby);
		g_key_file_set_integer(file, KFG_DPMS, KFK_DPMS_SuspendTimeout, state.DPMS.suspend);
		g_key_file_set_integer(file, KFG_DPMS, KFK_DPMS_OffTimeout, state.DPMS.off);
	}
	if (support.XKeyboard) {
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_MouseKeysDfltBtn, state.XKeyboard.desc->ctrls->mk_dflt_btn);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_RepeatKeysEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbRepeatKeysMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_SlowKeysEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbSlowKeysMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_BounceKeysEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbBounceKeysMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_StickyKeysEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbStickyKeysMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_MouseKeysEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbMouseKeysMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_MouseKeysAccelEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbMouseKeysAccelMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_AccessXKeysEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbAccessXKeysMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_AccessXTimeoutMaskEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbAccessXTimeoutMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_AccessXFeedbackMaskEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbAccessXFeedbackMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_AudibleBellMaskEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbAudibleBellMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_Overlay1MaskEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbOverlay1Mask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_Overlay2MaskEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbOverlay2Mask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_IgnoreGroupLockEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbIgnoreGroupLockMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_GroupsWrapEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbGroupsWrapMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_InternalModsEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbInternalModsMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_IgnoreLockModsEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbIgnoreLockModsMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_PerKeyRepeatEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbPerKeyRepeatMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_ControlsEnabledEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbControlsEnabledMask ? TRUE : FALSE);
		g_key_file_set_boolean(file, KFG_XKeyboard,
				       KFK_XKeyboard_AccessXOptionsEnabled,
				       state.XKeyboard.desc->ctrls->enabled_ctrls &
				       XkbAccessXOptionsMask ? TRUE : FALSE);
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_RepeatDelay, state.XKeyboard.desc->ctrls->repeat_delay);
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_RepeatInterval, state.XKeyboard.desc->ctrls->repeat_interval);
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_SlowKeysDelay, state.XKeyboard.desc->ctrls->slow_keys_delay);
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_DebounceDelay, state.XKeyboard.desc->ctrls->debounce_delay);
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_MouseKeysDelay, state.XKeyboard.desc->ctrls->mk_delay);
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_MouseKeysInterval, state.XKeyboard.desc->ctrls->mk_interval);
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_MouseKeysTimeToMax,
				       state.XKeyboard.desc->ctrls->mk_time_to_max);
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_MouseKeysMaxSpeed, state.XKeyboard.desc->ctrls->mk_max_speed);
		g_key_file_set_integer(file, KFG_XKeyboard,
				       KFK_XKeyboard_MouseKeysCurve, state.XKeyboard.desc->ctrls->mk_curve);
	}
#endif
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
getXrmColor(const char *val, GdkColor **color)
{
	GdkColor c, *p;

	if (gdk_color_parse(val, &c) && (p = calloc(1, sizeof(*p)))) {
		*p = c;
		free(*color);
		*color = p;
		return True;
	}
	EPRINTF("could not parse color '%s'\n", val);
	return False;
}

Bool
getXrmFont(const char *val, PangoFontDescription **face)
{
	FcPattern *pattern;
	PangoFontDescription *font;

	if ((pattern = FcNameParse((FcChar8 *) val))) {
		if ((font = pango_fc_font_description_from_pattern(pattern, True))) {
			pango_font_description_free(*face);
			*face = font;
			DPRINTF(1, "Font description is: %s\n", pango_font_description_to_string(font));
			return True;
		}
		FcPatternDestroy(pattern);
	}
	EPRINTF("could not parse font descriptikon '%s'\n", val);
	return False;
}

Bool
getXrmInt(const char *val, int *integer)
{
	char *endptr = NULL;
	int value;

	value = strtol(val, &endptr, 0);
	if (*endptr)
		return False;
	*integer = value;
	return True;
}

Bool
getXrmUint(const char *val, unsigned int *integer)
{
	char *endptr = NULL;
	unsigned int value;

	value = strtoul(val, &endptr, 0);
	if (*endptr)
		return False;
	*integer = value;
	return True;
}

Bool
getXrmTime(const char *val, Time *time)
{
	char *endptr = NULL;
	unsigned int value;

	value = strtoul(val, &endptr, 0);
	if (*endptr)
		return False;
	*time = value;
	return True;
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

static void
get_resources(void)
{
	XrmDatabase rdb;
	const char *val;
	char *usrdflt;

	PTRACE(5);
	XrmInitialize();
	if (getenv("DISPLAY")) {
		Display *dpy;

		if (!(dpy = XOpenDisplay(NULL))) {
			EPRINTF("could not open display %s\n", getenv("DISPLAY"));
			exit(EXIT_FAILURE);
		}
		rdb = XrmGetDatabase(dpy);
		if (!rdb)
			DPRINTF(1, "no resource manager database allocated\n");
		XCloseDisplay(dpy);
	}
	if (options.filename) {
		DPRINTF(1, "merging config from %s\n", options.filename);
		if (!XrmCombineFileDatabase(options.filename, &rdb, False))
			DPRINTF(1, "could not open rcfile %s\n", options.filename);
	}
	usrdflt = g_build_filename(g_get_user_config_dir(), RESNAME, "rc", NULL);
	if (!options.filename || strcmp(options.filename, usrdflt)) {
		DPRINTF(1, "merging config from %s\n", usrdflt);
		if (!XrmCombineFileDatabase(usrdflt, &rdb, False))
			DPRINTF(1, "could not open rcfile %s\n", usrdflt);
	}
	g_free(usrdflt);
	DPRINTF(1, "merging config from %s\n", APPDFLT);
	if (!XrmCombineFileDatabase(APPDFLT, &rdb, False))
		DPRINTF(1, "could not open rcfile %s\n", APPDFLT);
	if (!rdb) {
		DPRINTF(1, "no resource manager database found\n");
		rdb = XrmGetStringDatabase("");
	}
	if ((val = get_resource(rdb, "debug", NULL)))
		getXrmInt(val, &options.debug);
	if ((val = get_resource(rdb, "verbose", NULL)))
		getXrmInt(val, &options.output);
	/* get a bunch of resources */
	if ((val = get_resource(rdb, "theme", NULL)))
		getXrmString(val, &options.theme);
	if ((val = get_resource(rdb, "icontheme", NULL)))
		getXrmString(val, &options.itheme);
	if ((val = get_resource(rdb, "which", "default")))
		getXrmWhich(val, &options.which, &options.screen);
	if ((val = get_resource(rdb, "where", "default")))
		getXrmWhere(val, &options.where, &options.geom);
	if ((val = get_resource(rdb, "tooltips", "false")))
		getXrmBool(val, &options.tooltips);

	XrmDestroyDatabase(rdb);
}

/** @} */

/** @section Getting Key File
  * @{ */

void
get_keyfile(void)
{
}

/** @} */

/** @section System Tray Icon
  * @{ */

#if 0
static gboolean
button_press(GtkStatusIcon *icon, GdkEvent *event, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	(void) xscr;
	/* FIXME: do something on icon button press */
	return GTK_EVENT_PROPAGATE;
}

static void popup_show(XdeScreen *xscr);

static void
edit_selected(GtkMenuItem *item, gpointer user_data)
{
	popup_show(user_data);
}

static void
save_selected(GtkMenuItem *item, gpointer user_data)
{
#if 0
	edit_sav_values();
#endif
}

static void
popup_refresh(XdeScreen *xscr)
{
}

void
refresh_selected(GtkMenuItem *item, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	popup_refresh(xscr);
	return;
}

void
about_selected(GtkMenuItem *item, gpointer user_data)
{
	gchar *authors[] = { "Brian F. G. Bidulock <bidulock@openss7.org>", NULL };
	gtk_show_about_dialog(NULL,
			      "authors", authors,
			      "comments", XDE_DESCRIP,
			      "copyright", "Copyright (c) 2013, 2014, 2015, 2016, 2017, 2018, 2019  OpenSS7 Corporation",
			      "license", "Do what thou wilt shall be the whole of the law.\n\n-- Aleister Crowley",
			      "logo-icon-name", LOGO_NAME,
			      "program-name", NAME,
			      "version", VERSION,
			      "website", "http://www.unexicon.com/",
			      "website-label", "Unexicon - Linux spun for telecom", NULL);
	return;
}

/** @brief restart
  *
  * We restart by executing ourselves with the same arguments that were provided
  * in the command that started us.  However, if we are running under session
  * management with restart hint SmRestartImmediately, the session manager will
  * restart us if we simply exit.
  */
static void
popup_restart(void)
{
	/* asked to restart (as though we were re-executed) */
	char **argv;
	int i;

	if (smcConn) {
		/* When running under a session manager, simply exit and the session
		   manager will restart us immediately. */
		exit(EXIT_SUCCESS);
	}

	argv = calloc(saveArgc + 1, sizeof(*argv));
	for (i = 0; i < saveArgc; i++)
		argv[i] = saveArgv[i];

	DPRINTF(1, "restarting\n");
	if (execvp(argv[0], argv) == -1)
		EPRINTF("%s: %s\n", argv[0], strerror(errno));
	return;
}

void
redo_selected(GtkMenuItem *item, gpointer user_data)
{
	popup_restart();
}

void
quit_selected(GtkMenuItem *item, gpointer user_data)
{
	mainloop_quit();
}

static void
popup_menu(GtkStatusIcon *icon, guint button, guint time, gpointer user_data)
{
	XdeScreen *xscr = user_data;
	GtkWidget *menu, *item;

	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock("gtk-edit", NULL);
	g_signal_connect(item, "activate", G_CALLBACK(edit_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-save", NULL);
	g_signal_connect(item, "activate", G_CALLBACK(save_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-refresh", NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(refresh_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-about", NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(about_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-redo", NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(redo_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-quit", NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(quit_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, icon, button, time);
	return;
}

#if 0
void
present_popup(XdeScreen *xscr)
{
	if (!gtk_widget_get_mapped(xscr->ttwindow)) {
#if 0
		GdkEventMask mask =
		    GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
#endif
		gtk_window_set_position(GTK_WINDOW(xscr->ttwindow), GTK_WIN_POS_MOUSE);
		gtk_window_present(GTK_WINDOW(xscr->ttwindow));
		gtk_widget_show_now(GTK_WIDGET(xscr->ttwindow));
		gdk_window_focus(xscr->ttwindow->window, GDK_CURRENT_TIME);

#if 0
		gdk_keyboard_grab(xscr->ttwindow->window, TRUE, GDK_CURRENT_TIME);
		gdk_pointer_grab(xscr->ttwindow->window, FALSE, mask, NULL, NULL, GDK_CURRENT_TIME);
#endif
		stop_popup_timer(xscr);
		inside = TRUE;
	}
}
#endif

gboolean
query_tooltip(GtkStatusIcon *icon, gint x, gint y, gboolean keyboard_mode,
		 GtkTooltip *tooltip, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	(void) xscr;
#if 0
	if (xscr->ttwindow) {
		present_popup(xscr);
		restart_popup_timer(xscr);
		return FALSE;
	}
#endif
	return TRUE;		/* show it now */
}

#if 1
static void
popup_widget_realize(GtkWidget *popup, gpointer user)
{
	gdk_window_add_filter(popup->window, popup_handler, user);
	gdk_window_set_override_redirect(popup->window, TRUE);
	// gdk_window_set_accept_focus(popup->window, FALSE);
	// gdk_window_set_focus_on_map(popup->window, FALSE);
}
#endif

#if 0
static gboolean
popup_grab_broken_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;
	GdkEventGrabBroken *ev = (typeof(ev)) event;

	if (ev->keyboard) {
		restart_popup_timer(xscr);
	} else {
		drop_popup(xscr);
	}
	return GTK_EVENT_STOP;	/* event handled */
}
#endif

void
systray_tooltip(XdeScreen *xscr)
{
#if 1
	GtkWidget *w, *h, *f, *s;

	if (xscr->ttwindow)
		return;

	w = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_add_events(w, GDK_ALL_EVENTS_MASK);
	gtk_window_set_accept_focus(GTK_WINDOW(w), FALSE);
	gtk_window_set_focus_on_map(GTK_WINDOW(w), FALSE);
	// gtk_window_set_type_hint(GTK_WINDOW(w), GDK_WINDOW_TEMP);
	gtk_window_stick(GTK_WINDOW(w));
	gtk_window_set_keep_above(GTK_WINDOW(w), TRUE);

	h = gtk_hbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(h), 3);
	f = gtk_frame_new("Bell");
	gtk_container_set_border_width(GTK_CONTAINER(f), 3);
	gtk_frame_set_label_align(GTK_FRAME(f), 0.5, 0.5);
	gtk_box_pack_start(GTK_BOX(h), f, FALSE, FALSE, 0);
	s = gtk_vscale_new_with_range(0.0, 100.0, 1.0);
	gtk_scale_set_draw_value(GTK_SCALE(s), TRUE);
	gtk_scale_set_value_pos(GTK_SCALE(s), GTK_POS_TOP);
	g_signal_connect(G_OBJECT(s), "format-value", G_CALLBACK(format_value_percent), NULL);
	gtk_container_add(GTK_CONTAINER(f), s);
	gtk_widget_set_tooltip_markup(s, "\
Set the bell volume as a percentage of\n\
maximum volume: from 0% to 100%.");
#if 0
	g_signal_connect(G_OBJECT(s), "value-changed", G_CALLBACK(bell_percent_value_changed), NULL);
#endif
	controls.Icon.BellPercent = s;

	gtk_container_add(GTK_CONTAINER(w), h);
	gtk_widget_show_all(h);

	gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(w), 3);
	// gtk_window_set_default_size(GTK_WINDOW(w), -1, 200);
	gtk_widget_set_size_request(w, -1, 200);

#if 0
	g_signal_connect(G_OBJECT(w), "grab_broken_event", G_CALLBACK(popup_grab_broken_event), xscr);
#endif
	g_signal_connect(G_OBJECT(w), "realize", G_CALLBACK(popup_widget_realize), xscr);

	xscr->ttwindow = w;
#endif
}

static void
systray_show(XdeScreen *xscr)
{
	if (!xscr->icon) {
		xscr->icon = gtk_status_icon_new_from_icon_name(LOGO_NAME);
		gtk_status_icon_set_tooltip_text(xscr->icon, "Click for menu...");
		g_signal_connect(G_OBJECT(xscr->icon), "button_press_event", G_CALLBACK(button_press), xscr);
		g_signal_connect(G_OBJECT(xscr->icon), "popup_menu", G_CALLBACK(popup_menu), xscr);
		// g_signal_connect(xscr->icon, "query_tooltip", G_CALLBACK(query_tooltip), xscr);
		systray_tooltip(xscr);
	}
	gtk_status_icon_set_visible(xscr->icon, TRUE);
}

#endif

/** @} */

/** @section Event handlers
  * @{ */

/** @section libwnck Event handlers
  * @{ */

/** @section Workspace Events
  * @{ */

#if 0
static Bool
good_window_manager(XdeScreen *xscr)
{
	PTRACE(5);
	/* ignore non fully compliant names */
	if (!xscr->wmname)
		return False;
	/* XXX: 2bwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "2bwm"))
		return True;
	/* XXX: adwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "adwm"))
		return True;
	/* XXX: awewm(1) does not really support desktops and is, therefore, not
	   supported.  (Welllll, it does.) */
	if (!strcasecmp(xscr->wmname, "aewm"))
		return True;
	/* XXX: afterstep(1) provides both workspaces and viewports (large desktops).
	   libwnck+ does not support these well, so when RESNAME detects that it is
	   running under afterstep(1), it does nothing.  (It has a desktop button proxy,
	   but it does not relay scroll wheel events by default.) */
	if (!strcasecmp(xscr->wmname, "afterstep"))
		return False;
	/* XXX: awesome(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "awesome"))
		return True;
	/* XXX: When running with bbkeys(1), blackbox(1) has its own window cycling
	   feedback.  When running under blackbox(1), xde-cycle does nothing. Otherwise,
	   blackbox(1) is largely supported and works well. */
	if (!strcasecmp(xscr->wmname, "blackbox")) {
		xscr->winds = False;
		xscr->cycle = False;
		return True;
	}
	/* XXX: bspwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "bspwm"))
		return True;
	/* XXX: ctwm(1) is only GNOME/WinWM compliant and is not yet supported by
	   libwnck+.  Use etwm(1) instead.  RESNAME mitigates this somewhat, so it is
	   still listed as supported. */
	if (!strcasecmp(xscr->wmname, "ctwm"))
		return True;
	/* XXX: cwm(1) is supported, but it doesn't work that well because cwm(1) is not
	   placing _NET_WM_STATE on client windows, so libwnck+ cannot locate them and
	   will not provide contents in the pager. */
	if (!strcasecmp(xscr->wmname, "cwm"))
		return True;
	/* XXX: dtwm(1) is only OSF/Motif compliant and does support multiple desktops;
	   however, libwnck+ does not yet support OSF/Motif/CDE.  This is not mitigated
	   by RESNAME. */
	if (!strcasecmp(xscr->wmname, "dtwm")) {
		xscr->pager = False;
		xscr->tasks = False;
		xscr->winds = False;
		xscr->cycle = False;
		xscr->setbg = False;
		xscr->start = False;
		return True;
	}
	/* XXX: dwm(1) is barely ICCCM compliant.  It is not supported. */
	if (!strcasecmp(xscr->wmname, "dwm")) {
		xscr->pager = False;
		xscr->tasks = False;
		xscr->winds = False;
		xscr->cycle = False;
		xscr->setbg = False;
		xscr->start = False;
		return True;
	}
	/* XXX: echinus(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "echinus"))
		return True;
	/* XXX: etwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "etwm"))
		return True;
	/* XXX: failsafewm(1) has no desktops and is not supported. */
	if (!strcasecmp(xscr->wmname, "failsafewm")) {
		xscr->pager = False;
		xscr->tasks = False;
		xscr->winds = False;
		xscr->cycle = False;
		xscr->setbg = False;
		xscr->start = False;
		return True;
	}
	/* XXX: fluxbox(1) provides its own window cycling feedback.  When running under
	   fluxbox(1), xde-cycle does nothing. Otherwise, fluxbox(1) is supported and
	   works well. */
	if (!strcasecmp(xscr->wmname, "fluxbox")) {
		xscr->tasks = False;
		xscr->cycle = False;
		xscr->winds = False;
		return True;
	}
	/* XXX: flwm(1) supports GNOME/WinWM but not EWMH/NetWM and is not currently
	   supported by libwnck+.  RESNAME mitigates this to some extent. */
	if (!strcasecmp(xscr->wmname, "flwm"))
		return True;
	/* XXX: fvwm(1) is supported and works well.  fvwm(1) provides a desktop button
	   proxy, but it needs the --noproxy option.  Viewports work better than on
	   afterstep(1) and behaviour is predictable. */
	if (!strcasecmp(xscr->wmname, "fvwm"))
		return True;
	/* XXX: herbstluftwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "herbstluftwm"))
		return True;
	/* XXX: i3(1) is supported; however, i3(1) does not support
	   _NET_NUMBER_OF_DESKTOPS, so only the current desktop is shown at any given
	   time, which makes it less effective. */
	if (!strcasecmp(xscr->wmname, "i3"))
		return True;
	/* XXX: icewm(1) provides its own pager on the panel, but does respect
	   _NET_DESKTOP_LAYOUT in some versions.  Although a desktop button proxy is
	   provided, older versions of icewm(1) will not proxy butt events sent by the
	   pager.  Use the version at https://github.com/bbidulock/icewm for best
	   results. */
	if (!strcasecmp(xscr->wmname, "icewm"))
		return True;
	/* XXX: jwm(1) provides its own pager on the panel, but does not respect or set
	   _NET_DESKTOP_LAYOUT, and key bindings are confused.  When RESNAME detects
	   that it is running under jwm(1) it will simply do nothing.  Otherwise, jwm(1)
	   is supported and works well. */
	if (!strcasecmp(xscr->wmname, "jwm")) {
		xscr->pager = False;
		return True;
	}
	/* XXX: matwm2(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "matwm2"))
		return True;
	/* XXX: metacity(1) provides its own competent desktop switching feedback pop-up.
	   When RESNAME detects that it is running under metacity(1), it will simply do
	   nothing. */
	if (!strcasecmp(xscr->wmname, "metacity")) {
		xscr->pager = False;
		xscr->tasks = False;
		xscr->cycle = False;
		xscr->winds = False;
		return True;
	}
	/* XXX: mwm(1) only supports OSF/Motif and does not support multiple desktops. It
	   is not supported. */
	if (!strcasecmp(xscr->wmname, "mwm"))
		return False;
	/* XXX: mutter(1) has not been tested. */
	if (!strcasecmp(xscr->wmname, "mutter"))
		return True;
	/* XXX: openbox(1) provides its own meager desktop switching feedback pop-up.  It
	   does respect _NET_DESKTOP_LAYOUT but does not provide any of the contents of
	   the desktop. When both are running it is a little confusing, so when
	   RESNAME detects that it is running under openbox(1), it will simply
	   do nothing. */
	if (!strcasecmp(xscr->wmname, "openbox")) {
		xscr->pager = False;
		xscr->cycle = False;
		xscr->winds = False;
		xscr->tasks = False;
		return True;
	}
	/* XXX: pekwm(1) provides its own broken desktop switching feedback pop-up;
	   however, it does not respect _NET_DESKTOP_LAYOUT and key bindings are
	   confused.  When RESNAME detects that it is running under pekwm(1), it will
	   simply do nothing. */
	if (!strcasecmp(xscr->wmname, "pekwm")) {
		xscr->pager = False;
		xscr->cycle = False;
		xscr->winds = False;
		xscr->tasks = False;
		return True;
	}
	/* XXX: spectrwm(1) is supported, but it doesn't work that well because, like
	   cwm(1), spectrwm(1) is not placing _NET_WM_STATE on client windows, so
	   libwnck+ cannot locate them and will not provide contents in the pager. */
	if (!strcasecmp(xscr->wmname, "spectrwm"))
		return True;
	/* XXX: twm(1) does not support multiple desktops and is not supported. */
	if (!strcasecmp(xscr->wmname, "twm")) {
		xscr->pager = False;
		xscr->tasks = False;
		xscr->cycle = False;
		xscr->winds = False;
		xscr->setbg = False;
		xscr->start = False;
		return True;
	}
	/* XXX: uwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "uwm"))
		return True;
	/* XXX: vtwm(1) is barely ICCCM compliant and currently unsupported: use etwm
	   instead. */
	if (!strcasecmp(xscr->wmname, "vtwm")) {
		xscr->pager = False;
		xscr->tasks = False;
		xscr->cycle = False;
		xscr->winds = False;
		xscr->setbg = False;
		xscr->start = False;
		return True;
	}
	/* XXX: waimea(1) is supported; however, waimea(1) defaults to triple-sized large
	   desktops in a 2x2 arrangement.  With large virtual desktops, libwnck+ gets
	   confused just as with afterstep(1).  fvwm(1) must be doing something right. It
	   appears to be _NET_DESKTOP_VIEWPORT, which is supposed to be set to the
	   viewport position of each desktop (and isn't).  Use the waimea at
	   https://github.com/bbidulock/waimea for a corrected version. */
	if (!strcasecmp(xscr->wmname, "waimea"))
		return True;
	/* XXX: wind(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "wind"))
		return True;
	/* XXX: wmaker(1) is supported and works well.  wmaker(1) has its own background
	   switcher and window cycling. */
	if (!strcasecmp(xscr->wmname, "wmaker")) {
		xscr->setbg = False;
		xscr->cycle = False;
		xscr->winds = False;
		return True;
	}
	/* XXX: wmii(1) is supported and works well.  wmii(1) was stealing the focus back
	   from the pop-up, but this was fixed. */
	if (!strcasecmp(xscr->wmname, "wmii"))
		return True;
	/* XXX: wmx(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "wmx"))
		return True;
	/* XXX: xdwm(1) does not support EWMH/NetWM for desktops. */
	if (!strcasecmp(xscr->wmname, "xdwm")) {	/* XXX */
		xscr->pager = False;
		xscr->setbg = False;
		return True;
	}
	/* XXX: yeahwm(1) does not support EWMH/NetWM and is currently unsupported.  The
	   pager will simply not do anything while this window manager is running. */
	if (!strcasecmp(xscr->wmname, "yeahwm")) {
		xscr->pager = False;
		xscr->tasks = False;
		xscr->cycle = False;
		xscr->winds = False;
		xscr->setbg = False;
		xscr->start = False;
		return True;
	}
	return True;
}

static void setup_button_proxy(XdeScreen *xscr);

static void
window_manager_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = user;
	const char *name;

	PTRACE(5);
	/* I suppose that what we should do here is set a timer and wait before doing
	   anything; however, I think that libwnck++ already does this (waits before even
	   giving us the signal). */
	wnck_screen_force_update(wnck);
	if (options.proxy)
		setup_button_proxy(xscr);
	free(xscr->wmname);
	xscr->wmname = NULL;
	xscr->goodwm = False;
	/* start with all True and let wm check set False */
	xscr->winds = options.show.winds;
	xscr->pager = options.show.pager;
	xscr->tasks = options.show.tasks;
	xscr->cycle = options.show.cycle;
	xscr->setbg = options.show.setbg;
	xscr->start = options.show.start;
	xscr->input = options.show.input;
	if ((name = wnck_screen_get_window_manager_name(wnck))) {
		xscr->wmname = strdup(name);
		*strchrnul(xscr->wmname, ' ') = '\0';
		/* Some versions of wmx have an error in that they only set the
		   _NET_WM_NAME to the first letter of wmx. */
		if (!strcmp(xscr->wmname, "w")) {
			free(xscr->wmname);
			xscr->wmname = strdup("wmx");
		}
		/* Ahhhh, the strange naming of μwm...  Unfortunately there are several
		   ways to make a μ in utf-8!!! */
		if (!strcmp(xscr->wmname, "\xce\xbcwm") || !strcmp(xscr->wmname, "\xc2\xb5wm")) {
			free(xscr->wmname);
			xscr->wmname = strdup("uwm");
		}
		if (!(xscr->goodwm = good_window_manager(xscr))) {
			xscr->winds = False;
			xscr->pager = False;
			xscr->tasks = False;
			xscr->cycle = False;
			xscr->setbg = False;
			xscr->start = False;
			xscr->input = False;
		}
	}
	DPRINTF(1, "window manager is '%s'\n", xscr->wmname);
	DPRINTF(1, "window manager is %s\n", xscr->goodwm ? "usable" : "unusable");
}

static void
workspace_destroyed(WnckScreen *wnck, WnckWorkspace *space, gpointer data)
{
	/* pager can handle this on its own */
}

static void
workspace_created(WnckScreen *wnck, WnckWorkspace *space, gpointer data)
{
	/* pager can handle this on its own */
}

static void
viewports_changed(WnckScreen *wnck, gpointer data)
{
	/* pager can handle this on its own */
}

static void
background_changed(WnckScreen *wnck, gpointer data)
{
	/* XXX: might have setbg do something here */
}

static void
active_workspace_changed(WnckScreen *wnck, WnckWorkspace *prev, gpointer data)
{
	/* XXX: should be handled by update_current_desktop */
}
#endif

/** @} */

/** @section Specific Window Events
  * @{ */

#if 0
static void
actions_changed(WnckWindow *window, WnckWindowActions changed, WnckWindowActions state, gpointer xscr)
{
}

static void
geometry_changed(WnckWindow *window, gpointer xscr)
{
}

static void
icon_changed(WnckWindow *window, gpointer xscr)
{
}

static void
name_changed(WnckWindow *window, gpointer xscr)
{
}

static void
state_changed(WnckWindow *window, WnckWindowState changed, WnckWindowState state, gpointer xscr)
{
}

static void
workspace_changed(WnckWindow *window, gpointer xscr)
{
}
#endif

/** @} */

/** @section Window Events
  * @{ */

#if 0
static WnckWorkspace *
same_desk(WnckScreen *wnck, WnckWindow *win1, WnckWindow *win2)
{
	WnckWorkspace *desk;

	if (((desk = wnck_window_get_workspace(win1)) && wnck_window_is_on_workspace(win2, desk)) ||
	    ((desk = wnck_window_get_workspace(win2)) && wnck_window_is_on_workspace(win1, desk)) ||
	    ((desk = wnck_screen_get_active_workspace(wnck)) &&
	     wnck_window_is_on_workspace(win2, desk) && wnck_window_is_on_workspace(win1, desk)))
		return (desk);
	return (NULL);
}

static gboolean
is_cyclic(WnckWorkspace *desk, WnckWindow *prev, WnckWindow *actv, GList *list)
{
	GList *l, *lprev = NULL, *lactv = NULL;

	for (l = list; l; l = l->next) {
		if (l->data == prev)
			lprev = l;
		if (l->data == actv)
			lactv = l;
	}
	if (lprev && lactv) {
		for (l = lprev->next ? : list; l; l = l->next ? : list) {
			WnckWindow *curr = l->data;

			if (l == lactv)
				return TRUE;
			if (!wnck_window_is_visible_on_workspace(curr, desk))
				continue;
			if (wnck_window_is_skip_tasklist(curr))
				continue;
			break;
		}
		for (l = lactv->next ? : list; l; l = l->next ? : list) {
			WnckWindow *curr = l->data;

			if (l == lprev)
				return TRUE;
			if (!wnck_window_is_visible_on_workspace(curr, desk))
				continue;
			if (wnck_window_is_skip_tasklist(curr))
				continue;
			break;
		}
	}
	return FALSE;
}

static XdeMonitor *
is_cycle(XdeScreen *xscr, WnckScreen *wnck, WnckWindow *prev, WnckWindow *actv)
{
	XdeMonitor *xmon = NULL;
	WnckWorkspace *desk;
	GdkWindow *wp, *wa;
	int mp, ma;

	if (!prev || !actv)
		return (NULL);
	if (prev == actv)
		return (NULL);
	wp = gdk_x11_window_foreign_new_for_display(disp, wnck_window_get_xid(prev));
	wa = gdk_x11_window_foreign_new_for_display(disp, wnck_window_get_xid(actv));
	if (!wp || !wa) {
		if (wp)
			g_object_unref(G_OBJECT(wp));
		if (wa)
			g_object_unref(G_OBJECT(wa));
		return (NULL);
	}
	mp = gdk_screen_get_monitor_at_window(xscr->scrn, wp);
	ma = gdk_screen_get_monitor_at_window(xscr->scrn, wa);
	if (mp != ma) {
		g_object_unref(G_OBJECT(wp));
		g_object_unref(G_OBJECT(wa));
		return (NULL);
	}
	xmon = &xscr->mons[ma];

	if (!(desk = same_desk(wnck, prev, actv)))
		return (NULL);
	if (options.order != WindowOrderStacking)
		if (is_cyclic(desk, prev, actv, wnck_screen_get_windows(wnck)))
			return (xmon);
	if (options.order != WindowOrderClient)
		if (is_cyclic(desk, prev, actv, wnck_screen_get_windows_stacked(wnck)))
			return (xmon);
	return (NULL);
}

/** @brief active window changed
  *
  * The active window changing only affects the cycling window.  We should only
  * pop the cycle window when the window has cycled according to criteria.
  */
static void
active_window_changed(WnckScreen *wnck, WnckWindow *prev, gpointer user)
{
	static const GdkModifierType buttons =
	    (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK);
	static const GdkModifierType dontcare = (GDK_SHIFT_MASK | GDK_LOCK_MASK | GDK_RELEASE_MASK);
	XdeScreen *xscr = user;
	XdeMonitor *xmon = NULL;
	GdkModifierType mask = 0;
	WnckWindow *actv;
	int i;

	if (!options.show.cycle && !options.show.tasks && !options.show.winds)
		return;
	gdk_display_get_pointer(disp, NULL, NULL, NULL, &mask);
	/* if button down, do nothing */
	if (mask & buttons)
		return;
	if (mask & ~(buttons | dontcare)) {
		actv = wnck_screen_get_active_window(wnck);
		if (actv && prev)
			xmon = is_cycle(xscr, wnck, prev, actv);
	}
	if (!xmon) {
		for (i = 0; i < xscr->nmon; i++) {
			if (options.show.cycle) {
				stop_popup_timer(&xscr->mons[i].cycle);
				drop_popup(&xscr->mons[i].cycle);
			}
			if (options.show.tasks) {
				stop_popup_timer(&xscr->mons[i].tasks);
				drop_popup(&xscr->mons[i].tasks);
			}
			if (options.show.winds) {
				stop_popup_timer(&xscr->mons[i].winds);
				drop_popup(&xscr->mons[i].winds);
			}
		}
		return;
	}
	if (xscr->cycle)
		show_popup(xscr, &xmon->cycle, TRUE, TRUE);
	if (xscr->tasks)
		show_popup(xscr, &xmon->tasks, TRUE, TRUE);
	if (xscr->winds)
		show_popup(xscr, &xmon->winds, TRUE, TRUE);
	return;
}

static void
clients_changed(WnckScreen *wnck, XdeScreen *xscr)
{
}

static void
application_closed(WnckScreen *wnck, WnckApplication * app, gpointer xscr)
{
	clients_changed(wnck, xscr);
}

static void
application_opened(WnckScreen *wnck, WnckApplication * app, gpointer xscr)
{
	clients_changed(wnck, xscr);
}

static void
class_group_closed(WnckScreen *wnck, WnckClassGroup * class_group, gpointer xscr)
{
	clients_changed(wnck, xscr);
}

static void
class_group_opened(WnckScreen *wnck, WnckClassGroup * class_group, gpointer xscr)
{
	clients_changed(wnck, xscr);
}

static void
window_closed(WnckScreen *wnck, WnckWindow *window, gpointer xscr)
{
	clients_changed(wnck, xscr);
}

static void
window_opened(WnckScreen *wnck, WnckWindow *window, gpointer xscr)
{
	g_signal_connect(G_OBJECT(window), "actions_changed", G_CALLBACK(actions_changed), xscr);
	g_signal_connect(G_OBJECT(window), "geometry_changed", G_CALLBACK(geometry_changed), xscr);
	g_signal_connect(G_OBJECT(window), "icon_changed", G_CALLBACK(icon_changed), xscr);
	g_signal_connect(G_OBJECT(window), "name_changed", G_CALLBACK(name_changed), xscr);
	g_signal_connect(G_OBJECT(window), "state_changed", G_CALLBACK(state_changed), xscr);
	g_signal_connect(G_OBJECT(window), "workspace_changed", G_CALLBACK(workspace_changed), xscr);
	clients_changed(wnck, xscr);
}

static void
window_stacking_changed(WnckScreen *wnck, gpointer xscr)
{
	clients_changed(wnck, xscr);
}

static void
showing_desktop_changed(WnckScreen *wnck, gpointer xscr)
{
}
#endif

/** @} */

/** @} */

/** @section X Event Handlers
  * @{ */

#ifdef STARTUP_NOTIFICATION
static void
sn_handler(SnMonitorEvent *event, void *data)
{
	SnStartupSequence *sn_seq = NULL;
	XdeScreen *xscr = data;
	Sequence *seq;
	const char *id;

	sn_seq = sn_monitor_event_get_startup_sequence(event);

	id = sn_startup_sequence_get_id(sn_seq);
	switch (sn_monitor_event_get_type(event)) {
	case SN_MONITOR_EVENT_INITIATED:
		add_sequence(xscr, id, sn_seq);
		break;
	case SN_MONITOR_EVENT_CHANGED:
		if ((seq = find_sequence(xscr, id)))
			cha_sequence(xscr, seq);
		break;
	case SN_MONITOR_EVENT_COMPLETED:
	case SN_MONITOR_EVENT_CANCELED:
		if ((seq = find_sequence(xscr, id)))
			rem_sequence(xscr, seq);
		break;
	}
}
#endif				/* STARTUP_NOTIFICATION */

#if 0
/** @brief refresh desktop
  *
  * The current desktop has changed for the screen.  Update the root pixmaps for
  * the screen.  Whether the window manager is multihead aware can be determined
  * by checking the mhaware boolean on the screen structure.
  */
static void
refresh_desktop(XdeScreen *xscr)
{
	GdkWindow *root = gdk_screen_get_root_window(xscr->scrn);
	GdkColormap *cmap = gdk_drawable_get_colormap(GDK_DRAWABLE(root));
	GdkPixmap *pixmap;
	cairo_t *cr;
	XdeMonitor *xmon;
	XdeImage *im;
	XdePixmap *pm;
	int d, m;
	Pixmap pmap;

	/* render the current desktop on the screen */
	pmap = get_temporary_pixmap(xscr);
	DPRINTF(1, "using temporary pixmap (0x%08lx)\n", pmap);
	DPRINTF(1, "creating temporary pixmap contents\n");
	pixmap = gdk_pixmap_foreign_new_for_display(disp, pmap);
	gdk_drawable_set_colormap(GDK_DRAWABLE(pixmap), cmap);
	cr = gdk_cairo_create(GDK_DRAWABLE(pixmap));
	for (m = 0, xmon = xscr->mons; m < xscr->nmon; m++, xmon++) {
		DPRINTF(1, "adding monitor %d to pixmap\n", m);
		d = xmon->current;
		DPRINTF(1, "monitor %d current destop is %d\n", m, d);
		if ((im = xscr->backdrops[d]) && (im->pixbuf || im->file)) {
			DPRINTF(1, "monitor %d desktop %d has an image\n", m, d);
			for (pm = im->pixmaps; pm; pm = pm->next) {
				if (pm->geom.width == xmon->geom.width && pm->geom.height == xmon->geom.height)
					break;
			}
			if (!pm && !im->pixbuf) {
				DPRINTF(1, "creating pixbuf from file %s\n", im->file);
				im->pixbuf = gdk_pixbuf_new_from_file(im->file, NULL);
			}
			if (!pm && im->pixbuf) {
				GdkPixbuf *scaled;
				cairo_t *cr;

				DPRINTF(1, "allocating a new pixmap for image\n");
				pm = calloc(1, sizeof(*pm));
				pm->refs = 1;
				pm->index = im->index;
				if ((pm->next = im->pixmaps))
					pm->next->pprev = &pm->next;
				pm->pprev = &im->pixmaps;
				im->pixmaps = pm;
				pm->geom = xmon->geom;
				pm->pixmap =
				    gdk_pixmap_new(GDK_DRAWABLE(root), xmon->geom.width, xmon->geom.height, -1);
				gdk_drawable_set_colormap(GDK_DRAWABLE(pm->pixmap), cmap);
				cr = gdk_cairo_create(GDK_DRAWABLE(pm->pixmap));
				/* FIXME: tiling and other things.... */
				scaled = gdk_pixbuf_scale_simple(im->pixbuf,
								 pm->geom.width,
								 pm->geom.height, GDK_INTERP_BILINEAR);
				gdk_cairo_set_source_pixbuf(cr, scaled, 0, 0);
				cairo_paint(cr);
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_destroy(cr);
				g_object_unref(scaled);
				if (im->pixbuf && im->file) {
					g_object_unref(im->pixbuf);
					im->pixbuf = NULL;
					/* save some resident memory */
				}
			} else
				DPRINTF(1, "using existing pixmap for image\n");
			gdk_cairo_rectangle(cr, &xmon->geom);
			if (pm) {
				DPRINTF(1, "painting pixmap into screen image\n");
				gdk_cairo_set_source_pixmap(cr, pm->pixmap, 0, 0);
				cairo_pattern_set_extend(cairo_get_source(cr), CAIRO_EXTEND_REPEAT);
				cairo_paint(cr);
			} else {
				/* FIXME: use color for desktop */
				DPRINTF(1, "painting color into screen image\n");
				cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
				cairo_paint(cr);
			}
		} else {
			DPRINTF(1, "monitor %d desktop %d has no image\n", m, d);
		}
	}
	cairo_destroy(cr);
	DPRINTF(1, "installing pixmap 0x%08lx as root pixmap\n", pmap);
	XChangeProperty(GDK_DISPLAY_XDISPLAY(disp), GDK_WINDOW_XID(root), _XA_XROOTPMAP_ID,
			XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pmap, 1);
	gdk_window_set_back_pixmap(root, pixmap, FALSE);
	gdk_window_clear(root);
	if (xscr->pixmap) {
#if 1
		DPRINTF(0, "freeing old unused temporary pixmap 0x%08lx\n", xscr->pixmap);
		XFreePixmap(GDK_DISPLAY_XDISPLAY(disp), xscr->pixmap);
		XKillClient(GDK_DISPLAY_XDISPLAY(disp), AllTemporary);
#else
		DPRINTF(0, "killing old unused temporary pixmap 0x%08lx\n", xscr->pixmap);
		XKillClient(GDK_DISPLAY_XDISPLAY(disp), xscr->pixmap);
#endif
	}
	xscr->pixmap = pmap;
}

/** @brief refresh monitor
  *
  * The current view for a multi-head aware window manager has changed.  Update
  * the background for the monitor.
  */
static void
refresh_monitor(XdeMonitor *xmon)
{
	/* for now */
	refresh_desktop(xmon->xscr);
}
#endif

static void
update_client_list(XdeScreen *xscr, Atom prop)
{
#if 0
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0, num;
	unsigned long *data = NULL;
	GdkWindow *window;
	GdkWindowState state;
	GList *oldlist = NULL, **moldlist;
	GList *clients = NULL, **mclients;
	GList *stacked = NULL, **mstacked;
	GList *l;
	int i, m;

	if (prop == None || prop == _XA_WIN_CLIENT_LIST) {
	      try_harder1:
		if (XGetWindowProperty(dpy, root, _XA_WIN_CLIENT_LIST, 0L, 16,
				       False, XA_CARDINAL, &actual, &format,
				       &nitems, &after, (unsigned char **) &data)
		    == Success && format == 32) {
			if (after) {
				num += ((after + 1) >> 2);
				if (data) {
					XFree(data);
					data = NULL;
				}
				goto try_harder1;
			}
			for (i = 0; i < nitems; i++)
				if ((window = gdk_x11_window_foreign_new_for_display(disp, data[i])))
					oldlist = g_list_append(oldlist, window);
			if (data)
				XFree(data);
		}
	}
	if (prop == None || prop == _XA_NET_CLIENT_LIST) {
	      try_harder2:
		if (XGetWindowProperty(dpy, root, _XA_NET_CLIENT_LIST, 0L, 16,
				       False, XA_WINDOW, &actual, &format,
				       &nitems, &after, (unsigned char **) &data)
		    == Success && format == 32) {
			if (after) {
				num += ((after + 1) >> 2);
				if (data) {
					XFree(data);
					data = NULL;
				}
				goto try_harder2;
			}
			for (i = 0; i < nitems; i++)
				if ((window = gdk_x11_window_foreign_new_for_display(disp, data[i])))
					clients = g_list_append(clients, window);
			if (data)
				XFree(data);
		}
	}
	if (prop == None || prop == _XA_NET_CLIENT_LIST_STACKING) {
	      try_harder3:
		if (XGetWindowProperty(dpy, root, _XA_NET_CLIENT_LIST, 0L, 16,
				       False, XA_WINDOW, &actual, &format,
				       &nitems, &after, (unsigned char **) &data)
		    == Success && format == 32) {
			if (after) {
				num += ((after + 1) >> 2);
				if (data) {
					XFree(data);
					data = NULL;
				}
				goto try_harder3;
			}
			for (i = 0; i < nitems; i++)
				if ((window = gdk_x11_window_foreign_new_for_display(disp, data[i])))
					stacked = g_list_append(stacked, window);
			if (data)
				XFree(data);
		}
	}
	moldlist = calloc(xscr->nmon, sizeof(*moldlist));
	for (i = 0; i < xscr->nmon; i++) {
		for (l = oldlist; l; l = l->next) {
			window = l->data;
			state = gdk_window_get_state(window);
			if (state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED))
				continue;
			m = gdk_screen_get_monitor_at_window(xscr->scrn, window);
			g_object_ref(G_OBJECT(window));
			moldlist[m] = g_list_append(moldlist[m], window);
		}
	}
	mclients = calloc(xscr->nmon, sizeof(*mclients));
	for (i = 0; i < xscr->nmon; i++) {
		for (l = clients; l; l = l->next) {
			window = l->data;
			state = gdk_window_get_state(window);
			if (state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED))
				continue;
			m = gdk_screen_get_monitor_at_window(xscr->scrn, window);
			g_object_ref(G_OBJECT(window));
			mclients[m] = g_list_append(mclients[m], window);
		}
	}
	mstacked = calloc(xscr->nmon, sizeof(*mstacked));
	for (i = 0; i < xscr->nmon; i++) {
		for (l = stacked; l; l = l->next) {
			window = l->data;
			state = gdk_window_get_state(window);
			if (state & (GDK_WINDOW_STATE_WITHDRAWN | GDK_WINDOW_STATE_ICONIFIED))
				continue;
			m = gdk_screen_get_monitor_at_window(xscr->scrn, window);
			g_object_ref(G_OBJECT(window));
			mstacked[m] = g_list_append(mstacked[m], window);
		}
	}
#endif
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

#if 0
static void
update_screen_size(XdeScreen *xscr, int new_width, int new_height)
{
}

static void
create_monitor(XdeScreen *xscr, XdeMonitor *xmon, int m)
{
	memset(xmon, 0, sizeof(*xmon));
	xmon->index = m;
	xmon->xscr = xscr;
	gdk_screen_get_monitor_geometry(xscr->scrn, m, &xmon->geom);
}

static void
delete_monitor(XdeScreen *xscr, XdeMonitor *mon, int m)
{
}

static void
update_monitor(XdeScreen *xscr, XdeMonitor *mon, int m)
{
	gdk_screen_get_monitor_geometry(xscr->scrn, m, &mon->geom);
}
#endif

#if 0
static void
update_root_pixmap(XdeScreen *xscr, Atom prop)
{
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;
	Pixmap pmap = None;

	PTRACE(5);
	if (prop == None || prop == _XA_ESETROOT_PMAP_ID) {
		if (XGetWindowProperty
		    (dpy, root, _XA_ESETROOT_PMAP_ID, 0, 1, False, AnyPropertyType, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 1 && data) {
			pmap = data[0];
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (prop == None || prop == _XA_XROOTPMAP_ID) {
		if (XGetWindowProperty
		    (dpy, root, _XA_XROOTPMAP_ID, 0, 1, False, AnyPropertyType, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 1 && data) {
			pmap = data[0];
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (pmap && xscr->pixmap != pmap) {
		DPRINTF(1, "root pixmap changed from 0x%08lx to 0x%08lx\n", xscr->pixmap, pmap);
		xscr->pixmap = pmap;
		/* FIXME: do more */
		/* Adjust the style of the desktop to use the pixmap specified by
		   _XROOTPMAP_ID as the background.  Uses GTK+ 2.0 styles to do this. The
		   root _XROOTPMAP_ID must be retrieved before calling this function for
		   it to work correctly.  */
	}
}
#endif

#if 1
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
	char *name = NULL;
	Bool gotone = False;

	DPRINTF(1, "updating current desktop for %s\n", prop ? (name = XGetAtomName(dpy, prop)) : "None");

	PTRACE(5);
	current = calloc(xscr->nmon + 1, sizeof(*current));

	if (prop == None || prop == _XA_WM_DESKTOP) {
		if (XGetWindowProperty(dpy, root, _XA_WM_DESKTOP, 0, 64, False,
				       _XA_WM_DESKTOP, &actual, &format, &nitems, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && nitems >= 1 && data) {
			gotone = True;
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
			gotone = True;
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
			gotone = True;
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
	if (gotone) {
		/* There are two things to do when the workspace changes: */
		/* First off, drop any cycle or task windows that we have open. */
		/* Second, queue deferred action to refresh pixmaps on the desktop. */
		/* Third, pop the pager window. */
		if (xscr->current != current[0]) {
			DPRINTF(1, "Current desktop for screen %d changed from %d to %lu\n", xscr->index,
				xscr->current, current[0]);
			xscr->current = current[0];
		}
		for (i = 0, xmon = xscr->mons; i < xscr->nmon; i++, xmon++) {
			if (xmon->current != current[i + 1]) {
				DPRINTF(1, "Current view for monitor %d changed from %d to %lu\n", xmon->index,
					xmon->current, current[i + 1]);
				xmon->current = current[i + 1];
			}
		}
	}
	free(current);
	if (name)
		XFree(name);
}

#endif

#if 1
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
#if 0
		// refresh_layout(xscr); /* XXX: should be deferred */
		add_deferred_refresh_layout(xscr);
#endif
	}
}
#endif

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
		} else {
			char *name = NULL;

			EPRINTF("could not get text list for %s property\n", (name = XGetAtomName(dpy, prop)));
			if (name)
				XFree(name);
		}
		if (xtp.value)
			XFree(xtp.value);
	} else {
		char *name = NULL;

		DPRINTF(1, "could not get %s for root 0x%lx\n", (name = XGetAtomName(dpy, prop)), root);
		if (name)
			XFree(name);
	}
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
	} else
		DPRINTF(1, "No change in current icon theme %s\n", xscr->itheme);
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
		} else {
			char *name = NULL;

			EPRINTF("could not get text list for %s property\n", (name = XGetAtomName(dpy, prop)));
			if (name)
				XFree(name);
		}
		if (xtp.value)
			XFree(xtp.value);
	} else {
		char *name = NULL;

		DPRINTF(1, "could not get %s for root 0x%lx\n", (name = XGetAtomName(dpy, prop)), root);
		if (name)
			XFree(name);
	}
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

#if 0
static void
update_screen(XdeScreen *xscr)
{
#if 1
	if (options.show.setbg)
		update_root_pixmap(xscr, None);
	update_layout(xscr, None);
	update_current_desktop(xscr, None);
#endif
	update_theme(xscr, None);
	update_icon_theme(xscr, None);
}

static void
refresh_screen(XdeScreen *xscr, GdkScreen *scrn)
{
	XdeMonitor *mon;
	int m, nmon, width, height, index;

	index = gdk_screen_get_number(scrn);
	if (xscr->index != index) {
		EPRINTF("Arrrghhh! screen index changed from %d to %d\n", xscr->index, index);
		xscr->index = index;
	}
	if (xscr->scrn != scrn) {
		DPRINTF(1, "Arrrghhh! screen pointer changed from %p to %p\n", xscr->scrn, scrn);
		xscr->scrn = scrn;
	}
	width = gdk_screen_get_width(scrn);
	height = gdk_screen_get_height(scrn);
	DPRINTF(1, "Screen %d dimensions are %dx%d\n", index, width, height);
	if (xscr->width != width || xscr->height != height) {
		DPRINTF(1, "Screen %d dimensions changed %dx%d -> %dx%d\n", index,
			xscr->width, xscr->height, width, height);
		/* FIXME: reset size of screen */
		update_screen_size(xscr, width, height);
		xscr->width = width;
		xscr->height = height;
	}
	nmon = gdk_screen_get_n_monitors(scrn);
	DPRINTF(1, "Reallocating %d monitors\n", nmon);
	xscr->mons = realloc(xscr->mons, nmon * sizeof(*xscr->mons));
	if (nmon > xscr->nmon) {
		DPRINTF(1, "Screen %d number of monitors increased from %d to %d\n", index, xscr->nmon, nmon);
		for (m = xscr->nmon; m < nmon; m++) {
			mon = xscr->mons + m;
			create_monitor(xscr, mon, m);
		}
	} else if (nmon < xscr->nmon) {
		DPRINTF(1, "Screen %d number of monitors decreased from %d to %d\n", index, xscr->nmon, nmon);
		for (m = nmon; m < xscr->nmon; m++) {
			mon = xscr->mons + m;
			delete_monitor(xscr, mon, m);
		}
	}
	if (nmon != xscr->nmon)
		xscr->nmon = nmon;
	for (m = 0, mon = xscr->mons; m < nmon; m++, mon++)
		update_monitor(xscr, mon, m);
	update_screen(xscr);
}

/** @brief monitors changed
  *
  * Emitted when the number, size or position of the monitors attached to the screen change.  The
  * number and/or size of monitors belonging to a screen have changed.  This may be as a result of
  * RANDR or XINERAMA changes.  Walk through the monitors and adjust the necessary parameters.
  */
static void
monitors_changed(GdkScreen *scrn, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	wnck_screen_force_update(xscr->wnck);
	refresh_screen(xscr, scrn);
#if 1
	refresh_layout(xscr);
	if (options.show.setbg)
		read_theme(xscr);
#endif
}

/** @brief screen size changed
  *
  * The size (pixel width or height) of the screen changed.  This may be as a result of RANDR or
  * XINERAMA changes.  Walk through the screen and the monitors on the screen and adjust the
  * necessary parameters.
  */
static void
size_changed(GdkScreen *scrn, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	wnck_screen_force_update(xscr->wnck);
	refresh_screen(xscr, scrn);
#if 1
	refresh_layout(xscr);
	if (options.show.setbg)
		read_theme(xscr);
#endif
}

static void
popup_show(XdeScreen *xscr)
{
	menu_show(xscr);
}
#endif

static GdkFilterReturn
event_handler_ClientMessage(XEvent *xev)
{
	XdeScreen *xscr = NULL;
	int s, nscr = ScreenCount(dpy);
	Atom type = xev->xclient.message_type;
	char *name = NULL;

	PTRACE(5);
	for (s = 0; s < nscr; s++)
		if (xev->xclient.window == RootWindow(dpy, s)) {
			xscr = screens + s;
			break;
		}
	if (!xscr) {
#ifdef STARTUP_NOTIFICATION
		if (type != _XA_NET_STARTUP_INFO && type != _XA_NET_STARTUP_INFO_BEGIN)
#endif
			EPRINTF("could not find screen for client message %s with window 0%08lx\n",
				name ? : (name = XGetAtomName(dpy, type)), xev->xclient.window);
		xscr = screens;
	}
	if (options.debug > 1) {
		fprintf(stderr, "==> ClientMessage: %p\n", xscr);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xclient.window);
		fprintf(stderr, "    --> message_type = %s\n", name ? : (name = XGetAtomName(dpy, type)));
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
	if (name) {
		XFree(name);
		name = NULL;
	}
	if (type == _XA_GTK_READ_RCFILES) {
		update_theme(xscr, type);
		update_icon_theme(xscr, type);
		return GDK_FILTER_REMOVE;	/* event handled */
#if 0
	} else if (type == _XA_PREFIX_REFRESH) {
		set_scmon(xev->xclient.data.l[1]);
		set_flags(xev->xclient.data.l[2]);
		set_word1(xev->xclient.data.l[3]);
		set_word2(xev->xclient.data.l[4]);
		popup_refresh(xscr);
		return GDK_FILTER_REMOVE;
	} else if (type == _XA_PREFIX_RESTART) {
		set_scmon(xev->xclient.data.l[1]);
		set_flags(xev->xclient.data.l[2]);
		set_word1(xev->xclient.data.l[3]);
		set_word2(xev->xclient.data.l[4]);
		popup_restart();
		return GDK_FILTER_REMOVE;
	} else if (type == _XA_PREFIX_POPMENU) {
		set_scmon(xev->xclient.data.l[1]);
		set_flags(xev->xclient.data.l[2]);
		set_word1(xev->xclient.data.l[3]);
		set_word2(xev->xclient.data.l[4]);
		menu_show(xscr);
		return GDK_FILTER_REMOVE;
	} else if (type == _XA_PREFIX_EDITOR) {
		set_scmon(xev->xclient.data.l[1]);
		set_flags(xev->xclient.data.l[2]);
		set_word1(xev->xclient.data.l[3]);
		set_word2(xev->xclient.data.l[4]);
		popup_show(xscr);
		return GDK_FILTER_REMOVE;
#endif
	}
#ifdef STARTUP_NOTIFICATION
	if (type == _XA_NET_STARTUP_INFO) {
		return sn_display_process_event(sn_dpy, xev);
	} else if (type == _XA_NET_STARTUP_INFO_BEGIN) {
		return sn_display_process_event(sn_dpy, xev);
	}
#endif
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
event_handler_SelectionClear(XEvent *xev, XdeScreen *xscr)
{
	PTRACE(5);
	if (options.debug > 1) {
		char *name = NULL;

		fprintf(stderr, "==> SelectionClear: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n", xev->xselectionclear.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xselectionclear.window);
		fprintf(stderr, "    --> selection = %s\n", (name = XGetAtomName(dpy, xev->xselectionclear.selection)));
		fprintf(stderr, "    --> time = %lu\n", xev->xselectionclear.time);
		fprintf(stderr, "<== SelectionClear: %p\n", xscr);
		if (name)
			XFree(name);
	}
	if (xscr && xev->xselectionclear.window == xscr->selwin) {
		XDestroyWindow(dpy, xscr->selwin);
		EPRINTF("selection cleared, exiting\n");
#if 0
		if (smcConn) {
			/* Care must be taken where if we are running under a session
			   manager. We set the restart hint to SmRestartImmediately which
			   means that the session manager will re-execute us if we exit.
			   We should really request a local shutdown. */
			SmcRequestSaveYourself(smcConn, SmSaveLocal, True, SmInteractStyleNone, False, False);
			return GDK_FILTER_CONTINUE;
		}
		exit(EXIT_SUCCESS);
#endif
		mainloop_quit();
		return GDK_FILTER_REMOVE;
	}
	if (xscr && xev->xselectionclear.window == xscr->laywin) {
		XDestroyWindow(dpy, xscr->laywin);
		xscr->laywin = None;
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
selwin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = data;

	PTRACE(5);
	switch (xev->type) {
	case SelectionClear:
		return event_handler_SelectionClear(xev, xscr);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;
}

#if 1
static GdkFilterReturn
laywin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = data;

	PTRACE(5);
	switch (xev->type) {
	case SelectionClear:
		return event_handler_SelectionClear(xev, xscr);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;
}
#endif

static GdkFilterReturn
event_handler_PropertyNotify(XEvent *xev, XdeScreen *xscr)
{
	Atom atom;

	PTRACE(5);
	if (options.debug > 2) {
		char *name = NULL;

		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", (name = XGetAtomName(dpy, xev->xproperty.atom)));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n",
			(xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
		if (name)
			XFree(name);
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
#if 0
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
	mainloop_quit();
	return G_SOURCE_CONTINUE;
}

/** @} */

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

#ifdef STARTUP_NOTIFICATION
	sn_dpy = sn_display_new(dpy, NULL, NULL);
#endif

	atom = gdk_atom_intern_static_string("MANAGER");
	_XA_MANAGER = gdk_x11_atom_to_xatom_for_display(disp, atom);

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
	atom = gdk_atom_intern_static_string("_NET_STARTUP_INFO");
	_XA_NET_STARTUP_INFO = gdk_x11_atom_to_xatom_for_display(disp, atom);
#ifdef STARTUP_NOTIFICATION
	gdk_display_add_client_message_filter(disp, atom, client_handler, NULL);
#endif				/* STARTUP_NOTIFICATION */

	atom = gdk_atom_intern_static_string("_NET_STARTUP_INFO_BEGIN");
	_XA_NET_STARTUP_INFO_BEGIN = gdk_x11_atom_to_xatom_for_display(disp, atom);
#ifdef STARTUP_NOTIFICATION
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

	atom = gdk_atom_intern_static_string(XA_PREFIX "_POPMENU");
	_XA_PREFIX_POPMENU = gdk_x11_atom_to_xatom_for_display(disp, atom);
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
init_screens(Window selwin)
{
	XdeMonitor *xmon = NULL;
	XdeScreen *xscr;
	int s, nscr;

	nscr = gdk_display_get_n_screens(disp);
	screens = calloc(nscr, sizeof(*screens));

	if (selwin) {
		GdkWindow *sel;

		sel = gdk_x11_window_foreign_new_for_display(disp, selwin);
		gdk_window_add_filter(sel, selwin_handler, screens);
		g_object_unref(G_OBJECT(sel));
	}
	gdk_window_add_filter(NULL, events_handler, NULL);

	for (s = 0, xscr = screens; s < nscr; s++, xscr++) {
		xscr->index = s;
		if (selwin) {
			char selection[65] = { 0, };

			snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
			xscr->atom = XInternAtom(dpy, selection, False);
			xscr->selwin = selwin;
		}
		xscr->scrn = gdk_display_get_screen(disp, s);
		xscr->root = gdk_screen_get_root_window(xscr->scrn);
		xscr->width = gdk_screen_get_width(xscr->scrn);
		xscr->height = gdk_screen_get_height(xscr->scrn);
#ifdef STARTUP_NOTIFICATION
		xscr->ctx = sn_monitor_context_new(sn_dpy, s, &sn_handler, xscr, NULL);
#endif
		gdk_window_add_filter(xscr->root, root_handler, xscr);
		init_wnck(xscr);
		init_monitors(xscr);
#if 1
#if 0
		if (options.proxy)
			setup_button_proxy(xscr);
		if (options.show.setbg)
			update_root_pixmap(xscr, None);
#endif
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

Window
get_desktop_layout_selection(XdeScreen *xscr)
{
	Window root = RootWindow(dpy, xscr->index);
	char selection[64] = { 0, };
	GdkWindow *lay;
	Window owner;
	Atom atom;

	if (xscr->laywin)
		return None;

	xscr->laywin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XSelectInput(dpy, xscr->laywin, StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask);
	lay = gdk_x11_window_foreign_new_for_display(disp, xscr->laywin);
	gdk_window_add_filter(lay, laywin_handler, xscr);
	snprintf(selection, sizeof(selection), XA_NET_DESKTOP_LAYOUT, xscr->index);
	atom = XInternAtom(dpy, selection, False);
	if (!(owner = XGetSelectionOwner(dpy, atom)))
		DPRINTF(1, "No owner for %s\n", selection);
	XSetSelectionOwner(dpy, atom, xscr->laywin, options.timestamp);
	XSync(dpy, False);

	if (xscr->laywin) {
		XEvent ev;

		ev.xclient.type = ClientMessage;
		ev.xclient.serial = 0;
		ev.xclient.send_event = False;
		ev.xclient.display = dpy;
		ev.xclient.window = root;
		ev.xclient.message_type = _XA_MANAGER;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = options.timestamp;
		ev.xclient.data.l[1] = atom;
		ev.xclient.data.l[2] = xscr->laywin;
		ev.xclient.data.l[3] = 0;
		ev.xclient.data.l[4] = 0;

		XSendEvent(dpy, root, False, StructureNotifyMask, &ev);
		XFlush(dpy);
	}
	return (owner);
}

#if 0
static void
startup_notification_complete(Window selwin)
{
	if (options.startup_id) {
		int l, len = 20 + strlen(options.startup_id);
		XEvent xev = { 0, };
		Window from, root = DefaultRootWindow(dpy);
		char *msg, *p;

		msg = calloc(len + 1, sizeof(*msg));
		snprintf(msg, len, "remove: ID=%s", options.startup_id);

		if (!(from = selwin))
			from = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, ParentRelative, ParentRelative);
		xev.type = ClientMessage;
		xev.xclient.message_type = _XA_NET_STARTUP_INFO_BEGIN;
		xev.xclient.display = dpy;
		xev.xclient.window = from;
		xev.xclient.format = 8;

		l = strlen((p = msg)) + 1;
		while (l > 0) {
			strncpy(xev.xclient.data.b, p, 20);
			p += 20;
			l -= 20;
			/* just PropertyChange mask in the spec doesn't work :( */
			if (!XSendEvent(dpy, root, False, StructureNotifyMask | SubstructureNotifyMask |
					SubstructureRedirectMask | PropertyChangeMask, &xev))
				EPRINTF("XSendEvent: failed!\n");
			xev.xclient.message_type = _XA_NET_STARTUP_INFO;
		}
		XSync(dpy, False);

		if (from != selwin)
			XDestroyWindow(dpy, from);
	}
}

static void
announce_selection(Window root, Window selwin, Atom selection)
{
	XEvent ev;

	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = False;
	ev.xclient.display = dpy;
	ev.xclient.window = root;
	ev.xclient.message_type = _XA_MANAGER;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = options.timestamp;	/* FIXME */
	ev.xclient.data.l[1] = selection;
	ev.xclient.data.l[2] = selwin;
	ev.xclient.data.l[3] = 0;
	ev.xclient.data.l[4] = 0;

	XSendEvent(dpy, root, False, StructureNotifyMask, &ev);
	XSync(dpy, False);
}

static Window
get_selection(Bool replace, Window selwin)
{
	char selection[64] = { 0, };
	int s, nscr;
	Atom atom;
	Window owner, gotone = None;

	PTRACE(5);
	nscr = gdk_display_get_n_screens(disp);

	for (s = 0; s < nscr; s++) {
		snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
		atom = XInternAtom(dpy, selection, False);
		if (!(owner = XGetSelectionOwner(dpy, atom)))
			DPRINTF(1, "No owner for %s\n", selection);
		if ((owner && replace) || (!owner && selwin)) {
			DPRINTF(1, "Setting owner of %s to 0x%08lx from 0x%08lx\n", selection, selwin, owner);
			XSetSelectionOwner(dpy, atom, selwin, options.timestamp);
			XSync(dpy, False);
			/* XXX: should do XIfEvent for owner window destruction */
		}
		if (!gotone && owner)
			gotone = owner;
	}
	if (replace) {
		if (gotone) {
			if (selwin)
				DPRINTF(1, "replacing running instance\n");
			else
				DPRINTF(1, "quitting running instance\n");
		} else {
			if (selwin)
				DPRINTF(1, "no running instance to replace\n");
			else
				DPRINTF(1, "no running instance to quit\n");
		}
		if (selwin) {
			for (s = 0; s < nscr; s++) {
				Screen *scrn = ScreenOfDisplay(dpy, s);
				Window root = RootWindowOfScreen(scrn);

				snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
				atom = XInternAtom(dpy, selection, False);

				announce_selection(root, selwin, atom);
			}
		}
	} else if (gotone) {
		DPRINTF(1, "not replacing running instance\n");
		return (gotone);
	}
	startup_notification_complete(selwin);
	return (gotone);
}

static void
fork_and_exit(void)
{
	pid_t pid = getpid();

	if ((pid = fork()) < 0) {
		EPRINTF("%s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (!pid)
		/* child continues */
		return;
	/* parent exits */
	exit(EXIT_SUCCESS);
}
#endif

static int
have_button(int button)
{
	if (button) {
		GdkWindow *root = gdk_get_default_root_window();
		GdkDevice *device = gdk_device_get_core_pointer();
		GdkModifierType mask = 0;

		gdk_device_get_state(device, root, NULL, &mask);

		switch(button) {
		case 1:
			if ((mask & GDK_BUTTON1_MASK))
				return (button);
			break;
		case 2:
			if ((mask & GDK_BUTTON2_MASK))
				return (button);
			break;
		case 3:
			if ((mask & GDK_BUTTON3_MASK))
				return (button);
			break;
		case 4:
			if ((mask & GDK_BUTTON4_MASK))
				return (button);
			break;
		case 5:
			if ((mask & GDK_BUTTON5_MASK))
				return (button);
			break;
		default:
			break;
		}
	}
	return (0);
}

static void
do_run(int argc, char *argv[])
{
	XdeMonitor *xmon;
	GtkWidget *menu;

	oldhandler = XSetErrorHandler(handler);
	oldiohandler = XSetIOErrorHandler(iohandler);

	xmon = init_screens(None);

	g_unix_signal_add(SIGTERM, &term_signal_handler, NULL);
	g_unix_signal_add(SIGINT, &int_signal_handler, NULL);
	g_unix_signal_add(SIGHUP, &hup_signal_handler, NULL);

	if (!(menu = popup_menu_new(xmon))) {
		EPRINTF("cannot get menu\n");
		exit(EXIT_FAILURE);
	}
	g_signal_connect(G_OBJECT(menu), "selection-done", G_CALLBACK(selection_done), NULL);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, position_menu, xmon,
		       have_button(options.button),
		       options.timestamp ? : gtk_get_current_event_time());
	mainloop();
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
Copyright (c) 2010-2019  Monavacon Limited <http://www.monavacon.com/>\n\
Copyright (c) 2002-2009  OpenSS7 Corporation <http://www.openss7.com/>\n\
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
Copyright (c) 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019  Monavacon Limited.\n\
Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009  OpenSS7 Corporation.\n\
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

#if 0
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

#if 0
static const char *
show_window(Window win)
{
	static char window[64] = { 0, };

	if (!options.window)
		return ("None");
	snprintf(window, sizeof(window), "0x%lx", options.window);
	return (window);
}
#endif

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
	case UseScreenSelect:
		return ("select");
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
	case PositionIconGeom:
		return ("icongeom");
	}
	return NULL;
}

const char *
show_include(Include include)
{
	switch (include) {
	case IncludeDefault:
	case IncludeDocs:
		return ("documents");
	case IncludeApps:
		return ("applications");
	case IncludeBoth:
		return ("both");
	}
	return NULL;
}

const char *
show_sorting(Sorting sorting)
{
	switch (sorting) {
	case SortByDefault:
	case SortByRecent:
		return ("recent");
	case SortByFavorite:
		return ("favorite");
	}
	return NULL;
}

const char *
show_organize(Organize organize)
{
	switch (organize) {
	case OrganizeDefault:
	case OrganizeNone:
		return ("none");
	case OrganizeDate:
		return ("date");
	case OrganizeFreq:
		return ("freq");
	case OrganizeGroup:
		return ("group");
	case OrganizeContent:
		return ("content");
	case OrganizeApp:
		return ("app");
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
    -M, --monitor MONITOR\n\
        specify the monitor number, MONITOR, to use [default: %6$s]\n\
    -f, --filename FILENAME\n\
        use the file, FILENAME, for configuration [default: %7$s]\n\
    -b, --button BUTTON\n\
        specify the mouse button number, BUTTON, for popup [default: %8$d]\n\
    -w, --which {active|focused|pointer|SCREEN}\n\
        specify the screen for which to pop the menu [default: %9$s]\n\
        \"active\"   - the screen with EWMH/NetWM active client\n\
        \"focused\"  - the screen with EWMH/NetWM focused client\n\
        \"pointer\"  - the screen with EWMH/NetWM pointer\n\
        \"SCREEN\"   - the specified screen number\n\
    -W, --where {pointer|center|topleft|GEOMETRY}\n\
        specify where to place the menu [default: %10$s]\n\
        \"pointer\"  - northwest corner under the pointer\n\
        \"center\"   - center of associated monitor\n\
        \"topleft\"  - northwest corner of work area\n\
        GEOMETRY   - postion on screen as X geometry\n\
    -K, --keyboard\n\
        indicate that the keyboard was used to launch [default: %11$s]\n\
    -P, --pointer\n\
        indicate that the pointer was used to launch [default: %12$s]\n\
    -T, --timestamp TIMESTAMP\n\
        use the time, TIMESTAMP, for button/keyboard event [default: %13$lu]\n\
    -i, --include {docs|apps|both}\n\
        specify which things to include [default: %15$s]\n\
        \"docs\"     - include only documents\n\
        \"apps\"     - include only applications\n\
        \"both\"     - include both documents and applications\n\
    -S, --sorting {recent|favorite}\n\
        specify how to sort the menu [default: %16$s]\n\
        \"default\"  - the default setting\n\
        \"recent\"   - arrange the most recent item first\n\
        \"favorite\" - arrange the most used item first\n\
    -O, --organize {default|date|freq|group|content|application}\n\
        organize items into submenus [default: %17$s]\n\
        \"default\"  - no submenus\n\
        \"date\"     - organize into submenus by date\n\
        \"freq\"     - organize into submenus by frequency\n\
        \"group\"    - organize into submenus by document group\n\
        \"content\"  - organize into submenus by content type\n\
        \"app\"      - organize into submenus by application\n\
    -g, --groups\n\
        specify whether to sort items by group [default: %18$s]\n\
    -m, --maximum MAX\n\
        place no more than MAX items in menu [default: %19$d]\n\
    -t, --tooltips\n\
        provide detailed tooltips on menu items [default: %14$s]\n\
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
	, show_screen(options.monitor)
	, options.filename
	, options.button
	, show_which(options.which)
	, show_where(options.where)
	, show_bool(options.keyboard)
	, show_bool(options.pointer)
	, options.timestamp
	, show_bool(options.tooltips)
	, show_include(options.include)
	, show_sorting(options.sorting)
	, show_organize(options.organize)
	, show_bool(options.groups)
	, options.maximum
);
	/* *INDENT-ON* */
}

static Bool
get_text_property(Display *dpy, Window root, Atom prop, char ***listp, int *stringsp)
{
	XTextProperty xtp = { NULL, };

	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		*listp = NULL;
		*stringsp = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, listp, stringsp) == Success)
			return True;
		else {
			char *name = NULL;

			DPRINTF(1, "could not get text list for %s property\n", (name = XGetAtomName(dpy, prop)));
			if (name)
				XFree(name);
		}
	} else {
		char *name = NULL;

		DPRINTF(1, "could not get %s for root 0x%lx\n", (name = XGetAtomName(dpy, prop)), root);
		if (name)
			XFree(name);
	}
	return False;
}

static void
set_default_wmname(void)
{
	if (options.display) {
		Display *dpy;
		Window root;
		Atom prop;
		char **list = NULL;
		int strings = 0;

		if (!(dpy = XOpenDisplay(NULL))) {
			EPRINTF("could not open display %s\n", getenv("DISPLAY"));
			return;
		}
		root = RootWindow(dpy, 0);
		prop = XInternAtom(dpy, "_XDE_WM_NAME", False);
		if (get_text_property(dpy, root, prop, &list, &strings) && list) {
			options.wmname = strdup(list[0]);
			XFreeStringList(list);
		} else {
			char *name = NULL;

			DPRINTF(1, "could not get %s for root 0x%lx\n", (name = XGetAtomName(dpy, prop)), root);
			if (name)
				XFree(name);
		}
		XCloseDisplay(dpy);
	} else
		DPRINTF(1, "cannot determine wmname without DISPLAY\n");
	if (options.wmname)
		DPRINTF(1, "assigned wmname as '%s'\n", options.wmname);
}

static void
set_default_desktop(void)
{
	const char *env;
	char *p;

	if (!options.desktop || !strcmp(options.desktop, "XDE") || !options.wmname
	    || strcasecmp(options.wmname, options.desktop)) {
		if ((env = getenv("XDG_CURRENT_DESKTOP"))) {
			free(options.desktop);
			options.desktop = strdup(env);
		} else if (options.wmname) {
			free(options.desktop);
			options.desktop = strdup(options.wmname);
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
set_default_theme(void)
{
}

static void
set_default_icon_theme(void)
{
}

static void
set_default_config(void)
{
	char *file;

	if (options.wmname)
		file = g_build_filename(g_get_user_config_dir(), RESNAME, options.wmname, "rc", NULL);
	else
		file = g_build_filename(g_get_user_config_dir(), RESNAME, "rc", NULL);
	options.filename = strdup(file);
	g_free(file);
}

static void
set_default_files(void)
{
	gchar *file;

	file = g_build_filename(g_get_user_config_dir(), "xde", "run-history", NULL);
	free(options.runhist);
	options.runhist = strdup(file);
	g_free(file);
	file = g_build_filename(g_get_user_config_dir(), "xde", "recent-applications", NULL);
	free(options.recapps);
	options.recapps = strdup(file);
	g_free(file);
	file = g_build_filename(g_get_user_data_dir(), "recently-used", NULL);
	if (access(file, R_OK | W_OK)) {
		g_free(file);
		file = g_build_filename(g_get_home_dir(), ".recently-used", NULL);
	}
	free(options.recently);
	options.recently = strdup(file);
	g_free(file);
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
	char *endptr = NULL;
	int n, monitor;
	Time timestamp;

	if ((env = getenv("DISPLAY"))) {
		free(options.display);
		options.display = strdup(env);
		if (options.screen < 0 && (p = strrchr(options.display, '.'))
		    && (n = strspn(++p, "0123456789")) && *(p + n) == '\0')
			options.screen = atoi(p);
	}
	if ((env = getenv("DESKTOP_STARTUP_ID"))) {
		free(options.startup_id);
		options.startup_id = strdup(env);
		/* we can get the timestamp from the startup id */
		if ((p = strstr(env, "_TIME"))) {
			timestamp = strtoul(p + 5, &endptr, 10);
			if (!*endptr)
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
	if ((env = getenv("XDE_DEBUG"))) {
		options.debug = atoi(env);
		options.output = options.debug + 1;
	}
	set_default_wmname();
	set_default_desktop();
	set_default_theme();
	set_default_icon_theme();
	set_default_config();
	set_default_files();
}

static void
get_default_wmname(void)
{
	if (options.wmname) {
		DPRINTF(1, "option wmname is set to '%s'\n", options.wmname);
		return;
	}

	if (options.display) {
		Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
		Window root = DefaultRootWindow(dpy);
		Atom prop = _XA_XDE_WM_NAME;
		char **list = NULL;
		int strings = 0;

		if (get_text_property(dpy, root, prop, &list, &strings)) {
			if (!options.wmname) {
				free(options.wmname);
				options.wmname = strdup(list[0]);
			} else if (strcmp(options.wmname, list[0]))
				DPRINTF(1, "default wmname %s different from actual %s\n",
					options.wmname, list[0]);
			if (list)
				XFreeStringList(list);
		} else {
			char *name = NULL;

			DPRINTF(1, "could not get %s for root 0x%lx\n", (name = XGetAtomName(dpy, prop)), root);
			if (name)
				XFree(name);
		}
	} else
		EPRINTF("cannot determine wmname without DISPLAY\n");

	if (options.wmname)
		DPRINTF(1, "assigned wmname as '%s'\n", options.wmname);
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
		} else if (options.wmname) {
			free(options.desktop);
			options.desktop = strdup(options.wmname);
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
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	Window root = DefaultRootWindow(dpy);
	XTextProperty xtp = { NULL, };
	Bool changed = False;
	Atom prop = _XA_XDE_THEME_NAME;
	GtkSettings *set;

	gtk_rc_reparse_all();

	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				char *rc_string;

				rc_string = g_strdup_printf("gtk-theme-name=\"%s\"", list[0]);
				gtk_rc_parse_string(rc_string);
				g_free(rc_string);
				if (!options.theme || strcmp(options.theme, list[0])) {
					free(options.theme);
					options.theme = strdup(list[0]);
					changed = True;
				}
			}
			if (list)
				XFreeStringList(list);
		} else {
			char *name = NULL;

			EPRINTF("could not get text list for %s property\n", (name = XGetAtomName(dpy, prop)));
			if (name)
				XFree(name);
		}
		if (xtp.value)
			XFree(xtp.value);
	} else {
		char *name = NULL;

		DPRINTF(1, "could not get %s for root 0x%lx\n", (name = XGetAtomName(dpy, prop)), root);
		if (name)
			XFree(name);
	}
	if ((set = gtk_settings_get_for_screen(scrn))) {
		GValue theme_v = G_VALUE_INIT;
		const char *theme;

		g_value_init(&theme_v, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(set), "gtk-theme-name", &theme_v);
		theme = g_value_get_string(&theme_v);
		if (theme && (!options.theme || strcmp(options.theme, theme))) {
			free(options.theme);
			options.theme = strdup(theme);
			changed = True;
		}
		g_value_unset(&theme_v);
	}
	if (changed) {
		DPRINTF(1, "New theme is %s\n", options.theme);
	}
}

static void
get_default_icon_theme(void)
{
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	Window root = DefaultRootWindow(dpy);
	XTextProperty xtp = { NULL, };
	Bool changed = False;
	Atom prop = _XA_XDE_ICON_THEME_NAME;
	GtkSettings *set;

	gtk_rc_reparse_all();

	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				char *rc_string;

				rc_string = g_strdup_printf("gtk-icon-theme-name=\"%s\"", list[0]);
				gtk_rc_parse_string(rc_string);
				g_free(rc_string);
				if (!options.itheme || strcmp(options.itheme, list[0])) {
					free(options.itheme);
					options.itheme = strdup(list[0]);
					changed = True;
				}
			}
			if (list)
				XFreeStringList(list);
		} else {
			char *name = NULL;

			EPRINTF("could not get text list for %s property\n", (name = XGetAtomName(dpy, prop)));
			if (name)
				XFree(name);
		}
		if (xtp.value)
			XFree(xtp.value);
	} else {
		char *name = NULL;

		DPRINTF(1, "could not get %s for root 0x%lx\n", (name = XGetAtomName(dpy, prop)), root);
		if (name)
			XFree(name);
	}
	if ((set = gtk_settings_get_for_screen(scrn))) {
		GValue theme_v = G_VALUE_INIT;
		const char *itheme;

		g_value_init(&theme_v, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(set), "gtk-icon-theme-name", &theme_v);
		itheme = g_value_get_string(&theme_v);
		if (itheme && (!options.itheme || strcmp(options.itheme, itheme))) {
			free(options.itheme);
			options.itheme = strdup(itheme);
			changed = True;
		}
		g_value_unset(&theme_v);
	}
	if (changed) {
		DPRINTF(1, "New icon theme is %s\n", options.itheme);
	}
}

static void
get_default_config(void)
{
	char *file;

	if (options.filename)
		return;
	if (options.wmname)
		file = g_build_filename(g_get_user_config_dir(), RESNAME, options.wmname, "rc", NULL);
	else
		file = g_build_filename(g_get_user_config_dir(), RESNAME, "rc", NULL);
	free(options.filename);
	options.filename = strdup(file);
	g_free(file);
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
	get_default_wmname();
	get_default_desktop();
	get_default_theme();
	get_default_icon_theme();
	get_default_config();
}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;

	saveArgc = argc;
	saveArgv = g_strdupv(argv);

	setlocale(LC_ALL, "");

	set_defaults();

	get_resources();

	if (options.debug > 0) {
		char **arg;

		DPRINTF(1, "Command was:");
		for (arg = saveArgv; arg && *arg; arg++)
			fprintf(stderr, " %s", *arg);
		fprintf(stderr, "\n");
	}

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

			{"include",		required_argument,	NULL,	'i'},
			{"sorting",		required_argument,	NULL,	'S'},
			{"organize",		required_argument,	NULL,	'O'},
			{"groups",		no_argument,		NULL,	'g'},
			{"maximum",		required_argument,	NULL,	'm'},

			{"tooltips",		no_argument,		NULL,	't'},
			{"debug",		optional_argument,	NULL,	'D'},
			{"verbose",		optional_argument,	NULL,	'v'},
			{"help",		no_argument,		NULL,	'h'},
			{"version",		no_argument,		NULL,	'V'},
			{"copying",		no_argument,		NULL,	'C'},
			{"?",			no_argument,		NULL,	'h'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "d:s:M:pf:T:PKb:w:W:i:S:O:gm:tD::v::hVC?",
				     long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:M:pf:T:PKb:w:W:i:S:O:gm:tD:vhVC?");
#endif				/* _GNU_SOURCE */
		if (c == -1) {
			DPRINTF(1, "%s: done options processing\n", argv[0]);
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
			if (*endptr)
				goto bad_option;
			options.screen = val;
			break;
		case 'M':	/* -M, --monitor MONITOR */
			val = strtol(optarg, &endptr, 0);
			if (*endptr)
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

		case 'b':	/* -b, --button [BUTTON] */
			if (optarg) {
				val = strtoul(optarg, &endptr, 0);
				if (*endptr)
					goto bad_option;
				if (val < 0 || val > 8)
					goto bad_option;
			} else
				val = 1;
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
			if (!strncasecmp("default", optarg, len))
				options.which = UseScreenDefault;
			else if (!strncasecmp("active", optarg, len))
				options.which = UseScreenActive;
			else if (!strncasecmp("focused", optarg, len))
				options.which = UseScreenFocused;
			else if (!strncasecmp("pointer", optarg, len))
				options.which = UseScreenPointer;
			else {
				options.screen = strtoul(optarg, &endptr, 0);
				if (*endptr)
					goto bad_option;
				options.which = UseScreenSpecified;
			}
			break;
		case 'W':	/* -W, --where WHERE */
			if (options.where != PositionDefault)
				goto bad_option;
			if (!(len = strlen(optarg)))
				goto bad_option;
			if (!strncasecmp("default", optarg, len))
				options.where = PositionDefault;
			else if (!strncasecmp("pointer", optarg, len))
				options.where = PositionPointer;
			else if (!strncasecmp("center", optarg, len))
				options.where = PositionCenter;
			else if (!strncasecmp("topleft", optarg, len))
				options.where = PositionTopLeft;
			else if (!strncasecmp("bottomright", optarg, len))
				options.where = PositionBottomRight;
			else if (!strncasecmp("icongeom", optarg, len))
				options.where = PositionIconGeom;
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

		case 'T':	/* -T, --timestamp TIMESTAMP */
			options.timestamp = strtoul(optarg, &endptr, 0);
			if (*endptr)
				goto bad_option;
			break;

		case 'i':	/* -i, --include INCLUDE */
			if (options.include != IncludeDefault)
				goto bad_option;
			len = strlen(optarg);
			if (!strncasecmp("docs", optarg, len))
				options.include = IncludeDocs;
			else if (!strncasecmp("apps", optarg, len))
				options.include = IncludeApps;
			else if (!strncasecmp("both", optarg, len))
				options.include = IncludeBoth;
			else
				goto bad_option;
			break;
		case 'S':	/* -S, --sorting SORTING */
			if (options.sorting != SortByDefault)
				goto bad_option;
			len = strlen(optarg);
			if (!strncasecmp("recent", optarg, len))
				options.sorting = SortByRecent;
			else if (!strncasecmp("favorite", optarg, len))
				options.sorting = SortByFavorite;
			else
				goto bad_option;
			break;
		case 'O':	/* -O, --organize ORGANIZE */
			if (options.organize != OrganizeDefault)
				goto bad_option;
			len = strlen(optarg);
			if (!strncasecmp("none", optarg, len))
				options.organize = OrganizeNone;
			else if (!strncasecmp("date", optarg, len))
				options.organize = OrganizeDate;
			else if (!strncasecmp("freq", optarg, len))
				options.organize = OrganizeFreq;
			else if (!strncasecmp("group", optarg, len))
				options.organize = OrganizeGroup;
			else if (!strncasecmp("content", optarg, len))
				options.organize = OrganizeContent;
			else if (!strncasecmp("app", optarg, len))
				options.organize = OrganizeApp;
			else
				goto bad_option;
			break;
		case 'g':	/* -g, --groups */
			options.groups = TRUE;
			break;
		case 'm':	/* -m, --maximum MAX */
			if ((val = strtol(optarg, &endptr, 0)) < 0 && val != -1)
				goto bad_option;
			if (*endptr)
				goto bad_option;
			options.maximum = val;
			break;

		case 't':	/* -t, --tooltips */
			options.tooltips = True;
			break;

		case 'D':	/* -D, --debug [LEVEL] */
			DPRINTF(1, "increasing debug verbosity\n");
			if (optarg == NULL) {
				options.debug++;
				break;
			}
			if ((val = strtol(optarg, &endptr, 0)) < 0)
				goto bad_option;
			if (*endptr)
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
			if (*endptr)
				goto bad_option;
			options.output = val;
			break;
		case 'h':	/* -h, --help, -?, --? */
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
	DPRINTF(1, "option index = %d\n", optind);
	DPRINTF(1, "option count = %d\n", argc);
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
		options.command = CommandRun;
		__attribute__((fallthrough));
	case CommandRun:
		DPRINTF(1, "popping the menu\n");
		do_run(argc, argv);
		break;
	case CommandPopMenu:
		DPRINTF(1, "popping the menu\n");
		do_run(argc, argv);
		break;
	case CommandHelp:
		DPRINTF(1, "printing help message\n");
		help(argc, argv);
		break;
	case CommandVersion:
		DPRINTF(1, "printing version message\n");
		version(argc, argv);
		break;
	case CommandCopying:
		DPRINTF(1, "printing copying message\n");
		copying(argc, argv);
		break;
	default:
		usage(argc, argv);
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}

/** @} */

// vim: set sw=8 tw=80 com=srO\:/**,mb\:*,ex\:*/,srO\:/*,mb\:*,ex\:*/,b\:TRANS fo+=tcqlorn foldmarker=@{,@} foldmethod=marker:

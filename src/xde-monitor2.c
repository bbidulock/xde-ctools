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
#include <X11/extensions/scrnsaver.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/xf86misc.h>
#include <X11/XKBlib.h>
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
#include <gtk/gtk.h>
#include <cairo.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <pwd.h>

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

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

#define NEED_DESKTOP_LAYOUT		0
#define NEED_CURRENT_DESKTOP		0
#define NEED_ROOT_PIXMAPS		0
#define NEED_DESKTOP_INFO		0
#define NEED_CLIENT_INFO		0
#define NEED_POPUP_WINDOW		1
#define NEED_STARTUP_IDS		1

#define NEED_PAGER			0
#define NEED_TASKS			0
#define NEED_MODEL			1

#define XA_PREFIX               "_XDE_MONITOR"
#define XA_SELECTION_NAME	XA_PREFIX "_S%d"
#define XA_NET_DESKTOP_LAYOUT	"_NET_DESKTOP_LAYOUT_S%d"
#define LOGO_NAME		"mate-panel-launcher"

static int saveArgc;
static char **saveArgv;

#define RESNAME "xde-monitor"
#define RESCLAS "XDE-Monitor"
#define RESTITL "XDG Launch Monitor"

#define USRDFLT "%s/.config/" RESNAME "/rc"
#define APPDFLT "/usr/share/X11/app-defaults/" RESCLAS

static Atom _XA_XDE_THEME_NAME;
static Atom _XA_GTK_READ_RCFILES;

#if NEED_DESKTOP_LAYOUT
static Atom _XA_NET_DESKTOP_LAYOUT;
#endif
#if NEED_CURRENT_DESKTOP
static Atom _XA_NET_CURRENT_DESKTOP;
static Atom _XA_WIN_WORKSPACE;
static Atom _XA_WM_DESKTOP;
#endif
#if NEED_ROOT_PIXMAPS
static Atom _XA_XROOTPMAP_ID;
static Atom _XA_ESETROOT_PMAP_ID;
#endif
#if NEED_DESKTOP_INFO
static Atom _XA_NET_DESKTOP_NAMES;
static Atom _XA_NET_NUMBER_OF_DESKTOPS;
static Atom _XA_WIN_WORKSPACE_COUNT;
static Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
static Atom _XA_NET_WM_ICON_GEOMETRY;
#endif
#if 0
static Atom _XA_WIN_AREA;
static Atom _XA_WIN_AREA_COUNT;
#endif
#if NEED_CLIENT_INFO
static Atom _XA_NET_ACTIVE_WINDOW;
static Atom _XA_NET_CLIENT_LIST;
static Atom _XA_NET_CLIENT_LIST_STACKING;
static Atom _XA_WIN_FOCUS;
static Atom _XA_WIN_CLIENT_LIST;
#endif				/* NEED_CLIENT_INFO */
#if NEED_STARTUP_IDS
#ifdef STARTUP_NOTIFICATION
static Atom _XA_NET_STARTUP_INFO;
static Atom _XA_NET_STARTUP_INFO_BEGIN;
#endif				/* STARTUP_NOTIFICATION */
#endif				/* NEED_STARTUP_IDS */

static Atom _XA_PREFIX_EDIT;
static Atom _XA_PREFIX_TRAY;

typedef enum {
	CommandDefault,
	CommandRun,
	CommandQuit,
	CommandHelp,
	CommandVersion,
	CommandCopying,
} Command;

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

typedef struct {
	int debug;
	int output;
	char *display;
	int screen;
	int monitor;
	Time timeout;
	int border;
	Bool keyboard;
	Bool pointer;
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
	char *filename;
	Bool replace;
	Bool editor;
	Bool trayicon;
	char *keys;
	Bool proxy;
	Bool cycle;
	Bool hidden;
	Bool minimized;
	Bool monitors;
	Bool workspaces;
	Bool activate;
	Bool raise;
	Bool restore;
	Command command;
	char *clientId;
	char *saveFile;
	Bool dryrun;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.monitor = 0,
	.timeout = 1000,
	.border = 5,
	.keyboard = False,
	.pointer = False,
	.button = 0,
	.timestamp = CurrentTime,
	.which = UseScreenDefault,
	.where = PositionDefault,
	.x = {
	      .value = 0,
	      .sign = 1,
	      },
	.y = {
	      .value = 0,
	      .sign = 1,
	      },
	.w = 0,
	.h = 0,
	.order = WindowOrderDefault,
	.filename = NULL,
	.replace = False,
	.editor = False,
	.trayicon = False,
	.keys = NULL,
	.proxy = False,
	.cycle = False,
	.hidden = False,
	.minimized = False,
	.monitors = False,
	.workspaces = False,
	.activate = True,
	.raise = False,
	.restore = True,
	.command = CommandDefault,
	.clientId = NULL,
	.saveFile = NULL,
	.dryrun = False,
};

Display *dpy = NULL;
GdkDisplay *disp = NULL;

struct XdeScreen;
typedef struct XdeScreen XdeScreen;
struct XdeMonitor;
typedef struct XdeMonitor XdeMonitor;
struct XdeImage;
typedef struct XdeImage XdeImage;
struct XdePixmap;
typedef struct XdePixmap XdePixmap;

#if NEED_STARTUP_IDS
#ifdef STARTUP_NOTIFICATION
SnDisplay *sn_dpy = NULL;

typedef struct Sequence {
	int screen;			/* screen number */
	int monitor;			/* monitor number */
	XdeMonitor *xmon;		/* monitor */
	char *launcher;
	char *launchee;
	char *hostname;
	pid_t pid;
	long sequence;
	long timestamp;
	GList **list;			/* the list we are on */
	GtkTreeIter iter;		/* the position in the model */
	SnStartupSequence *seq;		/* the sequence itself */
	GDesktopAppInfo *info;		/* the desktop entry */
} Sequence;
#endif				/* STARTUP_NOTIFICATION */
#endif				/* NEED_STARTUP_IDS */

struct XdePixmap {
	int refs;			/* number of references */
	int index;			/* background image index */
	XdePixmap *next;		/* next pixmap in list */
	XdePixmap **pprev;		/* previous next pointer in list */
	GdkPixmap *pixmap;		/* pixmap for background image */
	GdkRectangle geom;		/* pixmap geometry */
};

struct XdeImage {
	int refs;			/* number of references */
	int index;			/* background image index */
	char *file;			/* filename of image source */
	GdkPixbuf *pixbuf;		/* pixbuf for this image */
	XdePixmap *pixmaps;		/* list of pixmaps at various geometries */
};

struct XdeMonitor {
	int index;			/* monitor number */
	XdeScreen *xscr;		/* screen */
	int current;			/* current desktop for this monitor */
	GdkRectangle geom;		/* geometry of the monitor */
#if NEED_POPUP_WINDOW
	GtkWidget *popup;
	union {
		GtkWidget *view;
		GtkWidget *pager;
		GtkWidget *tasks;
	};
	GtkListStore *model;
	int seqcount;			/* number of sequences in store */
	unsigned timer;
	Bool inside;			/* pointer inside popup */
	Bool keyboard;			/* have a keyboard grab */
	Bool pointer;			/* have a pointer grab */
	GdkModifierType mask;
#endif					/* NEED_POPUP_WINDOW */
};

struct XdeScreen {
	int index;			/* index */
	GdkScreen *scrn;		/* screen */
	GdkWindow *root;
	WnckScreen *wnck;
	gint nmon;			/* number of monitors */
	XdeMonitor *mons;		/* monitors for this screen */
	Bool mhaware;			/* multi-head aware NetWM */
	Pixmap pixmap;			/* current pixmap for the entire screen */
	char *theme;			/* XDE theme name */
	GKeyFile *entry;		/* XDE theme file entry */
	int nimg;			/* number of images */
	XdeImage **sources;		/* the images for the theme */
	Window selwin;			/* selection owner window */
	Atom atom;			/* selection atom for this screen */
#if NEED_DESKTOP_LAYOUT
	Window laywin;			/* desktop layout selection owner */
	Atom prop;			/* dekstop layout selection atom */
#endif
	int width, height;		/* dimensions of screen */
	guint timer;			/* timer source of running timer */
	int rows;			/* number of rows in layout */
	int cols;			/* number of cols in layout */
	int desks;			/* number of desks in layout */
	int ndsk;			/* number of desktops */
	XdeImage **backdrops;		/* the desktops */
	int current;			/* current desktop for this screen */
	char *wmname;			/* window manager name (adjusted) */
	Bool goodwm;			/* is the window manager usable? */
#if NEED_STARTUP_IDS
#ifdef STARTUP_NOTIFICATION
	SnMonitorContext *ctx;		/* monitor context for this screen */
	GList *sequences;		/* startup notification sequences */
#endif
#endif
	GtkWindow *desktop;
	GdkWindow *proxy;
	guint deferred_refresh_layout;
	guint deferred_refresh_desktop;
};

XdeScreen *screens = NULL;		/* array of screens */

#if NEED_DESKTOP_LAYOUT
static void refresh_layout(XdeScreen *xscr);
#endif				/* NEED_DESKTOP_LAYOUT */

#if NEED_ROOT_PIXMAPS
static void refresh_desktop(XdeScreen *xscr);
#endif				/* NEED_ROOT_PIXMAPS */

#if NEED_ROOT_PIXMAPS
static gboolean
on_deferred_refresh_layout(gpointer data)
{
	XdeScreen *xscr = data;

	xscr->deferred_refresh_layout = 0;
	refresh_layout(xscr);
	return G_SOURCE_REMOVE;
}

static gboolean
on_deferred_refresh_desktop(gpointer data)
{
	XdeScreen *xscr = data;

	xscr->deferred_refresh_desktop = 0;
	refresh_desktop(xscr);
	return G_SOURCE_REMOVE;
}
#endif				/* NEED_ROOT_PIXMAPS */

#if NEED_ROOT_PIXMAPS
static void
add_deferred_refresh_layout(XdeScreen *xscr)
{
	if (!xscr->deferred_refresh_layout)
		xscr->deferred_refresh_layout = g_idle_add(on_deferred_refresh_layout, xscr);
	if (xscr->deferred_refresh_desktop) {
		g_source_remove(xscr->deferred_refresh_desktop);
		xscr->deferred_refresh_desktop = 0;
	}
}

static void
add_deferred_refresh_desktop(XdeScreen *xscr)
{
	if (xscr->deferred_refresh_layout)
		return;
	if (xscr->deferred_refresh_desktop)
		return;
	xscr->deferred_refresh_desktop = g_idle_add(on_deferred_refresh_desktop, xscr);
}
#endif				/* NEED_ROOT_PIXMAPS */

#if NEED_ROOT_PIXMAPS
void
xde_pixmap_ref(XdePixmap * pixmap)
{
	if (pixmap)
		pixmap->refs++;
}

void
xde_pixmap_delete(XdePixmap * pixmap)
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
xde_pixmap_unref(XdePixmap ** pixmapp)
{
	if (pixmapp && *pixmapp) {
		if (--(*pixmapp)->refs <= 0) {
			xde_pixmap_delete(*pixmapp);
			*pixmapp = NULL;
		}
	}
}

void
xde_image_ref(XdeImage * image)
{
	if (image)
		image->refs++;
}

void
xde_image_delete(XdeImage * image)
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
xde_image_unref(XdeImage ** imagep)
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
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
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

	XSendEvent(dpy, root, False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
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
	GdkDisplay *disp = gdk_screen_get_display(xscr->scrn);
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
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
	GdkDisplay *disp = gdk_screen_get_display(xscr->scrn);
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
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
	Display *dpy;
	Pixmap pmap;
	int s;

	if (!(dpy = XOpenDisplay(NULL))) {
		DPRINTF(1, "cannot open display %s\n", getenv("DISPLAY"));
		return (None);
	}
	XSetCloseDownMode(dpy, RetainTemporary);
	s = xscr->index;
	pmap = XCreatePixmap(dpy, RootWindow(dpy, s), xscr->width, xscr->height, DefaultDepth(dpy, s));
	XFlush(dpy);
	XSync(dpy, True);
	XCloseDisplay(dpy);
	return (pmap);
}

static void
refresh_desktop(XdeScreen *xscr)
{
	GdkDisplay *disp = gdk_screen_get_display(xscr->scrn);
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
		DPRINTF(1, "killing old unused temporary pixmap 0x%08lx\n", xscr->pixmap);
		XKillClient(GDK_DISPLAY_XDISPLAY(disp), xscr->pixmap);
	}
	xscr->pixmap = pmap;
}

#endif				/* NEED_ROOT_PIXMAPS */

#if NEED_POPUP_WINDOW

#if NEED_STARTUP_IDS

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
get_icons(GIcon * gicon, const char *const *inames)
{
	GtkIconTheme *theme;
	GtkIconInfo *info;
	const char *const *iname;

	PTRACE(0);
	if ((theme = gtk_icon_theme_get_default())) {
		PTRACE(0);
		if (gicon && (info = gtk_icon_theme_lookup_by_gicon(theme, gicon, 48,
								    GTK_ICON_LOOKUP_USE_BUILTIN |
								    GTK_ICON_LOOKUP_GENERIC_FALLBACK |
								    GTK_ICON_LOOKUP_FORCE_SIZE))) {
			PTRACE(0);
			if (gtk_icon_info_get_filename(info))
				return gtk_icon_info_load_icon(info, NULL);
			return gtk_icon_info_get_builtin_pixbuf(info);
		}
		PTRACE(0);
		for (iname = inames; iname && *iname; iname++) {
			DPRINTF(2, "Testing for icon name: %s\n", *iname);
			if ((info = gtk_icon_theme_lookup_icon(theme, *iname, 48,
							       GTK_ICON_LOOKUP_USE_BUILTIN |
							       GTK_ICON_LOOKUP_GENERIC_FALLBACK |
							       GTK_ICON_LOOKUP_FORCE_SIZE))) {
				if (gtk_icon_info_get_filename(info))
					return gtk_icon_info_load_icon(info, NULL);
				return gtk_icon_info_get_builtin_pixbuf(info);
			}
		}
		PTRACE(0);
	}
	return (NULL);
}

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

	PTRACE(0);
	inames = calloc(16, sizeof(*inames));

	if ((name = sn_startup_sequence_get_icon_name(seq->seq))) {
		PTRACE(0);
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
		PTRACE(0);
		/* FIXME: look up entry file too */
		/* try both mixed- and lower-case WMCLASS= */
		if ((name = sn_startup_sequence_get_wmclass(seq->seq))) {
			PTRACE(0);
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
			PTRACE(0);
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
	PTRACE(0);
	base = strdup("unknown");
	inames[i++] = base;
	DPRINTF(2, "Choice %d for icon name: %s\n", i, base);
	pixbuf = get_icons(gicon, (const char *const *) inames);
	g_strfreev((gchar **) inames);
	if (gicon)
		g_object_unref(G_OBJECT(gicon));
	return (pixbuf);
}

#endif				/* NEED_STARTUP_IDS */

/** @section Popup Window Event Handlers
  * @{ */

static gboolean
stop_popup_timer(XdeMonitor *xmon)
{
	PTRACE(5);
	if (xmon->timer) {
		DPRINTF(1, "stopping popup timer\n");
		g_source_remove(xmon->timer);
		xmon->timer = 0;
		return TRUE;
	}
	return FALSE;
}

static void
release_grabs(XdeMonitor *xmon)
{
	PTRACE(5);
	if (xmon->pointer) {
#if 0
		/* will be broken when window unmaps */
		DPRINTF(1, "ungrabbing pointer\n");
		gdk_display_pointer_ungrab(xmon->disp, GDK_CURRENT_TIME);
#endif
		xmon->pointer = False;
	}
	if (xmon->keyboard) {
#if 0
		/* will be broken when window unmaps */
		DPRINTF(1, "ungrabbing keyboard\n");
		gdk_display_keyboard_ungrab(xmon->disp, GDK_CURRENT_TIME);
#endif
		xmon->keyboard = False;
	}
}

static void
drop_popup(XdeMonitor *xmon)
{
	PTRACE(5);
	if (gtk_widget_get_mapped(xmon->popup)) {
		stop_popup_timer(xmon);
		release_grabs(xmon);
		gtk_widget_hide(xmon->popup);
	}
}

static gboolean
workspace_timeout(gpointer user)
{
	XdeMonitor *xmon = user;

	DPRINTF(1, "popup timeout!\n");
	drop_popup(xmon);
	xmon->timer = 0;
	return G_SOURCE_REMOVE;
}

static gboolean
start_popup_timer(XdeMonitor *xmon)
{
	PTRACE(5);
	if (xmon->timer)
		return FALSE;
	DPRINTF(1, "starting popup timer\n");
	xmon->timer = g_timeout_add(options.timeout, workspace_timeout, xmon);
	return TRUE;
}

void
restart_popup_timer(XdeMonitor *xmon)
{
	DPRINTF(1, "restarting popup timer\n");
	stop_popup_timer(xmon);
	start_popup_timer(xmon);
}

static void
show_popup(XdeMonitor *xmon, gboolean grab_p, gboolean grab_k)
{
	GdkGrabStatus status;
	Window win;

	DPRINTF(1, "popping the window\n");
	gdk_display_get_pointer(disp, NULL, NULL, NULL, &xmon->mask);
	stop_popup_timer(xmon);
	gtk_window_set_screen(GTK_WINDOW(xmon->popup), gdk_display_get_screen(disp,
				xmon->xscr->index));
	gtk_window_set_position(GTK_WINDOW(xmon->popup), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_present(GTK_WINDOW(xmon->popup));
	gtk_widget_show_now(GTK_WIDGET(xmon->popup));
	win = GDK_WINDOW_XID(xmon->popup->window);

	if (grab_p && !xmon->pointer) {
		GdkEventMask mask =
		    GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
		XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
		status = gdk_pointer_grab(xmon->popup->window, TRUE, mask, NULL, NULL, GDK_CURRENT_TIME);
		switch (status) {
		case GDK_GRAB_SUCCESS:
			DPRINTF(1, "pointer grabbed\n");
			xmon->pointer = True;
			break;
		case GDK_GRAB_ALREADY_GRABBED:
			DPRINTF(1, "%s: pointer already grabbed\n", NAME);
			break;
		case GDK_GRAB_INVALID_TIME:
			EPRINTF("%s: pointer grab invalid time\n", NAME);
			break;
		case GDK_GRAB_NOT_VIEWABLE:
			EPRINTF("%s: pointer grab on unviewable window\n", NAME);
			break;
		case GDK_GRAB_FROZEN:
			EPRINTF("%s: pointer grab on frozen pointer\n", NAME);
			break;
		}
	}
	if (grab_k && !xmon->keyboard) {
		XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
		status = gdk_keyboard_grab(xmon->popup->window, TRUE, GDK_CURRENT_TIME);
		switch (status) {
		case GDK_GRAB_SUCCESS:
			DPRINTF(1, "keyboard grabbed\n");
			xmon->keyboard = True;
			break;
		case GDK_GRAB_ALREADY_GRABBED:
			DPRINTF(1, "%s: keyboard already grabbed\n", NAME);
			break;
		case GDK_GRAB_INVALID_TIME:
			EPRINTF("%s: keyboard grab invalid time\n", NAME);
			break;
		case GDK_GRAB_NOT_VIEWABLE:
			EPRINTF("%s: keyboard grab on unviewable window\n", NAME);
			break;
		case GDK_GRAB_FROZEN:
			EPRINTF("%s: keyboard grab on frozen keyboard\n", NAME);
			break;
		}
	}
	// if (!xmon->keyboard || !xmon->pointer)
	if (!(xmon->mask & ~(GDK_LOCK_MASK | GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
		if (!xmon->inside)
			start_popup_timer(xmon);
}

/** @section Popup Window GDK Events
  * @{ */

#define PASSED_EVENT_MASK (KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|SubstructureNotifyMask|SubstructureRedirectMask)

static GdkFilterReturn
event_handler_KeyPress(Display *dpy, XEvent *xev, XdeMonitor *xmon)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> KeyPress: %p\n", xmon);
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
		fprintf(stderr, "<== KeyPress: %p\n", xmon);
	}
	if (!xev->xkey.send_event) {
		XEvent ev = *xev;

		start_popup_timer(xmon);
		ev.xkey.window = ev.xkey.root;
		XSendEvent(dpy, ev.xkey.root, True, PASSED_EVENT_MASK, &ev);
		XFlush(dpy);
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
}

static GdkFilterReturn
event_handler_KeyRelease(Display *dpy, XEvent *xev, XdeMonitor *xmon)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> KeyRelease: %p\n", xmon);
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
		fprintf(stderr, "<== KeyRelease: %p\n", xmon);
	}
	if (!xev->xkey.send_event) {
		XEvent ev = *xev;

		// start_popup_timer(xmon);
		ev.xkey.window = ev.xkey.root;
		XSendEvent(dpy, ev.xkey.root, True, PASSED_EVENT_MASK, &ev);
		XFlush(dpy);
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
}

static GdkFilterReturn
event_handler_ButtonPress(Display *dpy, XEvent *xev, XdeMonitor *xmon)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> ButtonPress: %p\n", xmon);
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
		fprintf(stderr, "<== ButtonPress: %p\n", xmon);
	}
	if (!xev->xbutton.send_event) {
		XEvent ev = *xev;

		if (ev.xbutton.button == 4 || ev.xbutton.button == 5) {
			if (!xmon->inside)
				start_popup_timer(xmon);
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
event_handler_ButtonRelease(Display *dpy, XEvent *xev, XdeMonitor *xmon)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> ButtonRelease: %p\n", xmon);
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
		fprintf(stderr, "<== ButtonRelease: %p\n", xmon);
	}
	if (!xev->xbutton.send_event) {
		XEvent ev = *xev;

		if (ev.xbutton.button == 4 || ev.xbutton.button == 5) {
			// start_popup_timer(xmon);
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
event_handler_EnterNotify(Display *dpy, XEvent *xev, XdeMonitor *xmon)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> EnterNotify: %p\n", xmon);
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
		fprintf(stderr, "<== EnterNotify: %p\n", xmon);
	}
	if (xev->xcrossing.mode == NotifyNormal) {
		if (!xmon->inside) {
			DPRINTF(1, "entered popup\n");
			stop_popup_timer(xmon);
			xmon->inside = True;
		}
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_LeaveNotify(Display *dpy, XEvent *xev, XdeMonitor *xmon)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> LeaveNotify: %p\n", xmon);
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
		fprintf(stderr, "<== LeaveNotify: %p\n", xmon);
	}
	if (xev->xcrossing.mode == NotifyNormal) {
		if (xmon->inside) {
			DPRINTF(1, "left popup\n");
			start_popup_timer(xmon);
			xmon->inside = False;
		}
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_FocusIn(Display *dpy, XEvent *xev, XdeMonitor *xmon)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> FocusIn: %p\n", xmon);
		fprintf(stderr, "    --> send_event = %s\n", xev->xfocus.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xfocus.window);
		fprintf(stderr, "    --> mode = %s\n", show_mode(xev->xfocus.mode));
		fprintf(stderr, "    --> detail = %s\n", show_detail(xev->xfocus.detail));
		fprintf(stderr, "<== FocusIn: %p\n", xmon);
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
event_handler_FocusOut(Display *dpy, XEvent *xev, XdeMonitor *xmon)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> FocusOut: %p\n", xmon);
		fprintf(stderr, "    --> send_event = %s\n", xev->xfocus.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xfocus.window);
		fprintf(stderr, "    --> mode = %s\n", show_mode(xev->xfocus.mode));
		fprintf(stderr, "    --> detail = %s\n", show_detail(xev->xfocus.detail));
		fprintf(stderr, "<== FocusOut: %p\n", xmon);
	}
	switch (xev->xfocus.mode) {
	case NotifyNormal:
	case NotifyGrab:
	case NotifyWhileGrabbed:
		DPRINTF(1, "unfocused popup\n");
		if (!xmon->keyboard) {
			DPRINTF(1, "no grab or focus\n");
			start_popup_timer(xmon);
		}
		break;
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
popup_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeMonitor *xmon = data;

	PTRACE(5);
	switch (xev->type) {
	case KeyPress:
		return event_handler_KeyPress(dpy, xev, xmon);
	case KeyRelease:
		return event_handler_KeyRelease(dpy, xev, xmon);
	case ButtonPress:
		return event_handler_ButtonPress(dpy, xev, xmon);
	case ButtonRelease:
		return event_handler_ButtonRelease(dpy, xev, xmon);
	case EnterNotify:
		return event_handler_EnterNotify(dpy, xev, xmon);
	case LeaveNotify:
		return event_handler_LeaveNotify(dpy, xev, xmon);
	case FocusIn:
		return event_handler_FocusIn(dpy, xev, xmon);
	case FocusOut:
		return event_handler_FocusOut(dpy, xev, xmon);
	}
	return GDK_FILTER_CONTINUE;
}

#if NEED_CURRENT_DESKTOP

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

#endif				/* NEED_CURRENT_DESKTOP */

/** @} */

/** @section Popup Window GTK Events
  * @{ */

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
	XdeMonitor *xmon = user;
	GdkEventGrabBroken *ev = (typeof(ev)) event;

	PTRACE(5);
	if (ev->keyboard) {
		DPRINTF(1, "keyboard grab was broken\n");
		xmon->keyboard = False;
		/* IF we lost a keyboard grab, it is because another hot-key was pressed, 
		   either doing something else or moving to another desktop.  Start the
		   timeout in this case. */
		start_popup_timer(xmon);
	} else {
		DPRINTF(1, "pointer grab was broken\n");
		xmon->pointer = False;
		/* If we lost a pointer grab, it is because somebody clicked on another
		   window.  In this case we want to drop the popup altogether.  This will 
		   break the keyboard grab if any. */
		drop_popup(xmon);
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
	return TRUE;		/* event handled */
}

static void
widget_realize(GtkWidget *popup, gpointer xmon)
{
	gdk_window_add_filter(popup->window, popup_handler, xmon);
	gdk_window_set_override_redirect(popup->window, TRUE);
	gdk_window_set_accept_focus(popup->window, FALSE);
	gdk_window_set_focus_on_map(popup->window, FALSE);
}

static gboolean
button_press_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
button_release_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
enter_notify_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
#if 0
	/* currently done by event handler, but considering grab */
	stop_popup_timer(xmon);
	xmon->inside = True;
#endif
	return GTK_EVENT_PROPAGATE;
}

static gboolean
focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
focus_out_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
	return GTK_EVENT_PROPAGATE;
}

static void
grab_focus(GtkWidget *widget, gpointer xmon)
{
}

static gboolean
key_press_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
key_release_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
	GdkEventKey *ev = (typeof(ev)) event;

	if (ev->is_modifier) {
		DPRINTF(1, "released key is modifier: dropping popup\n");
		drop_popup(xmon);
	}
	return GTK_EVENT_PROPAGATE;
}

static gboolean
leave_notify_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
#if 0
	/* currently done by event handler, but considering grab */
	start_popup_timer(xmon);
	xmon->inside = False;
#endif
	return GTK_EVENT_PROPAGATE;
}

static gboolean
map_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
scroll_event(GtkWidget *widget, GdkEvent *event, gpointer xmon)
{
	return GTK_EVENT_PROPAGATE;
}

static gboolean
visibility_notify_event(GtkWidget *popup, GdkEvent *event, gpointer xmon)
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

/** @} */

/** @} */

#endif				/* NEED_POPUP_WINDOW */

/** @section Startup Notification Handling
  * @{ */

#if NEED_STARTUP_IDS
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
		if (endptr && *endptr)
			break;
		if (pid_p)
			*pid_p = pid;
		q = p + 1;
		if (!(p = strchr(q, '-')))
			break;
		tmp = strndup(q, p - q);
		sequence = strtoul(tmp, &endptr, 10);
		free(tmp);
		if (endptr && *endptr)
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
		if (endptr && *endptr)
			break;
		if (timestamp_p)
			*timestamp_p = timestamp;
		return (True);
	}
	while (0);

	if (!timestamp && (p = strstr(id, "_TIME"))) {
		timestamp = strtoul(p + 5, &endptr, 10);
		if (!endptr || !*endptr)
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
del_sequence(XdeMonitor *xmon, Sequence *seq)
{
	gtk_list_store_remove(xmon->model, &seq->iter);
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
	xmon->seqcount--;
	if (xmon->seqcount <= 0)
		drop_popup(xmon);
}

static gboolean
seq_timeout(gpointer data)
{
	Sequence *seq = data;
	XdeMonitor *xmon = seq->xmon;

	del_sequence(xmon, seq);
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
	XdeMonitor *xmon = seq->xmon;

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
	markup = g_markup_printf_escaped("<b>%s</b>\n%s", name ? : "", desc ? : "");
	/* for now, ellipsize later */
	tip = desc;
		/* *INDENT-OFF* */
		gtk_list_store_set(xmon->model, &seq->iter,
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
		xscr->sequences = g_list_append(xscr->sequences, sn_seq);
		xmon = xscr->mons + seq->monitor;
		gtk_list_store_append(xmon->model, &seq->iter);
		xmon->seqcount++;
		seq->xmon = xmon;

		cha_sequence(xscr, seq);

		show_popup(xmon, FALSE, FALSE);
	}
	return (seq);
}

#endif				/* STARTUP_NOTIFICATION */
#endif				/* NEED_STARTUP_IDS */

/** @} */

/** @section Session Management
  * @{ */

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

#if 0
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

#if 0
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

#if 0
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
	prop[j].vals[0].value = "/usr/bin/xde-pager";
	prop[j].vals[0].length = strlen("/usr/bin/xde-pager");
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
	   restart it in the current session.  The RestartNever(3) style specifies that
	   the client does not wish to be restarted in the next session. */
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
	prop[j].vals[0].value = "/usr/bin/xde-pager";
	prop[j].vals[0].length = strlen("/usr/bin/xde-pager");
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
	gtk_main_quit();
}

static void
clientSaveCompleteCB(SmcConn smcConn, SmPointer data)
{
	if (saving_yourself) {
		saving_yourself = False;
		gtk_main_quit();
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
	gtk_main_quit();
}

static unsigned long clientCBMask = SmcSaveYourselfProcMask |
    SmcDieProcMask | SmcSaveCompleteProcMask | SmcShutdownCancelledProcMask;

static SmcCallbacks clientCBs = {
	.save_yourself = {.callback = &clientSaveYourselfCB,.client_data = NULL,},
	.die = {.callback = &clientDieCB,.client_data = NULL,},
	.save_complete = {.callback = &clientSaveCompleteCB,.client_data = NULL,},
	.shutdown_cancelled = {.callback = &clientShutdownCancelledCB,.client_data = NULL,},
};

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
put_resource(XrmDatabase xrdb, const char *resource, const char *value)
{
	put_nc_resource(xrdb, RESCLAS, resource, value);
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
	if ((val = putXrmInt(options.debug))) {
		put_resource(rdb, "debug", val);
		g_free(val);
	}
	if ((val = putXrmInt(options.output))) {
		put_resource(rdb, "output", val);
		g_free(val);
	}
	/* put a bunch of resources */
	if ((val = putXrmBool(options.trayicon))) {
		put_resource(rdb, "trayIcon", val);
		g_free(val);
	}
	if ((val = putXrmTime(options.timeout))) {
		put_resource(rdb, "timeout", val);
		g_free(val);
	}
	if ((val = putXrmInt(options.border))) {
		put_resource(rdb, "border", val);
		g_free(val);
	}

	XrmPutFileDatabase(rdb, usrdb);
	XrmDestroyDatabase(rdb);
	return;
}

/** @} */

/** @section Putting Key File
  * @{ */

void
put_keyfile(void)
{
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
	if ((val = get_resource(rdb, "output", "1")))
		getXrmInt(val, &options.output);
	/* get a bunch of resources */
	if ((val = get_resource(rdb, "trayIcon", NULL)))
		getXrmBool(val, &options.trayicon);
	if ((val = get_resource(rdb, "timeout", "1000")))
		getXrmTime(val, &options.timeout);
	if ((val = get_resource(rdb, "border", "5")))
		getXrmInt(val, &options.border);

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

/** @} */

/** @section Event handlers
  * @{ */

/** @section libwnck Event handlers
  * @{ */

/** @section Workspace Events
  * @{ */

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
	   libwnck+ does not support these well, so when xde-pager detects that it is
	   running under afterstep(1), it does nothing.  (It has a desktop button proxy,
	   but it does not relay scroll wheel events by default.) */
	if (!strcasecmp(xscr->wmname, "afterstep"))
		return False;
	/* XXX: awesome(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "awesome"))
		return True;
	/* XXX: When running with bbkeys(1), blackbox(1) has its own window cycling
	   feedback.  WHen running under blackbox(1), xde-cycle does nothing. */
	if (!strcasecmp(xscr->wmname, "blackbox"))
		return False;
	/* XXX: bspwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "bspwm"))
		return True;
	/* XXX: ctwm(1) is only GNOME/WinWM compliant and is not yet supported by
	   libwnck+.  Use etwm(1) instead.  xde-pager mitigates this somewhat, so it is
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
	   by xde-pager. */
	if (!strcasecmp(xscr->wmname, "dtwm"))
		return False;
	/* XXX: dwm(1) is barely ICCCM compliant.  It is not supported. */
	if (!strcasecmp(xscr->wmname, "dwm"))
		return False;
	/* XXX: echinus(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "echinus"))
		return True;
	/* XXX: etwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "etwm"))
		return True;
	/* XXX: failsafewm(1) has no desktops and is not supported. */
	if (!strcasecmp(xscr->wmname, "failsafewm"))
		return False;
	/* XXX: fluxbox(1) provides its own window cycling feedback.  When running under
	   fluxbox(1), xde-cycle does nothing. */
	if (!strcasecmp(xscr->wmname, "fluxbox"))
		return False;
	/* XXX: flwm(1) supports GNOME/WinWM but not EWMH/NetWM and is not currently
	   supported by libwnck+.  xde-pager mitigates this to some extent. */
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
	   _NET_DESKTOP_LAYOUT, and key bindings are confused.  When xde-pager detects
	   that it is running under jwm(1) it will simply do nothing. */
	if (!strcasecmp(xscr->wmname, "jwm"))
		return False;
	/* XXX: matwm2(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "matwm2"))
		return True;
	/* XXX: metacity(1) provides its own competent desktop switching feedback pop-up. 
	   When xde-pager detects that it is running under metacity(1), it will simply do 
	   nothing. */
	if (!strcasecmp(xscr->wmname, "metacity"))
		return False;
	/* XXX: mwm(1) only supports OSF/Motif and does not support multiple desktops. It 
	   is not supported. */
	if (!strcasecmp(xscr->wmname, "mwm"))
		return False;
	/* XXX: mutter(1) has not been tested. */
	if (!strcasecmp(xscr->wmname, "mutter"))
		return True;
	/* XXX: openbox(1) provides its own meager desktop switching feedback pop-up.  It 
	   does respect _NET_DESKTOP_LAYOUT but does not provide any of the contents of
	   the desktop. When both are running it is a little confusing, so when xde-pager 
	   detects that it is running under openbox(1), it will simply do nothing. */
	if (!strcasecmp(xscr->wmname, "openbox"))
		return False;
	/* XXX: pekwm(1) provides its own broken desktop switching feedback pop-up;
	   however, it does not respect _NET_DESKTOP_LAYOUT and key bindings are
	   confused.  When xde-pager detects that it is running under pekwm(1), it will
	   simply do nothing. */
	if (!strcasecmp(xscr->wmname, "pekwm"))
		return False;
	/* XXX: spectrwm(1) is supported, but it doesn't work that well because, like
	   cwm(1), spectrwm(1) is not placing _NET_WM_STATE on client windows, so
	   libwnck+ cannot locate them and will not provide contents in the pager. */
	if (!strcasecmp(xscr->wmname, "spectrwm"))
		return True;
	/* XXX: twm(1) does not support multiple desktops and is not supported. */
	if (!strcasecmp(xscr->wmname, "twm"))
		return False;
	/* XXX: uwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "uwm"))
		return True;
	/* XXX: vtwm(1) is barely ICCCM compliant and currently unsupported: use etwm
	   instead. */
	if (!strcasecmp(xscr->wmname, "vtwm"))
		return False;
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
	/* XXX: wmaker(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "wmaker"))
		return True;
	/* XXX: wmii(1) is supported and works well.  wmii(1) was stealing the focus back 
	   from the pop-up, but this was fixed. */
	if (!strcasecmp(xscr->wmname, "wmii"))
		return True;
	/* XXX: wmx(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "wmx"))
		return True;
	/* XXX: xdwm(1) does not support EWMH/NetWM for desktops. */
	if (!strcasecmp(xscr->wmname, "xdwm"))	/* XXX */
		return False;
	/* XXX: yeahwm(1) does not support EWMH/NetWM and is currently unsupported.  The
	   pager will simply not do anything while this window manager is running. */
	if (!strcasecmp(xscr->wmname, "yeahwm"))
		return True;
	return True;
}

#if NEED_DESKTOP_INFO
static void setup_button_proxy(XdeScreen *xscr);
#endif				/* NEED_DESKTOP_INFO */

static void
window_manager_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = user;
	const char *name;

	PTRACE(5);
	wnck_screen_force_update(wnck);
#if NEED_DESKTOP_INFO
	if (options.proxy)
		setup_button_proxy(xscr);
#endif				/* NEED_DESKTOP_INFO */
	free(xscr->wmname);
	xscr->wmname = NULL;
	xscr->goodwm = False;
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
		xscr->goodwm = good_window_manager(xscr);
	}
	DPRINTF(1, "window manager is '%s'\n", xscr->wmname);
	DPRINTF(1, "window manager is %s\n", xscr->goodwm ? "usable" : "unusable");
}

#if NEED_DESKTOP_INFO || NEED_ROOT_PIXMAPS

static void
something_changed(WnckScreen *wnck, XdeMonitor *xmon)
{
#if NEED_POPUP_WINDOW
	show_popup(xmon, TRUE, TRUE);
#endif				/* NEED_POPUP_WINDOW */
}

static void
workspace_destroyed(WnckScreen *wnck, WnckWorkspace *space, gpointer data)
{
	XdeScreen *xscr = data;
	int i;

	if (xscr->goodwm)
		for (i = 0; i < xscr->nmon; i++)
			something_changed(wnck, &xscr->mons[i]);
}

static void
workspace_created(WnckScreen *wnck, WnckWorkspace *space, gpointer data)
{
#if 0
	XdeScreen *xscr = data;
	int i;

	if (xscr->goodwm)
		for (i = 0; i < xscr->nmon; i++)
			something_changed(wnck, &xscr->mons[i]);
#endif
}

static void
viewports_changed(WnckScreen *wnck, gpointer data)
{
	XdeScreen *xscr = data;
	int i;

	if (xscr->goodwm)
		for (i = 0; i < xscr->nmon; i++)
			something_changed(wnck, &xscr->mons[i]);
}

static void
background_changed(WnckScreen *wnck, gpointer data)
{
#if 0
	XdeScreen *xscr = data;
	int i;

	if (xscr->goodwm)
		for (i = 0; i < xscr->nmon; i++)
			something_changed(wnck, &xscr->mons[i]);
#endif
}

static void
active_workspace_changed(WnckScreen *wnck, WnckWorkspace *prev, gpointer data)
{
	XdeScreen *xscr = data;
	int i;

	if (xscr->goodwm)
		for (i = 0; i < xscr->nmon; i++)
			something_changed(wnck, &xscr->mons[i]);
}

#endif				/* DESKTOP_INFO || NEED_ROOT_PIXMAPS */

/** @} */

/** @section Window Events
  * @{ */

#if NEED_CLIENT_INFO

static void
windows_changed(WnckScreen *wnck, XdeMonitor *xmon)
{
#if NEED_POPUP_WINDOW
	show_popup(xmon, TRUE, TRUE);
#endif				/* NEED_POPUP_WINDOW */
}

static void
active_window_changed(WnckScreen *wnck, WnckWindow *previous, gpointer user)
{
	XdeScreen *xscr = user;
	WnckWindow *active = wnck_screen_get_active_window(wnck);
	int i;

	if (!active || !previous) {
#if NEED_POPUP_WINDOW
		for (i = 0; i < xscr->nmon; i++) {
			stop_popup_timer(&xscr->mons[i]);
			drop_popup(&xscr->mons[i]);
		}
#endif				/* NEED_POPUP_WINDOW */
		return;
	}
	if (xscr->goodwm)
		for (i = 0; i < xscr->nmon; i++)
			windows_changed(wnck, &xscr->mons[i]);
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
	clients_changed(wnck, xscr);
}

static void
window_stacking_changed(WnckScreen *wnck, gpointer xscr)
{
	clients_changed(wnck, xscr);
}

#endif				/* NEED_CLIENT_INFO */

/** @} */

/** @} */

/** @section X Event Handlers
  * @{ */

#if NEED_STARTUP_IDS
#ifdef STARTUP_NOTIFICATION
static void
sn_handler(SnMonitorEvent * event, void *data)
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
#endif				/* NEED_STARTUP_IDS */

static void
update_client_list(XdeScreen *xscr, Atom prop)
{
#if NEED_CLIENT_INFO
	if (prop == None || prop == _XA_NET_CLIENT_LIST_STACKING) {
	}
	if (prop == None || prop == _XA_NET_CLIENT_LIST) {
	} else if (prop == None || prop == _XA_WIN_CLIENT_LIST) {
	}
#endif				/* NEED_CLIENT_INFO */
}

static void
update_window(XdeScreen *xscr, Atom prop)
{
}

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

#if NEED_ROOT_PIXMAPS
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
#endif				/* NEED_ROOT_PIXMAPS */

#if NEED_CURRENT_DESKTOP
static void
update_current_desktop(XdeScreen *xscr, Atom prop)
{
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0, i, j = 0, *x;
	unsigned long *data = NULL;
	Bool changed = False;
	XdeMonitor *xmon;

	PTRACE(5);
	if (prop == None || prop == _XA_WM_DESKTOP) {
		if (XGetWindowProperty(dpy, root, _XA_WM_DESKTOP, 0, 64, False,
				       XA_CARDINAL, &actual, &format, &nitems, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && nitems >= 1 && data) {
			if (xscr->current != (int) data[0]) {
				xscr->current = data[0];
				changed = True;
			}
			x = (xscr->mhaware = (nitems >= xscr->nmon)) ? &i : &j;
			for (i = 0, xmon = xscr->mons; i < xscr->nmon; i++, xmon++) {
				if (xmon->current != (int) data[*x]) {
					xmon->current = data[*x];
					changed = True;
				}
			}
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
			if (xscr->current != (int) data[0]) {
				xscr->current = data[0];
				changed = True;
			}
			x = (xscr->mhaware = (nitems >= xscr->nmon)) ? &i : &j;
			for (i = 0, xmon = xscr->mons; i < xscr->nmon; i++, xmon++) {
				if (xmon->current != (int) data[*x]) {
					xmon->current = data[*x];
					changed = True;
				}
			}
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
			if (xscr->current != (int) data[0]) {
				xscr->current = data[0];
				changed = True;
			}
			x = (xscr->mhaware = (nitems >= xscr->nmon)) ? &i : &j;
			for (i = 0, xmon = xscr->mons; i < xscr->nmon; i++, xmon++) {
				if (xmon->current != (int) data[*x]) {
					xmon->current = data[*x];
					changed = True;
				}
			}
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (changed) {
		DPRINTF(1, "Current desktop changed.\n");
		/* FIXME: if changed update something */
		add_deferred_refresh_desktop(xscr);
	} else
		DPRINTF(1, "No change in current desktop.\n");
}

static GdkFilterReturn
proxy_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = (typeof(xscr)) data;
	int num;

	PTRACE(5);
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	switch (xev->type) {
	case ButtonPress:
		if (options.debug) {
			fprintf(stderr, "==> ButtonPress: %p\n", xscr);
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
			fprintf(stderr, "<== ButtonPress: %p\n", xscr);
		}
		switch (xev->xbutton.button) {
		case 4:
			update_current_desktop(xscr, None);
			num = (xscr->current - 1 + xscr->desks) % xscr->desks;
#if 0
			set_current_desktop(xscr, num, xev->xbutton.time);
			return GDK_FILTER_REMOVE;
#else
			(void) num;
			break;
#endif
		case 5:
			update_current_desktop(xscr, None);
			num = (xscr->current + 1 + xscr->desks) % xscr->desks;
#if 0
			set_current_desktop(xscr, num, xev->xbutton.time);
			return GDK_FILTER_REMOVE;
#else
			(void) num;
			break;
#endif
		}
		return GDK_FILTER_CONTINUE;
	case ButtonRelease:
		if (options.debug > 1) {
			fprintf(stderr, "==> ButtonRelease: %p\n", xscr);
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
			fprintf(stderr, "<== ButtonRelease: %p\n", xscr);
		}
		return GDK_FILTER_CONTINUE;
	case PropertyNotify:
		if (options.debug > 2) {
			fprintf(stderr, "==> PropertyNotify:\n");
			fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
			fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
			fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
			fprintf(stderr, "    --> state = %s\n",
				(xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
			fprintf(stderr, "<== PropertyNotify:\n");
		}
		return GDK_FILTER_CONTINUE;
	}
	EPRINTF("wrong message type for handler %d on window 0x%08lx\n", xev->type, xev->xany.window);
	return GDK_FILTER_CONTINUE;
}

#endif				/* NEED_CURRENT_DESKTOP */

#if NEED_DESKTOP_LAYOUT || NEED_ROOT_PIXMAPS
static void
refresh_layout(XdeScreen *xscr)
{
	unsigned int w, h, f, wmax, hmax;
	int i;

	w = xscr->width * xscr->cols;
	h = xscr->height * xscr->rows;
	for (i = 0; i < xscr->nmon; i++) {
		XdeMonitor *xmon = &xscr->mons[i];

		wmax = (xmon->geom.width * 8) / 10;
		hmax = (xmon->geom.height * 8) / 10;
		for (f = 10; w > wmax * f || h > hmax * f; f++) ;
		gtk_window_set_default_size(GTK_WINDOW(xmon->popup), w / f, h / f);
	}
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
		add_deferred_refresh_layout(xscr);
	}
}
#endif				/* NEED_DESKTOP_LAYOUT */

static void
update_theme(XdeScreen *xscr, Atom prop)
{
	Window root = RootWindow(dpy, xscr->index);
	XTextProperty xtp = { NULL, };
	char **list = NULL;
	int strings = 0;
	Bool changed = False;
	GtkSettings *set;

	PTRACE(5);
	gtk_rc_reparse_all();
	if (XGetTextProperty(dpy, root, &xtp, _XA_XDE_THEME_NAME)) {
		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				static const char *prefix = "gtk-theme-name=\"";
				static const char *suffix = "\"";
				char *rc_string;
				int len;

				len = strlen(prefix) + strlen(list[0]) + strlen(suffix) + 1;
				rc_string = calloc(len, sizeof(*rc_string));
				strncpy(rc_string, prefix, len);
				strncat(rc_string, list[0], len);
				strncat(rc_string, suffix, len);
				gtk_rc_parse_string(rc_string);
				free(rc_string);
				if (!xscr->theme || strcmp(xscr->theme, list[0])) {
					free(xscr->theme);
					xscr->theme = strdup(list[0]);
					changed = True;
				}
			}
			if (list)
				XFreeStringList(list);
		} else
			EPRINTF("could not get text list for property\n");
		if (xtp.value)
			XFree(xtp.value);
	} else
		DPRINTF(1, "could not get _XDE_THEME_NAME for root 0x%lx\n", root);
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
#if NEED_ROOT_PIXMAPS
		read_theme(xscr);
#endif				/* NEED_ROOT_PIXMAPS */
	} else
		DPRINTF(1, "No change in current theme %s\n", xscr->theme);
}

static void
update_screen(XdeScreen *xscr)
{
#if NEED_ROOT_PIXMAPS
	update_root_pixmap(xscr, None);
#endif
#if NEED_DESKTOP_LAYOUT
	update_layout(xscr, None);
#endif
#if NEED_DESKTOP_INFO
	update_current_desktop(xscr, None);
#endif
	update_theme(xscr, None);
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
  * The number and/or size of monitors belonging to a screen have changed.  This
  * may be as a result of RANDR or XINERAMA changes.  Walk through the monitors
  * and adjust the necessary parameters.
  */
static void
on_monitors_changed(GdkScreen *scrn, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	wnck_screen_force_update(xscr->wnck);
	refresh_screen(xscr, scrn);
#if NEED_DESKTOP_LAYOUT
	refresh_layout(xscr);
#endif				/* NEED_DESKTOP_LAYOUT */
#if NEED_ROOT_PIXMAPS
	read_theme(xscr);
#endif				/* NEED_ROOT_PIXMAPS */
}

/** @brief screen size changed
  *
  * The size of the screen changed.  This may be as a result of RANDR or
  * XINERAMA changes.  Walk through the screen and the monitors on the screen
  * and adjust the necessary parameters.
  */
static void
on_size_changed(GdkScreen *scrn, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	wnck_screen_force_update(xscr->wnck);
	refresh_screen(xscr, scrn);
#if NEED_DESKTOP_LAYOUT
	refresh_layout(xscr);
#endif				/* NEED_DESKTOP_LAYOUT */
#if NEED_ROOT_PIXMAPS
	read_theme(xscr);
#endif				/* NEED_ROOT_PIXMAPS */
}

static void
edit_input(XdeScreen *xscr)
{
}

static void
show_stray(XdeScreen *xscr)
{
}

static GdkFilterReturn
event_handler_ClientMessage(Display *dpy, XEvent *xev)
{
	XdeScreen *xscr = NULL;
	int s, nscr = ScreenCount(dpy);

	for (s = 0; s < nscr; s++)
		if (xev->xclient.window == RootWindow(dpy, s)) {
			xscr = screens + s;
			break;
		}

	PTRACE(5);
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
		fprintf(stderr, "<== ClientMessage: %p\n", xscr);
	}
	if (xscr) {
		if (xev->xclient.message_type == _XA_GTK_READ_RCFILES) {
			update_theme(xscr, xev->xclient.message_type);
			return GDK_FILTER_REMOVE;	/* event handled */
		} else if (xev->xclient.message_type == _XA_PREFIX_EDIT) {
			edit_input(xscr);
			return GDK_FILTER_REMOVE;
		} else if (xev->xclient.message_type == _XA_PREFIX_TRAY) {
			show_stray(xscr);
			return GDK_FILTER_REMOVE;
		}
	} else
		xscr = screens;
#if NEED_STARTUP_IDS
	if (xev->xclient.message_type == _XA_NET_STARTUP_INFO) {
#ifdef STARTUP_NOTIFICATION
		return sn_display_process_event(sn_dpy, xev);
#endif
	} else if (xev->xclient.message_type == _XA_NET_STARTUP_INFO_BEGIN) {
#ifdef STARTUP_NOTIFICATION
		return sn_display_process_event(sn_dpy, xev);
#endif
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
		return event_handler_ClientMessage(dpy, xev);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;	/* event not handled, continue processing */
}

static GdkFilterReturn
event_handler_SelectionClear(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	PTRACE(5);
	if (options.debug > 1) {
		fprintf(stderr, "==> SelectionClear: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n", xev->xselectionclear.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xselectionclear.window);
		fprintf(stderr, "    --> selection = %s\n", XGetAtomName(dpy, xev->xselectionclear.selection));
		fprintf(stderr, "    --> time = %lu\n", xev->xselectionclear.time);
		fprintf(stderr, "<== SelectionClear: %p\n", xscr);
	}
	if (xscr && xev->xselectionclear.window == xscr->selwin) {
		XDestroyWindow(dpy, xscr->selwin);
		EPRINTF("selection cleared, exiting\n");
		gtk_main_quit();
		return GDK_FILTER_REMOVE;
	}
#if NEED_DESKTOP_LAYOUT
	if (xscr && xev->xselectionclear.window == xscr->laywin) {
		XDestroyWindow(dpy, xscr->laywin);
		xscr->laywin = None;
	}
#endif				/* NEED_DESKTOP_LAYOUT */
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
		return event_handler_SelectionClear(dpy, xev, xscr);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;
}

#if NEED_DESKTOP_LAYOUT
static GdkFilterReturn
laywin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = data;

	PTRACE(5);
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	switch (xev->type) {
	case SelectionClear:
		return event_handler_SelectionClear(dpy, xev, xscr);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;
}
#endif				/* NEED_DESKTOP_LAYOUT */

static GdkFilterReturn
event_handler_PropertyNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
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
	if (xev->xproperty.atom == _XA_XDE_THEME_NAME && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_theme(xscr, xev->xproperty.atom);
		return GDK_FILTER_REMOVE;	/* event handled */
#if NEED_DESKTOP_LAYOUT
	} else if (xev->xproperty.atom == _XA_NET_DESKTOP_LAYOUT && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_layout(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_NET_NUMBER_OF_DESKTOPS && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_layout(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_WIN_WORKSPACE_COUNT && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_layout(xscr, xev->xproperty.atom);
#endif				/* NEED_DESKTOP_LAYOUT */
#if NEED_CURRENT_DESKTOP
	} else if (xev->xproperty.atom == _XA_NET_CURRENT_DESKTOP && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_current_desktop(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_WIN_WORKSPACE && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_current_desktop(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_WM_DESKTOP && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_current_desktop(xscr, xev->xproperty.atom);
#endif				/* NEED_CURRENT_DESKTOP */
#if NEED_ROOT_PIXMAPS
	} else if (xev->xproperty.atom == _XA_XROOTPMAP_ID && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_root_pixmap(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_ESETROOT_PMAP_ID && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_root_pixmap(xscr, xev->xproperty.atom);
#endif				/* NEED_ROOT_PIXMAPS */
#if NEED_CLIENT_INFO
	} else if (xev->xproperty.atom == _XA_NET_ACTIVE_WINDOW && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_window(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_NET_CLIENT_LIST && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_client_list(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_NET_CLIENT_LIST_STACKING && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_client_list(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_WIN_FOCUS && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_window(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_WIN_CLIENT_LIST && xev->xproperty.state == PropertyNewValue) {
		PTRACE(5);
		update_client_list(xscr, xev->xproperty.atom);
#endif				/* NEED_CLIENT_INFO */
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
		return event_handler_PropertyNotify(dpy, xev, xscr);
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
		XGetErrorDatabaseText(dpy, "xdg-launch", num, def, req, sizeof(req));
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

/** @} */

/** @section Initialization
  * @{ */

#if NEED_POPUP_WINDOW

#if NEED_PAGER
static void
add_pager(XdeScreen *xscr, XdeMonitor *xmon, GtkWidget *popup)
{
	GtkWidget *pager = wnck_pager_new(xscr->wnck);

	wnck_pager_set_orientation(WNCK_PAGER(pager), GTK_ORIENTATION_HORIZONTAL);
	wnck_pager_set_n_rows(WNCK_PAGER(pager), 2);
	wnck_pager_set_layout_policy(WNCK_PAGER(pager), WNCK_PAGER_LAYOUT_POLICY_AUTOMATIC);
	wnck_pager_set_display_mode(WNCK_PAGER(pager), WNCK_PAGER_DISPLAY_CONTENT);
	wnck_pager_set_show_all(WNCK_PAGER(pager), TRUE);
	wnck_pager_set_shadow_type(WNCK_PAGER(pager), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(popup), GTK_WIDGET(pager));
	gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_widget_show(GTK_WIDGET(pager));
	xmon->pager = pager;
}
#endif				/* NEED_PAGER */

#if NEED_TASKS
static void
add_tasks(XdeScreen *xscr, XdeMonitor *xmon, GtkWidget *popup)
{
	GtkWidget *tasks = wnck_tasklist_new(xscr->wnck);

	wnck_tasklist_set_grouping(WNCK_TASKLIST(tasks), WNCK_TASKLIST_NEVER_GROUP);
	wnck_tasklist_set_include_all_workspaces(WNCK_TASKLIST(tasks), FALSE);
	wnck_tasklist_set_switch_workspace_on_unminimize(WNCK_TASKLIST(tasks), FALSE);
	wnck_tasklist_set_button_relief(WNCK_TASKLIST(tasks), GTK_RELIEF_HALF);
	/* use wnck_tasklist_get_size_hint_list() to size tasks */
	gtk_container_add(GTK_CONTAINER(popup), GTK_WIDGET(tasks));
	gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_widget_show(GTK_WIDGET(tasks));
	xmon->tasks = tasks;
}
#endif				/* NEED_TASKS */

#if NEED_MODEL
static void
add_model(XdeScreen *xscr, XdeMonitor *xmon, GtkWidget *popup)
{
	GtkWidget *view;
	GtkListStore *model;

	DPRINTF(1, "monitor %d: creating list store\n", xmon->index);
	/* *INDENT-OFF* */
	xmon->model = model = gtk_list_store_new(6
			,GDK_TYPE_PIXBUF	/* icon */
			,G_TYPE_STRING		/* name */
			,G_TYPE_STRING		/* description */
			,G_TYPE_STRING		/* markup */
			,G_TYPE_STRING		/* tooltip */
			,G_TYPE_POINTER		/* seq */
			);
	/* *INDENT-ON* */
	DPRINTF(1, "monitor %d: creating icon view\n", xmon->index);
	xmon->view = view = gtk_icon_view_new_with_model(GTK_TREE_MODEL(model));
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(view), 0);
	gtk_icon_view_set_markup_column(GTK_ICON_VIEW(view), 3);
	gtk_icon_view_set_tooltip_column(GTK_ICON_VIEW(view), 4);
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(view), GTK_SELECTION_NONE);
	gtk_icon_view_set_item_orientation(GTK_ICON_VIEW(view), GTK_ORIENTATION_HORIZONTAL);
	gtk_icon_view_set_columns(GTK_ICON_VIEW(view), -1);
	gtk_icon_view_set_item_width(GTK_ICON_VIEW(view), -1);
	gtk_icon_view_set_spacing(GTK_ICON_VIEW(view), 5);
	gtk_icon_view_set_row_spacing(GTK_ICON_VIEW(view), 2);
	gtk_icon_view_set_column_spacing(GTK_ICON_VIEW(view), 2);
	gtk_icon_view_set_margin(GTK_ICON_VIEW(view), 5);
	gtk_icon_view_set_item_padding(GTK_ICON_VIEW(view), 3);

	gtk_container_add(GTK_CONTAINER(popup), view);
}
#endif				/* NEED_MODEL */

static void
add_items(XdeScreen *xscr, XdeMonitor *xmon, GtkWidget *popup)
{
#if NEED_PAGER
	return add_pager(xscr, xmon, popup);
#endif
#if NEED_TASKS
	return add_tasks(xscr, xmon, popup);
#endif
#if NEED_MODEL
	return add_model(xscr, xmon, popup);
#endif
}

static void
init_window(XdeScreen *xscr, XdeMonitor *xmon)
{
	GtkWidget *popup;

	PTRACE(5);
	xmon->popup = popup = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_add_events(popup, GDK_ALL_EVENTS_MASK);
	/* don't want the window accepting focus: it messes up the window manager's
	   setting of focus after the desktop has changed... */
	gtk_window_set_accept_focus(GTK_WINDOW(popup), FALSE);
	/* no, don't want it acceptin focus when it is mapped either */
	gtk_window_set_focus_on_map(GTK_WINDOW(popup), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(popup), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
	gtk_window_stick(GTK_WINDOW(popup));
	gtk_window_set_keep_above(GTK_WINDOW(popup), TRUE);

	add_items(xscr, xmon, popup);

	gtk_container_set_border_width(GTK_CONTAINER(popup), options.border);

	g_signal_connect(G_OBJECT(popup), "button_press_event", G_CALLBACK(button_press_event), xmon);
	g_signal_connect(G_OBJECT(popup), "button_release_event", G_CALLBACK(button_release_event), xmon);
	g_signal_connect(G_OBJECT(popup), "enter_notify_event", G_CALLBACK(enter_notify_event), xmon);
	g_signal_connect(G_OBJECT(popup), "focus_in_event", G_CALLBACK(focus_in_event), xmon);
	g_signal_connect(G_OBJECT(popup), "focus_out_event", G_CALLBACK(focus_out_event), xmon);
	g_signal_connect(G_OBJECT(popup), "grab_broken_event", G_CALLBACK(grab_broken_event), xmon);
	g_signal_connect(G_OBJECT(popup), "grab_focus", G_CALLBACK(grab_focus), xmon);
	g_signal_connect(G_OBJECT(popup), "key_press_event", G_CALLBACK(key_press_event), xmon);
	g_signal_connect(G_OBJECT(popup), "key_release_event", G_CALLBACK(key_release_event), xmon);
	g_signal_connect(G_OBJECT(popup), "leave_notify_event", G_CALLBACK(leave_notify_event), xmon);
	g_signal_connect(G_OBJECT(popup), "map_event", G_CALLBACK(map_event), xmon);
	g_signal_connect(G_OBJECT(popup), "realize", G_CALLBACK(widget_realize), xmon);
	g_signal_connect(G_OBJECT(popup), "scroll_event", G_CALLBACK(scroll_event), xmon);
	g_signal_connect(G_OBJECT(popup), "visibility_notify_event", G_CALLBACK(visibility_notify_event), xmon);
	g_signal_connect(G_OBJECT(popup), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
}

#endif				/* NEED_POPUP_WINDOW */

#if NEED_DESKTOP_INFO

static void
setup_button_proxy(XdeScreen *xscr)
{
	Window root = RootWindow(dpy, xscr->index), proxy;
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;

	PTRACE(5);
	xscr->proxy = NULL;
	if (XGetWindowProperty(dpy, root, _XA_WIN_DESKTOP_BUTTON_PROXY,
			       0, 1, False, XA_CARDINAL, &actual, &format,
			       &nitems, &after, (unsigned char **) &data) == Success &&
	    format == 32 && nitems >= 1 && data) {
		proxy = data[0];
		if ((xscr->proxy = gdk_x11_window_foreign_new_for_display(disp, proxy))) {
			GdkEventMask mask;

			mask = gdk_window_get_events(xscr->proxy);
			mask |= GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK | GDK_SUBSTRUCTURE_MASK;
			gdk_window_set_events(xscr->proxy, mask);
			DPRINTF(1, "adding filter for desktop button proxy\n");
			gdk_window_add_filter(xscr->proxy, proxy_handler, xscr);
		}
	}
	if (data) {
		XFree(data);
		data = NULL;
	}
}

#endif				/* NEED_DESKTOP_INFO */

static void
init_monitors(XdeScreen *xscr)
{
	XdeMonitor *xmon;
	int m;

	g_signal_connect(G_OBJECT(xscr->scrn), "monitors-changed", G_CALLBACK(on_monitors_changed), xscr);
	g_signal_connect(G_OBJECT(xscr->scrn), "size-changed", G_CALLBACK(on_size_changed), xscr);

	xscr->nmon = gdk_screen_get_n_monitors(xscr->scrn);
	xscr->mons = calloc(xscr->nmon + 1, sizeof(*xscr->mons));
	for (m = 0, xmon = xscr->mons; m < xscr->nmon; m++, xmon++) {
		xmon->index = m;
		xmon->xscr = xscr;
		gdk_screen_get_monitor_geometry(xscr->scrn, m, &xmon->geom);
#if NEED_POPUP_WINDOW
		init_window(xscr, xmon);
#endif				/* NEED_POPUP_WINDOW */
	}
}

static void
init_wnck(XdeScreen *xscr)
{
	WnckScreen *wnck = xscr->wnck = wnck_screen_get(xscr->index);

	g_signal_connect(G_OBJECT(wnck), "window_manager_changed", G_CALLBACK(window_manager_changed), xscr);

#if NEED_DESKTOP_INFO
	g_signal_connect(G_OBJECT(wnck), "workspace_destroyed", G_CALLBACK(workspace_destroyed), xscr);
	g_signal_connect(G_OBJECT(wnck), "workspace_created", G_CALLBACK(workspace_created), xscr);
	g_signal_connect(G_OBJECT(wnck), "viewports_changed", G_CALLBACK(viewports_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "background_changed", G_CALLBACK(background_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "active_workspace_changed", G_CALLBACK(active_workspace_changed), xscr);
#endif				/* NEED_DESKTOP_INFO */
#if NEED_CLIENT_INFO
	g_signal_connect(G_OBJECT(wnck), "active_window_changed", G_CALLBACK(active_window_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "application_closed", G_CALLBACK(application_closed), xscr);
	g_signal_connect(G_OBJECT(wnck), "application_opened", G_CALLBACK(application_opened), xscr);
	g_signal_connect(G_OBJECT(wnck), "class_group_closed", G_CALLBACK(class_group_closed), xscr);
	g_signal_connect(G_OBJECT(wnck), "class_group_opened", G_CALLBACK(class_group_opened), xscr);
	g_signal_connect(G_OBJECT(wnck), "window_closed", G_CALLBACK(window_closed), xscr);
	g_signal_connect(G_OBJECT(wnck), "window_opened", G_CALLBACK(window_opened), xscr);
	g_signal_connect(G_OBJECT(wnck), "window_stacking_changed", G_CALLBACK(window_stacking_changed), xscr);
#endif				/* NEED_CLIENT_INFO */

	wnck_screen_force_update(wnck);
	window_manager_changed(wnck, xscr);
}

static gboolean
on_ifd_watch(GIOChannel *chan, GIOCondition cond, pointer data)
{
	SmcConn smcConn = data;
	IceConn iceConn = SmcGetIceConnection(smcConn);

	if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR)) {
		EPRINTF("poll failed: %s %s %s\n",
			(cond & G_IO_NVAL) ? "NVAL" : "",
			(cond & G_IO_HUP) ? "HUP" : "", (cond & G_IO_ERR) ? "ERR" : "");
		return G_SOURCE_REMOVE;	/* remove event source */
	} else if (cond & (G_IO_IN | G_IO_PRI)) {
		IceProcessMessages(iceConn, NULL, NULL);
	}
	return G_SOURCE_CONTINUE;	/* keep event source */
}

static void
init_smclient(void)
{
	char err[256] = { 0, };
	GIOChannel *chan;
	int ifd, mask = G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_PRI;
	char *env;
	SmcConn smcConn;
	IceConn iceConn;

	if (!(env = getenv("SESSION_MANAGER"))) {
		if (options.clientId)
			EPRINTF("clientId provided but no SESSION_MANAGER\n");
		return;
	}
	smcConn = SmcOpenConnection(env, NULL, SmProtoMajor, SmProtoMinor,
				    clientCBMask, &clientCBs, options.clientId,
				    &options.clientId, sizeof(err), err);
	if (!smcConn) {
		EPRINTF("SmcOpenConnection: %s\n", err);
		return;
	}
	iceConn = SmcGetIceConnection(smcConn);
	ifd = IceConnectionNumber(iceConn);
	chan = g_io_channel_unix_new(ifd);
	g_io_add_watch(chan, mask, on_ifd_watch, smcConn);
}

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

	init_smclient();

	gtk_init(&argc, &argv);

	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);

	if (options.screen >= 0 && options.screen >= nscr) {
		EPRINTF("bad screen specified: %d\n", options.screen);
		exit(EXIT_FAILURE);
	}

	dpy = GDK_DISPLAY_XDISPLAY(disp);

#if NEED_STARTUP_IDS
#ifdef STARTUP_NOTIFICATION
	sn_dpy = sn_display_new(dpy, NULL, NULL);
#endif
#endif

	atom = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_XA_XDE_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_GTK_READ_RCFILES");
	_XA_GTK_READ_RCFILES = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);

#if NEED_DESKTOP_LAYOUT
	atom = gdk_atom_intern_static_string("_NET_DESKTOP_LAYOUT");
	_XA_NET_DESKTOP_LAYOUT = gdk_x11_atom_to_xatom_for_display(disp, atom);
#endif				/* NEED_DESKTOP_LAYOUT */

#if NEED_CURRENT_DESKTOP
	atom = gdk_atom_intern_static_string("_NET_CURRENT_DESKTOP");
	_XA_NET_CURRENT_DESKTOP = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKSPACE");
	_XA_WIN_WORKSPACE = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("WM_DESKTOP");
	_XA_WM_DESKTOP = gdk_x11_atom_to_xatom_for_display(disp, atom);
#endif				/* NEED_CURRENT_DESKTOP */

#if NEED_DESKTOP_INFO
	atom = gdk_atom_intern_static_string("_NET_DESKTOP_NAMES");
	_XA_NET_DESKTOP_NAMES = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_NUMBER_OF_DESKTOPS");
	_XA_NET_NUMBER_OF_DESKTOPS = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_DESKTOP_BUTTON_PROXY");
	_XA_WIN_DESKTOP_BUTTON_PROXY = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKSPACE_COUNT");
	_XA_WIN_WORKSPACE_COUNT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_WM_ICON_GEOMETRY");
	_XA_NET_WM_ICON_GEOMETRY = gdk_x11_atom_to_xatom_for_display(disp, atom);
#endif				/* NEED_DESKTOP_INFO */

#if NEED_ROOT_PIXMAPS
	atom = gdk_atom_intern_static_string("_XROOTPMAP_ID");
	_XA_XROOTPMAP_ID = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("ESETROOT_PMAP_ID");
	_XA_ESETROOT_PMAP_ID = gdk_x11_atom_to_xatom_for_display(disp, atom);
#endif				/* NEED_ROOT_PIXMAPS */

#if NEED_CLIENT_INFO
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
#endif				/* NEED_CLIENT_INFO */

#if NEED_STARTUP_IDS
#ifdef STARTUP_NOTIFICATION
	atom = gdk_atom_intern_static_string("_NET_STARTUP_INFO");
	_XA_NET_STARTUP_INFO = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);

	atom = gdk_atom_intern_static_string("_NET_STARTUP_INFO_BEGIN");
	_XA_NET_STARTUP_INFO_BEGIN = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);
#endif				/* STARTUP_NOTIFICATION */
#endif				/* NEED_STARTUP_IDS */

	atom = gdk_atom_intern_static_string(XA_PREFIX "_EDIT");
	_XA_PREFIX_EDIT = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);

	atom = gdk_atom_intern_static_string(XA_PREFIX "_TRAY");
	_XA_PREFIX_TRAY = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);

	scrn = gdk_display_get_default_screen(disp);
	root = gdk_screen_get_root_window(scrn);
	mask = gdk_window_get_events(root);
	mask |= GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK | GDK_SUBSTRUCTURE_MASK;
	gdk_window_set_events(root, mask);

	wnck_set_client_type(WNCK_CLIENT_TYPE_PAGER);
}

#if NEED_DESKTOP_LAYOUT
Window
get_desktop_layout_selection(XdeScreen *xscr)
{
	GdkDisplay *disp = gdk_screen_get_display(xscr->scrn);
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
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
	XSetSelectionOwner(dpy, atom, xscr->laywin, CurrentTime);
	XSync(dpy, False);

	if (xscr->laywin) {
		XEvent ev;

		ev.xclient.type = ClientMessage;
		ev.xclient.serial = 0;
		ev.xclient.send_event = False;
		ev.xclient.display = dpy;
		ev.xclient.window = root;
		ev.xclient.message_type = XInternAtom(dpy, "MANAGER", False);
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = CurrentTime;
		ev.xclient.data.l[1] = atom;
		ev.xclient.data.l[2] = xscr->laywin;
		ev.xclient.data.l[3] = 0;
		ev.xclient.data.l[4] = 0;

		XSendEvent(dpy, root, False, StructureNotifyMask, &ev);
		XFlush(dpy);
	}
	return (owner);
}
#endif				/* NEED_DESKTOP_LAYOUT */

static Window
get_selection(Bool replace, Window selwin)
{
	char selection[64] = { 0, };
	GdkDisplay *disp;
	Display *dpy;
	int s, nscr;
	Window owner;
	Atom atom;
	Window gotone = None;

	PTRACE(5);
	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);

	dpy = GDK_DISPLAY_XDISPLAY(disp);

	for (s = 0; s < nscr; s++) {
		snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
		atom = XInternAtom(dpy, selection, False);
		if (!(owner = XGetSelectionOwner(dpy, atom)))
			DPRINTF(1, "No owner for %s\n", selection);
		if ((owner && replace) || (!owner && selwin)) {
			DPRINTF(1, "Setting owner of %s to 0x%08lx from 0x%08lx\n", selection, selwin, owner);
			XSetSelectionOwner(dpy, atom, selwin, CurrentTime);
			XSync(dpy, False);
		}
		if (!gotone && owner)
			gotone = owner;
	}
	if (replace) {
		if (gotone) {
			if (selwin)
				DPRINTF(1, "%s: replacing running instance\n", NAME);
			else
				DPRINTF(1, "%s: quitting running instance\n", NAME);
		} else {
			if (selwin)
				DPRINTF(1, "%s: no running instance to replace\n", NAME);
			else
				DPRINTF(1, "%s: no running instance to quit\n", NAME);
		}
		if (selwin) {
			XEvent ev = { 0, };
			Atom manager = XInternAtom(dpy, "MANAGER", False);
			GdkScreen *scrn;
			Window root;

			for (s = 0; s < nscr; s++) {
				scrn = gdk_display_get_screen(disp, s);
				root = GDK_WINDOW_XID(gdk_screen_get_root_window(scrn));
				snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
				atom = XInternAtom(dpy, selection, False);

				ev.xclient.type = ClientMessage;
				ev.xclient.serial = 0;
				ev.xclient.send_event = False;
				ev.xclient.display = dpy;
				ev.xclient.window = root;
				ev.xclient.message_type = manager;
				ev.xclient.format = 32;
				ev.xclient.data.l[0] = CurrentTime;	/* FIXME:
									   mimestamp */
				ev.xclient.data.l[1] = atom;
				ev.xclient.data.l[2] = selwin;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;

				XSendEvent(dpy, root, False, StructureNotifyMask, &ev);
				XFlush(dpy);
			}
		}
	} else if (gotone)
		DPRINTF(1, "%s: not replacing running instance\n", NAME);
	return (gotone);
}

static void
do_run(int argc, char *argv[], Bool replace)
{
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	GdkWindow *root = gdk_screen_get_root_window(scrn), *sel;
	char selection[65] = { 0, };
	Window selwin, owner, broadcast = GDK_WINDOW_XID(root);
	XdeScreen *xscr;
	int s, nscr;

	PTRACE(5);
	selwin = XCreateSimpleWindow(dpy, broadcast, 0, 0, 1, 1, 0, 0, 0);

	if ((owner = get_selection(replace, selwin))) {
		if (!replace) {
			XDestroyWindow(dpy, selwin);
			if (!options.editor && !options.trayicon) {
				EPRINTF("%s: instance already running\n", NAME);
				exit(EXIT_FAILURE);
			}
			scrn = gdk_display_get_screen(disp, options.screen);
			root = gdk_screen_get_root_window(scrn);
			broadcast = GDK_WINDOW_XID(root);
			if (options.editor) {
				XEvent ev = { 0, };

				DPRINTF(1, "instance running: asking it to launch editor\n");
				ev.xclient.type = ClientMessage;
				ev.xclient.serial = 0;
				ev.xclient.send_event = False;
				ev.xclient.display = dpy;
				ev.xclient.window = broadcast;
				ev.xclient.message_type = _XA_PREFIX_EDIT;
				ev.xclient.format = 32;
				ev.xclient.data.l[0] = CurrentTime;
				ev.xclient.data.l[1] = owner;
				ev.xclient.data.l[2] = 0;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;
				XSendEvent(dpy, broadcast, False, NoEventMask, &ev);
				XSync(dpy, False);
			}
			if (options.trayicon) {
				XEvent ev = { 0, };

				DPRINTF(1, "instance running: asking it to install trayicon\n");
				ev.xclient.type = ClientMessage;
				ev.xclient.serial = 0;
				ev.xclient.send_event = False;
				ev.xclient.display = dpy;
				ev.xclient.window = broadcast;
				ev.xclient.message_type = _XA_PREFIX_TRAY;
				ev.xclient.format = 32;
				ev.xclient.data.l[0] = CurrentTime;
				ev.xclient.data.l[1] = owner;
				ev.xclient.data.l[2] = 0;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;
				XSendEvent(dpy, broadcast, False, NoEventMask, &ev);
				XSync(dpy, False);
			}
			exit(EXIT_SUCCESS);
		}
	}
	XSelectInput(dpy, selwin, StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask);

	oldhandler = XSetErrorHandler(handler);
	oldiohandler = XSetIOErrorHandler(iohandler);

	nscr = gdk_display_get_n_screens(disp);
	screens = calloc(nscr, sizeof(*screens));

	sel = gdk_x11_window_foreign_new_for_display(disp, selwin);
	gdk_window_add_filter(sel, selwin_handler, screens);
	gdk_window_add_filter(NULL, events_handler, NULL);

	for (s = 0, xscr = screens; s < nscr; s++, xscr++) {
		snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
		xscr->index = s;
		xscr->atom = XInternAtom(dpy, selection, False);
		xscr->scrn = gdk_display_get_screen(disp, s);
		xscr->root = gdk_screen_get_root_window(xscr->scrn);
		xscr->selwin = selwin;
		xscr->width = gdk_screen_get_width(xscr->scrn);
		xscr->height = gdk_screen_get_height(xscr->scrn);
#if NEED_STARTUP_IDS
#ifdef STARTUP_NOTIFICATION
		xscr->ctx = sn_monitor_context_new(sn_dpy, s, &sn_handler, xscr, NULL);
#endif
#endif
		gdk_window_add_filter(xscr->root, root_handler, xscr);
		init_wnck(xscr);
		init_monitors(xscr);
#if NEED_DESKTOP_INFO
		if (options.proxy)
			setup_button_proxy(xscr);
#endif				/* NEED_DESKTOP_INFO */
#if NEED_ROOT_PIXMAPS
		update_root_pixmap(xscr, None);
#endif				/* NEED_ROOT_PIXMAPS */
#if NEED_DESKTOP_LAYOUT
		update_layout(xscr, None);
#endif				/* NEED_DESKTOP_LAYOUT */
#if NEED_CURRENT_DESKTOP
		update_current_desktop(xscr, None);
#endif				/* NEED_CURRENT_DESKTOP */
		update_theme(xscr, None);
		update_window(xscr, None);
		update_client_list(xscr, None);
	}
	g_unix_signal_add(SIGTERM, &term_signal_handler, NULL);
	g_unix_signal_add(SIGINT, &int_signal_handler, NULL);
	g_unix_signal_add(SIGHUP, &hup_signal_handler, NULL);
	if (options.trayicon) {
		xscr = screens + options.screen;
		show_stray(xscr);
	}
	if (options.editor) {
		xscr = screens + options.screen;
		edit_input(xscr);
	}
	gtk_main();
}

/** @brief Ask a running instance to quit.
  *
  * This is performed by checking for an owner of the selection and clearing the
  * selection if it exists.
  */
static void
do_quit(int argc, char *argv[])
{
	PTRACE(5);
	get_selection(True, None);
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
    %1$s [options]\n\
    %1$s {-q|--quit} [options]\n\
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

const char *
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

const char *
show_screen(int snum)
{
	static char screen[64] = { 0, };

	if (options.screen == -1)
		return ("None");
	snprintf(screen, sizeof(screen), "%d", options.screen);
	return (screen);
}

const char *
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

const char *
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
    %1$s [options]\n\
    %1$s {-q|--quit} [options]\n\
    %1$s {-h|--help} [options]\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
Command options:\n\
    -q, --quit\n\
        ask a running instance to quit\n\
    -h, --help, -?, --?\n\
        print this usage information and exit\n\
    -V, --version\n\
        print version and exit\n\
    -C, --copying\n\
        print copying permission and exit\n\
Options:\n\
    -r, --replace\n\
        replace a running instance [default: %15$s]\n\
    -e, --editor\n\
        launch the settings editor [default: %22$s]\n\
    -d, --display DISPLAY\n\
        specify the X display, DISPLAY, to use [default: %4$s]\n\
    -s, --screen SCREEN\n\
        specify the screen number, SCREEN, to use [default: %5$s]\n\
    -M, --monitor MONITOR\n\
        specify the monitor numer, MONITOR, to use [default: %16$d]\n\
    -f, --filename FILENAME\n\
        use the file, FILENAME, for configuration [default: %8$s]\n\
    -t, --timeout MILLISECONDS\n\
        specify timeout when not modifier [default: %6$lu]\n\
    -B, --border PIXELS\n\
        border surrounding feedback popup [default: %7$d]\n\
    -K, --keyboard\n\
        indicate that the keyboard was used to launch [default: %17$s]\n\
    -P, --pointer\n\
        indicate that the pointer was used to launch [default: %18$s]\n\
    -b, --button BUTTON\n\
        specify the mouse button number, BUTTON, for popup [default: %9$d]\n\
    -w, --which {active|focused|pointer|SCREEN}\n\
        specify the screen for which to pop the menu [default: %10$s]\n\
        \"active\"   - the screen with EWMH/NetWM active client\n\
        \"focused\"  - the screen with EWMH/NetWM focused client\n\
        \"pointer\"  - the screen with EWMH/NetWM pointer\n\
        \"SCREEN\"   - the specified screen number\n\
    -W, --where {pointer|center|topleft|GEOMETRY}\n\
        specify where to place the menu [default: %11$s]\n\
        \"pointer\"  - northwest corner under the pointer\n\
        \"center\"   - center of associated monitor\n\
        \"topleft\"  - northwest corner of work area\n\
        GEOMETRY   - postion on screen as X geometry\n\
    -O, --order {client|stacking}\n\
        specify the order of windows [default: %12$s]\n\
    -T, --timestamp TIMESTAMP\n\
        use the time, TIMESTAMP, for button/keyboard event [default: %19$lu]\n\
    -k, --keys FORWARD:REVERSE\n\
        specify keys for cycling [default: %20$s]\n\
    -p, --proxy\n\
        respond to button proxy [default: %21$s]\n\
    -n, --dry-run\n\
        do not change anything, just report actions [default: %23$s]\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %2$d]\n\
        this option may be repeated.\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %3$d]\n\
        this option may be repeated.\n\
Session Management:\n\
    -clientID CLIENTID\n\
        client id for session management [default: %13$s]\n\
    -restore SAVEFILE\n\
        file in which to save session info [default: %14$s]\n\
", argv[0]
	, options.debug
	, options.output
	, options.display
	, show_screen(options.screen)
	, options.timeout
	, options.border
	, options.filename
	, options.button
	, show_which(options.which)
	, show_where(options.where)
	, show_order(options.order)
	, options.clientId
	, options.saveFile
	, show_bool(options.replace)
	, options.monitor
	, show_bool(options.keyboard)
	, show_bool(options.pointer)
	, options.timestamp
	, options.keys ?: "AS+Tab:A+Tab"
	, show_bool(options.proxy)
	, show_bool(options.editor)
	, show_bool(options.dryrun)
);
	/* *INDENT-ON* */
}

static void
set_defaults(void)
{
	const char *env, *p;
	char *file;
	int n;

	if ((env = getenv("DISPLAY"))) {
		options.display = strdup(env);
		if (options.screen < 0 && (p = strrchr(options.display, '.'))
		    && (n = strspn(++p, "0123456789")) && *(p + n) == '\0')
			options.screen = atoi(p);
	}
	if ((p = getenv("XDE_DEBUG")))
		options.debug = atoi(p);
	file = g_build_filename(g_get_home_dir(), ".config", "xde", "input.ini", NULL);
	options.filename = g_strdup(file);
	g_free(file);
}

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
	if (options.screen < 0 && (options.editor || options.trayicon)) {
		if (options.button)
			options.screen = find_pointer_screen();
		else
			options.screen = find_focus_screen();
	}
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

			{"filename",		required_argument,	NULL,	'f'},
			{"timeout",		required_argument,	NULL,	't'},
			{"border",		required_argument,	NULL,	'B'},
			{"pointer",		no_argument,		NULL,	'P'},
			{"keyboard",		no_argument,		NULL,	'K'},
			{"button",		required_argument,	NULL,	'b'},
			{"which",		required_argument,	NULL,	'w'},
			{"where",		required_argument,	NULL,	'W'},
			{"order",		optional_argument,	NULL,	'O'},

			{"proxy",		no_argument,		NULL,	'p'},
			{"timestamp",		required_argument,	NULL,	'T'},
			{"key",			optional_argument,	NULL,	'k'},


			{"trayicon",		no_argument,		NULL,	'y'},
			{"editor",		no_argument,		NULL,	'e'},

			{"quit",		no_argument,		NULL,	'q'},
			{"replace",		no_argument,		NULL,	'r'},

			{"clientId",		required_argument,	NULL,	'8'},
			{"restore",		required_argument,	NULL,	'9'},

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

		c = getopt_long_only(argc, argv, "d:s:m:f:t:B:b:PKw:W:O::yeqrD::v::hVCH?",
				     long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:m:f:t:B:b:PKw:W:O:yeqr89D:vhVCH?");
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

		case 'f':	/* -f, --filename FILENAME */
			free(options.filename);
			options.filename = strdup(optarg);
			break;
		case 't':	/* -t, --timeout MILLISECONDS */
			val = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			options.timeout = val;
			break;
		case 'B':	/* -B, --border PIXELS */
			val = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			if (val < 0 || val > 20)
				goto bad_option;
			options.border = val;
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
		case 'p':	/* -p, --proxy */
			options.proxy = True;
			break;

		case 'y':	/* -y, --trayicon */
			options.trayicon = True;
			break;
		case 'e':	/* -e, --editor */
			options.editor = True;
			break;

		case 'q':	/* -q, --quit */
			if (options.command != CommandDefault)
				goto bad_command;
			if (command == CommandDefault)
				command = CommandQuit;
			options.command = CommandQuit;
			break;
		case 'r':	/* -r, --replace */
			options.replace = True;
			break;

		case '8':	/* -clientId CLIENTID */
			free(options.clientId);
			options.clientId = strdup(optarg);
			break;
		case '9':	/* -restore SAVEFILE */
			free(options.saveFile);
			options.saveFile = strdup(optarg);
			break;

		case 'N':	/* -N, --dry-run */
			options.dryrun = True;
			break;
		case 'D':	/* -D, --debug [LEVEL] */
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
		case 'v':	/* -v, --verbose [LEVEL] */
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
		case 'h':	/* -h, --help */
		case 'H':	/* -H, --? */
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
		DPRINTF(1, "%s: running a %s instance\n", argv[0], options.replace ? "replacement" : "new");
		startup(argc, argv);
		do_run(argc, argv, options.replace);
		break;
	case CommandQuit:
		DPRINTF(1, "%s: asking existing instance to quit\n", argv[0]);
		startup(argc, argv);
		do_quit(argc, argv);
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

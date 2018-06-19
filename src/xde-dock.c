/*****************************************************************************

 Copyright (c) 2010-2017  Monavacon Limited <http://www.monavacon.com/>
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

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1
#define EXIT_SYNTAXERR	2

#define XA_SELECTION_NAME	    "_XDE_DOCK_S%d"

static int saveArgc;
static char **saveArgv;

Atom _XA_XDE_THEME_NAME;
Atom _XA_GTK_READ_RCFILES;
Atom _XA_NET_CLIENT_LIST;
Atom _XA_WIN_CLIENT_LIST;

const char *program = NAME;

Bool shut_it_down = False;

typedef enum {
	DockPositionEast,
	DockPositionNorthEast,
	DockPositionNorth,
	DockPositionNorthWest,
	DockPositionWest,
	DockPositionSouthWest,
	DockPositionSouth,
	DockPositionSouthEast,
} DockPosition;

typedef enum {
	DockDirectionVertical,
	DockDirectionHorizontal,
} DockDirection;

typedef struct {
	int index;			/* monitor number */
	int current;			/* current desktop for this monitor */
	GdkRectangle geom;		/* geometry of the monitor */
} XdeMonitor;

typedef struct {
	int index;			/* index */
	GdkDisplay *disp;
	GdkScreen *scrn;
	GdkWindow *root;
	WnckScreen *wnck;
	gint nmon;			/* number of monitors */
	XdeMonitor *mons;		/* monitors for this screen */
	Bool mhaware;			/* multi-head aware NetWM */
	Pixmap pixmap;			/* current pixmap for the entire screen */
	char *theme;			/* XDE theme name */
	GKeyFile *entry;		/* XDE theme file entry */
	Window selwin;			/* selection owner window */
	Atom atom;			/* selection atom for this screen */
	int width, height;
	char *wmname;			/* window manager name (adjusted) */
	Bool hasdock;			/* windowm manager has own dock? */
	Bool goodwm;			/* is the window manager usable? */
	Window save;
	GList *docks;			/* docks for this screen */
	GList *clients;			/* clients of this screen */
} XdeScreen;

XdeScreen *screens;			/* array of screens */

typedef struct {
	Window save;
	GtkWidget *dock;
	GtkWidget *vbox;
	GdkWindow *gwin;
	Window dwin;
	gint x, y, w, h, d;
	DockPosition position;
	DockDirection direction;
	XdeScreen *xscr;		/* screen for this dock */
	int napps;			/* number of clients */
	GList *clients;			/* clients of this dock */
} XdeDock;

#define XDE_DOCK_DONT_WAIT 1

typedef struct {
	Window window;
	Bool withdrawing;
	Bool mapped;
	Bool reparented;
	Window root;
	Window parent;
	Bool needsremap;
} XdeWindow;

typedef struct {
	XdeWindow wind, icon;
	Bool swallowed;
	XClassHint ch;
	char **argv;
	int argc;
	GtkWidget *ebox;
	GtkWidget *sock;
	int retries;
	Window plugged;
	XdeDock *dock;			/* dock for this client */
} XdeClient;

XContext ClientContext;

typedef enum {
	DockAppTestMine,
	DockAppTestFlipse,
	DockAppTestBlackbox,
	DockAppTestFluxbox,
	DockAppTestOpenbox,
} DockAppTest;

typedef enum {
	CommandDefault,
	CommandRun,
	CommandQuit,
	CommandReplace,
	CommandHelp,
	CommandVersion,
	CommandCopying,
} Command;

typedef struct {
	int debug;
	int output;
	char *display;
	int screen;
	Time timestamp;
	DockPosition position;
	DockDirection direction;
	DockAppTest test;
	Command command;
	char *clientId;
	char *saveFile;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.timestamp = CurrentTime,
	.position = DockPositionEast,
	.direction = DockDirectionVertical,
	.test = DockAppTestMine,
	.command = CommandDefault,
	.clientId = NULL,
	.saveFile = NULL,
};

static void
copying(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	(void) fprintf(stdout, "\
--------------------------------------------------------------------------------\n\
%1$s\n\
--------------------------------------------------------------------------------\n\
Copyright (c) 2010-2017  Monavacon Limited <http://www.monavacon.com/>\n\
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
Copyright (c) 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017  Monavacon Limited.\n\
Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2010  OpenSS7 Corporation.\n\
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
    %1$s {-r|--replace} [options]\n\
    %1$s {-q|--quit} [options]\n\
    %1$s {-h|--help}\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
", argv[0]);
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
    %1$s {-r|--replace} [options]\n\
    %1$s {-q|--quit} [options]\n\
    %1$s {-h|--help}\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
Command options:\n\
    -r, --replace\n\
        replace a running instance\n\
    -q, --quit\n\
        ask a running instance to quit\n\
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
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %2$d]\n\
        this option may be repeated.\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %3$d]\n\
        this option may be repeated.\n\
Session Management:\n\
    -clientID CLIENTID\n\
        client id for session management [default: %6$s]\n\
    -restore SAVEFILE\n\
        file in which to save session info [default: %7$s]\n\
", argv[0]
	, options.debug
	, options.output
	, options.display
	, options.screen
	, options.clientId
	, options.saveFile
);
	/* *INDENT-ON* */
}

void
relax(void)
{
//      XSync(dpy, False);
	while (gtk_events_pending())
		if (gtk_main_iteration())
			break;
}

void
dock_rearrange(XdeDock *dock)
{
	XdeScreen *xscr = dock->xscr;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	DockPosition pos;
	DockDirection dir;
	struct {
		long left, right, top, bottom;
		long left_start_y, left_end_y;
		long right_start_y, right_end_y;
		long top_start_x, top_end_x;
		long bottom_start_x, bottom_end_x;
	} strut = {
	0,};

	dock->w = xscr->width;
	dock->h = xscr->height;
	dock->d = 0;

	if (options.debug)
		fprintf(stderr, "==> REARRANGING DOCK:\n");
	if (dock->napps <= 0) {
		if (options.debug)
			fprintf(stderr, "    --> HIDING DOCK: no dock apps\n");
		gtk_widget_hide(GTK_WIDGET(dock->dock));
		return;
	}
	pos = options.position;
	dir = options.direction;
	dock->x = dock->y = 0;
	switch (pos) {
	case DockPositionNorth:
		switch (dir) {
		case DockDirectionVertical:
			strut.top = dock->napps * 64;
			strut.top_start_x = (dock->w - 64) / 2;
			strut.top_end_x = (dock->w + 64) / 2;
			break;
		case DockDirectionHorizontal:
			strut.top = 64;
			strut.top_start_x = (dock->w - (dock->napps * 64)) / 2;
			strut.top_end_x = (dock->w + (dock->napps * 64)) / 2;
			break;
		}
		dock->x = strut.top_start_x;
		dock->y = 0;
		break;
	case DockPositionNorthWest:
		switch (dir) {
		case DockDirectionVertical:
			strut.left = 64;
			strut.left_start_y = 0;
			strut.left_end_y = dock->napps * 64;
			break;
		case DockDirectionHorizontal:
			strut.top = 64;
			strut.top_start_x = 0;
			strut.top_end_x = dock->napps * 64;
			break;
		}
		dock->x = 0;
		dock->y = 0;
		break;
	case DockPositionWest:
		switch (dir) {
		case DockDirectionVertical:
			strut.left = 64;
			strut.left_start_y = (dock->h - (dock->napps * 64)) / 2;
			strut.left_end_y = (dock->h + (dock->napps * 64)) / 2;
			break;
		case DockDirectionHorizontal:
			strut.left = dock->napps * 64;
			strut.left_start_y = (dock->h - 64) / 2;
			strut.left_end_y = (dock->h + 64) / 2;
			break;
		}
		dock->x = 0;
		dock->y = strut.left_start_y;
		break;
	case DockPositionSouthWest:
		switch (dir) {
		case DockDirectionVertical:
			strut.left = 64;
			strut.left_start_y = dock->h - dock->napps * 64;
			strut.left_end_y = dock->h;
			dock->y = strut.left_start_y;
			break;
		case DockDirectionHorizontal:
			strut.bottom = 64;
			strut.bottom_start_x = 0;
			strut.bottom_end_x = dock->napps * 64;
			dock->y = dock->h - strut.bottom;
			break;
		}
		dock->x = 0;
		break;
	case DockPositionSouth:
		switch (dir) {
		case DockDirectionVertical:
			strut.bottom = dock->napps * 64;
			strut.bottom_start_x = (dock->h - 64) / 2;
			strut.bottom_end_x = (dock->h + 64) / 2;
			break;
		case DockDirectionHorizontal:
			strut.bottom = 64;
			strut.bottom_start_x = (dock->h - (dock->napps * 64)) / 2;
			strut.bottom_end_x = (dock->h + (dock->napps * 64)) / 2;
			break;
		}
		dock->x = strut.bottom_start_x;
		dock->y = dock->h - strut.bottom;
		break;
	case DockPositionSouthEast:
		switch (dir) {
		case DockDirectionVertical:
			strut.right = 64;
			strut.right_start_y = dock->h - dock->napps * 64;
			strut.right_end_y = dock->h;
			dock->x = dock->w - strut.right;
			dock->y = strut.right_start_y;
			break;
		case DockDirectionHorizontal:
			strut.bottom = 64;
			strut.bottom_start_x = dock->w - dock->napps * 64;
			strut.bottom_end_x = dock->w;
			dock->x = strut.bottom_start_x;
			dock->y = dock->h = strut.bottom;
			break;
		}
		break;
	case DockPositionEast:
		switch (dir) {
		case DockDirectionVertical:
			strut.right = 64;
			strut.right_start_y = (dock->h - (dock->napps * 64)) / 2;
			strut.right_end_y = (dock->h + (dock->napps * 64)) / 2;
			break;
		case DockDirectionHorizontal:
			strut.right = dock->napps * 64;
			strut.right_start_y = (dock->h - 64) / 2;
			strut.right_end_y = (dock->h + 64) / 2;
			break;
		}
		dock->x = dock->w - strut.right;
		dock->y = strut.right_start_y;
		break;
	case DockPositionNorthEast:
		switch (dir) {
		case DockDirectionVertical:
			strut.right = 64;
			strut.right_start_y = 0;
			strut.right_end_y = dock->napps * 64;
			dock->x = dock->w - strut.right;
			dock->y = strut.right_start_y;
			break;
		case DockDirectionHorizontal:
			strut.top = 64;
			strut.top_start_x = dock->w - (dock->napps * 64);
			strut.top_end_x = dock->w;
			dock->x = strut.top_start_x;
			dock->y = 0;
			break;
		}
		break;
	}
	if (options.debug)
		fprintf(stderr, "    --> MOVING dock to (%d,%d)\n", dock->x, dock->y);

	gtk_window_move(GTK_WINDOW(dock->dock), dock->x, dock->y);
	gdk_window_move(GDK_WINDOW(dock->gwin), dock->x, dock->y);
	relax();
	gdk_window_get_geometry(GDK_WINDOW(dock->gwin), &dock->x, &dock->y, &dock->w, &dock->h,
				&dock->d);
	if (options.debug) {
		fprintf(stderr, "    --> GEOMETRY now (%dx%d+%d+%d:%d)\n", dock->w, dock->h,
			dock->x, dock->y, dock->d);
		fprintf(stderr, "    --> _NET_WM_STRUT set (%ld,%ld,%ld,%ld)\n", strut.left,
			strut.right, strut.top, strut.bottom);
		fprintf(stderr,
			"    --> _NET_WM_STRUT_PARTIAL set (%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld)\n",
			strut.left, strut.right, strut.top, strut.bottom, strut.left_start_y,
			strut.left_end_y, strut.right_start_y, strut.right_end_y, strut.top_start_x,
			strut.top_end_x, strut.bottom_start_x, strut.bottom_end_x);
	}
	XChangeProperty(dpy, dock->dwin, XInternAtom(dpy, "_NET_WM_STRUT", False), XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *) &strut, 4);
	XChangeProperty(dpy, dock->dwin, XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False),
			XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &strut, 12);
	relax();
}

void add_dockapp(XdeDock *d, XdeClient *c, XdeWindow *cwin);

gboolean
on_plug_removed(GtkSocket * s, gpointer data)
{
	XdeClient *c = data;
	XdeDock *d = c->dock;

	if (options.debug)
		fprintf(stderr, "*** PLUG REMOVED: wind.window = 0x%08lx icon.window = 0x%08lx\n",
			c->wind.window, c->icon.window);
	if (c->ebox) {
		gtk_widget_destroy(GTK_WIDGET(c->ebox));
		c->ebox = NULL;
		c->swallowed = False;
		c->dock = NULL;
		d->clients = g_list_remove(d->clients, c);
		d->napps -= 1;
		if (c->plugged && c->retries++ < 5) {
			if (c->plugged == c->wind.window) {
				add_dockapp(d, c, &c->wind);
			} else {
				add_dockapp(d, c, &c->icon);
			}
		} else {
			c->plugged = None;
			c->retries = 0;
			dock_rearrange(d);
		}
	}
	return FALSE;
}

void
swallow(XdeScreen *xscr, XdeClient *c)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	XdeDock *d = xscr->docks->data;
	Window wind = c->wind.window;
	Window icon = c->icon.window;
	XdeWindow *cwin;

	if (c->swallowed) {
		if (options.debug)
			fprintf(stderr,
				"==> NOT SWALLOWING: wind.window 0x%08lx icon.window 0x%08lx\n",
				wind, icon);
		return;
	}
	if (options.debug)
		fprintf(stderr, "==> SWALLOWING window 0x%08lx with icon 0x%08lx!\n", wind, icon);
	relax();
	if (XGetClassHint(dpy, wind, &c->ch)) {
		if (options.debug)
			fprintf(stderr, "    --> window 0x%08lx '%s', '%s'\n", wind, c->ch.res_name,
				c->ch.res_class);
	}
	if (XGetCommand(dpy, c->wind.window, &c->argv, &c->argc)) {
		if (options.debug) {
			int i;

			fprintf(stderr, "    --> window 0x%08lx '", wind);
			for (i = 0; i < c->argc; i++)
				fprintf(stderr, "%s%s", c->argv[i],
					(c->argc == i + 1) ? "" : "', '");

			fprintf(stderr, "%s\n", "'");
		}
	}
	{
		Window *children;
		unsigned int i, nchild;

		if (wind
		    && XQueryTree(dpy, wind, &c->wind.root, &c->wind.parent, &children, &nchild)) {
			if (options.debug) {
				fprintf(stderr, "    --> wind.window 0x%08lx has root 0x%08lx\n",
					wind, c->wind.root);
				fprintf(stderr, "    --> wind.window 0x%08lx has parent 0x%08lx\n",
					wind, c->wind.parent);
				for (i = 0; i < nchild; i++)
					fprintf(stderr,
						"    --> wind.window 0x%08lx has child 0x%08lx\n",
						wind, children[i]);
			}
			if (children)
				XFree(children);
		}
		if (icon
		    && XQueryTree(dpy, icon, &c->icon.root, &c->icon.parent, &children, &nchild)) {
			if (options.debug) {
				fprintf(stderr, "    --> icon.window 0x%08lx has root 0x%08lx\n",
					icon, c->icon.root);
				fprintf(stderr, "    --> icon.window 0x%08lx has parent 0x%08lx\n",
					icon, c->icon.parent);
				for (i = 0; i < nchild; i++)
					fprintf(stderr,
						"    --> icon.window 0x%08lx has child 0x%08lx\n",
						icon, children[i]);
			}
			if (children)
				XFree(children);
		}
	}
	if (!c->icon.window || (c->icon.parent == c->wind.window)) {
		if (options.debug)
			fprintf(stderr, "    --> REPARENTING: wind.window 0x%08lx to socket\n",
				c->wind.window);
		cwin = &c->wind;
	} else {
#ifdef XDE_DOCK_DONT_WAIT
		XUnmapWindow(dpy, c->wind.window);
		relax();
		relax();
#else
		if (options.debug)
			fprintf(stderr,
				"    --> REPARENTING: wind.window 0x%08lx to save 0x%08lx\n",
				c->wind.window, dock->save);
		XAddToSaveSet(dpy, c->wind.window);
		XReparentWindow(dpy, c->wind.window, dock->save, 0, 0);
		XMapWindow(dpy, c->wind.window);
#endif
		if (options.debug)
			fprintf(stderr, "    --> REPARENTING: icon.window 0x%08lx to socket\n",
				c->icon.window);
		cwin = &c->icon;
	}

	cwin->mapped = True;

	/* Now we can actually swallow the dock app. */

	add_dockapp(d, c, cwin);
}

void
add_dockapp(XdeDock *dock, XdeClient *c, XdeWindow *cwin)
{
	XdeScreen *xscr = dock->xscr;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	int tpad, bpad, lpad, rpad;
	int x, y;
	unsigned int width, height, border, depth;

	if (!XGetGeometry(dpy, cwin->window, &cwin->root, &x, &y, &width, &height, &border, &depth)) {
		fprintf(stderr, "ERROR: window 0x%08lx cannot get geometry\n", cwin->window);
		return;
	}
	if (options.debug) {
		fprintf(stderr, "==> DOCKAPP: geometry %dx%d+%d+%d:%d\n", width, height, x, y,
			border);
	}

	if (height < 64) {
		tpad = (64 - height) / 2;
		bpad = 64 - height - tpad;
	} else {
		tpad = bpad = 0;
	}
	if (width < 64) {
		lpad = (64 - width) / 2;
		rpad = 64 - width - lpad;
	} else {
		lpad = rpad = 0;
	}

	GtkWidget *b, *a, *s;

#if 1
	b = c->ebox = gtk_event_box_new();
	gtk_container_set_border_width(GTK_CONTAINER(b), 0);
	gtk_event_box_set_above_child(GTK_EVENT_BOX(b), TRUE);
#else
	b = c->ebox = gtk_button_new();
	gtk_container_set_border_width(GTK_CONTAINER(b), 0);
	gtk_button_set_relief(GTK_BUTTON(b), GTK_RELIEF_NONE);
	gtk_button_set_alignment(GTK_BUTTON(b), 0.5, 0.5);
#endif
	gtk_widget_set_size_request(GTK_WIDGET(b), 64, 64);

	a = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_widget_set_size_request(GTK_WIDGET(a), width, height);
	gtk_container_set_border_width(GTK_CONTAINER(a), 0);
	gtk_widget_set_size_request(GTK_WIDGET(a), width, height);
#if 0
	gtk_alignment_set_padding(GTK_ALIGNMENT(a), tpad, bpad, lpad, rpad);
#else
	gtk_alignment_set_padding(GTK_ALIGNMENT(a), 0, 0, 0, 0);
#endif
	gtk_container_add(GTK_CONTAINER(b), GTK_WIDGET(a));

	s = gtk_socket_new();
	gtk_container_set_border_width(GTK_CONTAINER(s), 0);
	g_signal_connect(G_OBJECT(s), "plug_removed", G_CALLBACK(on_plug_removed), (gpointer) c);
	gtk_container_add(GTK_CONTAINER(a), GTK_WIDGET(s));

	if (options.debug)
		fprintf(stderr, "    --> PACKING SOCKET for window 0x%08lx into dock\n",
			cwin->window);
	gtk_box_pack_start(GTK_BOX(dock->vbox), GTK_WIDGET(b), FALSE, FALSE, 0);
	dock->clients = g_list_append(dock->clients, c);
	dock->napps += 1;
	relax();
	dock_rearrange(dock);
	gtk_widget_show_all(GTK_WIDGET(b));
	gtk_widget_show_all(GTK_WIDGET(a));
	gtk_widget_show_all(GTK_WIDGET(s));
	gtk_widget_show_all(GTK_WIDGET(dock->dock));
	if (options.debug)
		fprintf(stderr, "    --> ADDING window 0x%08lx into socket\n", cwin->window);
	/* We might also use gtk_socket_steal() here.  I don't quite know what the
	   difference is as both appear to behave the same; however, there is no way to
	   add the window to Gtk2's save set, so we need to figure out how to do that.
	   Perhaps stealing has this effect. */

	cwin->reparented = True;

	c->plugged = cwin->window;
	c->swallowed = True;
	c->dock = dock;

	relax();
	relax();
	if (0) {
		gtk_socket_steal(GTK_SOCKET(s), c->plugged);
	} else {
		gtk_socket_add_id(GTK_SOCKET(s), c->plugged);
	}

	relax();
}

Bool
withdraw_window(XdeScreen *xscr, XdeWindow *cwin)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window *children = NULL;
	unsigned int nchild = 0;
	Bool retval = False;

	if (options.debug)
		fprintf(stderr, "==> WITHDRAWING: window 0x%08lx\n", cwin->window);
	if (XQueryTree(dpy, cwin->window, &cwin->root, &cwin->parent, &children, &nchild)) {
		if (options.debug)
			fprintf(stderr, "    --> window 0x%08lx root 0x%08lx parent 0x%08lx\n",
				cwin->window, cwin->root, cwin->parent);
		if (cwin->root && cwin->parent && (cwin->root != cwin->parent)) {
			if (options.debug)
				fprintf(stderr,
					"    --> DEPARENTING: window 0x%08lx from parent 0x%08lx\n",
					cwin->window, cwin->parent);
			cwin->withdrawing = True;
			XUnmapWindow(dpy, cwin->window);
			XWithdrawWindow(dpy, cwin->window, xscr->index);
			relax();
			retval = True;
		}
		if (children)
			XFree(children);
	} else {
		fprintf(stderr, "!!! ERROR: could not query tree for window 0x%08lx\n",
			cwin->window);
	}
	return retval;
}

void
withdraw_client(XdeScreen *xscr, XdeClient *c)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);

	if (options.debug)
		fprintf(stderr, "==> WITHDRAWING: window 0x%08lx\n", c->wind.window);
	if (c->icon.window)
		XUnmapWindow(dpy, c->icon.window);
	XUnmapWindow(dpy, c->wind.window);
	XWithdrawWindow(dpy, c->wind.window, xscr->index);
	if (1) {
		if (c->icon.window && c->icon.parent != c->wind.window) {
			XEvent ev;

			ev.xdestroywindow.type = DestroyNotify;
			ev.xdestroywindow.serial = 0;
			ev.xdestroywindow.send_event = False;
			ev.xdestroywindow.display = dpy;
			ev.xdestroywindow.window = c->icon.window;
			if (1) {
				ev.xdestroywindow.event = c->icon.parent;
				if (options.debug)
					fprintf(stderr,
						"    --> DestroyNotify: event=0x%08lx window=0x%08lx\n",
						c->icon.parent, c->icon.window);
				XSendEvent(dpy, c->icon.parent, False,
					   SubstructureNotifyMask | SubstructureRedirectMask, &ev);
			}
			if (0) {
				ev.xdestroywindow.event = c->icon.window;
				if (options.debug)
					fprintf(stderr,
						"    --> DestroyNotify: event=0x%08lx window=0x%08lx\n",
						c->icon.window, c->icon.window);
				XSendEvent(dpy, c->icon.window, False, StructureNotifyMask, &ev);
			}
		}
	}
	if (1) {
		/* mimic destruction for window managers that are not ICCCM 2.0 compliant
		 */
		/* do the primary window first */
		if (c->wind.window) {
			XEvent ev;

			ev.xdestroywindow.type = DestroyNotify;
			ev.xdestroywindow.serial = 0;
			ev.xdestroywindow.send_event = False;
			ev.xdestroywindow.display = dpy;
			ev.xdestroywindow.window = c->wind.window;
			if (1) {
				ev.xdestroywindow.event = c->wind.parent;
				if (options.debug)
					fprintf(stderr,
						"    --> DestroyNotify: event=0x%08lx window=0x%08lx\n",
						c->wind.parent, c->wind.window);
				XSendEvent(dpy, c->wind.parent, False,
					   SubstructureNotifyMask | SubstructureRedirectMask, &ev);
			}
			if (1) {
				ev.xdestroywindow.event = c->wind.window;
				if (options.debug)
					fprintf(stderr,
						"    --> DestroyNotify: event=0x%08lx window=0x%08lx\n",
						c->wind.window, c->wind.window);
				XSendEvent(dpy, c->wind.window, False, StructureNotifyMask, &ev);
			}
		}
	}
	relax();
}

XWMHints *
is_dockapp(XdeScreen *xscr, Window win)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, win))) {
		switch (options.test) {
		case DockAppTestMine:
			/* Many libxpm based dockapps use xlib to set the state hint
			   correctly. */
			if ((wmh->flags & StateHint) && wmh->initial_state == WithdrawnState)
				return (wmh);
			/* Many docapps that were based on GTK+ < 2.4.0 are having their
			   initial_state changed to NormalState by GTK+ >= 2.4.0, so when
			   the other flags are set, accept it anyway */
			if (wmh->initial_state == WithdrawnState
			    || (wmh->flags & ~IconPositionHint) ==
			    (WindowGroupHint | StateHint | IconWindowHint))
				return (wmh);
			/* In an attempt to get around GTK+ >= 2.4.0 limitations, some
			   GTK+ dock apps simply set the res_class to "DockApp". */
			{
				XClassHint ch = { NULL, NULL };

				if (XGetClassHint(dpy, win, &ch)) {
					if (ch.res_class) {
						if (strcmp(ch.res_class, "DockApp") == 0) {
							if (ch.res_name)
								XFree(ch.res_name);
							XFree(ch.res_class);
							return (wmh);
						}
						XFree(ch.res_class);
					}
					if (ch.res_name)
						XFree(ch.res_name);
				}
			}
			break;
		case DockAppTestBlackbox:
		case DockAppTestFluxbox:
			if ((wmh->flags & StateHint) && wmh->initial_state == WithdrawnState)
				return (wmh);
			break;
		case DockAppTestFlipse:
			/* The flipse approach: strange, but StateHint not checked first;
			   so, this translates to the following conditions: (1)
			   initial_state not set, or, (2) initial_state set to
			   WithdrawnState, or, (3), exactly group, state and icon-window
			   hints set. */
			if (wmh->initial_state == WithdrawnState
			    || wmh->flags == (WindowGroupHint | StateHint | IconWindowHint))
				return (wmh);
			break;
		case DockAppTestOpenbox:
			if ((wmh->flags & StateHint) && wmh->initial_state == WithdrawnState)
				return (wmh);
			{
				XClassHint ch = { NULL, NULL };

				if (XGetClassHint(dpy, win, &ch)) {
					if (ch.res_class) {
						if (strcmp(ch.res_class, "DockApp") == 0) {
							if (ch.res_name)
								XFree(ch.res_name);
							XFree(ch.res_class);
							return (wmh);
						}
						XFree(ch.res_class);
					}
					if (ch.res_name)
						XFree(ch.res_name);
				}
			}
			break;
		}
		XFree(wmh);
		wmh = NULL;
	}
	return (wmh);
}

static GdkFilterReturn window_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);

Bool
test_window(XdeScreen *xscr, Window wind)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	GdkWindow *win;
	Window icon;
	XWMHints *wmh;
	XdeClient *client;
	Window *children;
	unsigned int nchild;

#ifndef XDE_DOCK_DONT_WAIT
	Bool need_withdraw = False;
#endif

	if (!XFindContext(dpy, wind, ClientContext, (XPointer *) &client))
		return False;
	if (shut_it_down)
		return False;
	if (options.debug)
		fprintf(stderr, "==> TESTING WINDOW: window = 0x%08lx\n", wind);
	relax();
	if (!(wmh = is_dockapp(xscr, wind)))
		return False;
	win = gdk_x11_window_foreign_new_for_display(xscr->disp, wind);
	gdk_window_add_filter(win, window_handler, xscr);

	/* some dockapps use their main window as an icon window */
	if ((wmh->flags & IconWindowHint))
		icon = wmh->icon_window;
	else
		icon = None;
	/* some dockapp icon windows point to themselves... */
	if (icon == wind)
		icon = None;
	if (options.debug)
		fprintf(stderr, "    --> Should swallow window 0x%08lx with icon 0x%08lx!\n", wind,
			icon);

	client = calloc(1, sizeof(*client));
	xscr->clients = g_list_append(xscr->clients, client);
	client->wind.window = wind;
	client->wind.withdrawing = False;
	client->wind.mapped = False;
	XSaveContext(dpy, wind, ClientContext, (XPointer) client);
	client->icon.window = icon;
	client->icon.withdrawing = False;
	client->icon.mapped = False;
	XSaveContext(dpy, icon, ClientContext, (XPointer) client);
	client->swallowed = False;
	client->ch.res_name = NULL;
	client->ch.res_class = NULL;
	client->argv = NULL;
	client->argc = 0;

	if (wind
	    && XQueryTree(dpy, wind, &client->wind.root, &client->wind.parent, &children,
			  &nchild)) {
		if (children)
			XFree(children);
#ifndef XDE_DOCK_DONT_WAIT
		if (client->wind.parent != client->wind.root) {
			if (options.debug)
				fprintf(stderr,
					"    --> WITHDRAW REQUIRED: wind.window 0x%08lx is not toplevel (child of 0x%08lx)\n",
					client->wind.window, client->wind.parent);
			need_withdraw = True;
			client->wind.withdrawing = True;
		}
#endif
	}
	if (icon
	    && XQueryTree(dpy, icon, &client->icon.root, &client->icon.parent, &children,
			  &nchild)) {
		if (children)
			XFree(children);
#ifndef XDE_DOCK_DONT_WAIT
		if (client->icon.parent != client->icon.root) {
			if (client->icon.parent != client->wind.window) {
				if (options.debug)
					fprintf(stderr,
						"    --> WITHDRAW REQUIRED: icon.window 0x%08lx is not toplevel (child or 0x%08lx)\n",
						client->icon.window, client->icon.parent);
				need_withdraw = True;
				client->icon.withdrawing = True;
			}
		}
#endif
	}
	/* Some window managers leave the wind.window mapped as a toplevel (offscreen)
	   and just reparent the icon.window; however, it takes a withdraw request for
	   the toplevel wind.window to cause the icon.window to be reparented (and
	   unmapped). */

#ifndef XDE_DOCK_DONT_WAIT
	if (need_withdraw) {
		withdraw_client(xscr, client);
	} else
#endif
		swallow(xscr, client);

	relax();
	return True;
}

static void
update_clients(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);

	if (prop == _XA_NET_CLIENT_LIST) {
		Atom actual = None;
		int format = None;
		unsigned long items = 0, after = 0, num = 1, n;
		long *data = NULL;

	      try_harder1:
		if (XGetWindowProperty(dpy, root, prop, 0, num, False,
				       XA_WINDOW, &actual, &format, &items, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && items >= 1 && data) {
			if (after) {
				num += ((after + 1) >> 2);
				after = 0;
				XFree(data);
				data = NULL;
				goto try_harder1;
			}
			for (n = 0; n < items; n++)
				test_window(xscr, data[n]);
			XFree(data);
		}
	}
	if (prop == _XA_WIN_CLIENT_LIST) {
		Atom actual = None;
		int format = None;
		unsigned long items = 0, after = 0, num = 1, n;
		long *data = NULL;

	      try_harder2:
		if (XGetWindowProperty(dpy, root, prop, 0, num, False,
				       XA_CARDINAL, &actual, &format, &items, &after,
				       (unsigned char **) &data) == Success &&
		    format == 32 && actual && items >= 1 && data) {
			if (after) {
				num += ((after + 1) >> 2);
				after = 0;
				XFree(data);
				data = NULL;
				goto try_harder2;
			}
			for (n = 0; n < items; n++)
				test_window(xscr, data[n]);
			XFree(data);
		}
	}
}

void search_kids(XdeScreen *xscr, Window win);

void
search_window(XdeScreen *xscr, Window win)
{
	if (!test_window(xscr, win))
		 search_kids(xscr, win);
}

void
search_kids(XdeScreen *xscr, Window win)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	XdeDock *dock = xscr->docks->data;
	Window wroot = None, parent = None, *children = NULL;
	unsigned int i, nchild = 0;

	if (win == dock->dwin || win == dock->save)
		return;

	relax();
	if (XQueryTree(dpy, win, &wroot, &parent, &children, &nchild)) {
		for (i = 0; i < nchild; i++)
			search_window(xscr, children[i]);
		if (children)
			XFree(children);
	}
}

void
find_dockapps(XdeScreen *xscr)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);

	search_kids(xscr, root);
}

void reparse(void);

/** @brief Find dock app clients
  *
  * Uses the EWMH client list to search for dock applications managed by the
  * window manager.  This only works for window managers that do not have a dock
  * of their own.  That is, this might not work for fluxbox(1), blackbox(1),
  * openbox(1), pekwm(1) and wmaker(1); but, it is intended to work for
  * icewm(1), jwm(1), fvwm(1), afterstep(1), metacity(1), wmx(1), etc.
  */
void
find_dockapp_clients(XdeScreen *xscr)
{
	GList *list;

	for (list = wnck_screen_get_windows(xscr->wnck); list; list = list->next) {
		WnckWindow *win;
		Window w;

		win = list->data;
		if ((w = wnck_window_get_xid(win)))
			test_window(xscr, w);
	}
}

void
create_dock(XdeScreen *xscr)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	unsigned long black = BlackPixel(dpy, xscr->index);
	XdeDock *dock;

	dock = calloc(1, sizeof(*dock));
	xscr->docks = g_list_append(xscr->docks, dock);
	dock->xscr = xscr;
	/* FIXME: we should wait for a window manager before creating the dock */
	dock->save = XCreateSimpleWindow(dpy, root, -1, -1, 1, 1, 0, black, black);
	dock->dock = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(dock->dock), GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_window_set_default_size(GTK_WINDOW(dock->dock), 64, -1);
	gtk_window_set_decorated(GTK_WINDOW(dock->dock), FALSE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(dock->dock), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dock->dock), TRUE);
	gtk_window_stick(GTK_WINDOW(dock->dock));
	gtk_window_set_deletable(GTK_WINDOW(dock->dock), FALSE);
	gtk_window_set_focus_on_map(GTK_WINDOW(dock->dock), FALSE);
	gtk_window_set_has_frame(GTK_WINDOW(dock->dock), FALSE);
	gtk_window_set_keep_above(GTK_WINDOW(dock->dock), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(dock->dock), FALSE);

	dock->vbox = gtk_vbox_new(TRUE, 0);
	gtk_widget_set_size_request(GTK_WIDGET(dock->vbox), 64, -1);
	gtk_container_add(GTK_CONTAINER(dock->dock), GTK_WIDGET(dock->vbox));
	gtk_widget_realize(GTK_WIDGET(dock->dock));
	dock->gwin = gtk_widget_get_window(GTK_WIDGET(dock->dock));
	dock->dwin = GDK_WINDOW_XID(dock->gwin);

	find_dockapps(xscr);
}

static GdkFilterReturn
event_handler_CreateNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	if (options.debug) {
		fprintf(stderr, "==> CreateNotify:\n");
		fprintf(stderr, "    --> parent = 0x%08lx\n", xev->xcreatewindow.parent);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xcreatewindow.window);
		fprintf(stderr, "    --> x = %d\n", xev->xcreatewindow.x);
		fprintf(stderr, "    --> y = %d\n", xev->xcreatewindow.y);
		fprintf(stderr, "    --> width = %d\n", xev->xcreatewindow.width);
		fprintf(stderr, "    --> height = %d\n", xev->xcreatewindow.height);
		fprintf(stderr, "    --> border-width = %d\n", xev->xcreatewindow.border_width);
		fprintf(stderr, "    --> override-redirect = %s\n", (xev->xcreatewindow.override_redirect) ? "True" : "False");
		fprintf(stderr, "<== CreateNotify:\n");
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_DestroyNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	if (options.debug) {
		fprintf(stderr, "==> DestroyNotify:\n");
		fprintf(stderr, "    --> event = 0x%08lx\n", xev->xdestroywindow.event);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xdestroywindow.window);
		fprintf(stderr, "<== DestroyNotify:\n");
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_ReparentNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
#ifdef XDE_DOCK_DONT_WAIT
	if (options.debug) {
		fprintf(stderr, "==> ReparentNotify:\n");
		fprintf(stderr, "    --> event = 0x%08lx\n", xev->xreparent.event);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xreparent.window);
		fprintf(stderr, "    --> parent = 0x%08lx\n", xev->xreparent.parent);
		fprintf(stderr, "    --> x = %d\n", xev->xreparent.x);
		fprintf(stderr, "    --> y = %d\n", xev->xreparent.y);
		fprintf(stderr, "    --> override-redirect = %s\n",
			xev->xreparent.override_redirect ? "True" : "False");
		fprintf(stderr, "<== ReparentNotify:\n");
	}
	return GDK_FILTER_CONTINUE;
#else
	XReparentEvent *xre = &xev->xreparent;
	XdeClient *c = NULL;

	if (options.debug)
		fprintf(stderr, "==> ReparentNotify: event=0x%08lx window=0x%08lx parent=0x%08lx\n",
			xre->event, xre->window, xre->parent);
	if (xre->override_redirect) {
		if (options.debug)
			fprintf(stderr,
				"^^^ ReparentNotify: ignoring override-redirect window 0x%08lx\n",
				xre->window);
		return;
	}
	if (shut_it_down) {
		if (options.debug)
			fprintf(stderr,
				"^^^ ReparentNotify: ignoring window 0x%08lx while shutting down\n",
				xre->window);
		return;
	}

	XFindContext(dpy, xre->window, ClientContext, (XPointer *) &c);

	if (c) {
		Bool dockapp_ready = False;
		Bool dockapp_waits = False;

		if (xre->window == c->wind.window) {
			if (options.debug)
				fprintf(stderr, "    --> wind.window 0x%08lx reparented\n",
					c->wind.window);
			if (xre->parent == c->wind.root) {
				if (options.debug)
					fprintf(stderr,
						"    --> wind.window 0x%08lx reparented to root\n",
						c->wind.window);
				c->wind.parent = xre->parent;
				if (c->wind.withdrawing) {
					c->wind.withdrawing = False;
					c->wind.needsremap = True;
					if (options.debug)
						fprintf(stderr,
							"    --> ACQUIRED: wind.window 0x%08lx\n",
							c->wind.window);
					dockapp_ready = True;
					if (c->icon.withdrawing) {
						if (options.debug)
							fprintf(stderr,
								"    --> WAITING: icon.window 0x%08lx\n",
								c->icon.window);
						dockapp_ready = False;
						dockapp_waits = True;
					}
				}
			} else {
				if (options.debug)
					fprintf(stderr,
						"    --> wind.window 0x%08lx reparented to 0x%08lx\n",
						c->wind.window, xre->parent);
				c->wind.parent = xre->parent;
			}
		} else if (xre->window == c->icon.window) {
			if (options.debug)
				fprintf(stderr, "    --> icon.window 0x%08lx reparented\n",
					c->icon.window);
			if (xre->parent == c->icon.root || xre->parent == c->wind.window) {
				if (options.debug)
					fprintf(stderr,
						"    --> icon.window 0x%08lx reparented to %s\n",
						c->icon.window,
						(xre->parent == c->icon.root) ? "root" : "owner");
				c->icon.parent = xre->parent;
				if (c->icon.withdrawing) {
					c->icon.withdrawing = False;
					c->icon.needsremap = True;
					if (options.debug)
						fprintf(stderr,
							"    --> RESTORED: icon.window 0x%08lx\n",
							c->icon.window);
					dockapp_ready = True;
					if (c->wind.withdrawing) {
						if (options.debug)
							fprintf(stderr,
								"    --> WAITING: wind.window 0x%08lx\n",
								c->wind.window);
						dockapp_ready = False;
						dockapp_waits = True;
					}
				}
			} else {
				if (options.debug)
					fprintf(stderr,
						"    --> icon.window 0x%08lx reparented to 0x%08lx\n",
						c->icon.window, xre->parent);
				c->icon.parent = xre->parent;
			}
		} else {
			fprintf(stderr,
				"    --> ERROR: unknown window 0x%08lx: wind=0x%08lx, icon=0x%08lx\n",
				xre->window, c->wind.window, c->icon.window);
		}
		if (dockapp_waits)
			return;
		if (dockapp_ready)
			swallow(xscr, c);
	} else {
		if (options.debug)
			fprintf(stderr,
				"    --> ReparentNotify: window 0x%08lx is unknown window\n",
				xre->window);
		if (xre->parent == xscr->root) {
			if (options.debug)
				fprintf(stderr,
					"    --> ReparentNotify: window 0x%08lx reparented to root\n",
					xre->window);
		} else {
			if (options.debug)
				fprintf(stderr,
					"    --> ReparentNotify: window 0x%08lx reparented to 0x%08lx\n",
					xre->window, xre->parent);
		}
		if (options.debug)
			fprintf(stderr,
				"    --> ReparentNotify: CALLING TEST WINDOW: window = 0x%08lx\n",
				xre->window);
		test_window(xscr, xre->window);
	}
#endif
}

static GdkFilterReturn
event_handler_UnmapNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	if (options.debug) {
		fprintf(stderr, "==> UnmapNotify:\n");
		fprintf(stderr, "    --> event = 0x%08lx\n", xev->xunmap.event);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xunmap.window);
		fprintf(stderr, "    --> from-configure = %s\n",
			xev->xunmap.from_configure ? "True" : "False");
		fprintf(stderr, "<== UnmapNotify:\n");
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_MapNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	XdeClient *c;

	if (options.debug) {
		fprintf(stderr, "==> MapNotify:\n");
		fprintf(stderr, "    --> event = 0x%08lx\n", xev->xmap.event);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xmap.window);
		fprintf(stderr, "    --> override-redirect = %s\n",
			xev->xmap.override_redirect ? "True" : "False");
		fprintf(stderr, "<== MapNotify:\n");
	}
	if (shut_it_down)
		return GDK_FILTER_CONTINUE;
	if (XFindContext(dpy, xev->xany.window, ClientContext, (XPointer *) &c)) {
		if (options.debug)
			fprintf(stderr, "    --> MapNotify: CALLING TEST WINDOW: window = 0x%08lx\n", xev->xmap.window);
		test_window(xscr, xev->xmap.window);
	}
	return GDK_FILTER_CONTINUE;
}

static void update_theme(XdeScreen *xscr, Atom prop);

static GdkFilterReturn
event_handler_PropertyNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	if (options.debug) {
		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n",
			(xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
	}
	if (xscr && xev->xproperty.state == PropertyNewValue) {
		if (xev->xproperty.atom == _XA_XDE_THEME_NAME) {
			update_theme(xscr, xev->xclient.message_type);
			return GDK_FILTER_CONTINUE;	/* event handled */
		}
		if (xev->xproperty.atom == _XA_NET_CLIENT_LIST) {
			update_clients(xscr, xev->xproperty.atom);
			return GDK_FILTER_CONTINUE;	/* event handled */
		}
		if (xev->xproperty.atom == _XA_WIN_CLIENT_LIST) {
			update_clients(xscr, xev->xproperty.atom);
			return GDK_FILTER_CONTINUE;	/* event handled */
		}
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
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

	DPRINT();
	if (options.debug) {
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
	if (xscr && xev->xclient.message_type == _XA_GTK_READ_RCFILES) {
		update_theme(xscr, xev->xclient.message_type);
		return GDK_FILTER_CONTINUE;	/* event handled */
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
}

static GdkFilterReturn
event_handler_SelectionClear(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> SelectionClear: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xselectionclear.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xselectionclear.window);
		fprintf(stderr, "    --> selection = %s\n",
			XGetAtomName(dpy, xev->xselectionclear.selection));
		fprintf(stderr, "    --> time = %lu\n", xev->xselectionclear.time);
		fprintf(stderr, "<== SelectionClear: %p\n", xscr);
	}
	if (xscr && xev->xselectionclear.window == xscr->selwin) {
		XDestroyWindow(dpy, xscr->selwin);
		EPRINTF("selection cleared, exiting\n");
		exit(EXIT_SUCCESS);
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
window_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = (typeof(xscr)) data;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	switch (xev->type) {
	case CreateNotify:
		return event_handler_CreateNotify(dpy, xev, xscr);
	case DestroyNotify:
		return event_handler_DestroyNotify(dpy, xev, xscr);
	case ReparentNotify:
		return event_handler_ReparentNotify(dpy, xev, xscr);
	case UnmapNotify:
		return event_handler_UnmapNotify(dpy, xev, xscr);
	case MapNotify:
		return event_handler_MapNotify(dpy, xev, xscr);
	case PropertyNotify:
		return event_handler_PropertyNotify(dpy, xev, xscr);
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
root_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = (typeof(xscr)) data;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	switch (xev->type) {
	case CreateNotify:
		return event_handler_CreateNotify(dpy, xev, xscr);
	case DestroyNotify:
		return event_handler_DestroyNotify(dpy, xev, xscr);
	case ReparentNotify:
		return event_handler_ReparentNotify(dpy, xev, xscr);
	case UnmapNotify:
		return event_handler_UnmapNotify(dpy, xev, xscr);
	case MapNotify:
		return event_handler_MapNotify(dpy, xev, xscr);
	case PropertyNotify:
		return event_handler_PropertyNotify(dpy, xev, xscr);
	}
	return GDK_FILTER_CONTINUE;
}

void
window_opened(WnckScreen *wscreen, WnckWindow *win, gpointer data)
{
	XdeScreen *xscr = data;
	Window w;

	if ((w = wnck_window_get_xid(win)))
		test_window(xscr, w);
}

Bool
good_window_manager(XdeScreen *xscr)
{
	DPRINT();
	/* ignore non fully compliant names */
	if (!xscr->wmname)
		return False;
	/* XXX: 2bwm(1) is supported and works well */
	if (!strcasecmp(xscr->wmname, "2bwm"))
		return True;
	/* XXX: adwm(1) provides its own dock, but should handle reparenting. */
	if (!strcasecmp(xscr->wmname, "adwm"))
		return True;
	/* XXX: aewm(1) is supported and works well */
	if (!strcasecmp(xscr->wmname, "aewm"))
		return True;
	/* XXX: aewm++(1) does not support EWMH */
	if (!strcasecmp(xscr->wmname, "aewm++"))
		return False;
	/* XXX: afterstep(1) provides its own dock, but should handle reparenting */
	if (!strcasecmp(xscr->wmname, "afterstep"))
		return True;
	/* XXX: awesome(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "awesome"))
		return True;
	/* XXX: blackbox(1) provides its own dock, but should handle reparenting */
	if (!strcasecmp(xscr->wmname, "blackbox"))
		return True;
	/* XXX: bspwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "bspwm"))
		return True;
	/* XXX: ctwm(1) is only GNOME/WinWM compliant and is not yet supported by
	   libwnck+.  Use etwm(1) instead.  xde-dock mitigates this somewhat, so it is
	   still listed as supported. */
	if (!strcasecmp(xscr->wmname, "ctwm"))
		return True;
	/* XXX: cwm(1) is supported, but it doesn't work that well because cwm(1) is not
	   placing _NET_WM_STATE on client windows, so libwnck+ cannot locate them and
	   will not reparent dock apps to the dock. */
	if (!strcasecmp(xscr->wmname, "cwm"))
		return True;
	/* XXX: dtwm(1) is only OSF/Motif compliant and does support multiple desktops;
	   however, libwnck+ does not yet support OSF/Motif/CDE.  This is not mitigated
	   by xde-dock. */
	if (!strcasecmp(xscr->wmname, "dtwm"))
		return False;
	return False;
}

Bool
window_manager_has_dock(XdeScreen *xscr)
{
	DPRINT();
	/* ignore non fully compliant names */
	if (!xscr->wmname)
		return False;
	if (!strcasecmp(xscr->wmname, "adwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "afterstep"))
		return True;
	if (!strcasecmp(xscr->wmname, "blackbox"))
		return True;
	if (!strcasecmp(xscr->wmname, "fluxbox"))
		return True;
	if (!strcasecmp(xscr->wmname, "fvwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "mvwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "openbox"))
		return True;
	if (!strcasecmp(xscr->wmname, "pekwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "uwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "waimea"))
		return True;
	if (!strcasecmp(xscr->wmname, "wmaker"))
		return True;
	return False;
}

void
window_manager_changed(WnckScreen *wscreen, gpointer data)
{
	XdeScreen *xscr = data;
	const char *name;

	DPRINT();
	wnck_screen_force_update(xscr->wnck);
	free(xscr->wmname);
	xscr->wmname = NULL;
	xscr->hasdock = False;
	xscr->goodwm = False;
	if ((name = wnck_screen_get_window_manager_name(xscr->wnck))) {
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
		xscr->hasdock = window_manager_has_dock(xscr);
		xscr->goodwm = good_window_manager(xscr);
	}
}

gboolean
on_int_signal(gpointer user_data)
{
	shut_it_down = True;
	gtk_main_quit();
	return G_SOURCE_CONTINUE;
}

gboolean
on_hup_signal(gpointer user_data)
{
	shut_it_down = True;
	gtk_main_quit();
	return G_SOURCE_CONTINUE;
}

gboolean
on_term_signal(gpointer user_data)
{
	shut_it_down = True;
	gtk_main_quit();
	return G_SOURCE_CONTINUE;
}

gboolean
on_quit_signal(gpointer user_data)
{
	shut_it_down = True;
	gtk_main_quit();
	return G_SOURCE_CONTINUE;
}

static void
update_theme(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	XTextProperty xtp = { NULL, };
	char **list = NULL;
	int strings = 0;
	Bool changed = False;
	GtkSettings *set;

	DPRINT();
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
		DPRINTF("could not get _XDE_THEME_NAME for root 0x%lx\n", root);
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
		DPRINTF("New theme is %s\n", xscr->theme);
		/* FIXME: do something more about it. */
	}
	GList *dock;

	for (dock = xscr->docks; dock; dock = dock->next) {
		XdeDock *d = dock->data;

#if 0
		GList *client;

#if 0
		gtk_widget_reset_rc_styles(d->dock);	/* doesn't help */
		gtk_widget_queue_clear(d->dock);	/* doesn't help */
#endif
		relax();
		XClearArea(dpy, d->dwin, 0, 0, 0, 0, True);
		XSync(dpy, False);

		for (client = xscr->clients; client; client = client->next) {
			XdeClient *c = client->data;

#if 0
			if (c->ebox) {
				gtk_widget_queue_clear(c->ebox);
				relax();
			}
#endif
			if (c->plugged) {
				relax();
				XClearArea(dpy, c->plugged, 0, 0, 0, 0, True);
				XSync(dpy, False);
			}
		}
#else
		XClearArea(dpy, root, d->x, d->y, d->w, d->h, True);
#endif
	}
}

static GdkFilterReturn
client_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	Display *dpy = data;

	DPRINT();
	switch (xev->type) {
	case ClientMessage:
		return event_handler_ClientMessage(dpy, xev);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;	/* event not handled, continue processing */
}

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

	DPRINT();
	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);

	dpy = GDK_DISPLAY_XDISPLAY(disp);

	for (s = 0; s < nscr; s++) {
		snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
		atom = XInternAtom(dpy, selection, False);
		if (!(owner = XGetSelectionOwner(dpy, atom)))
			DPRINTF("No owner for %s\n", selection);
		if ((owner && replace) || (!owner && selwin)) {
			DPRINTF("Setting owner of %s to 0x%08lx from 0x%08lx\n", selection,
				selwin, owner);
			XSetSelectionOwner(dpy, atom, selwin, options.timestamp);
			XSync(dpy, False);
		}
		if (!gotone && owner)
			gotone = owner;
	}
	if (replace) {
		if (gotone) {
			if (selwin)
				DPRINTF("%s: replacing running instance\n", NAME);
			else
				DPRINTF("%s: quitting running instance\n", NAME);
		} else {
			if (selwin)
				DPRINTF("%s: no running instance to replace\n", NAME);
			else
				DPRINTF("%s: no running instance to quit\n", NAME);
		}
		if (selwin) {
			XEvent ev;
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
				ev.xclient.data.l[0] = options.timestamp;	/* FIXME */
				ev.xclient.data.l[1] = atom;
				ev.xclient.data.l[2] = selwin;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;

				XSendEvent(dpy, root, False, StructureNotifyMask, &ev);
				XFlush(dpy);
			}
		}
	} else if (gotone)
		DPRINTF("%s: not replacing running instance\n", NAME);
	return (gotone);
}

static void
update_screen_size(XdeScreen *xscr, int new_width, int new_height)
{
}

static void
create_monitor(XdeScreen *xscr, XdeMonitor *mon, int m)
{
	memset(mon, 0, sizeof(*mon));
}

static void
delete_monitor(XdeScreen *xscr, XdeMonitor *mon, int m)
{
}

static void
update_monitor(XdeScreen *xscr, XdeMonitor *mon, int m)
{
}

static void
update_screen(XdeScreen *xscr)
{
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
		DPRINTF("Arrrghhh! screen pointer changed from %p to %p\n", xscr->scrn, scrn);
		xscr->scrn = scrn;
	}
	width = gdk_screen_get_width(scrn);
	height = gdk_screen_get_height(scrn);
	if (xscr->width != width || xscr->height != height) {
		DPRINTF("Screen %d dimensions changed %dx%d -> %dx%d\n", index,
			xscr->width, xscr->height, width, height);
		/* FIXME: reset size of screen */
		update_screen_size(xscr, width, height);
		xscr->width = width;
		xscr->height = height;
	}
	nmon = gdk_screen_get_n_monitors(scrn);
	DPRINTF("Reallocating %d monitors\n", nmon);
	xscr->mons = realloc(xscr->mons, nmon * sizeof(*xscr->mons));
	if (nmon > xscr->nmon) {
		DPRINTF("Screen %d number of monitors increased from %d to %d\n",
			index, xscr->nmon, nmon);
		for (m = xscr->nmon; m < nmon; m++) {
			mon = xscr->mons + m;
			create_monitor(xscr, mon, m);
		}
	} else if (nmon < xscr->nmon) {
		DPRINTF("Screen %d number of monitors decreased from %d to %d\n",
			index, xscr->nmon, nmon);
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
on_monitors_changed(GdkScreen *scrn, gpointer xscr)
{
	refresh_screen(xscr, scrn);
}

/** @brief screen size changed
  *
  * The size of the screen changed.  This may be as a result of RANDR or
  * XINERAMA changes.  Walk through the screen and the monitors on the screen
  * and adjust the necessary parameters.
  */
static void
on_size_changed(GdkScreen *scrn, gpointer xscr)
{
	refresh_screen(xscr, scrn);
}

static void
init_monitors(XdeScreen *xscr)
{
	int m;
	XdeMonitor *mon;

	g_signal_connect(G_OBJECT(xscr->scrn), "monitors-changed",
			G_CALLBACK(on_monitors_changed), xscr);
	g_signal_connect(G_OBJECT(xscr->scrn), "size-changed",
			G_CALLBACK(on_size_changed), xscr);

	xscr->nmon = gdk_screen_get_n_monitors(xscr->scrn);
	xscr->mons = calloc(xscr->nmon, sizeof(*xscr->mons));
	for (m = 0, mon = xscr->mons; m < xscr->nmon; m++, mon++) {
		mon->index = m;
		gdk_screen_get_monitor_geometry(xscr->scrn, m, &mon->geom);
	}
}

static GdkFilterReturn
selwin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = data;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);

	DPRINT();
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

static void
init_wnck(XdeScreen *xscr)
{
	WnckScreen *wnck = xscr->wnck = wnck_screen_get(xscr->index);

	g_signal_connect(G_OBJECT(wnck), "window_opened", G_CALLBACK(window_opened), xscr);
	g_signal_connect(G_OBJECT(wnck), "window_manager_changed",
			 G_CALLBACK(window_manager_changed), xscr);

	wnck_screen_force_update(wnck);
	window_manager_changed(wnck, xscr);
}

/** @brief Ask a running instance to quit.
  *
  * This is performed by checking for an owner of the selection and clearing the
  * selection if it exists.
  */
static void
do_quit(int argc, char *argv[])
{
	DPRINT();
	get_selection(True, None);
}

static void
do_run(int argc, char *argv[], Bool replace)
{
	GdkDisplay *disp = gdk_display_get_default();
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	char selection[64] = { 0, };
	GdkWindow *root = gdk_screen_get_root_window(scrn), *sel;
	Window selwin, owner;
	XdeScreen *xscr;
	int s, nscr;

	DPRINT();
	selwin = XCreateSimpleWindow(dpy, GDK_WINDOW_XID(root), 0, 0, 1, 1, 0, 0, 0);

	if ((owner = get_selection(replace, selwin))) {
		if (!replace) {
			XDestroyWindow(dpy, selwin);
			EPRINTF("%s: instance already running\n", NAME);
			exit(EXIT_FAILURE);
		}
	}
	XSelectInput(dpy, selwin,
		     StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask);

	nscr = gdk_display_get_n_screens(disp);
	screens = calloc(nscr, sizeof(*screens));

	sel = gdk_x11_window_foreign_new_for_display(disp, selwin);
	gdk_window_add_filter(sel, selwin_handler, screens);

	for (s = 0, xscr = screens; s < nscr; s++, xscr++) {
		snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
		xscr->index = s;
		xscr->atom = XInternAtom(dpy, selection, False);
		xscr->disp = disp;
		xscr->scrn = gdk_display_get_screen(disp, s);
		xscr->root = gdk_screen_get_root_window(xscr->scrn);
		xscr->selwin = selwin;
		xscr->width = gdk_screen_get_width(xscr->scrn);
		xscr->height = gdk_screen_get_height(xscr->scrn);
		gdk_window_add_filter(xscr->root, root_handler, xscr);
		init_monitors(xscr);
		create_dock(xscr);
		init_wnck(xscr);
		update_theme(xscr, None);
	}
	gtk_main();
}

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
clientSaveYourselfCB(SmcConn smcConn, SmPointer data, int saveType, Bool shutdown,
		     int interactStyle, Bool fast)
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

/* *INDENT-OFF* */
static unsigned long clientCBMask =
	SmcSaveYourselfProcMask |
	SmcDieProcMask |
	SmcSaveCompleteProcMask |
	SmcShutdownCancelledProcMask;

static SmcCallbacks clientCBs = {
	.save_yourself = {
		.callback = &clientSaveYourselfCB,
		.client_data = NULL,
	},
	.die = {
		.callback = &clientDieCB,
		.client_data = NULL,
	},
	.save_complete = {
		.callback = &clientSaveCompleteCB,
		.client_data = NULL,
	},
	.shutdown_cancelled = {
		.callback = &clientShutdownCancelledCB,
		.client_data = NULL,
	},
};
/* *INDENT-ON* */

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

void
startup(int argc, char *argv[])
{
	GdkDisplay *disp;
	GdkAtom atom;
	Display *dpy;
	char *file;
	int nscr;

	file = g_strdup_printf("%s/.gtkrc-2.0.xde", g_get_home_dir());
	gtk_rc_add_default_file(file);
	g_free(file);

	ClientContext = XUniqueContext();

	init_smclient();

	gtk_init(&argc, &argv);

	g_unix_signal_add(SIGINT, on_int_signal, NULL);
	g_unix_signal_add(SIGHUP, on_hup_signal, NULL);
	g_unix_signal_add(SIGTERM, on_term_signal, NULL);

	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);

	if (options.screen >= 0 && options.screen >= nscr) {
		EPRINTF("bad screen specified: %d\n", options.screen);
		exit(EXIT_FAILURE);
	}

	dpy = GDK_DISPLAY_XDISPLAY(disp);

	atom = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_XA_XDE_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_GTK_READ_RCFILES");
	_XA_GTK_READ_RCFILES = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);

	atom = gdk_atom_intern_static_string("_NET_CLIENT_LIST");
	_XA_NET_CLIENT_LIST = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_CLIENT_LIST");
	_XA_WIN_CLIENT_LIST = gdk_x11_atom_to_xatom_for_display(disp, atom);

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
		options.command = CommandRun;
}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;

	setlocale(LC_ALL, "");

	set_defaults();

	saveArgc = argc;
	saveArgv = argv;

	while (1) {
		int c, val;
		char *endptr = NULL;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"display",	required_argument,	NULL,	'd'},
			{"screen",	required_argument,	NULL,	's'},

			{"quit",	no_argument,		NULL,	'q'},
			{"replace",	no_argument,		NULL,	'r'},

			{"clientId",	required_argument,	NULL,	'8'},
			{"restore",	required_argument,	NULL,	'9'},

			{"debug",	optional_argument,	NULL,	'D'},
			{"verbose",	optional_argument,	NULL,	'v'},
			{"help",	no_argument,		NULL,	'h'},
			{"version",	no_argument,		NULL,	'V'},
			{"copying",	no_argument,		NULL,	'C'},
			{"?",		no_argument,		NULL,	'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "d:s:qrD::v::hVCH?", long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:qrD:vhVCH?");
#endif				/* _GNU_SOURCE */
		if (c == -1) {
			if (options.debug)
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
			options.screen = strtoul(optarg, &endptr, 0);
			if (*endptr)
				goto bad_option;
			break;

		case 'q':	/* -q, --quit */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandQuit;
			options.command = CommandQuit;
			break;
		case 'r':	/* -r, --replace */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandReplace;
			options.command = CommandReplace;
			break;

		case '8':	/* -clientId CLIENTID */
			free(options.clientId);
			options.clientId = strdup(optarg);
			break;
		case '9':	/* -restore SAVEFILE */
			free(options.saveFile);
			options.saveFile = strdup(optarg);
			break;

		case 'D':	/* -D, --debug [LEVEL] */
			if (options.debug)
				fprintf(stderr, "%s: increasing debug verbosity\n", argv[0]);
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
			if (options.debug)
				fprintf(stderr, "%s: increasing output verbosity\n", argv[0]);
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
		case 'h':	/* -h, --help */
		case 'H':	/* -H, --? */
			command = CommandHelp;
			break;
		case 'V':	/* -V, --version */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandVersion;
			options.command = CommandVersion;
			break;
		case 'C':	/* -C, --copying */
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
	DPRINTF("%s: option index = %d\n", argv[0], optind);
	DPRINTF("%s: option count = %d\n", argv[0], argc);
#if 0
	/* glibc keeps breaking this for optional arguments */
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
#endif
	get_defaults();
	startup(argc, argv);
	switch (command) {
	default:
	case CommandDefault:
	case CommandRun:
		DPRINTF("%s: running a new instance\n", argv[0]);
		do_run(argc, argv, False);
		break;
	case CommandQuit:
		DPRINTF("%s: asking existing instance to quit\n", argv[0]);
		do_quit(argc, argv);
		break;
	case CommandReplace:
		DPRINTF("%s: replacing existing instance\n", argv[0]);
		do_run(argc, argv, True);
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

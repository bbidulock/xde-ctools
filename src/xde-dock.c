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
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>


#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

const char *program = NAME;

Display *dpy;
Window root;
int napps;
WnckScreen *wnck;
int screen;
Bool shutting_down = False;

Atom _XA_XDE_THEME_NAME;
Atom _XA_GTK_READ_RCFILES;

GdkAtom _GA_XDE_THEME_NAME;
GdkAtom _GA_GTK_READ_RCFILES;
GdkDisplayManager *gmgr;
GdkDisplay *gdpy;
GdkScreen *gscr;
GdkWindow *groot;

typedef struct {
	Window save;
	GtkWidget *dock;
	GtkWidget *vbox;
	GdkWindow *gwin;
	Window dwin;
	gint x, y, w, h, d;
} Dock;

Dock dock;

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

#define XDE_DOCK_DONT_WAIT 1

typedef struct {
	Window window;
	Bool withdrawing;
	Bool mapped;
	Bool reparented;
	Window root;
	Window parent;
	Bool needsremap;
} ClientWindow;

typedef struct _Client {
	struct _Client *next;
	ClientWindow wind, icon;
	Bool swallowed;
	XClassHint ch;
	char **argv;
	int argc;
	GtkWidget *ebox;
	GtkWidget *sock;
	int retries;
	Window plugged;
} Client;

Client *clients;

XContext ClientContext;

typedef enum {
	DockAppTestMine,
	DockAppTestFlipse,
	DockAppTestBlackbox,
	DockAppTestFluxbox,
	DockAppTestOpenbox,
} DockAppTest;

struct Options {
	int debug;
	int output;
	DockPosition position;
	DockDirection direction;
	DockAppTest test;
} options = {
	.debug = 0,
	.output = 1,
	.position = DockPositionEast,
	.direction = DockDirectionVertical,
	.test = DockAppTestMine,
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
    %1$s [options]\n\
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
	(void) fprintf(stdout, "\
Usage:\n\
    %1$s [options]\n\
    %1$s {-h|--help}\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
Command options:\n\
    -h, --help, -?, --?\n\
        print this usage information and exit\n\
    -V, --version\n\
        print version and exit\n\
    -C, --copying\n\
        print copying permission and exit\n\
Options:\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: 0]\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: 1]\n\
        this option may be repeated.\n\
", argv[0]);
}

void
relax()
{
	XSync(dpy, False);
	while (gtk_events_pending())
		if (gtk_main_iteration())
			break;
}

void
dock_rearrange()
{
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
	Screen *scrn = ScreenOfDisplay(dpy, screen);
	dock.w = scrn->width;
	dock.h = scrn->height;
	dock.d = 0;

	if (options.debug)
		fprintf(stderr, "==> REARRANGING DOCK:\n");
	if (napps <= 0) {
		if (options.debug)
			fprintf(stderr, "    --> HIDING DOCK: no dock apps\n");
		gtk_widget_hide(GTK_WIDGET(dock.dock));
		return;
	}
	pos = options.position;
	dir = options.direction;
	dock.x = dock.y = 0;
	switch (pos) {
	case DockPositionNorth:
		switch (dir) {
		case DockDirectionVertical:
			strut.top = napps * 64;
			strut.top_start_x = (dock.w - 64) / 2;
			strut.top_end_x = (dock.w + 64) / 2;
			break;
		case DockDirectionHorizontal:
			strut.top = 64;
			strut.top_start_x = (dock.w - (napps * 64)) / 2;
			strut.top_end_x = (dock.w + (napps * 64)) / 2;
			break;
		}
		dock.x = strut.top_start_x;
		dock.y = 0;
		break;
	case DockPositionNorthWest:
		switch (dir) {
		case DockDirectionVertical:
			strut.left = 64;
			strut.left_start_y = 0;
			strut.left_end_y = napps * 64;
			break;
		case DockDirectionHorizontal:
			strut.top = 64;
			strut.top_start_x = 0;
			strut.top_end_x = napps * 64;
			break;
		}
		dock.x = 0;
		dock.y = 0;
		break;
	case DockPositionWest:
		switch (dir) {
		case DockDirectionVertical:
			strut.left = 64;
			strut.left_start_y = (dock.h - (napps * 64)) / 2;
			strut.left_end_y = (dock.h + (napps * 64)) / 2;
			break;
		case DockDirectionHorizontal:
			strut.left = napps * 64;
			strut.left_start_y = (dock.h - 64) / 2;
			strut.left_end_y = (dock.h + 64) / 2;
			break;
		}
		dock.x = 0;
		dock.y = strut.left_start_y;
		break;
	case DockPositionSouthWest:
		switch (dir) {
		case DockDirectionVertical:
			strut.left = 64;
			strut.left_start_y = dock.h - napps * 64;
			strut.left_end_y = dock.h;
			dock.y = strut.left_start_y;
			break;
		case DockDirectionHorizontal:
			strut.bottom = 64;
			strut.bottom_start_x = 0;
			strut.bottom_end_x = napps * 64;
			dock.y = dock.h - strut.bottom;
			break;
		}
		dock.x = 0;
		break;
	case DockPositionSouth:
		switch (dir) {
		case DockDirectionVertical:
			strut.bottom = napps * 64;
			strut.bottom_start_x = (dock.h - 64) / 2;
			strut.bottom_end_x = (dock.h + 64) / 2;
			break;
		case DockDirectionHorizontal:
			strut.bottom = 64;
			strut.bottom_start_x = (dock.h - (napps * 64)) / 2;
			strut.bottom_end_x = (dock.h + (napps * 64)) / 2;
			break;
		}
		dock.x = strut.bottom_start_x;
		dock.y = dock.h - strut.bottom;
		break;
	case DockPositionSouthEast:
		switch (dir) {
		case DockDirectionVertical:
			strut.right = 64;
			strut.right_start_y = dock.h - napps * 64;
			strut.right_end_y = dock.h;
			dock.x = dock.w - strut.right;
			dock.y = strut.right_start_y;
			break;
		case DockDirectionHorizontal:
			strut.bottom = 64;
			strut.bottom_start_x = dock.w - napps * 64;
			strut.bottom_end_x = dock.w;
			dock.x = strut.bottom_start_x;
			dock.y = dock.h = strut.bottom;
			break;
		}
		break;
	case DockPositionEast:
		switch (dir) {
		case DockDirectionVertical:
			strut.right = 64;
			strut.right_start_y = (dock.h - (napps * 64)) / 2;
			strut.right_end_y = (dock.h + (napps * 64)) / 2;
			break;
		case DockDirectionHorizontal:
			strut.right = napps * 64;
			strut.right_start_y = (dock.h - 64) / 2;
			strut.right_end_y = (dock.h + 64) / 2;
			break;
		}
		dock.x = dock.w - strut.right;
		dock.y = strut.right_start_y;
		break;
	case DockPositionNorthEast:
		switch (dir) {
		case DockDirectionVertical:
			strut.right = 64;
			strut.right_start_y = 0;
			strut.right_end_y = napps * 64;
			dock.x = dock.w - strut.right;
			dock.y = strut.right_start_y;
			break;
		case DockDirectionHorizontal:
			strut.top = 64;
			strut.top_start_x = dock.w - (napps * 64);
			strut.top_end_x = dock.w;
			dock.x = strut.top_start_x;
			dock.y = 0;
			break;
		}
		break;
	}
	if (options.debug)
		fprintf(stderr, "    --> MOVING dock to (%d,%d)\n", dock.x, dock.y);

	gtk_window_move(GTK_WINDOW(dock.dock), dock.x, dock.y);
	gdk_window_move(GDK_WINDOW(dock.gwin), dock.x, dock.y);
	relax();
	gdk_window_get_geometry(GDK_WINDOW(dock.gwin), &dock.x, &dock.y, &dock.w, &dock.h, &dock.d);
	if (options.debug) {
		fprintf(stderr, "    --> GEOMETRY now (%dx%d+%d+%d:%d)\n", dock.w, dock.h, dock.x, dock.y, dock.d);
		fprintf(stderr, "    --> _NET_WM_STRUT set (%ld,%ld,%ld,%ld)\n", strut.left, strut.right, strut.top, strut.bottom);
		fprintf(stderr, "    --> _NET_WM_STRUT_PARTIAL set (%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld)\n",
			strut.left, strut.right, strut.top, strut.bottom,
			strut.left_start_y, strut.left_end_y, strut.right_start_y, strut.right_end_y,
			strut.top_start_x, strut.top_end_x, strut.bottom_start_x, strut.bottom_end_x);
	}
	XChangeProperty(dpy, dock.dwin, XInternAtom(dpy, "_NET_WM_STRUT", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &strut, 4);
	XChangeProperty(dpy, dock.dwin, XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False), XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &strut, 12);
	relax();
}

void add_dockapp(Client * c, ClientWindow * cwin);

gboolean
on_plug_removed(GtkSocket * s, gpointer data)
{
	Client *c = (typeof(c)) data;

	if (options.debug)
		fprintf(stderr, "*** PLUG REMOVED: wind.window = 0x%08lx icon.window = 0x%08lx\n", c->wind.window, c->icon.window);
	if (c->ebox) {
		gtk_widget_destroy(GTK_WIDGET(c->ebox));
		c->ebox = NULL;
		c->swallowed = False;
		napps -= 1;
		if (c->plugged && c->retries++ < 5) {
			if (c->plugged == c->wind.window) {
				add_dockapp(c, &c->wind);
			} else {
				add_dockapp(c, &c->icon);
			}
		} else {
			c->plugged = None;
			c->retries = 0;
			dock_rearrange();
		}
	}
	return FALSE;
}

void
swallow(Client * c)
{
	Window wind = c->wind.window;
	Window icon = c->icon.window;
	ClientWindow *cwin;

	if (c->swallowed) {
		if (options.debug)
			fprintf(stderr, "==> NOT SWALLOWING: wind.window 0x%08lx icon.window 0x%08lx\n", wind, icon);
		return;
	}
	if (options.debug)
		fprintf(stderr, "==> SWALLOWING window 0x%08lx with icon 0x%08lx!\n", wind, icon);
	relax();
	if (XGetClassHint(dpy, wind, &c->ch)) {
		if (options.debug)
			fprintf(stderr, "    --> window 0x%08lx '%s', '%s'\n", wind, c->ch.res_name, c->ch.res_class);
	}
	if (XGetCommand(dpy, c->wind.window, &c->argv, &c->argc)) {
		if (options.debug) {
			int i;

			fprintf(stderr, "    --> window 0x%08lx '", wind);
			for (i = 0; i < c->argc; i++)
				fprintf(stderr, "%s%s", c->argv[i], (c->argc == i + 1) ? "" : "', '");

			fprintf(stderr, "%s\n", "'");
		}
	}
	{
		Window *children;
		unsigned int i, nchild;

		if (wind && XQueryTree(dpy, wind, &c->wind.root, &c->wind.parent, &children, &nchild)) {
			if (options.debug) {
				fprintf(stderr, "    --> wind.window 0x%08lx has root 0x%08lx\n", wind, c->wind.root);
				fprintf(stderr, "    --> wind.window 0x%08lx has parent 0x%08lx\n", wind, c->wind.parent);
				for (i = 0; i < nchild; i++)
					fprintf(stderr, "    --> wind.window 0x%08lx has child 0x%08lx\n", wind, children[i]);
			}
			if (children)
				XFree(children);
		}
		if (icon && XQueryTree(dpy, icon, &c->icon.root, &c->icon.parent, &children, &nchild)) {
			if (options.debug) {
				fprintf(stderr, "    --> icon.window 0x%08lx has root 0x%08lx\n", icon, c->icon.root);
				fprintf(stderr, "    --> icon.window 0x%08lx has parent 0x%08lx\n", icon, c->icon.parent);
				for (i = 0; i < nchild; i++)
					fprintf(stderr, "    --> icon.window 0x%08lx has child 0x%08lx\n", icon, children[i]);
			}
			if (children)
				XFree(children);
		}
	}
	if (!c->icon.window || (c->icon.parent == c->wind.window)) {
		if (options.debug)
			fprintf(stderr, "    --> REPARENTING: wind.window 0x%08lx to socket\n", c->wind.window);
		cwin = &c->wind;
	} else {
#ifdef XDE_DOCK_DONT_WAIT
		XUnmapWindow(dpy, c->wind.window);
		relax();
		relax();
#else
		if (options.debug)
			fprintf(stderr, "    --> REPARENTING: wind.window 0x%08lx to save 0x%08lx\n", c->wind.window, dock.save);
		XAddToSaveSet(dpy, c->wind.window);
		XReparentWindow(dpy, c->wind.window, dock.save, 0, 0);
		XMapWindow(dpy, c->wind.window);
#endif
		if (options.debug)
			fprintf(stderr, "    --> REPARENTING: icon.window 0x%08lx to socket\n", c->icon.window);
		cwin = &c->icon;
	}

	cwin->mapped = True;

	/* Now we can actually swallow the dock app. */

	add_dockapp(c, cwin);
}

void
add_dockapp(Client * c, ClientWindow * cwin)
{

	int tpad, bpad, lpad, rpad;
	int x, y;
	unsigned int width, height, border, depth;

	if (!XGetGeometry(dpy, cwin->window, &cwin->root, &x, &y, &width, &height, &border, &depth)) {
		fprintf(stderr, "ERROR: window 0x%08lx cannot get geometry\n", cwin->window);
		return;
	}
	if (options.debug) {
		fprintf(stderr, "==> DOCKAPP: geometry %dx%d+%d+%d:%d\n", width, height, x, y, border);
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
		fprintf(stderr, "    --> PACKING SOCKET for window 0x%08lx into dock\n", cwin->window);
	gtk_box_pack_start(GTK_BOX(dock.vbox), GTK_WIDGET(b), FALSE, FALSE, 0);
	napps += 1;
	relax();
	dock_rearrange();
	gtk_widget_show_all(GTK_WIDGET(b));
	gtk_widget_show_all(GTK_WIDGET(a));
	gtk_widget_show_all(GTK_WIDGET(s));
	gtk_widget_show_all(GTK_WIDGET(dock.dock));
	if (options.debug)
		fprintf(stderr, "    --> ADDING window 0x%08lx into socket\n", cwin->window);
	/* We might also use gtk_socket_steal() here.  I don't quite know what the difference is as 
	   both appear to behave the same; however, there is no way to add the window to Gtk2's
	   save set, so we need to figure out how to do that. Perhaps stealing has this effect. */

	cwin->reparented = True;

	c->plugged = cwin->window;
	c->swallowed = True;

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
withdraw_window(ClientWindow * cwin)
{
	Window *children = NULL;
	unsigned int nchild = 0;
	Bool retval = False;

	if (options.debug)
		fprintf(stderr, "==> WITHDRAWING: window 0x%08lx\n", cwin->window);
	if (XQueryTree(dpy, cwin->window, &cwin->root, &cwin->parent, &children, &nchild)) {
		if (options.debug)
			fprintf(stderr, "    --> window 0x%08lx root 0x%08lx parent 0x%08lx\n", cwin->window, cwin->root, cwin->parent);
		if (cwin->root && cwin->parent && (cwin->root != cwin->parent)) {
			if (options.debug)
				fprintf(stderr, "    --> DEPARENTING: window 0x%08lx from parent 0x%08lx\n", cwin->window, cwin->parent);
			cwin->withdrawing = True;
			XUnmapWindow(dpy, cwin->window);
			XWithdrawWindow(dpy, cwin->window, screen);
			relax();
			retval = True;
		}
		if (children)
			XFree(children);
	} else {
		fprintf(stderr, "!!! ERROR: could not query tree for window 0x%08lx\n", cwin->window);
	}
	return retval;
}

void
withdraw_client(Client * c)
{
	if (options.debug)
		fprintf(stderr, "==> WITHDRAWING: window 0x%08lx\n", c->wind.window);
	if (c->icon.window)
		XUnmapWindow(dpy, c->icon.window);
	XUnmapWindow(dpy, c->wind.window);
	XWithdrawWindow(dpy, c->wind.window, screen);
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
					fprintf(stderr, "    --> DestroyNotify: event=0x%08lx window=0x%08lx\n", c->icon.parent, c->icon.window);
				XSendEvent(dpy, c->icon.parent, False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
			}
			if (0) {
				ev.xdestroywindow.event = c->icon.window;
				if (options.debug)
					fprintf(stderr, "    --> DestroyNotify: event=0x%08lx window=0x%08lx\n", c->icon.window, c->icon.window);
				XSendEvent(dpy, c->icon.window, False, StructureNotifyMask, &ev);
			}
		}
	}
	if (1) {
		/* mimic destruction for window managers that are not ICCCM 2.0 compliant */
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
					fprintf(stderr, "    --> DestroyNotify: event=0x%08lx window=0x%08lx\n", c->wind.parent, c->wind.window);
				XSendEvent(dpy, c->wind.parent, False, SubstructureNotifyMask | SubstructureRedirectMask, &ev);
			}
			if (1) {
				ev.xdestroywindow.event = c->wind.window;
				if (options.debug)
					fprintf(stderr, "    --> DestroyNotify: event=0x%08lx window=0x%08lx\n", c->wind.window, c->wind.window);
				XSendEvent(dpy, c->wind.window, False, StructureNotifyMask, &ev);
			}
		}
	}
	relax();
}

XWMHints *
is_dockapp(Window win)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, win))) {
		switch (options.test) {
		case DockAppTestMine:
			/* Many libxpm based dockapps use xlib to set the state hint correctly. */
			if ((wmh->flags & StateHint) && wmh->initial_state == WithdrawnState)
				return (wmh);
			/* Many docapps that were based on GTK+ < 2.4.0 are having their
			   initial_state changed to NormalState by GTK+ >= 2.4.0, so when the other
			   flags are set, accept it anyway */
			if (wmh->initial_state == WithdrawnState || (wmh->flags & ~IconPositionHint) == (WindowGroupHint | StateHint | IconWindowHint))
				return (wmh);
			/* In an attempt to get around GTK+ >= 2.4.0 limitations, some GTK+ dock
			   apps simply set the res_class to "DockApp". */
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
			/* The flipse approach: strange, but StateHint not checked first; so, this
			   translates to the following conditions: (1) initial_state not set, or,
			   (2) initial_state set to WithdrawnState, or, (3), exactly group, state
			   and icon-window hints set. */
			if (wmh->initial_state == WithdrawnState || wmh->flags == (WindowGroupHint | StateHint | IconWindowHint))
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

Bool
test_window(Window wind)
{
	Window icon;
	XWMHints *wmh;
	Client *client;
	Window *children;
	unsigned int nchild;

#ifndef XDE_DOCK_DONT_WAIT
	Bool need_withdraw = False;
#endif

	if (shutting_down)
		return False;
	if (options.debug)
		fprintf(stderr, "==> TESTING WINDOW: window = 0x%08lx\n", wind);
	relax();
	if (!(wmh = is_dockapp(wind)))
		return False;

	/* some dockapps use their main window as an icon window */
	if ((wmh->flags & IconWindowHint))
		icon = wmh->icon_window;
	else
		icon = None;
	/* some dockapp icon windows point to themselves... */
	if (icon == wind)
		icon = None;
	if (options.debug)
		fprintf(stderr, "    --> Should swallow window 0x%08lx with icon 0x%08lx!\n", wind, icon);

	client = calloc(1, sizeof(*client));
	client->next = clients;
	clients = client;
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

	if (wind && XQueryTree(dpy, wind, &client->wind.root, &client->wind.parent, &children, &nchild)) {
		if (children)
			XFree(children);
#ifndef XDE_DOCK_DONT_WAIT
		if (client->wind.parent != client->wind.root) {
			if (options.debug)
				fprintf(stderr, "    --> WITHDRAW REQUIRED: wind.window 0x%08lx is not toplevel (child of 0x%08lx)\n", client->wind.window,
					client->wind.parent);
			need_withdraw = True;
			client->wind.withdrawing = True;
		}
#endif
	}
	if (icon && XQueryTree(dpy, icon, &client->icon.root, &client->icon.parent, &children, &nchild)) {
		if (children)
			XFree(children);
#ifndef XDE_DOCK_DONT_WAIT
		if (client->icon.parent != client->icon.root) {
			if (client->icon.parent != client->wind.window) {
				if (options.debug)
					fprintf(stderr, "    --> WITHDRAW REQUIRED: icon.window 0x%08lx is not toplevel (child or 0x%08lx)\n",
						client->icon.window, client->icon.parent);
				need_withdraw = True;
				client->icon.withdrawing = True;
			}
		}
#endif
	}
	/* Some window managers leave the wind.window mapped as a toplevel (offscreen) and just
	   reparent the icon.window; however, it takes a withdraw request for the toplevel
	   wind.window to cause the icon.window to be reparented (and unmapped). */

#ifndef XDE_DOCK_DONT_WAIT
	if (need_withdraw) {
		withdraw_client(client);
	} else
#endif
		swallow(client);

	relax();
	return True;
}

void search_kids(Window win);

void
search_window(Window win)
{
	Client *client;

	if (XFindContext(dpy, win, ClientContext, (XPointer *) &client)
	    && !test_window(win))
		 search_kids(win);
}

void
search_kids(Window win)
{
	Window wroot = None, parent = None, *children = NULL;
	unsigned int i, nchild = 0;

	if (win == dock.dwin || win == dock.save)
		return;

	relax();
	if (XQueryTree(dpy, win, &wroot, &parent, &children, &nchild)) {
		for (i = 0; i < nchild; i++)
			search_window(children[i]);
		if (children)
			XFree(children);
	}
}

void
find_dockapps()
{
	search_kids(root);
}

void reparse();

void
create_dock()
{
	dock.dock = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(dock.dock), GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_window_set_default_size(GTK_WINDOW(dock.dock), 64, -1);
	gtk_window_set_decorated(GTK_WINDOW(dock.dock), FALSE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(dock.dock), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dock.dock), TRUE);
	gtk_window_stick(GTK_WINDOW(dock.dock));
	gtk_window_set_deletable(GTK_WINDOW(dock.dock), FALSE);
	gtk_window_set_focus_on_map(GTK_WINDOW(dock.dock), FALSE);
	gtk_window_set_has_frame(GTK_WINDOW(dock.dock), FALSE);
	gtk_window_set_keep_above(GTK_WINDOW(dock.dock), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(dock.dock), FALSE);

	dock.vbox = gtk_vbox_new(TRUE, 0);
	gtk_widget_set_size_request(GTK_WIDGET(dock.vbox), 64, -1);
	gtk_container_add(GTK_CONTAINER(dock.dock), GTK_WIDGET(dock.vbox));
	gtk_widget_realize(GTK_WIDGET(dock.dock));
	dock.gwin = gtk_widget_get_window(GTK_WIDGET(dock.dock));
	dock.dwin = GDK_WINDOW_XID(dock.gwin);

	reparse();
	find_dockapps();
}

int
handler(Display *display, XErrorEvent * xev)
{
	if (options.debug) {
		char msg[80], req[80], num[80], def[80];

		snprintf(num, sizeof(num), "%d", xev->request_code);
		snprintf(def, sizeof(def), "[request_code=%d]", xev->request_code);
		XGetErrorDatabaseText(dpy, "xdg-setwm", num, def, req, sizeof(req));
		if (XGetErrorText(dpy, xev->error_code, msg, sizeof(msg)) != Success)
			msg[0] = '\0';
		fprintf(stderr, "X error %s(0x%08lx): %s\n", req, xev->resourceid, msg);
	}
	return (0);
}

void
event_handler_CreateNotify(XEvent  *xev)
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
	return;
}

void
event_handler_DestroyNotify(XEvent *xev)
{
	if (options.debug) {
		fprintf(stderr, "==> DestroyNotify:\n");
		fprintf(stderr, "    --> event = 0x%08lx\n", xev->xdestroywindow.event);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xdestroywindow.window);
		fprintf(stderr, "<== DestroyNotify:\n");
	}
	return;
}

void
event_handler_ReparentNotify(XEvent *xev)
{
#ifdef XDE_DOCK_DONT_WAIT
	if (options.debug) {
		fprintf(stderr, "==> ReparentNotify:\n");
		fprintf(stderr, "    --> event = 0x%08lx\n", xev->xreparent.event);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xreparent.window);
		fprintf(stderr, "    --> parent = 0x%08lx\n", xev->xreparent.parent);
		fprintf(stderr, "    --> x = %d\n", xev->xreparent.x);
		fprintf(stderr, "    --> y = %d\n", xev->xreparent.y);
		fprintf(stderr, "    --> override-redirect = %s\n", xev->xreparent.override_redirect ? "True" : "False");
		fprintf(stderr, "<== ReparentNotify:\n");
	}
	return;
#else
	XReparentEvent *xre = (typeof(xre)) xev;
	Client *c = NULL;


	if (options.debug)
		fprintf(stderr, "==> ReparentNotify: event=0x%08lx window=0x%08lx parent=0x%08lx\n", xre->event, xre->window, xre->parent);
	if (xre->override_redirect) {
		if (options.debug)
			fprintf(stderr, "^^^ ReparentNotify: ignoring override-redirect window 0x%08lx\n", xre->window);
		return;
	}
	if (shutting_down) {
		if (options.debug)
			fprintf(stderr, "^^^ ReparentNOtify: ignoring window 0x%08lx while shutting down\n", xre->window);
		return;
	}

	XFindContext(dpy, xre->window, ClientContext, (XPointer *) &c);

	if (c) {
		Bool dockapp_ready = False;
		Bool dockapp_waits = False;

		if (xre->window == c->wind.window) {
			if (options.debug)
				fprintf(stderr, "    --> wind.window 0x%08lx reparented\n", c->wind.window);
			if (xre->parent == c->wind.root) {
				if (options.debug)
					fprintf(stderr, "    --> wind.window 0x%08lx reparented to root\n", c->wind.window);
				c->wind.parent = xre->parent;
				if (c->wind.withdrawing) {
					c->wind.withdrawing = False;
					c->wind.needsremap = True;
					if (options.debug)
						fprintf(stderr, "    --> ACQUIRED: wind.window 0x%08lx\n", c->wind.window);
					dockapp_ready = True;
					if (c->icon.withdrawing) {
						if (options.debug)
							fprintf(stderr, "    --> WAITING: icon.window 0x%08lx\n", c->icon.window);
						dockapp_ready = False;
						dockapp_waits = True;
					}
				}
			} else {
				if (options.debug)
					fprintf(stderr, "    --> wind.window 0x%08lx reparented to 0x%08lx\n", c->wind.window, xre->parent);
				c->wind.parent = xre->parent;
			}
		} else if (xre->window == c->icon.window) {
			if (options.debug)
				fprintf(stderr, "    --> icon.window 0x%08lx reparented\n", c->icon.window);
			if (xre->parent == c->icon.root || xre->parent == c->wind.window) {
				if (options.debug)
					fprintf(stderr, "    --> icon.window 0x%08lx reparented to %s\n", c->icon.window,
						(xre->parent == c->icon.root) ? "root" : "owner");
				c->icon.parent = xre->parent;
				if (c->icon.withdrawing) {
					c->icon.withdrawing = False;
					c->icon.needsremap = True;
					if (options.debug)
						fprintf(stderr, "    --> RESTORED: icon.window 0x%08lx\n", c->icon.window);
					dockapp_ready = True;
					if (c->wind.withdrawing) {
						if (options.debug)
							fprintf(stderr, "    --> WAITING: wind.window 0x%08lx\n", c->wind.window);
						dockapp_ready = False;
						dockapp_waits = True;
					}
				}
			} else {
				if (options.debug)
					fprintf(stderr, "    --> icon.window 0x%08lx reparented to 0x%08lx\n", c->icon.window, xre->parent);
				c->icon.parent = xre->parent;
			}
		} else {
			fprintf(stderr, "    --> ERROR: unknown window 0x%08lx: wind=0x%08lx, icon=0x%08lx\n", xre->window, c->wind.window, c->icon.window);
		}
		if (dockapp_waits)
			return;
		if (dockapp_ready)
			swallow(c);
	} else {
		if (options.debug)
			fprintf(stderr, "    --> ReparentNotify: window 0x%08lx is unknown window\n", xre->window);
		if (xre->parent == root) {
			if (options.debug)
				fprintf(stderr, "    --> ReparentNotify: window 0x%08lx reparented to root\n", xre->window);
		} else {
			if (options.debug)
				fprintf(stderr, "    --> ReparentNotify: window 0x%08lx reparented to 0x%08lx\n", xre->window, xre->parent);
		}
		if (options.debug)
			fprintf(stderr, "    --> ReparentNotify: CALLING TEST WINDOW: window = 0x%08lx\n", xre->window);
		test_window(xre->window);
	}
#endif
}

void
event_handler_UnmapNotify(XEvent *xev)
{
	if (options.debug) {
		fprintf(stderr, "==> UnmapNotify:\n");
		fprintf(stderr, "    --> event = 0x%08lx\n", xev->xunmap.event);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xunmap.window);
		fprintf(stderr, "    --> from-configure = %s\n", xev->xunmap.from_configure ? "True" : "False");
		fprintf(stderr, "<== UnmapNotify:\n");
	}
	return;
}

void
event_handler_MapNotify(XEvent *xev)
{
	Client *c;

	if (options.debug) {
		fprintf(stderr, "==> MapNotify:\n");
		fprintf(stderr, "    --> event = 0x%08lx\n", xev->xmap.event);
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xmap.window);
		fprintf(stderr, "    --> override-redirect = %s\n", xev->xmap.override_redirect ?  "True" : "False");
		fprintf(stderr, "<== MapNotify:\n");
	}
	if (shutting_down)
		return;
	if (XFindContext(dpy, xev->xany.window, ClientContext, (XPointer *) &c)) {
		if (options.debug)
			fprintf(stderr, "    --> MapNotify: CALLING TEST WINDOW: window = 0x%08lx\n", xev->xmap.window);
		test_window(xev->xmap.window);
	}
	return;
}

void
event_handler_PropertyNotify(XEvent *xev)
{
	if (options.debug) {
		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n", (xev->xproperty.state == PropertyNewValue) ?  "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
	}
	if (xev->xproperty.atom == _XA_XDE_THEME_NAME && xev->xproperty.state == PropertyNewValue)
		reparse();
	return;
}

void
event_handler_ClientMessage(XEvent *xev)
{
	if (options.debug) {
		fprintf(stderr, "==> ClientMessage:\n");
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
		fprintf(stderr, "<== ClientMessage:\n");
	}
	if (xev->xclient.message_type == _XA_GTK_READ_RCFILES)
		reparse();
	return;
}

void
handle_event(XEvent *xev)
{
	switch (xev->type) {
	case CreateNotify:
		event_handler_CreateNotify(xev);
		break;
	case DestroyNotify:
		event_handler_DestroyNotify(xev);
		break;
	case ReparentNotify:
		event_handler_ReparentNotify(xev);
		break;
	case UnmapNotify:
		event_handler_UnmapNotify(xev);
		break;
	case MapNotify:
		event_handler_MapNotify(xev);
		break;
	case PropertyNotify:
		event_handler_PropertyNotify(xev);
		break;
	case ClientMessage:
		event_handler_ClientMessage(xev);
		break;
	}
}

void
on_window_opened(WnckScreen *wscreen, WnckWindow *win, gpointer data)
{
}

void
handle_events()
{
	XEvent ev;

	XSync(dpy, False);
	while (XPending(dpy) && !shutting_down) {
		XNextEvent(dpy, &ev);
		handle_event(&ev);
	}
}

gboolean
on_watch(GIOChannel * chan, GIOCondition cond, gpointer data)
{
	if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR)) {
		fprintf(stderr, "ERROR: poll failed: %s %s %s\n",
			(cond & G_IO_NVAL) ? "NVAL" : "", (cond & G_IO_HUP) ? "HUP" : "",
			(cond & G_IO_ERR) ? "ERR" : "");
		exit(EXIT_FAILURE);
	} else if (cond & (G_IO_IN | G_IO_PRI)) {
		handle_events();
	}
	return TRUE; /* keep event source */
}

void
on_int_signal(int signum)
{
	shutting_down = True;
	gtk_main_quit();
}

void
on_hup_signal(int signum)
{
	shutting_down = True;
	gtk_main_quit();
}

void
on_term_signal(int signum)
{
	shutting_down = True;
	gtk_main_quit();
}

void
on_quit_signal(int signum)
{
	shutting_down = True;
	gtk_main_quit();
}

void
reparse()
{
	XTextProperty xtp = { NULL, };
	char **list = NULL;
	int strings = 0;

	gtk_rc_reparse_all();
	if (XGetTextProperty(dpy, root, &xtp, _XA_XDE_THEME_NAME) == Success) {
		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				char *rc_string;
				int len;

				len = 17 + strlen(list[0]) + 2;
				rc_string = calloc(len, sizeof(*rc_string));
				strncpy(rc_string, "gtk-theme-name=\"", len);
				strncat(rc_string, list[0], len);
				strncat(rc_string, "\"", len);
				gtk_rc_parse_string(rc_string);
				free(rc_string);
			}
			if (list)
				XFreeStringList(list);
		}
		if (xtp.value)
			XFree(xtp.value);
	}

#if 0
	Client *c;

	// gtk_widget_reset_rc_styles(GTK_WIDGET(dock.dock));	// doesn't help

	// gtk_widget_queue_clear(GTK_WIDGET(dock.dock));	// doesn't help
	relax();
	XClearArea(dpy, dock.dwin, 0, 0, 0, 0, True);
	XSync(dpy, False);

	for (c = clients; c; c = c->next) {
		//if (c->ebox) {
		//	gtk_widget_queue_clear(GTK_WIDGET(c->ebox));
		//	relax();
		//}
		if (c->plugged) {
			relax();
			XClearArea(dpy, c->plugged, 0, 0, 0, 0, True);
			XSync(dpy, False);
		}
	}
#else
	XClearArea(dpy, root, dock.x, dock.y, dock.w, dock.h, True);
#endif
}

void
event_handler(GdkEvent * event, gpointer data)
{
	if (event->type == GDK_CLIENT_EVENT) {
		GdkEventClient *ev = (typeof(ev)) event;

		if (ev->message_type == _GA_GTK_READ_RCFILES)
			reparse();
	} else if (event->type == GDK_PROPERTY_NOTIFY) {
		GdkEventProperty *ev = (typeof(ev)) event;

		if (ev->atom == _GA_XDE_THEME_NAME)
			reparse();
	}
	gtk_main_do_event(event);
}

int
runit(int argc, char *argv[])
{
	int xfd;
	GIOChannel *chan;
	gint srce;

	{
		const char *home;
		char *file;
		int len;

		home = getenv("HOME");
		len = 15 + strlen(home) + 2;
		file = calloc(len, sizeof(*file));
		strncpy(file, home, len);
		strncat(file, "/.gtkrc-2.0.xde", len);
		gtk_rc_add_default_file(file);
		free(file);
	}

	gtk_init(&argc, &argv);

	signal(SIGINT, on_int_signal);
	signal(SIGHUP, on_hup_signal);
	signal(SIGTERM, on_term_signal);
	signal(SIGQUIT, on_quit_signal);

	ClientContext = XUniqueContext();

#if 1
	if (!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "%s: %s\n", argv[0], "cannot open display");
		exit(127);
	}

	xfd = ConnectionNumber(dpy);
	chan = g_io_channel_unix_new(xfd);
	srce = g_io_add_watch(chan, G_IO_IN|G_IO_ERR|G_IO_HUP|G_IO_PRI, on_watch, (gpointer) 0);
	(void) srce;

	XSetErrorHandler(handler);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
#else
	dpy = gdk_x11_get_default_xdisplay();
	screen = gdk_x11_get_default_screen();
	root = gdk_x11_get_default_root_window();
#endif
	_XA_XDE_THEME_NAME = XInternAtom(dpy, "_XDE_THEME_NAME", False);
	_XA_GTK_READ_RCFILES = XInternAtom(dpy, "_GTK_READ_RCFILES", False);

#if 0
	_GA_XDE_THEME_NAME = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_GA_GTK_READ_RCFILE = gdk_atom_intern_static_string("_GTK_READ_RCFILES");

	{
		GdkEventMask mask;

		gmgr = gdk_display_manager_get();
		gdpy = gdk_display_manager_get_default_display(gmgr);
		gscr = gdk_display_get_default_screen(gdpy);
		groot = gdk_screen_get_root_window(gscr);
		mask = gdk_window_get_events(groot);

		mask |= GDK_PROPERTY_CHANGE_MASK|GDK_STRUCTURE_MASK|GDK_SUBSTRUCTURE_MASK;
		gdk_window_set_events(groot, mask);
	}
	gdk_event_handler_set(event_handler, (gpointer)NULL, NULL);
#endif
	XSelectInput(dpy, root, PropertyChangeMask | StructureNotifyMask | SubstructureNotifyMask);
	dock.save = XCreateSimpleWindow(dpy, root, -1, -1, 1, 1, 0,
			BlackPixel(dpy, screen), BlackPixel(dpy, screen));

	wnck = wnck_screen_get(screen);
	g_signal_connect(G_OBJECT(wnck), "window_opened",
			 G_CALLBACK(on_window_opened), NULL);

	create_dock();

	handle_events();
	relax();
	gtk_main();

	return (0);
}


int
main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	while (1) {
		int c, val;
		char *endptr = NULL;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"debug",	optional_argument,	NULL, 'D'},
			{"verbose",	optional_argument,	NULL, 'v'},
			{"help",	no_argument,		NULL, 'h'},
			{"version",	no_argument,		NULL, 'V'},
			{"copying",	no_argument,		NULL, 'C'},
			{"?",		no_argument,		NULL, 'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "D::v::hVCH?", long_options,
				     &option_index);
#else				/* defined _GNU_SOURCE */
		c = getopt(argc, argv, "DvhVC?");
#endif				/* defined _GNU_SOURCE */
		if (c == -1) {
			if (options.debug)
				fprintf(stderr, "%s: done options processing\n", argv[0]);
			break;
		}
		switch (c) {
		case 0:
			goto bad_usage;

		case 'D':	/* -D, --debug [level] */
			if (options.debug)
				fprintf(stderr, "%s: increasing debug verbosity\n",
					argv[0]);
			if (optarg == NULL) {
				options.debug++;
			} else {
				if ((val = strtol(optarg, &endptr, 0)) < 0)
					goto bad_option;
				if (endptr && *endptr)
					goto bad_option;
				options.debug = val;
			}
			break;
		case 'v':	/* -v, --verbose [level] */
			if (options.debug)
				fprintf(stderr, "%s: increasing output verbosity\n",
					argv[0]);
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
			if (options.debug)
				fprintf(stderr, "%s: printing help message\n", argv[0]);
			help(argc, argv);
			exit(0);
		case 'V':	/* -V, --version */
			if (options.debug)
				fprintf(stderr, "%s: printing version message\n",
					argv[0]);
			version(argc, argv);
			exit(0);
		case 'C':	/* -C, --copying */
			if (options.debug)
				fprintf(stderr, "%s: printing copying message\n",
					argv[0]);
			copying(argc, argv);
			exit(0);
		case '?':
		default:
		      bad_option:
			optind--;
			goto bad_nonopt;
		      bad_nonopt:
			if (options.output || options.debug) {
				if (optind < argc) {
					fprintf(stderr, "%s: syntax error near '",
						argv[0]);
					while (optind < argc) {
						fprintf(stderr, "%s", argv[optind++]);
						fprintf(stderr, "%s", (optind < argc) ? " " : "");
					}
					fprintf(stderr, "'\n");
				} else {
					fprintf(stderr, "%s: missing option or argument",
						argv[0]);
					fprintf(stderr, "\n");
				}
				fflush(stderr);
			      bad_usage:
				usage(argc, argv);
			}
			exit(2);
		}
	}
	if (options.debug) {
		fprintf(stderr, "%s: option index = %d\n", argv[0], optind);
		fprintf(stderr, "%s: option count = %d\n", argv[0], argc);
	}
	if (optind < argc) {
		fprintf(stderr, "%s: excess non-options arguments near '", argv[0]);
		while (optind < argc) {
			fprintf(stderr, "%s", argv[optind++]);
			fprintf(stderr, "%s", (optind < argc) ? " " : "");
		}
		fprintf(stderr, "'\n");
		usage(argc, argv);
		exit(2);
	}
	exit(runit(argc, argv));
}

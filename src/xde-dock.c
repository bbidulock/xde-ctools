/*****************************************************************************

 Copyright (c) 2008-2014  Monavacon Limited <http://www.monavacon.com/>
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

#include "xde-dock.h"

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

const char *program = NAME;

Display *dpy;
Window root;
Window save;
GtkWidget *dock;
GtkWidget *vbox;
int napps;
WnckScreen *wnck;
int screen;
Bool shutting_down = False;

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
} Client;

Client *clients;

XContext ClientContext;

struct Options {
	int debug;
	int output;
	DockPosition position;
	DockDirection direction;
} options = {
	.debug = 0,
	.output = 1,
	.position = DockPositionEast,
	.direction = DockDirectionVertical,
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
Copyright (c) 2008-2014  Monavacon Limited <http://www.monavacon.com/>\n\
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
Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014  Monavacon Limited.\n\
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
	gint x, y;
	Screen *scrn = ScreenOfDisplay(dpy, screen);
	gint w = scrn->width;
	gint h = scrn->height;
	gint d = 0;

	if (napps <= 0) {
		gtk_widget_hide(GTK_WIDGET(dock));
		return;
	}
	pos = options.position;
	dir = options.direction;
	x = y = 0;
	switch (pos) {
	case DockPositionNorth:
		switch (dir) {
		case DockDirectionVertical:
			strut.top = napps * 64;
			strut.top_start_x = (w - 64) / 2;
			strut.top_end_x = (w + 64) / 2;
			break;
		case DockDirectionHorizontal:
			strut.top = 64;
			strut.top_start_x = (w - (napps * 64)) / 2;
			strut.top_end_x = (w + (napps * 64)) / 2;
			break;
		}
		x = strut.top_start_x;
		y = 0;
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
		x = 0;
		y = 0;
		break;
	case DockPositionWest:
		switch (dir) {
		case DockDirectionVertical:
			strut.left = 64;
			strut.left_start_y = (h - (napps * 64)) / 2;
			strut.left_end_y = (h + (napps * 64)) / 2;
			break;
		case DockDirectionHorizontal:
			strut.left = napps * 64;
			strut.left_start_y = (h - 64) / 2;
			strut.left_end_y = (h + 64) / 2;
			break;
		}
		x = 0;
		y = strut.left_start_y;
		break;
	case DockPositionSouthWest:
		switch (dir) {
		case DockDirectionVertical:
			strut.left = 64;
			strut.left_start_y = h - napps * 64;
			strut.left_end_y = h;
			y = strut.left_start_y;
			break;
		case DockDirectionHorizontal:
			strut.bottom = 64;
			strut.bottom_start_x = 0;
			strut.bottom_end_x = napps * 64;
			y = h - strut.bottom;
			break;
		}
		x = 0;
		break;
	case DockPositionSouth:
		switch (dir) {
		case DockDirectionVertical:
			strut.bottom = napps * 64;
			strut.bottom_start_x = (h - 64) / 2;
			strut.bottom_end_x = (h + 64) / 2;
			break;
		case DockDirectionHorizontal:
			strut.bottom = 64;
			strut.bottom_start_x = (h - (napps * 64)) / 2;
			strut.bottom_end_x = (h + (napps * 64)) / 2;
			break;
		}
		x = strut.bottom_start_x;
		y = h - strut.bottom;
		break;
	case DockPositionSouthEast:
		switch (dir) {
		case DockDirectionVertical:
			strut.right = 64;
			strut.right_start_y = h - napps * 64;
			strut.right_end_y = h;
			x = w - strut.right;
			y = strut.right_start_y;
			break;
		case DockDirectionHorizontal:
			strut.bottom = 64;
			strut.bottom_start_x = w - napps * 64;
			strut.bottom_end_x = w;
			x = strut.bottom_start_x;
			y = h = strut.bottom;
			break;
		}
		break;
	case DockPositionEast:
		switch (dir) {
		case DockDirectionVertical:
			strut.right = 64;
			strut.right_start_y = (h - (napps * 64)) / 2;
			strut.right_end_y = (h + (napps * 64)) / 2;
			break;
		case DockDirectionHorizontal:
			strut.right = napps * 64;
			strut.right_start_y = (h - 64) / 2;
			strut.right_end_y = (h + 64) / 2;
			break;
		}
		x = w - strut.right;
		y = strut.right_start_y;
		break;
	case DockPositionNorthEast:
		switch (dir) {
		case DockDirectionVertical:
			strut.right = 64;
			strut.right_start_y = 0;
			strut.right_end_y = napps * 64;
			x = w - strut.right;
			y = strut.right_start_y;
			break;
		case DockDirectionHorizontal:
			strut.top = 64;
			strut.top_start_x = w - (napps * 64);
			strut.top_end_x = w;
			x = strut.top_start_x;
			y = 0;
			break;
		}
		break;
	}
	gtk_widget_realize(GTK_WIDGET(dock));
	if (options.debug)
		fprintf(stderr, "--> MOVING dock to (%d,%d)\n", x, y);

	GdkWindow *gwin = gtk_widget_get_window(GTK_WIDGET(dock));

	gtk_window_move(GTK_WINDOW(dock), x, y);
	gdk_window_move(GDK_WINDOW(gwin), x, y);
	gdk_window_get_geometry(GDK_WINDOW(gwin), &x, &y, &w, &h, &d);
	if (options.debug) {
		fprintf(stderr, "--> GEOMETRY now (%dx%d+%d+%d:%d)\n", w, h, x, y, d);
		fprintf(stderr, "--> _NET_WM_STRUT set (%ld,%ld,%ld,%ld)\n",
				strut.left, strut.right, strut.top, strut.bottom);
		fprintf(stderr, "--> _NET_WM_STRUT_PARTIAL set (%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld)\n",
				strut.left, strut.right, strut.top, strut.bottom,
				strut.left_start_y, strut.left_end_y,
				strut.right_start_y, strut.right_end_y,
				strut.top_start_x, strut.top_end_x,
				strut.bottom_start_x, strut.bottom_end_x);
	}
	Window dwin = GDK_WINDOW_XID(gwin);

	XChangeProperty(dpy, dwin, XInternAtom(dpy, "_NET_WM_STRUT", False),
			XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&strut, 4);
	XChangeProperty(dpy, dwin, XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False),
			XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&strut, 12);
}

gboolean
on_plug_removed(GtkSocket *sock, gpointer data)
{
	Client *client = (typeof(client)) data;

	if (options.debug)
		fprintf(stderr, "*** PLUG REMOVED: wind.window = 0x%lx (%ld) icon.window = 0x%ld (%ld)\n",
				client->wind.window, client->wind.window,
				client->icon.window, client->icon.window);
	if (client->ebox) {
		gtk_widget_destroy(GTK_WIDGET(client->ebox));
		client->ebox = NULL;
		napps -= 1;
	}
	dock_rearrange();
	return FALSE;
}

void
swallow(Client * client)
{
	Window wind = client->wind.window;
	Window icon = client->icon.window;
	Window used;

	if (client->swallowed)
		return;
	XSync(dpy, False);
	if (options.debug)
		fprintf(stderr,
			"==> SWALLOWING window 0x%lx (%ld) with icon 0x%lx (%ld)!\n",
			wind, wind, icon, icon);
	if (XGetClassHint(dpy, wind, &client->ch)) {
		if (options.debug)
			fprintf(stderr, "--> window 0x%lx (%ld) '%s', '%s'\n",
				wind, wind, client->ch.res_name, client->ch.res_class);
	}
	if (XGetCommand(dpy, client->wind.window, &client->argv, &client->argc)) {
		if (options.debug) {
			int i;

			fprintf(stderr, "--> window 0x%lx (%ld) '", wind, wind);
			for (i = 0; i < client->argc; i++)
				fprintf(stderr, "%s%s", client->argv[i],
					(client->argc == i + 1) ? "" : "', '");
			fprintf(stderr, "%s\n", "'");
		}
	}
	if (options.debug) {
		Window wroot, parent, *children;
		unsigned int i, nchild;

		if (wind && XQueryTree(dpy, wind, &wroot, &parent, &children, &nchild)) {
			fprintf(stderr,
				"--> wind.window 0x%lx (%ld) has root 0x%lx (%ld)\n",
				wind, wind, wroot, wroot);
			fprintf(stderr,
				"--> wind.window 0x%lx (%ld) has parent 0x%lx (%ld)\n",
				wind, wind, parent, parent);
			for (i = 0; i < nchild; i++)
				fprintf(stderr,
					"--> wind.window 0x%lx (%ld) has child 0x%lx (%ld)\n",
					wind, wind, children[i], children[i]);
			if (children)
				XFree(children);
		}
		if (icon && XQueryTree(dpy, icon, &wroot, &parent, &children, &nchild)) {
			fprintf(stderr,
				"--> icon.window 0x%lx (%ld) has root 0x%lx (%ld)\n",
				icon, icon, wroot, wroot);
			fprintf(stderr,
				"--> icon.window 0x%lx (%ld) has parent 0x%lx (%ld)\n",
				icon, icon, parent, parent);
			for (i = 0; i < nchild; i++)
				fprintf(stderr,
					"--> icon.window 0x%lx (%ld) has child 0x%lx (%ld)\n",
					icon, icon, children[i], children[i]);
			if (children)
				XFree(children);
		}
	}
	used = wind;
	if (icon) {
		Window wroot, parent, *children;
		unsigned int nchild;

		if (XQueryTree(dpy, icon, &wroot, &parent, &children, &nchild)) {
			if (parent == wind) {
				/* When the icon.window is a child of its toplevel
				   window, we ant to reparent the toplevel and not the
				   child. */
				if (options.debug)
					fprintf(stderr,
						"::: icon.window 0x%lx (%ld) has owner 0x%lx (%ld) as parent\n",
						icon, icon, parent, parent);
				used = wind;
			} else if (parent == wroot) {
				/* When the icon.window is its own toplevel, we might
				   have been withdrawing it and we just haven't received
				   the event yet... */
				if (client->icon.withdrawing) {
					/* If we are an icon.window that is its own
					   toplevel window, wait for it to appear on its
					   own. */
					if (options.debug)
						fprintf(stderr,
							"!!! icon.window 0x%lx (%ld) is being withdrawn\n",
							icon, icon);
					return;
				}
			} else {
				/* Otherwise make the toplevel icon window the window to
				   reparent.  */
				if (options.debug)
					fprintf(stderr,
						"::: icon.window 0x%lx (%ld) is a child of window 0x%lx (%ld)\n",
						icon, icon, parent, parent);
				used = icon;
			}
			if (children)
				XFree(children);
		}
	}
	client->wind.mapped = (used == wind) ? True : False;
	client->icon.mapped = (used == icon) ? True : False;

	/* Now we can actually swallow the dock app. */

	int tpad, bpad, lpad, rpad;
	int x, y;
	unsigned int width, height, border, depth;
	Window wroot;

	if (!XGetGeometry(dpy, used, &wroot, &x, &y, &width, &height, &border, &depth)) {
		fprintf(stderr, "ERROR: window 0x%lx (%ld) cannot get geometry\n", used,
			used);
		return;
	}

	tpad = (64 - height) / 2;
	bpad = 64 - height - tpad;
	lpad = (64 - width) / 2;
	rpad = 64 - width - lpad;

	GtkWidget *b, *a, *s;

	b = client->ebox = gtk_event_box_new();

	gtk_widget_set_size_request(GTK_WIDGET(b), 64, 64);
	a = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(a), tpad, bpad, lpad, rpad);
	gtk_container_add(GTK_CONTAINER(b), GTK_WIDGET(a));
	s = gtk_socket_new();
	g_signal_connect(G_OBJECT(s), "plug_removed",
			 G_CALLBACK(on_plug_removed), (gpointer)client);
	gtk_container_add(GTK_CONTAINER(a), GTK_WIDGET(s));
	if (options.debug)
		fprintf(stderr, "--> PACKING SOCKET for window 0x%lx (%ld) into dock\n",
			used, used);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(b), FALSE, FALSE, 0);
	napps += 1;
	dock_rearrange();
	gtk_widget_show_all(GTK_WIDGET(b));
	gtk_widget_show_all(GTK_WIDGET(a));
	gtk_widget_show_all(GTK_WIDGET(s));
	gtk_widget_show_all(GTK_WIDGET(dock));
	if (options.debug)
		fprintf(stderr, "--> ADDING window 0x%ld (%ld) into socket\n", used,
			used);
	/* We might also use gtk_socket_steal() here.  I don't quite know what the
	   difference is as both appear to behave the same; however, there is no way to
	   add the window to Gtk2's save set, so we need to figure out how to do that.
	   Perhaps stealing has this effect. */
	client->wind.reparented = (used == wind) ? True : False;
	client->icon.reparented = (used == icon) ? True : False;

	if (0) {
		gtk_socket_steal(GTK_SOCKET(s), used);
	} else {
		gtk_socket_add_id(GTK_SOCKET(s), used);
	}
	client->swallowed = True;
}

Bool
withdraw_window(ClientWindow * cwin)
{
	Window *children = NULL;
	unsigned int nchild = 0;
	Bool retval = False;

	if (XQueryTree(dpy, cwin->window, &cwin->root, &cwin->parent, &children, &nchild)) {
		if (options.debug)
			fprintf(stderr,
				"::: window 0x%lx (%ld) root 0x%lx (%ld) parent 0x%lx (%ld)\n",
				cwin->window, cwin->window, cwin->root, cwin->root,
				cwin->parent, cwin->parent);
		if (cwin->root && cwin->parent && (cwin->root != cwin->parent)) {
			if (options.debug)
				fprintf(stderr,
					"==> DEPARENTING: window 0x%lx (%ld) from parent 0x%lx (%ld)\n",
					cwin->window, cwin->window, cwin->parent,
					cwin->parent);
			cwin->withdrawing = True;
			retval = True;
		}
		XWithdrawWindow(dpy, cwin->window, screen);
		XSync(dpy, False);
		if (children)
			XFree(children);
	}
	return retval;
}

Bool
test_window(Window win)
{
	Window icon;
	XWMHints *wmh;
	Client *client;
	Bool wait_wind, wait_icon = False;

	if (shutting_down)
		return False;
	if (options.debug)
		fprintf(stderr, "--> TESTING WINDOW: window = 0x%lx (%ld)\n", win, win);
	if (!(wmh = XGetWMHints(dpy, win)))
		return False;
	if (!(wmh->flags & StateHint) || wmh->initial_state != WithdrawnState)
		return False;
	/* some dockapps use their main window as an icon window */
	if (!(wmh->flags & IconWindowHint))
		icon = wmh->icon_window;
	else
		icon = None;
	/* some dockapp icon windows point to themselves... */
	if (icon == win)
		icon = None;
	if (options.debug)
		fprintf(stderr,
			"==> Should swallow window 0x%lx (%ld) with icon 0x%lx (%ld)!\n",
			win, win, icon, icon);

	client = calloc(1, sizeof(*client));
	client->next = clients;
	clients = client;
	client->wind.window = win;
	client->wind.withdrawing = False;
	client->wind.mapped = False;
	XSaveContext(dpy, win, ClientContext, (XPointer) client);
	client->icon.window = icon;
	client->icon.withdrawing = False;
	client->icon.mapped = False;
	XSaveContext(dpy, icon, ClientContext, (XPointer) client);
	client->swallowed = False;
	client->ch.res_name = NULL;
	client->ch.res_class = NULL;
	client->argv = NULL;
	client->argc = 0;

	wait_wind = withdraw_window(&client->wind);

	/* A couple of problems here: some dockapps make the icon window a child of their 
	   toplevel window, presumably so that WM's will map the child with the toplevel
	   if it doesn't understand windows being mapped in the withdrawn state.  When
	   the icon_window is a child of the toplevel, we do not want to steal it away
	   from its parent because it will be reparented to root on the way out.
	   Therefore, when the icon_window is a child of the toplevel, we do not want to
	   withdraw it and we want to reparent only the toplevel. */

	if (icon) {
		Window wroot, parent, *children;
		unsigned int nchild;

		if (XQueryTree(dpy, icon, &wroot, &parent, &children, &nchild)) {
			if (children)
				XFree(children);
			if (options.debug)
				fprintf(stderr,
					"::: icon_window 0x%lx (%ld) root 0x%lx (%ld) parent 0x%lx (%ld)\n",
					win, win, wroot, wroot, parent, parent);
			if (parent == win) {
				if (options.debug)
					fprintf(stderr,
						"::: icon_window 0x%lx (%ld) is a child of toplevel 0x%lx (%ld)\n",
						icon, icon, win, win);
			} else if (parent == wroot) {
				if (options.debug)
					fprintf(stderr,
						"::: icon_window 0x%lx (%ld) is its own toplevel\n",
						icon, icon);
				withdraw_window(&client->icon);
			}

		}
	}
	if (!wait_wind && !wait_icon)
		swallow(client);
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

void
create_dock()
{
	dock = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(dock), GDK_WINDOW_TYPE_HINT_DOCK);
	gtk_window_set_default_size(GTK_WINDOW(dock), 64, -1);
	gtk_window_set_decorated(GTK_WINDOW(dock), FALSE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(dock), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dock), TRUE);
	gtk_window_stick(GTK_WINDOW(dock));
	gtk_window_set_deletable(GTK_WINDOW(dock), FALSE);
	gtk_window_set_focus_on_map(GTK_WINDOW(dock), FALSE);
	gtk_window_set_has_frame(GTK_WINDOW(dock), FALSE);
	gtk_window_set_keep_above(GTK_WINDOW(dock), TRUE);
	gtk_window_set_resizable(GTK_WINDOW(dock), FALSE);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_set_size_request(GTK_WIDGET(vbox), 64, -1);
	gtk_container_add(GTK_CONTAINER(dock), GTK_WIDGET(vbox));
	gtk_widget_realize(GTK_WIDGET(dock));
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
		fprintf(stderr, "X error %s(0x%lx): %s\n", req, xev->resourceid, msg);
	}
	return (0);
}

void
event_handler_CreateNotify(XEvent  *xev)
{
	return;
}

void
event_handler_DestroyNotify(XEvent *xev)
{
	return;
}

void
event_handler_ReparentNotify(XEvent *xev)
{
	XReparentEvent *xre = (typeof(xre)) xev;
	Client *c = NULL;

	if (options.debug)
		fprintf(stderr,
			"::: ReparentNotify: event=0x%lx window=0x%lx parent=0x%lx\n",
			xre->event, xre->window, xre->parent);
	if (xre->override_redirect)
		return;
	if (shutting_down)
		return;

	XFindContext(dpy, xre->window, ClientContext, (XPointer *) &c);

	if (xre->parent == root) {
		Window win = None;

		/* window was parented back to the root window */
		if (c) {
			if (xre->window == c->wind.window) {
				if (options.debug)
					fprintf(stderr,
						"::: RESTORED dockapp 0x%lx (%ld) from parent 0x%lx (%ld)\n",
						xre->window, xre->window, c->wind.parent,
						c->wind.parent);
				if (c->wind.withdrawing) {
					c->wind.needsremap = True;
					if (options.debug)
						fprintf(stderr,
							"::: ACQUIRED: dockapp = 0x%lx (%ld)\n",
							c->wind.window, c->wind.window);
					win = c->wind.window;
					if (c->icon.withdrawing) {
						if (options.debug)
							fprintf(stderr,
								"... WAITING: iconwin = 0x%lx (%ld)\n",
								c->icon.window,
								c->icon.window);
						win = None;
					}

				}
			} else if (xre->window == c->icon.window) {
				if (options.debug)
					fprintf(stderr,
						"::: RESTORED iconwin 0x%lx (%ld) from parent 0x%lx (%ld)\n",
						xre->window, xre->window, c->icon.parent,
						c->icon.parent);
				if (c->icon.withdrawing) {
					c->icon.needsremap = True;
					if (options.debug)
						fprintf(stderr,
							"::: ACQUIRED: iconwin = 0x%lx (%ld)\n",
							c->icon.window, c->icon.window);
					win = c->icon.window;
					if (c->wind.withdrawing) {
						if (options.debug)
							fprintf(stderr,
								"... WAITING: dockapp = 0x%lx (%ld)\n",
								c->wind.window,
								c->wind.window);
						win = None;
					}
				}
			}
		}
		if (win) {
			if (options.debug)
				fprintf(stderr,
					"==> REPARENTING: window 0x%lx (%ld) to internal 0x%ld (%ld)\n",
					c->wind.window, c->wind.window, save, save);
			XAddToSaveSet(dpy, win);
			XReparentWindow(dpy, win, save, 0, 0);
			XMapWindow(dpy, win);
			XSync(dpy, False);
			swallow(c);
			if (1) {
				XDestroyWindowEvent xev;

				if (c->wind.window) {
					if (options.debug)
						fprintf(stderr,
							"*** DESTROYING: dockapp 0x%lx (%ld)\n",
							c->wind.window, c->wind.window);
					xev.type = DestroyNotify;
					xev.serial = 0;
					xev.send_event = False;
					xev.display = dpy;
					xev.event = root;
					xev.window = c->wind.window;
					XSendEvent(dpy, root, False, SubstructureNotifyMask
						   | SubstructureRedirectMask, (XEvent *)&xev);
				}
				if (c->icon.window) {
					if (options.debug)
						fprintf(stderr,
							"*** DESTROYING: iconwin 0x%lx (%ld)\n",
							c->icon.window, c->icon.window);
					xev.type = DestroyNotify;
					xev.serial = 0;
					xev.send_event = False;
					xev.display = dpy;
					xev.event = root;
					xev.window = c->icon.window;
					XSendEvent(dpy, root, False, SubstructureNotifyMask
						   | SubstructureRedirectMask, (XEvent *)&xev);
				}
				XSync(dpy, False);
			}
			/* Remap the window under the new parent so that it will map itself
			 * when reparented to root. */
			XMapWindow(dpy, win);
		}
	} else {
		/* Window was reparented away from the root window. */
		if (!c) {
			/* and it wasn't just us, test this window to see if it can be
			   ours */
			test_window(xre->window);
		}
	}
}

void
event_handler_UnmapNotify(XEvent *xev)
{
	return;
}

void
event_handler_MapNotify(XEvent *xev)
{
	Client *c;

	if (shutting_down)
		return;
	if (XFindContext(dpy, xev->xany.window, ClientContext, (XPointer *) &c))
		test_window(xev->xany.window);
	return;
}

void
event_handler_PropertyNotify(XEvent *xev)
{
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
	}
}

void
on_window_opened(WnckScreen *wscreen, WnckWindow *win, gpointer data)
{
}

gboolean
on_watch(GIOChannel * chan, GIOCondition cond, gpointer data)
{
	if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR)) {
		fprintf(stderr, "ERROR: poll failed: %s %s %s\n",
			(cond & G_IO_NVAL) ? "NVAL" : "", (cond & G_IO_HUP) ? "HUP" : "",
			(cond & G_IO_ERR) ? "ERR" : "");
		exit(EXIT_FAILURE);
	} else if (cond & (G_IO_IN)) {
		XEvent ev;

		while (XPending(dpy) && !shutting_down) {
			XNextEvent(dpy, &ev);
			handle_event(&ev);
		}
	}
	return TRUE; /* keep event source */
}

int
runit(int argc, char *argv[])
{
	int xfd;
	GIOChannel *chan;
	gint srce;

	gtk_init(&argc, &argv);

	ClientContext = XUniqueContext();

	if (!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "%s: %s\n", argv[0], "cannot open display");
		exit(127);
	}

	xfd = ConnectionNumber(dpy);
	chan = g_io_channel_unix_new(xfd);
	srce = g_io_add_watch(chan, G_IO_IN|G_IO_ERR|G_IO_HUP, on_watch, (gpointer) 0);
	(void) srce;

	XSetErrorHandler(handler);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	XSelectInput(dpy, root, PropertyChangeMask | SubstructureNotifyMask);
	save = XCreateSimpleWindow(dpy, root, -1, -1, 1, 1, 0,
			BlackPixel(dpy, screen), BlackPixel(dpy, screen));

	wnck = wnck_screen_get(screen);
	g_signal_connect(G_OBJECT(wnck), "window_opened",
			 G_CALLBACK(on_window_opened), NULL);

	create_dock();

	gtk_main();

	return (0);
}


int
main(int argc, char *argv[])
{
	while (1) {
		int c, val;

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
				if ((val = strtol(optarg, NULL, 0)) < 0)
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
			if ((val = strtol(optarg, NULL, 0)) < 0)
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

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

#include "xde-winmenu.h"

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

Atom _XA_XDE_THEME_NAME;
Atom _XA_GTK_READ_RCFILES;

typedef enum {
	UseWindowDefault,
	UseWindowActive,
	UseWindowFocused,
	UseWindowPointer,
	UseWindowSpecified,
	UseWindowSelect,
} UseWindow;

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
	UseWindow which;
	Window window;
	Command command;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.button = 0,
	.timestamp = CurrentTime,
	.which = UseWindowDefault,
	.window = None,
	.command = CommandDefault,
};

WnckScreen *
find_specified_screen(GdkDisplay *disp)
{
	WnckScreen *scrn = NULL;
	GdkWindow *win;
	GdkScreen *scr;
	int screen;

	if (!(win = gdk_x11_window_foreign_new_for_display(disp, options.window)))
		return (scrn);
	if (!(scr = gdk_window_get_screen(win))) {
		g_object_unref(G_OBJECT(win));
		return (scrn);
	}
	g_object_unref(G_OBJECT(win));
	screen = gdk_screen_get_number(scr);
	scrn = wnck_screen_get(screen);
	return (scrn);
}

/** @brief find the specified screen
  * 
  * Either specified with options.screen, or if the DISPLAY environment variable
  * specifies a screen, use that screen; otherwise, return NULL.
  */
WnckScreen *
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
WnckScreen *
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

WnckScreen *
find_pointer_screen(GdkDisplay *disp)
{
	WnckScreen *scrn = NULL;
	GdkScreen *screen = NULL;
	gdk_display_get_pointer(disp, &screen, NULL, NULL, NULL);
	if (screen)
		scrn = wnck_screen_get(gdk_screen_get_number(screen));
	return (scrn);
}

WnckScreen *
find_screen(GdkDisplay *disp)
{
	WnckScreen *scrn = NULL;

	if (options.which == UseWindowSpecified)
		return find_specified_screen(disp);
	if ((scrn = find_specific_screen(disp)))
		return (scrn);
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
	if (!scrn)
		scrn = wnck_screen_get_default();
	return (scrn);
}

WnckWindow *
find_specified_window(GdkDisplay *disp, WnckScreen *scrn)
{
	return wnck_window_get(options.window);
}

WnckWindow *
find_active_window(GdkDisplay *disp, WnckScreen *scrn)
{
	return wnck_screen_get_active_window(scrn);
}

WnckWindow *
find_focus_window(GdkDisplay *disp, WnckScreen * scrn)
{
	WnckWindow *wind = NULL;
	GList *windows, *w;

	if ((windows = wnck_screen_get_windows(scrn)))
		for (w = g_list_first(windows);
		     w && (!(wind = (WnckWindow *) w->data) ||
		     !wnck_window_is_focused(wind));
		     wind = NULL, w = g_list_next(w)) ;
	return (wind);
}

gboolean
is_over(Display *dpy, WnckWindow *wind, int px, int py)
{
	int x, y, w, h;
	XWindowAttributes xwa;
	Window win;

	if (!(win = wnck_window_get_xid(wind)))
		return FALSE;
	if (!XGetWindowAttributes(dpy, win, &xwa))
		return FALSE;
	if (xwa.map_state != IsViewable)
		return FALSE;
	wnck_window_get_geometry(wind, &x, &y, &w, &h);
	if (x <= px && px < x + w && y <= py && py < y + h)
		return TRUE;
	return FALSE;
}


WnckWindow *
find_pointer_window(GdkDisplay *disp, WnckScreen *scrn)
{
	WnckWindow *wind = NULL;
	GList *windows, *w;
	gint x, y;
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
	
	if ((windows = wnck_screen_get_windows(scrn))) {
		gdk_display_get_pointer(disp, NULL, &x, &y, NULL);
		for (w = g_list_last(windows);
		     w && (!(wind = (WnckWindow *)w->data) ||
		     !is_over(dpy, wind, x, y));
		     wind = NULL, w = g_list_previous(w)) ;
	}
	return (wind);
}

WnckWindow *
find_window(GdkDisplay *disp, WnckScreen *scrn)
{
	WnckWindow *wind = NULL;

	switch (options.which) {
	case UseWindowActive:
		if ((wind = find_active_window(disp, scrn)))
			return (wind);
		break;
	case UseWindowFocused:
		if ((wind = find_focus_window(disp, scrn)))
			return (wind);
		if ((wind = find_pointer_window(disp, scrn)))
			return (wind);
		break;
	case UseWindowPointer:
		if ((wind = find_pointer_window(disp, scrn)))
			return (wind);
		if ((wind = find_focus_window(disp, scrn)))
			return (wind);
		break;
	case UseWindowSpecified:
		if ((wind = find_specified_window(disp, scrn)))
			return (wind);
		break;
	default:
	case UseWindowSelect:
		/* FIXME */
		break;
	}
	return (wind);
}

static void
pos_at_mouse(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	GdkDisplay *disp = (typeof(disp)) user_data;
	*push_in = TRUE;
	gdk_display_get_pointer(disp, NULL, x, y, NULL);
}

static void
pos_on_wwind(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	int xpos, ypos, w, h;
	GtkRequisition req = { 0, 0 };

	WnckWindow *wind = (typeof(wind)) user_data;
	*push_in = TRUE;
	wnck_window_get_geometry(wind, &xpos, &ypos, &w, &h);
	xpos += w / 2;
	ypos += h / 2;
	gtk_widget_get_requisition(GTK_WIDGET(menu), &req);
	xpos -= req.width / 2;
	ypos -= req.height / 2;
	*x = xpos;
	*y = ypos;
}

void
on_selection_done(GtkMenuShell *menushell, gpointer user_data)
{
	gtk_main_quit();
}

void
pop_the_menu(int argc, char *argv[])
{
	GdkDisplay *disp;
	WnckScreen *scrn;
	WnckWindow *wind;
	GtkWidget *menu;

	if (!(disp = gdk_display_get_default())) {
		fprintf(stderr, "ERROR: cannot get default display\n");
		exit(EXIT_FAILURE);
	}
	if (!(scrn = find_screen(disp))) {
		fprintf(stderr, "ERROR: cannot find screen\n");
		exit(EXIT_FAILURE);
	}
	wnck_screen_force_update(scrn);
	if (!(wind = find_window(disp, scrn))) {
		fprintf(stderr, "ERROR: cannot find window\n");
		exit(EXIT_FAILURE);
	}
	if (!(menu = wnck_action_menu_new(wind))) {
		fprintf(stderr, "ERROR: cannot get menu\n");
		exit(EXIT_FAILURE);
	}
	if (options.button)
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, pos_at_mouse, disp, options.button, options.timestamp);
	else
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, pos_on_wwind, wind, options.button, options.timestamp);
	g_signal_connect(G_OBJECT(menu), "selection-done", G_CALLBACK(on_selection_done), NULL);
	gtk_main();
}

void
reparse(Display *dpy, Window root)
{
	XTextProperty xtp = { NULL, };
	char **list = NULL;
	int strings = 0;

	gtk_rc_reparse_all();
	if (XGetTextProperty(dpy, root, &xtp, _XA_XDE_THEME_NAME) == Success) {
		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				static const char *prefix = "gtk-theme-name=\"";
				static const char *suffix = "\"";
				char *rc_string;
				int len;

				len = strlen(prefix) + strlen(list[0]) + strlen(suffix) + 1;
				rc_string = calloc(len, sizeof(*rc_string));
				strncpy(rc_string, prefix, len);
				strncpy(rc_string, list[0], len);
				strncpy(rc_string, suffix, len);
				gtk_rc_parse_string(rc_string);
				free(rc_string);
			}
			if (list)
				XFreeStringList(list);
		}
		if (xtp.value)
			XFree(xtp.value);
	}
}

static GdkFilterReturn
event_handler_PropertyNotify(Display *dpy, XEvent *xev)
{
	if (options.debug) {
		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n", (xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
	}
	if (xev->xproperty.atom == _XA_XDE_THEME_NAME && xev->xproperty.state == PropertyNewValue) {
		reparse(dpy, xev->xproperty.window);
		return GDK_FILTER_REMOVE; /* event handled */
	}
	return GDK_FILTER_CONTINUE; /* event not handled */
}

static GdkFilterReturn
event_handler_ClientMessage(Display *dpy, XEvent *xev)
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
	if (xev->xclient.message_type == _XA_GTK_READ_RCFILES) {
		reparse(dpy, xev->xclient.window);
		return GDK_FILTER_REMOVE;	/* event handled */
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
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
	return GDK_FILTER_CONTINUE;	/* event not handled, continue processing */
}

static GdkFilterReturn
filter_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	Display *dpy = (typeof(dpy)) data;

	return handle_event(dpy, xev);
}

void
startup(int argc, char *argv[])
{
	static const char *suffix = "/.gtkrc-2.0.xde";
	const char *home;
	GdkAtom atom;
	GdkEventMask mask;
	GdkDisplay *disp;
	GdkScreen *scrn;
	GdkWindow *root;
	Display *dpy;
	char *file;
	int len, nscr;

	home = getenv("HOME") ? : ".";
	len = strlen(home) + strlen(suffix) + 1;
	file = calloc(len, sizeof(*file));
	strncpy(file, home, len);
	strncat(file, suffix, len);
	gtk_rc_add_default_file(file);
	free(file);

	gtk_init(&argc, &argv);

	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);

	if (options.screen >= 0 && options.screen >= nscr) {
		fprintf(stderr, "ERROR: bad screen specified: %d\n", options.screen);
		exit(EXIT_FAILURE);
	}

	dpy = GDK_DISPLAY_XDISPLAY(disp);
	gdk_window_add_filter(NULL, filter_handler, dpy);

	atom = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_XA_XDE_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_GTK_READ_RCFILES");
	_XA_GTK_READ_RCFILES = gdk_x11_atom_to_xatom_for_display(disp, atom);

	scrn = gdk_display_get_default_screen(disp);
	root = gdk_screen_get_root_window(scrn);
	mask = gdk_window_get_events(root);
	mask |= GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK | GDK_SUBSTRUCTURE_MASK;
	gdk_window_set_events(root, mask);

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

static const char *
show_window(Window win)
{
	static char window[64] = { 0, };

	if (!options.window)
		return ("None");
	snprintf(window, sizeof(window), "0x%lx", options.window);
	return (window);
}

static const char *
show_which(UseWindow which)
{
	switch (which) {
	case UseWindowDefault:
		return ("default");
	case UseWindowActive:
		return ("active");
	case UseWindowFocused:
		return ("focused");
	case UseWindowPointer:
		return ("pointer");
	case UseWindowSelect:
		return ("select");
	case UseWindowSpecified:
		return show_window(options.window);
	}
	return NULL;
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
   [-p, --popup]\n\
        popup the menu\n\
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
    -w, --which {active|focused|pointer|select|WINDOW}\n\
        specify the window for which to pop the menu [default: %8$s]\n\
	\"active\"  - the EWMH/NetWM active client\n\
	\"focused\" - the EWMH/NetWM focused client\n\
	\"pointer\" - the EWMH/NetWM client under the pointer\n\
	\"select\"  - the EWMH/NetWM client from selection\n\
	 WINDOW   - the EWMH/NetWM client XID to use\n\
    -x, --id WINDOW\n\
        specify the window to use by XID [default: %9$s]\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %2$d]\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %3$d]\n\
        this option may be repeated.\n\
", argv[0], options.debug, options.output, options.display, options.screen, options.button, options.timestamp, show_which(options.which), show_window(options.window));
}

void
set_defaults(void)
{
	const char *env;

	if ((env = getenv("DISPLAY")))
		options.display = strdup(env);
}

void
get_defaults(void)
{
	const char *p;
	int n;

	if (!options.display) {
		fprintf(stderr, "ERROR: No DISPLAY environment variable or --display option\n");
		exit(EXIT_FAILURE);
	}
	if (options.screen < 0 && (p = strrchr(options.display, '.')) && (n = strspn(++p, "0123456789")) && *(p + n) == '\0')
		options.screen = atoi(p);
	if (options.which == UseWindowDefault)
		options.which = (options.button) ? UseWindowPointer : UseWindowFocused;
	if (options.command == CommandDefault)
		options.command = CommandPopup;

}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;

	set_defaults();

	while (1) {
		int c, val, len;

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
			{"id",			required_argument,	NULL,	'x'},

			{"debug",		optional_argument,	NULL,	'D'},
			{"verbose",		optional_argument,	NULL,	'v'},
			{"help",		no_argument,		NULL,	'h'},
			{"version",		no_argument,		NULL,	'V'},
			{"copying",		no_argument,		NULL,	'C'},
			{"?",			no_argument,		NULL,	'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "d:s:pb:T:w:x:D::v::hVCH?", long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = = getopt(argc, argv, "d:s:pb:T:w:x:D:vhVC?");
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
			options.screen = strtoul(optarg, NULL, 0);
			break;
		case 'p':	/* -p, --popup */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandPopup;
			options.command = CommandPopup;
			break;
		case 'b':	/* -b, --button BUTTON */
			options.button = strtoul(optarg, NULL, 0);
			break;
		case 'T':	/* -T, --timestamp TIMESTAMP */
			options.timestamp = strtoul(optarg, NULL, 0);
			break;
		case 'w':	/* -w, --which WHICH */
			if (options.which != UseWindowDefault)
				goto bad_option;
			len = strlen(optarg);
			if (!strncasecmp("active", optarg, len))
				options.which = UseWindowActive;
			else if (!strncasecmp("focused", optarg, len))
				options.which = UseWindowFocused;
			else if (!strncasecmp("pointer", optarg, len))
				options.which = UseWindowPointer;
			else if (!strncasecmp("select", optarg, len)) 
				options.which = UseWindowSelect;
			else if ((options.window = strtoul(optarg, NULL, 0)))
				options.which = UseWindowSpecified;
			else
				goto bad_option;
			break;
		case 'x':	/* -x, --id WINDOW */
			if (options.which != UseWindowDefault)
				goto bad_option;
			options.window = strtoul(optarg, NULL, 0);
			if (!options.window)
				goto bad_option;
			options.which = UseWindowSpecified;
			break;
		case 'D':	/* -D, --debug [level] */
			if (options.debug)
				fprintf(stderr, "%s: increasing debug verbosity\n", argv[0]);
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
				fprintf(stderr, "%s: increasing output verbosity\n", argv[0]);
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
	get_defaults();
	startup(argc, argv);
	switch (command) {
	case CommandDefault:
	case CommandPopup:
		if (options.debug)
			fprintf(stderr, "%s: popping the menu\n", argv[0]);
		pop_the_menu(argc, argv);
		break;
	case CommandHelp:
		if (options.debug)
			fprintf(stderr, "%s: printing help message\n", argv[0]);
		help(argc, argv);
		break;
	case CommandVersion:
		if (options.debug)
			fprintf(stderr, "%s: printing version message\n", argv[0]);
		version(argc, argv);
		break;
	case CommandCopying:
		if (options.debug)
			fprintf(stderr, "%s: printing copying message\n", argv[0]);
		copying(argc, argv);
		break;
	}
	exit(0);
}

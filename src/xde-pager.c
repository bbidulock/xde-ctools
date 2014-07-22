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

#include "xde-pager.h"

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

static Atom _XA_XDE_THEME_NAME;
static Atom _XA_GTK_READ_RCFILES;
static Atom _XA_NET_WM_ICON_GEOMETRY;
static Atom _XA_NET_DESKTOP_LAYOUT;
static Atom _XA_NET_NUMBER_OF_DESKTOPS;
static Atom _XA_NET_CURRENT_DESKTOP;
static Atom _XA_WIN_DESKTOP_BUTTON_PROXY;

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
	unsigned long timeout;
	unsigned int border;
	Bool proxy;
	Command command;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.timeout = 1000,
	.border = 5,
	.proxy = True,
	.command = CommandDefault,
};

typedef struct {
	GdkDisplay *disp;
	GdkScreen *screen;
	GdkWindow *root, *proxy;
	WnckScreen *scrn;
	GtkWidget *pager;
	Window selwin;			/* selection owner window */
	Atom atom;			/* selection atom for this screen */
	int width, height;
	GtkWidget *popup;
	guint timer;			/* timer source of running timer */
	Bool keyboard;			/* have a keyboard grab */
	Bool pointer;			/* have a pointer grab */
	GdkModifierType mask;
	int rows;			/* number of rows in layout */
	int cols;			/* number of cols in layout */
	int desks;			/* number of desks in layout */
	int current;			/* current desktop */
	Bool inside;			/* pointer inside popup */
	char *wmname;			/* window manager name (adjusted) */
	Bool goodwm;			/* is the window manager usable? */
} XdeScreen;

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
		snprintf(selection, sizeof(selection), "_XDE_PAGER_S%d", s);
		atom = XInternAtom(dpy, selection, False);
		if (!(owner = XGetSelectionOwner(dpy, atom)))
			DPRINTF("No owner for %s\n", selection);
		if ((owner && replace) || (!owner && selwin)) {
			DPRINTF("Setting owner of %s to 0x%08lx from 0x%08lx\n", selection,
				selwin, owner);
			XSetSelectionOwner(dpy, atom, selwin, CurrentTime);
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
			GdkScreen *screen;
			Window root;

			for (s = 0; s < nscr; s++) {
				screen = gdk_display_get_screen(disp, s);
				root = GDK_WINDOW_XID(gdk_screen_get_root_window(screen));
				snprintf(selection, sizeof(selection), "_XDE_PAGER_S%d", s);
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
		DPRINTF("%s: not replacing running instance\n", NAME);
	return (gotone);
}

static void
workspace_destroyed(WnckScreen *screen, WnckWorkspace *space, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
}

static void
workspace_created(WnckScreen *screen, WnckWorkspace *space, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
}

static void setup_button_proxy(XdeScreen *xscr);

static Bool
good_window_manager(XdeScreen *xscr)
{
	DPRINT();
	/* ignore non fully compliant names */
	if (!xscr->wmname)
		return False;
	if (!strcasecmp(xscr->wmname, "afterstep"))
		return False;
	if (!strcasecmp(xscr->wmname, "jwm"))
		return False;
	if (!strcasecmp(xscr->wmname, "metacity"))
		return False;
	if (!strcasecmp(xscr->wmname, "openbox"))
		return False;
	if (!strcasecmp(xscr->wmname, "pekwm"))
		return False;
	if (!strcasecmp(xscr->wmname, "xdwm"))
		return False;
#if 0
	if (!strcasecmp(xscr->wmname, "cwm"))
		return False;
	if (!strcasecmp(xscr->wmname, "spectrwm"))
		return False;
#endif
	return True;
}

static void
window_manager_changed(WnckScreen *screen, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;
	const char *name;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINT();
	wnck_screen_force_update(screen);
	if (options.proxy)
		setup_button_proxy(xscr);
	free(xscr->wmname);
	xscr->wmname = NULL;
	xscr->goodwm = False;
	if ((name = wnck_screen_get_window_manager_name(screen))) {
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
	DPRINTF("window manager is '%s'\n", xscr->wmname);
	DPRINTF("window manager is %s\n", xscr->goodwm ? "usable" : "unusable");
}

static void drop_popup(XdeScreen *xscr);

static gboolean
workspace_timeout(gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINTF("popup timeout!\n");
	drop_popup(xscr);
	xscr->timer = 0;
	return G_SOURCE_REMOVE;
}

static gboolean
stop_popup_timer(XdeScreen *xscr)
{
	DPRINT();
	if (xscr->timer) {
		DPRINTF("stopping popup timer\n");
		g_source_remove(xscr->timer);
		xscr->timer = 0;
		return TRUE;
	}
	return FALSE;
}

static gboolean
start_popup_timer(XdeScreen *xscr)
{
	DPRINT();
	if (xscr->timer)
		return FALSE;
	DPRINTF("starting popup timer\n");
	xscr->timer = g_timeout_add(options.timeout, workspace_timeout, xscr);
	return TRUE;
}

void
restart_popup_timer(XdeScreen *xscr)
{
	DPRINTF("restarting popup timer\n");
	stop_popup_timer(xscr);
	start_popup_timer(xscr);
}

static void
release_grabs(XdeScreen *xscr)
{
	DPRINT();
	if (xscr->pointer) {
		DPRINTF("ungrabbing pointer\n");
		gdk_display_pointer_ungrab(xscr->disp, GDK_CURRENT_TIME);
		xscr->pointer = False;
	}
	if (xscr->keyboard) {
		DPRINTF("ungrabbing keyboard\n");
		gdk_display_keyboard_ungrab(xscr->disp, GDK_CURRENT_TIME);
		xscr->keyboard = False;
	}
}

static void
drop_popup(XdeScreen *xscr)
{
	DPRINT();
	if (gtk_widget_get_mapped(xscr->popup)) {
		stop_popup_timer(xscr);
		release_grabs(xscr);
		gtk_widget_hide(xscr->popup);
	}
}

static void
something_changed(WnckScreen *screen, XdeScreen *xscr)
{
	GdkGrabStatus status;
	GdkDisplay *disp;
	Display *dpy;
	Window win;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINT();
	if (!xscr->goodwm)
		return;
	disp = gdk_display_get_default();
	dpy = GDK_DISPLAY_XDISPLAY(disp);

	gdk_display_get_pointer(disp, NULL, NULL, NULL, &xscr->mask);
	DPRINTF("modifier mask was: 0x%08x\n", xscr->mask);

	stop_popup_timer(xscr);
	gtk_window_set_position(GTK_WINDOW(xscr->popup), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_present(GTK_WINDOW(xscr->popup));
	gtk_widget_show_now(GTK_WIDGET(xscr->popup));
	win = GDK_WINDOW_XID(xscr->popup->window);
	if (!xscr->pointer) {
		GdkEventMask mask =
		    GDK_POINTER_MOTION_MASK |
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON_MOTION_MASK |
		    GDK_BUTTON_PRESS_MASK |
		    GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
		DPRINT();
		XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
		status = gdk_pointer_grab(xscr->popup->window, TRUE,
					  mask, NULL, NULL, GDK_CURRENT_TIME);
		switch (status) {
		case GDK_GRAB_SUCCESS:
			DPRINTF("pointer grabbed\n");
			xscr->pointer = True;
			break;
		case GDK_GRAB_ALREADY_GRABBED:
			DPRINTF("%s: pointer already grabbed\n", NAME);
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
	if (!xscr->keyboard) {
		XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
		status = gdk_keyboard_grab(xscr->popup->window, TRUE, GDK_CURRENT_TIME);
		switch (status) {
		case GDK_GRAB_SUCCESS:
			DPRINTF("keyboard grabbed\n");
			xscr->keyboard = True;
			break;
		case GDK_GRAB_ALREADY_GRABBED:
			DPRINTF("%s: keyboard already grabbed\n", NAME);
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
	// if (!xscr->keyboard || !xscr->pointer)
	if (!(xscr->mask & ~(GDK_LOCK_MASK | GDK_BUTTON1_MASK |
			     GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)))
		if (!xscr->inside)
			start_popup_timer(xscr);
}

static void
viewports_changed(WnckScreen *screen, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	something_changed(screen, xscr);
}

static void
background_changed(WnckScreen *screen, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
}

static void
active_workspace_changed(WnckScreen *screen, WnckWorkspace *prev, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	something_changed(screen, xscr);
}

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
	XdeScreen *xscr = (typeof(xscr)) user;
	GdkEventGrabBroken *ev = (typeof(ev)) event;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINT();
	if (ev->keyboard) {
		DPRINTF("keyboard grab was broken\n");
		xscr->keyboard = False;
		/* IF we lost a keyboard grab, it is because another hot-key was pressed, 
		   either doing something else or moving to another desktop.  Start the
		   timeout in this case. */
		start_popup_timer(xscr);
	} else {
		DPRINTF("pointer grab was broken\n");
		xscr->pointer = False;
		/* If we lost a pointer grab, it is because somebody clicked on another
		   window.  In this case we want to drop the popup altogether.  This will 
		   break the keyboard grab if any. */
		drop_popup(xscr);
	}
	if (ev->implicit) {
		DPRINTF("broken grab was implicit\n");
	} else {
		DPRINTF("broken grab was explicit\n");
	}
	if (ev->grab_window) {
		DPRINTF("we broke the grab\n");
	} else {
		DPRINTF("another application broke the grab\n");
	}
	return TRUE;		/* event handled */
}

static GdkFilterReturn popup_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);

static void
widget_realize(GtkWidget *popup, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINT();
	gdk_window_add_filter(popup->window, popup_handler, xscr);
}

static gboolean
button_press_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	(void) xscr;
	DPRINT();
	return FALSE;
}

static gboolean
button_release_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	(void) xscr;
	DPRINT();
	return FALSE;
}

static gboolean
enter_notify_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINT();
	(void) xscr;
	// stop_popup_timer(xscr);
	// xscr->inside = True;
	return FALSE;
}

static gboolean
focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	(void) xscr;
	DPRINT();
	return FALSE;
}

static gboolean
focus_out_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	(void) xscr;
	DPRINT();
	return FALSE;
}

static void
grab_focus(GtkWidget *widget, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	(void) xscr;
	DPRINT();
}

static gboolean
key_press_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	(void) xscr;
	DPRINT();
	return FALSE;
}

static gboolean
key_release_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;
	GdkEventKey *ev = (typeof(ev)) event;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINT();
	if (ev->is_modifier) {
		DPRINTF("released key is modifier: dropping popup\n");
		drop_popup(xscr);
	}
	return FALSE;
}

static gboolean
leave_notify_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINT();
	(void) xscr;
	// start_popup_timer(xscr);
	// xscr->inside = False;
	return FALSE;
}

static gboolean
map_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	(void) xscr;
	DPRINT();
	return FALSE;
}

static gboolean
scroll_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	(void) xscr;
	DPRINT();
	return FALSE;
}

static GdkFilterReturn proxy_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);

static void
setup_button_proxy(XdeScreen *xscr)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = GDK_WINDOW_XID(xscr->root), proxy;
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;

	DPRINT();
	xscr->proxy = NULL;
	if (XGetWindowProperty(dpy, root, _XA_WIN_DESKTOP_BUTTON_PROXY,
			       0, 1, False, XA_CARDINAL, &actual, &format,
			       &nitems, &after, (unsigned char **) &data) == Success &&
	    format == 32 && nitems >= 1 && data) {
		proxy = data[0];
		if ((xscr->proxy = gdk_x11_window_foreign_new_for_display(xscr->disp, proxy))) {
			GdkEventMask mask;

			mask = gdk_window_get_events(xscr->proxy);
			mask |=
			    GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK |
			    GDK_SUBSTRUCTURE_MASK;
			gdk_window_set_events(xscr->proxy, mask);
			DPRINTF("adding filter for desktop button proxy\n");
			gdk_window_add_filter(xscr->proxy, proxy_handler, xscr);
		}
	}
	if (data) {
		XFree(data);
		data = NULL;
	}

}

static void
update_current_desktop(XdeScreen *xscr)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = GDK_WINDOW_XID(xscr->root);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;

	DPRINT();
	if (XGetWindowProperty(dpy, root, _XA_NET_CURRENT_DESKTOP, 0, 64, False, XA_CARDINAL,
			       &actual, &format, &nitems, &after,
			       (unsigned char **) &data) == Success && format == 32
	    && nitems >= 1 && data) {
		xscr->current = data[0];
	}
	if (data) {
		XFree(data);
		data = NULL;
	}
}

static void redo_layout(XdeScreen *xscr);

static GdkFilterReturn root_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);

static void
init_popup(XdeScreen *xscr)
{
	GtkWidget *popup;
	WnckScreen *scrn = xscr->scrn;
	WnckPager *pager = (typeof(pager)) xscr->pager;

	DPRINT();
	gdk_window_add_filter(xscr->root, root_handler, xscr);

	if (options.proxy)
		setup_button_proxy(xscr);

	xscr->popup = popup = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_add_events(popup, GDK_ALL_EVENTS_MASK);
	gtk_window_set_focus_on_map(GTK_WINDOW(popup), TRUE);
	gtk_window_set_type_hint(GTK_WINDOW(popup), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
	gtk_window_stick(GTK_WINDOW(popup));
	gtk_window_set_keep_above(GTK_WINDOW(popup), TRUE);
	wnck_pager_set_layout_policy(pager, WNCK_PAGER_LAYOUT_POLICY_AUTOMATIC);
	wnck_pager_set_display_mode(pager, WNCK_PAGER_DISPLAY_CONTENT);
	wnck_pager_set_show_all(pager, TRUE);
	wnck_pager_set_shadow_type(pager, GTK_SHADOW_IN);
	gtk_container_set_border_width(GTK_CONTAINER(popup), options.border);
	gtk_container_add(GTK_CONTAINER(popup), GTK_WIDGET(pager));

	redo_layout(xscr);
	update_current_desktop(xscr);

	gtk_window_set_position(GTK_WINDOW(popup), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_widget_show(GTK_WIDGET(pager));
	g_signal_connect(G_OBJECT(scrn), "workspace_destroyed",
			 G_CALLBACK(workspace_destroyed), xscr);
	g_signal_connect(G_OBJECT(scrn), "workspace_created",
			 G_CALLBACK(workspace_created), xscr);
	g_signal_connect(G_OBJECT(scrn), "window_manager_changed",
			 G_CALLBACK(window_manager_changed), xscr);
	g_signal_connect(G_OBJECT(scrn), "viewports_changed",
			 G_CALLBACK(viewports_changed), xscr);
	g_signal_connect(G_OBJECT(scrn), "background_changed",
			 G_CALLBACK(background_changed), xscr);
	g_signal_connect(G_OBJECT(scrn), "active_workspace_changed",
			 G_CALLBACK(active_workspace_changed), xscr);
	g_signal_connect(G_OBJECT(popup), "button_press_event",
			 G_CALLBACK(button_press_event), xscr);
	g_signal_connect(G_OBJECT(popup), "button_release_event",
			 G_CALLBACK(button_release_event), xscr);
	g_signal_connect(G_OBJECT(popup), "enter_notify_event",
			 G_CALLBACK(enter_notify_event), xscr);
	g_signal_connect(G_OBJECT(popup), "focus_in_event", G_CALLBACK(focus_in_event), xscr);
	g_signal_connect(G_OBJECT(popup), "focus_out_event",
			 G_CALLBACK(focus_out_event), xscr);
	g_signal_connect(G_OBJECT(popup), "grab_broken_event",
			 G_CALLBACK(grab_broken_event), xscr);
	g_signal_connect(G_OBJECT(popup), "grab_focus", G_CALLBACK(grab_focus), xscr);
	g_signal_connect(G_OBJECT(popup), "key_press_event",
			 G_CALLBACK(key_press_event), xscr);
	g_signal_connect(G_OBJECT(popup), "key_release_event",
			 G_CALLBACK(key_release_event), xscr);
	g_signal_connect(G_OBJECT(popup), "leave_notify_event",
			 G_CALLBACK(leave_notify_event), xscr);
	g_signal_connect(G_OBJECT(popup), "map_event", G_CALLBACK(map_event), xscr);
	g_signal_connect(G_OBJECT(popup), "realize", G_CALLBACK(widget_realize), xscr);
	g_signal_connect(G_OBJECT(popup), "scroll_event", G_CALLBACK(scroll_event), xscr);
}

static GdkFilterReturn selwin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);

static void
do_run(int argc, char *argv[], Bool replace)
{
	GdkDisplay *disp = gdk_display_get_default();
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
	GdkScreen *screen = gdk_display_get_default_screen(disp);
	GdkWindow *root = gdk_screen_get_root_window(screen), *sel;
	char selection[64] = { 0, };
	Window selwin, owner;
	XdeScreen *screens, *xscr;
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
		snprintf(selection, sizeof(selection), "_XDE_PAGER_S%d", s);
		xscr->atom = XInternAtom(dpy, selection, False);
		xscr->disp = disp;
		xscr->screen = screen = gdk_display_get_screen(disp, s);
		xscr->root = gdk_screen_get_root_window(screen);
		xscr->scrn = wnck_screen_get(s);
		wnck_screen_force_update(xscr->scrn);
		window_manager_changed(xscr->scrn, xscr);
		xscr->pager = wnck_pager_new(xscr->scrn);
		xscr->selwin = selwin;
		xscr->width = gdk_screen_get_width(screen);
		xscr->height = gdk_screen_get_height(screen);
		init_popup(xscr);
	}
	gtk_main();
}

/** @brief Ask a running instance to quit.
  *
  * This is performed by checking for an owner of the _XDE_PAGER_S%d selection and clearing the
  * selection if it exists.
  */
static void
do_quit(int argc, char *argv[])
{
	DPRINT();
	get_selection(True, None);
}

static void
reparse(Display *dpy, Window root)
{
	XTextProperty xtp = { NULL, };
	char **list = NULL;
	int strings = 0;

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

static void
redo_layout(XdeScreen *xscr)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = GDK_WINDOW_XID(xscr->root);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;
	unsigned int w, h, f, wmax, hmax;

	DPRINT();
	if (XGetWindowProperty(dpy, root, _XA_NET_DESKTOP_LAYOUT, 0, 4, False, AnyPropertyType,
			       &actual, &format, &nitems, &after,
			       (unsigned char **) &data) == Success && format == 32
	    && nitems >= 4 && data) {
		xscr->rows = data[2];
		xscr->cols = data[1];
	}
	if (data) {
		XFree(data);
		data = NULL;
	}
	if (XGetWindowProperty(dpy, root, _XA_NET_NUMBER_OF_DESKTOPS, 0, 1, False, XA_CARDINAL,
			       &actual, &format, &nitems, &after,
			       (unsigned char **) &data) == Success && format == 32
	    && nitems >= 1 && data) {
		xscr->desks = data[0];
	}
	if (data) {
		XFree(data);
		data = NULL;
	}
	if (xscr->desks <= 0)
		xscr->desks = 1;
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
	if (xscr->cols == 0) {
		int num = xscr->desks;

		while (num > 0) {
			xscr->cols++;
			num -= xscr->rows;
		}
	}
	if (xscr->rows == 0) {
		int num = xscr->desks;

		while (num > 0) {
			xscr->rows++;
			num -= xscr->cols;
		}
	}
	w = xscr->width * xscr->cols;
	h = xscr->height * xscr->rows;
	wmax = (xscr->width * 8) / 10;
	hmax = (xscr->height * 8) / 10;
	for (f = 10; w > wmax * f || h > hmax * f; f++) ;
	gtk_window_set_default_size(GTK_WINDOW(xscr->popup), w / f, h / f);
}

static GdkFilterReturn
event_handler_PropertyNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 2) {
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
		DPRINT();
		reparse(dpy, xev->xproperty.window);
		return GDK_FILTER_REMOVE;	/* event handled */
	}
	if (xev->xproperty.atom == _XA_NET_DESKTOP_LAYOUT
	    && xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		redo_layout(xscr);
	}
	if (xev->xproperty.atom == _XA_NET_NUMBER_OF_DESKTOPS
	    && xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		redo_layout(xscr);
	}
	if (xev->xproperty.atom == _XA_NET_CURRENT_DESKTOP
	    && xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_current_desktop(xscr);
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
}

static GdkFilterReturn
event_handler_ClientMessage(Display *dpy, XEvent *xev)
{
	DPRINT();
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
		return GDK_FILTER_REMOVE;	/* event handled */
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
}

#define PASSED_EVENT_MASK (KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|SubstructureNotifyMask|SubstructureRedirectMask)

static GdkFilterReturn
event_handler_KeyPress(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> KeyPress: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xkey.send_event ? "true" : "false");
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
		fprintf(stderr, "    --> same_screen = %s\n",
			xev->xkey.same_screen ? "true" : "false");
		fprintf(stderr, "<== KeyPress: %p\n", xscr);
	}
	if (!xev->xkey.send_event) {
		XEvent ev = *xev;

		start_popup_timer(xscr);
		ev.xkey.window = ev.xkey.root;
		XSendEvent(dpy, ev.xkey.root, True, PASSED_EVENT_MASK, &ev);
		XFlush(dpy);
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
}

static GdkFilterReturn
event_handler_KeyRelease(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> KeyRelease: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xkey.send_event ? "true" : "false");
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
		fprintf(stderr, "    --> same_screen = %s\n",
			xev->xkey.same_screen ? "true" : "false");
		fprintf(stderr, "<== KeyRelease: %p\n", xscr);
	}
	if (!xev->xkey.send_event) {
		XEvent ev = *xev;

		// start_popup_timer(xscr);
		ev.xkey.window = ev.xkey.root;
		XSendEvent(dpy, ev.xkey.root, True, PASSED_EVENT_MASK, &ev);
		XFlush(dpy);
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
}

static GdkFilterReturn
event_handler_ButtonPress(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> ButtonPress: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xbutton.send_event ? "true" : "false");
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
		fprintf(stderr, "    --> same_screen = %s\n",
			xev->xbutton.same_screen ? "true" : "false");
		fprintf(stderr, "<== ButtonPress: %p\n", xscr);
	}
	if (!xev->xbutton.send_event) {
		XEvent ev = *xev;

		if (ev.xbutton.button == 4 || ev.xbutton.button == 5) {
			if (!xscr->inside)
				start_popup_timer(xscr);
			DPRINTF("ButtonPress = %d passing to root window\n",
				ev.xbutton.button);
			ev.xbutton.window = ev.xbutton.root;
			XSendEvent(dpy, ev.xbutton.root, True, PASSED_EVENT_MASK, &ev);
			XFlush(dpy);
		}
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
}

static GdkFilterReturn
event_handler_ButtonRelease(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> ButtonRelease: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xbutton.send_event ? "true" : "false");
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
		fprintf(stderr, "    --> same_screen = %s\n",
			xev->xbutton.same_screen ? "true" : "false");
		fprintf(stderr, "<== ButtonRelease: %p\n", xscr);
	}
	if (!xev->xbutton.send_event) {
		XEvent ev = *xev;

		if (ev.xbutton.button == 4 || ev.xbutton.button == 5) {
			// start_popup_timer(xscr);
			DPRINTF("ButtonRelease = %d passing to root window\n",
				ev.xbutton.button);
			ev.xbutton.window = ev.xbutton.root;
			XSendEvent(dpy, ev.xbutton.root, True, PASSED_EVENT_MASK, &ev);
			XFlush(dpy);
		}
		return GDK_FILTER_CONTINUE;
	}
	return GDK_FILTER_REMOVE;
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

static const char *
show_mode(int mode)
{
	DPRINT();
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
	DPRINT();
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
event_handler_EnterNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> EnterNotify: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xcrossing.send_event ? "true" : "false");
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
		fprintf(stderr, "    --> same_screen = %s\n",
			xev->xcrossing.same_screen ? "true" : "false");
		fprintf(stderr, "    --> focus = %s\n",
			xev->xcrossing.focus ? "true" : "false");
		fprintf(stderr, "    --> state = 0x%08x\n", xev->xcrossing.state);
		fprintf(stderr, "<== EnterNotify: %p\n", xscr);
	}
	if (xev->xcrossing.mode == NotifyNormal) {
		if (!xscr->inside) {
			DPRINTF("entered popup\n");
			stop_popup_timer(xscr);
			xscr->inside = True;
		}
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_LeaveNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> LeaveNotify: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xcrossing.send_event ? "true" : "false");
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
		fprintf(stderr, "    --> same_screen = %s\n",
			xev->xcrossing.same_screen ? "true" : "false");
		fprintf(stderr, "    --> focus = %s\n",
			xev->xcrossing.focus ? "true" : "false");
		fprintf(stderr, "    --> state = 0x%08x\n", xev->xcrossing.state);
		fprintf(stderr, "<== LeaveNotify: %p\n", xscr);
	}
	if (xev->xcrossing.mode == NotifyNormal) {
		if (xscr->inside) {
			DPRINTF("left popup\n");
			start_popup_timer(xscr);
			xscr->inside = False;
		}
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_FocusIn(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> FocusIn: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xfocus.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xfocus.window);
		fprintf(stderr, "    --> mode = %s\n", show_mode(xev->xfocus.mode));
		fprintf(stderr, "    --> detail = %s\n", show_detail(xev->xfocus.detail));
		fprintf(stderr, "<== FocusIn: %p\n", xscr);
	}
	switch (xev->xfocus.mode) {
	case NotifyNormal:
	case NotifyUngrab:
		DPRINTF("focused popup\n");
		break;
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_FocusOut(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> FocusOut: %p\n", xscr);
		fprintf(stderr, "    --> send_event = %s\n",
			xev->xfocus.send_event ? "true" : "false");
		fprintf(stderr, "    --> window = 0x%lx\n", xev->xfocus.window);
		fprintf(stderr, "    --> mode = %s\n", show_mode(xev->xfocus.mode));
		fprintf(stderr, "    --> detail = %s\n", show_detail(xev->xfocus.detail));
		fprintf(stderr, "<== FocusOut: %p\n", xscr);
	}
	switch (xev->xfocus.mode) {
	case NotifyNormal:
	case NotifyGrab:
	case NotifyWhileGrabbed:
		DPRINTF("unfocused popup\n");
		if (!xscr->keyboard) {
			DPRINTF("no grab or focus\n");
			start_popup_timer(xscr);
		}
		break;
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
	case PropertyNotify:
		return event_handler_PropertyNotify(dpy, xev, xscr);
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
popup_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
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
	case KeyPress:
		return event_handler_KeyPress(dpy, xev, xscr);
	case KeyRelease:
		return event_handler_KeyRelease(dpy, xev, xscr);
	case ButtonPress:
		return event_handler_ButtonPress(dpy, xev, xscr);
	case ButtonRelease:
		return event_handler_ButtonRelease(dpy, xev, xscr);
	case EnterNotify:
		return event_handler_EnterNotify(dpy, xev, xscr);
	case LeaveNotify:
		return event_handler_LeaveNotify(dpy, xev, xscr);
	case FocusIn:
		return event_handler_FocusIn(dpy, xev, xscr);
	case FocusOut:
		return event_handler_FocusOut(dpy, xev, xscr);
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
selwin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
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
	case SelectionClear:
		return event_handler_SelectionClear(dpy, xev, xscr);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
client_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	Display *dpy = (typeof(dpy)) data;

	DPRINT();
	switch (xev->type) {
	case ClientMessage:
		return event_handler_ClientMessage(dpy, xev);
	}
	EPRINTF("wrong message type for handler %d\n", xev->type);
	return GDK_FILTER_CONTINUE;
}

static void
set_current_desktop(XdeScreen *xscr, int index, Time timestamp)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = GDK_WINDOW_XID(xscr->root);
	XEvent ev;

	DPRINT();
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

static GdkFilterReturn
proxy_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = (typeof(xscr)) data;
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	int num;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	switch (xev->type) {
	case ButtonPress:
		if (options.debug) {
			fprintf(stderr, "==> ButtonPress: %p\n", xscr);
			fprintf(stderr, "    --> send_event = %s\n",
				xev->xbutton.send_event ? "true" : "false");
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
			fprintf(stderr, "    --> same_screen = %s\n",
				xev->xbutton.same_screen ? "true" : "false");
			fprintf(stderr, "<== ButtonPress: %p\n", xscr);
		}
		switch (xev->xbutton.button) {
		case 4:
			update_current_desktop(xscr);
			num = (xscr->current - 1 + xscr->desks) % xscr->desks;
			set_current_desktop(xscr, num, xev->xbutton.time);
			break;
		case 5:
			update_current_desktop(xscr);
			num = (xscr->current + 1 + xscr->desks) % xscr->desks;
			set_current_desktop(xscr, num, xev->xbutton.time);
			break;
		}
		return GDK_FILTER_CONTINUE;
	case ButtonRelease:
		if (options.debug > 1) {
			fprintf(stderr, "==> ButtonRelease: %p\n", xscr);
			fprintf(stderr, "    --> send_event = %s\n",
				xev->xbutton.send_event ? "true" : "false");
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
			fprintf(stderr, "    --> same_screen = %s\n",
				xev->xbutton.same_screen ? "true" : "false");
			fprintf(stderr, "<== ButtonRelease: %p\n", xscr);
		}
		return GDK_FILTER_CONTINUE;
	case PropertyNotify:
		if (options.debug > 2) {
			fprintf(stderr, "==> PropertyNotify:\n");
			fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
			fprintf(stderr, "    --> atom = %s\n",
				XGetAtomName(dpy, xev->xproperty.atom));
			fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
			fprintf(stderr, "    --> state = %s\n",
				(xev->xproperty.state ==
				 PropertyNewValue) ? "NewValue" : "Delete");
			fprintf(stderr, "<== PropertyNotify:\n");
		}
		return GDK_FILTER_CONTINUE;
	}
	EPRINTF("wrong message type for handler %d on window 0x%08lx\n", xev->type,
		xev->xany.window);
	return GDK_FILTER_CONTINUE;
}

static void
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
	int len;

	DPRINT();
	home = getenv("HOME") ? : ".";
	len = strlen(home) + strlen(suffix) + 1;
	file = calloc(len, sizeof(*file));
	strncpy(file, home, len);
	strncat(file, suffix, len);
	gtk_rc_add_default_file(file);
	free(file);

	gtk_init(&argc, &argv);

	disp = gdk_display_get_default();

	dpy = GDK_DISPLAY_XDISPLAY(disp);

	atom = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_XA_XDE_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_GTK_READ_RCFILES");
	_XA_GTK_READ_RCFILES = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);

	atom = gdk_atom_intern_static_string("_NET_WM_ICON_GEOMETRY");
	_XA_NET_WM_ICON_GEOMETRY = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_DESKTOP_LAYOUT");
	_XA_NET_DESKTOP_LAYOUT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_NUMBER_OF_DESKTOPS");
	_XA_NET_NUMBER_OF_DESKTOPS = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_CURRENT_DESKTOP");
	_XA_NET_CURRENT_DESKTOP = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_DESKTOP_BUTTON_PROXY");
	_XA_WIN_DESKTOP_BUTTON_PROXY = gdk_x11_atom_to_xatom_for_display(disp, atom);

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
show_bool(Bool value)
{
	if (value)
		return ("true");
	return ("false");
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
        specify the X display, DISPLAY, to use [default: %2$s]\n\
    -t, --timeout MILLISECONDS\n\
        specify timeout when not modifier [default: %3$lu]\n\
    -b, --border PIXELS\n\
        border surrounding feedback popup [default: %4$u]\n\
    -n, --noproxy\n\
        do not respond to button proxy [default: %5$s]\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %6$d]\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %7$d]\n\
        this option may be repeated.\n\
", argv[0] 
	, options.display
	, options.timeout
	, options.border
	, show_bool(options.proxy)
	, options.debug
	, options.output
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
	if (!options.display) {
		EPRINTF("No DISPLAY environment variable or --display option\n");
		exit(EXIT_FAILURE);
	}
	if (options.command == CommandDefault)
		options.command = CommandRun;

}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;

	set_defaults();

	while (1) {
		int c, val;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"display",		required_argument,	NULL,	'd'},
			{"timeout",		required_argument,	NULL,	't'},
			{"border",		required_argument,	NULL,	'b'},
			{"noproxy",		no_argument,		NULL,	'n'},

			{"quit",		no_argument,		NULL,	'q'},
			{"replace",		no_argument,		NULL,	'r'},

			{"debug",		optional_argument,	NULL,	'D'},
			{"verbose",		optional_argument,	NULL,	'v'},
			{"help",		no_argument,		NULL,	'h'},
			{"version",		no_argument,		NULL,	'V'},
			{"copying",		no_argument,		NULL,	'C'},
			{"?",			no_argument,		NULL,	'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "d:s:pb:T:w:x:W:D::v::hVCH?",
				     long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:pb:T:w:x:W:D:vhVC?");
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
		case 't':	/* -t, --timeout MILLISECONDS */
			options.timeout = strtoul(optarg, NULL, 0);
			if (!options.timeout)
				goto bad_option;
			break;
		case 'b':	/* -b, --border PIXELS */
			options.border = strtoul(optarg, NULL, 0);
			if (options.border > 20)
				goto bad_option;
			break;
		case 'n':	/* -n, --noproxy */
			options.proxy = False;
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
						fprintf(stderr, "%s",
							(optind < argc) ? " " : "");
					}
					fprintf(stderr, "'\n");
				} else {
					fprintf(stderr,
						"%s: missing option or argument", argv[0]);
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
	case CommandRun:
		if (options.debug)
			fprintf(stderr, "%s: running a new instance\n", argv[0]);
		do_run(argc, argv, False);
		break;
	case CommandQuit:
		if (options.debug)
			fprintf(stderr, "%s: asking existing instance to quit\n", argv[0]);
		do_quit(argc, argv);
		break;
	case CommandReplace:
		if (options.debug)
			fprintf(stderr, "%s: replacing existing instance\n", argv[0]);
		do_run(argc, argv, True);
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

// vim: tw=100 com=sr0\:/**,mb\:*,ex\:*/,sr0\:/*,mb\:*,ex\:*/,b\:TRANS formatoptions+=tcqlor

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

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1
#define EXIT_SYNTAXERR	2

#define XA_SELECTION_NAME	"_XDE_DESKTOP_S%d"
#define XA_NET_DESKTOP_LAYOUT	"_NET_DESKTOP_LAYOUT_S%d"

static int saveArgc;
static char **saveArgv;

static Atom _XA_XDE_THEME_NAME;
static Atom _XA_GTK_READ_RCFILES;
static Atom _XA_NET_WM_ICON_GEOMETRY;
static Atom _XA_NET_DESKTOP_LAYOUT;
static Atom _XA_NET_DESKTOP_NAMES;
static Atom _XA_NET_NUMBER_OF_DESKTOPS;
static Atom _XA_NET_CURRENT_DESKTOP;
static Atom _XA_WIN_DESKTOP_BUTTON_PROXY;
static Atom _XA_WIN_WORKSPACE_COUNT;
static Atom _XA_WIN_WORKSPACE;
static Atom _XA_WM_DESKTOP;
static Atom _XA_XROOTPMAP_ID;
static Atom _XA_ESETROOT_PMAP_ID;

// static Atom _XA_WIN_AREA;
// static Atom _XA_WIN_AREA_COUNT;

static Atom _XA_NET_ACTIVE_WINDOW;
static Atom _XA_NET_CLIENT_LIST;
static Atom _XA_NET_CLIENT_LIST_STACKING;
static Atom _XA_WIN_FOCUS;
static Atom _XA_WIN_CLIENT_LIST;

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
	CommandRun,
	CommandQuit,
	CommandReplace,
	CommandHelp,
	CommandVersion,
	CommandCopying,
} Command;

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
	unsigned long timeout;
	unsigned int border;
	Bool proxy;
	int button;
	Time timestamp;
	UseScreen which;
	MenuPosition where;
	struct {
		int value;
		int sign;
	} x, y;
	unsigned int w, h;
	Bool cycle;
	Bool hidden;
	Bool minimized;
	Bool monitors;
	Bool workspaces;
	WindowOrder order;
	Bool activate;
	Bool raise;
	Bool restore;
	char *keys;
	Command command;
	char *clientId;
	char *saveFile;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.timeout = 1000,
	.border = 5,
	.proxy = False,
	.button = 0,
	.timestamp = CurrentTime,
	.which = UseScreenDefault,
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
	.cycle = False,
	.hidden = False,
	.minimized = False,
	.monitors = False,
	.workspaces = False,
	.order = WindowOrderDefault,
	.activate = True,
	.raise = False,
	.restore = True,
	.keys = NULL,
	.command = CommandDefault,
	.clientId = NULL,
	.saveFile = NULL,
};

typedef struct _XdePixmap {
	int refs;			/* number of references */
	int index;			/* background image index */
	struct _XdePixmap *next;	/* next pixmap in list */
	struct _XdePixmap **pprev;	/* previous next pointer in list */
	GdkPixmap *pixmap;		/* pixmap for background image */
	GdkRectangle geom;		/* pixmap geometry */
} XdePixmap;

typedef struct {
	int refs;			/* number of references */
	int index;			/* background image index */
	char *file;			/* filename of image source */
	GdkPixbuf *pixbuf;		/* pixbuf for this image */
	XdePixmap *pixmaps;		/* list of pixmaps at various geometries */
} XdeImage;

typedef struct {
	int index;			/* monitor number */
	int current;			/* current desktop for this monitor */
	GdkRectangle geom;		/* geometry of the monitor */
} XdeMonitor;

typedef struct {
	int index;			/* index */
	GdkDisplay *disp;
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
	Window laywin;			/* desktop layout selection owner */
	Atom prop;			/* dekstop layout selection atom */
	int width, height;
	guint timer;			/* timer source of running timer */
	int rows;			/* number of rows in layout */
	int cols;			/* number of cols in layout */
	int desks;			/* number of desks in layout */
	int ndsk;			/* number of desktops */
	XdeImage **desktops;		/* the desktops */
	int current;			/* current desktop for this screen */
	char *wmname;			/* window manager name (adjusted) */
	Bool goodwm;			/* is the window manager usable? */
	GtkWindow *desktop;
	GdkWindow *proxy;
	guint deferred_refresh_layout;
	guint deferred_refresh_desktop;
	GtkWidget *popup;
	Bool inside;			/* pointer inside popup */
	Bool keyboard;			/* have a keyboard grab */
	Bool pointer;			/* have a pointer grab */
	GdkModifierType mask;
} XdeScreen;

XdeScreen *screens;			/* array of screens */

#if 0
static void refresh_layout(XdeScreen *xscr);
static void refresh_desktop(XdeScreen *xscr);

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

static void
add_deferred_refresh_layout(XdeScreen *xscr)
{
	if (!xscr->deferred_refresh_layout)
		xscr->deferred_refresh_layout =
			g_idle_add(on_deferred_refresh_layout, xscr);
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
	xscr->deferred_refresh_desktop =
		g_idle_add(on_deferred_refresh_desktop, xscr);
}
#endif

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
			DPRINTF("There are %d refs left for %p\n", (*imagep)->refs, *imagep);
	}
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
		DPRINTF("%s: not replacing running instance\n", NAME);
	return (gotone);
}

#if 0
static GdkFilterReturn laywin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);

static Window
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
	XSelectInput(dpy, xscr->laywin,
		StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask);
	lay = gdk_x11_window_foreign_new_for_display(disp, xscr->laywin);
	gdk_window_add_filter(lay, laywin_handler, xscr);
	snprintf(selection, sizeof(selection), XA_NET_DESKTOP_LAYOUT, xscr->index);
	atom = XInternAtom(dpy, selection, False);
	if (!(owner = XGetSelectionOwner(dpy, atom)))
		DPRINTF("No owner for %s\n", selection);
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
#endif

static void
workspace_destroyed(WnckScreen *wnck, WnckWorkspace *space, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
}

static void
workspace_created(WnckScreen *wnck, WnckWorkspace *space, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
}

static Bool
good_window_manager(XdeScreen *xscr)
{
	DPRINT();
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
	/* XXX: blackbox(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "blackbox"))
		return True;
	/* XXX: bspwm(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "baspwm"))
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
	/* XXX: fluxbox(1) is supported and works well. */
	if (!strcasecmp(xscr->wmname, "fluxbox"))
		return True;
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

static void setup_button_proxy(XdeScreen *xscr);

static void
window_manager_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;
	const char *name;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	DPRINT();
	wnck_screen_force_update(wnck);
	if (options.proxy)
		setup_button_proxy(xscr);
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
something_changed(WnckScreen *wnck, XdeScreen *xscr)
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
viewports_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	something_changed(wnck, xscr);
}

static void
background_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
}

static void
active_workspace_changed(WnckScreen *wnck, WnckWorkspace *prev, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;

	DPRINT();
	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	something_changed(wnck, xscr);
}

static GdkFilterReturn proxy_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);

static void
setup_button_proxy(XdeScreen *xscr)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index), proxy;
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
windows_changed(WnckScreen *wnck, XdeScreen *xscr)
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
	gtk_widget_show_now(GTK_WIDGET(xscr->popup));	/* XXX works for menus? */
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
clients_changed(WnckScreen *wnck, XdeScreen *xscr)
{
}

static void
active_window_changed(WnckScreen *wnck, WnckWindow *previous, gpointer user)
{
	XdeScreen *xscr = user;
	WnckWindow *active = wnck_screen_get_active_window(wnck);

	if (!active || !previous) {
		stop_popup_timer(xscr);
		drop_popup(xscr);
		return;
	}
	windows_changed(wnck, xscr);
}

static void
application_closed(WnckScreen *wnck, WnckApplication *app, gpointer user)
{
	/* XXX: might not be necessary */

}

static void
application_opened(WnckScreen *wnck, WnckApplication *app, gpointer user)
{
	XdeScreen *xscr = user;
	/* XXX: might not be necessary */
	clients_changed(wnck, xscr);
}

static void
class_group_closed(WnckScreen *wnck, WnckClassGroup *class_group, gpointer user)
{
	XdeScreen *xscr = user;
	/* XXX: might not be necessary */
	clients_changed(wnck, xscr);
}

static void
class_group_opened(WnckScreen *wnck, WnckClassGroup *class_group, gpointer user)
{
	XdeScreen *xscr = user;
	/* XXX: might not be necessary */
	clients_changed(wnck, xscr);
}

static void
window_closed(WnckScreen *wnck, WnckWindow *window, gpointer user)
{
	XdeScreen *xscr = user;
	clients_changed(wnck, xscr);
}

static void
window_opened(WnckScreen *wnck, WnckWindow *window, gpointer user)
{
	XdeScreen *xscr = user;
	clients_changed(wnck, xscr);
}

static void
window_stacking_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = user;
	/* XXX: might not be necessary */
	clients_changed(wnck, xscr);
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

static void
update_current_desktop(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0, i, j = 0, *x;
	unsigned long *data = NULL;
	Bool changed = False;
	XdeMonitor *xmon;

	DPRINT();
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
		DPRINTF("Current desktop changed.\n");
		/* FIXME: if changed update something */
	} else
		DPRINTF("No change in current desktop.\n");
}

static void
update_client_list(XdeScreen *xscr, Atom prop)
{
}

static void
update_window(XdeScreen *xscr, Atom prop)
{
}

static void update_root_pixmap(XdeScreen *xscr, Atom prop);

static void
init_window(XdeScreen *xscr)
{
	GtkWindow *win;
	GdkWindow *window;
	char *geometry;

	xscr->desktop = win = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_screen(win, xscr->scrn);
	gtk_window_set_accept_focus(win, FALSE);
	gtk_window_set_auto_startup_notification(TRUE);
	gtk_window_set_decorated(win, FALSE);
	gtk_window_set_default_size(win, xscr->width - 2, xscr->height - 2);

	GdkGeometry hints = {
		.min_width = xscr->width - 2,
		.min_height = xscr->height - 2,
		.max_width = xscr->width - 2,
		.max_height = xscr->height - 2,
	};
	gtk_window_set_geometry_hints(win, GTK_WIDGET(win), &hints,
				      GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_USER_SIZE);

	gtk_window_set_deletable(win, FALSE);
	gtk_window_set_focus_on_map(win, FALSE);
//      gtk_window_set_frame_dimensions(win, 0, 0, 0, 0);
//      gtk_window_fullscreen(win);
	gtk_window_set_gravity(win, GDK_GRAVITY_STATIC);
	gtk_window_set_has_frame(win, FALSE);
//      gtk_window_set_keep_below(win, TRUE);
	gtk_window_move(win, 1, 1);
	gtk_window_set_opacity(win, 1.0);
	gtk_window_set_position(win, GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_resizable(win, FALSE);
	gtk_window_resize(win, xscr->width - 2, xscr->height - 2);
	gtk_window_set_skip_pager_hint(win, TRUE);
	gtk_window_set_skip_taskbar_hint(win, TRUE);
	gtk_window_stick(win);
	gtk_window_set_type_hint(win, GDK_WINDOW_TYPE_HINT_DESKTOP);
//      gtk_window_set_app_paintable(win, TRUE);
	if (!gtk_widget_get_double_buffered(GTK_WIDGET(win)))
		gtk_widget_set_double_buffered(GTK_WIDGET(win), TRUE);
	gtk_widget_set_size_request(GTK_WIDGET(win), xscr->width - 2, xscr->height - 2);

	geometry = g_strdup_printf("%dx%d+1+1", xscr->width - 2, xscr->height - 2);
	gtk_window_parse_geometry(win, geometry);
	g_free(geometry);

	gtk_widget_add_events(GTK_WIDGET(win),
			      GDK_BUTTON_PRESS_MASK |
			      GDK_BUTTON_RELEASE_MASK |
			      GDK_BUTTON1_MOTION_MASK |
			      GDK_BUTTON2_MOTION_MASK | GDK_BUTTON3_MOTION_MASK);
	gtk_widget_realize(GTK_WIDGET(win));
	window = gtk_widget_get_window(GTK_WIDGET(win));
	gdk_window_set_override_redirect(window, TRUE);
	gdk_window_set_back_pixmap(window, NULL, TRUE);

	g_signal_connect(G_OBJECT(win), "button_press_event", G_CALLBACK(button_press_event), xscr);
	g_signal_connect(G_OBJECT(win), "button_release_event", G_CALLBACK(button_release_event), xscr);
//	g_signal_connect(G_OBJECT(win), "motion_notify_event", G_CALLBACK(motion_notify_event), xscr);
//	g_signal_connect(G_OBJECT(win), "expose_event", G_CALLBACK(expose_event), xscr);
	g_signal_connect(G_OBJECT(win), "scroll_event", G_CALLBACK(scroll_event), xscr);

//	g_signal_connect(G_OBJECT(win), "drag_drop", G_CALLBACK(drag_drop), xscr);
//	g_signal_connect(G_OBJECT(win), "drag_data_received", G_CALLBACK(drag_data_received), xscr);
//	g_signal_connect(G_OBJECT(win), "drag_motion", G_CALLBACK(drag_motion), xscr);
//	g_signal_connect(G_OBJECT(win), "drag_leave", G_CALLBACK(drag_leave), xscr);

//	g_signal_connect(G_OBJECT(win), "enter_notify_event", G_CALLBACK(enter_notify_event), xscr);
//	g_signal_connect(G_OBJECT(win), "focus_in_event", G_CALLBACK(focus_in_event), xscr);
//	g_signal_connect(G_OBJECT(win), "focus_out_event", G_CALLBACK(focus_out_event), xscr);
//	g_signal_connect(G_OBJECT(win), "grab_broken_event", G_CALLBACK(grab_broken_event), xscr);
//	g_signal_connect(G_OBJECT(win), "grab_focus", G_CALLBACK(grab_focus), xscr);
	g_signal_connect(G_OBJECT(win), "key_press_event", G_CALLBACK(key_press_event), xscr);
	g_signal_connect(G_OBJECT(win), "key_release_event", G_CALLBACK(key_release_event), xscr);
//	g_signal_connect(G_OBJECT(win), "leave_notify_event", G_CALLBACK(leave_notify_event), xscr);
//	g_signal_connect(G_OBJECT(win), "map_event", G_CALLBACK(map_event), xscr);
//	g_signal_connect(G_OBJECT(win), "realize", G_CALLBACK(widget_realize), xscr);

	GtkWidget *aln = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_container_set_border_width(GTK_CONTAINER(win), 0);;;;;
	gtk_container_add(GTK_CONTAINER(win), aln);

	GtkTable *tab = GTK_TABLE(gtk_table_new(1, 1, TRUE));
	gtk_table_set_col_spacings(tab, 0);
	gtk_table_set_row_spacings(tab, 0);
	gtk_table_set_homogeneous(tab, TRUE);
#define ICON_WIDE 80
#define ICON_HIGH 80
	gtk_widget_set_size_request(GTK_WIDGET(tab), ICON_WIDE, ICON_HIGH);
//	gtk_widget_set_tooltip_text(GTK_WIDGET(tab), "Click Me!");

	gtk_container_add(GTK_CONTAINER(aln), GTK_WIDGET(tab));

	update_root_pixmap(xscr, None);

	gtk_widget_show(GTK_WIDGET(tab));
	gtk_widget_show(GTK_WIDGET(aln));
	gtk_widget_show(GTK_WIDGET(win));

	gdk_window_lower(window);



	(void) button_press_event;
	(void) button_release_event;
	(void) enter_notify_event;
	(void) focus_in_event;
	(void) focus_out_event;
	(void) grab_broken_event;
	(void) grab_focus;
	(void) key_press_event;
	(void) key_release_event;
	(void) leave_notify_event;
	(void) map_event;
	(void) widget_realize;
	(void) scroll_event;
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
update_root_pixmap(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;
	Pixmap pmap = None;

	DPRINT();
	if (prop == None || prop == _XA_ESETROOT_PMAP_ID) {
		if (XGetWindowProperty
				(dpy, root, _XA_ESETROOT_PMAP_ID, 0, 1, False, AnyPropertyType, &actual,
				 &format, &nitems, &after, (unsigned char **)&data) == Success
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
				 &format, &nitems, &after, (unsigned char **)&data) == Success
				&& format == 32 && actual && nitems >= 1 && data) {
			pmap = data[0];
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
	}
	if (pmap && xscr->pixmap != pmap) {
		DPRINTF("root pixmap changed from 0x%08lx to 0x%08lx\n", xscr->pixmap, pmap);
		xscr->pixmap = pmap;
		/* FIXME: do more */
		/* Adjust the style of the desktop to use the pixmap specified by
		   _XROOTPMAP_ID as the background.  Uses GTK+ 2.0 styles to do this.
		   The root _XROOTPMAP_ID must be retrieved before calling this function
		   for it to work correctly.  */
	}
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

static void
init_wnck(XdeScreen *xscr)
{
	WnckScreen *wnck = xscr->wnck = wnck_screen_get(xscr->index);

	g_signal_connect(G_OBJECT(wnck), "workspace_destroyed",
			 G_CALLBACK(workspace_destroyed), xscr);
	g_signal_connect(G_OBJECT(wnck), "workspace_created",
			 G_CALLBACK(workspace_created), xscr);
	g_signal_connect(G_OBJECT(wnck), "window_manager_changed",
			 G_CALLBACK(window_manager_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "viewports_changed",
			 G_CALLBACK(viewports_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "background_changed",
			 G_CALLBACK(background_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "active_workspace_changed",
			 G_CALLBACK(active_workspace_changed), xscr);

	g_signal_connect(G_OBJECT(wnck), "active_window_changed",
			G_CALLBACK(active_window_changed), xscr);
	g_signal_connect(G_OBJECT(wnck), "application_closed",
			G_CALLBACK(application_closed), xscr);
	g_signal_connect(G_OBJECT(wnck), "application_opened",
			G_CALLBACK(application_opened), xscr);
	g_signal_connect(G_OBJECT(wnck), "class_group_closed",
			G_CALLBACK(class_group_closed), xscr);
	g_signal_connect(G_OBJECT(wnck), "class_group_opened",
			G_CALLBACK(class_group_opened), xscr);
	g_signal_connect(G_OBJECT(wnck), "window_closed",
			G_CALLBACK(window_closed), xscr);
	g_signal_connect(G_OBJECT(wnck), "window_opened",
			G_CALLBACK(window_opened), xscr);
	g_signal_connect(G_OBJECT(wnck), "window_stacking_changed",
			G_CALLBACK(window_stacking_changed), xscr);

	wnck_screen_force_update(wnck);
	window_manager_changed(wnck, xscr);
}

static GdkFilterReturn selwin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);
static GdkFilterReturn root_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);
static void update_layout(XdeScreen *xscr, Atom prop);
static void update_theme(XdeScreen *xscr, Atom prop);

static void
do_run(int argc, char *argv[], Bool replace)
{
	GdkDisplay *disp = gdk_display_get_default();
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	GdkWindow *root = gdk_screen_get_root_window(scrn), *sel;
	char selection[64] = { 0, };
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
		init_wnck(xscr);
		init_monitors(xscr);
		init_window(xscr);
		if (options.proxy)
			setup_button_proxy(xscr);
		update_root_pixmap(xscr, None);
		update_layout(xscr, None);
		update_current_desktop(xscr, None);
		update_theme(xscr, None);
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
	DPRINT();
	get_selection(True, None);
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
		/* FIXME: do somthing more about it. */
	}
}

static void
refresh_layout(XdeScreen *xscr)
{
	unsigned int w, h, f, wmax, hmax;

	w = xscr->width * xscr->cols;
	h = xscr->height * xscr->rows;
	wmax = (xscr->width * 8) / 10;
	hmax = (xscr->height * 8) / 10;
	for (f = 10; w > wmax * f || h > hmax * f; f++) ;
	gtk_window_set_default_size(GTK_WINDOW(xscr->popup), w / f, h / f);
}

static void
update_layout(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	Atom actual = None;
	int format = 0, num;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;
	Bool propok = False;

	DPRINT();
	if (prop == None || prop == _XA_NET_DESKTOP_LAYOUT) {
		if (XGetWindowProperty
		    (dpy, root, _XA_NET_DESKTOP_LAYOUT, 0, 4, False, AnyPropertyType, &actual,
		     &format, &nitems, &after, (unsigned char **) &data) == Success
		    && format == 32 && actual && nitems >= 4 && data) {
			xscr->rows = data[2];
			xscr->cols = data[1];
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
			xscr->desks = data[0];
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
			xscr->desks = data[0];
		}
		if (data) {
			XFree(data);
			data = NULL;
		}
		propok = True;
	}
	if (!propok)
		EPRINTF("wrong property passed\n");

	if (xscr->desks <= 0)
		xscr->desks = 1;
	if (xscr->desks > 64)
		xscr->desks = 64;
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

	refresh_layout(xscr); /* XXX: should be deferred */
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
		update_theme(xscr, xev->xproperty.atom);
		return GDK_FILTER_REMOVE;	/* event handled */
	} else
	    if (xev->xproperty.atom == _XA_NET_DESKTOP_LAYOUT
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_layout(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_NET_NUMBER_OF_DESKTOPS
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_layout(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_NET_CURRENT_DESKTOP
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_current_desktop(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_WIN_WORKSPACE_COUNT
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_layout(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_WIN_WORKSPACE
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_current_desktop(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_WM_DESKTOP
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_current_desktop(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_XROOTPMAP_ID
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_root_pixmap(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_ESETROOT_PMAP_ID
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_root_pixmap(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_NET_ACTIVE_WINDOW
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_window(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_NET_CLIENT_LIST
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_client_list(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_NET_CLIENT_LIST_STACKING
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_client_list(xscr, xev->xproperty.atom);
	} else if (xev->xproperty.atom == _XA_WIN_FOCUS && xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_window(xscr, xev->xproperty.atom);
	} else
	    if (xev->xproperty.atom == _XA_WIN_CLIENT_LIST
		&& xev->xproperty.state == PropertyNewValue) {
		DPRINT();
		update_client_list(xscr, xev->xproperty.atom);
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
}

static GdkFilterReturn
event_handler_ClientMessage(Display *dpy, XEvent *xev)
{
	XdeScreen *xscr = NULL;
	int s, nscr = ScreenCount(dpy);

	for (s = 0; s < nscr; s++)
		if (xev->xclient.window == RootWindow(dpy, s))
			xscr = screens + s;

	DPRINT();
	if (options.debug > 1) {
		fprintf(stderr, "==> ClientMessage: %p\n", xscr);
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
		fprintf(stderr, "<== ClientMessage: %p\n", xscr);
	}
	if (xscr && xev->xclient.message_type == _XA_GTK_READ_RCFILES) {
		update_theme(xscr, xev->xclient.message_type);
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
	if (xscr && xev->xselectionclear.window == xscr->laywin){
		XDestroyWindow(dpy, xscr->laywin);
		xscr->laywin = None;
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

#if 0
static GdkFilterReturn
laywin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
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
#endif

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
	return GDK_FILTER_CONTINUE;	/* event not handled, continue processing */
}

#if 0
static void
set_current_desktop(XdeScreen *xscr, int index, Time timestamp)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
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
#endif

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

	init_smclient();

	gtk_init(&argc, &argv);

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

	atom = gdk_atom_intern_static_string("_NET_WM_ICON_GEOMETRY");
	_XA_NET_WM_ICON_GEOMETRY = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_DESKTOP_LAYOUT");
	_XA_NET_DESKTOP_LAYOUT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_DESKTOP_NAMES");
	_XA_NET_DESKTOP_NAMES = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_NUMBER_OF_DESKTOPS");
	_XA_NET_NUMBER_OF_DESKTOPS = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_NET_CURRENT_DESKTOP");
	_XA_NET_CURRENT_DESKTOP = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_DESKTOP_BUTTON_PROXY");
	_XA_WIN_DESKTOP_BUTTON_PROXY = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKSPACE_COUNT");
	_XA_WIN_WORKSPACE_COUNT = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_WIN_WORKSPACE");
	_XA_WIN_WORKSPACE = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("WM_DESKTOP");
	_XA_WM_DESKTOP = gdk_x11_atom_to_xatom_for_display(disp, atom);

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

#if 0
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
#endif

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
    -t, --timeout MILLISECONDS\n\
        specify timeout when not modifier [default: %6$lu]\n\
    -B, --border PIXELS\n\
        border surrounding feedback popup [default: %7$u]\n\
    -p, --proxy\n\
        respond to button proxy [default: %8$s]\n\
    -b, --button BUTTON\n\
        specify the mouse button number, BUTTON, for popup [default: %9$d]\n\
    -T, --timestamp TIMESTAMP\n\
        use the time, TIMESTAMP, for button/keyboard event [default: %10$lu]\n\
    -k, --keys FORWARD:REVERSE\n\
        specify keys for cycling [default: %11$s]\n\
    -c, --cycle\n\
        show a window cycle list [default: %12$s]\n\
    --hidden\n\
        list hidden windows as well [default: %13$s]\n\
    --minimized\n\
        list minimized windows as well [default: %14$s]\n\
    --all-monitors\n\
        list windows on all monitors [deefault: %15$s]\n\
    --all-workspaces\n\
        list windows on all workspaces [default: %16$s]\n\
    -O, --order {client|stacking}\n\
        specify the order of windows [default: %17$s]\n\
    -n, --noactivate\n\
        do not activate windows [default: %18$s]\n\
    --raise\n\
        raise windows when selected/cycling [default: %19$s]\n\
    -R, --restore\n\
        restore previous windows when cycling [default: %20$s]\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %2$d]\n\
        this option may be repeated.\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %3$d]\n\
        this option may be repeated.\n\
Session Management:\n\
    -clientID CLIENTID\n\
        client id for session management [default: %21$s]\n\
    -restore SAVEFILE\n\
        file in which to save session info [default: %22$s]\n\
", argv[0] 
	, options.debug
	, options.output
	, options.display
	, options.screen
	, options.timeout
	, options.border
	, show_bool(options.proxy)
	, options.button
	, options.timestamp
	, options.keys ?: "AC+Left:AC+Right"
	, show_bool(options.cycle)
	, show_bool(options.hidden)
	, show_bool(options.minimized)
	, show_bool(options.monitors)
	, show_bool(options.workspaces)
	, show_order(options.order)
	, show_bool(!options.activate)
	, show_bool(options.raise)
	, show_bool(options.restore)
	, options.clientId
	, options.saveFile
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
		int c, val, len;
		char *endptr = NULL;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"display",	required_argument,	NULL,	'd'},
			{"screen",	required_argument,	NULL,	's'},
			{"timeout",	required_argument,	NULL,	't'},
			{"border",	required_argument,	NULL,	'B'},
			{"proxy",	no_argument,		NULL,	'p'},
			{"button",	required_argument,	NULL,	'b'},
			{"timestamp",	required_argument,	NULL,	'T'},
			{"which",	required_argument,	NULL,	'w'},
			{"where",	required_argument,	NULL,	'W'},

			{"cycle",	no_argument,		NULL,	'c'},
			{"hidden",	no_argument,		NULL,	'1'},
			{"minimized",	no_argument,		NULL,	'm'},
			{"all-monitors", no_argument,		NULL,	'2'},
			{"all-workspaces", no_argument,		NULL,	'3'},
			{"order",	optional_argument,	NULL,	'O'},
			{"noactivate",	no_argument,		NULL,	'n'},
			{"raise",	no_argument,		NULL,	'4'},
			{"restore",	no_argument,		NULL,	'R'},
			{"key",		optional_argument,	NULL,	'k'},

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

		c = getopt_long_only(argc, argv, "d:s:t:B:pb:T:cmO::nRk::qrD::v::hVCH?",
				     long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:t:B:pb:T:cmO:nRk:qrD:vhVCH?");
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
			if (endptr && *endptr)
				goto bad_option;
			break;
		case 't':	/* -t, --timeout MILLISECONDS */
			options.timeout = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			if (!options.timeout)
				goto bad_option;
			break;
		case 'B':	/* -B, --border PIXELS */
			options.border = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			if (options.border > 20)
				goto bad_option;
			break;
		case 'p':	/* -p, --proxy */
			options.proxy = True;
			break;

		case 'b':	/* -b, --button BUTTON */
			options.button = strtoul(optarg, &endptr, 0);
			if (endptr && *endptr)
				goto bad_option;
			break;
		case 'T':	/* -T, --timestamp TIMESTAMP */
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

		case 'c':	/* -c, --cycle */
			options.cycle = True;
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
		case 'O':	/* -O, --order ORDERTYPE */
			if (options.order != WindowOrderDefault)
				goto bad_option;
			if (!strcasecmp("client", optarg))
				options.order = WindowOrderClient;
			else
			if (!strcasecmp("stacking", optarg))
				options.order = WindowOrderStacking;
			else
				goto bad_option;
			break;
		case 'n':	/* -n, --noactivate */
			options.activate = False;
			break;
		case '4':	/* --raise */
			options.raise = True;
			break;
		case 'R':	/* -R, --restore */
			options.restore = True;
			break;
		case 'k':	/* -k, --key [KEY1:KEY2] */
			free(options.keys);
			options.keys = strdup(optarg);
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
			if (endptr && *endptr)
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

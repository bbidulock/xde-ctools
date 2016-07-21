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
#define NEED_POPUP_WINDOW		0
#define NEED_STARTUP_IDS		0

#define NEED_PAGER			0
#define NEED_TASKS			0
#define NEED_MODEL			0

#define XA_PREFIX		"_XDE_INPUT"
#define XA_SELECTION_NAME	XA_PREFIX "_S%d"
#define XA_NET_DESKTOP_LAYOUT	"_NET_DESKTOP_LAYOUT_S%d"
#define LOGO_NAME		"input-keyboard"

static int saveArgc;
static char **saveArgv;

#define RESNAME "xde-input"
#define RESCLAS "XDE-Input"
#define RESTITL "X11 Input"

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
static Atom _XA_NET_ACTIVE_WINDOW;
static Atom _XA_NET_CLIENT_LIST;
static Atom _XA_NET_CLIENT_LIST_STACKING;
static Atom _XA_WIN_FOCUS;
static Atom _XA_WIN_CLIENT_LIST;
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
	XdeImage **desktops;		/* the desktops */
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

typedef struct {
	Bool Keyboard;			/* support for core Keyboard */
	Bool Pointer;			/* support for core Pointer */
	Bool ScreenSaver;		/* support for extension ScreenSaver */
	Bool DPMS;			/* support for extension DPMS */
	Bool XKeyboard;			/* support for extension XKEYBOARD */
	Bool XF86Misc;			/* support for extension XF86MISC */
	Bool RANDR;			/* suppott for extension RANDR */
} Support;

Support support;

typedef struct {
	XKeyboardState Keyboard;
	struct {
		int accel_numerator;
		int accel_denominator;
		int threshold;
	} Pointer;
	struct {
		int opcode;
		int event;
		int error;
		int major_version;
		int minor_version;
		XkbDescPtr desc;
	} XKeyboard;
	struct {
		int event;		/* event base */
		int error;		/* error base */
		int major_version;
		int minor_version;
		int timeout;
		int interval;
		int prefer_blanking;
		int allow_exposures;
		XScreenSaverInfo info;
	} ScreenSaver;
	struct {
		int event;		/* event base */
		int error;		/* error base */
		int major_version;
		int minor_version;
		CARD16 power_level;
		BOOL state;
		CARD16 standby;
		CARD16 suspend;
		CARD16 off;
	} DPMS;
	struct {
		int event;		/* event base */
		int error;		/* error base */
		int major_version;
		int minor_version;
		XF86MiscMouseSettings mouse;
		XF86MiscKbdSettings keyboard;
	} XF86Misc;
	struct {
		int event;		/* event base */
		int error;		/* error base */
		int major_version;
		int minor_version;
	} RANDR;
} State;

State state;

GKeyFile *file = NULL;

typedef struct {
	struct {
		GtkWidget *BellPercent;
	} Icon;
	struct {
		GtkWidget *GlobalAutoRepeat;
		GtkWidget *KeyClickPercent;
		GtkWidget *BellPercent;
		GtkWidget *BellPitch;
		GtkWidget *BellDuration;
	} Keyboard;
	struct {
		GtkWidget *AccelerationNumerator;
		GtkWidget *AccelerationDenominator;
		GtkWidget *Threshold;
	} Pointer;
	struct {
		GtkWidget *Timeout;
		GtkWidget *Interval;
		GtkWidget *Preferblanking[3];
		GtkWidget *Allowexposures[3];
	} ScreenSaver;
	struct {
		GtkWidget *State;
		GtkWidget *StandbyTimeout;
		GtkWidget *SuspendTimeout;
		GtkWidget *OffTimeout;
	} DPMS;
	struct {
		GtkWidget *RepeatKeysEnabled;
		GtkWidget *RepeatDelay;
		GtkWidget *RepeatInterval;
		GtkWidget *SlowKeysEnabled;
		GtkWidget *SlowKeysDelay;
		GtkWidget *BounceKeysEnabled;
		GtkWidget *DebounceDelay;
		GtkWidget *StickyKeysEnabled;
		GtkWidget *MouseKeysEnabled;
		GtkWidget *MouseKeysDfltBtn;
		GtkWidget *MouseKeysAccelEnabled;
		GtkWidget *MouseKeysDelay;
		GtkWidget *MouseKeysInterval;
		GtkWidget *MouseKeysTimeToMax;
		GtkWidget *MouseKeysMaxSpeed;
		GtkWidget *MouseKeysCurve;
	} XKeyboard;
	struct {
		GtkWidget *KeyboardRate;
		GtkWidget *KeyboardDelay;
		GtkWidget *MouseEmulate3Buttons;
		GtkWidget *MouseEmulate3Timeout;
		GtkWidget *MouseChordMiddle;
	} XF86Misc;
} Controls;

Controls controls;

GtkWindow *editor = NULL;
GtkStatusIcon *trayicon = NULL;

typedef struct {
	struct {
		Bool GlobalAutoRepeat;
		int KeyClickPercent;
		int BellPercent;
		unsigned int BellPitch;
		unsigned int BellDuration;
	} Keyboard;
	struct {
		unsigned int AccelerationNumerator;
		unsigned int AccelerationDenominator;
		unsigned int Threshold;
	} Pointer;
	struct {
		unsigned int Timeout;
		unsigned int Interval;
		unsigned int Preferblanking;
		unsigned int Allowexposures;
	} ScreenSaver;
	struct {
		int State;
		unsigned int StandbyTimeout;
		unsigned int SuspendTimeout;
		unsigned int OffTimeout;
	} DPMS;
	struct {
		Bool RepeatKeysEnabled;
		unsigned int RepeatDelay;
		unsigned int RepeatInterval;
		Bool SlowKeysEnabled;
		unsigned int SlowKeysDelay;
		Bool BounceKeysEnabled;
		unsigned int DebounceDelay;
		Bool StickyKeysEnabled;
		Bool MouseKeysEnabled;
		unsigned int MouseKeysDfltBtn;
		Bool MouseKeysAccelEnabled;
		unsigned int MouseKeysDelay;
		unsigned int MouseKeysInterval;
		unsigned int MouseKeysTimeToMax;
		unsigned int MouseKeysMaxSpeed;
		unsigned int MouseKeysCurve;
	} XKeyboard;
	struct {
		unsigned int KeyboardRate;
		unsigned int KeyboardDelay;
		Bool MouseEmulate3Buttons;
		unsigned int MouseEmulate3Timeout;
		Bool MouseChordMiddle;
	} XF86Misc;
} Resources;

Resources resources = {
};

static const char *KFG_Pointer = "Pointer";
static const char *KFG_Keyboard = "Keyboard";
static const char *KFG_XKeyboard = "XKeyboard";
static const char *KFG_ScreenSaver = "ScreenSaver";
static const char *KFG_DPMS = "DPMS";
static const char *KFG_XF86Misc = "XF86Misc";

static const char *KFK_Pointer_AccelerationDenominator = "AccelerationDenominator";
static const char *KFK_Pointer_AccelerationNumerator = "AccelerationNumerator";
static const char *KFK_Pointer_Threshold = "Threshold";

const char *KFK_Keyboard_AutoRepeats = "AutoRepeats";
static const char *KFK_Keyboard_BellDuration = "BellDuration";
static const char *KFK_Keyboard_BellPercent = "BellPercent";
static const char *KFK_Keyboard_BellPitch = "BellPitch";
static const char *KFK_Keyboard_GlobalAutoRepeat = "GlobalAutoRepeat";
static const char *KFK_Keyboard_KeyClickPercent = "KeyClickPercent";
const char *KFK_Keyboard_LEDMask = "LEDMask";

static const char *KFK_XKeyboard_AccessXFeedbackMaskEnabled = "AccessXFeedbackMaskEnabled";
static const char *KFK_XKeyboard_AccessXKeysEnabled = "AccessXKeysEnabled";
static const char *KFK_XKeyboard_AccessXOptions = "AccessXOptions";
static const char *KFK_XKeyboard_AccessXOptionsEnabled = "AccessXOptionsEnabled";
static const char *KFK_XKeyboard_AccessXTimeout = "AccessXTimeout";
static const char *KFK_XKeyboard_AccessXTimeoutMask = "AccessXTimeoutMask";
static const char *KFK_XKeyboard_AccessXTimeoutMaskEnabled = "AccessXTimeoutMaskEnabled";
static const char *KFK_XKeyboard_AccessXTimeoutOptionsMask = "AccessXTimeoutOptionsMask";
static const char *KFK_XKeyboard_AccessXTimeoutOptionsValues = "AccessXTimeoutOptionsValues";
static const char *KFK_XKeyboard_AccessXTimeoutValues = "AccessXTimeoutValues";
static const char *KFK_XKeyboard_AudibleBellMaskEnabled = "AudibleBellMaskEnabled";
static const char *KFK_XKeyboard_BounceKeysEnabled = "BounceKeysEnabled";
static const char *KFK_XKeyboard_ControlsEnabledEnabled = "ControlsEnabledEnabled";
static const char *KFK_XKeyboard_DebounceDelay = "DebounceDelay";
static const char *KFK_XKeyboard_GroupsWrapEnabled = "GroupsWrapEnabled";
static const char *KFK_XKeyboard_IgnoreGroupLockModsEnabled = "IgnoreGroupLockModsEnabled";
static const char *KFK_XKeyboard_IgnoreLockModsEnabled = "IgnoreLockModsEnabled";
static const char *KFK_XKeyboard_InternalModsEnabled = "InternalModsEnabled";
static const char *KFK_XKeyboard_MouseKeysAccelEnabled = "MouseKeysAccelEnabled";
static const char *KFK_XKeyboard_MouseKeysCurve = "MouseKeysCurve";
static const char *KFK_XKeyboard_MouseKeysDelay = "MouseKeysDelay";
static const char *KFK_XKeyboard_MouseKeysDfltBtn = "MouseKeysDfltBtn";
static const char *KFK_XKeyboard_MouseKeysEnabled = "MouseKeysEnabled";
static const char *KFK_XKeyboard_MouseKeysInterval = "MouseKeysInterval";
static const char *KFK_XKeyboard_MouseKeysMaxSpeed = "MouseKeysMaxSpeed";
static const char *KFK_XKeyboard_MouseKeysTimeToMax = "MouseKeysTimeToMax";
static const char *KFK_XKeyboard_Overlay1MaskEnabled = "Overlay1MaskEnabled";
static const char *KFK_XKeyboard_Overlay2MaskEnabled = "Overlay2MaskENabled";
static const char *KFK_XKeyboard_PerKeyRepeatEnabled = "PerKeyRepeatEnabled";
const char *KFK_XKeyboard_PerKeyRepeat = "PerKeyRepeat";
static const char *KFK_XKeyboard_RepeatDelay = "RepeatDelay";
static const char *KFK_XKeyboard_RepeatInterval = "RepeatInterval";
static const char *KFK_XKeyboard_RepeatKeysEnabled = "RepeatKeysEnabled";
const char *KFK_XKeyboard_RepeatRate = "RepeatRate";
static const char *KFK_XKeyboard_SlowKeysDelay = "SlowKeysDelay";
static const char *KFK_XKeyboard_SlowKeysEnabled = "SlowKeysEnabled";
static const char *KFK_XKeyboard_StickyKeysEnabled = "StickyKeysEnabled";

static const char *KFK_ScreenSaver_AllowExposures = "AllowExposures";
static const char *KFK_ScreenSaver_Interval = "Interval";
static const char *KFK_ScreenSaver_PreferBlanking = "PreferBlanking";
static const char *KFK_ScreenSaver_Timeout = "Timeout";

static const char *KFK_DPMS_OffTimeout = "OffTimeout";
static const char *KFK_DPMS_PowerLevel = "PowerLevel";
static const char *KFK_DPMS_StandbyTimeout = "StandbyTimeout";
static const char *KFK_DPMS_State = "State";
static const char *KFK_DPMS_SuspendTimeout = "SuspendTimeout";

static const char *KFK_XF86Misc_KeyboardRate = "KeyboardRate";
static const char *KFK_XF86Misc_KeyboardDelay = "KeyboardDelay";
static const char *KFK_XF86Misc_MouseEmulate3Buttons = "MouseEmulate3Buttons";
static const char *KFK_XF86Misc_MouseEmulate3Timeout = "MouseEmulate3Timeout";
static const char *KFK_XF86Misc_MouseChordMiddle = "MouseChordMiddle";

static void
edit_set_values()
{
	int value;
	gboolean flag;
	char *str;

	if (!file) {
		EPRINTF("no key file!\n");
		return;
	}
	if (support.Keyboard) {
		value = g_key_file_get_integer(file, KFG_Keyboard, KFK_Keyboard_KeyClickPercent, NULL);
		gtk_range_set_value(GTK_RANGE(controls.Keyboard.KeyClickPercent), value);
		value = g_key_file_get_integer(file, KFG_Keyboard, KFK_Keyboard_BellPercent, NULL);
		gtk_range_set_value(GTK_RANGE(controls.Keyboard.BellPercent), value);
		if (controls.Icon.BellPercent)
			gtk_range_set_value(GTK_RANGE(controls.Icon.BellPercent), value);
		value = g_key_file_get_integer(file, KFG_Keyboard, KFK_Keyboard_BellPitch, NULL);
		gtk_range_set_value(GTK_RANGE(controls.Keyboard.BellPitch), value);
		value = g_key_file_get_integer(file, KFG_Keyboard, KFK_Keyboard_BellDuration, NULL);
		gtk_range_set_value(GTK_RANGE(controls.Keyboard.BellDuration), value);
		flag = g_key_file_get_boolean(file, KFG_Keyboard, KFK_Keyboard_GlobalAutoRepeat, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.Keyboard.GlobalAutoRepeat), flag);
	}
	if (support.Pointer) {
		value = g_key_file_get_integer(file, KFG_Pointer, KFK_Pointer_AccelerationNumerator, NULL);
		gtk_range_set_value(GTK_RANGE(controls.Pointer.AccelerationNumerator), value);
		value = g_key_file_get_integer(file, KFG_Pointer, KFK_Pointer_AccelerationDenominator, NULL);
		gtk_range_set_value(GTK_RANGE(controls.Pointer.AccelerationDenominator), value);
		value = g_key_file_get_integer(file, KFG_Pointer, KFK_Pointer_Threshold, NULL);
		gtk_range_set_value(GTK_RANGE(controls.Pointer.Threshold), value);
	}
	if (support.XKeyboard) {
		flag = g_key_file_get_boolean(file, KFG_XKeyboard, KFK_XKeyboard_RepeatKeysEnabled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.XKeyboard.RepeatKeysEnabled), flag);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_RepeatDelay, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XKeyboard.RepeatDelay), value);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_RepeatInterval, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XKeyboard.RepeatInterval), value);

		flag = g_key_file_get_boolean(file, KFG_XKeyboard, KFK_XKeyboard_SlowKeysEnabled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.XKeyboard.SlowKeysEnabled), flag);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_SlowKeysDelay, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XKeyboard.SlowKeysDelay), value);

		flag = g_key_file_get_boolean(file, KFG_XKeyboard, KFK_XKeyboard_StickyKeysEnabled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.XKeyboard.StickyKeysEnabled), flag);

		flag = g_key_file_get_boolean(file, KFG_XKeyboard, KFK_XKeyboard_BounceKeysEnabled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.XKeyboard.BounceKeysEnabled), flag);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_DebounceDelay, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XKeyboard.DebounceDelay), value);

		flag = g_key_file_get_boolean(file, KFG_XKeyboard, KFK_XKeyboard_MouseKeysEnabled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.XKeyboard.MouseKeysEnabled), flag);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_MouseKeysDfltBtn, NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(controls.XKeyboard.MouseKeysDfltBtn), value - 1);

		flag = g_key_file_get_boolean(file, KFG_XKeyboard, KFK_XKeyboard_MouseKeysAccelEnabled, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.XKeyboard.MouseKeysAccelEnabled), flag);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_MouseKeysDelay, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XKeyboard.MouseKeysDelay), value);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_MouseKeysInterval, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XKeyboard.MouseKeysInterval), value);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_MouseKeysTimeToMax, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XKeyboard.MouseKeysTimeToMax), value);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_MouseKeysMaxSpeed, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XKeyboard.MouseKeysMaxSpeed), value);
		value = g_key_file_get_integer(file, KFG_XKeyboard, KFK_XKeyboard_MouseKeysCurve, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XKeyboard.MouseKeysCurve), value);
	}
	if (support.ScreenSaver) {
		value = g_key_file_get_integer(file, KFG_ScreenSaver, KFK_ScreenSaver_Timeout, NULL);
		gtk_range_set_value(GTK_RANGE(controls.ScreenSaver.Timeout), value);
		value = g_key_file_get_integer(file, KFG_ScreenSaver, KFK_ScreenSaver_Interval, NULL);
		gtk_range_set_value(GTK_RANGE(controls.ScreenSaver.Interval), value);
		str = g_key_file_get_string(file, KFG_ScreenSaver, KFK_ScreenSaver_PreferBlanking, NULL);
		if (str && strcmp(str, "DontPreferBlanking") == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Preferblanking[1]),
						     TRUE);
		} else if (str && strcmp(str, "PreferBlanking") == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Preferblanking[2]),
						     TRUE);
		} else {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Preferblanking[0]),
						     TRUE);
		}
		if (str)
			g_free(str);
		str = g_key_file_get_string(file, KFG_ScreenSaver, KFK_ScreenSaver_AllowExposures, NULL);
		if (str && strcmp(str, "DontAllowExposures") == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Allowexposures[1]),
						     TRUE);
		} else if (str && strcmp(str, "AllowExposures") == 0) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Allowexposures[2]),
						     TRUE);
		} else {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Allowexposures[0]),
						     TRUE);
		}
		if (str)
			g_free(str);
	}
	if (support.DPMS) {
		value = g_key_file_get_integer(file, KFG_DPMS, KFK_DPMS_StandbyTimeout, NULL);
		gtk_range_set_value(GTK_RANGE(controls.DPMS.StandbyTimeout), value);
		value = g_key_file_get_integer(file, KFG_DPMS, KFK_DPMS_SuspendTimeout, NULL);
		gtk_range_set_value(GTK_RANGE(controls.DPMS.SuspendTimeout), value);
		value = g_key_file_get_integer(file, KFG_DPMS, KFK_DPMS_OffTimeout, NULL);
		gtk_range_set_value(GTK_RANGE(controls.DPMS.OffTimeout), value);
		flag = g_key_file_get_boolean(file, KFG_DPMS, KFK_DPMS_State, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.DPMS.State), flag);
	}
	if (support.XF86Misc) {
		value = g_key_file_get_integer(file, KFG_XF86Misc, KFK_XF86Misc_KeyboardRate, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XF86Misc.KeyboardRate), value);
		value = g_key_file_get_integer(file, KFG_XF86Misc, KFK_XF86Misc_KeyboardDelay, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XF86Misc.KeyboardDelay), value);
		flag = g_key_file_get_boolean(file, KFG_XF86Misc, KFK_XF86Misc_MouseEmulate3Buttons, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.XF86Misc.MouseEmulate3Buttons), flag);
		value = g_key_file_get_integer(file, KFG_XF86Misc, KFK_XF86Misc_MouseEmulate3Timeout, NULL);
		gtk_range_set_value(GTK_RANGE(controls.XF86Misc.MouseEmulate3Timeout), value);
		flag = g_key_file_get_boolean(file, KFG_XF86Misc, KFK_XF86Misc_MouseChordMiddle, NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(controls.XF86Misc.MouseChordMiddle), flag);
	}
}

/** @brief read input settings
  * 
  * Read the input settings from the configuration file.  Simple and direct.
  * The file is an .ini-style keyfile.  Use glib to read in the values.
  */
static void
read_input(void)
{
	GError *error = NULL;
	gchar *filename;
	const gchar *const *dirs;

	PTRACE(5);
	if (!file && !(file = g_key_file_new())) {
		EPRINTF("could not create key file\n");
		return;
	}
	filename = options.filename ? g_strdup(options.filename) :
	    g_build_filename(g_get_user_config_dir(), "xde", "input.ini", NULL);
	if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
		DPRINTF(1, "file %s does not yet exist\n", filename);
		g_free(filename);
		filename = NULL;
		for (dirs = g_get_system_config_dirs(); dirs && *dirs; dirs++) {
			filename = g_build_filename(dirs[0], "xde", "input.ini", NULL);
			if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
				break;
			g_free(filename);
			filename = NULL;
		}
	}
	if (!filename) {
		DPRINTF(1, "could not find input file\n");
		return;
	}
	g_key_file_load_from_file(file, filename, G_KEY_FILE_NONE, &error);
	if (error) {
		EPRINTF("could not load key file %s: %s\n", filename, error->message);
		g_error_free(error);
	}
	g_free(filename);
}

static void
edit_get_values()
{
	char buf[256] = { 0, };
	int i, j;

	PTRACE(5);
	if (!file && !(file = g_key_file_new())) {
		EPRINTF("could not create key file\n");
		exit(EXIT_FAILURE);
	}
	if (support.Keyboard) {
		XGetKeyboardControl(dpy, &state.Keyboard);

		if (options.debug) {
			fputs("Keyboard Control:\n", stderr);
			fprintf(stderr, "\tkey-click-percent: %d\n", state.Keyboard.key_click_percent);
			fprintf(stderr, "\tbell-percent: %d\n", state.Keyboard.bell_percent);
			fprintf(stderr, "\tbell-pitch: %u Hz\n", state.Keyboard.bell_pitch);
			fprintf(stderr, "\tbell-duration: %u milliseconds\n", state.Keyboard.bell_duration);
			fprintf(stderr, "\tled-mask: 0x%08lx\n", state.Keyboard.led_mask);
			fprintf(stderr, "\tglobal-auto-repeat: %s\n",
				state.Keyboard.global_auto_repeat ? "Yes" : "No");
			fputs("\tauto-repeats: ", stderr);
			for (i = 0; i < 32; i++)
				fprintf(stderr, "%02X", state.Keyboard.auto_repeats[i]);
			fputs("\n", stderr);
		}

		resources.Keyboard.KeyClickPercent = state.Keyboard.key_click_percent;
		g_key_file_set_integer(file, KFG_Keyboard,
				       KFK_Keyboard_KeyClickPercent, state.Keyboard.key_click_percent);

		resources.Keyboard.BellPercent = state.Keyboard.bell_percent;
		g_key_file_set_integer(file, KFG_Keyboard, KFK_Keyboard_BellPercent, state.Keyboard.bell_percent);

		resources.Keyboard.BellPitch = state.Keyboard.bell_pitch;
		g_key_file_set_integer(file, KFG_Keyboard, KFK_Keyboard_BellPitch, state.Keyboard.bell_pitch);

		resources.Keyboard.BellDuration = state.Keyboard.bell_duration;
		g_key_file_set_integer(file, KFG_Keyboard,
				       KFK_Keyboard_BellDuration, state.Keyboard.bell_duration);

		/* FIXME: do resources */
		g_key_file_set_integer(file, KFG_Keyboard, KFK_Keyboard_LEDMask, state.Keyboard.led_mask);

		resources.Keyboard.GlobalAutoRepeat = state.Keyboard.global_auto_repeat ? True : False;
		g_key_file_set_boolean(file, KFG_Keyboard,
				       KFK_Keyboard_GlobalAutoRepeat, state.Keyboard.global_auto_repeat);

		/* FIXME: do resources */
		for (i = 0, j = 0; i < 32; i++, j += 2)
			snprintf(buf + j, sizeof(buf) - j, "%02X", state.Keyboard.auto_repeats[i]);
		g_key_file_set_string(file, KFG_Keyboard, KFK_Keyboard_AutoRepeats, buf);
	}

	if (support.Pointer) {
		XGetPointerControl(dpy, &state.Pointer.accel_numerator,
				   &state.Pointer.accel_denominator, &state.Pointer.threshold);

		if (options.debug) {
			fputs("Pointer Control:\n", stderr);
			fprintf(stderr, "\tacceleration-numerator: %d\n", state.Pointer.accel_numerator);
			fprintf(stderr, "\tacceleration-denominator: %d\n", state.Pointer.accel_denominator);
			fprintf(stderr, "\tthreshold: %d\n", state.Pointer.threshold);
		}

		resources.Pointer.AccelerationDenominator = state.Pointer.accel_denominator;
		g_key_file_set_integer(file, KFG_Pointer,
				       KFK_Pointer_AccelerationDenominator, state.Pointer.accel_denominator);
		resources.Pointer.AccelerationNumerator = state.Pointer.accel_numerator;
		g_key_file_set_integer(file, KFG_Pointer,
				       KFK_Pointer_AccelerationNumerator, state.Pointer.accel_numerator);
		resources.Pointer.Threshold = state.Pointer.threshold;
		g_key_file_set_integer(file, KFG_Pointer, KFK_Pointer_Threshold, state.Pointer.threshold);
	}

	if (support.ScreenSaver) {

		XGetScreenSaver(dpy, &state.ScreenSaver.timeout, &state.ScreenSaver.interval,
				&state.ScreenSaver.prefer_blanking, &state.ScreenSaver.allow_exposures);

		if (options.debug) {
			fputs("Screen Saver:\n", stderr);
			fprintf(stderr, "\ttimeout: %d seconds\n", state.ScreenSaver.timeout);
			fprintf(stderr, "\tinterval: %d seconds\n", state.ScreenSaver.interval);
			fputs("\tprefer-blanking: ", stderr);
			switch (state.ScreenSaver.prefer_blanking) {
			case DontPreferBlanking:
				fputs("DontPreferBlanking\n", stderr);
				break;
			case PreferBlanking:
				fputs("PreferBlanking\n", stderr);
				break;
			case DefaultBlanking:
				fputs("DefaultBlanking\n", stderr);
				break;
			default:
				fprintf(stderr, "(unknown) %d\n", state.ScreenSaver.prefer_blanking);
				break;
			}
			fputs("\tallow-exposures: ", stderr);
			switch (state.ScreenSaver.allow_exposures) {
			case DontAllowExposures:
				fputs("DontAllowExposures\n", stderr);
				break;
			case AllowExposures:
				fputs("AllowExposures\n", stderr);
				break;
			case DefaultExposures:
				fputs("DefaultExposures\n", stderr);
				break;
			default:
				fprintf(stderr, "(unknown) %d\n", state.ScreenSaver.allow_exposures);
				break;
			}
		}

		switch (state.ScreenSaver.allow_exposures) {
		case DontAllowExposures:
			strncpy(buf, "DontAllowExposures", sizeof(buf));
			break;
		case AllowExposures:
			strncpy(buf, "AllowExposures", sizeof(buf));
			break;
		case DefaultExposures:
			strncpy(buf, "DefaultExposures", sizeof(buf));
			break;
		default:
			snprintf(buf, sizeof(buf), "%d", state.ScreenSaver.allow_exposures);
			break;
		}
		resources.ScreenSaver.Allowexposures = state.ScreenSaver.allow_exposures;
		g_key_file_set_string(file, KFG_ScreenSaver, KFK_ScreenSaver_AllowExposures, buf);

		resources.ScreenSaver.Interval = state.ScreenSaver.interval;
		g_key_file_set_integer(file, KFG_ScreenSaver,
				       KFK_ScreenSaver_Interval, state.ScreenSaver.interval);

		switch (state.ScreenSaver.prefer_blanking) {
		case DontPreferBlanking:
			strncpy(buf, "DontPreferBlanking", sizeof(buf));
			break;
		case PreferBlanking:
			strncpy(buf, "PreferBlanking", sizeof(buf));
			break;
		case DefaultBlanking:
			strncpy(buf, "DefaultBlanking", sizeof(buf));
			break;
		default:
			snprintf(buf, sizeof(buf), "%d", state.ScreenSaver.prefer_blanking);
			break;
		}
		resources.ScreenSaver.Preferblanking = state.ScreenSaver.prefer_blanking;
		g_key_file_set_string(file, KFG_ScreenSaver, KFK_ScreenSaver_PreferBlanking, buf);
		resources.ScreenSaver.Timeout = state.ScreenSaver.timeout;
		g_key_file_set_integer(file, KFG_ScreenSaver, KFK_ScreenSaver_Timeout, state.ScreenSaver.timeout);
	}

	if (support.DPMS) {
		DPMSGetTimeouts(dpy, &state.DPMS.standby, &state.DPMS.suspend, &state.DPMS.off);
		if (options.debug) {
			fputs("DPMS:\n", stderr);
			fprintf(stderr, "\tDPMS Version: %d.%d\n", state.DPMS.major_version,
				state.DPMS.minor_version);
			fputs("\tpower-level: ", stderr);
			switch (state.DPMS.power_level) {
			case DPMSModeOn:
				fputs("DPMSModeOn\n", stderr);
				break;
			case DPMSModeStandby:
				fputs("DPMSModeStandby\n", stderr);
				break;
			case DPMSModeSuspend:
				fputs("DPMSModeSuspend\n", stderr);
				break;
			case DPMSModeOff:
				fputs("DPMSModeOff\n", stderr);
				break;
			default:
				fprintf(stderr, "%d (unknown)\n", state.DPMS.power_level);
				break;
			}
			fprintf(stderr, "\tstate: %s\n", state.DPMS.state ? "True" : "False");
			fprintf(stderr, "\tstandby-timeout: %hu seconds\n", state.DPMS.standby);
			fprintf(stderr, "\tsuspend-timeout: %hu seconds\n", state.DPMS.suspend);
			fprintf(stderr, "\toff-timeout: %hu seconds\n", state.DPMS.off);
		}

		switch (state.DPMS.power_level) {
		case DPMSModeOn:
			strncpy(buf, "DPMSModeOn", sizeof(buf));
			break;
		case DPMSModeStandby:
			strncpy(buf, "DPMSModeStandby", sizeof(buf));
			break;
		case DPMSModeSuspend:
			strncpy(buf, "DPMSModeSuspend", sizeof(buf));
			break;
		case DPMSModeOff:
			strncpy(buf, "DPMSModeOff", sizeof(buf));
			break;
		default:
			snprintf(buf, sizeof(buf), "%d (unknown)", state.DPMS.power_level);
			break;
		}
		/* FIXME: resources.DPMS.PowerLevel = state.DPMS.power_level; */
		g_key_file_set_string(file, KFG_DPMS, KFK_DPMS_PowerLevel, buf);
		resources.DPMS.State = state.DPMS.state;
		g_key_file_set_boolean(file, KFG_DPMS, KFK_DPMS_State, state.DPMS.state ? TRUE : FALSE);
		resources.DPMS.StandbyTimeout = state.DPMS.standby;
		g_key_file_set_integer(file, KFG_DPMS, KFK_DPMS_StandbyTimeout, state.DPMS.standby);
		resources.DPMS.SuspendTimeout = state.DPMS.suspend;
		g_key_file_set_integer(file, KFG_DPMS, KFK_DPMS_SuspendTimeout, state.DPMS.suspend);
		resources.DPMS.OffTimeout = state.DPMS.off;
		g_key_file_set_integer(file, KFG_DPMS, KFK_DPMS_OffTimeout, state.DPMS.off);
	}

	PTRACE(5);
	if (support.XKeyboard) {
		state.XKeyboard.desc = XkbGetKeyboard(dpy, XkbControlsMask, XkbUseCoreKbd);
		if (!state.XKeyboard.desc) {
			EPRINTF("NO XKEYBOARD DESCRIPTION!\n");
			exit(EXIT_FAILURE);
		}
		XkbGetControls(dpy, XkbAllControlsMask, state.XKeyboard.desc);
		if (!state.XKeyboard.desc->ctrls) {
			EPRINTF("NO XKEYBOARD DESCRIPTION CONTROLS!\n");
			exit(EXIT_FAILURE);
		}
#if 0
		unsigned int which = XkbControlsMask;
#endif

		PTRACE(5);
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
				       KFK_XKeyboard_IgnoreGroupLockModsEnabled,
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

		{
			unsigned int repeat_delay, repeat_interval;

			XkbGetAutoRepeatRate(dpy, XkbUseCoreKbd, &repeat_delay, &repeat_interval);

			g_key_file_set_integer(file, KFG_XKeyboard,
					       KFK_XKeyboard_RepeatDelay,
					       state.XKeyboard.desc->ctrls->repeat_delay);
			g_key_file_set_integer(file, KFG_XKeyboard,
					       KFK_XKeyboard_RepeatInterval,
					       state.XKeyboard.desc->ctrls->repeat_interval);
		}

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

		static struct {
			unsigned short mask;
			char *name;
		} axoptions[12] = {
			{
			XkbAX_SKPressFBMask, "SlowKeysPress"}, {
			XkbAX_SKAcceptFBMask, "SlowKeysAccept"}, {
			XkbAX_FeatureFBMask, "Feature"}, {
			XkbAX_SlowWarnFBMask, "SlowWarn"}, {
			XkbAX_IndicatorFBMask, "Indicator"}, {
			XkbAX_StickyKeysFBMask, "StickyKeys"}, {
			XkbAX_TwoKeysMask, "TwoKeys"}, {
			XkbAX_LatchToLockMask, "LatchToLock"}, {
			XkbAX_SKReleaseFBMask, "SlowKeysRelease"}, {
			XkbAX_SKRejectFBMask, "SlowKeysReject"}, {
			XkbAX_BKRejectFBMask, "BounceKeysReject"}, {
			XkbAX_DumbBellFBMask, "DumbBell"}
		};

		for (*buf = '\0', j = 0, i = 0; i < 12; i++) {
			if (state.XKeyboard.desc->ctrls->ax_options & axoptions[i].mask) {
				if (j++)
					strncat(buf, ";", sizeof(buf) - 1);
				strncat(buf, axoptions[i].name, sizeof(buf) - 1);
			}
		}
		g_key_file_set_string(file, KFG_XKeyboard, KFK_XKeyboard_AccessXOptions, buf);

		/* XkbAX_SKPressFBMask */
		/* XkbAX_SKAcceptFBMask */
		/* XkbAX_FeatureFBMask */
		/* XkbAX_SlowWarnFBMask */
		/* XkbAX_IndicatorFBMask */
		/* XkbAX_StickKeysFBMask */
		/* XkbAX_TwoKeysMask */
		/* XkbAX_LatchToLockMask */
		/* XkbAX_SKReleaseFBMask */
		/* XkbAX_SKRejectFBMask */
		/* XkbAX_BKRejectFBMask */
		/* XkbAX_DumbBellFBMask */

		/* XkbAX_FBOptionsMask */
		/* XkbAX_SKOptionsMask */
		/* XkbAX_AllOptiosMask */

		{
#if 0
			int ax_timeout, axt_ctrls_mask, axt_ctrls_values, axt_opts_mask, axt_opts_values;

			XkbGetAccessXTimeout(dpy, XkbUseCoreKbd, &ax_timeout, &axt_ctrls_mask,
					     &axt_ctrls_values, &axt_opts_mask, &axt_opts_values);
#endif

			g_key_file_set_integer(file, KFG_XKeyboard,
					       KFK_XKeyboard_AccessXTimeout,
					       state.XKeyboard.desc->ctrls->ax_timeout);
			for (*buf = '\0', j = 0, i = 0; i < 12; i++) {
				if (state.XKeyboard.desc->ctrls->axt_opts_mask & axoptions[i].mask) {
					if (j++)
						strncat(buf, ";", sizeof(buf) - 1);
					strncat(buf, axoptions[i].name, sizeof(buf) - 1);
				}

			}
			g_key_file_set_string(file, KFG_XKeyboard, KFK_XKeyboard_AccessXTimeoutOptionsMask, buf);

			for (*buf = '\0', j = 0, i = 0; i < 12; i++) {
				if (state.XKeyboard.desc->ctrls->axt_opts_values & axoptions[i].mask) {
					if (j++)
						strncat(buf, ";", sizeof(buf) - 1);
					strncat(buf, axoptions[i].name, sizeof(buf) - 1);
				}

			}
			g_key_file_set_string(file, KFG_XKeyboard, KFK_XKeyboard_AccessXTimeoutOptionsValues, buf);

			for (*buf = '\0', j = 0, i = 0; i < 12; i++) {
				if (state.XKeyboard.desc->ctrls->axt_ctrls_mask & axoptions[i].mask) {
					if (j++)
						strncat(buf, ";", sizeof(buf) - 1);
					strncat(buf, axoptions[i].name, sizeof(buf) - 1);
				}

			}
			g_key_file_set_string(file, KFG_XKeyboard, KFK_XKeyboard_AccessXTimeoutMask, buf);

			for (*buf = '\0', j = 0, i = 0; i < 12; i++) {
				if (state.XKeyboard.desc->ctrls->axt_ctrls_values & axoptions[i].mask) {
					if (j++)
						strncat(buf, ";", sizeof(buf) - 1);
					strncat(buf, axoptions[i].name, sizeof(buf) - 1);
				}

			}
			g_key_file_set_string(file, KFG_XKeyboard, KFK_XKeyboard_AccessXTimeoutValues, buf);
		}
	}
	if (support.XF86Misc) {
		XF86MiscGetKbdSettings(dpy, &state.XF86Misc.keyboard);
		g_key_file_set_integer(file, KFG_XF86Misc,
				       KFK_XF86Misc_KeyboardRate, state.XF86Misc.keyboard.rate);
		g_key_file_set_integer(file, KFG_XF86Misc,
				       KFK_XF86Misc_KeyboardDelay, state.XF86Misc.keyboard.delay);

		XF86MiscGetMouseSettings(dpy, &state.XF86Misc.mouse);
		g_key_file_set_boolean(file, KFG_XF86Misc,
				       KFK_XF86Misc_MouseEmulate3Buttons,
				       state.XF86Misc.mouse.emulate3buttons ? TRUE : FALSE);
		g_key_file_set_integer(file, KFG_XF86Misc,
				       KFK_XF86Misc_MouseEmulate3Timeout, state.XF86Misc.mouse.emulate3timeout);
		g_key_file_set_boolean(file, KFG_XF86Misc,
				       KFK_XF86Misc_MouseChordMiddle,
				       state.XF86Misc.mouse.chordmiddle ? TRUE : FALSE);
	}
	PTRACE(5);
}

/** @brief write input settings
  *
  * Write the input settings back to the configuration file.  Simple and direct.
  * The file is an .ini-style keyfile.  Use glib to write out the values.
  */
void
save_config()
{
	GError *error = NULL;
	gchar *filename, *dir;

	PTRACE(5);
	if (!file) {
		EPRINTF("no key file!\n");
		return;
	}
	filename = options.filename ? g_strdup(options.filename) :
	    g_build_filename(g_get_user_config_dir(), "xde", "input.ini", NULL);
	dir = g_path_get_dirname(filename);
	if (g_mkdir_with_parents(dir, 0755) == -1) {
		EPRINTF("could not create directory %s: %s\n", dir, strerror(errno));
		g_free(dir);
		g_free(filename);
		return;
	}
	g_free(dir);
	g_key_file_save_to_file(file, filename, &error);
	if (error) {
		EPRINTF("COULD NOT SAVE KEY FILE %s: %s\n", filename, error->message);
		g_error_free(error);
	}
	g_free(filename);
}

static void
edit_put_values(void)
{
	PTRACE(5);
	read_input();
	if (!file) {
		EPRINTF("no key file!\n");
		return;
	}
	PTRACE(5);
	if (g_key_file_has_group(file, KFG_Keyboard) && support.Keyboard) {
		XKeyboardControl kbd = { 0, };
		unsigned long value_mask = 0;

		PTRACE(5);
		if ((kbd.key_click_percent =
		     g_key_file_get_integer(file, KFG_Keyboard, KFK_Keyboard_KeyClickPercent, NULL)))
			value_mask |= KBKeyClickPercent;
		if ((kbd.bell_percent =
		     g_key_file_get_integer(file, KFG_Keyboard, KFK_Keyboard_BellPercent, NULL)))
			value_mask |= KBBellPercent;
		if ((kbd.bell_pitch = g_key_file_get_integer(file, KFG_Keyboard, KFK_Keyboard_BellPitch, NULL)))
			value_mask |= KBBellPitch;
		if ((kbd.bell_duration =
		     g_key_file_get_integer(file, KFG_Keyboard, KFK_Keyboard_BellDuration, NULL)))
			value_mask |= KBBellDuration;
		if ((kbd.auto_repeat_mode =
		     g_key_file_get_boolean(file, KFG_Keyboard, KFK_Keyboard_GlobalAutoRepeat, NULL)))
			value_mask |= KBAutoRepeatMode;
		if (value_mask)
			XChangeKeyboardControl(dpy, value_mask, &kbd);
	}
	PTRACE(5);
	if (g_key_file_has_group(file, KFG_Pointer) && support.Pointer) {
		Bool do_accel, do_threshold;
		int accel_numerator, accel_denominator, threshold;

		PTRACE(5);
		accel_denominator =
		    g_key_file_get_integer(file, KFG_Pointer, KFK_Pointer_AccelerationDenominator, NULL);
		accel_numerator =
		    g_key_file_get_integer(file, KFG_Pointer, KFK_Pointer_AccelerationNumerator, NULL);
		threshold = g_key_file_get_integer(file, KFG_Pointer, KFK_Pointer_Threshold, NULL);
		do_accel = (accel_denominator && accel_numerator) ? True : False;
		do_threshold = threshold ? True : False;
		XChangePointerControl(dpy, do_accel, do_threshold, accel_numerator, accel_denominator, threshold);
	}
	PTRACE(5);
	if (g_key_file_has_group(file, KFG_XKeyboard) && support.XKeyboard) {
		PTRACE(5);
	}
	PTRACE(5);
	if (g_key_file_has_group(file, KFG_ScreenSaver) && support.ScreenSaver) {
		int timeout, interval, prefer_blanking, allow_exposures;
		char *str;

		PTRACE(5);
		timeout = g_key_file_get_integer(file, KFG_ScreenSaver, KFK_ScreenSaver_Timeout, NULL);
		interval = g_key_file_get_integer(file, KFG_ScreenSaver, KFK_ScreenSaver_Interval, NULL);
		str = g_key_file_get_string(file, KFG_ScreenSaver, KFK_ScreenSaver_PreferBlanking, NULL);
		if (str && strcmp(str, "DontPreferBlanking") == 0) {
			prefer_blanking = DontPreferBlanking;
		} else if (str && strcmp(str, "PreferBlanking") == 0) {
			prefer_blanking = PreferBlanking;
		} else {
			prefer_blanking = DefaultBlanking;
		}
		if (str)
			g_free(str);
		str = g_key_file_get_string(file, KFG_ScreenSaver, KFK_ScreenSaver_AllowExposures, NULL);
		if (str && strcmp(str, "DontAllowExposures")) {
			allow_exposures = DontAllowExposures;
		} else if (str && strcmp(str, "AllowExposures")) {
			allow_exposures = AllowExposures;
		} else {
			allow_exposures = DefaultExposures;
		}
		if (str)
			g_free(str);
		XSetScreenSaver(dpy, timeout, interval, prefer_blanking, allow_exposures);
	}
	PTRACE(5);
	if (g_key_file_has_group(file, KFG_DPMS) && support.DPMS) {
		int standby, suspend, off, level;

		PTRACE(5);
		standby = g_key_file_get_integer(file, KFG_DPMS, KFK_DPMS_StandbyTimeout, NULL);
		suspend = g_key_file_get_integer(file, KFG_DPMS, KFK_DPMS_SuspendTimeout, NULL);
		off = g_key_file_get_integer(file, KFG_DPMS, KFK_DPMS_OffTimeout, NULL);
		DPMSSetTimeouts(dpy, standby, suspend, off);
		if (g_key_file_get_boolean(file, KFG_DPMS, KFK_DPMS_State, NULL)) {
			DPMSEnable(dpy);
		} else {
			DPMSDisable(dpy);
		}
		level = g_key_file_get_integer(file, KFG_DPMS, KFK_DPMS_PowerLevel, NULL);
		DPMSForceLevel(dpy, level);
	}
	PTRACE(5);
}

void
reprocess_input()
{
	PTRACE(5);
	edit_get_values();
	edit_set_values();
}

static void
init_extensions()
{
	Window window = RootWindow(dpy, options.screen);
	Bool missing = False;

	PTRACE(5);
	support.Keyboard = True;
	DPRINTF(1, "Keyboard: core protocol support\n");
	support.Pointer = True;
	DPRINTF(1, "Pointer: core protocol support\n");
	if (XkbQueryExtension(dpy, &state.XKeyboard.opcode, &state.XKeyboard.event,
			      &state.XKeyboard.error, &state.XKeyboard.major_version,
			      &state.XKeyboard.minor_version)) {
		support.XKeyboard = True;
		DPRINTF(1, "XKeyboard: opcode=%d, event=%d, error=%d, major=%d, minor=%d\n",
			state.XKeyboard.opcode, state.XKeyboard.event, state.XKeyboard.error,
			state.XKeyboard.major_version, state.XKeyboard.minor_version);
		XkbSelectEvents(dpy, XkbUseCoreKbd, XkbControlsNotifyMask, XkbControlsNotifyMask);
	} else {
		missing = True;
		support.XKeyboard = False;
		DPRINTF(1, "XKeyboard: not supported\n");
	}
	if (XScreenSaverQueryExtension(dpy, &state.ScreenSaver.event, &state.ScreenSaver.error) &&
	    XScreenSaverQueryVersion(dpy, &state.ScreenSaver.major_version, &state.ScreenSaver.minor_version)) {
		support.ScreenSaver = True;
		DPRINTF(1, "ScreenSaver: event=%d, error=%d, major=%d, minor=%d\n",
			state.ScreenSaver.event, state.ScreenSaver.error,
			state.ScreenSaver.major_version, state.ScreenSaver.minor_version);
		XScreenSaverQueryInfo(dpy, window, &state.ScreenSaver.info);
		if (options.debug) {
			fputs("Screen Saver:\n", stderr);
			fputs("\tstate: ", stderr);
			switch (state.ScreenSaver.info.state) {
			case ScreenSaverOff:
				fputs("Off\n", stderr);
				break;
			case ScreenSaverOn:
				fputs("On\n", stderr);
				break;
			case ScreenSaverDisabled:
				fputs("Disabled\n", stderr);
				break;
			default:
				fprintf(stderr, "%d (unknown)\n", state.ScreenSaver.info.state);
				break;
			}
			fputs("\tkind: ", stderr);
			switch (state.ScreenSaver.info.kind) {
			case ScreenSaverBlanked:
				fputs("Blanked\n", stderr);
				break;
			case ScreenSaverInternal:
				fputs("Internal\n", stderr);
				break;
			case ScreenSaverExternal:
				fputs("External\n", stderr);
				break;
			default:
				fprintf(stderr, "%d (unknown)\n", state.ScreenSaver.info.kind);
				break;
			}
			fprintf(stderr, "\ttil-or-since: %lu milliseconds\n", state.ScreenSaver.info.til_or_since);
			fprintf(stderr, "\tidle: %lu milliseconds\n", state.ScreenSaver.info.idle);
			fputs("\tevent-mask: ", stderr);
			if (state.ScreenSaver.info.eventMask & ScreenSaverNotifyMask)
				fputs("NotifyMask ", stderr);
			if (state.ScreenSaver.info.eventMask & ScreenSaverCycleMask)
				fputs("CycleMask", stderr);
			fputs("\n", stderr);
		}
	} else {
		missing = True;
		support.ScreenSaver = False;
		DPRINTF(1, "ScreenSaver: not supported\n");
	}
	if (DPMSQueryExtension(dpy, &state.DPMS.event, &state.DPMS.error) &&
	    DPMSGetVersion(dpy, &state.DPMS.major_version, &state.DPMS.minor_version)) {
		support.DPMS = True;
		DPRINTF(1, "DPMS: event=%d, error=%d, major=%d, minor=%d\n",
			state.DPMS.event, state.DPMS.error, state.DPMS.major_version, state.DPMS.minor_version);
	} else {
		missing = True;
		support.DPMS = False;
		DPRINTF(1, "DPMS: not supported\n");
	}
	if (XF86MiscQueryExtension(dpy, &state.XF86Misc.event, &state.XF86Misc.error) &&
	    XF86MiscQueryVersion(dpy, &state.XF86Misc.major_version, &state.XF86Misc.minor_version)) {
		support.XF86Misc = True;
		DPRINTF(1, "XF86Misc: event=%d, error=%d, major=%d, minor=%d\n",
			state.XF86Misc.event, state.XF86Misc.error, state.XF86Misc.major_version,
			state.XF86Misc.minor_version);
	} else {
		missing = True;
		support.XF86Misc = False;
		DPRINTF(1, "XF86Misc: not supported\n");
	}
	if (XRRQueryExtension(dpy, &state.RANDR.event, &state.RANDR.error) &&
	    XRRQueryVersion(dpy, &state.RANDR.major_version, &state.RANDR.minor_version)) {
		support.RANDR = True;
		DPRINTF(1, "RANDR: event=%d, error=%d, major=%d, minor=%d\n",
			state.RANDR.event, state.RANDR.error, state.RANDR.major_version,
			state.RANDR.minor_version);
	} else {
		missing = True;
		support.RANDR = False;
	}
	if (missing && options.debug > 0) {
		char **list;
		int i, n = 0;

		if ((list = XListExtensions(dpy, &n))) {
			fputs("Extensions are:", stderr);
			for (i = 0; i < n; i++)
				fprintf(stderr, " %s", list[i]);
			fputs("\n", stderr);
			XFreeExtensionList(list);
		}
	}
}

void present_popup(XdeScreen *xscr);

void
applet_refresh(XdeScreen *xscr)
{
}

/** @brief restart the applet
  *
  * We restart the applet by executing ourselves with the same arguments that were
  * provided in the command that started us.  However, if we are running under
  * session management with restart hint SmRestartImmediately, the session
  * manager will restart us if we simply exit.
  */
void
applet_restart(void)
{
	/* asked to restart the applet (as though we were re-executed) */
	char **argv;
	int i;

#if 0
	if (smcConn) {
		/* When running under a session manager, simply exit and the session
		   manager will restart us immediately. */
		exit(EXIT_SUCCESS);
	}
#endif

	argv = calloc(saveArgc + 1, sizeof(*argv));
	for (i = 0; i < saveArgc; i++)
		argv[i] = saveArgv[i];

	DPRINTF(1, "%s: restarting the applet\n", NAME);
	if (execvp(argv[0], argv) == -1)
		EPRINTF("%s: %s\n", argv[0], strerror(errno));
	return;
}

static gboolean
on_button_press(GtkStatusIcon *icon, GdkEvent *event, gpointer user_data)
{
	XdeScreen *xscr = user_data;
	GdkEventButton *ev;

	(void) xscr;
	ev = (typeof(ev)) event;
	if (ev->button != 1)
		return GTK_EVENT_PROPAGATE;
	/* FIXME: do something */
	present_popup(xscr);
	return GTK_EVENT_STOP;
}

static void edit_input(XdeScreen *xscr);

void
on_edit_selected(GtkMenuItem *item, gpointer user_data)
{
	edit_input(user_data);
}

static void edit_sav_values();

void
on_save_selected(GtkMenuItem *item, gpointer user_data)
{
	edit_sav_values();
}

void
on_refresh_selected(GtkMenuItem *item, gpointer user_data)
{
}

void
on_redo_selected(GtkMenuItem *item, gpointer user_data)
{
}

void
on_about_selected(GtkMenuItem *item, gpointer user_data)
{
	gchar *authors[] = { "Brian F. G. Bidulock <bidulock@openss7.org>", NULL };
	gtk_show_about_dialog(NULL,
			      "authors", authors,
			      "comments", "An keyboard system tray icon.",
			      "copyright", "Copyright (c) 2013, 2014, 2015, 2016  OpenSS7 Corporation",
			      "license", "Do what thou wilt shall be the whole of the law.\n\n-- Aleister Crowley",
			      "logo-icon-name", LOGO_NAME,
			      "program-name", NAME,
			      "version", VERSION,
			      "website", "http://www.unexicon.com/",
			      "website-label", "Unexicon - Linux spun for telecom", NULL);
	return;
}

void
on_quit_selected(GtkMenuItem *item, gpointer user_data)
{
	gtk_main_quit();
}

static void
on_popup_menu(GtkStatusIcon *icon, guint button, guint time, gpointer user_data)
{
	XdeScreen *xscr = user_data;
	GtkWidget *menu, *item;

	menu = gtk_menu_new();

	item = gtk_image_menu_item_new_from_stock("gtk-edit", NULL);
	g_signal_connect(item, "activate", G_CALLBACK(on_edit_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-save", NULL);
	g_signal_connect(item, "activate", G_CALLBACK(on_save_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-about", NULL);
	g_signal_connect(item, "activate", G_CALLBACK(on_about_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_separator_menu_item_new();
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-refresh", NULL);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(on_refresh_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-redo", NULL);
	g_signal_connect(item, "activate", G_CALLBACK(on_redo_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	item = gtk_image_menu_item_new_from_stock("gtk-quit", NULL);
	g_signal_connect(item, "activate", G_CALLBACK(on_quit_selected), xscr);
	gtk_widget_show(item);
	gtk_menu_append(menu, item);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, icon, button, time);
	return;
}

static guint popuptimer = 0;

static void drop_popup(XdeScreen *xscr);

static gboolean
popup_timeout(gpointer user)
{
	XdeScreen *xscr = user;

	DPRINTF(1, "popup timeout\n");
	drop_popup(xscr);
	popuptimer = 0;
	return G_SOURCE_REMOVE;
}

static gboolean
stop_popup_timer(XdeScreen *xscr)
{
	if (popuptimer) {
		DPRINTF(1, "stopped popup timer\n");
		g_source_remove(popuptimer);
		popuptimer = 0;
		return TRUE;
	}
	return FALSE;
}

static gboolean
start_popup_timer(XdeScreen *xscr)
{
	if (popuptimer)
		return FALSE;
	DPRINTF(1, "started popup timer\n");
	popuptimer = g_timeout_add(options.timeout, popup_timeout, xscr);
	return TRUE;
}

void
restart_popup_timer(XdeScreen *xscr)
{
	stop_popup_timer(xscr);
	start_popup_timer(xscr);
}

static GtkWidget *ttwindow = NULL;
Bool gotit = False;

static gchar *format_value_percent(GtkScale * scale, gdouble value, gpointer user_data);
static void bell_percent_value_changed(GtkRange * range, gpointer user_data);

static gboolean inside = False;

void
present_popup(XdeScreen *xscr)
{
	if (!gtk_widget_get_mapped(ttwindow)) {
#if 0
		GdkEventMask mask =
		    GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
#endif
		gtk_window_set_position(GTK_WINDOW(ttwindow), GTK_WIN_POS_MOUSE);
		gtk_window_present(GTK_WINDOW(ttwindow));
		gtk_widget_show_now(GTK_WIDGET(ttwindow));
		gdk_window_focus(ttwindow->window, GDK_CURRENT_TIME);

#if 0
		gdk_keyboard_grab(ttwindow->window, TRUE, GDK_CURRENT_TIME);
		gdk_pointer_grab(ttwindow->window, FALSE, mask, NULL, NULL, GDK_CURRENT_TIME);
#endif
		stop_popup_timer(xscr);
		inside = TRUE;
	}
}

gboolean
on_query_tooltip(GtkStatusIcon *icon, gint x, gint y, gboolean keyboard_mode,
		 GtkTooltip *tooltip, gpointer user_data)
{
	XdeScreen *xscr = user_data;

	(void) xscr;
#if 0
	if (ttwindow) {
		present_popup(xscr);
		start_popup_timer(xscr);
		return FALSE;
	}
#endif
	return TRUE;		/* show it now */
}

static void
drop_popup(XdeScreen *xscr)
{
	if (gtk_widget_get_mapped(ttwindow)) {
		stop_popup_timer(xscr);
		gtk_widget_hide(ttwindow);
	}
}

static gboolean
popup_grab_broken_event(GtkWidget *widget, GdkEvent *event, gpointer user)
{
	XdeScreen *xscr = (typeof(xscr)) user;
	GdkEventGrabBroken *ev = (typeof(ev)) event;

	if (ev->keyboard) {
		start_popup_timer(xscr);
	} else {
		drop_popup(xscr);
	}
	return GTK_EVENT_STOP;	/* event handled */
}

static GdkFilterReturn
popup_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;
	XdeScreen *xscr = data;

	switch (xev->type) {
	case KeyPress:
		DPRINTF(2, "KeyPress\n");
		if (!inside) {
			drop_popup(xscr);
			return GDK_FILTER_REMOVE;
		}
		return GDK_FILTER_CONTINUE;
	case KeyRelease:
		DPRINTF(2, "KeyRelease\n");
		return GDK_FILTER_CONTINUE;
	case ButtonPress:
		DPRINTF(2, "ButtonPress\n");
		if (!inside) {
			drop_popup(xscr);
			return GDK_FILTER_REMOVE;
		}
		return GDK_FILTER_CONTINUE;
	case ButtonRelease:
		DPRINTF(2, "ButtonRelease\n");
		return GDK_FILTER_CONTINUE;
	case EnterNotify:
		DPRINTF(2, "EnterNotify\n");
		if (xev->xcrossing.mode == NotifyNormal) {
			DPRINTF(2, "EnterNotify: NotifyNormal\n");
			if (!inside) {
				DPRINTF(2, "now I'm inside\n");
				stop_popup_timer(xscr);
				inside = TRUE;
			}
		}
		return GDK_FILTER_CONTINUE;
	case LeaveNotify:
		DPRINTF(2, "LeaveNotify\n");
		if (xev->xcrossing.mode == NotifyNormal) {
			DPRINTF(2, "LeaveNotify: NotifyNormal\n");
			if (inside) {
				DPRINTF(2, "now I'm outside\n");
				start_popup_timer(xscr);
				inside = FALSE;
			}
		}
		return GDK_FILTER_CONTINUE;
	case FocusIn:
		DPRINTF(0, "FocusIn\n");
		switch (xev->xfocus.mode) {
		case NotifyNormal:
		case NotifyGrab:
		default:
			stop_popup_timer(xscr);
			DPRINTF(0, "focused popup\n");
			break;
		}
		return GDK_FILTER_CONTINUE;
	case FocusOut:
		DPRINTF(0, "FocusOut\n");
		switch (xev->xfocus.mode) {
		case NotifyNormal:
		case NotifyUngrab:
		case NotifyWhileGrabbed:
		default:
			DPRINTF(0, "unfocused popup\n");
			drop_popup(xscr);
			break;
		}
		return GDK_FILTER_CONTINUE;
	default:
		DPRINTF(2, "unhandled event\n");
		return GDK_FILTER_CONTINUE;
	}
}

static void
popup_widget_realize(GtkWidget *popup, gpointer user)
{
	gdk_window_add_filter(popup->window, popup_handler, user);
	gdk_window_set_override_redirect(popup->window, TRUE);
	// gdk_window_set_accept_focus(popup->window, TRUE);
	// gdk_window_set_focus_on_map(popup->window, TRUE);
}

void
create_tooltip(XdeScreen *xscr)
{
	GtkWidget *w, *h, *f, *s;

	if (ttwindow)
		return;

	w = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_add_events(w, GDK_ALL_EVENTS_MASK);
	gtk_window_set_accept_focus(GTK_WINDOW(w), TRUE);
	gtk_window_set_focus_on_map(GTK_WINDOW(w), TRUE);
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
	g_signal_connect(G_OBJECT(s), "value-changed", G_CALLBACK(bell_percent_value_changed), NULL);
	controls.Icon.BellPercent = s;

	gtk_container_add(GTK_CONTAINER(w), h);
	gtk_widget_show_all(h);

	gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_MOUSE);
	gtk_container_set_border_width(GTK_CONTAINER(w), 3);
	gtk_window_set_default_size(GTK_WINDOW(w), -1, 200);
	gtk_widget_set_size_request(w, -1, 200);

	g_signal_connect(G_OBJECT(w), "grab_broken_event", G_CALLBACK(popup_grab_broken_event), xscr);
	g_signal_connect(G_OBJECT(w), "realize", G_CALLBACK(popup_widget_realize), xscr);

	ttwindow = w;
}

static GtkStatusIcon *
create_icon(XdeScreen *xscr)
{
	GtkStatusIcon *icon;

	icon = gtk_status_icon_new_from_icon_name(LOGO_NAME);
	gtk_status_icon_set_tooltip_text(icon, "Click for menu...");
	g_signal_connect(icon, "button_press_event", G_CALLBACK(on_button_press), xscr);
	g_signal_connect(icon, "popup_menu", G_CALLBACK(on_popup_menu), xscr);
	// g_signal_connect(icon, "query_tooltip", G_CALLBACK(on_query_tooltip), xscr);
	create_tooltip(xscr);
	return (icon);
}

GtkWindow *create_window();

static gchar *
format_value_milliseconds(GtkScale * scale, gdouble value, gpointer user_data)
{
	return g_strdup_printf("%.6g ms", /* gtk_scale_get_digits(scale), */ value);
}

static gchar *
format_value_seconds(GtkScale * scale, gdouble value, gpointer user_data)
{
	return g_strdup_printf("%.6g s", /* gtk_scale_get_digits(scale), */ value);
}

static gchar *
format_value_percent(GtkScale * scale, gdouble value, gpointer user_data)
{
	return g_strdup_printf("%.6g%%", /* gtk_scale_get_digits(scale), */ value);
}

static char *
format_value_hertz(GtkScale * scale, gdouble value, gpointer user_data)
{
	return g_strdup_printf("%.6g Hz", /* gtk_scale_get_digits(scale), */ value);
}

static void
accel_numerator_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.Pointer.accel_numerator) {
		PTRACE(5);
		XChangePointerControl(dpy, True, False, val, state.Pointer.accel_denominator,
				      state.Pointer.threshold);
		reprocess_input();
	}
}

static void
accel_denominator_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.Pointer.accel_denominator) {
		PTRACE(5);
		XChangePointerControl(dpy, True, False, state.Pointer.accel_numerator, val,
				      state.Pointer.threshold);
		reprocess_input();
	}
}

static void
threshold_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value;
	int val;

	value = gtk_range_get_value(range);
	val = round(value);
	if (val != state.Pointer.threshold) {
		PTRACE(5);
		XChangePointerControl(dpy, False, True,
				      state.Pointer.accel_denominator, state.Pointer.accel_numerator, val);
		reprocess_input();
	}
}

static void
global_autorepeat_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	int val = active ? AutoRepeatModeOn : AutoRepeatModeOff;

	if (val != state.Keyboard.global_auto_repeat) {
		XKeyboardControl kb = {
			.auto_repeat_mode = val,
		};
		PTRACE(5);
		XChangeKeyboardControl(dpy, KBAutoRepeatMode, &kb);
		reprocess_input();
	}

}

static void
keyclick_percent_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.Keyboard.key_click_percent) {
		XKeyboardControl kb = {
			.key_click_percent = val,
		};
		PTRACE(5);
		XChangeKeyboardControl(dpy, KBKeyClickPercent, &kb);
		reprocess_input();
	}
}

static void
bell_percent_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.Keyboard.bell_percent) {
		XKeyboardControl kb = {
			.bell_percent = val,
		};
		PTRACE(5);
		XChangeKeyboardControl(dpy, KBBellPercent, &kb);
		reprocess_input();
	}
}

static void
bell_pitch_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.Keyboard.bell_pitch) {
		XKeyboardControl kb = {
			.bell_pitch = val,
		};
		PTRACE(5);
		XChangeKeyboardControl(dpy, KBBellPitch, &kb);
		reprocess_input();
	}
}

static void
ring_bell_clicked(GtkButton *button, gpointer user_data)
{
	XBell(dpy, 0);
	XFlush(dpy);
}

static void
bell_duration_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.Keyboard.bell_duration) {
		XKeyboardControl kb = {
			.bell_duration = val,
		};
		PTRACE(5);
		XChangeKeyboardControl(dpy, KBBellDuration, &kb);
		reprocess_input();
	}
}

static void
repeat_keys_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	unsigned int value = active ? XkbRepeatKeysMask : 0;

	if (value != (state.XKeyboard.desc->ctrls->enabled_ctrls & XkbRepeatKeysMask)) {
#if 1
		PTRACE(5);
		XkbChangeEnabledControls(dpy, XkbUseCoreKbd, XkbRepeatKeysMask, value);
#else
		state.XKeyboard.desc->ctrls->enabled_ctrls ^= XkbRepeatKeysMask;
		XkbSetControls(dpy, XkbRepeatKeysMask, state.XKeyboard.desc);
#endif
		reprocess_input();
	}
}

static void
repeat_delay_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	unsigned short val = round(value);

	if (val != state.XKeyboard.desc->ctrls->repeat_delay) {
		state.XKeyboard.desc->ctrls->repeat_delay = val;
#if 0
		XkbSetAutoRepeatRate(dpy, XkbUseCoreKbd, state.XKeyboard.desc->ctrls->repeat_delay,
				     state.XKeyboard.desc->ctrls->repeat_interval);
#else
		PTRACE(5);
		XkbSetControls(dpy, XkbRepeatKeysMask, state.XKeyboard.desc);
#endif
		reprocess_input();
	}
}

static void
repeat_interval_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	unsigned short val = round(value);

	if (val != state.XKeyboard.desc->ctrls->repeat_interval) {
		state.XKeyboard.desc->ctrls->repeat_interval = val;
#if 0
		XkbSetAutoRepeatRate(dpy, XkbUseCoreKbd, state.XKeyboard.desc->ctrls->repeat_delay,
				     state.XKeyboard.desc->ctrls->repeat_interval);
#else
		PTRACE(5);
		XkbSetControls(dpy, XkbRepeatKeysMask, state.XKeyboard.desc);
#endif
		reprocess_input();
	}
}

static void
slow_keys_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	unsigned int value = active ? XkbSlowKeysMask : 0;

	if (value != (state.XKeyboard.desc->ctrls->enabled_ctrls & XkbSlowKeysMask)) {
#if 1
		PTRACE(5);
		XkbChangeEnabledControls(dpy, XkbUseCoreKbd, XkbSlowKeysMask, value);
#else
		state.XKeyboard.desc->ctrls->enabled_ctrls ^= XkbSlowKeysMask;
		XkbSetControls(dpy, XkbSlowKeysMask, state.XKeyboard.desc);
#endif
		reprocess_input();
	}
}

static void
slow_keys_delay_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	unsigned short val = round(value);

	if (val != state.XKeyboard.desc->ctrls->slow_keys_delay) {
		PTRACE(5);
		state.XKeyboard.desc->ctrls->slow_keys_delay = val;
		XkbSetControls(dpy, XkbSlowKeysMask, state.XKeyboard.desc);
		reprocess_input();
	}
}

static void
bounce_keys_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	unsigned int value = active ? XkbBounceKeysMask : 0;

	if (value != (state.XKeyboard.desc->ctrls->enabled_ctrls & XkbBounceKeysMask)) {
#if 1
		PTRACE(5);
		XkbChangeEnabledControls(dpy, XkbUseCoreKbd, XkbBounceKeysMask, value);
#else
		state.XKeyboard.desc->ctrls->enabled_ctrls ^= XkbBounceKeysMask;
		XkbSetControls(dpy, XkbBounceKeysMask, state.XKeyboard.desc);
#endif
		reprocess_input();
	}
}

static void
debounce_delay_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	unsigned short val = round(value);

	if (val != state.XKeyboard.desc->ctrls->debounce_delay) {
		PTRACE(5);
		state.XKeyboard.desc->ctrls->debounce_delay = val;
		XkbSetControls(dpy, XkbBounceKeysMask, state.XKeyboard.desc);
		reprocess_input();
	}
}

static void
sticky_keys_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	unsigned int value = active ? XkbStickyKeysMask : 0;

	if (value != (state.XKeyboard.desc->ctrls->enabled_ctrls & XkbStickyKeysMask)) {
#if 1
		PTRACE(5);
		XkbChangeEnabledControls(dpy, XkbUseCoreKbd, XkbStickyKeysMask, value);
#else
		state.XKeyboard.desc->ctrls->enabled_ctrls ^= XkbStickyKeysMask;
		XkbSetControls(dpy, XkbStickyKeysMask, state.XKeyboard.desc);
#endif
		reprocess_input();
	}
}

static void
mouse_keys_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	unsigned int value = active ? XkbMouseKeysMask : 0;

	if (value != (state.XKeyboard.desc->ctrls->enabled_ctrls & XkbMouseKeysMask)) {
#if 1
		PTRACE(5);
		XkbChangeEnabledControls(dpy, XkbUseCoreKbd, XkbMouseKeysMask, value);
#else
		state.XKeyboard.desc->ctrls->enabled_ctrls ^= XkbMouseKeysMask;
		XkbSetControls(dpy, XkbMouseKeysMask, state.XKeyboard.desc);
#endif
		reprocess_input();
	}
}

static void
default_mouse_button_changed(GtkComboBox * box, gpointer user_data)
{
	gint value = gtk_combo_box_get_active(box);
	unsigned char val = value + 1;

	if (val != state.XKeyboard.desc->ctrls->mk_dflt_btn) {
		PTRACE(5);
		state.XKeyboard.desc->ctrls->mk_dflt_btn = val;
		XkbSetControls(dpy, XkbMouseKeysMask, state.XKeyboard.desc);
		reprocess_input();
	}
}

static void
mouse_keys_accel_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);
	unsigned int value = active ? XkbMouseKeysAccelMask : 0;

	if (value != (state.XKeyboard.desc->ctrls->enabled_ctrls & XkbMouseKeysAccelMask)) {
#if 1
		PTRACE(5);
		XkbChangeEnabledControls(dpy, XkbUseCoreKbd, XkbMouseKeysAccelMask, value);
#else
		state.XKeyboard.desc->ctrls->enabled_ctrls ^= XkbMouseKeysAccelMask;
		XkbSetControls(dpy, XkbMouseKeysAccelMask, state.XKeyboard.desc);
#endif
		reprocess_input();
	}
}

static void
mouse_keys_delay_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	unsigned short val = round(value);

	if (val != state.XKeyboard.desc->ctrls->mk_delay) {
		PTRACE(5);
		state.XKeyboard.desc->ctrls->mk_delay = val;
		XkbSetControls(dpy, XkbMouseKeysAccelMask, state.XKeyboard.desc);
		reprocess_input();
	}
}

static void
mouse_keys_interval_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	unsigned short val = round(value);

	if (val != state.XKeyboard.desc->ctrls->mk_interval) {
		PTRACE(5);
		state.XKeyboard.desc->ctrls->mk_interval = val;
		XkbSetControls(dpy, XkbMouseKeysAccelMask, state.XKeyboard.desc);
		reprocess_input();
	}
}

static void
mouse_keys_time_to_max_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	unsigned short val = round(value);

	if (val != state.XKeyboard.desc->ctrls->mk_time_to_max) {
		PTRACE(5);
		state.XKeyboard.desc->ctrls->mk_time_to_max = val;
		XkbSetControls(dpy, XkbMouseKeysAccelMask, state.XKeyboard.desc);
		reprocess_input();
	}
}

static void
mouse_keys_max_speed_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	unsigned short val = round(value);

	if (val != state.XKeyboard.desc->ctrls->mk_max_speed) {
		PTRACE(5);
		state.XKeyboard.desc->ctrls->mk_max_speed = val;
		XkbSetControls(dpy, XkbMouseKeysAccelMask, state.XKeyboard.desc);
		reprocess_input();
	}
}

static void
mouse_keys_curve_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	short val = round(value);

	if (val != state.XKeyboard.desc->ctrls->mk_curve) {
		PTRACE(5);
		state.XKeyboard.desc->ctrls->mk_curve = val;
		XkbSetControls(dpy, XkbMouseKeysAccelMask, state.XKeyboard.desc);
		reprocess_input();
	}
}

static void
screensaver_timeout_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.ScreenSaver.timeout) {
		PTRACE(5);
		state.ScreenSaver.timeout = val;
		XSetScreenSaver(dpy,
				state.ScreenSaver.timeout,
				state.ScreenSaver.interval,
				state.ScreenSaver.prefer_blanking, state.ScreenSaver.allow_exposures);
		reprocess_input();
	}
}

static void
activate_screensaver_clicked(GtkButton *button, gpointer user_data)
{
	PTRACE(5);
	XForceScreenSaver(dpy, ScreenSaverActive);
	XFlush(dpy);
}

static void
screensaver_interval_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.ScreenSaver.interval) {
		PTRACE(5);
		state.ScreenSaver.interval = val;
		XSetScreenSaver(dpy,
				state.ScreenSaver.timeout,
				state.ScreenSaver.interval,
				state.ScreenSaver.prefer_blanking, state.ScreenSaver.allow_exposures);
		reprocess_input();
	}
}

static void
rotate_screensaver_clicked(GtkButton *button, gpointer user_data)
{
	PTRACE(5);
	XForceScreenSaver(dpy, ScreenSaverActive);
	XFlush(dpy);
}

static void
prefer_blanking_toggled(GtkToggleButton * button, gpointer user_data)
{
	int value = DefaultBlanking;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Preferblanking[0])))
		value = DefaultBlanking;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Preferblanking[1])))
		value = DontPreferBlanking;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Preferblanking[2])))
		value = PreferBlanking;
	if (value != state.ScreenSaver.prefer_blanking) {
		PTRACE(5);
		state.ScreenSaver.prefer_blanking = value;
		XSetScreenSaver(dpy,
				state.ScreenSaver.timeout,
				state.ScreenSaver.interval,
				state.ScreenSaver.prefer_blanking, state.ScreenSaver.allow_exposures);
		reprocess_input();
	}
}

static void
allow_exposures_toggled(GtkToggleButton * button, gpointer user_data)
{
	int value = DefaultExposures;

	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Allowexposures[0])))
		value = DefaultExposures;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Allowexposures[1])))
		value = DontAllowExposures;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(controls.ScreenSaver.Allowexposures[2])))
		value = AllowExposures;
	if (value != state.ScreenSaver.allow_exposures) {
		PTRACE(5);
		state.ScreenSaver.allow_exposures = value;
		XSetScreenSaver(dpy,
				state.ScreenSaver.timeout,
				state.ScreenSaver.interval,
				state.ScreenSaver.prefer_blanking, state.ScreenSaver.allow_exposures);
		reprocess_input();
	}
}

static void
dpms_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);

	PTRACE(5);
	if (active)
		DPMSEnable(dpy);
	else
		DPMSDisable(dpy);
	XFlush(dpy);
	reprocess_input();
}

static void
standby_timeout_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	CARD16 val = round(value);

	if (val != state.DPMS.standby) {
		PTRACE(5);
		state.DPMS.standby = val;
		DPMSSetTimeouts(dpy, state.DPMS.standby, state.DPMS.suspend, state.DPMS.off);
		reprocess_input();
	}
}

static void
activate_standby_clicked(GtkButton *button, gpointer user_data)
{
	PTRACE(5);
	DPMSForceLevel(dpy, DPMSModeStandby);
}

static void
suspend_timeout_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	CARD16 val = round(value);

	if (val != state.DPMS.suspend) {
		PTRACE(5);
		state.DPMS.suspend = val;
		DPMSSetTimeouts(dpy, state.DPMS.standby, state.DPMS.suspend, state.DPMS.off);
		reprocess_input();
	}
}

static void
activate_suspend_clicked(GtkButton *button, gpointer user_data)
{
	DPMSForceLevel(dpy, DPMSModeSuspend);
}

static void
off_timeout_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	CARD16 val = round(value);

	if (val != state.DPMS.off) {
		PTRACE(5);
		state.DPMS.off = val;
		DPMSSetTimeouts(dpy, state.DPMS.standby, state.DPMS.suspend, state.DPMS.off);
		reprocess_input();
	}
}

static void
activate_off_clicked(GtkButton *button, gpointer user_data)
{
	PTRACE(5);
	DPMSForceLevel(dpy, DPMSModeOff);
}

static void
keyboard_rate_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.XF86Misc.keyboard.rate) {
		PTRACE(5);
		state.XF86Misc.keyboard.rate = val;
		XF86MiscSetKbdSettings(dpy, &state.XF86Misc.keyboard);
	}
	reprocess_input();
}

static void
keyboard_delay_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.XF86Misc.keyboard.delay) {
		PTRACE(5);
		state.XF86Misc.keyboard.delay = val;
		XF86MiscSetKbdSettings(dpy, &state.XF86Misc.keyboard);
	}
	reprocess_input();
}

static void
emulate_3_buttons_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);

	PTRACE(5);
	if (active) {
		state.XF86Misc.mouse.emulate3buttons = True;
	} else {
		state.XF86Misc.mouse.emulate3buttons = False;
	}
	XF86MiscSetMouseSettings(dpy, &state.XF86Misc.mouse);
	reprocess_input();
}

static void
emulate_3_timeout_value_changed(GtkRange * range, gpointer user_data)
{
	gdouble value = gtk_range_get_value(range);
	int val = round(value);

	if (val != state.XF86Misc.mouse.emulate3timeout) {
		PTRACE(5);
		state.XF86Misc.mouse.emulate3timeout = val;
		XF86MiscSetMouseSettings(dpy, &state.XF86Misc.mouse);
	}
	reprocess_input();
}

static void
chord_middle_toggled(GtkToggleButton * button, gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(button);

	PTRACE(5);
	if (active) {
		state.XF86Misc.mouse.chordmiddle = True;
	} else {
		state.XF86Misc.mouse.chordmiddle = False;
	}
	XF86MiscSetMouseSettings(dpy, &state.XF86Misc.mouse);
	reprocess_input();
}

GtkWindow *
create_window()
{
	GtkWindow *w;
	GtkWidget *h, *n, *v, *l, *f, *u, *s, *q, *r;
	GSList *group;

	w = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	gtk_window_set_wmclass(w, "xde-input", "Xde-input");
	gtk_window_set_title(w, "XDE X Input Control");
	gtk_window_set_gravity(w, GDK_GRAVITY_CENTER);
	gtk_window_set_type_hint(w, GDK_WINDOW_TYPE_HINT_DIALOG);
	gtk_container_set_border_width(GTK_CONTAINER(w), 10);
	gtk_window_set_skip_pager_hint(w, FALSE);
	gtk_window_set_skip_taskbar_hint(w, FALSE);
	gtk_window_set_position(w, GTK_WIN_POS_CENTER_ALWAYS);
	g_signal_connect(G_OBJECT(w), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	h = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(w), h);

	n = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(h), n, FALSE, FALSE, 0);

	if (support.Pointer) {
		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("Pointer");
		gtk_notebook_append_page(GTK_NOTEBOOK(n), v, l);

		f = gtk_frame_new("Acceleration Numerator");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(1.0, 100.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Set the acceleration numerator.  The effective\n\
acceleration factor is the numerator divided by\n\
the denominator.  A typical value is 24.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(accel_numerator_value_changed), NULL);
		controls.Pointer.AccelerationNumerator = h;

		f = gtk_frame_new("Acceleration Denominator");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(1.0, 100.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		gtk_widget_set_tooltip_markup(h, "\
Set the acceleration denominator.  The effective\n\
acceleration factor is the numerator divided by\n\
the denominator.  A typical value is 10.");
		gtk_container_add(GTK_CONTAINER(f), h);
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(accel_denominator_value_changed), NULL);
		controls.Pointer.AccelerationDenominator = h;

		f = gtk_frame_new("Threshold");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(1.0, 100.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		gtk_widget_set_tooltip_markup(h, "\
Set the number of pixels moved before acceleration\n\
begins.  A typical and usable value is 10 pixels.");
		gtk_container_add(GTK_CONTAINER(f), h);
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(threshold_value_changed), NULL);
		controls.Pointer.Threshold = h;
	}

	if (support.Keyboard) {
		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("Keyboard");
		gtk_notebook_append_page(GTK_NOTEBOOK(n), v, l);

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("Global Auto Repeat");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When enabled, all keyboard keys will auto-repeat;\n\
otherwise, only per-key autorepeat settings are\n\
observed.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(global_autorepeat_toggled), NULL);
		controls.Keyboard.GlobalAutoRepeat = u;

		f = gtk_frame_new("Key Click Percent (%)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_percent), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Set the key click volume as a percentage of\n\
maximum volume: from 0% to 100%.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(keyclick_percent_value_changed), NULL);
		controls.Keyboard.KeyClickPercent = h;

		f = gtk_frame_new("Bell Percent (%)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_percent), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Set the bell volume as a percentage of\n\
maximum volume: from 0% to 100%.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(bell_percent_value_changed), NULL);
		controls.Keyboard.BellPercent = h;

		f = gtk_frame_new("Bell Pitch (Hz)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(60.0, 2000.0, 20.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_hertz), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Set the bell pitch in Hertz.  Usable values\n\
are from 200 to 800 Hz.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(bell_pitch_value_changed), NULL);
		controls.Keyboard.BellPitch = h;

		f = gtk_frame_new("Bell Duration (milliseconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(10.0, 500.0, 10.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_milliseconds), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Set the bell duration in milliseconds.  Usable\n\
values are 100 to 300 milliseconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(bell_duration_value_changed), NULL);
		controls.Keyboard.BellDuration = h;

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		u = gtk_button_new_with_label("Ring Bell");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "Press to ring bell.");
		g_signal_connect(G_OBJECT(u), "clicked", G_CALLBACK(ring_bell_clicked), NULL);
	}

	if (support.XKeyboard) {
		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("XKeyboard");
		gtk_notebook_append_page(GTK_NOTEBOOK(n), v, l);
		s = gtk_notebook_new();
		gtk_box_pack_start(GTK_BOX(v), s, TRUE, TRUE, 0);
		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("Repeat Keys");
		gtk_notebook_append_page(GTK_NOTEBOOK(s), v, l);

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("Repeat Keys Enabled");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When enabled, all keyboard keys will auto-repeat;\n\
otherwise, only per-key autorepeat settings are\n\
observed.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(repeat_keys_toggled), NULL);
		controls.XKeyboard.RepeatKeysEnabled = u;

		f = gtk_frame_new("Repeat Delay (milliseconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 1000.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_milliseconds), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Set the delay after key press before auto-repeat\n\
begins in milliseconds.  Usable values are from\n\
250 to 500 milliseconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(repeat_delay_value_changed), NULL);
		controls.XKeyboard.RepeatDelay = h;

		f = gtk_frame_new("Repeat Interval (milliseconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(10.0, 100.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_milliseconds), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Set the interval between repeats after auto-repeat\n\
has begun.  Usable values are from 10 to 100\n\
milliseconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(repeat_interval_value_changed), NULL);
		controls.XKeyboard.RepeatInterval = h;

		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("Slow Keys");
		gtk_notebook_append_page(GTK_NOTEBOOK(s), v, l);

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("Slow Keys Enabled");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When checked, slow keys are enabled;\n\
otherwise slow keys are disabled.\n\
When enabled, keys pressed and released\n\
before the slow keys delay expires will\n\
be ignored.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(slow_keys_toggled), NULL);
		controls.XKeyboard.SlowKeysEnabled = u;

		f = gtk_frame_new("Slow Keys Delay (milliseconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 1000.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_milliseconds), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Set the duration in milliseconds for which a\n\
key must remain pressed to be considered\n\
a key press.  Usable values are 100 to 300\n\
milliseconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(slow_keys_delay_value_changed), NULL);
		controls.XKeyboard.SlowKeysDelay = h;

		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("Bounce Keys");
		gtk_notebook_append_page(GTK_NOTEBOOK(s), v, l);

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("Bounce Keys Enabled");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When checked, keys that are repeatedly\n\
pressed within the debounce delay will be\n\
ignored; otherwise, keys are not debounced.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(bounce_keys_toggled), NULL);
		controls.XKeyboard.BounceKeysEnabled = u;

		f = gtk_frame_new("Debounce Delay (milliseconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 1000.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_milliseconds), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Ignores repeated key presses and releases\n\
that occur within the debounce delay after\n\
the key was released.  Usable values are\n\
300 milliseconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(debounce_delay_value_changed), NULL);
		controls.XKeyboard.DebounceDelay = h;

		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("Sticky Keys");
		gtk_notebook_append_page(GTK_NOTEBOOK(s), v, l);

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("Sticky Keys Enabled");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When checked, sticky keys are enabled;\n\
otherwise, sticky keys are disabled.\n\
When enabled, modifier keys will stick\n\
when pressed and released until a non-\n\
modifier key is pressed.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(sticky_keys_toggled), NULL);
		controls.XKeyboard.StickyKeysEnabled = u;

		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("Mouse Keys");
		gtk_notebook_append_page(GTK_NOTEBOOK(s), v, l);

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("Mouse Keys Enabled");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When checked, mouse keys are enabled;\n\
otherwise they are disabled.  Mouse\n\
keys permit operating the pointer using\n\
only the keyboard.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(mouse_keys_toggled), NULL);
		controls.XKeyboard.MouseKeysEnabled = u;

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		q = gtk_hbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(f), q);
		l = gtk_label_new("Default Mouse Button");
		gtk_box_pack_start(GTK_BOX(q), l, FALSE, FALSE, 5);
		u = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(u), "1");
		gtk_combo_box_append_text(GTK_COMBO_BOX(u), "2");
		gtk_combo_box_append_text(GTK_COMBO_BOX(u), "3");
		gtk_combo_box_append_text(GTK_COMBO_BOX(u), "4");
		gtk_combo_box_append_text(GTK_COMBO_BOX(u), "5");
		gtk_combo_box_append_text(GTK_COMBO_BOX(u), "6");
		gtk_combo_box_append_text(GTK_COMBO_BOX(u), "7");
		gtk_combo_box_append_text(GTK_COMBO_BOX(u), "8");
		gtk_box_pack_end(GTK_BOX(q), u, TRUE, TRUE, 0);
		gtk_widget_set_tooltip_markup(u, "Select the default mouse button.");
		g_signal_connect(G_OBJECT(u), "changed", G_CALLBACK(default_mouse_button_changed), NULL);
		controls.XKeyboard.MouseKeysDfltBtn = u;

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("Mouse Keys Accel Enabled");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When checked, mouse key acceleration\n\
is enabled; otherwise it is disabled.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(mouse_keys_accel_toggled), NULL);
		controls.XKeyboard.MouseKeysAccelEnabled = u;

		f = gtk_frame_new("Mouse Keys Delay (milliseconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 1000.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_milliseconds), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Specifies the amount of time in milliseconds\n\
between the initial key press and the first\n\
repeated motion event.  A usable value is\n\
about 160 milliseconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(mouse_keys_delay_value_changed), NULL);
		controls.XKeyboard.MouseKeysDelay = h;

		f = gtk_frame_new("Mouse Keys Interval (milliseconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 1000.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_milliseconds), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Specifies the amount of time in milliseconds\n\
between repeated mouse key events.  Usable\n\
values are from 10 to 40 milliseconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(mouse_keys_interval_value_changed),
				 NULL);
		controls.XKeyboard.MouseKeysInterval = h;

		f = gtk_frame_new("Mouse Keys Time to Maximum (count)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(1.0, 100.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Sets the number of key presses after which the\n\
mouse key acceleration will be at the maximum.\n\
Usable values are from 10 to 40.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(mouse_keys_time_to_max_value_changed),
				 NULL);
		controls.XKeyboard.MouseKeysTimeToMax = h;

		f = gtk_frame_new("Mouse Keys Maximum Speed (multiplier)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Specifies the multiplier for mouse events at\n\
the maximum speed.  Usable values are\n\
from 10 to 40.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(mouse_keys_max_speed_value_changed),
				 NULL);
		controls.XKeyboard.MouseKeysMaxSpeed = h;

		f = gtk_frame_new("Mouse Keys Curve (factor)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(-1000.0, 1000.0, 50.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Sets the curve ramp up to maximum acceleration.\n\
Negative values ramp sharply to the maximum;\n\
positive values ramp slowly.  Usable values are\n\
from -1000 to 1000.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(mouse_keys_curve_value_changed), NULL);
		controls.XKeyboard.MouseKeysCurve = h;
	}

	if (support.ScreenSaver) {
		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("Screen Saver");
		gtk_notebook_append_page(GTK_NOTEBOOK(n), v, l);

		f = gtk_frame_new("Timeout (seconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		q = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(f), q);
		h = gtk_hscale_new_with_range(0.0, 3600.0, 30.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_seconds), NULL);
		gtk_box_pack_start(GTK_BOX(q), h, FALSE, FALSE, 0);
		u = gtk_button_new_with_label("Activate Screen Saver");
		gtk_box_pack_start(GTK_BOX(q), u, FALSE, FALSE, 0);
		gtk_widget_set_tooltip_markup(u, "Click to activate the screen saver.");
		g_signal_connect(G_OBJECT(u), "clicked", G_CALLBACK(activate_screensaver_clicked), NULL);
		gtk_widget_set_tooltip_markup(h, "\
Specify the time in seconds that pointer and keyboard\n\
must be idle before the screensaver is activated.\n\
Typical values are 600 seconds (10 minutes).  Set\n\
to zero to disable the screensaver.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(screensaver_timeout_value_changed),
				 NULL);
		controls.ScreenSaver.Timeout = h;

		f = gtk_frame_new("Interval (seconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		q = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(f), q);
		h = gtk_hscale_new_with_range(0.0, 3600.0, 30.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_seconds), NULL);
		gtk_box_pack_start(GTK_BOX(q), h, FALSE, FALSE, 0);
		u = gtk_button_new_with_label("Rotate Screen Saver");
		gtk_box_pack_start(GTK_BOX(q), u, FALSE, FALSE, 0);
		gtk_widget_set_tooltip_markup(u, "Click to rotate the screen saver.");
		g_signal_connect(G_OBJECT(u), "clicked", G_CALLBACK(rotate_screensaver_clicked), NULL);
		gtk_widget_set_tooltip_markup(h, "\
Specify the time in seconds after which the screen\n\
saver will change (if other than blanking).  A\n\
typical value is 600 seconds (10 minutes).");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(screensaver_interval_value_changed),
				 NULL);
		controls.ScreenSaver.Interval = h;

		f = gtk_frame_new("Blanking");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		r = gtk_vbox_new(FALSE, 0);
		u = gtk_radio_button_new_with_label(NULL, "Don't Prefer Blanking");
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(u));
		gtk_widget_set_tooltip_markup(u, "\
When checked, use a screen saver if enabled;\n\
otherwise, blank the screen instead of using\n\
a screen saver enabled.");
		gtk_box_pack_start(GTK_BOX(r), u, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(prefer_blanking_toggled), NULL);
		controls.ScreenSaver.Preferblanking[1] = u;
		u = gtk_radio_button_new_with_label(group, "Prefer Blanking");
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(u));
		gtk_widget_set_tooltip_markup(u, "\
When checked, blank the screen instead of\n\
using a screen saver; otherwise, use a screen\n\
saver if enabled.");
		gtk_box_pack_start(GTK_BOX(r), u, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(prefer_blanking_toggled), NULL);
		controls.ScreenSaver.Preferblanking[2] = u;
		u = gtk_radio_button_new_with_label(group, "Default Blanking");
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(u), FALSE);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(u));
		gtk_widget_set_tooltip_markup(u, "\
When checked, use the default blanking for the\n\
server.");
		gtk_box_pack_start(GTK_BOX(r), u, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(prefer_blanking_toggled), NULL);
		controls.ScreenSaver.Preferblanking[0] = u;
		gtk_container_add(GTK_CONTAINER(f), r);

		f = gtk_frame_new("Exposures");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		r = gtk_vbox_new(FALSE, 0);
		u = gtk_radio_button_new_with_label(NULL, "Don't Allow Exposures");
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(u));
		gtk_widget_set_tooltip_markup(u, "\
When checked, do not use a screen saver when the\n\
server is not capable of performing screen saving\n\
without sending exposure events to existing clients.");
		gtk_box_pack_start(GTK_BOX(r), u, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(allow_exposures_toggled), NULL);
		controls.ScreenSaver.Allowexposures[1] = u;
		u = gtk_radio_button_new_with_label(group, "Allow Exposures");
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(u));
		gtk_widget_set_tooltip_markup(u, "\
When checked, use a screen saver even when the server\n\
is not capable of performing screen saving without\n\
sending exposure events to existing clients.  Not\n\
normally needed nowadays.");
		gtk_box_pack_start(GTK_BOX(r), u, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(allow_exposures_toggled), NULL);
		controls.ScreenSaver.Allowexposures[2] = u;
		u = gtk_radio_button_new_with_label(group, "Default Exposures");
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(u), FALSE);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(u));
		gtk_widget_set_tooltip_markup(u, "\
When checked, us the default exposures for the\n\
server.");
		gtk_box_pack_start(GTK_BOX(r), u, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(allow_exposures_toggled), NULL);
		controls.ScreenSaver.Allowexposures[0] = u;
		gtk_container_add(GTK_CONTAINER(f), r);
	}
	if (support.DPMS) {
		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("DPMS");
		gtk_notebook_append_page(GTK_NOTEBOOK(n), v, l);

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("DPMS Enabled");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When checked, enable DPMS functions;\n\
otherwise disabled.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(dpms_toggled), NULL);
		controls.DPMS.State = u;

		f = gtk_frame_new("Standby Timeout (seconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		q = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(f), q);
		h = gtk_hscale_new_with_range(0.0, 3600.0, 30.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_seconds), NULL);
		gtk_box_pack_start(GTK_BOX(q), h, FALSE, FALSE, 0);
		u = gtk_button_new_with_label("Activate Standby");
		gtk_box_pack_start(GTK_BOX(q), u, FALSE, FALSE, 0);
		gtk_widget_set_tooltip_markup(u, "Click to activate standby mode.");
		g_signal_connect(G_OBJECT(u), "clicked", G_CALLBACK(activate_standby_clicked), NULL);
		gtk_widget_set_tooltip_markup(h, "\
Specifies the period of inactivity after which the\n\
monitor will enter standby mode.  A typical value\n\
is 600 seconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(standby_timeout_value_changed), NULL);
		controls.DPMS.StandbyTimeout = h;

		f = gtk_frame_new("Suspend Timeout (seconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		q = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(f), q);
		h = gtk_hscale_new_with_range(0.0, 3600.0, 30.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_seconds), NULL);
		gtk_box_pack_start(GTK_BOX(q), h, FALSE, FALSE, 0);
		u = gtk_button_new_with_label("Activate Suspend");
		gtk_box_pack_start(GTK_BOX(q), u, FALSE, FALSE, 0);
		gtk_widget_set_tooltip_markup(u, "Click to activate suspend mode.");
		g_signal_connect(G_OBJECT(u), "clicked", G_CALLBACK(activate_suspend_clicked), NULL);
		gtk_widget_set_tooltip_markup(h, "\
Specifies the period of inactivity after which the\n\
monitor will enter suspend mode.  A typical value\n\
is 1200 seconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(suspend_timeout_value_changed), NULL);
		controls.DPMS.SuspendTimeout = h;

		f = gtk_frame_new("Off Timeout (seconds)");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		q = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(f), q);
		h = gtk_hscale_new_with_range(0.0, 3600.0, 30.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_seconds), NULL);
		gtk_box_pack_start(GTK_BOX(q), h, FALSE, FALSE, 0);
		u = gtk_button_new_with_label("Activate Off");
		gtk_box_pack_start(GTK_BOX(q), u, FALSE, FALSE, 0);
		gtk_widget_set_tooltip_markup(u, "Click to activate off mode.");
		g_signal_connect(G_OBJECT(u), "clicked", G_CALLBACK(activate_off_clicked), NULL);
		gtk_widget_set_tooltip_markup(h, "\
Specifies the period of inactivity after which the\n\
monitor will be turned off.  A typical value is\n\
1800 seconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(off_timeout_value_changed), NULL);
		controls.DPMS.OffTimeout = h;
	}
	if (support.XF86Misc) {
		v = gtk_vbox_new(FALSE, 5);
		gtk_container_set_border_width(GTK_CONTAINER(v), 5);
		l = gtk_label_new("Miscellaneous");
		gtk_notebook_append_page(GTK_NOTEBOOK(n), v, l);

		f = gtk_frame_new("Keyboard");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		q = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(f), q);

		f = gtk_frame_new("Rate");
		gtk_box_pack_start(GTK_BOX(q), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 1000.0, 1.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_hertz), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Set the number of key repeats per second.  Usable\n\
values are between 0 and 300.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(keyboard_rate_value_changed), NULL);
		controls.XF86Misc.KeyboardRate = h;

		f = gtk_frame_new("Delay");
		gtk_box_pack_start(GTK_BOX(q), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 2000.0, 10.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_milliseconds), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Specifies the delay in milliseconds after a key is\n\
pressed before the key starts repeating.  Usable\n\
values are between 10 and 500 milliseconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(keyboard_delay_value_changed), NULL);
		controls.XF86Misc.KeyboardDelay = h;

		f = gtk_frame_new("Mouse");
		gtk_box_pack_start(GTK_BOX(v), f, FALSE, FALSE, 0);
		q = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(f), q);

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(q), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("Emulate 3 Buttons");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When enabled, emulate a 3-button mouse by clicking buttons\n\
1 and 3 at about the same time; otherwise, no emulation is\n\
performed.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(emulate_3_buttons_toggled), NULL);
		controls.XF86Misc.MouseEmulate3Buttons = u;

		f = gtk_frame_new("Emulate 3 Buttons Timeout");
		gtk_box_pack_start(GTK_BOX(q), f, FALSE, FALSE, 0);
		h = gtk_hscale_new_with_range(0.0, 2000.0, 10.0);
		gtk_scale_set_draw_value(GTK_SCALE(h), TRUE);
		g_signal_connect(G_OBJECT(h), "format-value", G_CALLBACK(format_value_milliseconds), NULL);
		gtk_container_add(GTK_CONTAINER(f), h);
		gtk_widget_set_tooltip_markup(h, "\
Specifies the delay after button 1 or 3 is pressed after\n\
which the button presses will be considered independent\n\
and not a 3-button emulation.  Usable values are from\n\
10 to 200 milliseconds.");
		g_signal_connect(G_OBJECT(h), "value-changed", G_CALLBACK(emulate_3_timeout_value_changed), NULL);
		controls.XF86Misc.MouseEmulate3Timeout = h;

		f = gtk_frame_new(NULL);
		gtk_box_pack_start(GTK_BOX(q), f, FALSE, FALSE, 0);
		u = gtk_check_button_new_with_label("Chord Middle");
		gtk_container_add(GTK_CONTAINER(f), u);
		gtk_widget_set_tooltip_markup(u, "\
When enabled, treat buttons 1 and 3 pressed at the same\n\
time as button 2; otherwise, buttons 1 and 3 are\n\
considered independent.");
		g_signal_connect(G_OBJECT(u), "toggled", G_CALLBACK(chord_middle_toggled), NULL);
		controls.XF86Misc.MouseChordMiddle = u;
	}

	return (w);
}

static void
edit_sav_values(void)
{
	if (editor) {
		gtk_widget_destroy(GTK_WIDGET(editor));
		editor = NULL;
	}
	/* grab once more before exiting, in case we missed and update */
	edit_get_values();
	/* write the file */
	save_config();
}

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
#endif

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
get_icons(GIcon *gicon, const char *const *inames)
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
		for (iname = inames; *iname; iname++) {
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

	/* FIXME: look up entry file too */
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
	pixbuf = get_icons(gicon, (const char *const *)inames);
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

static void
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
	return GTK_EVENT_PROPAGATE; /* event not fully handled */
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

Sequence *
add_sequence(XdeScreen *xscr, const char *id, SnStartupSequence * sn_seq)
{
	Sequence *seq;
	Time timestamp;
	XdeMonitor *xmon;

	if ((seq = calloc(1, sizeof(*seq)))) {
		int screen;
		GdkPixbuf *pixbuf;
		const char *name = NULL, *desc = NULL, *tip;
		char *markup;

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
		pixbuf = get_sequence_pixbuf(seq);

		/* FIXME: go get .desktop file and use that too. */

		if (!name)
			name = sn_startup_sequence_get_name(seq->seq);
		if (!name)
			name = sn_startup_sequence_get_wmclass(seq->seq);
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

		if (!gtk_widget_get_mapped(xmon->popup)) {
			GdkScreen *scrn = gdk_display_get_screen(disp, xscr->index);

#if 0
			GdkEventMask mask =
			    GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
			    GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
#endif

			DPRINTF(1, "popping the window\n");
			gtk_window_set_screen(GTK_WINDOW(xmon->popup), scrn);
			gtk_window_set_position(GTK_WINDOW(xmon->popup), GTK_WIN_POS_CENTER_ALWAYS);
			gtk_window_present(GTK_WINDOW(xmon->popup));
			gtk_widget_show_now(GTK_WIDGET(xmon->popup));
#if 0
			gdk_pointer_grab(xmon->popup->window, TRUE, mask, NULL, NULL, GDK_CURRENT_TIME);
#endif

		}
	}
	return (seq);
}

void
cha_sequence(XdeScreen *xscr, Sequence *seq)
{
	/* FIXME: change information in monitor's list store */
}

void
rem_sequence(XdeScreen *xscr, Sequence *seq)
{
	/* FIXME: remove from monitor's list store */
	*seq->list = g_list_remove(*seq->list, seq);
	sn_startup_sequence_unref(seq->seq);
	free(seq);
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
	if ((val = putXrmBool(resources.Keyboard.GlobalAutoRepeat))) {
		put_resource(rdb, "keyboard.globalAutoRepeat", val);
		g_free(val);
	}
	if ((val = putXrmInt(resources.Keyboard.KeyClickPercent))) {
		put_resource(rdb, "keyboard.keyClickPercent", val);
		g_free(val);
	}
	if ((val = putXrmInt(resources.Keyboard.BellPercent))) {
		put_resource(rdb, "keyboard.bellPercent", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.Keyboard.BellPitch))) {
		put_resource(rdb, "keyboard.bellPitch", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.Keyboard.BellDuration))) {
		put_resource(rdb, "keyboard.bellDuration", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.Pointer.AccelerationNumerator))) {
		put_resource(rdb, "pointer.accelerationNumerator", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.Pointer.AccelerationDenominator))) {
		put_resource(rdb, "pointer.accelerationDenominator", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.Pointer.Threshold))) {
		put_resource(rdb, "pointer.threshold", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.ScreenSaver.Timeout))) {
		put_resource(rdb, "screenSaver.timeout", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.ScreenSaver.Interval))) {
		put_resource(rdb, "screenSaver.interval", val);
		g_free(val);
	}
	if ((val = putXrmBlanking(resources.ScreenSaver.Preferblanking))) {
		put_resource(rdb, "screenSaver.preferBlanking", val);
		g_free(val);
	}
	if ((val = putXrmExposures(resources.ScreenSaver.Allowexposures))) {
		put_resource(rdb, "screenSaver.allowExposures", val);
		g_free(val);
	}
	if ((val = putXrmInt(resources.DPMS.State))) {
		put_resource(rdb, "dPMS.state", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.DPMS.StandbyTimeout))) {
		put_resource(rdb, "dPMS.standbyTimeout", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.DPMS.SuspendTimeout))) {
		put_resource(rdb, "dPMS.suspendTimeout", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.DPMS.OffTimeout))) {
		put_resource(rdb, "dPMS.offTimeout", val);
		g_free(val);
	}
	if ((val = putXrmBool(resources.XKeyboard.RepeatKeysEnabled))) {
		put_resource(rdb, "xKeyboard.repeatKeysEnabled", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XKeyboard.RepeatDelay))) {
		put_resource(rdb, "xKeyboard.repeatDelay", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XKeyboard.RepeatInterval))) {
		put_resource(rdb, "xKeyboard.repeatInterval", val);
		g_free(val);
	}
	if ((val = putXrmBool(resources.XKeyboard.SlowKeysEnabled))) {
		put_resource(rdb, "xKeyboard.slowKeysEnabled", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XKeyboard.SlowKeysDelay))) {
		put_resource(rdb, "xKeyboard.slowKeysDelay", val);
		g_free(val);
	}
	if ((val = putXrmBool(resources.XKeyboard.BounceKeysEnabled))) {
		put_resource(rdb, "xKeyboard.bounceKeysEnabled", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XKeyboard.DebounceDelay))) {
		put_resource(rdb, "xKeyboard.debounceDelay", val);
		g_free(val);
	}
	if ((val = putXrmBool(resources.XKeyboard.StickyKeysEnabled))) {
		put_resource(rdb, "xKeyboard.stickyKeysEnabled", val);
		g_free(val);
	}
	if ((val = putXrmBool(resources.XKeyboard.MouseKeysEnabled))) {
		put_resource(rdb, "xKeyboard.mouseKeysEnabled", val);
		g_free(val);
	}
	if ((val = putXrmButton(resources.XKeyboard.MouseKeysDfltBtn))) {
		put_resource(rdb, "xKeyboard.mouseKeysDfltBtn", val);
		g_free(val);
	}
	if ((val = putXrmBool(resources.XKeyboard.MouseKeysAccelEnabled))) {
		put_resource(rdb, "xKeyboard.mouseKeysAccelEnabled", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XKeyboard.MouseKeysDelay))) {
		put_resource(rdb, "xKeyboard.mouseKeysDelay", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XKeyboard.MouseKeysInterval))) {
		put_resource(rdb, "xKeyboard.mouseKeysInterval", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XKeyboard.MouseKeysTimeToMax))) {
		put_resource(rdb, "xKeyboard.mouseKeysTimeToMax", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XKeyboard.MouseKeysMaxSpeed))) {
		put_resource(rdb, "xKeyboard.mouseKeysMaxSpeed", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XKeyboard.MouseKeysCurve))) {
		put_resource(rdb, "xKeyboard.mouseKeysCurve", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XF86Misc.KeyboardRate))) {
		put_resource(rdb, "xF86Misc.keyboardRate", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XF86Misc.KeyboardDelay))) {
		put_resource(rdb, "xF86Misc.keyboardDelay", val);
		g_free(val);
	}
	if ((val = putXrmBool(resources.XF86Misc.MouseEmulate3Buttons))) {
		put_resource(rdb, "xF86Misc.mouseEmulate3Buttons", val);
		g_free(val);
	}
	if ((val = putXrmUint(resources.XF86Misc.MouseEmulate3Timeout))) {
		put_resource(rdb, "xF86Misc.mouseEmulate3Timeout", val);
		g_free(val);
	}
	if ((val = putXrmBool(resources.XF86Misc.MouseChordMiddle))) {
		put_resource(rdb, "xF86Misc.mouseChordMiddle", val);
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
				       KFK_XKeyboard_IgnoreGroupLockModsEnabled,
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
	if ((val = get_resource(rdb, "keyboard.globalAutoRepeat", NULL)))
		getXrmBool(val, &resources.Keyboard.GlobalAutoRepeat);
	if ((val = get_resource(rdb, "keyboard.keyClickPercent", NULL)))
		getXrmInt(val, &resources.Keyboard.KeyClickPercent);
	if ((val = get_resource(rdb, "keyboard.bellPercent", NULL)))
		getXrmInt(val, &resources.Keyboard.BellPercent);
	if ((val = get_resource(rdb, "keyboard.bellPitch", NULL)))
		getXrmUint(val, &resources.Keyboard.BellPitch);
	if ((val = get_resource(rdb, "keyboard.bellDuration", NULL)))
		getXrmUint(val, &resources.Keyboard.BellDuration);
	if ((val = get_resource(rdb, "pointer.accelerationNumerator", NULL)))
		getXrmUint(val, &resources.Pointer.AccelerationNumerator);
	if ((val = get_resource(rdb, "pointer.accelerationDenominator", NULL)))
		getXrmUint(val, &resources.Pointer.AccelerationDenominator);
	if ((val = get_resource(rdb, "pointer.threshold", NULL)))
		getXrmUint(val, &resources.Pointer.Threshold);
	if ((val = get_resource(rdb, "screenSaver.timeout", NULL)))
		getXrmUint(val, &resources.ScreenSaver.Timeout);
	if ((val = get_resource(rdb, "screenSaver.interval", NULL)))
		getXrmUint(val, &resources.ScreenSaver.Interval);
	if ((val = get_resource(rdb, "screenSaver.preferBlanking", NULL)))
		getXrmBlanking(val, &resources.ScreenSaver.Preferblanking);
	if ((val = get_resource(rdb, "screenSaver.allowExposures", NULL)))
		getXrmExposures(val, &resources.ScreenSaver.Allowexposures);
	if ((val = get_resource(rdb, "dPMS.state", NULL)))
		getXrmInt(val, &resources.DPMS.State);
	if ((val = get_resource(rdb, "dPMS.standbyTimeout", NULL)))
		getXrmUint(val, &resources.DPMS.StandbyTimeout);
	if ((val = get_resource(rdb, "dPMS.suspendTimeout", NULL)))
		getXrmUint(val, &resources.DPMS.SuspendTimeout);
	if ((val = get_resource(rdb, "dPMS.offTimeout", NULL)))
		getXrmUint(val, &resources.DPMS.OffTimeout);
	if ((val = get_resource(rdb, "xKeyboard.repeatKeysEnabled", NULL)))
		getXrmBool(val, &resources.XKeyboard.RepeatKeysEnabled);
	if ((val = get_resource(rdb, "xKeyboard.repeatDelay", NULL)))
		getXrmUint(val, &resources.XKeyboard.RepeatDelay);
	if ((val = get_resource(rdb, "xKeyboard.repeatInterval", NULL)))
		getXrmUint(val, &resources.XKeyboard.RepeatInterval);
	if ((val = get_resource(rdb, "xKeyboard.slowKeysEnabled", NULL)))
		getXrmBool(val, &resources.XKeyboard.SlowKeysEnabled);
	if ((val = get_resource(rdb, "xKeyboard.slowKeysDelay", NULL)))
		getXrmUint(val, &resources.XKeyboard.SlowKeysDelay);
	if ((val = get_resource(rdb, "xKeyboard.bounceKeysEnabled", NULL)))
		getXrmBool(val, &resources.XKeyboard.BounceKeysEnabled);
	if ((val = get_resource(rdb, "xKeyboard.debounceDelay", NULL)))
		getXrmUint(val, &resources.XKeyboard.DebounceDelay);
	if ((val = get_resource(rdb, "xKeyboard.stickyKeysEnabled", NULL)))
		getXrmBool(val, &resources.XKeyboard.StickyKeysEnabled);
	if ((val = get_resource(rdb, "xKeyboard.mouseKeysEnabled", NULL)))
		getXrmBool(val, &resources.XKeyboard.MouseKeysEnabled);
	if ((val = get_resource(rdb, "xKeyboard.mouseKeysDfltBtn", NULL)))
		getXrmButton(val, &resources.XKeyboard.MouseKeysDfltBtn);
	if ((val = get_resource(rdb, "xKeyboard.mouseKeysAccelEnabled", NULL)))
		getXrmBool(val, &resources.XKeyboard.MouseKeysAccelEnabled);
	if ((val = get_resource(rdb, "xKeyboard.mouseKeysDelay", NULL)))
		getXrmUint(val, &resources.XKeyboard.MouseKeysDelay);
	if ((val = get_resource(rdb, "xKeyboard.mouseKeysInterval", NULL)))
		getXrmUint(val, &resources.XKeyboard.MouseKeysInterval);
	if ((val = get_resource(rdb, "xKeyboard.mouseKeysTimeToMax", NULL)))
		getXrmUint(val, &resources.XKeyboard.MouseKeysTimeToMax);
	if ((val = get_resource(rdb, "xKeyboard.mouseKeysMaxSpeed", NULL)))
		getXrmUint(val, &resources.XKeyboard.MouseKeysMaxSpeed);
	if ((val = get_resource(rdb, "xKeyboard.mouseKeysCurve", NULL)))
		getXrmUint(val, &resources.XKeyboard.MouseKeysCurve);
	if ((val = get_resource(rdb, "xF86Misc.keyboardRate", NULL)))
		getXrmUint(val, &resources.XF86Misc.KeyboardRate);
	if ((val = get_resource(rdb, "xF86Misc.keyboardDelay", NULL)))
		getXrmUint(val, &resources.XF86Misc.KeyboardDelay);
	if ((val = get_resource(rdb, "xF86Misc.mouseEmulate3Buttons", NULL)))
		getXrmBool(val, &resources.XF86Misc.MouseEmulate3Buttons);
	if ((val = get_resource(rdb, "xF86Misc.mouseEmulate3Timeout", NULL)))
		getXrmUint(val, &resources.XF86Misc.MouseEmulate3Timeout);
	if ((val = get_resource(rdb, "xF86Misc.mouseChordMiddle", NULL)))
		getXrmBool(val, &resources.XF86Misc.MouseChordMiddle);

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
	GdkGrabStatus status;
	Window win;

	PTRACE(5);
	gdk_display_get_pointer(disp, NULL, NULL, NULL, &xmon->mask);
	DPRINTF(1, "modifier mask was: 0x%08x\n", xmon->mask);

	stop_popup_timer(xmon);
	gtk_window_set_position(GTK_WINDOW(xmon->popup), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_present(GTK_WINDOW(xmon->popup));
	gtk_widget_show_now(GTK_WIDGET(xmon->popup));
	win = GDK_WINDOW_XID(xmon->popup->window);
	if (!xmon->pointer) {
		GdkEventMask mask =
		    GDK_POINTER_MOTION_MASK |
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON_MOTION_MASK |
		    GDK_BUTTON_PRESS_MASK |
		    GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
		PTRACE(5);
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
	if (!xmon->keyboard) {
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

#if NEED_POPUP_WINDOW

static void
windows_changed(WnckScreen *wnck, XdeMonitor *xmon)
{
	GdkGrabStatus status;
	Window win;

	PTRACE(5);
	gdk_display_get_pointer(disp, NULL, NULL, NULL, &xmon->mask);
	DPRINTF(1, "modifier mask was: 0x%08x\n", xmon->mask);

	stop_popup_timer(xmon);
	gtk_window_set_position(GTK_WINDOW(xmon->popup), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_present(GTK_WINDOW(xmon->popup));
	gtk_widget_show_now(GTK_WIDGET(xmon->popup));	/* XXX works for menus? */
	win = GDK_WINDOW_XID(xmon->popup->window);
	if (!xmon->pointer) {
		GdkEventMask mask =
		    GDK_POINTER_MOTION_MASK |
		    GDK_POINTER_MOTION_HINT_MASK |
		    GDK_BUTTON_MOTION_MASK |
		    GDK_BUTTON_PRESS_MASK |
		    GDK_BUTTON_RELEASE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
		PTRACE(5);
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
	if (!xmon->keyboard) {
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

#endif				/* NEED_POPUP_WINDOW */

#if NEED_CLIENT_INFO

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
	if (prop == None || prop == _XA_NET_CLIENT_LIST_STACKING) {
	}
	if (prop == None || prop == _XA_NET_CLIENT_LIST) {
	} else if (prop == None || prop == _XA_WIN_CLIENT_LIST) {
	}
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
			set_current_desktop(xscr, num, xev->xbutton.time);
			return GDK_FILTER_REMOVE;
		case 5:
			update_current_desktop(xscr, None);
			num = (xscr->current + 1 + xscr->desks) % xscr->desks;
			set_current_desktop(xscr, num, xev->xbutton.time);
			return GDK_FILTER_REMOVE;
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

#if NEED_DESKTOP_LAYOUT
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
	int format = 0, num;
	unsigned long nitems = 0, after = 0;
	unsigned long *data = NULL;
	Bool propok = False;

	PTRACE(5);
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

	refresh_layout(xscr);	/* XXX: should be deferred */
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
	}
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
}

static void
edit_input(XdeScreen *xscr)
{
	if (editor || (editor = create_window())) {
		gtk_window_set_screen(editor, xscr->scrn);
		edit_set_values();
		gtk_widget_show_all(GTK_WIDGET(editor));
	}
}

static void
show_stray(XdeScreen *xscr)
{
	if (!editor)
		editor = create_window();
	if (!trayicon)
		trayicon = create_icon(xscr);

	edit_set_values();
	gtk_status_icon_set_visible(trayicon, TRUE);
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

static guint deferred = 0;

static gboolean
deferred_action(gpointer user_data)
{
	deferred = 0;
	edit_get_values();
	if (editor)
		edit_set_values();
	return G_SOURCE_REMOVE;
}

static GdkFilterReturn
events_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	XEvent *xev = (typeof(xev)) xevent;

	/* defer action in case we get a burst of events */
	if (xev->type == state.XKeyboard.event) {
		DPRINTF(1, "got XKB event %d\n", xev->type);
		if (!deferred)
			deferred = g_timeout_add(1, &deferred_action, NULL);
	}
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
	edit_get_values();
	save_config();
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

#if NEED_POPUP_WINDOW
	g_signal_connect(G_OBJECT(wnck), "active_window_changed", G_CALLBACK(active_window_changed), xscr);
#endif				/* NEED_POPUP_WINDOW */
#if NEED_CLIENT_INFO
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
	init_extensions();
	edit_get_values();
	edit_put_values();
	if (options.trayicon) {
		xscr = screens + options.screen;
		show_stray(xscr);
	}
	if (options.editor) {
		xscr = screens + options.screen;
		edit_input(xscr);
	}
	gtk_main();
	edit_sav_values();
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

		c = getopt_long_only(argc, argv, "d:s:b:rqetf:nD::v::hVCH?", long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "d:s:b:rqetf:nDvhVCH?");
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
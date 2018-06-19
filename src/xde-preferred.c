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

#define GTK_EVENT_STOP		TRUE
#define GTK_EVENT_PROPAGATE	FALSE

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

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1
#define EXIT_SYNTAXERR  2

typedef enum {
	CommandDefault,
	CommandLaunch,
	CommandList,
	CommandSet,
	CommandEdit,
	CommandHelp,
	CommandVersion,
	CommandCopying,
} Command;

typedef enum {
	WhichDefault,
	WhichRecommend,
	WhichOther,
	WhichAll,
} Which;

typedef enum {
	KindDefault,
	KindTerminalEmulator,
	KindWebBrowser,
	KindMailReader,
	KindFileManager,
	KindTextEditor,
	KindScreenSaver,
	KindAudioMixer,
	KindAudioPlayer,
	KindVideoPlayer,
	KindCalculator,
	KindNone,
} Kind;

typedef struct {
	const char *kind;
	const char *category;
	const char *type;
} KindSpec;

KindSpec kinds[] = {
	[KindDefault]			= { "default",		NULL,			NULL			},
	[KindTerminalEmulator]		= { "terminal",		"TerminalEmulator",	NULL			},
	[KindWebBrowser]		= { "browser",		"WebBrowser",		"text/html"		},
	[KindMailReader]		= { "mailer",		"MailReader",		"message/rfc822"	},
	[KindFileManager]		= { "filemanager",	"FileManager",		"inode/directory"	},
	[KindTextEditor]		= { "editor",		"TextEditor",		"text/plain"		},
	[KindScreenSaver]		= { "screensaver",	"ScreenSaver",		NULL			},
	[KindAudioMixer]		= { "mixer",		"AudioMixer",		NULL			},
	[KindAudioPlayer]		= { "player",		"AudioPlayer",		NULL			},
	[KindVideoPlayer]		= { "movie",		"VideoPlayer",		NULL			},
	[KindCalculator]		= { "calculator",	"Calculator",		NULL			},
	[KindNone]			= { NULL,		NULL,			NULL			}
};

typedef struct {
	int debug;
	int output;
	char *display;
	int screen;
	Command command;
	Which which;
	Kind kind;
	char *category;
	char *type;
	char *appid;
	GList *types;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.display = NULL,
	.screen = -1,
	.command = CommandDefault,
	.which = WhichDefault,
	.kind = KindDefault,
	.category = NULL,
	.type = NULL,
	.appid = NULL,
	.types = NULL,
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
    %1$s [-l|--launch] [OPTIONS] [TYPE] -- [APP_OPTIONS]\n\
    %1$s {-L|--list} [OPTIONS]\n\
    %1$s {-S|--set} [OPTIONS] -- APPID\n\
    %1$s {-e|--edit} [OPTIONS]\n\
    %1$s {-h|--help}\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
", argv[0]);
}

const char *
show_which(Which which)
{
	switch (which) {
	case WhichDefault:
		return ("default");
	case WhichRecommend:
		return ("recommend");
	case WhichOther:
		return ("other");
	case WhichAll:
		return ("all");
	}
	return ("???");
}

const char *
show_kind(Kind kind)
{
	switch (kind) {
	case KindDefault:
		return ("default");
	case KindTerminalEmulator:
		return ("terminal");
	case KindWebBrowser:
		return ("browser");
	case KindMailReader:
		return ("mailer");
	case KindFileManager:
		return ("filemanager");
	case KindTextEditor:
		return ("editor");
	case KindScreenSaver:
		return ("screensaver");
	case KindAudioMixer:
		return ("mixder");
	case KindAudioPlayer:
		return ("player");
	case KindVideoPlayer:
		return ("movie");
	case KindCalculator:
		return ("calculator");
	case KindNone:
		return NULL;
	default:
		return ("???");
	}
}

static void
help(int argc, char *argv[])
{
	if (!options.output && !options.debug)
		return;
	/* *INDENT-OFF* */
	(void) fprintf(stdout, "\
Usage:\n\
    %1$s [-l|--launch] [OPTIONS] -- [APP_OPTIONS]\n\
    %1$s {-L|--list} [OPTIONS]\n\
    %1$s {-S|--set} [OPTIONS] -- APPID\n\
    %1$s {-e|--edit} [OPTIONS]\n\
    %1$s {-h|--help} [OPTIONS]\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
Command options:\n\
   [-l, --launch] -- [APP_OPTIONS]\n\
        launch the application for specified type with options\n\
    -L, --list\n\
        list applications for specified types\n\
    -S, --set -- APPID\n\
        set the application for specified types\n\
    -e, --edit\n\
        launch graphical editor for editing types\n\
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
    -w, --which {default|recommend|other|all}\n\
        specify which applications to list or set [default: %6$s]\n\
    -k, --kind KIND\n\
        specify the kind of application [default: %7$s]\n\
    -t, --type TYPE[,TYPE]\n\
        specify the content types [default: %8$s]\n\
    -g, --category CATEGORY[;CATEGORY]\n\
        specify the categories [default: %9$s]\n\
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
	, options.screen
	, show_which(options.which)
	, show_kind(options.kind)
	, options.type
	, options.category
);
	/* *INDENT-ON* */
}

void
set_defaults(void)
{
	const char *env;

	if ((env = getenv("DISPLAY")))
		options.display = strdup(env);
	options.kind = KindDefault;
	options.type = strdup("text/html");
	options.category = strdup("WebBrowser");
}

void
get_defaults(void)
{
	const char *p;
	int n;

	if (!options.display) {
		EPRINTF("No DISPLAY environment variable nor --display option\n");
		exit(EXIT_FAILURE);
	}
	if (options.screen < 0 && (p = strrchr(options.display, '.'))
	    && (n = strspn(++p, "012345689")) && *(p + n) == '\0')
		options.screen = atoi(p);
	if (options.command == CommandDefault)
		options.command = CommandLaunch;
}

void
check_type(int argc, char *argv[])
{
	char *t, *p;

	if ((t = options.type)) {
		if ((p = strchr(options.type, '/'))) {
			options.types = g_list_append(options.types, t);
			/* the type is a content-type */
		} else if (!strcasecmp(t, "calendar")		|| !strcmp(t, "Calendar")) {
			options.types = g_list_append(options.types, "text/calendar");
		} else if (!strcasecmp(t, "addressbook")	|| !strcmp(t, "ContactManager")) {
			options.types = g_list_append(options.types, "text/x-vcard");
		} else if (!strcasecmp(t, "dictionary")		|| !strcmp(t, "Dictionary")) {
		} else if (!strcasecmp(t, "mailer")		|| !strcmp(t, "Email")) {
			options.types = g_list_append(options.types, "application/mbox");
			options.types = g_list_append(options.types, "message/rfc822");
			options.types = g_list_append(options.types, "x-scheme-handler/mailto");
		} else if (					   !strcmp(t, "Presentation")) {
			options.types = g_list_append(options.types, "application/vnd.oasis.opendocument.presentation");
			options.types = g_list_append(options.types, "application/mspowerpoint");
			options.types = g_list_append(options.types, "application/vnd.ms-powerpoint");
			options.types = g_list_append(options.types, "application/vnd.sun.xml.impress");
			options.types = g_list_append(options.types, "application/vnd.openxmlformats-officedocument.presetnationml.presentation");
			options.types = g_list_append(options.types, "application/vnd.ms-powerpoint.slideshow.macroEnabled.12");
			options.types = g_list_append(options.types, "application/vnd.ms-powerpoint.presentation.macroEnabled.12");
		} else if (					   !strcmp(t, "Spreadsheet")) {
			options.types = g_list_append(options.types, "application/vnd.oasis.opendocument.spreadsheet");
			options.types = g_list_append(options.types, "application/vnd.sun.xml.calc");
			options.types = g_list_append(options.types, "application/msexcel");
			options.types = g_list_append(options.types, "application/vnd.ms-excel");
			options.types = g_list_append(options.types, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
			options.types = g_list_append(options.types, "application/csv");
			options.types = g_list_append(options.types, "application/excel");
			options.types = g_list_append(options.types, "application/tab-separated-values");
			options.types = g_list_append(options.types, "application/vnd.lotus-1-2-3");
			options.types = g_list_append(options.types, "application/x-dbase");
			options.types = g_list_append(options.types, "application/x-dos_ms_excel");
			options.types = g_list_append(options.types, "application/x-excel");
			options.types = g_list_append(options.types, "application/x-msexcel");
			options.types = g_list_append(options.types, "application/x-ms-excel");
			options.types = g_list_append(options.types, "application/x-quattropro");
			options.types = g_list_append(options.types, "application/x-123");
			options.types = g_list_append(options.types, "text/comma-separated-values");
			options.types = g_list_append(options.types, "text/tab-separated-values");
			options.types = g_list_append(options.types, "text/x-comma-separated-values");
			options.types = g_list_append(options.types, "text/x-csv");
			options.types = g_list_append(options.types, "application/vnd.oasis.opendocument.spreadsheet-flat-xml");
		} else if (					   !strcmp(t, "WordProcessor")) {
			options.types = g_list_append(options.types, "application/vnd.oasis.opendocument.text-template");
			options.types = g_list_append(options.types, "application/vnd.oasis.opendocument.text-web");
			options.types = g_list_append(options.types, "application/vnd.oasis.opendocument.text-master");
			options.types = g_list_append(options.types, "application/vnd.sun.xml.writer");
			options.types = g_list_append(options.types, "application/vnd.sun.xml.writer.template");
			options.types = g_list_append(options.types, "application/vnd.sun.xml.writer.global");
			options.types = g_list_append(options.types, "application/msword");
			options.types = g_list_append(options.types, "application/vnd.ms-word");
			options.types = g_list_append(options.types, "application/vnd.wordperfect");
			options.types = g_list_append(options.types, "application/wordperfect");
			options.types = g_list_append(options.types, "application/vnd.lotus-wordpro");
			options.types = g_list_append(options.types, "application/vnd.openxmlformats-officedocument.wordprocessingml.document");
			options.types = g_list_append(options.types, "application/vnd.ms-word.document.macroenabled.12");
			options.types = g_list_append(options.types, "application/vnd.openxmlformats-officedocument.wordprocessingml.template");
			options.types = g_list_append(options.types, "application/vnd.ms-word.template.macroenabled.12");
			options.types = g_list_append(options.types, "application/vnd.ms-works");
			options.types = g_list_append(options.types, "application/vnd.stardivision.write-global");
			options.types = g_list_append(options.types, "application/x-extension-text");
			options.types = g_list_append(options.types, "application/x-t602");
			options.types = g_list_append(options.types, "text/plain");
			options.types = g_list_append(options.types, "application/vnd.oasis.opendocument.text-flat-xml");
			options.types = g_list_append(options.types, "application/x-fictionbook+xml");
			options.types = g_list_append(options.types, "application/macwriteii");
			options.types = g_list_append(options.types, "application/x-aportisdoc");
			options.types = g_list_append(options.types, "application/prs.plucker");
			options.types = g_list_append(options.types, "application/vnd.palm");
			options.types = g_list_append(options.types, "application/carisworks");
		} else if (!strcasecmp(t, "imageviewer")	|| !strcmp(t, "2DGraphics")) {
			options.types = g_list_append(options.types, "image/gif");
			options.types = g_list_append(options.types, "image/jpeg");
			options.types = g_list_append(options.types, "image/jpg");
			options.types = g_list_append(options.types, "image/pjpeg");
			options.types = g_list_append(options.types, "image/png");
			options.types = g_list_append(options.types, "image/tiff");
			options.types = g_list_append(options.types, "image/x-bmp");
			options.types = g_list_append(options.types, "image/x-pcx");
			options.types = g_list_append(options.types, "image/x-png");
			options.types = g_list_append(options.types, "image/x-portable-anymap");
			options.types = g_list_append(options.types, "image/x-portable-bitmap");
			options.types = g_list_append(options.types, "image/x-portable-graymap");
			options.types = g_list_append(options.types, "image/x-portable-pixmap");
			options.types = g_list_append(options.types, "image/x-tga");
			options.types = g_list_append(options.types, "image/x-xbitmap");
		} else if (!strcasecmp(t, "pdfviewer")		|| !strcmp(t, "Viewer")) {
			options.types = g_list_append(options.types, "application/pdf");
		} else if (					   !strcmp(t, "Printing")) {
		} else if (					   !strcmp(t, "PackageManager")) {
			options.types = g_list_append(options.types, "application/x-rpm");
			options.types = g_list_append(options.types, "application/x-urpmi");
			options.types = g_list_append(options.types, "application/x-urpmi-media");
			options.types = g_list_append(options.types, "application/x-deb");
			options.types = g_list_append(options.types, "application/x-debian-package");
		} else if (!strcasecmp(t, "chat")		|| !strcmp(t, "IRCClient")) {
		} else if (					   !strcmp(t, "Feed")) {
			options.types = g_list_append(options.types, "application/rss+xml");
		} else if (					   !strcmp(t, "FileTransfer")) {
			options.types = g_list_append(options.types, "application/x-bittorrent");
		} else if (					   !strcmp(t, "News")) {
		} else if (					   !strcmp(t, "RemoteAccess")) {
			options.types = g_list_append(options.types, "application/x-remote-connection");
			options.types = g_list_append(options.types, "x-scheme-handler/vnc");
		} else if (!strcasecmp(t, "phone")		|| !strcmp(t, "Telephony")) {
		} else if (!strcasecmp(t, "conference")		|| !strcmp(t, "VideoConference")) {
		} else if (!strcasecmp(t, "browser")		|| !strcmp(t, "WebBrowser")) {
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
			options.types = g_list_append(options.types, "application/xxx");
		} else if (!strcasecmp(t, "audiomixer")		|| !strcmp(t, "Mixer")) {
		} else if (!strcasecmp(t, "audioplayer")	|| !strcmp(t, "Player")) {
		} else if (!strcasecmp(t, "movieplayer")	|| !strcmp(t, "Player")) {
		} else if (					   !strcmp(t, "Recorder")) {
		} else if (					   !strcmp(t, "Archiving")) {
		} else if (					   !strcmp(t, "DiscBurining")) {
		} else if (!strcasecmp(t, "filemanager")	|| !strcmp(t, "FileManager")) {
		} else if (!strcasecmp(t, "terminal")		|| !strcmp(t, "TerminalEmulator")) {
		} else if (					   !strcmp(t, "Monitor")) {
		} else if (!strcasecmp(t, "calculator")		|| !strcmp(t, "Calculator")) {
		} else if (!strcasecmp(t, "clock")		|| !strcmp(t, "Clock")) {
		} else if (!strcasecmp(t, "texteditor")		|| !strcmp(t, "TextEditor")) {
		} else if (					   !strcmp(t, "Documentation")) {
		} else if (!strcasecmp(t, "screensaver")) {
		} else if (!strcasecmp(t, "screenshot")) {
		} else {
			EPRINTF("%s: unrecognized type %s\n", argv[0], t);
			exit(EXIT_SYNTAXERR);
		}
	} else if (options.command != CommandLaunch && options.command != CommandSet) {
		EPRINTF("%s: need application type\n", argv[0]);
		exit(EXIT_SYNTAXERR);
	}
}

void
startup(int argc, char *argv[])
{
}

void
do_launch(int argc, char *argv[])
{
}

void
do_list(int argc, char *argv[])
{
	if (!options.type && !options.category) {
		EPRINTF("%s: must specify content type or category\n", argv[0]);
		exit(EXIT_SYNTAXERR);
	}
}

void
do_setapp(int argc, char *argv[])
{
}

void
do_editapps(int argc, char *argv[])
{
}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;

	setlocale(LC_ALL, "");
	set_defaults();

	while (1) {
		int c, val, k;
		char *endptr = NULL;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"launch",	no_argument,		NULL,	'l'},
			{"list",	no_argument,		NULL,	'L'},
			{"set",		no_argument,		NULL,	'S'},
			{"edit",	no_argument,		NULL,	'e'},

			{"display",	required_argument,	NULL,	'd'},
			{"screen",	required_argument,	NULL,	's'},
			{"which",	required_argument,	NULL,	'w'},
			{"kind",	required_argument,	NULL,	'k'},
			{"type",	required_argument,	NULL,	't'},
			{"category",	required_argument,	NULL,	'g'},

			{"debug",	optional_argument,	NULL,	'D'},
			{"verbose",	optional_argument,	NULL,	'v'},
			{"help",	no_argument,		NULL,	'h'},
			{"version",	no_argument,		NULL,	'V'},
			{"copying",	no_argument,		NULL,	'C'},
			{"?",		no_argument,		NULL,	'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "lLSed:s:w:k:t:g:D::v::hVCH?",
				     long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "lLSed:s:w:k:t:g:DvhVCH?");
#endif				/* _GNU_SOURCE */
		if (c == -1) {
			DPRINTF("%s: done options processing\n", argv[0]);
			break;
		}
		switch (c) {
		case 0:
			goto bad_usage;

		case 'l':	/* -l, --launch -- [APP_OPTIONS] */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandLaunch;
			options.command = CommandLaunch;
			break;
		case 'L':	/* -L, --list */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandList;
			options.command = CommandList;
			break;
		case 'S':	/* -S, --set -- APPID */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandSet;
			options.command = CommandSet;
			break;
		case 'e':	/* -e, --edit */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandEdit;
			options.command = CommandEdit;
			break;

		case 'd':       /* -d, --display DISPLAY */
			setenv("DISPLAY", optarg, TRUE);
			free(options.display);
			options.display = strdup(optarg);
			break;
		case 's':       /* -s, --screen SCREEN */
			options.screen = strtoul(optarg, &endptr, 0);
			if (*endptr)
				goto bad_option;
			break;
		case 'w':	/* -w, --which {default|recommend|all} */
			if (!strncasecmp(optarg, "default", strlen(optarg))) {
				options.which = WhichDefault;
			} else
			if (!strncasecmp(optarg, "recommend", strlen(optarg))) {
				options.which = WhichRecommend;
			} else
			if (!strncasecmp(optarg, "other", strlen(optarg))) {
				options.which = WhichOther;
			} else
			if (!strncasecmp(optarg, "all", strlen(optarg))) {
				options.which = WhichAll;
			} else
				goto bad_option;
			break;
		case 'k':
			for (k = 0; kinds[k].kind; k++) {
				if (!strncasecmp(optarg, kinds[k].kind, strlen(optarg))) {
					options.kind = k;
					free(options.type);
					options.type = NULL;
					if (kinds[k].type)
						options.type = strdup(kinds[k].type);
					free(options.category);
					options.category = NULL;
					if (kinds[k].category)
						options.category = strdup(kinds[k].category);
					break;
				}
			}
			if (!kinds[k].kind)
				goto bad_option;
			break;
		case 't':
			free(options.type);
			options.type = strdup(optarg);
			if (options.kind == KindDefault) {
				for (k = 0; kinds[k].kind; k++) {
					if (kinds[k].type && !strcasecmp(kinds[k].type, options.type)) {
						options.kind = k;
						free(options.category);
						options.category = NULL;
						if (kinds[k].category)
							options.category = strdup(kinds[k].category);
						break;
					}
				}
				if (!kinds[k].kind) {
					options.kind = KindNone;
					free(options.category);
					options.category = NULL;
				}
			}
			break;
		case 'g':
			free(options.category);
			options.category = strdup(optarg);
			if (options.kind == KindDefault) {
				for (k = 0; kinds[k].kind; k++) {
					if (kinds[k].category && !strcasecmp(kinds[k].category, options.category)) {
						options.kind = k;
						free(options.type);
						options.type = NULL;
						if (kinds[k].type)
							options.type = strdup(kinds[k].type);
						break;

					}
				}
				if (!kinds[k].kind) {
					options.kind = KindNone;
					free(options.type);
					options.type = NULL;
				}
			}
			break;

		case 'D':       /* -D, --debug [LEVEL] */
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
		case 'v':       /* -v, --verbose [LEVEL] */
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
		case 'h':       /* -h, --help */
		case 'H':       /* -H, --? */
			command = CommandHelp;
			break;
		case 'V':       /* -V, --version */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandVersion;
			options.command = CommandVersion;
			break;
		case 'C':       /* -C, --copying */
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
		if (command != CommandLaunch && command != CommandSet) {
			EPRINTF("%s: excess non-option arguments near '", argv[0]);
			while (optind < argc) {
				fprintf(stderr, "%s", argv[optind++]);
				fprintf(stderr, "%s", (optind < argc) ? " " : "");
			}
			fprintf(stderr, "'\n");
			usage(argc, argv);
			exit(EXIT_SYNTAXERR);
		}
	}
#endif
	get_defaults();
	// check_type(argc, argv);
	startup(argc, argv);
	switch (command) {
	default:
	case CommandDefault:
	case CommandLaunch:
		DPRINTF("%s: launching application for type\n", argv[0]);
		do_launch(argc, argv);
		break;
	case CommandList:
		DPRINTF("%s: listing all applications for type\n", argv[0]);
		do_list(argc, argv);
		break;
	case CommandSet:
		DPRINTF("%s: setting application for type\n", argv[0]);
		do_setapp(argc, argv);
		break;
	case CommandEdit:
		DPRINTF("%s: editing applications for types\n", argv[0]);
		do_editapps(argc, argv);
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

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

#ifdef HAVE_CONFIG_H
#include "autoconf.h"
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
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
	fprintf(stderr, "I: "); \
	fprintf(stderr, args); \
	fflush(stderr); } } while (0)
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

#define XA_SELECTION_NAME	"_XDE_MENUGEN_S%d"

static int saveArgc;
static char **saveArgv;

static Atom _XA_XDE_THEME_NAME;
static Atom _XA_GTK_READ_RCFILES;
static Atom _XA_MANAGER;

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
	StyleFullmenu,
	StyleAppmenu,
	StyleEntries,
} Style;

typedef struct {
	int debug;
	int output;
	Command command;
	char *format;
	Style style;
	char *charset;
	char *language;
	char *rootmenu;
	Bool dieonerr;
	char *filename;
	Bool noicons;
	char *theme;
	Bool launch;
	char *clientId;
	char *saveFile;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
	.command = CommandDefault,
	.format = NULL,
	.style = StyleFullmenu,
	.charset = NULL,
	.language = NULL,
	.rootmenu = NULL,
	.dieonerr = False,
	.filename = NULL,
	.noicons = False,
	.theme = NULL,
	.launch = True,
	.clientId = NULL,
	.saveFile = NULL,
};

Display *dpy;
Window root;

static Window
get_selection(Bool replace, Window selwin)
{
	char selection[64] = { 0, };
	int s, nscr;
	Atom atom;
	Window owner, gotone = None;

	for (s = 0, nscr = ScreenCount(dpy); s < nscr; s++) {
		snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);
		atom = XInternAtom(dpy, selection, False);
		if (!(owner = XGetSelectionOwner(dpy, atom)))
			DPRINTF("No owner for %s\n", selection);
		if ((owner && replace) || (!owner && selwin)) {
			DPRINTF("Setting owner of %s to 0x%08lx from 0x%08lx\n", selection, selwin,
				owner);
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

			for (s = 0; s < nscr; s++) {
				snprintf(selection, sizeof(selection), XA_SELECTION_NAME, s);

				ev.xclient.type = ClientMessage;
				ev.xclient.serial = 0;
				ev.xclient.send_event = False;
				ev.xclient.display = dpy;
				ev.xclient.window = RootWindow(dpy, s);
				ev.xclient.message_type = _XA_MANAGER;
				ev.xclient.format = 32;
				ev.xclient.data.l[0] = CurrentTime;
				ev.xclient.data.l[1] = XInternAtom(dpy, selection, False);
				ev.xclient.data.l[2] = selwin;
				ev.xclient.data.l[3] = 0;
				ev.xclient.data.l[4] = 0;

				XSendEvent(dpy, RootWindow(dpy, s), False, StructureNotifyMask,
					   &ev);
				XFlush(dpy);
			}
		}
	} else if (gotone)
		DPRINTF("%s: not replacing running instance\n", NAME);
	return (gotone);
}

static void
do_generate(int argc, char *argv[])
{
	Window owner;

	if ((owner = get_selection(False, None))) {
		EPRINTF("%s: an instance 0x%08lx is running\n", owner);
		exit(EXIT_FAILURE);
	}
	make_menu(argc, argv);
}

static void
do_run(int argc, char *argv[], Bool replace)
{
	Window selwin, owner;

	selwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	if ((owner = get_selection(replace, selwin))) {
		if (!replace) {
			XDestroyWindow(dpy, selwin);
			EPRINTF("%s: instance already running\n", NAME);
			exit(EXIT_FAILURE);
		}
	}
	XSelectInput(dpy, selwin,
		     StructureNotifyMask | SubstructureNotifyMask | PropertyChangeMask);
	make_menu(argc, argv);
	mainloop();
}

static void
do_quit(int argc, char *argv[])
{
	get_selection(True, None);
}

static void
startup(int argc, char *argv[])
{
	if (!(dpy = XOpenDisplay(NULL))) {
		EPRINTF("%s: could not open display %s\n", argv[0], getenv("DISPLAY"));
		exit(EXIT_FAILURE);
	}
	root DefaultRootWindow(dpy);

	_XA_XDE_THEME_NAME = XInternAtom(dpy, "_XDE_THEME_NAME", False);
	_XA_GTK_READ_RCFILES = XInternAtom(dpy, "_GTK_READ_RCFILE", False);
	_XA_MANAGER = XInternAtom(dpy, "MANAGER", False);
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
    %1$s {-m|--monitor} [options]\n\
    %1$s {-R|--replace} [options]\n\
    %1$s {-q|--quit} [options]\n\
    %1$s {-h|--help} [options]\n\
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
    %1$s {-m|--monitor} [options]\n\
    %1$s {-R|--replace} [options]\n\
    %1$s {-q|--quit} [options]\n\
    %1$s {-h|--help} [options]\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
Command options:\n\
    -m, --monitor\n\
        generate a menu and monitor for changes\n\
    -R, --replace\n\
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
    -f, --format FORMAT\n\
        specify the menu format [default: %2$s]\n\
    -F, --fullmenu, -N, --nofullmenu\n\
	full menu or applications menu [default: %3$s]\n\
    -d, --desktop DESKTOP\n\
	desktop environment [default: %4$s]\n\
    -c, --charset CHARSET\n\
	character set for menu [default: %5$s]\n\
    -l, --language LANGUAGE\n\
	language for menu [default: %6$s]\n\
    -r, --root-menu MENU\n\
	root menu file [default: %7$s]\n\
    -e, --die-on-error\n\
	abort on error [default: %8$s]\n\
    -o, --output [OUTPUT]\n\
	output file [default: %8$s]\n\
    -n, --noicons\n\
	do not place icons in menu [default: %9$s]\n\
    -t, --theme THEME\n\
	icon theme name to use [default: %10$s]\n\
    -L, --launch, --nolaunch\n\
	use xde-launch to launch programs [default: %11$s]\n\
    -s, --style STYLE\n\
	fullmenu, appmenu or entries [default: %124s]\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %13$d]\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %14$d]\n\
        this option may be repeated.\n\
", argv[0] 
	, options.format
	, show_style(options.style)
	, options.charset
	, options.language
	, options.rootmenu
	, show_bool(options.dieonerr)
	, options.filename
	, show_bool(options.noicons)
	, options.theme
	, show_bool(options.launch)
	, show_style(options.style)
	, options.debug
	, options.output
);
	/* *INDENT-ON* */
}

static void
set_defaults(void)
{
}

static void
get_defaults(void)
{
}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;

	setlocal(LC_ALL, "");

	set_defaults();

	saveArgc = argc;
	saveArgv = argv;

	while (1) {
		int c, val;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"format",	required_argument,	NULL,	'f'},
			{"fullmenu",	no_argument,		NULL,	'F'},
			{"nofullmenu",	no_arugment,		NULL,	'N'},
			{"desktop",	required_argument,	NULL,	'd'},
			{"charset",	required_argument,	NULL,	'c'},
			{"language",	required_argument,	NULL,	'l'},
			{"root-menu",	required_argument,	NULL,	'r'},
			{"die-on-error",no_argument,		NULL,	'e'},
			{"output",	optional_argument,	NULL,	'o'},
			{"noicons",	no_argument,		NULL,	'n'},
			{"theme",	required_argument,	NULL,	't'},
			{"monitor",	no_argument,		NULL,	'm'},
			{"launch",	no_argument,		NULL,	'L'},
			{"nolaunch",	no_argument,		NULL,	'0'},
			{"style",	required_argument,	NULL,	's'},

			{"quit",	no_argument,		NULL,	'q'},
			{"replace",	no_argument,		NULL,	'R'},

			{"debug",	optional_argument,	NULL,	'D'},
			{"verbose",	optional_argument,	NULL,	'v'},
			{"help",	no_argument,		NULL,	'h'},
			{"version",	no_argument,		NULL,	'V'},
			{"copying",	no_argument,		NULL,	'C'},
			{"?",		no_argument,		NULL,	'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "f:FNd:c:l:r:eo::nt:mL0s:D::v::hVCH?",
				     long_options, &option_index);
#else
		c = getopt(argc, argv, "f:FNd:c:l:r:eo:nt:mL0s:D:vhVC?");
#endif
		if (c == -1) {
			if (options.debug)
				fprintf(stderr, "%s: done options processing\n");
			break;
		}
		switch (c) {
		case 0:
			goto bad_usage;

		case 'f':	/* --format, -f FORMAT */
			free(options.format);
			options.format = strdup(optarg);
			break;
		case 'F':	/* -F, --fullmenu */
			options.style = StyleFullmenu;
			break;
		case 'N':	/* -N, --nofullmenu */
			options.style = StyleAppmenu;
			break;
		case 'd':	/* -d, --desktop DESKTOP */
			free(options.desktop);
			options.desktop = strdup(optarg);
			break;
		case 'c':	/* -c, --charset CHARSET */
			free(options.charset);
			options.charset = strdup(optarg);
			break;
		case 'l':	/* -l, --language LANGUAGE */
			free(options.language);
			options.language = strdup(optarg);
			break;
		case 'r':	/* -r, --root-menu MENU */
			free(options.rootmenu);
			options.rootmenu = strdup(optarg);
			break;
		case 'e':	/* -e, --die-on-error */
			options.dieonerr = True;
			break;
		case 'o':	/* -o, --output [OUTPUT] */
			options.fileout = True;
			if (optarg != NULL) {
				free(options.rootmenu);
				options.rootmenu = strdup(optarg);
			}
			break;
		case 'n':	/* -n, --noicons */
			options.noicons = True;
			break;
		case 't':	/* -t, --theme THEME */
			free(options.theme);
			options.theme = strdup(optarg);
			break;
		case 'm':	/* -m, --monitor */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandRun;
			options.command = CommandRun;
			break;
		case 'L':	/* -L, --launch */
			options.launch = True;
			break;
		case '0':	/* -0, --nolaunch */
			options.launch = False;
			break;
		case 's':	/* -s, --style STYLE */
			if (!strncmp("fullmenu", optarg, strlen(optarg))) {
				options.style = StyleFullmenu;
				break;
			}
			if (!strncmp("appmenu", optarg, strlen(optarg))) {
				options.style = StyleAppmenu;
				break;
			}
			if (!strncmp("entries", optarg, strlen(optarg))) {
				options.style = StyleEntries;
				break;
			}
			goto bad_option;

		case 'q':	/* -q, --quit */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandQuit;
			options.command = CommandQuit;
			break;
		case 'R':	/* -R, --replace */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandReplace;
			options.command = CommandReplace;
			break;

		case 'D':	/* -D, --debug [LEVEL] */
			if (options.debug)
				fprintf(stderr, "%s: increasing debug verbosity\n", argv[0]);
			if (optarg == NULL)
				options.debug++;
			else {
				if ((val = strtol(optarg, NULL, 0)) < 0)
					goto bad_option;
				options.debug = val;
			}
			break;
		case 'v':	/* -v, --verbose [LEVEL] */
			if (options.debug)
				fprintf(stderr, "%s: increasing output verbosity\n", argv[0]);
			if (optarg == NULL)
				options.output++;
			else {
				if ((val = strtol(optarg, NULL, 0)) < 0)
					goto bad_option;
				options.output = val;
			}
			break;
		case 'h':	/* -h, --help */
		case 'H':	/* -H, --? */
			command = CommandHelp;
			break;
		case 'V':
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
	case CommandDefault:
		if (options.debug)
			fprintf(stderr, "%s: running without monitoring\n", argv[0]);
		do_generate(argc, argv);
		break;
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
	exit(EXIT_SUCCESS);
}

// vim: tw=100 com=sr0\:/**,mb\:*,ex\:*/,sr0\:/*,mb\:*,ex\:*/,b\:TRANS formatoptions+=tcqlor

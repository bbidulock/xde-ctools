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
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

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


#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

typedef enum {
	COMMAND_DEFAULT = 0,
	COMMAND_HELP,
	COMMAND_VERSION,
	COMMAND_COPYING
} Commands;

Display *dpy;
unsigned int screen;
Window root;

Atom _XA_XDE_THEME_NAME;
Atom _XA_GTK_READ_RCFILES;

GtkListStore *store;
GtkWidget *dialog;
GtkWidget *combo;
GtkWidget *entry;
GtkWidget *term;
GtkWidget *icon;

int shutting_down;

typedef struct {
	Commands command;
	int debug;
	int output;
	int recent;
	char *runhist;
	char *recapps;
	int xdg;
} Options;

Options options = {
	.command = COMMAND_DEFAULT,
	.debug = 0,
	.output = 1,
	.recent = 10,
	.runhist = NULL,
	.recapps = NULL,
	.xdg = 0,
};

Options defaults = {
	.command = COMMAND_DEFAULT,
	.debug = 0,
	.output = 1,
	.recent = 10,
	.runhist = "~/.config/xde/run-history",
	.recapps = "~/.config/xde/recent-applications",
	.xdg = 0,
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
    -b, --binary\n\
        operate as a binary launcher [default: %8$s]\n\
    -x, --xdg\n\
        operate as an XDG launcher [default: %7$s]\n\
    -r, --recent NUMBER\n\
        only show and save NUMBER of most recent entries [default: %5$d]\n\
    -f, --file FILENAME\n\
        use alternate recent file [default: %9$s]\n\
    -l, --list FILENAME\n\
        use alternate run history file [default: %4$s]\n\
    -a, --apps FILENAME\n\
        use alternate recent application file [default: %6$s]\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %2$d]\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %3$d]\n\
        this option may be repeated.\n\
", argv[0], options.debug, options.output, options.runhist, options.recent, options.recapps, options.xdg ? "true" : "false", options.xdg ? "false" : "true", options.xdg ? options.recapps : options.runhist);
}

typedef struct {
	gchar *name;
	gchar *exec;
	time_t modified;
	guint count;
} RecentAppl;

typedef struct {
	gchar *href;
	time_t added;
	time_t modified;
	time_t visited;
	gchar *title;
	gchar *desc;
	gchar *owner;
	gchar *mime;
	GList *groups;
	GList *applications;
	gboolean private;
} RecentItem;

static void
xbel_start_element(GMarkupParseContext *ctx, const gchar *name, const gchar
		   **attrs, const gchar **values, gpointer user, GError **err)
{
	GList **list = user;
	GList *item = g_list_last(*list);
	RecentItem *mark = item ? item->data : NULL;

	if (!strcmp(name, "xbel")) {
	} else if (!strcmp(name, "bookmark")) {
		struct tm tm = { 0, };

		if (!(mark = calloc(1, sizeof(*mark)))) {
			EPRINTF("could not allocate bookmark\n");
			exit(1);
		}
		for (; *attrs; attrs++, values++) {
			if (!strcmp(*attrs, "href")) {
				free(mark->href);
				mark->href = strdup(*values);
			} else if (!strcmp(*attrs, "added")) {
				if (strptime(*values, "%FT%H:%M:%S%Z", &tm))
					mark->added = timegm(&tm);
			} else if (!strcmp(*attrs, "modified")) {
				if (strptime(*values, "%FT%H:%M:%S%Z", &tm))
					mark->modified = timegm(&tm);
			} else if (!strcmp(*attrs, "visited")) {
				if (strptime(*values, "%FT%H:%M:%S%Z", &tm))
					mark->visited = timegm(&tm);
			} else {
				DPRINTF("unrecognized attribute of bookmark: '%s'\n", *attrs);
			}
		}
		*list = g_list_append(*list, mark);
	} else if (!strcmp(name, "title")) {
	} else if (!strcmp(name, "desc")) {
	} else if (!strcmp(name, "info")) {
	} else if (!strcmp(name, "metadata")) {
		if (mark) {
			for (; *attrs; attrs++, values++) {
				if (!strcmp(*attrs, "owner")) {
					free(mark->owner);
					mark->owner = strdup(*values);
				}
			}
		}
	} else if (!strcmp(name, "mime:mime-type")) {
		if (mark) {
			for (; *attrs; attrs++, values++) {
				if (!strcmp(*attrs, "type")) {
					free(mark->mime);
					mark->mime = strdup(*values);
				} else {
					DPRINTF("unrecognized attribute of mime:mime-type: '%s'\n",
						*attrs);
				}
			}
		}
	} else if (!strcmp(name, "bookmark:groups")) {
	} else if (!strcmp(name, "bookmark:group")) {
	} else if (!strcmp(name, "bookmark:applications")) {
	} else if (!strcmp(name, "bookmark:application")) {
		if (mark) {
			RecentAppl *appl;
			struct tm tm = { 0, };

			if (!(appl = calloc(1, sizeof(*appl)))) {
				EPRINTF("could not allocate application\n");
				exit(1);
			}
			for (; *attrs; attrs++, values++) {
				if (!strcmp(*attrs, "name")) {
					free(appl->name);
					appl->name = strdup(*values);
				} else if (!strcmp(*attrs, "exec")) {
					free(appl->exec);
					appl->exec = strdup(*values);
				} else if (!strcmp(*attrs, "modified")) {
					if (strptime(*values, "%FT%H:%M:%S%Z", &tm))
						appl->modified = timegm(&tm);
				} else if (!strcmp(*attrs, "count")) {
					appl->count = strtoul(*values, NULL, 0);
				} else {
					DPRINTF
					    ("unrecognized attribute of bookmark:application: '%s'\n",
					     *attrs);
				}
			}
			mark->applications = g_list_append(mark->applications, appl);
		}
	} else if (!strcmp(name, "bookmark:private")) {
		if (mark)
			mark->private = TRUE;
	} else {
		DPRINTF("unrecognized start tag: '%s'\n", name);
	}
}

static void
xbel_text(GMarkupParseContext *ctx, const gchar *text, gsize len, gpointer user, GError **err)
{
	GList **list = user;
	GList *item = g_list_last(*list);
	RecentItem *mark = item ? item->data : NULL;
	const gchar *name;

	name = g_markup_parse_context_get_element(ctx);

	if (!strcmp(name, "xbel")) {
	} else if (!strcmp(name, "bookmark")) {
	} else if (!strcmp(name, "title")) {
		if (mark) {
			free(mark->title);
			mark->title = strndup(text, len);
		}
	} else if (!strcmp(name, "desc")) {
		if (mark) {
			free(mark->desc);
			mark->desc = strndup(text, len);
		}
	} else if (!strcmp(name, "info")) {
	} else if (!strcmp(name, "metadata")) {
	} else if (!strcmp(name, "mime:mime-type")) {
	} else if (!strcmp(name, "bookmark:groups")) {
	} else if (!strcmp(name, "bookmark:group")) {
		if (mark)
			mark->groups = g_list_append(mark->groups, strndup(text, len));
	} else if (!strcmp(name, "bookmark:applications")) {
	} else if (!strcmp(name, "bookmark:application")) {
	} else if (!strcmp(name, "bookmark:private")) {
	} else {
		DPRINTF("unrecognized cdata tag: '%s'\n", name);
	}
}

static void
text_free(gpointer data)
{
	free(data);
}

static void
appl_free(gpointer data)
{
	RecentAppl *appl = data;

	free(appl->name);
	appl->name = NULL;
	free(appl->exec);
	appl->exec = NULL;
	appl->modified = 0;
	appl->count = 0;
	free(appl);
}

static void
recent_free(gpointer data)
{
	RecentItem *book = data;

	free(book->href);
	book->href = NULL;
	book->added = 0;
	book->modified = 0;
	book->visited = 0;
	free(book->title);
	book->title = NULL;
	free(book->desc);
	book->desc = NULL;
	free(book->owner);
	book->owner = NULL;
	free(book->mime);
	book->mime = NULL;
	g_list_free_full(book->groups, text_free);
	g_list_free_full(book->applications, appl_free);
	free(book);
}

static gint
recent_sort(gconstpointer a, gconstpointer b)
{
	const RecentItem *A = a;
	const RecentItem *B = b;

	if (A->modified < B->modified)
		return (-1);
	if (A->modified > B->modified)
		return (1);
	return (0);
}

static gint
grp_match(gconstpointer data, gconstpointer user)
{
	return (strcmp(data, user));
}

static gint
app_match(gconstpointer data, gconstpointer user)
{
	const RecentAppl *a = data;

	return (strcmp(a->name, user));
}

static gint
appid_match(gconstpointer data, gconstpointer user)
{
	return (strcmp(data, user));
}

static void
recent_copy(gpointer data, gpointer user)
{
	RecentItem *b = data;
	GList **list = user;
	char *path, *name, *appid;
	int len;

	if (b->mime && strcmp(b->mime, "application/x-desktop"))
		return;
	if (!(name = path = b->href))
		return;
	path = strncmp(name, "file://", 7) ? name : name + 7;
	name = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
	if ((len = strlen(name) - strlen(".desktop")) <= 0)
		return;
	if (strcmp(name + len, ".desktop"))
		return;
	if (b->groups)
		if (!g_list_find_custom(b->groups, "recently-used-apps", grp_match))
			if (!g_list_find_custom(b->groups, "Application", grp_match))
				return;
	if (b->applications)
		if (!g_list_find_custom(b->applications, "XDG Launcher", app_match))
			if (!g_list_find_custom(b->applications, "xdg-launch", app_match))
				return;
	appid = strndup(name, len);
	if (g_list_find_custom(*list, appid, appid_match)) {
		free(appid);
		return;
	}
	*list = g_list_prepend(*list, appid);
	return;
}

void
get_recent_applications_xbel(GList **list, char *filename)
{
	GMarkupParseContext *ctx;
	gchar buf[BUFSIZ];
	gsize got;
	FILE *f;
	int dummy;
	char *file;

	GMarkupParser parser = {
		.start_element = xbel_start_element,
		.end_element = NULL,
		.text = xbel_text,
		.passthrough = NULL,
		.error = NULL,
	};

	if (!(file = g_build_filename(g_get_user_data_dir(), filename, NULL)))
		goto no_file;
	if (!(f = fopen(file, "r"))) {
		EPRINTF("cannot open file for reading: '%s'\n", file);
		goto no_file;
	}
	dummy = lockf(fileno(f), F_LOCK, 0);
	if (!(ctx = g_markup_parse_context_new(&parser, 0, list, NULL))) {
		EPRINTF("cannot create XML parser\n");
		goto unlock_done;
	}
	while ((got = fread(buf, 1, BUFSIZ, f)) > 0) {
		if (!g_markup_parse_context_parse(ctx, buf, got, NULL)) {
			EPRINTF("could not parse buffer contents\n");
			goto no_parse;
		}
	}
	if (!g_markup_parse_context_end_parse(ctx, NULL)) {
		EPRINTF("could not end parsing\n");
		goto no_parse;
	}
      no_parse:
	g_markup_parse_context_unref(ctx);
      unlock_done:
	dummy = lockf(fileno(f), F_ULOCK, 0);
	fclose(f);
      no_file:
	(void) dummy;
}

static void
recent_start_element(GMarkupParseContext *ctx, const gchar *name, const gchar **attrs,
		     const gchar **values, gpointer user, GError **err)
{
	GList **list = user;
	GList *item = g_list_last(*list);
	RecentItem *mark = item ? item->data : NULL;

	if (!strcmp(name, "RecentItem")) {
		if (!(mark = calloc(1, sizeof(*mark)))) {
			EPRINTF("could not allocate element\n");
			exit(1);
		}
		*list = g_list_append(*list, mark);
	} else if (!strcmp(name, "Private")) {
		mark->private = TRUE;
	}
}

static void
recent_text(GMarkupParseContext *ctx, const gchar *text, gsize len, gpointer user, GError **err)
{
	GList **list = user;
	GList *item = g_list_last(*list);
	RecentItem *mark = item ? item->data : NULL;

	const gchar *name;
	char *buf, *end = NULL;
	unsigned long int val;

	name = g_markup_parse_context_get_element(ctx);

	if (!strcmp(name, "URI")) {
		free(mark->href);
		mark->href = strndup(text, len);
	} else if (!strcmp(name, "Mime-Type")) {
		free(mark->mime);
		mark->mime = strndup(text, len);
	} else if (!strcmp(name, "Timestamp")) {
		mark->modified = 0;
		buf = strndup(text, len);
		val = strtoul(buf, &end, 0);
		if (end && *end == '\0')
			mark->modified = val;
		free(buf);
	} else if (!strcmp(name, "Group")) {
		buf = strndup(text, len);
		mark->groups = g_list_append(mark->groups, buf);
	}
}

gint
history_sort(gconstpointer a, gconstpointer b)
{
	const char *A = a;
	const char *B = b;
	gint result;

	DPRINTF("comparing %s and %s:\n", A, B);
	if ((result = strcmp(A, B)) == 0)
		DPRINTF("matched %s and %s in list!\n", A, B);

	return (strcmp(A, B));
}

void
get_recent_applications(GList **list, char *filename)
{
	GMarkupParseContext *ctx;
	gchar buf[BUFSIZ];
	gsize got;
	FILE *f;
	int dummy;
	char *file;

	GMarkupParser parser = {
		.start_element = recent_start_element,
		.end_element = NULL,
		.text = recent_text,
		.passthrough = NULL,
		.error = NULL,
	};

	if (!(file = g_build_filename(g_get_home_dir(), filename, NULL)))
		goto no_file;
	if (!(f = fopen(file, "r"))) {
		EPRINTF("cannot open file for reading: '%s'\n", file);
		goto no_file;
	}
	dummy = lockf(fileno(f), F_LOCK, 0);
	if (!(ctx = g_markup_parse_context_new(&parser, 0, list, NULL))) {
		EPRINTF("cannot create XML parser\n");
		goto unlock_done;
	}
	while ((got = fread(buf, 1, BUFSIZ, f)) > 0) {
		if (!g_markup_parse_context_parse(ctx, buf, got, NULL)) {
			EPRINTF("could not parse buffer contents\n");
			goto no_parse;
		}
	}
	if (!g_markup_parse_context_end_parse(ctx, NULL)) {
		EPRINTF("could not end parsing\n");
		goto no_parse;
	}
      no_parse:
	g_markup_parse_context_unref(ctx);
      unlock_done:
	dummy = lockf(fileno(f), F_ULOCK, 0);
	fclose(f);
      no_file:
	(void) dummy;
}

void
history_free(gpointer data)
{
	free((void *) data);
}

void
get_run_history(GList **hist)
{
	char *p, *file;
	FILE *f;
	int n;

	file = options.xdg ? options.recapps : options.runhist;
	if ((f = fopen(file, "r"))) {
		char *buf = calloc(PATH_MAX + 2, sizeof(*buf));
		int discarding = 0;
		int dummy;

		dummy = lockf(fileno(f), F_LOCK, 0);

		n = 0;
		while (fgets(buf, PATH_MAX, f)) {
			if ((p = strrchr(buf, '\n'))) {
				if (!discarding)
					*p = '\0';
				else {
					discarding = 0;
					continue;
				}
			} else {
				discarding = 1;
				continue;
			}
			p = buf + strspn(buf, " \t");
			if (!*p)
				continue;

			if (g_list_find_custom(*hist, p, history_sort))
				continue;
			*hist = g_list_append(*hist, strdup(p));
			if (++n >= options.recent)
				break;
		}

		dummy = lockf(fileno(f), F_ULOCK, 0);
		free(buf);
		fclose(f);
		(void) dummy;
	} else
		EPRINTF("open: could not open history file %s: %s\n", file, strerror(errno));
	return;
}

void
get_history(GList **hist)
{
	if (options.xdg) {
		GList *recent = NULL;

		get_recent_applications_xbel(&recent, "recently-used.xbel");
		get_recent_applications_xbel(&recent, "recent-applications.xbel");
		get_recent_applications(&recent, ".recently-used");
		get_recent_applications(&recent, ".recent-applications");
		recent = g_list_sort(recent, recent_sort);
		g_list_foreach(recent, recent_copy, hist);
		g_list_free_full(recent, recent_free);
	}
	if (!*hist)
		get_run_history(hist);
}

void
history_write(gpointer data, gpointer user)
{
	fprintf((FILE *) user, "%s\n", (char *) data);
}

void
put_run_history(GList **hist)
{
	char *file;
	FILE *f;

	file = options.xdg ? options.recapps : options.runhist;
	if ((f = fopen(file, "w"))) {
		int dummy;

		dummy = lockf(fileno(f), F_LOCK, 0);
		g_list_foreach(*hist, history_write, f);
		fflush(f);
		dummy = lockf(fileno(f), F_ULOCK, 0);
		fclose(f);
		(void) dummy;
	}
}

void
put_history(GList **hist)
{
	if (*hist)
		put_run_history(hist);
	g_list_free_full(*hist, history_free);
	*hist = NULL;
}

void
find_apps(const char *base, const char *subdir)
{
	char *path;
	struct dirent *d;
	DIR *dir;
	int len;

	len = strlen(base) + strlen(subdir) + 1;
	path = calloc(len, sizeof(*path));
	strncpy(path, base, len);
	strncat(path, subdir, len);

	if ((dir = opendir(path))) {
		char *entry = calloc(PATH_MAX + 1, sizeof(*entry));

		while ((d = readdir(dir))) {
			struct stat st;

			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) {
				DPRINTF("%s: skipping directory '%s'\n", __FUNCTION__, entry);
				continue;
			}
			strncpy(entry, path, PATH_MAX);
			strncat(entry, d->d_name, PATH_MAX);
			if (stat(entry, &st) == -1) {
				DPRINTF("stat: %s: %s\n", entry, strerror(errno));
				continue;
			}
			if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) {
				DPRINTF("stat: %s: %s\n", entry, "not regular file or directory");
				continue;
			}
			if (S_ISREG(st.st_mode)) {
				char *p;
				char *appid;
				GtkTreeIter iter;

				if (!(p = strstr(d->d_name, ".desktop")) || p[8]) {
					DPRINTF("%s: not a desktop file '%s'\n", __FUNCTION__,
						entry);
					continue;
				}
				len = strlen(subdir) + strlen(d->d_name) + 1;
				appid = calloc(len, sizeof(*appid));
				strncpy(appid, subdir, len);
				strncat(appid, d->d_name, len);
				if ((p = strstr(appid, ".desktop")) && !p[8])
					*p = '\0';

				gtk_list_store_append(store, &iter);
				gtk_list_store_set(store, &iter, 0, appid, -1);

			} else if (S_ISDIR(st.st_mode)) {
				strncpy(entry, subdir, PATH_MAX);
				strncat(entry, d->d_name, PATH_MAX);
				strncat(entry, "/", PATH_MAX);
				find_apps(base, entry);
			}
		}
		closedir(dir);
		free(entry);
	}
	free(path);
}

void
find_bins(const char *base, const char *subdir)
{
	char *path;
	struct dirent *d;
	DIR *dir;
	int len;

	len = strlen(base) + strlen(subdir) + 1;
	path = calloc(len, sizeof(*path));
	strncpy(path, base, len);
	strncat(path, subdir, len);

	if ((dir = opendir(path))) {
		char *entry = calloc(PATH_MAX + 1, sizeof(*entry));

		while ((d = readdir(dir))) {
			struct stat st;

			if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..")) {
				DPRINTF("%s: skipping directory '%s'\n", __FUNCTION__, entry);
				continue;
			}
			strncpy(entry, path, PATH_MAX);
			strncat(entry, d->d_name, PATH_MAX);
			if (stat(entry, &st) == -1) {
				DPRINTF("stat: %s: %s\n", entry, strerror(errno));
				continue;
			}
			if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) {
				DPRINTF("stat: %s: %s\n", entry, "not regular file or directory");
				continue;
			}
			if (S_ISREG(st.st_mode)) {
				char *binid;
				GtkTreeIter iter;

				if (!(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
					DPRINTF("%s: not executable '%s'\n", __FUNCTION__, entry);
					continue;
				}
				len = strlen(subdir) + strlen(d->d_name) + 1;
				binid = calloc(len, sizeof(*binid));
				strncpy(binid, subdir, len);
				strncat(binid, d->d_name, len);

				gtk_list_store_append(store, &iter);
				gtk_list_store_set(store, &iter, 0, binid, -1);

			} else if (S_ISDIR(st.st_mode)) {
				strncpy(entry, subdir, PATH_MAX);
				strncat(entry, d->d_name, PATH_MAX);
				strncat(entry, "/", PATH_MAX);
				find_bins(base, entry);
			}
		}
	}
}

void
create_store()
{
	store = gtk_list_store_new(1, G_TYPE_STRING);
	char *dirs, *save, *path, *dir;

	path = calloc(PATH_MAX + 1, sizeof(*path));

	if (options.xdg) {
		char *data, *home;
		int len;

		if (!(home = getenv("XDG_DATA_HOME"))) {
			snprintf(path, PATH_MAX, "%s/.local/share", getenv("HOME"));
			home = path;
		}
		if (!(data = getenv("XDG_DATA_DIRS"))) {
			data = "/usr/local/share:/usr/share";
		}
		len = strlen(home) + 1 + strlen(data) + 1;
		save = dirs = calloc(len, sizeof(*dirs));
		snprintf(dirs, len, "%s:%s", home, data);
	} else {
		save = dirs = strdup(getenv("PATH") ? : "");
	}

	while ((dir = strsep(&dirs, ":"))) {
		if (options.xdg) {
			strncpy(path, dir, PATH_MAX);
			strncat(path, "/applications/", PATH_MAX);
			find_apps(path, "");
		} else {
			find_bins(dir, "");
		}
	}
	free(save);
	free(path);
}

void
on_entry_changed(GtkTextBuffer *textbuffer, gpointer data)
{
	char *p;
	const char *command = gtk_entry_get_text(GTK_ENTRY(entry));

	if (command && *command) {
		char *copy = strdup(command);

		if ((p = strpbrk(copy, " \t")))
			*p = '\0';
		p = (p = strrchr(copy, '/')) ? p + 1 : copy;
		gtk_image_set_from_icon_name(GTK_IMAGE(icon), p, GTK_ICON_SIZE_DIALOG);
		free(copy);
	}
}

void
on_file_response(GtkDialog *dialog, gint response_id, gpointer data)
{
	if (response_id == GTK_RESPONSE_OK || response_id == 0) {
		char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
		int len = strlen(text) + 2 + strlen(file) + 1 + 1;
		char *cmd = calloc(len, sizeof(*cmd));

		strcpy(cmd, text);
		strcat(cmd, " '");
		strcat(cmd, file);
		strcat(cmd, "'");
		gtk_entry_set_text(GTK_ENTRY(entry), cmd);
		free(cmd);
		g_free(file);
	}
}

void
on_file_clicked(GtkButton *button, gpointer data)
{
	GtkWidget *choose = gtk_file_chooser_dialog_new("Choose File", GTK_WINDOW(dialog),
							GTK_FILE_CHOOSER_ACTION_OPEN,
							GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	g_signal_connect(G_OBJECT(choose), "response", G_CALLBACK(on_file_response), NULL);
	gtk_dialog_run(GTK_DIALOG(choose));
}

void
on_dialog_response(GtkDialog *dialog, gint response_id, gpointer data)
{
	GList **hist = data, *item;

	if (response_id == GTK_RESPONSE_OK || response_id == 0) {
		char *command;
		const char *text;
		int len, status;

		text = gtk_entry_get_text(GTK_ENTRY(entry));
		if (options.xdg) {
			static const char *launch = "xdg-launch --keyboard ";

			len = strlen(launch) + strlen(text) + 3;
			command = calloc(len, sizeof(*command));
			strncpy(command, launch, len);
			strncat(command, text, len);
			if ((item = g_list_find_custom(*hist, text, history_sort))) {
				*hist = g_list_remove_link(*hist, item);
				*hist = g_list_concat(item, *hist);
			} else
				*hist = g_list_prepend(*hist, strdup(text));
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(term))) {
			static const char *xterm = "xterm -e ";

			len = strlen(xterm) + strlen(text) + 3;
			command = calloc(len, sizeof(*command));
			strcpy(command, xterm);
			strcat(command, text);
			if ((item = g_list_find_custom(*hist, command, history_sort))) {
				*hist = g_list_remove_link(*hist, item);
				*hist = g_list_concat(item, *hist);
			} else
				*hist = g_list_prepend(*hist, strdup(command));
		} else {
			len = strlen(text) + 3;
			command = calloc(len, sizeof(*command));
			strcpy(command, text);
			if ((item = g_list_find_custom(*hist, command, history_sort))) {
				*hist = g_list_remove_link(*hist, item);
				*hist = g_list_concat(item, *hist);
			} else
				*hist = g_list_prepend(*hist, strdup(command));
		}
		put_history(hist);

		strncat(command, " &", len);
		if ((status = system(command)) == 0)
			exit(0);
		if (WIFSIGNALED(status)) {
			EPRINTF("system: %s: exited on signal %d\n", command, WTERMSIG(status));
			exit(WTERMSIG(status));
		} else if (WIFEXITED(status)) {
			EPRINTF("system: %s: exited with non-zero exit status %d\n", command,
				WEXITSTATUS(status));
			exit(WEXITSTATUS(status));
		}
		exit(EXIT_FAILURE);
	}
	gtk_main_quit();
}

void
on_entry_activate(GtkEntry *entry, gpointer data)
{
	gtk_signal_emit_by_name(GTK_OBJECT(dialog), "response", GTK_RESPONSE_OK, data);
}

void
run_command(GList **hist)
{
	icon = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_DIALOG);
	GtkWidget *align = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);

	gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(icon));

	combo = gtk_combo_new();
	entry = GTK_COMBO(combo)->entry;
	GtkWidget *list = GTK_COMBO(combo)->list;

	gtk_combo_disable_activate(GTK_COMBO(combo));
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), *hist);
	gtk_combo_set_use_arrows(GTK_COMBO(combo), TRUE);
	gtk_list_select_item(GTK_LIST(list), 0);

	GtkEntryCompletion *compl = gtk_entry_completion_new();

	gtk_entry_completion_set_model(GTK_ENTRY_COMPLETION(compl), GTK_TREE_MODEL(store));
	gtk_entry_completion_set_text_column(GTK_ENTRY_COMPLETION(compl), 0);
	gtk_entry_completion_set_minimum_key_length(GTK_ENTRY_COMPLETION(compl), 2);
	gtk_entry_set_completion(GTK_ENTRY(entry), GTK_ENTRY_COMPLETION(compl));
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(on_entry_changed), NULL);

	term = gtk_check_button_new_with_label("Run in terminal");
	gtk_widget_set_sensitive(GTK_WIDGET(term), options.xdg ? FALSE : TRUE);

	GtkWidget *file = gtk_button_new_with_label("Run with file...");

	g_signal_connect(G_OBJECT(file), "clicked", G_CALLBACK(on_file_clicked), NULL);

	GtkWidget *hbox = gtk_hbox_new(FALSE, 12);

	gtk_box_set_spacing(GTK_BOX(hbox), 12);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(term), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(file), FALSE, FALSE, 0);

	GtkWidget *vbox = gtk_vbox_new(FALSE, 6);

	gtk_box_set_spacing(GTK_BOX(vbox), 6);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(combo), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), TRUE, TRUE, 0);

	GtkWidget *mbox = gtk_hbox_new(FALSE, 6);

	gtk_container_set_border_width(GTK_CONTAINER(mbox), 5);
	gtk_box_set_spacing(GTK_BOX(mbox), 6);
	gtk_box_pack_start(GTK_BOX(mbox), GTK_WIDGET(align), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mbox), GTK_WIDGET(vbox), TRUE, TRUE, 0);

	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog), "Run Command");
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 5);
	gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(dialog), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog), TRUE);
	gint storage;

	if ((storage = gtk_image_get_storage_type(GTK_IMAGE(icon))) == GTK_IMAGE_PIXBUF
	    || storage == GTK_IMAGE_EMPTY)
		gtk_window_set_icon(GTK_WINDOW(dialog), gtk_image_get_pixbuf(GTK_IMAGE(icon)));
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			       GTK_STOCK_EXECUTE, GTK_RESPONSE_OK, NULL);
	GtkWidget *action = gtk_dialog_get_action_area(GTK_DIALOG(dialog));

	gtk_button_box_set_layout(GTK_BUTTON_BOX(action), GTK_BUTTONBOX_END);
	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(mbox), TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(on_dialog_response), hist);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate), hist);
	gtk_widget_show_all(GTK_WIDGET(dialog));
}

void
reparse(Display *dpy, Window root)
{
	XTextProperty xtp = { NULL, };
	char **list = NULL;
	int strings = 0;

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
		}
		if (xtp.value)
			XFree(xtp.value);
	}
}

static GdkFilterReturn
event_handler_PropertyNotify(Display *dpy, XEvent *xev)
{
	if (options.debug > 1) {
		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n",
			(xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
	}
	if (xev->xproperty.atom == _XA_XDE_THEME_NAME && xev->xproperty.state == PropertyNewValue) {
		reparse(dpy, xev->xproperty.window);
		return GDK_FILTER_REMOVE;	/* event handled */
	}
	return GDK_FILTER_CONTINUE;	/* event not handled */
}

static GdkFilterReturn
event_handler_ClientMessage(Display *dpy, XEvent *xev)
{
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
	Display *dpy = data;

	return handle_event(dpy, xev);
}

void
startup(int argc, char *argv[])
{
	static const char *suffix = "/.gtkrc-2.0.xde";
	GdkAtom atom;
	GdkEventMask mask;
	GdkDisplay *disp;
	GdkScreen *scrn;
	GdkWindow *root;
	Display *dpy;
	char *file;

	file = g_build_filename(g_get_home_dir(), suffix, NULL);
	gtk_rc_add_default_file(file);
	// free(file); /* just have to leak this? */

	gtk_init(&argc, &argv);

	disp = gdk_display_get_default();
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

	reparse(dpy, GDK_WINDOW_XID(root));
}

void
set_defaults()
{
	static const char *rsuffix = "/xde/run-history";
	static const char *asuffix = "/xde/recent-applications";
	int len;
	char *env;

	if ((env = getenv("XDG_CONFIG_HOME"))) {
		len = strlen(env) + strlen(rsuffix) + 1;
		free(options.runhist);
		options.runhist = calloc(len, sizeof(*options.runhist));
		strcpy(options.runhist, env);
		strcat(options.runhist, rsuffix);

		len = strlen(env) + strlen(asuffix) + 1;
		free(options.recapps);
		options.recapps = calloc(len, sizeof(*options.recapps));
		strcpy(options.recapps, env);
		strcat(options.recapps, asuffix);
	} else {
		static const char *cfgdir = "/.config";

		env = getenv("HOME") ? : ".";

		len = strlen(env) + strlen(cfgdir) + strlen(rsuffix) + 1;
		free(options.runhist);
		options.runhist = calloc(len, sizeof(*options.runhist));
		strcpy(options.runhist, env);
		strcat(options.runhist, cfgdir);
		strcat(options.runhist, rsuffix);

		len = strlen(env) + strlen(cfgdir) + strlen(asuffix) + 1;
		free(options.recapps);
		options.recapps = calloc(len, sizeof(*options.recapps));
		strcpy(options.recapps, env);
		strcat(options.recapps, cfgdir);
		strcat(options.recapps, asuffix);
	}
	return;
}

int
main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	set_defaults();

	while (1) {
		int c, val;

#ifdef _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"binary",	    no_argument,	NULL, 'b'},
			{"xdg",		    no_argument,	NULL, 'x'},
			{"recent",	    required_argument,	NULL, 'r'},
			{"file",	    required_argument,	NULL, 'f'},
			{"list",	    required_argument,	NULL, 'l'},
			{"apps",	    required_argument,	NULL, 'a'},
			{"debug",	    optional_argument,	NULL, 'D'},
			{"verbose",	    optional_argument,	NULL, 'v'},
			{"help",	    no_argument,	NULL, 'h'},
			{"version",	    no_argument,	NULL, 'V'},
			{"copying",	    no_argument,	NULL, 'C'},
			{"?",		    no_argument,	NULL, 'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "bxf:l:a:D::v::hVCH?", long_options,
				     &option_index);
#else
		c = getopt(argc, argv, "bxf:l:a:DvhVCH?");
#endif
		if (c == -1) {
			if (options.debug)
				fprintf(stderr, "%s: done options processing\n", argv[0]);
			break;
		}
		switch (c) {
		case 0:
			goto bad_usage;

		case 'b':	/* -b, --binary */
			options.xdg = 0;
			break;
		case 'x':	/* -x, --xdg */
			options.xdg = 1;
			break;
		case 'r':	/* -r, --recent NUMBER */
			if ((val = strtoul(optarg, NULL, 0)) < 0)
				goto bad_option;
			if (val < 1)
				val = 1;
			if (val > 100)
				val = 100;
			options.recent = val;
			break;
		case 'f':	/* -f, --file FILENAME */
			if (options.xdg) {
				free(options.recapps);
				options.recapps = strdup(optarg);
			} else {
				free(options.runhist);
				options.runhist = strdup(optarg);
			}
			break;
		case 'l':	/* -l, --list RUNHIST_FILE */
			free(options.runhist);
			options.runhist = strdup(optarg);
			break;
		case 'a':	/* -a, --apps RECAPPS_FILE */
			free(options.recapps);
			options.recapps = strdup(optarg);
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
			if (options.command != COMMAND_DEFAULT)
				goto bad_option;
			options.command = COMMAND_HELP;
			break;
		case 'V':	/* -V, --version */
			if (options.command != COMMAND_DEFAULT)
				goto bad_option;
			options.command = COMMAND_VERSION;
			break;
		case 'C':	/* -C, --copying */
			if (options.command != COMMAND_DEFAULT)
				goto bad_option;
			options.command = COMMAND_COPYING;
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
	switch (options.command) {
	case COMMAND_DEFAULT:
	default:
	{
		GList *history = NULL;

		if (options.debug)
			fprintf(stderr, "%s: running command\n", argv[0]);
		startup(argc, argv);
		create_store();
		get_history(&history);
		run_command(&history);
		gtk_main();
		break;
	}
	case COMMAND_HELP:
		if (options.debug)
			fprintf(stderr, "%s: printing help message\n", argv[0]);
		help(argc, argv);
		break;
	case COMMAND_VERSION:
		if (options.debug)
			fprintf(stderr, "%s: printing version message\n", argv[0]);
		version(argc, argv);
		break;
	case COMMAND_COPYING:
		if (options.debug)
			fprintf(stderr, "%s: printing copying message\n", argv[0]);
		copying(argc, argv);
		break;
	}
	exit(0);
}

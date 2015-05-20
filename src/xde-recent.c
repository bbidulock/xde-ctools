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

typedef struct {
	Commands command;
	int debug;
	int output;
	int recent;
	char *runhist;
	char *recapps;
	char *recently;
	int xdg;
} Options;

Options options = {
	.command = COMMAND_DEFAULT,
	.debug = 0,
	.output = 1,
	.recent = 10,
	.runhist = NULL,
	.recapps = NULL,
	.recently = NULL,
	.xdg = 1,
};

Options defaults = {
	.command = COMMAND_DEFAULT,
	.debug = 0,
	.output = 1,
	.recent = 10,
	.runhist = "~/.config/xde/run-history",
	.recapps = "~/.config/xde/recent-applications",
	.recently = "~/.local/share/recently-used",
	.xdg = 1,
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

GtkRecentManager *manager = NULL;
GList *items = NULL;

void
items_free(gpointer data)
{
	gtk_recent_info_unref((GtkRecentInfo *) data);
}

void
items_print(gpointer data, gpointer user_data)
{
	FILE *f = (typeof(f)) user_data;
	GtkRecentInfo *i = (typeof(i)) data;
	gchar **apps = NULL;
	gsize length = 0, n;
	gchar *name, *path;

	fprintf(f, "----------------------------------\n");
	if ((name = gtk_recent_info_get_short_name(i))) {
		fprintf(f, "Name:\t\t%s\n", name);
		g_free(name);
	}
	if ((path = gtk_recent_info_get_uri_display(i))) {
		fprintf(f, "Path:\t\t%s\n", path);
		g_free(path);
	}
	fprintf(f, "Local:\t\t%s\n", gtk_recent_info_is_local(i) ? "yes" : "no");
	fprintf(f, "Exists:\t\t%s\n", gtk_recent_info_exists(i) ? "yes" : "no");
	fprintf(f, "Age:\t\t%d\n", gtk_recent_info_get_age(i));
	fprintf(f, "URI:\t\t%s\n", gtk_recent_info_get_uri(i));
	fprintf(f, "DisplayName:\t%s\n", gtk_recent_info_get_display_name(i));
	fprintf(f, "Description:\t%s\n", gtk_recent_info_get_description(i));
	fprintf(f, "MimeType:\t%s\n", gtk_recent_info_get_mime_type(i));
	fprintf(f, "Added:\t\t%lu\n", gtk_recent_info_get_added(i));
	fprintf(f, "Modified:\t%lu\n", gtk_recent_info_get_modified(i));
	fprintf(f, "Visited:\t%lu\n", gtk_recent_info_get_visited(i));
	fprintf(f, "Private:\t%s\n", gtk_recent_info_get_private_hint(i) ? "yes" : "no");
	if ((apps = gtk_recent_info_get_applications(i, &length))) {
		fprintf(f, "Applications:\t\n");
		for (n = 0; n < length; n++) {
			const gchar *app_exec = NULL;
			guint count;
			time_t time_;

			fprintf(f, "\t\t%s\n", apps[n]);
			if (gtk_recent_info_get_application_info
			    (i, apps[n], &app_exec, &count, &time_)) {
				fprintf(f, "\t\tName:\t\t%s\n", apps[n]);
				fprintf(f, "\t\tExec:\t\t%s\n", app_exec);
				fprintf(f, "\t\tCount:\t\t%u\n", count);
				fprintf(f, "\t\tTime:\t\t%lu\n", time_);
			}
		}
		g_strfreev(apps);
		apps = NULL;
	}
	if ((apps = gtk_recent_info_get_groups(i, &length))) {
		fprintf(f, "Groups:\t\t\n");
		for (n = 0; n < length; n++) {
			fprintf(f, "\t\t%s\n", apps[n]);
		}
		g_strfreev(apps);
		apps = NULL;
	}
}

GList *recent = NULL;

typedef struct {
	gchar *uri;
	gchar *mime;
	time_t stamp;
	gboolean private;
	GSList *groups;
} RecentItem;

RecentItem *current = NULL;

static void
xml_start_element(GMarkupParseContext *context, const gchar *element_name,
		  const gchar **attribute_names, const gchar **attribute_values, gpointer user_data,
		  GError **error)
{
	if (!strcmp(element_name, "RecentFiles")) {
		/* don't care */
	} else if (!strcmp(element_name, "RecentItem")) {
		if (!current && !(current = calloc(1, sizeof(*current)))) {
			EPRINTF("could not allocate element\n");
			exit(1);
		}
	} else if (!strcmp(element_name, "URI")) {
		/* don't care */
	} else if (!strcmp(element_name, "Mime-Type")) {
		/* don't care */
	} else if (!strcmp(element_name, "Timestamp")) {
		/* don't care */
	} else if (!strcmp(element_name, "Private")) {
		current->private = TRUE;
	} else if (!strcmp(element_name, "Groups")) {
		/* don't care */
	} else if (!strcmp(element_name, "Group")) {
		/* don't care */
	} else {
		DPRINTF("bad element name '%s'\n", element_name);
		return;
	}
}

static void
xml_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data,
		GError **error)
{
	if (!strcmp(element_name, "RecentFiles")) {
		/* don't care */
	} else if (!strcmp(element_name, "RecentItem")) {
		recent = g_list_append(recent, (gpointer) current);
		current = NULL;
	} else if (!strcmp(element_name, "URI")) {
		/* don't care */
	} else if (!strcmp(element_name, "Mime-Type")) {
		/* don't care */
	} else if (!strcmp(element_name, "Timestamp")) {
		/* don't care */
	} else if (!strcmp(element_name, "Private")) {
		/* don't care */
	} else if (!strcmp(element_name, "Groups")) {
		/* don't care */
	} else if (!strcmp(element_name, "Group")) {
		/* don't care */
	} else {
		DPRINTF("bad element name '%s'\n", element_name);
		return;
	}
}

static void
xml_character_data(GMarkupParseContext *context, const gchar *text, gsize text_len,
		   gpointer user_data, GError **error)
{
	const gchar *element_name;

	element_name = g_markup_parse_context_get_element(context);
	if (!strcmp(element_name, "RecentFiles")) {
		/* don't care */
	} else if (!strcmp(element_name, "RecentItem")) {
		/* don't care */
	} else if (!strcmp(element_name, "URI")) {
		free(current->uri);
		current->uri = calloc(1, text_len + 1);
		memcpy(current->uri, text, text_len);
	} else if (!strcmp(element_name, "Mime-Type")) {
		free(current->mime);
		current->mime = calloc(1, text_len + 1);
		memcpy(current->mime, text, text_len);
	} else if (!strcmp(element_name, "Timestamp")) {
		char *buf, *end = NULL;
		unsigned long int val;

		current->stamp = 0;
		buf = calloc(1, text_len + 1);
		memcpy(buf, text, text_len);
		val = strtoul(buf, &end, 0);
		if (end && *end == '\0')
			current->stamp = val;
	} else if (!strcmp(element_name, "Private")) {
		/* don't care */
	} else if (!strcmp(element_name, "Groups")) {
		/* don't care */
	} else if (!strcmp(element_name, "Group")) {
		char *buf;

		buf = calloc(1, text_len + 1);
		memcpy(buf, text, text_len);
		current->groups = g_slist_append(current->groups, (gpointer) buf);
	} else {
		DPRINTF("bad element name '%s'\n", element_name);
		return;
	}
}

static void
xml_passthrough(GMarkupParseContext *context, const gchar *passthrough_text, gsize text_len,
		gpointer user_data, GError **error)
{
	/* don't care */
}

static void
xml_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
	EPRINTF("got an error during parsing\n");
	exit(1);
}

GMarkupParser xml_parser = {
	.start_element = xml_start_element,
	.end_element = xml_end_element,
	.text = xml_character_data,
	.passthrough = xml_passthrough,
	.error = xml_error,
};

void
group_free(gpointer data)
{
	free((char *) data);
}

void
recent_free(gpointer data)
{
	RecentItem *r = (typeof(r)) data;

	free(r->uri);
	free(r->mime);
	g_slist_free_full(r->groups, group_free);
	free(r);
}

static void
group_print(gpointer data, gpointer user_data)
{
	FILE *f = (typeof(f)) user_data;
	gchar *s = (typeof(s)) data;

	fprintf(f, "\t\t%s\n", s);
}

static void
recent_print(gpointer data, gpointer user_data)
{
	FILE *f = (typeof(f)) user_data;
	RecentItem *r = (typeof(r)) data;
	char *path, *name;

	fprintf(f, "----------------------------------\n");
	if ((name = path = r->uri)) {
		path = strncmp(name, "file://", 7) ? name : name + 7;
		name = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
	}

	if (name && name != r->uri)
		fprintf(f, "Name:\t\t%s\n", name);
	if (path && path != r->uri) {
		fprintf(f, "Path:\t\t%s\n", path);
		fprintf(f, "Local:\t\t%s\n", "yes");
		fprintf(f, "Exists:\t\t%s\n", access(path, F_OK) ? "no" : "yes");
	} else {
		fprintf(f, "Local:\t\t%s\n", "no");
		fprintf(f, "Exists:\t\t%s\n", "no");
	}
	fprintf(f, "URI:\t\t%s\n", r->uri);
	/* FIXME: pull this from the .desktop file if specified or discoverable */
	fprintf(f, "DisplayName:\t%s\n", "(null)");
	/* FIXME: pull this from the .desktop file if specified or discoverable */
	fprintf(f, "Description:\t%s\n", "(null)");
	fprintf(f, "MimeType:\t%s\n", r->mime);
	fprintf(f, "Added:\t\t%lu\n", r->stamp);
	fprintf(f, "Modified:\t%lu\n", r->stamp);
	fprintf(f, "Visited:\t%lu\n", r->stamp);
	fprintf(f, "Private:\t%s\n", r->private ? "yes" : "no");
#if 0
	/* FIXME: pull this out from the .desktop file if recently-used-apps or from
	   equivalent desktop file if group is also an APPID */
	fprintf(f, "Applications:\t\n");
#endif
	if (r->groups) {
		fprintf(f, "Groups:\t\t\n");
		g_slist_foreach(r->groups, group_print, user_data);
	}
}

static void
run_command2(int argc, char *argv[])
{
	GMarkupParseContext *context;
	GError *error = NULL;
	gchar buffer[BUFSIZ];
	gsize got;
	FILE *file;
	int dummy;

	g_list_free_full(recent, recent_free);

	if (!(file = fopen(options.recently, "r"))) {
		EPRINTF("cannot open file for reading: %s\n", options.recently);
		exit(1);
	}
	dummy = lockf(fileno(file), F_LOCK, 0);
	if (!(context = g_markup_parse_context_new(&xml_parser,
						   G_MARKUP_TREAT_CDATA_AS_TEXT |
						   G_MARKUP_PREFIX_ERROR_POSITION |
						   G_MARKUP_IGNORE_QUALIFIED, NULL, NULL))) {
		EPRINTF("cannot create XML parser\n");
		exit(1);
	}
	while ((got = fread(buffer, 1, BUFSIZ, file)) > 0) {
		if (!g_markup_parse_context_parse(context, buffer, got, &error)) {
			EPRINTF("could not parse buffer contents\n");
			exit(1);
		}
	}
	if (!g_markup_parse_context_end_parse(context, &error)) {
		EPRINTF("could not end parsing\n");
		exit(1);
	}
	g_markup_parse_context_unref(context);
	dummy = lockf(fileno(file), F_ULOCK, 0);
	fclose(file);
	g_list_foreach(recent, recent_print, (gpointer) stdout);
	g_list_free_full(recent, recent_free);
	recent = NULL;
	(void) dummy;
}

static void
run_command(int argc, char *argv[])
{
	if (!manager && !(manager = gtk_recent_manager_get_default())) {
		EPRINTF("could not get recent manager instance\n");
		exit(1);
	}
	if (!(items = gtk_recent_manager_get_items(manager))) {
		EPRINTF("could not get recent items list\n");
		exit(1);
	}
	g_list_foreach(items, items_print, (gpointer) stdout);
	g_list_free_full(items, items_free);
	items = NULL;
}

void
set_defaults(void)
{
	static const char *xsuffix = "/recently-used";
	static const char *hsuffix = "/.recently-used";
	int len;
	char *env;

	free(options.recently);
	options.recently = NULL;

	if ((env = getenv("XDG_DATA_HOME"))) {
		len = strlen(env) + strlen(xsuffix) + 1;
		free(options.recently);
		options.recently = calloc(len, sizeof(*options.recently));
		strcpy(options.recently, env);
		strcpy(options.recently, xsuffix);
	} else {
		static const char *subdir = "/.local/share";

		env = getenv("HOME") ? : ".";

		len = strlen(env) + strlen(subdir) + strlen(xsuffix) + 1;
		free(options.recently);
		options.recently = calloc(len, sizeof(*options.recently));
		strcpy(options.recently, env);
		strcat(options.recently, subdir);
		strcat(options.recently, xsuffix);
	}
	if (access(options.recently, R_OK | W_OK)) {
		env = getenv("HOME") ? : ".";

		len = strlen(env) + strlen(hsuffix) + 1;
		free(options.recently);
		options.recently = calloc(len, sizeof(*options.recently));
		strcpy(options.recently, env);
		strcat(options.recently, hsuffix);
	}
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
			{"debug",	    optional_argument,	NULL,	'D'},
			{"verbose",	    optional_argument,	NULL,	'v'},
			{"help",	    no_argument,	NULL,	'h'},
			{"version",	    no_argument,	NULL,	'V'},
			{"copying",	    no_argument,	NULL,	'C'},
			{"?",		    no_argument,	NULL,	'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "D::v::hVCH?", long_options, &option_index);
#else				/* _GNU_SOURCE */
		c = getopt(argc, argv, "DvhVCH?");
#endif				/* _GNU_SOURCE */
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
		if (options.debug)
			fprintf(stderr, "%s: running command\n", argv[0]);
		run_command2(argc, argv);
		run_command(argc, argv);
		break;
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

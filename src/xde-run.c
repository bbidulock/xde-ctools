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

#include "xde-run.h"

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

GList *history;
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
	.xdg = 0,
};

Options defaults = {
	.command = COMMAND_DEFAULT,
	.debug = 0,
	.output = 1,
	.recent = 10,
	.runhist = "~/.config/xde/run-history",
	.recapps = "~/.config/xde/recent-applications",
	.recently = "~/.local/share/recently-used",
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
ru_xml_start_element(GMarkupParseContext *ctx, const gchar *name, const gchar **attrs,
		  const gchar **values, gpointer user, GError **err)
{
	if (!strcmp(name, "RecentItem")) {
		if (!current && !(current = calloc(1, sizeof(*current)))) {
			EPRINTF("could not allocate element\n");
			exit(1);
		}
	} else if (!strcmp(name, "Private")) {
		current->private = TRUE;
	}
}

static void
ru_xml_end_element(GMarkupParseContext *ctx, const gchar *name, gpointer user, GError **err)
{
	if (!strcmp(name, "RecentItem")) {
		recent = g_list_append(recent, (gpointer) current);
		current = NULL;
	}
}

static void
ru_xml_text(GMarkupParseContext *ctx, const gchar *text, gsize len, gpointer user, GError **err)
{
	const gchar *name;
	char *buf, *end = NULL;
	unsigned long int val;

	name = g_markup_parse_context_get_element(ctx);
	if (!strcmp(name, "URI")) {
		free(current->uri);
		current->uri = calloc(1, len + 1);
		memcpy(current->uri, text, len);
	} else if (!strcmp(name, "Mime-Type")) {
		free(current->mime);
		current->mime = calloc(1, len + 1);
		memcpy(current->mime, text, len);
	} else if (!strcmp(name, "Timestamp")) {
		current->stamp = 0;
		buf = calloc(1, len + 1);
		memcpy(buf, text, len);
		val = strtoul(buf, &end, 0);
		if (end && *end == '\0')
			current->stamp = val;
	} else if (!strcmp(name, "Group")) {
		buf = calloc(1, len + 1);
		memcpy(buf, text, len);
		current->groups = g_slist_append(current->groups, (gpointer) buf);
	}
}

static void
ru_xml_passthrough(GMarkupParseContext *ctx, const gchar *text, gsize len, gpointer user,
		   GError **err)
{
	/* don't care */
}

static void
ru_xml_error(GMarkupParseContext *ctx, GError *err, gpointer user)
{
	EPRINTF("got an error during parsing\n");
	exit(1);
}

GMarkupParser ru_xml_parser = {
	.start_element = ru_xml_start_element,
	.end_element = ru_xml_end_element,
	.text = ru_xml_text,
	.passthrough = ru_xml_passthrough,
	.error = ru_xml_error,
};

static void
group_free(gpointer data)
{
	free((char *) data);
}

static void
recent_free(gpointer data)
{
	RecentItem *r = (typeof(r)) data;

	free(r->uri);
	free(r->mime);
	g_slist_free_full(r->groups, group_free);
	free(r);
}

gint
history_sort(gconstpointer a, gconstpointer b)
{
	char *A = (typeof(A)) a;
	char *B = (typeof(B)) b;
	gint result;

	DPRINTF("comparing %s and %s:\n", A, B);
	if ((result = strcmp(A, B)) == 0)
		DPRINTF("matched %s and %s in list!\n", A, B);

	return (strcmp(A, B));
}

static void
recent_copy(gpointer data, gpointer user)
{
	RecentItem *r = (typeof(r)) data;
	unsigned int i, numb;
	char *path, *name;
	int len;

	if (!(name = path = r->uri))
		return;
	path = strncmp(name, "file://", 7) ? name : name + 7;
	name = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
	len = strlen(name) - strlen(".desktop");
	if (strstr(name, ".desktop") != name + len)
		return;
	numb = g_slist_length(r->groups);
	for (i = 0; i < numb; i++) {
		char *grp;

		if ((grp = (char *) g_slist_nth_data(r->groups, i)) &&
		    !strcmp(grp, "recently-used-apps")) {
			char *appid = strndup(name, len);

			if (g_list_find_custom(history, appid, history_sort)) {
				DPRINTF("found %s in list!\n", appid);
				free(appid);
				return;
			}
			history = g_list_append(history, appid);
			return;
		}
	}
}

gint
recent_sort(gconstpointer a, gconstpointer b)
{
	RecentItem *A = (typeof(A)) a;
	RecentItem *B = (typeof(B)) b;

	if (A->stamp < B->stamp)
		return (-1);
	if (A->stamp > B->stamp)
		return (1);
	return (0);
}

void
get_recently_used()
{
	GMarkupParseContext *ctx;
	GError *err = NULL;
	gchar buf[BUFSIZ];
	gsize got;
	FILE *f;
	int dummy;

	g_list_free_full(recent, recent_free);

	if (!(f = fopen(options.recently, "r"))) {
		EPRINTF("cannot open file for reading: '%s'\n", options.recently);
		return;
	}
	dummy = lockf(fileno(f), F_LOCK, 0);
	if (!(ctx = g_markup_parse_context_new(&ru_xml_parser,
					       G_MARKUP_TREAT_CDATA_AS_TEXT |
					       G_MARKUP_PREFIX_ERROR_POSITION |
					       G_MARKUP_IGNORE_QUALIFIED, NULL, NULL))) {
		EPRINTF("cannot create XML parser\n");
		return;
	}
	while ((got = fread(buf, 1, BUFSIZ, f)) > 0) {
		if (!g_markup_parse_context_parse(ctx, buf, got, &err)) {
			EPRINTF("could not parse buffer contents\n");
			return;
		}
	}
	if (!g_markup_parse_context_end_parse(ctx, &err)) {
		EPRINTF("could not end parsing\n");
		return;
	}
	g_markup_parse_context_unref(ctx);
	dummy = lockf(fileno(f), F_ULOCK, 0);
	fclose(f);
	recent = g_list_sort(recent, recent_sort);
	g_list_foreach(recent, recent_copy, (gpointer) NULL);
	g_list_free_full(recent, recent_free);
	recent = NULL;
	(void) dummy;
}

void
history_free(gpointer data)
{
	free((void *) data);
}

void
get_run_history()
{
	char *p, *file;
	FILE *f;
	int n;

	g_list_free_full(history, history_free);

	file = options.xdg ? options.recapps : options.runhist;
	if ((f = fopen(file, "r"))) {
		char *buf = calloc(PATH_MAX + 2, sizeof(*buf));
		int discarding = 0;

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

			if (g_list_find_custom(history, p, history_sort))
				continue;
			history = g_list_append(history, (gpointer) strdup(p));
			if (++n >= options.recent)
				break;
		}
		free(buf);
		fclose(f);
	} else
		EPRINTF("open: could not open history file %s: %s\n", file, strerror(errno));
	if (options.xdg)
		get_recently_used();
	return;
}

void
history_write(gpointer data, gpointer user)
{
	FILE *f = (typeof(f)) user;
	char *t = (typeof(t)) data;

	fprintf(f, "%s\n", t);
}

void
put_run_history()
{
	char *file;
	FILE *f;

	file = options.xdg ? options.recapps : options.runhist;
	if ((f = fopen(file, "w"))) {
		g_list_foreach(history, history_write, (gpointer) f);
		fclose(f);
	}
	g_list_free_full(history, history_free);
	history = NULL;
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

	g_signal_connect(G_OBJECT(choose), "response", G_CALLBACK(on_file_response),
			 (gpointer) NULL);
	gtk_dialog_run(GTK_DIALOG(choose));
}

void
on_dialog_response(GtkDialog *dialog, gint response_id, gpointer data)
{
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
			if (!g_list_find_custom(history, text, history_sort))
				history = g_list_append(history, (gpointer) strdup(text));
			else
				DPRINTF("found %s in list!\n", text);
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(term))) {
			static const char *xterm = "xterm -e ";

			len = strlen(xterm) + strlen(text) + 3;
			command = calloc(len, sizeof(*command));
			strcpy(command, xterm);
			strcat(command, text);
			if (!g_list_find_custom(history, command, history_sort))
				history = g_list_append(history, (gpointer) strdup(command));
			else
				DPRINTF("found %s in list!\n", command);
		} else {
			len = strlen(text) + 3;
			command = calloc(len, sizeof(*command));
			strcpy(command, text);
			if (!g_list_find_custom(history, command, history_sort))
				history = g_list_append(history, (gpointer) strdup(command));
			else
				DPRINTF("found %s in list!\n", command);
		}
		put_run_history();

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
run_command()
{
	icon = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_DIALOG);
	GtkWidget *align = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);

	gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(icon));

	combo = gtk_combo_new();
	entry = GTK_COMBO(combo)->entry;
	GtkWidget *list = GTK_COMBO(combo)->list;

	gtk_combo_disable_activate(GTK_COMBO(combo));
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), history);
	gtk_combo_set_use_arrows(GTK_COMBO(combo), TRUE);
	gtk_list_select_item(GTK_LIST(list), 0);

	GtkEntryCompletion *compl = gtk_entry_completion_new();

	gtk_entry_completion_set_model(GTK_ENTRY_COMPLETION(compl), GTK_TREE_MODEL(store));
	gtk_entry_completion_set_text_column(GTK_ENTRY_COMPLETION(compl), 0);
	gtk_entry_completion_set_minimum_key_length(GTK_ENTRY_COMPLETION(compl), 2);
	gtk_entry_set_completion(GTK_ENTRY(entry), GTK_ENTRY_COMPLETION(compl));
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(on_entry_changed), (gpointer) NULL);

	term = gtk_check_button_new_with_label("Run in terminal");
	gtk_widget_set_sensitive(GTK_WIDGET(term), options.xdg ? FALSE : TRUE);

	GtkWidget *file = gtk_button_new_with_label("Run with file...");

	g_signal_connect(G_OBJECT(file), "clicked", G_CALLBACK(on_file_clicked), (gpointer) NULL);

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
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(on_dialog_response),
			 (gpointer) NULL);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate),
			 (gpointer) NULL);
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
	int len;

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
	static const char *xsuffix = "/recently-used";
	static const char *hsuffix = "/.recently-used";
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

		len = strlen(env) + strlen(xsuffix) + 1;
		free(options.recently);
		options.recently = calloc(len, sizeof(*options.recently));
		strcpy(options.recently, env);
		strcat(options.recently, xsuffix);
	} else {
		static const char *cfgdir = "/.config";
		static const char *datdir = "/.local/share";

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

		len = strlen(env) + strlen(datdir) + strlen(xsuffix) + 1;
		free(options.recently);
		options.recently = calloc(len, sizeof(*options.recently));
		strcpy(options.recently, env);
		strcat(options.recently, datdir);
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
		if (options.debug)
			fprintf(stderr, "%s: running command\n", argv[0]);
		startup(argc, argv);
		create_store();
		get_run_history();
		run_command();
		gtk_main();
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

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

#include "xde-run.h"

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

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
	int debug;
	int output;
} Options;

Options options = {
	.debug = 0,
	.output = 1,
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
history_free(gpointer data)
{
	free((void *) data);
}

void
get_run_history()
{
	static const char *rhfile = "/xde/run-history";
	const char *s;
	char *file, *p;
	FILE *f;

	g_list_free_full(history, history_free);

	if ((s = getenv("XDG_CONFIG_HOME"))) {
		int len;

		len = strlen(s) + strlen(rhfile) + 1;
		file = calloc(len, sizeof(*file));
		strncpy(file, s, len);
		strncat(file, rhfile, len);
	} else {
		static const char *config = "/.config";
		const char *h;
		int len;

		h = getenv("HOME");
		len = strlen(h) + strlen(config) + strlen(rhfile) + 1;
		file = calloc(len, sizeof(*file));
		strncpy(file, h, len);
		strncat(file, config, len);
		strncat(file, rhfile, len);
	}
	if ((f = fopen(file, "r"))) {
		char *buf = calloc(PATH_MAX + 2, sizeof(*buf));
		int discarding = 0;

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
			history = g_list_prepend(history, (gpointer) strdup(buf));
		}
		free(buf);
		fclose(f);
	} else
		fprintf(stderr, "open: could not open history file %s: %s\n", file, strerror(errno));
	free(file);
	return;
}

void
create_store()
{
	store = gtk_list_store_new(1, G_TYPE_STRING);
	char *path = strdup(getenv("PATH") ? : ""), *p = path - 1, *e = path + strlen(path), *b;
	struct dirent *d;
	DIR *dir;
	char *entry;
	GtkTreeIter iter;

	entry = calloc(PATH_MAX + 2, sizeof(*entry));
	while ((b = p + 1) < e) {
		*(p = strchrnul(b, ':')) = '\0';
		if ((dir = opendir(b))) {
			while ((d = readdir(dir))) {
				struct stat st;

				if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
					continue;
				strncpy(entry, b, PATH_MAX);
				strncat(entry, "/", PATH_MAX);
				strncat(entry, d->d_name, PATH_MAX);
				if (stat(entry, &st) == -1) {
					if (options.debug)
						fprintf(stderr, "stat: %s: %s\n", entry, strerror(errno));
					continue;
				}
				if (!S_ISREG(st.st_mode)) {
					if (options.debug)
						fprintf(stderr, "stat: %s: %s\n", entry, "not a regular file");
					continue;
				}
				if (!(st.st_mode & S_IXOTH)) {
					if (options.debug)
						fprintf(stderr, "stat: %s: %s\n", entry, "not executable");
					continue;
				}
				gtk_list_store_append(store, &iter);
				gtk_list_store_set(store, &iter, 0, entry, -1);
			}
			/* FIXME */
			closedir(dir);
		}
	}
	free(entry);
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
		gtk_image_set_from_icon_name(GTK_IMAGE(icon), p, GTK_ICON_SIZE_LARGE_TOOLBAR);
		free(copy);
	}
}

void
on_file_clicked(GtkButton *button, gpointer data)
{
}

void
on_dialog_response(GtkDialog *dialog, gint response_id, gpointer data)
{
	if (response_id == GTK_RESPONSE_OK || response_id == 0) {
		const char *command = gtk_entry_get_text(GTK_ENTRY(entry));

		history = g_list_prepend(history, (gpointer) strdup(command));
	}
}

void
on_entry_activate(GtkEntry *entry, gpointer data)
{
	gtk_signal_emit_by_name(GTK_OBJECT(dialog), "response", GTK_RESPONSE_OK, data);
}

void
run_command()
{
	icon = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_LARGE_TOOLBAR);
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
	gtk_window_set_icon(GTK_WINDOW(dialog), gtk_image_get_pixbuf(GTK_IMAGE(icon)));
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_EXECUTE, GTK_RESPONSE_OK, NULL);
	GtkWidget *action = gtk_dialog_get_action_area(GTK_DIALOG(dialog));

	gtk_button_box_set_layout(GTK_BUTTON_BOX(action), GTK_BUTTONBOX_END);
	GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

	gtk_box_pack_start(GTK_BOX(content), GTK_WIDGET(mbox), TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(on_dialog_response), (gpointer) NULL);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_entry_activate), (gpointer) NULL);
	gtk_widget_show_all(GTK_WIDGET(dialog));
}

void
reparse()
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

void
event_handler_PropertyNotify(XEvent *xev)
{
	if (options.debug) {
		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n", (xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
	}
	if (xev->xproperty.atom == _XA_XDE_THEME_NAME && xev->xproperty.state == PropertyNewValue)
		reparse();
	return;
}

void
event_handler_ClientMessage(XEvent *xev)
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
	if (xev->xclient.message_type == _XA_GTK_READ_RCFILES)
		reparse();
	return;
}

void
handle_event(XEvent *xev)
{
	switch (xev->type) {
	case PropertyNotify:
		event_handler_PropertyNotify(xev);
		break;
	case ClientMessage:
		event_handler_ClientMessage(xev);
		break;
	}
}

void
handle_events()
{
	XEvent xev;

	XSync(dpy, False);
	while (XPending(dpy) && !shutting_down) {
		XNextEvent(dpy, &xev);
		handle_event(&xev);
	}
}

gboolean
on_watch(GIOChannel *chan, GIOCondition cond, gpointer data)
{
	if (cond & (G_IO_NVAL | G_IO_HUP | G_IO_ERR)) {
		fprintf(stderr, "ERROR: poll failed: %s %s %s\n",
			(cond & G_IO_NVAL) ? "NVAL" : "", (cond & G_IO_HUP) ? "HUP" : "", (cond & G_IO_ERR) ? "ERR" : "");
		exit(EXIT_FAILURE);
	} else if (cond & (G_IO_IN | G_IO_PRI)) {
		handle_events();
	}
	return TRUE;		/* keep event source */
}

void
on_int_signal(int signum)
{
	shutting_down = True;
	gtk_main_quit();
}

void
on_hup_signal(int signum)
{
	shutting_down = True;
	gtk_main_quit();
}

void
on_term_signal(int signum)
{
	shutting_down = True;
	gtk_main_quit();
}

void
on_quit_signal(int signum)
{
	shutting_down = True;
	gtk_main_quit();
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
		fprintf(stderr, "X error %s(0x%08lx): %s\n", req, xev->resourceid, msg);
	}
	return (0);
}

void
startup(int argc, char *argv[])
{
	static const char *suffix = "/.gtkrc-2.0.xde";
	int xfd;
	GIOChannel *chan;
	gint srce;
	const char *home;
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

	signal(SIGINT, on_int_signal);
	signal(SIGHUP, on_hup_signal);
	signal(SIGTERM, on_term_signal);
	signal(SIGQUIT, on_quit_signal);

	if (!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "%s: %s %s\n", argv[0], "cannot open display", getenv("DISPLAY") ? : "");
		exit(127);
	}
	xfd = ConnectionNumber(dpy);
	chan = g_io_channel_unix_new(xfd);
	srce = g_io_add_watch(chan, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_PRI, on_watch, (gpointer) 0);
	(void) srce;

	XSetErrorHandler(handler);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	_XA_XDE_THEME_NAME = XInternAtom(dpy, "_XDE_THEME_NAME", False);
	_XA_GTK_READ_RCFILES = XInternAtom(dpy, "_GTK_READ_RCFILES", False);

	XSelectInput(dpy, root, PropertyChangeMask | StructureNotifyMask | SubstructureNotifyMask);

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
			{"debug",	    optional_argument,	NULL, 'D'},
			{"verbose",	    optional_argument,	NULL, 'v'},
			{"help",	    no_argument,	NULL, 'h'},
			{"version",	    no_argument,	NULL, 'V'},
			{"copying",	    no_argument,	NULL, 'C'},
			{"?",		    no_argument,	NULL, 'H'},
			{ 0, }
		};
		/* *INDENT-ON* */

		c = getopt_long_only(argc, argv, "D::v::hVCH?", long_options, &option_index);
#else
		c = getop(argc, argv, "DvhVC?");
#endif
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
			if (options.debug)
				fprintf(stderr, "%s: printing help message\n", argv[0]);
			help(argc, argv);
			exit(0);
		case 'V':	/* -V, --version */
			if (options.debug)
				fprintf(stderr, "%s: printing version message\n", argv[0]);
			version(argc, argv);
			exit(0);
		case 'C':	/* -C, --copying */
			if (options.debug)
				fprintf(stderr, "%s: printing copying message\n", argv[0]);
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
	if (optind >= argc) {
		fprintf(stderr, "%s: missing non-option argument\n", argv[0]);
		usage(argc, argv);
		exit(2);
	}
	startup(argc, argv);
	create_store();
	get_run_history();
	run_command();
	exit(0);
}

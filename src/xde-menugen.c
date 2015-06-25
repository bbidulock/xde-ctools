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
#include <langinfo.h>
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
#define WPRINTF(args...) do { \
	fprintf(stderr, "W: %s +%d %s(): ", __FILE__, __LINE__, __func__); \
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

static Atom _XA_XDE_WM_NAME;
static Atom _XA_XDE_WM_MENU;
static Atom _XA_XDE_WM_THEME;
static Atom _XA_XDE_WM_ICONTHEME;
static Atom _XA_XDE_THEME_NAME;
static Atom _XA_XDE_ICON_THEME_NAME;
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
	char *desktop;
	char *charset;
	char *language;
	char *locale;
	char *rootmenu;
	Bool dieonerr;
	Bool fileout;
	char *filename;
	Bool noicons;
	char *theme;
	Bool launch;
	char *clientId;
	char *saveFile;
	char *runhist;
	char *recapps;
	char *recently;
	char *recent;
	char *keep;
	char *menu;
} Options;

Options options = { 0, 1, };

Options defaults = {
	.debug = 0,
	.output = 1,
	.command = CommandDefault,
	.format = NULL,
	.style = StyleFullmenu,
	.desktop = "XDE",
	.charset = "UTF-8",
	.language = NULL,
	.locale = "en_US.UTF-8",
	.rootmenu = NULL,
	.dieonerr = False,
	.fileout = False,
	.filename = NULL,
	.noicons = False,
	.theme = NULL,
	.launch = True,
	.clientId = NULL,
	.saveFile = NULL,
	.runhist = "~/.config/xde/run-history",
	.recapps = "~/.config/xde/recent-applications",
	.recently = "~/.local/share/recently-used",
	.recent = NULL,
	.keep = "10",
	.menu = "applications",
};

typedef struct {
	int index;
	GdkDisplay *disp;
	GdkScreen *scrn;
	GdkWindow *root;
	WnckScreen *wnck;
	char *theme;
	char *itheme;
	Window selwin;
	Atom atom;
	char *wmname;
	Bool goodwm;
} XdeScreen;

static XdeScreen *screens;

char *xdg_data_home = NULL;
char *xdg_data_dirs = NULL;
char *xdg_data_path = NULL;
char *xdg_data_last = NULL;

static inline char *
xdg_find_str(char *s, char *b)
{
	for (s--; s > b && *s != '\0'; s--) ;
	if (s > b)
		s++;
	return (s);
}

char *xdg_config_home = NULL;
char *xdg_config_dirs = NULL;
char *xdg_config_path = NULL;
char *xdg_config_last = NULL;

typedef struct {
	char *label;
	GMarkupParser *parser;
} XdgParserMapping;

typedef struct {
	char *directory;		/* directory of menu file. */
	char *filename;			/* filename portion only. */
	GMarkupParseContext *ctx;	/* glib parser context */
	XdgParserMapping *current;	/* current element */
	GQueue *elements;		/* element stack */
	GNode *node;			/* current menu */
	GQueue *stack;			/* menu stack */
	GNode *tree;			/* menu tree */
	GNode *rule;			/* curent rule */
	GQueue *rules;			/* rule stack */
} XdgParseContext;

static void
xdg_context_free(gpointer data)
{
	if (data == NULL)
		return;
	XdgParseContext *base = data;

	free(base->directory);
	free(base->filename);
	g_queue_free(base->elements);
	base->elements = NULL;
	base->current = NULL;
	g_queue_free(base->stack);
	base->tree = base->node = NULL;
	g_queue_free(base->rules);
	base->rule = NULL;
	if (base->ctx) {
		g_markup_parse_context_unref(base->ctx);
		base->ctx = NULL;
	}
	free(base);
}

typedef struct {
	GNode *tree;			/* final parse tree */
	XdgParseContext *parser;	/* current parser */
	GQueue *parsers;		/* nested parsers */
} XdgContext;

enum itemType {
	MenuItem = 0,
	MenuClosure = 1,
	MenuEntry = 2,
};

typedef struct {
	enum itemType type;
	char *name;
} XdgMenuItem;

typedef struct {
	char *path;
	struct stat st;
} XdgDirectory;

GHashTable *xdg_directory_cache = NULL;	/* directory cache */

static void
xdg_directory_free(gpointer data)
{
	XdgDirectory *dir = data;

	free(dir->path);
	dir->path = NULL;
	free(dir);
}

enum mergeType {
	MergeTypeFilePath = 0,
	MergeTypeFileParent = 1,
	MergeTypeDirectory = 2,
	MergeTypeLegacy = 3,
};

typedef struct {
	enum mergeType type;
	char *prefix;
	char *path;
	struct stat st;
} XdgMerge;

static void
xdg_merge_free(gpointer data)
{
	XdgMerge *merge = data;

	free(merge->prefix);
	merge->prefix = NULL;
	free(merge->path);
	merge->path = NULL;
	free(merge);
}

typedef struct {
	char *path;
	char *id;
	struct stat st;
} XdgFileEntry;

GHashTable *xdg_fileentry_cache = NULL;	/* fileentry cache */

static void
xdg_fileentry_free(gpointer data)
{
	XdgFileEntry *file = data;

	free(file->path);
	file->path = NULL;
	free(file);
}

enum logicType {
	LogicTypeRoot = 0,
	LogicTypeInclude = 1,
	LogicTypeExclude = 2,
	LogicTypeAnd = 3,
	LogicTypeOr = 4,
	LogicTypeNot = 5,
	LogicTypeFilename = 6,
	LogicTypeCategory = 7,
	LogicTypeAll = 8,
};

char *
xdg_logic_type(enum logicType type)
{
	switch (type) {
	case LogicTypeRoot:
		return ("LogicTypeRoot");
	case LogicTypeInclude:
		return ("LogicTypeInclude");
	case LogicTypeExclude:
		return ("LogicTypeExclude");
	case LogicTypeAnd:
		return ("LogicTypeAnd");
	case LogicTypeOr:
		return ("LogicTypeOr");
	case LogicTypeNot:
		return ("LogicTypeNot");
	case LogicTypeFilename:
		return ("LogicTypeFilename");
	case LogicTypeCategory:
		return ("LogicTypeCategory");
	case LogicTypeAll:
		return ("LogicTypeAll");
	}
	return ("(unknown)");
}

typedef struct {
	enum logicType type;		/* type of rule */
	char *string;			/* string value associated with the rule
					   (Filename and Category only) */
} XdgRule;

static void
xdg_rule_free(gpointer data)
{
	XdgRule *rule = data;

	free(rule->string);
	rule->string = NULL;
	free(rule);
}

typedef struct {
	char *old;
	char *new;
} XdgMove;

static void
xdg_move_free(gpointer data)
{
	XdgMove *move = data;

	free(move->old);
	move->old = NULL;
	free(move->new);
	move->new = NULL;
	free(move);
}

enum layoutType {
	LayoutTypeDefault = 0,
	LayoutTypeFilename = 1,
	LayoutTypeMenuname = 2,
	LayoutTypeSeparator = 3,
	LayoutTypeMergeAll = 4,
	LayoutTypeMergeFiles = 5,
	LayoutTypeMergeMenus = 6,
};

typedef struct {
	enum layoutType type;
	char *string;
	gboolean show_empty;
	gboolean menu_inline;
	guint inline_limit;
	gboolean inline_header;
	gboolean inline_alias;
} XdgLayout;

static void
xdg_layout_free(gpointer data)
{
	XdgLayout *layout = data;

	free(layout->string);
	layout->string = NULL;
	free(layout);
}

typedef struct {
	char *name;
	GList *appdirs;			/* applications directories */
//      GHashTable *apps;               /* applications by id */
	GList *dirdirs;			/* directories directories */
//      GHashTable *dirs;               /* directories by id */
	GList *merge;			/* merge directories */
	char *directory;		/* directory for this menu item */
//      XdgFileEntry *dentry;           /* .directory file for this menu */
	gboolean only_u;		/* only unallocated? */
	gboolean deleted;		/* is menu manually deleted? */
	GList *rules;			/* rules (include/exclude) for this menu */
	GList *moves;			/* moves for this menu */
	GList *layout;			/* layout for the menu */
	GList *deflay;			/* default layout for the menu and sub-menus */
} XdgMenu;

static void
xdg_menu_free(gpointer data)
{
	XdgMenu *menu = data;

	free(menu->name);
	menu->name = NULL;
	g_list_free_full(menu->appdirs, xdg_directory_free);
	menu->appdirs = NULL;
	g_list_free_full(menu->dirdirs, xdg_directory_free);
	menu->dirdirs = NULL;
	g_list_free_full(menu->merge, xdg_merge_free);
	menu->merge = NULL;
	free(menu->directory);
	menu->directory = NULL;
	g_list_free_full(menu->rules, xdg_rule_free);
	menu->rules = NULL;
	g_list_free_full(menu->moves, xdg_move_free);
	menu->moves = NULL;
	g_list_free_full(menu->layout, xdg_layout_free);
	menu->layout = NULL;
	g_list_free_full(menu->deflay, xdg_layout_free);
	menu->deflay = NULL;
	free(menu);
}

void
xdg_menu_merge(XdgMenu *target, XdgMenu *menu)
{
	target->appdirs = g_list_concat(target->appdirs, menu->appdirs);
	menu->appdirs = NULL;
	target->dirdirs = g_list_concat(target->dirdirs, menu->dirdirs);
	menu->dirdirs = NULL;
	target->merge = g_list_concat(target->merge, menu->merge);
	menu->merge = NULL;
	if (menu->directory) {
		free(target->directory);
		target->directory = menu->directory;
		menu->directory = NULL;
	}
	target->only_u = menu->only_u;
	target->deleted = menu->deleted;
	target->rules = g_list_concat(target->rules, menu->rules);
	menu->rules = NULL;
	target->moves = g_list_concat(target->moves, menu->moves);
	menu->moves = NULL;
	if (menu->layout) {
		g_list_free_full(target->layout, xdg_layout_free);
		target->layout = menu->layout;
		menu->layout = NULL;
	}
	if (menu->deflay) {
		g_list_free_full(target->deflay, xdg_layout_free);
		target->deflay = menu->deflay;
		menu->deflay = NULL;
	}
	xdg_menu_free(menu);
}

static GNode *parse_menu(XdgContext *context, const char *path, GError **error);

/*
 * <Menu>
 *	The root element is <Menu>.  Each <Menu> element may contain any numer of nested <Menu>
 *	elements, indicating submenus.
 */
/*  NOTE: because of the way things are (a menu is not named until the <Name> element has been
 *  encountered), we cannot just start merging against an existing menu of the same name at the
 *  onset.  It may still be possible to handle merging on the end tag. */
void
xdg_menu_start_element(GMarkupParseContext *context, const gchar *element_name,
		       const gchar **attribute_names, const gchar **attribute_values,
		       gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgMenu *menu;
	GNode *node, *parent;

	if (attribute_names && *attribute_names)
		EPRINTF("Ignoring attributes in <Menu> element!\n");
	menu = calloc(1, sizeof(*menu));
	node = g_node_new(menu);
	if ((parent = base->node)) {
		g_node_append(parent, node);
		g_queue_push_tail(base->stack, parent);
	} else
		base->tree = node;
	base->node = node;
}

static void
xdg_menu_end_element(GMarkupParseContext *context,
		     const gchar *element_name, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;

	base->node = g_queue_pop_tail(base->stack);
}

static void
xdg_menu_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_menu_parser = {
	.start_element = xdg_menu_start_element,
	.end_element = xdg_menu_end_element,
	.error = xdg_menu_error,
};

XdgMenu *
xdg_get_menu(XdgContext *ctx, const gchar *element_name)
{
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;
	GNode *node;

	if (!(node = base->node)) {
		EPRINTF("Element <%s> can only occur within <Menu>!\n", element_name);
		return (NULL);
	}
	m = g_queue_peek_tail(base->elements);
	if (!m || !m->label || strcmp(m->label, "Menu")) {
		EPRINTF("Element <%s> can only occur directly beneath <Menu>!\n", element_name);
		return (NULL);
	}
	return (node->data);
}

/*
 * <AppDir>
 *	This element may only appear below <Menu>.  The content of this element is a directory name,
 *	desktop entries in this directory are scanned and added to the pool of entries that can be
 *	included in this <Menu> and its submenus.  Only files ending in ".desktop" should be used,
 *	other files are ignored.
 *
 *	Desktop entries in the pool of available entries are identified by their desktop-file id
 *	(see Desktop-File Id).  The desktop-file id of a desktop entry is equal to its filename,
 *	with any path components removed.  SO given a <AppDir> /foo/bar and desktop entry
 *	/foo/bar/Hello.desktop, the desktop entry would ge a Desktop-file id of Hello.desktop.
 *
 *	If the directory contains sub-directories then these sub-directories should be (recursively)
 *	scanned as well.  The name of the subdirectory should be added as prefix to the desktop-file
 *	id together with a dash character ("-").  So, givcen a <AppDir> /foo/bar and desktop entry
 *	/foo/bar/booz/Hello.desktop, the desktop entry would ge a desktop-file id of
 *	booz-Hello.desktop.  A desktop entry /foo/bar/bo/oz/Hello.desktop would result in a
 *	desktop-file id of bo-oz-Hello.desktop.
 *
 *	<AppDir> elements appearing later in the menu file have priority in cases of collisions
 *	between desktop-file ids.
 *
 *	If the filename given as an <AppDir> is not an absolute path, it should be located relative
 *	to the location of the menu file being parsed.
 *
 *	Duplicate <AppDir> elements (that specify the same directory) should be ignored, ut the last
 *	duplicate in the file should be used when establishing the order in which to scan the
 *	directories.  This is important when merging (see the section called "Merging").  The order
 *	of <AppDir> elements with respect to <Include> and <Exclude> elements is not relevant, also
 *	to facility merging.
 */
#if 0
XdgFileEntry *
xdg_add_appid(XdgMenu *menu, const char *path, const char *id, struct stat *st)
{
	XdgFileEntry *fentry;
	char *key;

	key = strdup(path);
	if ((fentry = g_hash_table_lookup(xdg_fileentry_cache, key))) {
		DPRINTF("Desktop entry file '%s' already used\n", key);
		free(key);
		return fentry;
	}
	DPRINTF("Adding new desktop entry file '%s'\n", key);
	fentry = calloc(1, sizeof(*fentry));
	fentry->path = key;
	fentry->id = strdup(id);
	fentry->st = *st;
	g_hash_table_replace(xdg_fileentry_cache, key, fentry);
	if (g_hash_table_replace(menu->apps, fentry->id, fentry))
		DPRINTF("Registered new appid '%s'\n", id);
	else
		DPRINTF("Replaced appid '%s'\n", id);
	return fentry;
}

static void
xdg_scan_appdir(XdgMenu *menu, const char *base, const char *stem)
{
	char *dirname, *file, *appid, *p, *e;
	int dlen, len;
	DIR *dir;
	struct dirent *d;
	struct stat st;

	dlen = strlen(base) + 1;
	if (stem[0] != '\0')
		dlen += strlen(stem) + 1;
	dirname = calloc(dlen + 1, sizeof(*dirname));
	strcpy(dirname, base);
	strcat(dirname, "/");
	if (stem[0] != '\0') {
		strcat(dirname, stem);
		strcat(dirname, "/");
	}
	if (!(dir = opendir(dirname))) {
		EPRINTF("%s: %s\n", dirname, strerror(errno));
		free(dirname);
		return;
	}
	while ((d = readdir(dir))) {
		if (d->d_name[0] == '.')
			continue;
		len = dlen + strlen(d->d_name);
		file = calloc(len + 1, sizeof(*file));
		strcpy(file, dirname);
		strcat(file, d->d_name);
		if (stat(file, &st)) {
			EPRINTF("%s: %s\n", file, strerror(errno));
			free(file);
			continue;
		}
		if (S_ISDIR(st.st_mode)) {
			char *stem2;

			len = strlen(d->d_name);
			if (stem[0] != '\0')
				len += strlen(stem) + 1;
			stem2 = calloc(len + 1, sizeof(*stem2));
			if (stem[0] != '\0') {
				strcpy(stem2, stem);
				strcat(stem2, "/");
			}
			strcat(stem2, d->d_name);
			/* recurse */
			xdg_scan_appdir(menu, base, stem2);
			free(stem2);
			continue;
		} else if (!S_ISREG(st.st_mode)) {
			DPRINTF("%s: not a file\n", file);
			free(file);
			continue;
		}
		if (!strstr(d->d_name, ".desktop")) {
			DPRINTF("%s: does not end in .desktop\n", d->d_name);
			free(file);
			continue;
		}
		len = strlen(d->d_name);
		if (stem[0] != '\0')
			len += strlen(stem) + 1;
		appid = calloc(len + 1, sizeof(*appid));
		if (stem[0] != '\0') {
			strcpy(appid, stem);
			strcat(appid, "/");
			for (p = appid, e = p + strlen(p); p < e; p++)
				if (*p == '/')
					*p = '-';
		}
		strcat(appid, d->d_name);
		xdg_add_appid(menu, file, appid, &st);
		free(file);
		free(appid);
		continue;
	}
	closedir(dir);
	free(dirname);
}

static XdgDirectory *
xdg_add_appdir(XdgMenu *menu, const gchar *path, gsize len)
{
	XdgDirectory *appdir;
	char *key;

	key = strndup(path, len);
	if ((appdir = g_hash_table_lookup(xdg_directory_cache, key))) {
		DPRINTF("Application directory '%s' already used\n", key);
		free(key);
		return appdir;
	}
	DPRINTF("Adding new application directory '%s'\n", key);
	appdir = calloc(1, sizeof(*appdir));
	appdir->path = key;
	g_hash_table_replace(xdg_directory_cache, key, appdir);
	if (!menu->apps)
		menu->apps = g_hash_table_new(g_str_hash, g_str_equal);
	xdg_scan_appdir(menu, key, "");
	return appdir;
}
#endif

static void
xdg_appdir_character_data(GMarkupParseContext *context,
			  const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgMenu *menu;
	XdgDirectory *appdir;
	const gchar *tend = text + text_len;
	char *path;

	if (!(menu = xdg_get_menu(user_data, "AppDir")))
		return;
	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;
	appdir = calloc(1, sizeof(*appdir));
	if (text[0] != '/') {
		int len = strlen(base->directory) + 1 + text_len + 1;

		/* path relative to directory of menu file */
		path = calloc(len, sizeof(*path));
		strcpy(path, base->directory);
		strcat(path, "/");
		strncat(path, text, text_len);
	} else {
		path = strndup(text, text_len);
	}
	appdir->path = path;
	DPRINTF("Adding application directory '%s'\n", appdir->path);
	menu->appdirs = g_list_append(menu->appdirs, appdir);

}

static void
xdg_appdir_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_appdir_parser = {
	.text = xdg_appdir_character_data,
	.error = xdg_appdir_error,
};

/*
 * <DefaultAppDirs>
 *	This element may only appear below <Menu>.  The element has no content.  The element should
 *	be treated as if it were a list of <AppDir> elements contianing the default app dir
 *	lcoations (datadir/applications/ etc.). When expanding <DefaultAppDir> to a list of
 *	<AppDir>, the default locations that are earlier in the search path go later in the <Menu>
 *	so that they have priority.
 */
static void
xdg_appdirs_end_element(GMarkupParseContext *context,
			const gchar *element_name, gpointer user_data, GError **error)
{
	static const char *suffix = "/applications";
	XdgMenu *menu;
	XdgDirectory *appdir;
	char *p, path[PATH_MAX];

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	/* must process in reverse order */
	for (p = xdg_find_str(xdg_data_last, xdg_data_path);
	     p >= xdg_data_path; p = xdg_find_str(p - 1, xdg_data_path)) {
		strncpy(path, p, PATH_MAX - 1);
		strncat(path, suffix, PATH_MAX - 1);
		appdir = calloc(1, sizeof(*appdir));
		appdir->path = strdup(path);
		DPRINTF("Adding application directory '%s'\n", appdir->path);
		menu->appdirs = g_list_append(menu->appdirs, appdir);
	}
}

static void
xdg_appdirs_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_appdirs_parser = {
	.end_element = xdg_appdirs_end_element,
	.error = xdg_appdirs_error,
};

/*
 * <DirectoryDir>
 *	This element may only appear below <Menu>.  The content of this element is a directory name.
 *	Each directory listed in a <DirectoryDir> element will be searched for directory entries to
 *	be used when resolving the <Directory> element for this menu and its submenus.  If the
 *	filename given as a <DirectoryDir> is not an absolute path, it should be located relative to
 *	the location of the menu file being parsed.
 *
 *	Directory entries in the pool of available entries are identified by their relative path
 *	(see Relative Path).
 *
 *	If two directory entires have duplicate relative paths, the one from the last (furthest
 *	down) element in the menu file must be used.  Only files ending in the extension
 *	".directory" should be loaded, other files should be ignored.
 *
 *	Duplicate <DirectoryDir> elements (that specify the same directory) are handled as with
 *	duplicalte <AppDir> elements (the last duplicate is used).
 *
 */
#if 0
XdgFileEntry *
xdg_add_dirid(XdgMenu *menu, const char *path, const char *id, struct stat *st)
{
	XdgFileEntry *fentry;
	char *key;

	key = strdup(path);
	if ((fentry = g_hash_table_lookup(xdg_fileentry_cache, key))) {
		DPRINTF("Desktop entry file '%s' already used\n", key);
		free(key);
		return fentry;
	}
	DPRINTF("Adding new desktop entry file '%s'\n", key);
	fentry = calloc(1, sizeof(*fentry));
	fentry->path = key;
	fentry->id = strdup(id);
	fentry->st = *st;
	g_hash_table_replace(xdg_fileentry_cache, key, fentry);
	if (g_hash_table_replace(menu->apps, fentry->id, fentry))
		DPRINTF("Registered new dirid '%s'\n", id);
	else
		DPRINTF("Replaced dirid '%s'\n", id);
	return fentry;
}

static void
xdg_scan_dirdir(XdgMenu *menu, const char *base, const char *stem)
{
	char *dirname, *file, *dirid;
	int dlen, len;
	DIR *dir;
	struct dirent *d;
	struct stat st;

	dlen = strlen(base) + 1;
	if (stem[0] != '\0')
		dlen += strlen(stem) + 1;
	dirname = calloc(dlen + 1, sizeof(*dirname));
	strcpy(dirname, base);
	strcat(dirname, "/");
	if (stem[0] != '\0') {
		strcat(dirname, stem);
		strcat(dirname, "/");
	}
	if (!(dir = opendir(dirname))) {
		EPRINTF("%s: %s\n", dirname, strerror(errno));
		free(dirname);
		return;
	}
	while ((d = readdir(dir))) {
		if (d->d_name[0] == '.')
			continue;
		len = dlen + strlen(d->d_name);
		file = calloc(len + 1, sizeof(*file));
		strcpy(file, dirname);
		strcat(file, d->d_name);
		if (stat(file, &st)) {
			EPRINTF("%s: %s\n", file, strerror(errno));
			free(file);
			continue;
		}
		if (S_ISDIR(st.st_mode)) {
			char *stem2;

			len = strlen(d->d_name);
			if (stem[0] != '\0')
				len += strlen(stem) + 1;
			stem2 = calloc(len + 1, sizeof(*stem2));
			if (stem[0] != '\0') {
				strcpy(stem2, stem);
				strcat(stem2, "/");
			}
			strcat(stem2, d->d_name);
			/* recurse */
			xdg_scan_dirdir(menu, base, stem2);
			free(stem2);
			continue;
		} else if (!S_ISREG(st.st_mode)) {
			DPRINTF("%s: not a file\n", file);
			free(file);
			continue;
		}
		if (!strstr(d->d_name, ".directory")) {
			DPRINTF("%s: does not end in .directory\n", d->d_name);
			free(file);
			continue;
		}
		len = strlen(d->d_name);
		if (stem[0] != '\0')
			len += strlen(stem) + 1;
		dirid = calloc(len + 1, sizeof(*dirid));
		if (stem[0] != '\0') {
			strcpy(dirid, stem);
			strcat(dirid, "/");
		}
		strcat(dirid, d->d_name);
		xdg_add_dirid(menu, file, dirid, &st);
		free(file);
		free(dirid);
		continue;
	}
	closedir(dir);
	free(dirname);
}

static XdgDirectory *
xdg_add_dirdir(XdgMenu *menu, const gchar *path, gsize len)
{
	XdgDirectory *dirdir;
	char *key;

	key = strndup(path, len);
	if ((dirdir = g_hash_table_lookup(xdg_directory_cache, key))) {
		DPRINTF("Directory directory '%s' already used\n", key);
		free(key);
		return dirdir;
	}
	DPRINTF("Adding new directory directory '%s'\n", key);
	dirdir = calloc(1, sizeof(*dirdir));
	dirdir->path = key;
	g_hash_table_replace(xdg_directory_cache, key, dirdir);
	if (!menu->dirs)
		menu->dirs = g_hash_table_new(g_str_hash, g_str_equal);
	xdg_scan_dirdir(menu, key, "");
	return dirdir;
}
#endif

static void
xdg_dirdir_character_data(GMarkupParseContext *context,
			  const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgMenu *menu;
	XdgDirectory *dirdir;
	const gchar *tend = text + text_len;
	char *path;

	if (!(menu = xdg_get_menu(user_data, "DirectoryDir")))
		return;
	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;
	dirdir = calloc(1, sizeof(*dirdir));
	if (text[0] != '/') {
		int len = strlen(base->directory) + 1 + text_len + 1;

		/* path relative to directory of menu file */
		path = calloc(len, sizeof(*path));
		strcpy(path, base->directory);
		strcat(path, "/");
		strncat(path, text, text_len);
	} else
		path = strndup(text, text_len);
	dirdir->path = path;
	DPRINTF("Adding directory directory '%s'\n", dirdir->path);
	menu->dirdirs = g_list_append(menu->dirdirs, dirdir);
}

static void
xdg_dirdir_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_dirdir_parser = {
	.text = xdg_dirdir_character_data,
	.error = xdg_dirdir_error,
};

/*
 * <DefaultDirectoryDirs>
 *	This element may only appear  below <Menu>.  The element has no content.  The element should
 *	be treated as if it were a list of <DirectoryDir> elements containing the default desktop
 *	directory locations (datadir/desktop-directories/ etc.).  THe default locates that are
 *	earlier in the search path go later in the <Menu> so that they have priority.
 */
static void
xdg_dirdirs_end_element(GMarkupParseContext *context,
			const gchar *element_name, gpointer user_data, GError **error)
{
	static const char *suffix = "/desktop-directories";
	XdgMenu *menu;
	XdgDirectory *dirdir;
	char *p, path[PATH_MAX];

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	/* must process in reverse order */
	for (p = xdg_find_str(xdg_data_last, xdg_data_path);
	     p >= xdg_data_path; p = xdg_find_str(p - 1, xdg_data_path)) {
		strncpy(path, p, PATH_MAX - 1);
		strncat(path, suffix, PATH_MAX - 1);
		dirdir = calloc(1, sizeof(*dirdir));
		dirdir->path = strdup(path);
		DPRINTF("Adding directory directory '%s'\n", dirdir->path);
		menu->dirdirs = g_list_append(menu->dirdirs, dirdir);
	}
}

static void
xdg_dirdirs_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_dirdirs_parser = {
	.end_element = xdg_dirdirs_end_element,
	.error = xdg_dirdirs_error,
};

/*
 * <Name>
 *	Each <Menu> element must have a single <Name> element.  The content of the <Name> element
 *	is a name to be used when referring to the given menu.  Each submenu of a given <Menu> must
 *	have a unique name.  <Menu> elements can thus be references by a menu path, for example
 *	"Applications/Graphics."  THe <Name> field must not contain the slash character ("/");
 *	implementations should discard any name containing a slas.  See also Menu path.
 */
static void
xdg_name_character_data(GMarkupParseContext *context,
			const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	XdgMenu *menu;
	const gchar *tend = text + text_len;
	char *name;

	if (!(menu = xdg_get_menu(user_data, "Name")))
		return;
	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;
	name = strndup(text, text_len);
	free(menu->name);
	menu->name = name;
	DPRINTF("Menu is named: '%s'\n", name);
}

static void
xdg_name_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_name_parser = {
	.text = xdg_name_character_data,
	.error = xdg_name_error,
};

/*
 * <Directory>
 *	Each <Menu> element has any number of <Directory> elements.  The content of the <Directory>
 *	lement is the relative path of a directory entry containing meta information about the
 *	<Menu>, such as its icon and localized name.  If no <Directory> is specified for a <Menu>,
 *	its <Name> field should be used as the user-visible name of the menu.
 *
 *	Duplicate <Directory> elements are allowed to simplify menu merging, and allow user menus to
 *	override system menus.  The last <Directory> element to appear in the menu file "wins" and
 *	other elements are ignored, unless the last element points to a nonexistent directory entry,
 *	in which case the previous element should eb tried instead, and so on.
 */
static void
xdg_dir_character_data(GMarkupParseContext *context,
		       const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	XdgMenu *menu;
	const gchar *tend = text + text_len;
	char *key;

	if (!(menu = xdg_get_menu(user_data, "Directory")))
		return;
	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;
	key = strndup(text, text_len);
	if (menu->directory)
		DPRINTF("Replacing menu directory with '%s'", key);
	else
		DPRINTF("Adding menu directory '%s'\n", key);
	free(menu->directory);
	menu->directory = key;
}

static void
xdg_dir_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_dir_parser = {
	.text = xdg_dir_character_data,
	.error = xdg_dir_error,
};

/*
 * <OnlyUnallocated> and <NotOnlyUnallocated>
 *	Each <Menu> may contain any number of <OnlyUnallocated> and <NotOnlyUnallocated> elements.
 *	Only the last such element to appear is relevant, as it determines whether the <Menu> can
 *	contain any desktop entries, or only those desktop entries that do not match other menus.
 *	If neither <OnlyUnallocated> nor <NotOnlyUnallocated> elements are present, the default is
 *	<NotOnlyUnallocated>.
 *
 *	To handle <OnlyUnallocated>, the menu file must be analyzed in two conceptual passes.  The
 *	first pass processes <Menu> elements that cna match any desktop entry.  During this pass,
 *	each desktop entry is marked as allocated according to whether it was matched by an
 *	<Include> rule in some <Menu>.  The second pass processes only <Menu> elements that are
 *	restircted to unallocated desktpo entires.  During the second pass, queries may only match
 *	desktop entries that were not marked as allocated during the first pass.  See the section
 *	called "Generating the menus".
 */
static void
xdg_only_u_end_element(GMarkupParseContext *context,
		       const gchar *element_name, gpointer user_data, GError **error)
{
	XdgMenu *menu;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	menu->only_u = TRUE;
}

static void
xdg_only_u_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_only_u_parser = {
	.end_element = xdg_only_u_end_element,
	.error = xdg_only_u_error,
};

static void
xdg_not_u_end_element(GMarkupParseContext *context,
		      const gchar *element_name, gpointer user_data, GError **error)
{
	XdgMenu *menu;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	menu->only_u = FALSE;
}

static void
xdg_not_u_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_not_u_parser = {
	.end_element = xdg_not_u_end_element,
	.error = xdg_not_u_error,
};

/*
 * <Deleted> and <NotDeleted>
 *	Each <Menu> may contain any number of <Deleted> and <NotDeleted> elements.  Only the last
 *	such element to appear is relevant, as it determines whether the <Menu> has been deleted.
 *	If neither <Deleted> no <NotDeleted> elemetns are present, the default is <NotDeleted>.  The
 *	purpose of this element is to support menuy editing.  If a menu contains a <Deleted> element
 *	not followed by a <NotDeleted> element, that menu should be ignored.
 */
static void
xdg_deleted_end_element(GMarkupParseContext *context,
			const gchar *element_name, gpointer user_data, GError **error)
{
	XdgMenu *menu;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	menu->deleted = TRUE;
}

static void
xdg_deleted_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_deleted_parser = {
	.end_element = xdg_deleted_end_element,
	.error = xdg_deleted_error,
};

static void
xdg_not_deleted_end_element(GMarkupParseContext *context,
			    const gchar *element_name, gpointer user_data, GError **error)
{
	XdgMenu *menu;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	menu->deleted = FALSE;
}

static void
xdg_not_deleted_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_not_deleted_parser = {
	.end_element = xdg_not_deleted_end_element,
	.error = xdg_not_deleted_error,
};

/*
 * <Include>
 *	An <Include> element is a set of rules attempting to match some of the known dsektop
 *	entries.  The <Include> element contains a list of any number of matching rules.  Matching
 *	rules are specified using the elements <And>, <Or>, <Not>, <All>, <Filename?>, and
 *	<Category>.  Each ruls in a list of rules has a logical OR felationship, that is, desktop
 *	entires that match any rule are included in the menu.
 *
 *	<Include> elements must appear immediately under <Menu> elements.  The desktop entires they
 *	match are included in the menu.  <Include> and <Exclude> elements for a given <Menu> are
 *	processed in order, with queries earlier in the file handled first.  This has implications
 *	for merging, see the section called "Merging".  Se the section called "Generating the menus"
 *	for full deatils on how to process <Include> and <Exclude> elements.
 */
void
xdg_include_start_element(GMarkupParseContext *context, const gchar *element_name,
			  const gchar **attribute_names, const gchar **attribute_values,
			  gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgMenu *menu;
	XdgRule *rule;
	GNode *node;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	rule = calloc(1, sizeof(*rule));
	rule->type = LogicTypeInclude;
	node = g_node_new(rule);
	menu->rules = g_list_append(menu->rules, node);
	if (base->rule)
		g_queue_push_tail(base->rules, base->rule);
	base->rule = node;
}

static void
xdg_include_end_element(GMarkupParseContext *context,
			const gchar *element_name, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;

	base->rule = g_queue_pop_tail(base->rules);
}

static void
xdg_include_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_include_parser = {
	.start_element = xdg_include_start_element,
	.end_element = xdg_include_end_element,
	.error = xdg_include_error,
};

/*
 * <Exclude>
 *	Any number of <Exclude> elementes may appear below a <Menu> element.  The content of an
 *	<Exclude> element is a list of matching rules, just as with an <Include>.  However, the
 *	desktpo entries matches are removed from the list of desktop entires included so far.  (Thus
 *	an <Exclude> element that appears before any <Include> elements will have no affect, for
 *	example, as no desktop entries have been incldued yet.)
 */
void
xdg_exclude_start_element(GMarkupParseContext *context, const gchar *element_name,
			  const gchar **attribute_names, const gchar **attribute_values,
			  gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgMenu *menu;
	XdgRule *rule;
	GNode *node;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	rule = calloc(1, sizeof(*rule));
	rule->type = LogicTypeExclude;
	node = g_node_new(rule);
	menu->rules = g_list_append(menu->rules, node);
	if (base->rule)
		g_queue_push_tail(base->rules, base->rule);
	base->rule = node;
}

static void
xdg_exclude_end_element(GMarkupParseContext *context,
			const gchar *element_name, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;

	base->rule = g_queue_pop_tail(base->rules);
}

static void
xdg_exclude_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_exclude_parser = {
	.start_element = xdg_exclude_start_element,
	.end_element = xdg_exclude_end_element,
	.error = xdg_exclude_error,
};

/*
 * <Filename>
 *	The <Filename> element is the most basic matching rule.  It matches a desktop entry if the
 *	desktop entry ahs the given desktop-file id.  See Desktop-File Id.
 */
static void
xdg_filename_character_data(GMarkupParseContext *context,
			    const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element_name = "Filename";
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;
	const gchar *tend = text + text_len;

	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;

	m = g_queue_peek_tail(base->elements);
	if (!strcmp(m->label, "Layout") || !strcmp(m->label, "DefaultLayout")) {
		XdgMenu *menu;
		XdgLayout *layout;
		GList **list;

		if (!base->node || !(menu = base->node->data)) {
			EPRINTF("Element <%s> must be within <Menu>!\n", element_name);
			return;
		}
		if (!strcmp(m->label, "Layout"))
			list = &menu->layout;
		else
			list = &menu->deflay;
		layout = calloc(1, sizeof(*layout));
		layout->type = LayoutTypeFilename;
		layout->string = strndup(text, text_len);
		*list = g_list_append(*list, layout);
	} else {
		XdgRule *rule;
		GNode *node, *parent;

		if (!(parent = base->rule)) {
			EPRINTF("Element <%s>, but no parent root node!\n", element_name);
			return;
		}
		if (!(rule = parent->data)) {
			EPRINTF("Element <%s> has parent, but rule missing!\n", element_name);
			return;
		}
		if (rule->type == LogicTypeRoot) {
			EPRINTF("Element <%s> must be within <Include> or <Exclude>!\n",
				element_name);
			return;
		}
		rule = calloc(1, sizeof(*rule));
		rule->type = LogicTypeFilename;
		rule->string = strndup(text, text_len);
		node = g_node_new(rule);
		g_node_append(parent, node);
	}
}

static void
xdg_filename_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_filename_parser = {
	.text = xdg_filename_character_data,
	.error = xdg_filename_error,
};

/*
 * <Category>
 *	The <Category> element is another basic matching predicate.  It matches a desktop entry if
 *	the desktop entry has the given category in its Cateogries field.
 */
static void
xdg_category_character_data(GMarkupParseContext *context,
			    const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element_name = "Category";
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgRule *rule;
	GNode *node, *parent;
	const gchar *tend = text + text_len;

	if (!(parent = base->rule)) {
		EPRINTF("Element <%s>, but no parent root node!\n", element_name);
		return;
	}
	if (!(rule = parent->data)) {
		EPRINTF("Element <%s> has parent, but rule missing!\n", element_name);
		return;
	}
	if (rule->type == LogicTypeRoot) {
		EPRINTF("Element <%s> must be within <Include> or <Exclude>!\n", element_name);
		return;
	}
	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;
	rule = calloc(1, sizeof(*rule));
	rule->type = LogicTypeCategory;
	rule->string = strndup(text, text_len);
	node = g_node_new(rule);
	g_node_append(parent, node);
}

static void
xdg_category_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_category_parser = {
	.text = xdg_category_character_data,
	.error = xdg_category_error,
};

/*
 * <All>
 *	The <All> element is a matching rule that matches all desktop entries.
 */
static void
xdg_all_end_element(GMarkupParseContext *context,
		    const gchar *element_name, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgRule *rule;
	GNode *node, *parent;

	if (!(parent = base->rule)) {
		EPRINTF("Element <%s>, but no parent root node!\n", element_name);
		return;
	}
	if (!(rule = parent->data)) {
		EPRINTF("Element <%s> has parent, but rule missing!\n", element_name);
		return;
	}
	if (rule->type == LogicTypeRoot) {
		EPRINTF("Element <%s> must be within <Include> or <Exclude>!\n", element_name);
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = LogicTypeAll;
	node = g_node_new(rule);
	g_node_append(parent, node);
}

static void
xdg_all_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_all_parser = {
	.end_element = xdg_all_end_element,
	.error = xdg_all_error,
};

/*
 * <And>
 *	The <And> element contains a list of matching fules.  If each of the matching rule inside
 *	the <And> element match a desktop entry, then the entire <And> rule matches the desktop
 *	entry.
 */
void
xdg_and_start_element(GMarkupParseContext *context, const gchar *element_name,
		      const gchar **attribute_names, const gchar **attribute_values,
		      gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgRule *rule;
	GNode *node, *parent;

	if (!(parent = base->rule)) {
		EPRINTF("Element <%s>, but no parent root node!\n", element_name);
		return;
	}
	if (!(rule = parent->data)) {
		EPRINTF("Element <%s> has parent, but rule missing!\n", element_name);
		return;
	}
	if (rule->type == LogicTypeRoot) {
		EPRINTF("Element <%s> must be within <Include> or <Exclude>!\n", element_name);
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = LogicTypeAnd;
	node = g_node_new(rule);
	g_node_append(parent, node);
	g_queue_push_tail(base->rules, base->rule);
	base->rule = node;
}

static void
xdg_and_end_element(GMarkupParseContext *context,
		    const gchar *element_name, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;

	base->rule = g_queue_pop_tail(base->rules);
}

static void
xdg_and_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_and_parser = {
	.start_element = xdg_and_start_element,
	.end_element = xdg_and_end_element,
	.error = xdg_and_error,
};

/*
 * <Or>
 *	The <Or> element contains a list of matching rules.  If any of the matching rules inside the
 *	<Or> element match a desktop entry, then the entire <Or> rule matches the desktop entry.
 */
void
xdg_or_start_element(GMarkupParseContext *context, const gchar *element_name,
		     const gchar **attribute_names, const gchar **attribute_values,
		     gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgRule *rule;
	GNode *node, *parent;

	if (!(parent = base->rule)) {
		EPRINTF("Element <%s>, but no parent root node!\n", element_name);
		return;
	}
	if (!(rule = parent->data)) {
		EPRINTF("Element <%s> has parent, but rule missing!\n", element_name);
		return;
	}
	if (rule->type == LogicTypeRoot) {
		EPRINTF("Element <%s> must be within <Include> or <Exclude>!\n", element_name);
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = LogicTypeOr;
	node = g_node_new(rule);
	g_node_append(parent, node);
	g_queue_push_tail(base->rules, base->rule);
	base->rule = node;
}

static void
xdg_or_end_element(GMarkupParseContext *context,
		   const gchar *element_name, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;

	base->rule = g_queue_pop_tail(base->rules);
}

static void
xdg_or_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_or_parser = {
	.start_element = xdg_or_start_element,
	.end_element = xdg_or_end_element,
	.error = xdg_or_error,
};

/*
 * <Not>
 *	The <Not> element contains a list of matching rules.  If any of the matching rules inside
 *	the <Not> element matches a desktop entry ,then the entire <Not> rules does not match the
 *	desktop entry.  That is, matching rules below <Not> have a logical OR rleationship.
 */
void
xdg_not_start_element(GMarkupParseContext *context, const gchar *element_name,
		      const gchar **attribute_names, const gchar **attribute_values,
		      gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgRule *rule;
	GNode *node, *parent;

	if (!(parent = base->rule)) {
		EPRINTF("Element <%s>, but no parent root node!\n", element_name);
		return;
	}
	if (!(rule = parent->data)) {
		EPRINTF("Element <%s> has parent, but rule missing!\n", element_name);
		return;
	}
	if (rule->type == LogicTypeRoot) {
		EPRINTF("Element <%s> must be within <Include> or <Exclude>!\n", element_name);
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = LogicTypeNot;
	node = g_node_new(rule);
	g_node_append(parent, node);
	g_queue_push_tail(base->rules, base->rule);
	base->rule = node;
}

static void
xdg_not_end_element(GMarkupParseContext *context,
		    const gchar *element_name, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;

	base->rule = g_queue_pop_tail(base->rules);
}

static void
xdg_not_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_not_parser = {
	.start_element = xdg_not_start_element,
	.end_element = xdg_not_end_element,
	.error = xdg_not_error,
};

/*
 * <MergeFile [type="path"|"parent"]>
 *	Any number of <MergeFile> elements may be listed below a <Menu> element, giving the name of
 *	another menu file to be merged into this one.  The section called "Merging" specifies how
 *	merging is done.  The root <Menu> of the merged file will be merged into the immediate
 *	parent of the <MergeFile> element.  The <Name> element of the root <Menu> of the merged
 *	file are ignored.
 *
 *	If the type attribute is missing or set to "path" then the contents of the <MergeFile.
 *	element indicates the file to be merged.  If this is not an absolute path then the file to
 *	be merged should be located relative to the location of the menu file that contains this
 *	<MergeFile> element.
 *
 *	Duplicate <MergeFile> elements (that specify the same file) are handled as with duplicate
 *	<AppDir> elements (the last duplicate is used).
 *
 *	If the type attribute is set to "parent" and the file that contains this <MergeFile> element
 *	is located under one of the paths specified by $XDG_CONFIG_DIRS, the contents of the
 *	element should be ignored and the remaining paths specified by $XDG_CONFIG_DIRS are search
 *	for a file with the same relative filename.  The first file encountered should be merged.
 *	There should be no merging at all if no matching file is found.
 *
 *	Compatibility note:  The filename specified inside the <MergeFile> element should be ignored
 *	if the type attribute is set to "parent", it should however be expected that
 *	imiplemeNtations based on previous versions of this specification will ingore the type
 *	attribute and that such implementations will sue the filename inside the <MergeFile> element
 *	instead.
 *
 *	Example 1: If $XDG_CONFIG_HOME is "~/.config/" and $XDG_CONFIG_DIRS is
 *	"/opt/gnome/:/etc/xdg/" and the file ~/.config/menus/applications.menu contains <MergeFile
 *	type="parent">/opt/kde3/etc/xdg/menus/applications.menu</MergeFile> then the file
 *	/opt/gnome/menus/applications.menu should be merged if it exists.  If that file does not
 *	exist then the file /etc/xdg/menus/applications.menu should be merged instead.
 *
 *	Example 2: If $XDG_CONFIG_HOME is "~/.config/" and $XDG_CONFIG_DIRS is
 *	"/opt/gnome/:/etc/xdg/" and the file /opt/gnome/menus/applications.menu contains <MergeFile
 *	type="parent">/opt/kde3/etc/xdg/menus/applications.menu</MergeFile> then the file
 *	/etc/xdg/menus/applications.menu should be merged if it exists.
 */
void
xdg_mergefile_start_element(GMarkupParseContext *context, const gchar *element_name,
			    const gchar **attribute_names, const gchar **attribute_values,
			    gpointer user_data, GError **error)
{
	XdgMenu *menu;
	XdgMerge *mfile;
	const gchar **name, **valu;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;

	mfile = calloc(1, sizeof(*mfile));
	mfile->type = MergeTypeFilePath;

	for (name = attribute_names, valu = attribute_values; name && *name; name++, valu++) {
		if (!strcmp(*name, "type")) {
			if (!strcasecmp(*valu, "path"))
				mfile->type = MergeTypeFilePath;
			else if (!strcasecmp(*valu, "parent"))
				mfile->type = MergeTypeFileParent;
			else
				WPRINTF
				    ("Unrecognized value '%s' of attribute '%s' of element <%s>!\n",
				     *valu, *name, element_name);
		} else
			WPRINTF("Unrecognized attribute '%s' of element <%s>!\n",
				*name, element_name);
	}

	menu->merge = g_list_append(menu->merge, mfile);

}

static void
xdg_mergefile_character_data(GMarkupParseContext *context,
			     const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	static const char *element_name = "MergeFile";
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgMenu *menu;
	XdgMerge *mfile;
	GList *item;
	const gchar *tend = text + text_len;
	char *path;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	if (!(item = g_list_last(menu->merge)))
		return;

	for (; text < text && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;

	mfile = item->data;
	if (mfile->type == MergeTypeFilePath && text[0] != '/') {
		int len = strlen(base->directory) + 1 + text_len + 1;

		/* path relative to directory of menu file */
		path = calloc(len, sizeof(*path));
		strcpy(path, base->directory);
		strcat(path, "/");
		strncat(path, text, text_len);
	} else
		path = strndup(text, text_len);
	mfile->path = strndup(text, text_len);
}

static void
xdg_mergefile_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_mergefile_parser = {
	.start_element = xdg_mergefile_start_element,
	.text = xdg_mergefile_character_data,
	.error = xdg_mergefile_error,
};

/*
 * <MergeDir>
 *	Any number of <MergeDir> elements may be listed below <Menu> element.  A <MergeDir> contains
 *	the name of a directory.  Each file in the given directoy which ends in the ".menu"
 *	extension should be merged in the same way that a <MergeFile> would be.  If the filename
 *	given as a <MergeDir> is not an absolute path, it should be located relative to the location
 *	of the menu file being parsed.  The files inside the merged directory are not merged in any
 *	specified order.
 *
 *	Duplicate <MergeDir> elements (that specify the same directory) are handled as with
 *	duplicate <AppDir> element (the last duplicate is used).
 */
static void
xdg_mergedir_character_data(GMarkupParseContext *context,
			    const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	static const char *element_name = "MergeDir";
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgMenu *menu;
	XdgMerge *mdir;
	const gchar *tend = text + text_len;
	char *path;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;

	for (; text < text && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;

	mdir = calloc(1, sizeof(*mdir));
	mdir->type = MergeTypeDirectory;

	if (text[0] != '/') {
		int len = strlen(base->directory) + 1 + text_len + 1;

		/* relative to directory of menu file */
		path = calloc(len, sizeof(*path));
		strcpy(path, base->directory);
		strcat(path, "/");
		strncat(path, text, text_len);
	} else
		path = strndup(text, text_len);
	mdir->path = path;
	DPRINTF("Adding merge directory '%s'\n", mdir->path);
	menu->merge = g_list_append(menu->merge, mdir);
}

static void
xdg_mergedir_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_mergedir_parser = {
	.text = xdg_mergedir_character_data,
	.error = xdg_mergedir_error,
};

/*
 * <DefaultMergeDirs>
 *	This element may only appear below <Menu>.  The element has no content.  The element should
 *	eb treated as if it were a list of <MergeDir> elements containing the default merge
 *	directory locations.  When expanding <DefaultMergeDirs> to a list of <MergeDir>, the
 *	default locations that are earlier in the serach path go later in the <Menu> so that they
 *	have priority.
 */
static void
xdg_mergedirs_end_element(GMarkupParseContext *context,
			  const gchar *element_name, gpointer user_data, GError **error)
{
	static const char *suffix = "/menus/applications-merged";
	XdgMenu *menu;
	XdgMerge *mrgdir;
	char *p, path[PATH_MAX];

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	/* must process in reverse order */
	for (p = xdg_find_str(xdg_config_last, xdg_config_path);
	     p >= xdg_config_path; p = xdg_find_str(p - 1, xdg_config_path)) {
		strncpy(path, p, PATH_MAX - 1);
		strncat(path, suffix, PATH_MAX - 1);
		DPRINTF("Adding merge directory '%s'\n", path);
		mrgdir = calloc(1, sizeof(*mrgdir));
		mrgdir->type = MergeTypeDirectory;
		mrgdir->path = strdup(path);
		menu->merge = g_list_append(menu->merge, mrgdir);
	}
}

static void
xdg_mergedirs_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_mergedirs_parser = {
	.end_element = xdg_mergedirs_end_element,
	.error = xdg_mergedirs_error,
};

/*
 * <LegacyDir [prefx="PREFIX-"]>
 *	This element may only appear below <Menu>.  The text content of this element is a directory
 *	name.  Each directory listed in a <LegacyDir> element will be an old-style legacy hierarchy
 *	of desktop entires, see the section called "Legacy Menu Hierarchies" for how to load such a
 *	hierarchy.  Implemetations must not load legacy hierarchies that are not explicitly
 *	specified in the menu file (because for example the menu file may not be the main menu).  If
 *	the filename given as a <LegacyDir> is not an absolute path, it should be located relative
 *	to the location of the menu field being parsed.
 *
 *	Duplicate <LegacyDir> elemetns (that specify the same directory) are handled as with
 *	duplicate <AppDir> elements (the last duplicate si used).
 *
 *	The <LegacyDir> element may have one attribute, prefix.  Normally, given a <LegacyDir>
 *	/foo/bar and desktop entry /foo/bar/baz/Hello.desktop the desktop entry would get a
 *	desktop-file id of Hello.desktop.  Given a prefix of boo-, it would instead by assigned the
 *	desktop-file id boo-Hello.desktop.  The prefix should not contain patch separator ('/')
 *	characters.
 */
void
xdg_legacydir_start_element(GMarkupParseContext *context, const gchar *element_name,
			    const gchar **attribute_names, const gchar **attribute_values,
			    gpointer user_data, GError **error)
{
	XdgMenu *menu;
	XdgMerge *legdir;
	const gchar **name, **valu;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	legdir = calloc(1, sizeof(*legdir));
	legdir->type = MergeTypeLegacy;
	for (name = attribute_names, valu = attribute_values; name && *name; name++, valu++) {
		if (!strcmp(*name, "prefix"))
			legdir->prefix = strdup(*valu);
	}
	menu->merge = g_list_append(menu->merge, legdir);
}

static void
xdg_legacydir_character_data(GMarkupParseContext *context,
			     const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	static const char *element_name = "LegacyDir";
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgMenu *menu;
	XdgMerge *legdir;
	GList *item;
	const gchar *tend = text + text_len;
	char *path;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	if (!(item = g_list_last(menu->merge)))
		return;
	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;
	legdir = item->data;
	if (text[0] != '/') {
		int len = strlen(base->directory) + 1 + text_len + 1;

		/* path is realtive to the directory of the menu file */
		path = calloc(len, sizeof(*path));
		strcpy(path, base->directory);
		strcat(path, "/");
		strncat(path, text, text_len);
	} else
		path = strndup(text, text_len);
	free(legdir->path);
	DPRINTF("Adding legacy directory '%s'\n", legdir->path);
	legdir->path = path;
}

static void
xdg_legacydir_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_legacydir_parser = {
	.start_element = xdg_legacydir_start_element,
	.text = xdg_legacydir_character_data,
	.error = xdg_legacydir_error,
};

/*
 * <KDELegacyDirs>
 *	This element man only appear below <Menu>.  The element has no content.  The element should
 *	be treated as it if were a list of <LegacyDir> elements containing the traditional desktop
 *	file locations supported by KDE with a hard coded prefix of "kde-".  When expanding
 *	<KDELegacyDirs> to a list of <LegacyDir>, the locations that are earlier in the earch path
 *	go later in the <Menu> so that they have priority.  The search path can be obtained by
 *	running kde-config --path apps.
 */
static void
xdg_kdedirs_end_element(GMarkupParseContext *context,
			const gchar *element_name, gpointer user_data, GError **error)
{
	DPRINTF("The <%s> element is obsolete and deprecated.\n", element_name);
}

static void
xdg_kdedirs_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_kdedirs_parser = {
	.end_element = xdg_kdedirs_end_element,
	.error = xdg_kdedirs_error,
};

/*
 * <Move>
 *	This element may only appear below <Menu>.  The <Move> elemtn contains pairs of <Old>/<New>
 *	elements indicating how to rename a descendant of the current <Menu>.  If the destination
 *	path already exists, the moved menu is merged with the destination menu (see the section
 *	called "Merging" for details).
 *
 *	<Move> is used primarily to fix up legacy directories.  For example, say you are merging a
 *	<LegacyDir> with folder names that don't match the current hierarchy; the legacy folder
 *	names can be moved to the new names, where they will be merged wit hthe new folders.
 *
 *	<Move> is also useful fo implementing menu editing, see the section called "Menu editing".
 */
void
xdg_move_start_element(GMarkupParseContext *context, const gchar *element_name,
		       const gchar **attribute_names, const gchar **attribute_values,
		       gpointer user_data, GError **error)
{
	XdgMenu *menu;
	XdgMove *move;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	move = calloc(1, sizeof(*move));
	menu->moves = g_list_append(menu->moves, move);
}

static void
xdg_move_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_move_parser = {
	.start_element = xdg_move_start_element,
	.error = xdg_move_error,
};

/*
 * <Old>
 *	This element may only appear below <Move>, and must be followed by a <New> element.  The
 *	content of both <Old> and <New> should be a menu path (slash-separated concatenation of
 *	<Name> fields, see Menu path).  Paths are interpreted relative to the menu containing the
 *	<Move> element.
 */
static void
xdg_old_character_data(GMarkupParseContext *context,
		       const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const char *element_name = "Old";
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;
	XdgMenu *menu;
	XdgMove *move;
	GList *item;
	const gchar *tend = text + text_len;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	m = g_queue_peek_tail(base->elements);
	if (!m || !m->label || strcmp(m->label, "Move")) {
		EPRINTF("Element <%s> can only occur directly beneath <Move>!\n", element_name);
		return;
	}
	if (!(item = g_list_last(menu->moves)))
		return;
	move = item->data;
	if (move->old) {
		WPRINTF("Multiple element <%s> for a given <Move>!\n", element_name);
		free(move->old);
	}
	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;
	move->old = strndup(text, text_len);
	DPRINTF("Old menu path is '%s'\n", move->old);
}

static void
xdg_old_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_old_parser = {
	.text = xdg_old_character_data,
	.error = xdg_old_error,
};

/*
 * <New>
 *	This element may only appear below <Move>, and must be preceded by an <Old> element.  The
 *	<New> element specifies the new path for the preceding <Old> element.
 */
static void
xdg_new_character_data(GMarkupParseContext *context,
		       const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const char *element_name = "New";
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;
	XdgMenu *menu;
	XdgMove *move;
	GList *item;
	const gchar *tend = text + text_len;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	m = g_queue_peek_tail(base->elements);
	if (!m || !m->label || strcmp(m->label, "Move")) {
		EPRINTF("Element <%s> can only occur directly beneath <Move>!\n", element_name);
		return;
	}
	if (!(item = g_list_last(menu->moves)))
		return;
	move = item->data;
	if (move->new) {
		WPRINTF("Multiple element <%s> for a given <Move>!\n", element_name);
		free(move->new);
	}
	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;
	move->new = strndup(text, text_len);
	DPRINTF("New menu path is '%s'\n", move->new);
}

static void
xdg_new_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_new_parser = {
	.text = xdg_new_character_data,
	.error = xdg_new_error,
};

/*
 * <Layout>
 *	The <Layout element is an optional part of this specification.  Implementations that do not
 *	support the <Layout> element should preserve any <Layout> elements and their contents as far
 *	as possible. Each <Menu> may optionally contain a <Layout> element.  If multiple elements
 *	appear then only the last such element is relevant.  The purpose of this element is to offer
 *	suggestions for the presentation of the menu.  If a menu does not contain a <Layout> element
 *	or if it contains an empty <Layout> element, the the default layout should be used.  The
 *	<Layout> element may contain <Filename>, <Menuname>, <Separator> and <Merge> elements.  The
 *	<Layout> element defines a suggested layout for the menu starting from the top to bottom.
 *	References to desktop entries that are not contained in this menu as defined by the
 *	<Include> and <Exclude> elements should be ignored.  References to sub-menus that are not
 *	directly contained in this menu as defined by the <Menu> elements should be ignored.
 */
void
xdg_layout_start_element(GMarkupParseContext *context, const gchar *element_name,
			 const gchar **attribute_names, const gchar **attribute_values,
			 gpointer user_data, GError **error)
{
	XdgMenu *menu;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	if (menu->layout) {
		g_list_free_full(menu->layout, xdg_layout_free);
		menu->layout = NULL;
	}
}

static void
xdg_layout_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_layout_parser = {
	.start_element = xdg_layout_start_element,
	.error = xdg_layout_error,
};

/*
 * <DefaultLayout [show_empty="false"] [inline="false"] [inline_limit="4"] [inline_header="true"]
 * [inline_alias="false"]>
 *	The <DefaultLayout> element is an optional part of this specfication.  Implementations that
 *	do not support the <DefaultLayout> element should preserve any <DefaultLayout> elements and
 *	their contents as far as possible.  Each <Menu> may optionally contain a <DefaultLayout>
 *	element that defines the default-layout for the current menu and all its sub-menus.  If a
 *	menu has a <DefaultLayout> element then this will override any default-layout specified by a
 *	parent menu.  The default-layout defines the suggested layout if a <Menu> element does
 *	either not have <Layout> element or if its has an empty <Layout> element.  For explanations
 *	of the various attributes, see the <Menuname> element.  If no default-layout has been
 *	specifeid, then the layou as specified by the following elements should be assumed:
 *	<DefaultLayout show_empty="false" inline="false" inline_limit="4" inline-header="true"
 *	inline_alias="false"><Merge type="menus"/><Merge type="files"/></DefaultLayout>
 */
void
xdg_defaultlayout_start_element(GMarkupParseContext *context, const gchar *element_name,
				const gchar **attribute_names, const gchar **attribute_values,
				gpointer user_data, GError **error)
{
	XdgMenu *menu;
	XdgLayout *layout;
	const gchar **name, **valu;

	if (!(menu = xdg_get_menu(user_data, element_name)))
		return;
	if (menu->deflay) {
		g_list_free_full(menu->deflay, xdg_layout_free);
		menu->deflay = NULL;
	}
	layout = calloc(1, sizeof(*layout));
	layout->type = LayoutTypeDefault;
	layout->show_empty = FALSE;
	layout->menu_inline = FALSE;
	layout->inline_limit = 4;
	layout->inline_header = TRUE;
	layout->inline_alias = FALSE;
	for (name = attribute_names, valu = attribute_values; name && *name; name++, valu++) {
		if (!strcmp(*name, "show_empty")) {
			if (!strcasecmp(*valu, "true"))
				layout->show_empty = TRUE;
		} else if (!strcmp(*name, "inline")) {
			if (!strcasecmp(*valu, "true"))
				layout->menu_inline = TRUE;
		} else if (!strcmp(*name, "inline_limit")) {
			layout->inline_limit = strtoul(*valu, NULL, 0);
		} else if (!strcmp(*name, "inline_header")) {
			if (!strcasecmp(*valu, "false"))
				layout->inline_header = FALSE;
		} else if (!strcmp(*name, "inline_alias")) {
			if (!strcasecmp(*valu, "true"))
				layout->menu_inline = TRUE;
		} else {
			WPRINTF("Unrecognized <%s> attribute '%s'\n", element_name, *name);
		}
	}
	menu->deflay = g_list_append(menu->deflay, layout);
}

static void
xdg_defaultlayout_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_defaultlayout_parser = {
	.start_element = xdg_defaultlayout_start_element,
	.error = xdg_defaultlayout_error,
};

/*
 * <Menuname [show_empty="..."] [inline="..."] [inline_limit="..."] [inline_header=",.."]
 * [inline_alias="..."]>
 *	This element may only appear as a chiled of a <Layout> or <DefaultLayout> menu.  Its
 *	contents references an immediate sub-menu of the curren menu as dfeined with the <Menu>
 *	lement, as such it should enver contain a slash.  If no such sub-menu exists the element
 *	should be ignored.  This element may have various attributes, the default values are taken
 *	from the DefaultLayout keyu.  The show_empty attribute defines whether a menu that contains
 *	no desktop entries and not sub-menus should eb shown at all.  THe show_empty attribute can
 *	be "true" or "false".  If the inline attribute is "true" the menu that is reference may be
 *	copied into the current menu at the current point instead of being instreted as a sub-menu
 *	of the current menu.  The optional inline_limit attribute defines the maximum number of
 *	entries that can be inlined.  If the inline_limit is 0 (zero) there is not limit.  The
 *	optional inline_header attribute defines whether an inlined menu should be preceded with a
 *	header entry listing the caption of the sub-menu.  The inline_header attribute can be either
 *	"true" or "false".  The optional inline_alias attribute defines whether a single inlined
 *	entry should adopt the capion of the inlined menu.  In such case no additional header entry
 *	will be added regardless of the value fo the inline_header attribute.  The inline_alias
 *	attribute can be either "true" or "false".  Example: if a menu has a sub-menu title
 *	"WordProcessor" with a single entry "OpenOffice 4.2", and both inline="true" and
 *	inline_alias="true" are specified then this sould result in teh "OpenOffice 4.2" entry being
 *	inlined in the current menu but the "OpenOffice 4.2" caption of the entry would be repalced
 *	with "WordProcessor".
 */
void
xdg_menuname_start_element(GMarkupParseContext *context, const gchar *element_name,
			   const gchar **attribute_names, const gchar **attribute_values,
			   gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;
	XdgMenu *menu;
	XdgLayout *layout;
	GList **list;
	const gchar **name, **valu;

	if (!base->node || !(menu = base->node->data)) {
		EPRINTF("Element <%s> must be within <Menu>!\n", element_name);
		return;
	}
	m = g_queue_peek_tail(base->elements);
	if (!strcmp(m->label, "Layout")) {
		list = &menu->layout;
	} else if (!strcmp(m->label, "DefaultLayout")) {
		list = &menu->deflay;
	} else {
		EPRINTF
		    ("Element <%s> can only occur directly beneath <Layout> or <DefaultLayout>!\n",
		     element_name);
		return;
	}
	layout = calloc(1, sizeof(*layout));
	layout->type = LayoutTypeMenuname;
	layout->show_empty = FALSE;
	layout->menu_inline = FALSE;
	layout->inline_limit = 4;
	layout->inline_header = TRUE;
	layout->inline_alias = FALSE;
	for (name = attribute_names, valu = attribute_values; name && *name; name++, valu++) {
		if (!strcmp(*name, "show_empty")) {
			if (!strcasecmp(*valu, "true"))
				layout->show_empty = TRUE;
		} else if (!strcmp(*name, "inline")) {
			if (!strcasecmp(*valu, "true"))
				layout->menu_inline = TRUE;
		} else if (!strcmp(*name, "inline_limit")) {
			layout->inline_limit = strtoul(*valu, NULL, 0);
		} else if (!strcmp(*name, "inline_header")) {
			if (!strcasecmp(*valu, "false"))
				layout->inline_header = FALSE;
		} else if (!strcmp(*name, "inline_alias")) {
			if (!strcasecmp(*valu, "true"))
				layout->menu_inline = TRUE;
		} else {
			WPRINTF("Unrecognized <%s> attribute '%s'\n", element_name, *name);
		}
	}
	*list = g_list_append(*list, layout);
}

static void
xdg_menuname_character_data(GMarkupParseContext *context,
			    const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const char *element_name = "Menuname";
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;
	XdgMenu *menu;
	XdgLayout *layout;
	GList *item;
	const gchar *tend = text + text_len;

	if (!base->node || !(menu = base->node->data)) {
		EPRINTF("Element <%s> must be within <Menu>!\n", element_name);
		return;
	}
	m = g_queue_peek_tail(base->elements);
	if (!strcmp(m->label, "Layout")) {
		item = g_list_last(menu->layout);
	} else if (!strcmp(m->label, "DefaultLayout")) {
		item = g_list_last(menu->deflay);
	} else {
		EPRINTF
		    ("Element <%s> can only occur directly beneath <Layout> or <DefaultLayout>!\n",
		     element_name);
		return;
	}
	for (; text < tend && isspace(*text); text++, text_len--) ;
	for (; tend > text && isspace(*tend); tend--, text_len--) ;
	layout = item->data;
	free(layout->string);
	layout->string = strndup(text, text_len);
}

static void
xdg_menuname_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_menuname_parser = {
	.start_element = xdg_menuname_start_element,
	.text = xdg_menuname_character_data,
	.error = xdg_menuname_error,
};

/*
 * <Separator>
 *	This element may only appear as a childe of a <Layout> or <DefaultLayout> menu.  It
 *	indicates a suggestion to draw a visual sewparator at this point in the menu.  <Separator>
 *	elements may be ignored.
 */
static void
xdg_separator_end_element(GMarkupParseContext *context,
			  const gchar *element_name, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;
	XdgMenu *menu;
	XdgLayout *layout;
	GList **list;

	if (!base->node || !(menu = base->node->data)) {
		EPRINTF("Element <%s> must be within <Menu>!\n", element_name);
		return;
	}
	m = g_queue_peek_tail(base->elements);
	if (!strcmp(m->label, "Layout")) {
		list = &menu->layout;
	} else if (!strcmp(m->label, "DefaultLayout")) {
		list = &menu->deflay;
	} else {
		EPRINTF
		    ("Element <%s> can only occur directly beneath <Layout> or <DefaultLayout>!\n",
		     element_name);
		return;
	}
	layout = calloc(1, sizeof(*layout));
	layout->type = LayoutTypeSeparator;
	*list = g_list_append(*list, layout);
}

static void
xdg_separator_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_separator_parser = {
	.end_element = xdg_separator_end_element,
	.error = xdg_separator_error,
};

/*
 * <Merge type="menus"|"files"|"all"/>
 *	This element may only appear as a child of a <Layout> or <DefaultLayout> menu.  It indicates
 *	the point where desktop entries and sub-menus that are not explicitly mentioned within the
 *	<Layout> or <DefaultLayout> element are to be inserted.  It has a type attribute that
 *	indicates which elements should be inserted in alphabetical order of their visual caption at
 *	this point.  type="files" means that all desktop entries contained in this menu that are not
 *	explicitly mentioned shoudl ne inserted in alphabetical order or their visual caption at
 *	this point.  Each <Layout> or <DefaultLayout> element shall have exactly one <Merge
 *	type="kall"> element or it shall have exactly one <Merge type="files"> and exactly one
 *	<Merge type="menus"> element.  An exception is made for a completely empty <Layout> element
 *	which may be used to indicate that the default-layout should eb used instead.
 */
void
xdg_merge_start_element(GMarkupParseContext *context, const gchar *element_name,
			const gchar **attribute_names, const gchar **attribute_values,
			gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;
	XdgMenu *menu;
	XdgLayout *layout;
	GList **list;
	const gchar **name, **valu;

	if (!base->node || !(menu = base->node->data)) {
		EPRINTF("Element <%s> must be within <Menu>!\n", element_name);
		return;
	}
	m = g_queue_peek_tail(base->elements);
	if (!strcmp(m->label, "Layout")) {
		list = &menu->layout;
	} else if (!strcmp(m->label, "DefaultLayout")) {
		list = &menu->deflay;
	} else {
		EPRINTF
		    ("Element <%s> can only occur directly beneath <Layout> or <DefaultLayout>!\n",
		     element_name);
		return;
	}
	layout = calloc(1, sizeof(*layout));
	layout->type = LayoutTypeMergeAll;
	for (name = attribute_names, valu = attribute_values; name && *name; name++, valu++) {
		if (!strcmp(*name, "type")) {
			if (!strcasecmp(*valu, "all"))
				layout->type = LayoutTypeMergeAll;
			else if (!strcasecmp(*valu, "files"))
				layout->type = LayoutTypeMergeFiles;
			else if (!strcasecmp(*valu, "menus"))
				layout->type = LayoutTypeMergeMenus;
			else
				WPRINTF("Unrecognized attribute value '%s'\n", *valu);
		} else
			WPRINTF("Unrecognized <%s> attribute '%s'\n", element_name, *name);
	}
	*list = g_list_append(*list, layout);
}

static void
xdg_merge_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
}

GMarkupParser xdg_merge_parser = {
	.start_element = xdg_merge_start_element,
	.error = xdg_merge_error,
};

XdgParserMapping mapping[] = {
	/* *INDENT-OFF* */
	{ "Menu",			&xdg_menu_parser		},
	{ "AppDir",			&xdg_appdir_parser		},
	{ "DefaultAppDirs",		&xdg_appdirs_parser		},
	{ "DirectoryDir",		&xdg_dirdir_parser		},
	{ "DefaultDirectoryDirs",	&xdg_dirdirs_parser		},
	{ "Name",			&xdg_name_parser		},
	{ "Directory",			&xdg_dir_parser			},
	{ "OnlyUnallocated",		&xdg_only_u_parser		},
	{ "NotOnlyUnallocated",		&xdg_not_u_parser		},
	{ "Deleted",			&xdg_deleted_parser		},
	{ "NotDeleted",			&xdg_not_deleted_parser		},
	{ "Include",			&xdg_include_parser		},
	{ "Exclude",			&xdg_exclude_parser		},
	{ "Filename",			&xdg_filename_parser		},
	{ "Category",			&xdg_category_parser		},
	{ "All",			&xdg_all_parser			},
	{ "And",			&xdg_and_parser			},
	{ "Or",				&xdg_or_parser			},
	{ "Not",			&xdg_not_parser			},
	{ "MergeFile",			&xdg_mergefile_parser		},
	{ "MergeDir",			&xdg_mergedir_parser		},
	{ "DefaultMergeDirs",		&xdg_mergedirs_parser		},
	{ "LegacyDir",			&xdg_legacydir_parser		},
	{ "KDELegacyDirs",		&xdg_kdedirs_parser		},
	{ "Move",			&xdg_move_parser		},
	{ "Old",			&xdg_old_parser			},
	{ "New",			&xdg_new_parser			},
	{ "Layout",			&xdg_layout_parser		},
	{ "DefaultLayout",		&xdg_defaultlayout_parser	},
	{ "Menuname",			&xdg_menuname_parser		},
	{ "Separator",			&xdg_separator_parser		},
	{ "Merge",			&xdg_merge_parser		},
	{ NULL,				NULL				}
	/* *INDENT-ON* */
};

static void
xdg_start_element(GMarkupParseContext *context,
		  const gchar *element_name,
		  const gchar **attribute_names,
		  const gchar **attribute_values, gpointer user_data, GError **error)
{
	XdgParserMapping *m;
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;

	DPRINTF("START-ELEMENT: %s\n", element_name);

	for (m = &mapping[0]; m->label; m++) {
		if (!strcmp(element_name, m->label)) {
			if (base->current)
				g_queue_push_tail(base->elements, base->current);
			base->current = m;
			break;
		}
	}
	if (m->label && m->parser->start_element)
		m->parser->start_element(context, element_name, attribute_names,
					 attribute_values, user_data, error);
}

static void
xdg_character_data(GMarkupParseContext *context,
		   const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;

	m = base->current;
	DPRINTF("CHARACTER-DATA: %s\n", m->label);
	if (m->parser->text)
		m->parser->text(context, text, text_len, user_data, error);
}

static void
xdg_end_element(GMarkupParseContext *context,
		const gchar *element_name, gpointer user_data, GError **error)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;

	DPRINTF("END-ELEMENT: %s\n", element_name);

	m = base->current;
	if (m->parser->end_element)
		m->parser->end_element(context, element_name, user_data, error);
	base->current = g_queue_pop_tail(base->elements);
}

static void
xdg_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
	XdgContext *ctx = user_data;
	XdgParseContext *base = ctx->parser;
	XdgParserMapping *m;

	m = base->current;
	DPRINTF("PARSE-ERROR: %s\n", m->label);
	if (m->parser->error)
		m->parser->error(context, error, user_data);
}

GMarkupParser xdg_parser = {
	.start_element = xdg_start_element,
	.end_element = xdg_end_element,
	.text = xdg_character_data,
	.error = xdg_error,
};

static GNode *
parse_menu(XdgContext *context, const char *path, GError **error)
{
	XdgParseContext *base = NULL;
	GNode *tree = NULL;
	FILE *f;
	int dummy;
	struct stat st;
	char *s, *p;
	GError *tmperr = NULL;

	if (!(f = fopen(path, "r"))) {
		EPRINTF("cannot open file: '%s'\n", path);
		return (tree);
	}
	DPRINTF("locking file '%s'\n", path);

	dummy = lockf(fileno(f), F_LOCK, 0);
	if (fstat(fileno(f), &st)) {
		EPRINTF("cannot stat open file: '%s'\n", path);
		goto no_stat;
	}
	if (st.st_size > 0) {
		gchar buf[BUFSIZ];
		gsize got;

		if (!(base = calloc(1, sizeof(*base)))) {
			EPRINTF("%s\n", strerror(errno));
			goto no_ctx;
		}
		base->directory = s = strdup(path);
		if ((p = strrchr(s, '/')))
			*p = '\0';
		base->filename = strdup((p = strrchr(path, '/')) ? p + 1 : path);
		base->elements = g_queue_new();
		base->stack = g_queue_new();
		base->rules = g_queue_new();
		if (context->parser)
			g_queue_push_tail(context->parsers, context->parser);
		context->parser = base;
		if (!(base->ctx = g_markup_parse_context_new(&xdg_parser,
							     G_MARKUP_TREAT_CDATA_AS_TEXT |
							     G_MARKUP_PREFIX_ERROR_POSITION,
							     context, NULL))) {
			EPRINTF("cannot create XML parser\n");
			goto no_tree;
		}
		while ((got = fread(buf, 1, BUFSIZ, f)) > 0) {
			DPRINTF("got %d more bytes\n", got);
			if (!g_markup_parse_context_parse(base->ctx, buf, got, &tmperr)) {
				EPRINTF("could not parse buffer contents\n");
				goto parse_error;
			}
		}
		if (!g_markup_parse_context_end_parse(base->ctx, &tmperr)) {
			EPRINTF("could not end parsing\n");
			goto parse_error;
		}
		g_markup_parse_context_unref(base->ctx);
		base->ctx = NULL;
	} else
		EPRINTF("file is empty: '%s'\n", path);

	(void) dummy;
      parse_error:
	if (tmperr)
		g_propagate_error(error, tmperr);
	tree = base->tree;
	base->tree = NULL;
      no_tree:
	context->parser = g_queue_pop_tail(context->parsers);
	DPRINTF("Freeing parse context.\n");
	xdg_context_free(base);
      no_ctx:
      no_stat:
	DPRINTF("unlocking file '%s'\n", path);
	dummy = lockf(fileno(f), F_ULOCK, 0);
	fclose(f);
	return (tree);

}

static GNode *
root_menu(XdgContext *context, GError **error)
{
	GError *tmperr = NULL;
	GNode *tree;

	tree = parse_menu(context, options.rootmenu, &tmperr);
	if (tmperr)
		g_propagate_error(error, tmperr);
	return (tree);
}

static void
make_menu(int argc, char *argv[])
{
	XdgContext context = { NULL, };
	GNode *tree;
	GError *error = NULL;

	if (!xdg_directory_cache)
		xdg_directory_cache =
		    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, xdg_directory_free);
	if (!xdg_fileentry_cache)
		xdg_fileentry_cache =
		    g_hash_table_new_full(g_str_hash, g_str_equal, NULL, xdg_fileentry_free);

	context.parsers = g_queue_new();
	tree = root_menu(&context, &error);
	(void) tree;
	/* FIXME: now do something with root */
}

static Window
get_selection(Bool replace, Window selwin)
{
	char selection[64] = { 0, };
	GdkDisplay *disp;
	Display *dpy;
	int s, nscr;
	Atom atom;
	Window owner, gotone = None;

	disp = gdk_display_get_default();
	nscr = gdk_display_get_n_screens(disp);

	dpy = GDK_DISPLAY_XDISPLAY(disp);

	for (s = 0; s < nscr; s++) {
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

static Bool
good_window_manager(XdeScreen *xscr)
{
	/* ignore non fully compliant names */
	if (!xscr->wmname)
		return False;
	if (!strcasecmp(xscr->wmname, "fluxbox"))
		return True;
	if (!strcasecmp(xscr->wmname, "blackbox"))
		return True;
	if (!strcasecmp(xscr->wmname, "openbox"))
		return True;
	if (!strcasecmp(xscr->wmname, "icewm"))
		return True;
	if (!strcasecmp(xscr->wmname, "pekwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "jwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "fvwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "wmaker"))
		return True;
	if (!strcasecmp(xscr->wmname, "ctwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "vtwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "twm"))
		return True;
	if (!strcasecmp(xscr->wmname, "uwm"))
		return True;
	if (!strcasecmp(xscr->wmname, "waimea"))
		return True;
	return False;
}

static void
window_manager_changed(WnckScreen *wnck, gpointer user)
{
	XdeScreen *xscr = user;
	const char *name;

	if (!xscr) {
		EPRINTF("xscr is NULL\n");
		exit(EXIT_FAILURE);
	}
	wnck_screen_force_update(wnck);
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

static void
init_wnck(XdeScreen *xscr)
{
	WnckScreen *wnck = xscr->wnck = wnck_screen_get(xscr->index);

	g_signal_connect(G_OBJECT(wnck), "window_manager_changed",
			 G_CALLBACK(window_manager_changed), xscr);

	window_manager_changed(wnck, xscr);
}

static GdkFilterReturn selwin_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);
static GdkFilterReturn root_handler(GdkXEvent *xevent, GdkEvent *event, gpointer data);
static void update_theme(XdeScreen *xscr, Atom prop);
static void update_icon_theme(XdeScreen *xscr, Atom prop);

static void
do_generate(int argc, char *argv[])
{
	Window owner;

	if ((owner = get_selection(False, None))) {
		EPRINTF("%s: an instance 0x%08lx is running\n", argv[0], owner);
		exit(EXIT_FAILURE);
	}
	make_menu(argc, argv);
}

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
		gdk_window_add_filter(xscr->root, root_handler, xscr);
		init_wnck(xscr);
		update_theme(xscr, None);
		update_icon_theme(xscr, None);
	}
	make_menu(argc, argv);
	gtk_main();
}

static void
do_quit(int argc, char *argv[])
{
	get_selection(True, None);
}

static void
update_theme(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	XTextProperty xtp = { NULL, };
	Bool changed = False;
	GtkSettings *set;

	gtk_rc_reparse_all();
	if (!prop || prop == _XA_GTK_READ_RCFILES)
		prop = _XA_XDE_THEME_NAME;
	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				static const char *prefix = "gtk-theme-name=\"";
				static const char *suffix = "\"";
				char *rc_string;
				int len;

				len = strlen(prefix) + strlen(list[0]) + strlen(suffix);
				rc_string = calloc(len + 1, sizeof(*rc_string));
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
		DPRINTF("could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
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
update_icon_theme(XdeScreen *xscr, Atom prop)
{
	Display *dpy = GDK_DISPLAY_XDISPLAY(xscr->disp);
	Window root = RootWindow(dpy, xscr->index);
	XTextProperty xtp = { NULL, };
	Bool changed = False;
	GtkSettings *set;

	gtk_rc_reparse_all();
	if (!prop || prop == _XA_GTK_READ_RCFILES)
		prop = _XA_XDE_ICON_THEME_NAME;
	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				static const char *prefix = "gtk-icon-theme-name=\"";
				static const char *suffix = "\"";
				char *rc_string;
				int len;

				len = strlen(prefix) + strlen(list[0]) + strlen(suffix);
				rc_string = calloc(len + 1, sizeof(*rc_string));
				strncpy(rc_string, prefix, len);
				strncat(rc_string, list[0], len);
				strncat(rc_string, suffix, len);
				gtk_rc_parse_string(rc_string);
				free(rc_string);
				if (!xscr->itheme || strcmp(xscr->itheme, list[0])) {
					free(xscr->itheme);
					xscr->itheme = strdup(list[0]);
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
		DPRINTF("could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
	if ((set = gtk_settings_get_for_screen(xscr->scrn))) {
		GValue theme_v = G_VALUE_INIT;
		const char *itheme;

		g_value_init(&theme_v, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(set), "gtk-icon-theme-name", &theme_v);
		itheme = g_value_get_string(&theme_v);
		if (itheme && (!xscr->itheme || strcmp(xscr->itheme, itheme))) {
			free(xscr->itheme);
			xscr->itheme = strdup(itheme);
			changed = True;
		}
		g_value_unset(&theme_v);
	}
	if (changed) {
		DPRINTF("New icon theme is %s\n", xscr->itheme);
		/* FIXME: do something more about it. */
	}
}

static GdkFilterReturn
event_handler_PropertyNotify(Display *dpy, XEvent *xev, XdeScreen *xscr)
{
	Atom atom;

	if (options.debug > 2) {
		fprintf(stderr, "==> PropertyNotify:\n");
		fprintf(stderr, "    --> window = 0x%08lx\n", xev->xproperty.window);
		fprintf(stderr, "    --> atom = %s\n", XGetAtomName(dpy, xev->xproperty.atom));
		fprintf(stderr, "    --> time = %ld\n", xev->xproperty.time);
		fprintf(stderr, "    --> state = %s\n",
			(xev->xproperty.state == PropertyNewValue) ? "NewValue" : "Delete");
		fprintf(stderr, "<== PropertyNotify:\n");
	}
	atom = xev->xproperty.atom;

	if (xev->xproperty.state == PropertyNewValue) {
		if (atom == _XA_XDE_THEME_NAME || atom == _XA_XDE_WM_THEME) {
			update_theme(xscr, xev->xproperty.atom);
			return GDK_FILTER_REMOVE;
		} else if (atom == _XA_XDE_ICON_THEME_NAME || atom == _XA_XDE_WM_ICONTHEME) {
			update_icon_theme(xscr, xev->xproperty.atom);
			return GDK_FILTER_REMOVE;
		}
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
event_handler_ClientMessage(Display *dpy, XEvent *xev)
{
	XdeScreen *xscr = NULL;
	int s, nscr = ScreenCount(dpy);

	for (s = 0; s < nscr; s++)
		if (xev->xclient.window == RootWindow(dpy, s))
			xscr = screens + s;
	if (options.debug) {
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
				fprintf(stderr, " %02x", (int) xev->xclient.data.b[i]);
			fprintf(stderr, "\n");
			break;
		case 16:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 10; i++)
				fprintf(stderr, " %04x", (int) xev->xclient.data.s[i]);
			fprintf(stderr, "\n");
			break;
		case 32:
			fprintf(stderr, "    --> data =");
			for (i = 0; i < 20; i++)
				fprintf(stderr, " %08lx", xev->xclient.data.l[i]);
			fprintf(stderr, "\n");
			break;
		}
		fprintf(stderr, "<== ClientMessage: %p\n", xscr);
	}
	if (xscr && xev->xclient.message_type == _XA_GTK_READ_RCFILES) {
		update_theme(xscr, xev->xclient.message_type);
		update_icon_theme(xscr, xev->xclient.message_type);
		return GDK_FILTER_REMOVE;
	}
	return GDK_FILTER_CONTINUE;
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
	len = strlen(home) + strlen(suffix);
	file = calloc(len + 1, sizeof(*file));
	strncpy(file, home, len);
	strncat(file, suffix, len);
	gtk_rc_add_default_file(file);
	free(file);

	init_smclient();

	gtk_init(&argc, &argv);

	disp = gdk_display_get_default();
	dpy = GDK_DISPLAY_XDISPLAY(disp);

	atom = gdk_atom_intern_static_string("_XDE_WM_NAME");
	_XA_XDE_WM_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_MENU");
	_XA_XDE_WM_MENU = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_THEME");
	_XA_XDE_WM_THEME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_WM_ICONTHEME");
	_XA_XDE_WM_ICONTHEME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_THEME_NAME");
	_XA_XDE_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_XDE_ICON_THEME_NAME");
	_XA_XDE_ICON_THEME_NAME = gdk_x11_atom_to_xatom_for_display(disp, atom);

	atom = gdk_atom_intern_static_string("_GTK_READ_RCFILES");
	_XA_GTK_READ_RCFILES = gdk_x11_atom_to_xatom_for_display(disp, atom);
	gdk_display_add_client_message_filter(disp, atom, client_handler, dpy);

	atom = gdk_atom_intern_static_string("MANAGER");
	_XA_MANAGER = gdk_x11_atom_to_xatom_for_display(disp, atom);

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
    %1$s {-m|--monitor} [options]\n\
    %1$s {-R|--replace} [options]\n\
    %1$s {-q|--quit} [options]\n\
    %1$s {-h|--help} [options]\n\
    %1$s {-V|--version}\n\
    %1$s {-C|--copying}\n\
", argv[0]);
}

const char *
show_bool(Bool boolval)
{
	if (boolval)
		return ("true");
	return ("false");
}

const char *
show_style(Style style)
{
	switch (style) {
	case StyleFullmenu:
		return ("fullmenu");
	case StyleAppmenu:
		return ("appmenu");
	case StyleEntries:
		return ("entries");
	}
	return ("(unknown)");
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
        output file [default: %9$s]\n\
    -n, --noicons\n\
        do not place icons in menu [default: %10$s]\n\
    -t, --theme THEME\n\
        icon theme name to use [default: %11$s]\n\
    -L, --launch, --nolaunch\n\
        use xde-launch to launch programs [default: %12$s]\n\
    -s, --style STYLE\n\
        fullmenu, appmenu or entries [default: %13$s]\n\
    -M, --menu MENU\n\
        filename stem of root menu filename [default: %16$s]\n\
    -D, --debug [LEVEL]\n\
        increment or set debug LEVEL [default: %14$d]\n\
    -v, --verbose [LEVEL]\n\
        increment or set output verbosity LEVEL [default: %15$d]\n\
        this option may be repeated.\n\
", argv[0] 
	, defaults.format
	, show_style(defaults.style)
	, defaults.desktop
	, defaults.charset
	, defaults.language
	, defaults.rootmenu
	, show_bool(defaults.dieonerr)
	, defaults.filename
	, show_bool(defaults.noicons)
	, defaults.theme
	, show_bool(defaults.launch)
	, show_style(defaults.style)
	, defaults.debug
	, defaults.output
	, defaults.menu
);
	/* *INDENT-ON* */
}

static void
set_default_paths()
{
	char *env, *p, *e;
	int len;

	if ((env = getenv("XDG_DATA_HOME")))
		xdg_data_home = strdup(env);
	else {
		static const char *suffix = "/.local/share";

		env = getenv("HOME") ? : "~";
		len = strlen(env) + strlen(suffix) + 1;
		xdg_data_home = calloc(len, sizeof(*xdg_data_home));
		strcpy(xdg_data_home, env);
		strcat(xdg_data_home, suffix);
	}
	env = getenv("XDG_DATA_DIRS") ? : "/usr/local/share:/usr/share";
	xdg_data_dirs = strdup(env);
	len = strlen(xdg_data_home) + 1 + strlen(xdg_data_dirs) + 1;
	xdg_data_path = calloc(len, sizeof(*xdg_data_path));
	strcpy(xdg_data_path, xdg_data_home);
	strcat(xdg_data_path, ":");
	strcat(xdg_data_path, xdg_data_dirs);
	xdg_data_last = xdg_data_path + strlen(xdg_data_path);
	DPRINTF("Full data path is: '%s'\n", xdg_data_path);
	p = xdg_data_path;
	e = xdg_data_last;
	while ((p = strchrnul(p, ':')) < e)
		*p++ = '\0';
	DPRINTF("Directories in forward order:\n");
	for (p = xdg_data_path; p < xdg_data_last; p += strlen(p) + 1) {
		DPRINTF("\t%s\n", p);
	}
	DPRINTF("Directories in reverse order:\n");
	for (p = xdg_find_str(xdg_data_last, xdg_data_path);
	     p >= xdg_data_path; p = xdg_find_str(p - 1, xdg_data_path)) {
		DPRINTF("\t%s\n", p);
	}

	if ((env = getenv("XDG_CONFIG_HOME")))
		xdg_config_home = strdup(env);
	else {
		static const char *suffix = "/.config";

		env = getenv("HOME") ? : "~";
		len = strlen(env) + strlen(suffix) + 1;
		xdg_config_home = calloc(len, sizeof(*xdg_config_home));
		strcpy(xdg_config_home, env);
		strcat(xdg_config_home, suffix);
	}
	env = getenv("XDG_CONFIG_DIRS") ? : "/etc/xdg";
	xdg_config_dirs = strdup(env);
	len = strlen(xdg_config_home) + 1 + strlen(xdg_config_dirs) + 1;
	xdg_config_path = calloc(len, sizeof(*xdg_config_path));
	strcpy(xdg_config_path, xdg_config_home);
	strcat(xdg_config_path, ":");
	strcat(xdg_config_path, xdg_config_dirs);
	xdg_config_last = xdg_config_path + strlen(xdg_config_path);
	DPRINTF("Full config path is; '%s'\n", xdg_config_path);
	p = xdg_config_path;
	e = xdg_config_last;
	while ((p = strchrnul(p, ':')) < e)
		*p++ = '\0';
	DPRINTF("Directories in forward order:\n");
	for (p = xdg_config_path; p < xdg_config_last; p += strlen(p) + 1) {
		DPRINTF("\t%s\n", p);
	}
	DPRINTF("Directories in reverse order:\n");
	for (p = xdg_find_str(xdg_config_last, xdg_config_path);
	     p >= xdg_config_path; p = xdg_find_str(p - 1, xdg_config_path)) {
		DPRINTF("\t%s\n", p);
	}
}

static void
set_default_files()
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
		defaults.runhist = options.runhist = calloc(len, sizeof(*options.runhist));
		strcpy(options.runhist, env);
		strcat(options.runhist, rsuffix);

		len = strlen(env) + strlen(asuffix) + 1;
		free(options.recapps);
		defaults.recapps = options.recapps = calloc(len, sizeof(*options.recapps));
		strcpy(options.recapps, env);
		strcat(options.recapps, asuffix);

		len = strlen(env) + strlen(xsuffix) + 1;
		free(options.recently);
		defaults.recently = options.recently = calloc(len, sizeof(*options.recently));
		strcpy(options.recently, env);
		strcat(options.recently, xsuffix);
	} else {
		static const char *cfgdir = "/.config";
		static const char *datdir = "/.local/share";

		env = getenv("HOME") ? : ".";

		len = strlen(env) + strlen(cfgdir) + strlen(rsuffix) + 1;
		free(options.runhist);
		defaults.runhist = options.runhist = calloc(len, sizeof(*options.runhist));
		strcpy(options.runhist, env);
		strcat(options.runhist, cfgdir);
		strcat(options.runhist, rsuffix);

		len = strlen(env) + strlen(cfgdir) + strlen(asuffix) + 1;
		free(options.recapps);
		defaults.recapps = options.recapps = calloc(len, sizeof(*options.recapps));
		strcpy(options.recapps, env);
		strcat(options.recapps, cfgdir);
		strcat(options.recapps, asuffix);

		len = strlen(env) + strlen(datdir) + strlen(xsuffix) + 1;
		free(options.recently);
		defaults.recently = options.recently = calloc(len, sizeof(*options.recently));
		strcpy(options.recently, env);
		strcat(options.recently, datdir);
		strcat(options.recently, xsuffix);
	}
	if (access(options.recently, R_OK | W_OK)) {
		env = getenv("HOME") ? : ".";

		len = strlen(env) + strlen(hsuffix) + 1;
		free(options.recently);
		defaults.recently = options.recently = calloc(len, sizeof(*options.recently));
		strcpy(options.recently, env);
		strcat(options.recently, hsuffix);
	}
	return;
}

static void
set_defaults(void)
{
	char *env;

	if ((env = getenv("XDG_CURRENT_DESKTOP"))) {
		free(options.desktop);
		defaults.desktop = options.desktop = strdup(env);
	}

	set_default_paths();
	set_default_files();
}

static void
get_default_locale()
{
	char *val;
	int len, set;

	set = (options.charset || options.language);

	if (!options.charset && (val = nl_langinfo(CODESET)))
		defaults.charset = options.charset = strdup(val);
	if (!options.language && options.locale) {
		defaults.language = options.language = strdup(options.locale);
		*strchrnul(options.language, '.') = '\0';
	}
	if (set && options.language && options.charset) {
		len = strlen(options.language) + 1 + strlen(options.charset);
		val = calloc(len, sizeof(*val));
		strcpy(val, options.language);
		strcat(val, ".");
		strcat(val, options.charset);
		DPRINTF("setting locale to: '%s'\n", val);
		if (!setlocale(LC_ALL, val))
			EPRINTF("cannot set locale to '%s'\n", val);
		free(val);
	}
	DPRINTF("locale is '%s'\n", options.locale);
	DPRINTF("charset is '%s'\n", options.charset);
	DPRINTF("language is '%s'\n", options.language);
}

static void
get_default_format()
{
	GdkDisplay *disp = gdk_display_get_default();
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	GdkWindow *wind = gdk_screen_get_root_window(scrn);
	Window root = GDK_WINDOW_XID(wind);
	XTextProperty xtp = { NULL, };
	Atom prop = _XA_XDE_WM_NAME;

	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				if (!options.format || strcmp(options.format, list[0])) {
					free(options.format);
					defaults.format = options.format = strdup(list[0]);
				}
			}
			if (list)
				XFreeStringList(list);
		} else
			EPRINTF("could not get text list for %s property\n",
				XGetAtomName(dpy, prop));
	} else
		DPRINTF("could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
}

static void
get_default_output()
{
	GdkDisplay *disp = gdk_display_get_default();
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	GdkWindow *wind = gdk_screen_get_root_window(scrn);
	Window root = GDK_WINDOW_XID(wind);
	XTextProperty xtp = { NULL, };
	Atom prop = _XA_XDE_WM_MENU;

	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				if (!options.filename || strcmp(options.filename, list[0])) {
					free(options.filename);
					defaults.filename = options.filename = strdup(list[0]);
				}
			}
			if (list)
				XFreeStringList(list);
		} else
			EPRINTF("could not get text list for %s property\n",
				XGetAtomName(dpy, prop));
	} else
		DPRINTF("could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
}

static void
get_default_theme()
{
#if 1
	GdkDisplay *disp = gdk_display_get_default();
	Display *dpy = GDK_DISPLAY_XDISPLAY(disp);
	GdkScreen *scrn = gdk_display_get_default_screen(disp);
	GdkWindow *wind = gdk_screen_get_root_window(scrn);
	Window root = GDK_WINDOW_XID(wind);
	XTextProperty xtp = { NULL, };
	Atom prop = _XA_XDE_ICON_THEME_NAME;
	GtkSettings *set;

	gtk_rc_reparse_all();

	if (XGetTextProperty(dpy, root, &xtp, prop)) {
		char **list = NULL;
		int strings = 0;

		if (Xutf8TextPropertyToTextList(dpy, &xtp, &list, &strings) == Success) {
			if (strings >= 1) {
				static const char *prefix = "gtk-icon-theme-name=\"";
				static const char *suffix = "\"";
				char *rc_string;
				int len;

				len = strlen(prefix) + strlen(list[0]) + strlen(suffix);
				rc_string = calloc(len + 1, sizeof(*rc_string));
				strncpy(rc_string, prefix, len);
				strncat(rc_string, list[0], len);
				strncat(rc_string, suffix, len);
				gtk_rc_parse_string(rc_string);
				free(rc_string);
				if (!options.theme || strcmp(options.theme, list[0])) {
					free(options.theme);
					defaults.theme = options.theme = strdup(list[0]);
				}
			}
			if (list)
				XFreeStringList(list);
		} else
			EPRINTF("could not get text list for %s property\n",
				XGetAtomName(dpy, prop));
		if (xtp.value)
			XFree(xtp.value);
	} else
		DPRINTF("could not get %s for root 0x%lx\n", XGetAtomName(dpy, prop), root);
	if ((set = gtk_settings_get_for_screen(scrn))) {
		GValue theme_v = G_VALUE_INIT;
		const char *itheme;

		g_value_init(&theme_v, G_TYPE_STRING);
		g_object_get_property(G_OBJECT(set), "gtk-icon-theme-name", &theme_v);
		itheme = g_value_get_string(&theme_v);
		if (itheme && (!options.theme || strcmp(options.theme, itheme))) {
			free(options.theme);
			defaults.theme = options.theme = strdup(itheme);
		}
		g_value_unset(&theme_v);
	}
#else

	static const char *suffix1 = "/etc/gtk-2.0/gtkrc";
	static const char *suffix2 = "/.gtkrc-2.0";
	static const char *suffix3 = "/.gtkrc-2.0.xde";
	static const char *prefix = "gtk-icon-theme-name=\"";
	char *env;
	int len;

	if (options.theme)
		return;
	if ((env = getenv("XDG_ICON_THEME"))) {
		free(options.theme);
		defaults.theme = options.theme = strdup(env);
		return;
	} else {
		char *path, *d, *e, *buf;

		/* need to go looking for the theme name */

		if ((env = getenv("GTK_RC_FILES"))) {
			path = strdup(env);
		} else {
			env = getenv("HOME") ? : "~";
			len = 1 + strlen(suffix1) + 1 + strlen(env) + strlen(suffix2) +
			    1 + strlen(env) + strlen(suffix3);
			path = calloc(len, sizeof(*path));
			strcpy(path, suffix1);
			strcat(path, ":");
			strcat(path, env);
			strcat(path, suffix2);
			strcat(path, ":");
			strcat(path, env);
			strcat(path, suffix3);
		}
		buf = calloc(BUFSIZ, sizeof(*buf));
		d = path;
		e = d + strlen(d);
		while ((d = strchrnul(d, ':')) < e)
			*d++ = '\0';
		for (d = path; d < e; d += strlen(d) + 1) {
			FILE *f;

			if (!(f = fopen(d, "r")))
				continue;
			while (fgets(buf, BUFSIZ, f)) {
				if (!strncmp(buf, prefix, strlen(prefix))) {
					char *p;

					p = buf + strlen(prefix);
					*strchrnul(p, '"') = '\0';
					free(options.theme);
					defaults.theme = options.theme = strdup(p);
				}
				/* FIXME: this does not traverse "include" statements */
			}
			fclose(f);
		}
		free(buf);
	}
	if (!options.theme)
		defaults.theme = options.theme = strdup("hicolor");
#endif
}

static void
get_default_root()
{
	char *env, *dirs, *pfx, *p, *q, *d, *e;

	if (!options.menu) {
		free(options.menu);
		defaults.menu = options.menu = strdup("applications");
	}

	if (options.rootmenu)
		return;

	dirs = calloc(PATH_MAX, sizeof(*dirs));

	if ((env = getenv("XDG_CONFIG_HOME"))) {
		strcpy(dirs, env);
	} else if ((env = getenv("HOME"))) {
		strcpy(dirs, env);
		strcat(dirs, "/.config");
	} else {
		strcpy(dirs, "~/.config");
	}
	if ((env = getenv("XDG_CONFIG_DIRS"))) {
		strcat(dirs, ":");
		strcat(dirs, env);
	} else {
		strcat(dirs, ":");
		strcat(dirs, "/etc/xdg");
	}
	DPRINTF("$XDG_CONFIG_HOME:$XDG_CONFIG_DIRS is '%s'\n", dirs);
	e = dirs + strlen(dirs);
	d = dirs;
	while ((d = strchrnul(d, ':')) < e)
		*d++ = '\0';

	pfx = calloc(PATH_MAX, sizeof(*dirs));

	if ((env = getenv("XDG_VENDOR_ID"))) {
		if (pfx[0])
			strcat(pfx, ":");
		strcat(pfx, env);
		strcat(pfx, "-");
	}
	if ((env = getenv("XDG_MENU_PREFIX"))) {
		strcat(pfx, env);
	}
	if (pfx[0])
		strcat(pfx, ":");
	strcat(pfx, "xde-");
	if (pfx[0])
		strcat(pfx, ":");
	strcat(pfx, "arch-");
	DPRINTF("$XDG_MENU_PREFIX:$XDG_VENDOR_ID- is '%s'\n", pfx);
	q = pfx + strlen(pfx);
	p = pfx;
	while ((p = strchrnul(p, ':')) < q)
		*p++ = '\0';

	for (p = pfx; p < q; p += strlen(p) + 1) {
		for (d = dirs; d < e; d += strlen(d) + 1) {
			int len;
			char *path;

			len = strlen(d) + strlen("/menus/") + strlen(p) + strlen(options.menu)
			    + strlen(".menu") + 1;
			path = calloc(len, sizeof(*path));
			strcpy(path, d);
			strcat(path, "/menus/");
			strcat(path, p);
			strcat(path, options.menu);
			strcat(path, ".menu");
			DPRINTF("Testing path: '%s'\n", path);
			if (access(path, R_OK) == 0) {
				free(options.rootmenu);
				defaults.rootmenu = options.rootmenu = path;
				break;
			}
			free(path);
		}
		if (options.rootmenu)
			break;
	}
	DPRINTF("Default root menu is: '%s'\n", options.rootmenu);
	free(dirs);
	free(pfx);
}

static void
get_defaults(void)
{
	get_default_locale();
	get_default_root();
}

int
main(int argc, char *argv[])
{
	Command command = CommandDefault;
	char *loc;

	if ((loc = setlocale(LC_ALL, ""))) {
		free(options.locale);
		defaults.locale = options.locale = strdup(loc);
	}
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
			{"nofullmenu",	no_argument,		NULL,	'N'},
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
			{"menu",	required_argument,	NULL,	'M'},

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

		c = getopt_long_only(argc, argv, "f:FNd:c:l:r:eo::nt:mL0s:M:qRD::v::hVCH?",
				     long_options, &option_index);
#else
		c = getopt(argc, argv, "f:FNd:c:l:r:eo:nt:mL0s:M:qRD:vhVC?");
#endif
		if (c == -1) {
			if (options.debug)
				fprintf(stderr, "%s: done options processing\n", argv[0]);
			break;
		}
		switch (c) {
		case 0:
			goto bad_usage;

		case 'f':	/* --format, -f FORMAT */
			free(options.format);
			defaults.format = options.format = strdup(optarg);
			break;
		case 'F':	/* -F, --fullmenu */
			defaults.style = options.style = StyleFullmenu;
			break;
		case 'N':	/* -N, --nofullmenu */
			defaults.style = options.style = StyleAppmenu;
			break;
		case 'd':	/* -d, --desktop DESKTOP */
			free(options.desktop);
			defaults.desktop = options.desktop = strdup(optarg);
			break;
		case 'c':	/* -c, --charset CHARSET */
			free(options.charset);
			defaults.charset = options.charset = strdup(optarg);
			break;
		case 'l':	/* -l, --language LANGUAGE */
			free(options.language);
			defaults.language = options.language = strdup(optarg);
			break;
		case 'r':	/* -r, --root-menu MENU */
			free(options.rootmenu);
			defaults.rootmenu = options.rootmenu = strdup(optarg);
			break;
		case 'e':	/* -e, --die-on-error */
			defaults.dieonerr = options.dieonerr = True;
			break;
		case 'o':	/* -o, --output [OUTPUT] */
			defaults.fileout = options.fileout = True;
			if (optarg != NULL) {
				free(options.rootmenu);
				defaults.rootmenu = options.rootmenu = strdup(optarg);
			}
			break;
		case 'n':	/* -n, --noicons */
			defaults.noicons = options.noicons = True;
			break;
		case 't':	/* -t, --theme THEME */
			free(options.theme);
			defaults.theme = options.theme = strdup(optarg);
			break;
		case 'm':	/* -m, --monitor */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandRun;
			defaults.command = options.command = CommandRun;
			break;
		case 'L':	/* -L, --launch */
			defaults.launch = options.launch = True;
			break;
		case '0':	/* -0, --nolaunch */
			defaults.launch = options.launch = False;
			break;
		case 's':	/* -s, --style STYLE */
			if (!strncmp("fullmenu", optarg, strlen(optarg))) {
				defaults.style = options.style = StyleFullmenu;
				break;
			}
			if (!strncmp("appmenu", optarg, strlen(optarg))) {
				defaults.style = options.style = StyleAppmenu;
				break;
			}
			if (!strncmp("entries", optarg, strlen(optarg))) {
				defaults.style = options.style = StyleEntries;
				break;
			}
			goto bad_option;
		case 'M':	/* -M, --menu MENUS */
			free(options.menu);
			defaults.menu = options.menu = strdup(optarg);
			break;

		case 'q':	/* -q, --quit */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandQuit;
			defaults.command = options.command = CommandQuit;
			break;
		case 'R':	/* -R, --replace */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandReplace;
			defaults.command = options.command = CommandReplace;
			break;

		case 'D':	/* -D, --debug [LEVEL] */
			if (options.debug)
				fprintf(stderr, "%s: increasing debug verbosity\n", argv[0]);
			if (optarg == NULL)
				defaults.debug = options.debug = options.debug + 1;
			else {
				if ((val = strtol(optarg, NULL, 0)) < 0)
					goto bad_option;
				defaults.debug = options.debug = val;
			}
			break;
		case 'v':	/* -v, --verbose [LEVEL] */
			if (options.debug)
				fprintf(stderr, "%s: increasing output verbosity\n", argv[0]);
			if (optarg == NULL)
				defaults.output = options.output = options.output + 1;
			else {
				if ((val = strtol(optarg, NULL, 0)) < 0)
					goto bad_option;
				defaults.output = options.output = val;
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
			defaults.command = options.command = CommandVersion;
			break;
		case 'C':	/* -C, --copying */
			if (options.command != CommandDefault)
				goto bad_option;
			if (command == CommandDefault)
				command = CommandCopying;
			defaults.command = options.command = CommandCopying;
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

	if (!options.format)
		get_default_format();
	if (!options.filename)
		get_default_output();
	if (!options.theme)
		get_default_theme();

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

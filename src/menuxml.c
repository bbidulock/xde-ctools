
#include "menuxml.h"

Element elements[] = {
	/* *INDENT-OFF* */
	/* Element name			context			interior		nest	cdata	beg				dat				end */
	{ "Menu",			MenuContextMenu,	MenuContextMenu,	TRUE,	FALSE,	&beg_Menu,			&dat_Menu,			&end_Menu			},
	{ "AppDir",			MenuContextMenu,	MenuContextNone,	FALSE,	TRUE,	&beg_AppDir,			&dat_AppDir,			&end_AppDir			},
	{ "DefaultAppDirs",		MenuContextMenu,	MenuContextNone,	FALSE,	FALSE,	&beg_DefaultAppDirs,		&dat_DefaultAppDirs,		&end_DefaultAppDirs		},
	{ "DirectoryDir",		MenuContextMenu,	MenuContextNone,	FALSE,	TRUE,	&beg_DirectoryDir,		&dat_DirectoryDir,		&end_DirectoryDir		},
	{ "DefaultDirectoryDirs",	MenuContextMenu,	MenuContextNone,	FALSE,	FALSE,	&beg_DefaultDirectoryDirs,	&dat_DefaultDirectoryDirs,	&end_DefaultDirectoryDirs	},
	{ "Name",			MenuContextMenu,	MenuContextNone,	FALSE,	TRUE,	&beg_Name,			&dat_Name,			&end_Name			},
	{ "Directory",			MenuContextMenu,	MenuContextNone,	FALSE,	TRUE,	&beg_Directory,			&dat_Directory,			&end_Directory			},
	{ "OnlyUnallocated",		MenuContextMenu,	MenuContextNone,	FALSE,	FALSE,	&beg_OnlyUnallocated,		&dat_OnlyUnallocated,		&end_OnlyUnallocated		},
	{ "NotOnlyUnallocated",		MenuContextMenu,	MenuContextNone,	FALSE,	FALSE,	&beg_NotOnlyUnallocated,	&dat_NotOnlyUnallocated,	&end_NotOnlyUnallocated		},
	{ "Deleted",			MenuContextMenu,	MenuContextNone,	FALSE,	FALSE,	&beg_Deleted,			&dat_Deleted,			&end_Deleted			},
	{ "NotDeleted",			MenuContextMenu,	MenuContextNone,	FALSE,	FALSE,	&beg_NotDeleted,		&dat_NotDeleted,		&end_NotDeleted			},
	{ "Include",			MenuContextMenu,	MenuContextRule,	TRUE,	FALSE,	&beg_Include,			&dat_Include,			&end_Include			},
	{ "Exclude",			MenuContextMenu,	MenuContextRule,	TRUE,	FALSE,	&beg_Exclude,			&dat_Exclude,			&end_Exclude			},
	{ "Filename",			MenuContextRule,	MenuContextNone,	FALSE,	TRUE,	&beg_Filename,			&dat_Filename,			&end_Filename			},
	{ "Category",			MenuContextRule,	MenuContextNone,	FALSE,	TRUE,	&beg_Category,			&dat_Category,			&end_Category			},
	{ "All",			MenuContextRule,	MenuContextNone,	FALSE,	FALSE,	&beg_All,			&dat_All,			&end_All			},
	{ "And",			MenuContextRule,	MenuContextRule,	TRUE,	FALSE,	&beg_And,			&dat_And,			&end_And			},
	{ "Or",				MenuContextRule,	MenuContextRule,	TRUE,	FALSE,	&beg_Or,			&dat_Or,			&end_Or				},
	{ "Not",			MenuContextRule,	MenuContextRule,	TRUE,	FALSE,	&beg_Not,			&dat_Not,			&end_Not			},
	{ "MergeFile",			MenuContextMenu,	MenuContextNone,	FALSE,	TRUE,	&beg_MergeFile,			&dat_MergeFile,			&end_MergeFile			},
	{ "MergeDir",			MenuContextMenu,	MenuContextNone,	FALSE,	TRUE,	&beg_MergeDir,			&dat_MergeDir,			&end_MergeDir			},
	{ "DefaultMergeDirs",		MenuContextMenu,	MenuContextNone,	FALSE,	FALSE,	&beg_DefaultMergeDirs,		&dat_DefaultMergeDirs,		&end_DefaultMergeDirsq		},
	{ "LegacyDir",			MenuContextMenu,	MenuContextNone,	FALSE,	TRUE,	&beg_LegacyDir,			&dat_LegacyDir,			&end_LegacyDir			},
	{ "KDELegacyDirs",		MenuContextMenu,	MenuContextNone,	FALSE,	FALSE,	&beg_KDELegacyDirs,		&dat_KDELegacyDirs,		&end_KDELegacyDirs		},
	{ "Move",			MenuContextMenu,	MenuContextMove,	TRUE,	FALSE,	&beg_Move,			&dat_Move,			&end_Move			},
	{ "Old",			MenuContextMove,	MenuContextNone,	FALSE,	TRUE,	&beg_Old,			&dat_Old,			&end_Old			},
	{ "New",			MenuContextMove,	MenuContextNone,	FALSE,	TRUE,	&beg_New,			&dat_New,			&end_New			},
	{ "Layout",			MenuContextMenu,	MenuContextLayout,	TRUE,	FALSE,	&beg_Layout,			&dat_Layout,			&end_Layout			},
	{ "DefaultLayout",		MenuContextMenu,	MenuContextLayout,	TRUE,	FALSE,	&beg_DefaultLayout,		&dat_DefaultLayout,		&end_DefaultLayout		},
	{ "Menuname",			MenuContextLayout,	MenuContextNone,	FALSE,	TRUE,	&beg_Menuname,			&dat_Menuname,			&end_Menuname			},
	{ "Separator",			MenuContextLayout,	MenuContextNone,	FALSE,	FALSE,	&beg_Separator,			&dat_Separator,			&end_Separator			},
	{ "Merge",			MenuContextLayout,	MenuContextNone,	FALSE,	FALSE,	&beg_Merge,			&dat_Merge,			&end_Merge			},
	{ NULL,				MenuContextNone,	MenuContextNone,	FALSE,	FALSE,	NULL,				NULL,				NULL				}
	/* *INDENT-ON* */
};

static void
beg_Menu(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu, *parent;

	menu = calloc(1, sizeof(*menu));
	menu->submenu = g_queue_new();
	menu->appdirs = g_queue_new();
	menu->dirdirs = g_queue_new();
	menu->directory = g_queue_new();
	menu->rules = g_queue_new();
	menu->stack = g_queue_new();
	menu->merge = g_queue_new();
	menu->parent = parent = g_queue_peek_tail(tree->menus);
	g_queue_push_tail(tree->menus, menu);
}

static void
dat_Menu(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_Menu(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu;

	menu = g_queue_pop_tail(tree->menus);
}

static void
beg_AppDir(GMarkupParseContext * context, const gchar *element_name,
	   const gchar **attribute_names, const gchar **attribute_values,
	   gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
dat_AppDir(GMarkupParseContext * context, const gchar *text, gsize text_len,
	   gpointer user_data, GError ** error)
{
	MenuTree *tree;
	MenuContext *menu;
	char *cdata;

	/* basically append the directory to the application directory list for the menu */
	tree = user_data;
	menu = g_queue_peek_tail(tree->menus);
	cdata = calloc(text_len + 1, sizeof(*cdata));
	strncpy(cdata, text, text_len);
	g_queue_push_tail(menu->appdirs, cdata);
}

static void
end_AppDir(GMarkupParseContext * context, const gchar *element_name,
	   gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
beg_DefaultAppDirs(GMarkupParseContext * context, const gchar *element_name,
		   const gchar **attribute_names, const gchar **attribute_values,
		   gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
dat_DefaultAppDirs(GMarkupParseContext * context, const gchar *text, gsize text_len,
		   gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_DefaultAppDirs(GMarkupParseContext * context, const gchar *element_name,
		   gpointer user_data, GError ** error)
{
	MenuTree *tree;
	MenuContext *menu;
	char *data, *home, *dirs, *p;
	int len;

	/* basically append the reverse of $XDG_DATA_DIRS/applications to the application 
	   directory list for the menu */

	data = strdup(getenv("XDG_DATA_DIRS") ? : "/usr/local/share:/usr/share");
	if (getenv("XDG_DATA_HOME"))
		home = strdup(getenv("XDG_DATA_HOME"));
	else {
		const char *user = getenv("HOME") ? : ".";

		len = strlen(user) + strlen("/.local/share");
		home = calloc(len + 1, sizeof(*home));
		strncpy(home, user, len);
		strncat(home, "/.local/share", len);
	}
	len = strlen(data) + 1 + strlen(home);
	dirs = calloc(len + 1, sizeof(*dirs));
	strncpy(dirs, home, len);
	strncat(dirs, ":", len);
	strncat(dirs, data, len);
	free(home);
	free(data);

	tree = user_data;
	menu = g_queue_peek_tail(tree->menus);

	for (;;) {
		char *dir;

		p = (p = strchr(dirs, ':')) ? p + 1 : dirs;
		len = strlen(p) + strlen("/applications");
		dir = calloc(len + 1, sizeof(*dir));
		strncpy(dir, p, len);
		strncat(dir, "/applications", len);
		g_queue_push_tail(menu->appdirs, dir);
		if (p == dirs)
			break;
		*(p - 1) = '\0';
	}
	free(dirs);
}

static void
beg_DirectoryDir(GMarkupParseContext * context, const gchar *element_name,
		 const gchar **attribute_names, const gchar **attribute_values,
		 gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
dat_DirectoryDir(GMarkupParseContext * context, const gchar *text, gsize text_len,
		 gpointer user_data, GError ** error)
{
	MenuTree *tree;
	MenuContext *menu;
	char *cdata;

	/* basically append the directory to the directory directory list for the menu */
	tree = user_data;
	menu = g_queue_peek_tail(tree->menus);
	cdata = calloc(text_len + 1, sizeof(*cdata));
	strncpy(cdata, text, text_len);
	g_queue_push_tail(menu->dirdirs, cdata);
}

static void
end_DirectoryDir(GMarkupParseContext * context, const gchar *element_name,
		 gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
beg_DefaultDirectoryDirs(GMarkupParseContext * context, const gchar *element_name,
			 const gchar **attribute_names, const gchar **attribute_values,
			 gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
dat_DefaultDirectoryDirs(GMarkupParseContext * context, const gchar *text, gsize text_len,
			 gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_DefaultDirectoryDirs(GMarkupParseContext * context, const gchar *element_name,
			 gpointer user_data, GError ** error)
{
	MenuTree *tree;
	MenuContext *menu;
	char *conf, *home, *dirs, *p;
	int len;

	/* basically append the reverse of $XDG_DATA_DIRS/desktop-directories to the directory
	 * directories list for the menu */

	conf = strdup(getenv("XDG_CONFIG_DIRS") ? : "/etc/xdg");
	if (getenv("XDG_CONFIG_HOME"))
		home = strdup(getenv("XDG_CONFIG_HOME"));
	else {
		const char *user = getenv("HOME") ? : ".";

		len = strlen(user) + strlen("/.config");
		home = calloc(len + 1, sizeof(*home));
		strncpy(home, user, len);
		strncat(home, "/.config", len);
	}
	len = strlen(conf) + 1 + strlen(home);
	dirs = calloc(len + 1, sizeof(*dirs));
	strncpy(dirs, home, len);
	strncat(dirs, ":", len);
	strncat(dirs, conf, len);
	free(home);
	free(conf);

	tree = user_data;
	menu = g_queue_peek_tail(tree->menus);

	for (;;) {
		char *dir;

		p = (p = strchr(dirs, ':')) ? p + 1 : dirs;
		len = strlen(p) + strlen("/desktop-directories");
		dir = calloc(len + 1, sizeof(*dir));
		strncpy(dir, p, len);
		strncat(dir, "/desktop-directories", len);
		g_queue_push_tail(menu->dirdirs, dir);
		if (p == dirs)
			break;
		*(p - 1) = '\0';
	}
	free(dirs);
}

static void
beg_Name(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
dat_Name(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
	MenuTree *tree;
	MenuContext *menu;
	char *cdata;

	cdata = calloc(text_len + 1, sizeof(*cdata));
	strncpy(cdata, text, text_len);

	/* ignore names that contain a slash according to menu-spec */
	if (strchr(cdata, '/')) {
		free(cdata);
		return;
	}
	tree = user_data;
	menu = g_queue_peek_tail(tree->menus);
	free(menu->name);
	menu->name = cdata;
}

static void
end_Name(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
beg_Directory(GMarkupParseContext * context, const gchar *element_name,
	      const gchar **attribute_names, const gchar **attribute_values,
	      gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
dat_Directory(GMarkupParseContext * context, const gchar *text, gsize text_len,
	      gpointer user_data, GError ** error)
{
	MenuTree *tree;
	MenuContext *menu;
	char *cdata;

	tree = user_data;
	menu = g_queue_peek_tail(tree->menus);
	cdata = calloc(text_len + 1, sizeof(*cdata));
	strncpy(cdata, text, text_len);
	g_queue_push_tail(menu->directory, cdata);
}

static void
end_Directory(GMarkupParseContext * context, const gchar *element_name,
	      gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
beg_OnlyUnallocated(GMarkupParseContext * context, const gchar *element_name,
		    const gchar **attribute_names, const gchar **attribute_values,
		    gpointer user_data, GError ** error)
{
	MenuTree *tree;
	MenuContext *menu;

	tree = user_data;
	menu = g_queue_peek_tail(tree->menus);
	menu->onlyunallocated = TRUE;
}

static void
dat_OnlyUnallocated(GMarkupParseContext * context, const gchar *text, gsize text_len,
		    gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_OnlyUnallocated(GMarkupParseContext * context, const gchar *element_name,
		    gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
beg_NotOnlyUnallocated(GMarkupParseContext * context, const gchar *element_name,
		       const gchar **attribute_names, const gchar **attribute_values,
		       gpointer user_data, GError ** error)
{
	MenuTree *tree;
	MenuContext *menu;

	tree = user_data;
	menu = g_queue_peek_tail(tree->menus);
	menu->onlyunallocated = FALSE;
}

static void
dat_NotOnlyUnallocated(GMarkupParseContext * context, const gchar *text, gsize text_len,
		       gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_NotOnlyUnallocated(GMarkupParseContext * context, const gchar *element_name,
		       gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
beg_Deleted(GMarkupParseContext * context, const gchar *element_name,
	    const gchar **attribute_names, const gchar **attribute_values,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);

	menu->deleted = TRUE;
}

static void
dat_Deleted(GMarkupParseContext * context, const gchar *text, gsize text_len,
	    gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_Deleted(GMarkupParseContext * context, const gchar *element_name,
	    gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
beg_NotDeleted(GMarkupParseContext * context, const gchar *element_name,
	       const gchar **attribute_names, const gchar **attribute_values,
	       gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);

	menu->deleted = FALSE;
}

static void
dat_NotDeleted(GMarkupParseContext * context, const gchar *text, gsize text_len,
	       gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_NotDeleted(GMarkupParseContext * context, const gchar *element_name,
	       gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
beg_Include(GMarkupParseContext * context, const gchar *element_name,
	    const gchar **attribute_names, const gchar **attribute_values,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *rule;

	if (!g_queue_is_empty(menu->stack)) {
		*error = g_error_new_literal(
				G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				"Include cannot be nested in Include or Exclude.");
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = MenuRuleInclude;
	rule->rules = g_queue_new();
	g_queue_push_tail(menu->rules, rule);
	g_queue_push_tail(menu->stack, rule);
}

static void
dat_Include(GMarkupParseContext * context, const gchar *text, gsize text_len,
	    gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_Include(GMarkupParseContext * context, const gchar *element_name,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);

	g_queue_pop_tail(menu->stack);
}

static void
beg_Exclude(GMarkupParseContext * context, const gchar *element_name,
	    const gchar **attribute_names, const gchar **attribute_values,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *rule;

	if (!g_queue_is_empty(menu->stack)) {
		*error = g_error_new_literal(
				G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				"Exclude cannot be nested in Include or Exclude.");
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = MenuRuleExclude;
	rule->rules = g_queue_new();
	g_queue_push_tail(menu->rules, rule);
	g_queue_push_tail(menu->stack, rule);
}

static void
dat_Exclude(GMarkupParseContext * context, const gchar *text, gsize text_len,
	    gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_Exclude(GMarkupParseContext * context, const gchar *element_name,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);

	g_queue_pop_tail(menu->stack);
}

static void
beg_Filename(GMarkupParseContext * context, const gchar *element_name,
	     const gchar **attribute_names, const gchar **attribute_values,
	     gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *stack = g_queue_peek_tail(menu->stack);
	MenuRule *rule;

	if (!stack) {
		*error = g_error_new_literal(
				G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				"<Filename> must be nested in a rule");
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = MenuRuleFilename;
	g_queue_push_tail(stack->rules, rule);
	g_queue_push_tail(menu->stack, rule);
}

static void
dat_Filename(GMarkupParseContext * context, const gchar *text, gsize text_len,
	     gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *rule = g_queue_peek_tail(menu->stack);
	char *cdata;

	cdata = calloc(text_len + 1, sizeof(*cdata));
	strncpy(cdata, text, text_len);
	rule->text = cdata;
}

static void
end_Filename(GMarkupParseContext * context, const gchar *element_name,
	     gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	
	g_queue_pop_tail(menu->stack);
}

static void
beg_Category(GMarkupParseContext * context, const gchar *element_name,
	     const gchar **attribute_names, const gchar **attribute_values,
	     gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *stack = g_queue_peek_tail(menu->stack);
	MenuRule *rule;

	if (!stack) {
		*error = g_error_new_literal(
				G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				"<Category> must be nested in a rule");
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = MenuRuleCategory;
	g_queue_push_tail(stack->rules, rule);
	g_queue_push_tail(menu->stack, rule);
}

static void
dat_Category(GMarkupParseContext * context, const gchar *text, gsize text_len,
	     gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *rule = g_queue_peek_tail(menu->stack);
	char *cdata;

	cdata = calloc(text_len + 1, sizeof(*cdata));
	strncpy(cdata, text, text_len);
	rule->text = cdata;
}

static void
end_Category(GMarkupParseContext * context, const gchar *element_name,
	     gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	
	g_queue_pop_tail(menu->stack);
}

static void
beg_All(GMarkupParseContext * context, const gchar *element_name,
	const gchar **attribute_names, const gchar **attribute_values,
	gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *stack = g_queue_peek_tail(menu->stack);
	MenuRule *rule;

	if (!stack) {
		*error = g_error_new_literal(
				G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				"<All> must be nested in a rule");
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = MenuRuleAll;
	g_queue_push_tail(stack->rules, rule);
	g_queue_push_tail(menu->stack, rule);
}

static void
dat_All(GMarkupParseContext * context, const gchar *text, gsize text_len,
	gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_All(GMarkupParseContext * context, const gchar *element_name,
	gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	
	g_queue_pop_tail(menu->stack);
}

static void
beg_And(GMarkupParseContext * context, const gchar *element_name,
	const gchar **attribute_names, const gchar **attribute_values,
	gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *stack = g_queue_peek_tail(menu->stack);
	MenuRule *rule;

	if (!stack) {
		*error = g_error_new_literal(
				G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				"<And> must be nested in a rule");
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = MenuRuleAnd;
	rule->rules = g_queue_new();
	g_queue_push_tail(stack->rules, rule);
	g_queue_push_tail(menu->stack, rule);
}

static void
dat_And(GMarkupParseContext * context, const gchar *text, gsize text_len,
	gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_And(GMarkupParseContext * context, const gchar *element_name,
	gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	
	g_queue_pop_tail(menu->stack);
}

static void
beg_Or(GMarkupParseContext * context, const gchar *element_name,
       const gchar **attribute_names, const gchar **attribute_values,
       gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *stack = g_queue_peek_tail(menu->stack);
	MenuRule *rule;

	if (!stack) {
		*error = g_error_new_literal(
				G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				"<Or> must be nested in a rule");
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = MenuRuleOr;
	rule->rules = g_queue_new();
	g_queue_push_tail(stack->rules, rule);
	g_queue_push_tail(menu->stack, rule);
}

static void
dat_Or(GMarkupParseContext * context, const gchar *text, gsize text_len,
       gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_Or(GMarkupParseContext * context, const gchar *element_name,
       gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	
	g_queue_pop_tail(menu->stack);
}

static void
beg_Not(GMarkupParseContext * context, const gchar *element_name,
	const gchar **attribute_names, const gchar **attribute_values,
	gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	MenuRule *stack = g_queue_peek_tail(menu->stack);
	MenuRule *rule;

	if (!stack) {
		*error = g_error_new_literal(
				G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				"<Not> must be nested in a rule");
		return;
	}
	rule = calloc(1, sizeof(*rule));
	rule->type = MenuRuleNot;
	rule->rules = g_queue_new();
	g_queue_push_tail(stack->rules, rule);
	g_queue_push_tail(menu->stack, rule);
}

static void
dat_Not(GMarkupParseContext * context, const gchar *text, gsize text_len,
	gpointer user_data, GError ** error)
{
	/* do nothing */
}

static void
end_Not(GMarkupParseContext * context, const gchar *element_name,
	gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu = g_queue_peek_tail(tree->menus);
	
	g_queue_pop_tail(menu->stack);
}

static void
beg_MergeFile(GMarkupParseContext * context, const gchar *element_name,
	      const gchar **attribute_names, const gchar **attribute_values,
	      gpointer user_data, GError ** error)
{
}

static void
dat_MergeFile(GMarkupParseContext * context, const gchar *text, gsize text_len,
	      gpointer user_data, GError ** error)
{
}

static void
end_MergeFile(GMarkupParseContext * context, const gchar *element_name,
	      gpointer user_data, GError ** error)
{
}

static void
beg_MergeDir(GMarkupParseContext * context, const gchar *element_name,
	     const gchar **attribute_names, const gchar **attribute_values,
	     gpointer user_data, GError ** error)
{
}

static void
dat_MergeDir(GMarkupParseContext * context, const gchar *text, gsize text_len,
	     gpointer user_data, GError ** error)
{
}

static void
end_MergeDir(GMarkupParseContext * context, const gchar *element_name,
	     gpointer user_data, GError ** error)
{
}

static void
beg_DefaultMergeDirs(GMarkupParseContext * context, const gchar *element_name,
		     const gchar **attribute_names, const gchar **attribute_values,
		     gpointer user_data, GError ** error)
{
}

static void
dat_DefaultMergeDirs(GMarkupParseContext * context, const gchar *text, gsize text_len,
		     gpointer user_data, GError ** error)
{
}

static void
end_DefaultMergeDirs(GMarkupParseContext * context, const gchar *element_name,
		     gpointer user_data, GError ** error)
{
}

static void
beg_LegacyDir(GMarkupParseContext * context, const gchar *element_name,
	      const gchar **attribute_names, const gchar **attribute_values,
	      gpointer user_data, GError ** error)
{
}

static void
dat_LegacyDir(GMarkupParseContext * context, const gchar *text, gsize text_len,
	      gpointer user_data, GError ** error)
{
}

static void
end_LegacyDir(GMarkupParseContext * context, const gchar *element_name,
	      gpointer user_data, GError ** error)
{
}

static void
beg_KDELegacyDirs(GMarkupParseContext * context, const gchar *element_name,
		  const gchar **attribute_names, const gchar **attribute_values,
		  gpointer user_data, GError ** error)
{
}

static void
dat_KDELegacyDirs(GMarkupParseContext * context, const gchar *text, gsize text_len,
		  gpointer user_data, GError ** error)
{
}

static void
end_KDELegacyDirs(GMarkupParseContext * context, const gchar *element_name,
		  gpointer user_data, GError ** error)
{
}

static void
beg_Move(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Move(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Move(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Old(GMarkupParseContext * context, const gchar *element_name,
	const gchar **attribute_names, const gchar **attribute_values,
	gpointer user_data, GError ** error)
{
}

static void
dat_Old(GMarkupParseContext * context, const gchar *text, gsize text_len,
	gpointer user_data, GError ** error)
{
}

static void
end_Old(GMarkupParseContext * context, const gchar *element_name,
	gpointer user_data, GError ** error)
{
}

static void
beg_New(GMarkupParseContext * context, const gchar *element_name,
	const gchar **attribute_names, const gchar **attribute_values,
	gpointer user_data, GError ** error)
{
}

static void
dat_New(GMarkupParseContext * context, const gchar *text, gsize text_len,
	gpointer user_data, GError ** error)
{
}

static void
end_New(GMarkupParseContext * context, const gchar *element_name,
	gpointer user_data, GError ** error)
{
}

static void
beg_Layout(GMarkupParseContext * context, const gchar *element_name,
	   const gchar **attribute_names, const gchar **attribute_values,
	   gpointer user_data, GError ** error)
{
}

static void
dat_Layout(GMarkupParseContext * context, const gchar *text, gsize text_len,
	   gpointer user_data, GError ** error)
{
}

static void
end_Layout(GMarkupParseContext * context, const gchar *element_name,
	   gpointer user_data, GError ** error)
{
}

static void
beg_DefaultLayout(GMarkupParseContext * context, const gchar *element_name,
		  const gchar **attribute_names, const gchar **attribute_values,
		  gpointer user_data, GError ** error)
{
}

static void
dat_DefaultLayout(GMarkupParseContext * context, const gchar *text, gsize text_len,
		  gpointer user_data, GError ** error)
{
}

static void
end_DefaultLayout(GMarkupParseContext * context, const gchar *element_name,
		  gpointer user_data, GError ** error)
{
}

static void
beg_Menuname(GMarkupParseContext * context, const gchar *element_name,
	     const gchar **attribute_names, const gchar **attribute_values,
	     gpointer user_data, GError ** error)
{
}

static void
dat_Menuname(GMarkupParseContext * context, const gchar *text, gsize text_len,
	     gpointer user_data, GError ** error)
{
}

static void
end_Menuname(GMarkupParseContext * context, const gchar *element_name,
	     gpointer user_data, GError ** error)
{
}

static void
beg_Separator(GMarkupParseContext * context, const gchar *element_name,
	      const gchar **attribute_names, const gchar **attribute_values,
	      gpointer user_data, GError ** error)
{
}

static void
dat_Separator(GMarkupParseContext * context, const gchar *text, gsize text_len,
	      gpointer user_data, GError ** error)
{
}

static void
end_Separator(GMarkupParseContext * context, const gchar *element_name,
	      gpointer user_data, GError ** error)
{
}

static void
beg_Merge(GMarkupParseContext * context, const gchar *element_name,
	  const gchar **attribute_names, const gchar **attribute_values,
	  gpointer user_data, GError ** error)
{
}

static void
dat_Merge(GMarkupParseContext * context, const gchar *text, gsize text_len,
	  gpointer user_data, GError ** error)
{
}

static void
end_Merge(GMarkupParseContext * context, const gchar *element_name,
	  gpointer user_data, GError ** error)
{
}

static void
beg_element(GMarkupParseContext * context, const gchar *element_name,
	    const gchar **attribute_names, const gchar **attribute_values,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	Element *p, *e;

	if ((p = g_queue_peek_tail(tree->element))) {
		if (!p->nest) {
			*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
					     "<%s> element cannot contain other elements", e->name);
			return;
		}
	}
	for (e = elements; e->name; e++)
		if (!strcmp(e->name, element_name))
			break;
	if (!e->name)
		e = NULL;
	if (g_queue_is_empty(tree->element) && e && e != &elements[0]) {
		*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				     "<%s> element cannot be a root element", e->name);
		return;
	}
	if (p && e && p->interior != e->context) {
		*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				     "<%s> element cannot occur within <%s> element",
				     e->name, p->name);
		return;
	}
	g_queue_push_tail(tree->element, e);
	if (e && e->beg)
		e->beg(context, element_name, attribute_names, attribute_values, user_data, error);
}

static void
dat_element(GMarkupParseContext * context, char gchar *text, gsize text_len,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	Element *e;

	if ((e = g_queue_peek_tail(tree->element)) && !e->cdata) {
		*error = g_error_new(G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
				     "<%s> element cannot contain CDATA", e->name);
		return;
	}
	if (e && e->dat)
		e->dat(context, text, text_len, user_data, error);
}

static void
end_element(GMarkupParseContext * context, const gchar *element_name,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	Element *e;

	if ((e = g_queue_pop_tail(tree->element)) && e->end)
		e->end(context, element_name, user_data, error);
}

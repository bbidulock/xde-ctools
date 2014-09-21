
#include "menuxml.h"

Element elements[] = {
	/* *INDENT-OFF* */
	{ "Menu",			&beg_Menu,			&dat_Menu,			&end_Menu			},
	{ "AppDir",			&beg_AppDir,			&dat_AppDir,			&end_AppDir			},
	{ "DefaultAppDirs",		&beg_DefaultAppDirs,		&dat_DefaultAppDirs,		&end_DefaultAppDirs		},
	{ "DirectoryDir",		&beg_DirectoryDir,		&dat_DirectoryDir,		&end_DirectoryDir		},
	{ "DefaultDirectoryDirs",	&beg_DefaultDirectoryDirs,	&dat_DefaultDirectoryDirs,	&end_DefaultDirectoryDirs	},
	{ "Name",			&beg_Name,			&dat_Name,			&end_Name			},
	{ "Directory",			&beg_Directory,			&dat_Directory,			&end_Directory			},
	{ "OnlyUnallocated",		&beg_OnlyUnallocated,		&dat_OnlyUnallocated,		&end_OnlyUnallocated		},
	{ "NotOnlyUnallocated",		&beg_NotOnlyUnallocated,	&dat_NotOnlyUnallocated,	&end_NotOnlyUnallocated		},
	{ "Deleted",			&beg_Deleted,			&dat_Deleted,			&end_Deleted			},
	{ "NotDeleted",			&beg_NotDeleted,		&dat_NotDeleted,		&end_NotDeleted			},
	{ "Include",			&beg_Include,			&dat_Include,			&end_Include			},
	{ "Exclude",			&beg_Exclude,			&dat_Exclude,			&end_Exclude			},
	{ "Filename",			&beg_Filename,			&dat_Filename,			&end_Filename			},
	{ "Category",			&beg_Category,			&dat_Category,			&end_Category			},
	{ "All",			&beg_All,			&dat_All,			&end_All			},
	{ "And",			&beg_And,			&dat_And,			&end_And			},
	{ "Or",				&beg_Or,			&dat_Or,			&end_Or				},
	{ "Not",			&beg_Not,			&dat_Not,			&end_Not			},
	{ "MergeFile",			&beg_MergeFile,			&dat_MergeFile,			&end_MergeFile			},
	{ "MergeDir",			&beg_MergeDir,			&dat_MergeDir,			&end_MergeDir			},
	{ "DefaultMergeDirs",		&beg_DefaultMergeDirs,		&dat_DefaultMergeDirs,		&end_DefaultMergeDirsq		},
	{ "LegacyDir",			&beg_LegacyDir,			&dat_LegacyDir,			&end_LegacyDir			},
	{ "Move",			&beg_Move,			&dat_Move,			&end_Move			},
	{ "Old",			&beg_Old,			&dat_Old,			&end_Old			},
	{ "New",			&beg_New,			&dat_New,			&end_New			},
	{ "Layout",			&beg_Layout,			&dat_Layout,			&end_Layout			},
	{ "DefaultLayout",		&beg_DefaultLayout,		&dat_DefaultLayout,		&end_DefaultLayout		},
	{ "Menuname",			&beg_Menuname,			&dat_Menuname,			&end_Menuname			},
	{ "Separator",			&beg_Separator,			&dat_Separator,			&end_Separator			},
	{ "Merge",			&beg_Merge,			&dat_Merge,			&end_Merge			},
	{ NULL,				NULL,				NULL,				NULL				}
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
	menu->tail = &menu->list;
	menu->parent = parent = tree->current;
	if (!parent)
		tree->root = menu;
	tree->current = menu;
	if (parent) {
		*parent->submenu.tail = menu;
		parent->submenu.tail = &menu->submenu.next;
	}
	menu->element = &elements[ElementMenu];
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

	tree->current = tree->current->parent;
}

static void
beg_AppDir(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_AppDir(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_AppDir(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_DefaultAppDirs(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_DefaultAppDirs(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_DefaultAppDirs(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_DirectoryDir(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_DirectoryDir(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_DirectoryDir(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_DefaultDirectoryDirs(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_DefaultDirectoryDirs(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_DefaultDirectoryDirs(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Name(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Name(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Name(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Directory(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Directory(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Directory(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_OnlyUnallocated(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_OnlyUnallocated(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_OnlyUnallocated(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_NotOnlyUnallocated(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_NotOnlyUnallocated(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_NotOnlyUnallocated(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Deleted(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Deleted(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Deleted(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_NotDeleted(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_NotDeleted(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_NotDeleted(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Include(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Include(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Include(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Exclude(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Exclude(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Exclude(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Filename(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Filename(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Filename(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Category(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Category(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Category(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_All(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_All(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_All(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_And(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_And(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_And(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Or(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Or(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Or(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
}

static void
beg_Not(GMarkupParseContext * context, const gchar *element_name,
	 const gchar **attribute_names, const gchar **attribute_values,
	 gpointer user_data, GError ** error)
{
}

static void
dat_Not(GMarkupParseContext * context, const gchar *text, gsize text_len,
	 gpointer user_data, GError ** error)
{
}

static void
end_Not(GMarkupParseContext * context, const gchar *element_name,
	 gpointer user_data, GError ** error)
{
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
	Element *e;

	for (e = elements; e->name; e++)
		if (!strcmp(e->name, element_name))
			break;
	if (e->beg)
		e->beg(context, element_name, attribute_names, attribute_values, user_data, error);
}

static void
dat_element(GMarkupParseContext * context, char gchar *text, gsize text_len,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	MenuContext *menu;
	Element *e;

	if (!(menu = tree->current))
		return;
	if (!(e = menu->element))
		return;
	if (e->dat)
		e->dat(context, text, text_len, user_data, error);
}

static void
end_element(GMarkupParseContext * context, const gchar *element_name,
	    gpointer user_data, GError ** error)
{
	MenuTree *tree = user_data;
	Element *e;

	for (e = elements; e->name; e++)
		if (!strcmp(e->name, element_name))
			break;
	if (e->end)
		e->end(context, element_name, user_data, error);
}

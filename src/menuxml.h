
typedef enum {
	ElementMenu,
	ElementAppDir,
	ElementDefaultAppDirs,
	ElementDirectoryDir,
	ElementDefaultDirectoryDirs,
	ElementName,
	ElementDirectory,
	ElementOnlyUnallocated,
	ElementNotOnlyUnallocated,
	ElementDeleted,
	ElementNotDeleted,
	ElementInclude,
	ElementExclude,
	ElementFilename,
	ElementCategory,
	ElementAll,
	ElementAnd,
	ElementOr,
	ElementNot,
	ElementMergeFile,
	ElementMergeDir,
	ElementDefaultMergeDirs,
	ElementLegacyDir,
	ElementKDELegacyDirs,
	ElementMove,
	ElementOld,
	ElementNew,
	ElementLayout,
	ElementDefaultLayout,
	ElementMenuname,
	ElementSeparator,
	ElementMerge,
} ElementType;

typedef struct {
	const gchar *name;
	void (*beg) (GMarkupParseContext *, const gchar *, const gchar **, const gchar **, gpointer,
		     GError **);
	void (*dat) (GMarkupParseContext *, const gchar *, gsize, gpointer, GError **);
	void (*end) (GMarkupParseContext *, const gchar *, gpointer, GError **);
} Element;

extern Elements elements[];

typedef struct _MenuContext MenuContext;

struct _MenuContext {
	MenuContext *parent;		/* parent menu for this one (NULL for root) */
	struct {
		MenuContext *next;	/* the next menu in the submenu list */
		MenuContext *list;	/* the list of submenus of this menu */
		MenuContext **tail;	/* pointer to the pointer at the end of the list */
	} submenu;
	Element *element;		/* current element being processed */
};

typedef struct _MenuTree MenuTree;

struct _MenuTree {
	gchar *filename;		/* the file name of the parsed menu file */
	MenuContext *root;		/* the root of the DOM */
	MenuContext *current;		/* the currently parsed <Menu></Menu> tag */
	struct {
		MenuTree *next;		/* the next tree in the merge list */
		MenuTree *list;		/* the list of trees to merge into this one */
		MenuTree **tail;	/* pointer to the pointer at the end of the list */
	} merge;
};

static void beg_element(GMarkupParseContext *, const gchar *,
			const gchar **, const gchar **, gpointer, GError **);
static void dat_element(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_element(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_Menu(GMarkupParseContext *, const gchar *,
		     const gchar **, const gchar **, gpointer, GError **);
static void dat_Menu(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_Menu(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_AppDir(GMarkupParseContext *, const gchar *,
		       const gchar **, const gchar **, gpointer, GError **);
static void dat_AppDir(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_AppDir(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_DefaultAppDirs(GMarkupParseContext *, const gchar *,
			       const gchar **, const gchar **, gpointer, GError **);
static void dat_DefaultAppDirs(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_DefaultAppDirs(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_DirectoryDir(GMarkupParseContext *, const gchar *,
			     const gchar **, const gchar **, gpointer, GError **);
static void dat_DirectoryDir(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_DirectoryDir(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_DefaultDirectoryDirs(GMarkupParseContext *, const gchar *,
				     const gchar **, const gchar **, gpointer, GError **);
static void dat_DefaultDirectoryDirs(GMarkupParseContext *, char gchar *, gsize, gpointer,
				     GError **);
static void end_DefaultDirectoryDirs(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_Name(GMarkupParseContext *, const gchar *,
		     const gchar **, const gchar **, gpointer, GError **);
static void dat_Name(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_Name(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_Directory(GMarkupParseContext *, const gchar *,
			  const gchar **, const gchar **, gpointer, GError **);
static void dat_Directory(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_Directory(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_OnlyUnallocated(GMarkupParseContext *, const gchar *,
				const gchar **, const gchar **, gpointer, GError **);
static void dat_OnlyUnallocated(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_OnlyUnallocated(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_NotOnlyUnallocated(GMarkupParseContext *, const gchar *,
				   const gchar **, const gchar **, gpointer, GError **);
static void dat_NotOnlyUnallocated(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_NotOnlyUnallocated(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_Deleted(GMarkupParseContext *, const gchar *,
			const gchar **, const gchar **, gpointer, GError **);
static void dat_Deleted(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_Deleted(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_NotDeleted(GMarkupParseContext *, const gchar *,
			   const gchar **, const gchar **, gpointer, GError **);
static void dat_NotDeleted(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_NotDeleted(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_Include(GMarkupParseContext *, const gchar *,
			const gchar **, const gchar **, gpointer, GError **);
static void dat_Include(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_Include(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_Exclude(GMarkupParseContext *, const gchar *,
			const gchar **, const gchar **, gpointer, GError **);
static void dat_Exclude(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_Exclude(GMarkupParseContext *, const gchar *, gpointer, GError **);

static void beg_Filename(GMarkupParseContext *, const gchar *,
			 const gchar **, const gchar **, gpointer, GError **);
static void dat_Filename(GMarkupParseContext *, char gchar *, gsize, gpointer, GError **);
static void end_Filename(GMarkupParseContext *, const gchar *, gpointer, GError **);

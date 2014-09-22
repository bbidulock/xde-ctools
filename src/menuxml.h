
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

typedef enum {
	MenuContextNone,
	MenuContextMenu,
	MenuContextRule,
	MenuContextMove,
	MenuContextLayout,
} MenuContextType;

typedef struct {
	const gchar *name;
	MenuContext context, interior;
	gboolean nest, cdata;
	void (*beg) (GMarkupParseContext *, const gchar *, const gchar **, const gchar **, gpointer,
		     GError **);
	void (*dat) (GMarkupParseContext *, const gchar *, gsize, gpointer, GError **);
	void (*end) (GMarkupParseContext *, const gchar *, gpointer, GError **);
} Element;

extern Elements elements[];

typedef enum {
	MenuRuleInclude,
	MenuRuleExclude,
	MenuRuleFilename,
	MenuRuleCategory,
	MenuRuleAll,
	MenuRuleAnd,
	MenuRuleOr,
	MenuRuleNot,
} MenuRuleType;

typedef struct _MenuRule MenuRule;

typedef struct {
	MenuRuleType type;
	char *text;
	GQueue *rules;
} MenuRule;

typedef struct _MenuContext MenuContext;

struct _MenuContext {
	MenuContext *parent;		/* parent menu for this one (NULL for root) */
	char *name;			/* name of the menu */
	gboolean onlyunallocated;	/* whether unallocated entries only */
	gboolean deleted;		/* whether the menu is deleted */
	GQueue *submenu;		/* submenus for this menu */
	GQueue *appdirs;		/* applications directories for menu and subdirs */
	GQueue *dirdirs;		/* directory directories for menu and subdirs */
	GQueue *directory;		/* directory entries in order of appearance */
	GQueue *rules;			/* rules */
	GQueue *stack;			/* rule stack */
	GQueue *merge;			/* menu trees to merge */
};

typedef struct {
	gchar *filename;		/* the file name of the parsed menu file */
	GQueue *menus;			/* the currently parsed <Menu></Menu> tag */
	GQueue *element;		/* element stack for this tree */
} MenuTree;


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

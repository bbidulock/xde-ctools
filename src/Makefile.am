
AM_CFLAGS = $(X11_CFLAGS) $(XINERAMA_CFLAGS) $(XRANDR_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS) $(WNCK_CFLAGS)

bin_PROGRAMS = \
	xde-app \
	xde-dock \
	xde-run \
	xde-winmenu \
	xde-winlist

xde_app_CPPFLAGS = -DNAME=\"xde-app\"
xde_app_SOURCES = xde-app.c xde-app.h
xde_app_LDADD = $(WNCK_LIBS) $(GTK_LIBS) $(GDK_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(X11_LIBS)

xde_dock_CPPFLAGS = -DNAME=\"xde-dock\"
xde_dock_SOURCES = xde-dock.c xde-dock.h
xde_dock_LDADD = $(WNCK_LIBS) $(GTK_LIBS) $(GDK_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(X11_LIBS)

xde_run_CPPFLAGS = -DNAME=\"xde-run\"
xde_run_SOURCES = xde-run.c xde-run.h
xde_run_LDADD = $(WNCK_LIBS) $(GTK_LIBS) $(GDK_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(X11_LIBS)

xde_winmenu_CPPFLAGS = -DNAME=\"xde-winmenu\"
xde_winmenu_SOURCES = xde-winmenu.c xde-winmenu.h
xde_winmenu_LDADD = $(WNCK_LIBS) $(GTK_LIBS) $(GDK_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(X11_LIBS)

xde_winlist_CPPFLAGS = -DNAME=\"xde-winlist\"
xde_winlist_SOURCES = xde-winlist.c xde-winlist.h
xde_winlist_LDADD = $(WNCK_LIBS) $(GTK_LIBS) $(GDK_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(X11_LIBS)


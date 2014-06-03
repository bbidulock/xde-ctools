
AM_CFLAGS = $(X11_CFLAGS) $(XINERAMA_CFLAGS) $(XRANDR_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS) $(WNCK_CFLAGS)

bin_PROGRAMS = \
       xde-dock \
       xde-run

xde_dock_CPPFLAGS = -DNAME=\"xde-dock\"
xde_dock_SOURCES = xde-dock.c xde-dock.h
xde_dock_LDADD = $(WNCK_LIBS) $(GTK_LIBS) $(GDK_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(X11_LIBS)

xde_run_CPPFLAGS = -DNAME=\"xde-run\"
xde_run_SOURCES = xde-run.c xde-run.h
xde_run_LDADD = $(WNCK_LIBS) $(GTK_LIBS) $(GDK_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(X11_LIBS)


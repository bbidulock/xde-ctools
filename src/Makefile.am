
AM_CFLAGS = $(X11_CFLAGS) $(XINERAMA_CFLAGS) $(XRANDR_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS)

bin_PROGRAMS = xde-dock

xde_dock_CPPFLAGS = -DNAME=\"xde-theme\"
xde_dock_SOURCES = xde-theme.c
xde_dock_LDADD = $(GTK_LIBS) $(GDK_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(X11_LIBS)



AM_CFLAGS = $(X11_CFLAGS) $(XEXT_CFLAGS) $(XSCRNSAVER_CFLAGS) $(XINERAMA_CFLAGS) $(XRANDR_CFLAGS) $(CAIRO_CFLAGS) $(GIO_UNIX_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS) $(WNCK_CFLAGS)
AM_LDFLAGS = $(WNCK_LIBS) $(GTK_LIBS) $(GDK_LIBS) $(GIO_UNIX_LIBS) $(CAIRO_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(XSCRNSAVER_LIBS) $(XEXT_LIBS) $(X11_LIBS)

bin_PROGRAMS = \
	xde-app \
	xde-desktop \
	xde-dock \
	xde-run \
	xde-winmenu \
	xde-recent \
	xde-winlist \
	xde-pager \
	xde-cycle \
	xde-setbg \
	xde-keys \
	xde-wkspmenu \
	xde-places

xde_app_CPPFLAGS = -DNAME=\"xde-app\"
xde_app_SOURCES = xde-app.c xde-app.h

xde_desktop_CPPFLAGS = -DNAME=\"xde-desktop\"
xde_desktop_SOURCES = xde-desktop.c
xde_desktop_CFLAGS = $(AM_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS)
xde_desktop_LDFLAGS = $(SMLIB_LIBS) $(ICE_LIBS) $(AM_LDFLAGS)

xde_dock_CPPFLAGS = -DNAME=\"xde-dock\"
xde_dock_SOURCES = xde-dock.c xde-dock.h

xde_run_CPPFLAGS = -DNAME=\"xde-run\"
xde_run_SOURCES = xde-run.c xde-run.h

xde_winmenu_CPPFLAGS = -DNAME=\"xde-winmenu\"
xde_winmenu_SOURCES = xde-winmenu.c

xde_recent_CPPFLAGS = -DNAME=\"xde-recent\"
xde_recent_SOURCES = xde-recent.c

xde_winlist_CPPFLAGS = -DNAME=\"xde-winlist\"
xde_winlist_SOURCES = xde-winlist.c
xde_winlist_CFLAGS = $(AM_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS)
xde_winlist_LDFLAGS = $(SMLIB_LIBS) $(ICE_LIBS) $(AM_LDFLAGS)

xde_pager_CPPFLAGS = -DNAME=\"xde-pager\"
xde_pager_SOURCES = xde-pager.c
xde_pager_CFLAGS = $(AM_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS)
xde_pager_LDFLAGS = $(SMLIB_LIBS) $(ICE_LIBS) $(AM_LDFLAGS)

xde_cycle_CPPFLAGS = -DNAME=\"xde-cycle\"
xde_cycle_SOURCES = xde-cycle.c xde-cycle.h

xde_setbg_CPPFLAGS = -DNAME=\"xde-setbg\"
xde_setbg_SOURCES = xde-setbg.c
xde_setbg_CFLAGS = $(AM_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS)
xde_setbg_LDFLAGS = $(SMLIB_LIBS) $(ICE_LIBS) $(AM_LDFLAGS)

xde_keys_CPPFLAGS = -DNAME=\"xde-keys\"
xde_keys_SOURCES = xde-keys.c xde-keys.h

xde_wkspmenu_CPPFLAGS = -DNAME=\"xde-wkspmenu\"
xde_wkspmenu_SOURCES = xde-wkspmenu.c

xde_places_CPPFLAGS = -DNAME=\"xde-places\"
xde_places_SOURCES = xde-places.c


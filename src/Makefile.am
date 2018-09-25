
AM_CFLAGS = $(X11_CFLAGS) $(XEXT_CFLAGS) $(XSCRNSAVER_CFLAGS) $(XINERAMA_CFLAGS) $(XRANDR_CFLAGS) $(CAIRO_CFLAGS) $(GIO_UNIX_CFLAGS) $(GDK_CFLAGS) $(GTK_CFLAGS) $(WNCK_CFLAGS)
AM_LDFLAGS = $(WNCK_LIBS) $(GTK_LIBS) $(GDK_LIBS) $(GIO_UNIX_LIBS) $(CAIRO_LIBS) $(XINERAMA_LIBS) $(XRANDR_LIBS) $(XSCRNSAVER_LIBS) $(XEXT_LIBS) $(X11_LIBS)

bin_PROGRAMS = \
	xde-app \
	xde-cycle \
	xde-dock \
	xde-feedback \
	xde-input \
	xde-keys \
	xde-monitor \
	xde-pager \
	xde-places \
	xde-preferred \
	xde-recent \
	xde-run \
	xde-select \
	xde-setbg \
	xde-tasks \
	xde-winds \
	xde-winlist \
	xde-winmenu \
	xde-wkspmenu

EXTRA_PROGRAMS = \
	xde-clip

xde_app_CPPFLAGS = -DNAME=\"xde-app\"
xde_app_SOURCES = xde-app.c

xde_clip_CPPFLAGS = -DNAME=\"xde-clip\"
xde_clip_SOURCES = xde-clip.c
xde_clip_CFLAGS = $(AM_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS)
xde_clip_LDFLAGS = $(SMLIB_LIBS) $(ICE_LIBS) $(AM_LDFLAGS)

xde_cycle_CPPFLAGS = -DNAME=\"xde-cycle\"
xde_cycle_SOURCES = xde-cycle.c
xde_cycle_CFLAGS = $(AM_CFLAGS) $(NOTIFY_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS) $(SN_CFLAGS)
xde_cycle_LDFLAGS = $(SN_LIBS) $(SMLIB_LIBS) $(ICE_LIBS) $(NOTIFY_LIBS) $(AM_LDFLAGS)

xde_dock_CPPFLAGS = -DNAME=\"xde-dock\"
xde_dock_SOURCES = xde-dock.c
xde_dock_CFLAGS = $(AM_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS)
xde_dock_LDFLAGS = $(SMLIB_LIBS) $(ICE_LIBS) $(AM_LDFLAGS)

xde_feedback_CPPFLAGS = -DNAME=\"xde-feedback\"
xde_feedback_SOURCES = xde-feedback.c
xde_feedback_CFLAGS = $(AM_CFLAGS) $(NOTIFY_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS) $(SN_CFLAGS)
xde_feedback_LDFLAGS = $(SN_LIBS) $(SMLIB_LIBS) $(ICE_LIBS) $(NOTIFY_LIBS) $(AM_LDFLAGS)

xde_input_CPPFLAGS = -DNAME=\"xde-input\"
xde_input_SOURCES = xde-input.c
xde_input_CFLAGS = $(AM_CFLAGS) $(XF86MISC_CFLAGS) $(NOTIFY_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS) $(SN_CFLAGS)
xde_input_LDFLAGS = $(SN_LIBS) $(SMLIB_LIBS) $(ICE_LIBS) $(XF86MISC_LIBS) $(NOTIFY_LIBS) -lm $(AM_LDFLAGS)

xde_keys_CPPFLAGS = -DNAME=\"xde-keys\"
xde_keys_SOURCES = xde-keys.c

xde_monitor_CPPFLAGS = -DNAME=\"xde-monitor\"
xde_monitor_SOURCES = xde-monitor.c
xde_monitor_CFLAGS = $(AM_CFLAGS) $(NOTIFY_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS) $(SN_CFLAGS)
xde_monitor_LDFLAGS = $(SN_LIBS) $(SMLIB_LIBS) $(ICE_LIBS) $(NOTIFY_LIBS) $(AM_LDFLAGS)

xde_pager_CPPFLAGS = -DNAME=\"xde-pager\"
xde_pager_SOURCES = xde-pager.c
xde_pager_CFLAGS = $(AM_CFLAGS) $(NOTIFY_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS) $(SN_CFLAGS) $(SOUND_CFLAGS)
xde_pager_LDFLAGS = $(SOUND_LIBS) $(SN_LIBS) $(SMLIB_LIBS) $(ICE_LIBS) $(NOTIFY_LIBS) $(AM_LDFLAGS)

xde_places_CPPFLAGS = -DNAME=\"xde-places\"
xde_places_SOURCES = xde-places.c

xde_preferred_CPPFLAGS = -DNAME=\"xde-preferred\"
xde_preferred_SOURCES = xde-preferred.c

xde_recent_CPPFLAGS = -DNAME=\"xde-recent\"
xde_recent_SOURCES = xde-recent.c

xde_run_CPPFLAGS = -DNAME=\"xde-run\"
xde_run_SOURCES = xde-run.c

xde_select_CPPFLAGS = -DNAME=\"xde-select\"
xde_select_SOURCES = xde-select.c
xde_select_CFLAGS = $(AM_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS)
xde_select_LDFLAGS = $(SMLIB_LIBS) $(ICE_LIBS) $(AM_LDFLAGS)

xde_setbg_CPPFLAGS = -DNAME=\"xde-setbg\"
xde_setbg_SOURCES = xde-setbg.c
xde_setbg_CFLAGS = $(AM_CFLAGS) $(NOTIFY_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS) $(SN_CFLAGS)
xde_setbg_LDFLAGS = $(SN_LIBS) $(SMLIB_LIBS) $(ICE_LIBS) $(NOTIFY_LIBS) $(AM_LDFLAGS)

xde_tasks_CPPFLAGS = -DNAME=\"xde-tasks\"
xde_tasks_SOURCES = xde-tasks.c
xde_tasks_CFLAGS = $(AM_CFLAGS) $(NOTIFY_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS) $(SN_CFLAGS)
xde_tasks_LDFLAGS = $(SN_LIBS) $(SMLIB_LIBS) $(ICE_LIBS) $(NOTIFY_LIBS) $(AM_LDFLAGS)

xde_winds_CPPFLAGS = -DNAME=\"xde-winds\"
xde_winds_SOURCES = xde-winds.c
xde_winds_CFLAGS = $(AM_CFLAGS) $(NOTIFY_CFLAGS) $(ICE_CFLAGS) $(SMLIB_CFLAGS) $(SN_CFLAGS)
xde_winds_LDFLAGS = $(SN_LIBS) $(SMLIB_LIBS) $(ICE_LIBS) $(NOTIFY_LIBS) $(AM_LDFLAGS)

xde_winlist_CPPFLAGS = -DNAME=\"xde-winlist\"
xde_winlist_SOURCES = xde-winlist.c

xde_winmenu_CPPFLAGS = -DNAME=\"xde-winmenu\"
xde_winmenu_SOURCES = xde-winmenu.c

xde_wkspmenu_CPPFLAGS = -DNAME=\"xde-wkspmenu\"
xde_wkspmenu_SOURCES = xde-wkspmenu.c


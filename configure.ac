
AC_PREREQ(2.69)
AC_INIT([xde-ctools],[1.4.10],[http://github.com/bbidulock/xde-ctools],[xde-ctools],[http://github.com/bbidulock/xde-ctools/issues])
AC_REVISION([1.4.10])

AC_CONFIG_SRCDIR([src/xde-dock.c])
AC_CONFIG_HEADERS([autoconf.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([scripts])
AC_USE_SYSTEM_EXTENSIONS

DATE='2016-07-17'
AC_SUBST([DATE])

AM_INIT_AUTOMAKE([gnits 1.14 dist-xz no-dist-gzip std-options -Wall color-tests silent-rules tar-pax])
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR
AC_PROG_MKDIR_P
LT_PATH_LD

AC_DISABLE_STATIC
LT_PREREQ([2.4.2])
LT_INIT([dlopen])
LT_LANG([C])
AC_SUBST([LIBTOOL_DEPS])

PKG_PROG_PKG_CONFIG([0.14.0])

# Checks for libraries.

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h locale.h stddef.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gethostname memmove select strcasecmp strchr strcspn strdup strerror strncasecmp strndup strpbrk strrchr strspn strstr strtol strtoul])

PKG_CHECK_MODULES([X11],[x11])
PKG_CHECK_MODULES([XEXT],[xext])
PKG_CHECK_MODULES([XAU],[xau])
PKG_CHECK_MODULES([XSCRNSAVER],[xscrnsaver])
PKG_CHECK_MODULES([XF86MISC],[xxf86misc])
PKG_CHECK_MODULES([XDMCP],[xdmcp])
PKG_CHECK_MODULES([ICE],[ice])
PKG_CHECK_MODULES([SMLIB],[sm])
PKG_CHECK_MODULES([XFT],[xft])
PKG_CHECK_MODULES([GDK],[gdk-pixbuf-xlib-2.0])
PKG_CHECK_MODULES([GLIB],[glib-2.0])
PKG_CHECK_MODULES([GOBJ],[gobject-2.0])
PKG_CHECK_MODULES([DBUS_GLIB],[dbus-glib-1])
PKG_CHECK_MODULES([GIO],[gio-2.0])
PKG_CHECK_MODULES([GIO_UNIX],[gio-unix-2.0])
PKG_CHECK_MODULES([GTK],[gtk+-2.0 >= 2.4.0])
PKG_CHECK_MODULES([UNIQUE],[unique-1.0])
PKG_CHECK_MODULES([WNCK],[libwnck-1.0])
PKG_CHECK_MODULES([CAIRO],[cairo])
PKG_CHECK_MODULES([SYSTEMD],[libsystemd])

AC_ARG_ENABLE([xrandr],
	AC_HELP_STRING([--disable-xrandr],
		[Disable XRANDR support @<:@default=auto@:>@]))
if test "x$enable_xrandr" != xno ; then
	PKG_CHECK_MODULES([XRANDR],[xrandr],
		[AC_DEFINE([XRANDR],[1], [Define to 1 to support XRANDR extension.])],
		[enable_xrandr=no])
fi
AC_MSG_CHECKING([for XRANDR support])
AC_MSG_RESULT([${enable_xrandr:-yes}])

AC_ARG_ENABLE([xinerama],
	AC_HELP_STRING([--disable-xinerama],
		[Disable XINERAMA support @<:@default=auto@:>@]))
if test "x$enable_xinerama" != xno ; then
	PKG_CHECK_MODULES([XINERAMA],[xinerama],
		[AC_DEFINE([XINERAMA],[1], [Define to 1 to support XINERAMA extension.])],
		[enable_xinerama=no])
fi
AC_MSG_CHECKING([for XINERAMA support])
AC_MSG_RESULT([${enable_xinerama:-yes}])

AC_ARG_ENABLE([startup-notification],
	AC_HELP_STRING([--disable-startup-notification],
		[Disable startup notification support @<:@default=auto@:>@]))
if test "x$enable_startup_notification" != xno ; then
	PKG_CHECK_MODULES([SN],[libstartup-notification-1.0],
		[AC_DEFINE([STARTUP_NOTIFICATION],[1], [Define to 1 to support startup notification.])],
		[enable_startup_notification=no])
fi
AC_MSG_CHECKING([for startup notification support])
AC_MSG_RESULT([${enable_startup_notification:-yes}])

AC_ARG_ENABLE([notify],
	AC_HELP_STRING([--disable-notify],
		[Disable notification daemon notifications @<:@default=auto@:>@]))
if test "x$enable_notify" != xno ; then
	PKG_CHECK_MODULES([NOTIFY],[gio-2.0 gio-unix-2.0 glib-2.0 gdk-pixbuf-2.0 libnotify],
		[AC_DEFINE([DESKTOP_NOTIFICATIONS],[1],[Define to 1 to support desktop notifications.])],
		[enable_notify=no])
fi
AC_MSG_CHECKING([for notification-daemon support])
AC_MSG_RESULT([${enable_notify:-yes}])
AM_CONDITIONAL([HAVE_DESKTOP_NOTIFICATIONS],[test  "x$enable_notify" != "xno"])

AC_ARG_ENABLE([status-icon],
	AC_HELP_STRING([--disable-status-icon],
		[Disable status icon in system tray @<:@default=auto@:>@]))
if test "x$enable_status_icon" != xno ; then
	PKG_CHECK_MODULES([GTK2],[gtk+-2.0 >= 2.10],
		[AC_DEFINE([SYSTEM_TRAY_STATUS_ICON],[1],[Define to 1 to support system tray icon.])],
		[enable_status_icon=no])
fi
AC_MSG_CHECKING([for status-icon support])
AC_MSG_RESULT([${enable_status_icon:-yes}])
AM_CONDITIONAL([HAVE_SYSTEM_TRAY_STATUS_ICON],[test  "x$enable_status_icon" != "xno"])

AC_ARG_ENABLE([debug],
	AC_HELP_STRING([--enable-debug],
		[Enable debugging support @<:@default=disabled@:>@]))
if test "x$enable_debug" = xyes ; then
	AC_DEFINE_UNQUOTED([DEBUG],[],[Define to enable debugging])
fi
AC_MSG_CHECKING([for debug support])
AC_MSG_RESULT([${enable_debug:-no}])

test "x$prefix" = xNONE && prefix="$ac_default_prefix"

sharedir="$datadir"
eval "sharedir=\"$sharedir\""
eval "sharedir=\"$sharedir\""
eval "sharedir=\"$sharedir\""
AC_SUBST([sharedir])

xdgconfdir="$sysconfdir"
eval "xdgconfdir=\"$xdgconfdir\""
eval "xdgconfdir=\"$xdgconfdir\""
eval "xdgconfdir=\"$xdgconfdir\""
AC_SUBST([xdgconfdir])

xdgdatadir="$prefix"
eval "xdgdatadir=\"$xdgdatadir\""
eval "xdgdatadir=\"$xdgdatadir\""
eval "xdgdatadir=\"$xdgdatadir\""
AC_SUBST([xdgdatadir])

autostartdir='$(xdgconfdir)/xdg/autostart'
AC_SUBST([autostartdir])

applicationsdir='$(xdgdatadir)/share/applications'
AC_SUBST([applicationsdir])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 man/Makefile
		 data/Makefile
		 ])
AC_OUTPUT

# vim: set ft=config:
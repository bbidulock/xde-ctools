[xde-ctools -- release notes.  2018-01-06]: #

Maintenance Release 1.8
=======================

This is the eighth release of the xde-ctools package.  This package
contains various tools (menus, popup, pages, feedback mechanisms) for
the _X Desktop Environment (XDE)_.  These tools are meant to have
minimal dependencies, GTK2-based, and each tool able to run independent
of the others (and for that matter, independent of the desktop
environment): they work well with many light-weight window managers.
Not all the tools are complete, but the many that are work nicely.  The
nice ones I use daily and are quite stable.

This release is a maintenance release and fixes some issues with GNU
options.  It also adds the xde-sound feedback utility that plays event
sound effects in response to desktop events.  It fixes a crash in
xde-setbg when it is run too early (before any desktops are set up at
all).

Included in the release is an autoconf tarball for building the release
from source.  See the [NEWS](NEWS) and [TODO](TODO) file in the release
for more information.

[ vim: set ft=markdown sw=4 tw=72 nocin nosi fo+=tcqlorn spell: ]: #

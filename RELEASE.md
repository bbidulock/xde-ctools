[xde-ctools -- release notes.  2019-09-03]: #

Maintenance Release 1.10
========================

This is the tenth release of the xde-ctools package.  This package
contains various tools (menus, popup, pages, feedback mechanisms) for
the _X Desktop Environment (XDE)_.  These tools are meant to have
minimal dependencies, GTK2-based, and each tool able to run independent
of the others (and for that matter, independent of the desktop
environment): they work well with many light-weight window managers.
Not all the tools are complete, but the many that are work nicely.  The
nice ones I use daily and are quite stable.

This release is a maintenance release that simply provides better
autoconf directory defaults and improves the build process more.
It also gets around gcc 9.1 compiler string truncation stupidity.

Included in the release is an autoconf tarball for building the package
from source.  See the [NEWS](NEWS) and [TODO](TODO) file in the release
for more information.  Please report problems to the issues list on
[GitHub](https://github.com/bbidulock/xde-ctools/issues).

[ vim: set ft=markdown sw=4 tw=72 nocin nosi fo+=tcqlorn spell: ]: #

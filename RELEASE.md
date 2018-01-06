[xde-ctools -- release notes.  2018-01-06]: #

Maintenance Release 1.7
=======================

This is the seventh release of the xde-ctools package.  This package
contains various tools (menus, popup, pages, feedback mechanisms) for
the _X Desktop Environment (XDE)_.  These tools are meant to have
minimal dependencies, GTK2-based, and each tool able to run independent
of the others (and for that matter, independent of the desktop
environment): they work well with many light-weight window managers.
Not all the tools are complete, but the many that are work nicely.  The
nice ones I use daily and are quite stable.

This release is a maintenance release and fixes some issues with startup
notification.  It also adds an opacity option to most of the tools for
use with composite managers.  Also, some file managers do not like the
file:// prefix on file names at all (e.g. pcmanfm) so xde-recent now
strips them.  Centered dialogues now use center window gravity to place
themselves in the center of the screen.  xde-pager now correctly updates
its layout when the number of workspaces changes, and behaves better
when hot keys are pressed while it is popped.  Manual page updates.

Starting with this release, commits are verified and tags are signed and
annotated (with this text).

Includes in the release is an Arch Linux source tarball (also on the
AUR) and an autoconf tarball for building the release from source.
See the [NEWS](NEWS) and [TODO](TODO) file in the release for more
information.

[ vim: set ft=markdown sw=4 tw=72 nocin nosi fo+=tcqlorn spell: ]: #

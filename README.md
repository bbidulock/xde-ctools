
## xde-ctools

Package xde-ctools-0.1.34 was released under GPL license 2014-07-22.

This package provides a number of `C`-language tools for working with
the X Desktop Envionment.  Most of these tools were previously written
in perl(1) and were part of the xde-tools package.  They have now been
codified in `C` for speed and to provide access to libraries not
available from perl(1).

### Release

This is the `xde-ctools-0.1.34` package, released 2014-07-22.  This release,
and the latest version, can be obtained from the GitHub repository at
http://github.com/bbidulock/xde-ctools, using a command such as:

```bash
git clone http://github.com/bbidulock/xde-ctools.git
```

Please see the [NEWS](NEWS) file for release notes and history of user visible
changes for the current version, and the [ChangeLog](ChangeLog) file for a more
detailed history of implementation changes.  The [TODO](TODO) file lists
features not yet implemented and other outstanding items.

Please see the [INSTALL](INSTALL) file for installation instructions.

When working from `git(1)', please see the [README-git](README-git) file.  An
abbreviated installation procedure that works for most applications
appears below.

This release is published under the GPL license that can be found in
the file [COPYING](COPYING).

### Quick Start:

The quickest and easiest way to get xde-ctools up and running is to run
the following commands:

```bash
git clone http://github.com/bbidulock/xde-ctools.git xde-ctools
cd xde-ctools
./autogen.sh
./configure --prefix=/usr
make V=0
make DESTDIR="$pkgdir" install
```

This will configure, compile and install xde-ctools the quickest.  For
those who would like to spend the extra 15 seconds reading `./configure
--help`, some compile time options can be turned on and off before the
build.

For general information on GNU's `./configure`, see the file [INSTALL](INSTALL).

### Running xde-ctools

Read the manual pages after installation:

    man xde-ctools

### Issues

Report issues to http://github.com/bbidulock/xde-ctools/issues.


xde_man_files = \
	    xde-dock.man

nodist_man1_MANS = $(xde_man_files)
CLEANFILES = $(xde_man_files)

EXTRA_DIST = \
	     xde-dock.pod

xde-dock.man: MANNAME = XDE-DOCK
xde-dock.man: xde-dock.pod

SUFFIXES = .man .pod

.pod.man:
	pod2man --center="User Commands" \
		--date="$(DATE)" \
		--errors="none" \
		--fixed="CW" \
		--fixedbold="CB" \
		--fixeditalic="CI" \
		--fixedbolditalic="CX" \
		--name="$(MANNAME)" \
		--nourls \
		--release="$(PACKAGE) $(VERSION)" \
		--section="1" \
		$< >$@


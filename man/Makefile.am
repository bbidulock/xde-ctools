
xde_man_files = \
	xde-app.man \
	xde-clip.man \
	xde-ctools.man \
	xde-cycle.man \
	xde-dock.man \
	xde-keys.man \
	xde-pager.man \
	xde-preferred.man \
	xde-run.man \
	xde-setbg.man \
	xde-winlist.man \
	xde-winmenu.man

nodist_man1_MANS = $(xde_man_files)
CLEANFILES = $(xde_man_files)

EXTRA_DIST = \
	xde-app.pod \
	xde-clip.pod \
	xde-ctools.pod \
	xde-cycle.pod \
	xde-dock.pod \
	xde-keys.pod \
	xde-pager.pod \
	xde-preferred.pod \
	xde-run.pod \
	xde-setbg.pod \
	xde-winlist.pod \
	xde-winmenu.pod

SUFFIXES = .man .pod

.pod.man:
	pod2man --center="User Commands" \
		--date="$(DATE)" \
		--errors="none" \
		--fixed="CW" \
		--fixedbold="CB" \
		--fixeditalic="CI" \
		--fixedbolditalic="CX" \
		--nourls \
		--release="$(PACKAGE) $(VERSION)" \
		--section="1" \
		$< >$@


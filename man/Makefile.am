
xde_man_files = \
	xde-app.man \
	xde-dock.man \
	xde-preferred.man \
	xde-run.man \
	xde-winmenu.man \
	xde-winlist.man \
	xde-pager.man \
	xde-cycle.man \
	xde-keys.man \
	xde-setbg.man

nodist_man1_MANS = $(xde_man_files)
CLEANFILES = $(xde_man_files)

EXTRA_DIST = \
	xde-app.man \
	xde-dock.pod \
	xde-preferred.man \
	xde-run.pod \
	xde-winmenu.pod \
	xde-winlist.pod \
	xde-pager.pod \
	xde-cycle.pod \
	xde-keys.pod \
	xde-setbg.pod

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


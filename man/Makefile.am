
xde_man_files = \
	xde-app.man \
	xde-clip.man \
	xde-ctools.man \
	xde-cycle.man \
	xde-dock.man \
	xde-input.man \
	xde-keys.man \
	xde-launch.man \
	xde-monitor.man \
	xde-open.man \
	xde-pager.man \
	xde-places.man \
	xde-preferred.man \
	xde-recent.man \
	xde-run.man \
	xde-select.man \
	xde-setbg.man \
	xde-winlist.man \
	xde-winmenu.man \
	xde-wkspmenu.man

nodist_man1_MANS = $(xde_man_files)
CLEANFILES = $(xde_man_files)

EXTRA_DIST = \
	xde-app.pod \
	xde-clip.pod \
	xde-ctools.pod \
	xde-cycle.pod \
	xde-dock.pod \
	xde-input.pod \
	xde-keys.pod \
	xde-launch.pod \
	xde-monitor.pod \
	xde-open.pod \
	xde-pager.pod \
	xde-places.pod \
	xde-preferred.pod \
	xde-recent.pod \
	xde-run.pod \
	xde-select.pod \
	xde-setbg.pod \
	xde-winlist.pod \
	xde-winmenu.pod \
	xde-wkspmenu.pod

SUFFIXES = .man .pod

.pod.man:
	$(AM_V_GEN)pod2man --center="User Commands" \
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



xde_man_files = \
	xde-dock.man \
	xde-launch.man \
	xde-preferred.man \
	xde-run.man

nodist_man1_MANS = $(xde_man_files)
CLEANFILES = $(xde_man_files)

EXTRA_DIST = \
	xde-dock.pod \
	xde-launch.man \
	xde-preferred.man \
	xde-run.pod

xde-dock.man: MANNAME = XDE-DOCK
xde-dock.man: xde-dock.pod

xde-launch.man: MANNAME = XDE-LAUNCH
xde-launch.man: xde-launch.pod

xde-preferred.man: MANNAME = XDE-PREFERRED
xde-preferred.man: xde-preferred.pod

xde-run.man: MANNAME = XDE-RUN
xde-run.man: xde-run.pod

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


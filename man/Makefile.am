
xde_man_files = \
	xde-app.man \
	xde-dock.man \
	xde-preferred.man \
	xde-run.man

nodist_man1_MANS = $(xde_man_files)
CLEANFILES = $(xde_man_files)

EXTRA_DIST = \
	xde-app.man \
	xde-dock.pod \
	xde-preferred.man \
	xde-run.pod

xde-app.man: MANNAME = XDE-APP
xde-app.man: xde-app.pod

xde-dock.man: MANNAME = XDE-DOCK
xde-dock.man: xde-dock.pod

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


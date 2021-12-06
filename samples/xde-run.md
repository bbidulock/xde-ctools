---
layout: default
---
Sample Screenshots
===============

xde-run
---------------

Provides a run command dialog for running applications from the command
line using a graphical utility.

<div style="text-align: center;" markdown="1">
___Figure 1:___  __Run Command -- Centered__
![xde-run.png](/scrot/xde-run.png "Run Command Dialog -- Centered")
</div>

___Figure 1___, above, illustrates the run command dialog centered in the
middle of the screen.  This is the only placement.  Placement cannot be
controlled with options.

The image shows the run command dialog launched with the ``adwm``
window manager and using the ``squared-blue`` GTK (window contents) and
ADWM (window decorations) theme.

<div style="text-align: center;" markdown="1">
___Figure 2:___  __Run Command Dialog__
![xde-run.2.png](/scrot/xde-run.2.png "Run Command Dialog")
</div>

___Figure 2___, above, shows how the run command dialog appears.

The image shows the run command dialog launched with the ``adwm``
window manager and using the ``squared-cadet`` GTK (window contents) and
ADWM (window decorations) theme.

As with a number of other ``XDE`` applications, the dialog attempts to
get it's theme settings from the ``~/.gtkrc-2.0.xde`` file instead of
the ``~/.gtkrc-2.0`` file so that it can maintain consistency with the
window manager decorations instead of the applications.

<div style="text-align: center;" markdown="1">
___Figure 3:___  __Run Command Dialog -- Command Selection__
![xde-run.3.png](/scrot/xde-run.3.png "Run Command Dialog -- Command Selection")
</div>

As illustrated in ___Figure 3___, above, when the drop-down selection
for the command is invoked, a history of recent commands is shown.  The
number of recent commands shown and stored, as well as the file from
which they are shown or to which they are stored, can be controlled
using options.  See ``man xde-run`` for a list of options.


[ vim: set ft=markdown sw=4 tw=72 nocin nosi fo+=tcqlorn spell: ]: #

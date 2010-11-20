#!/usr/bin/python
#
# This script should be linked to a keyboard shortcut. Under gnome,
# you can do this from the main preferences menu, or directly execute
# "gnome-keybinding-properties"
#
# Make the script executable. Install it somewhere in the executable
# path ("echo $PATH" to check what's in there), and then just enter
# its name as the action to perform, or copy it anywhere and copy the
# full path as the action.

import gtk
#import gtk.gdk
import wnck
import os
import sys
import time


def main():
    # We try to establish a timestamp for the calls to activate(), but
    # usually fail (the event_peek() calls return None). 
    #
    # Try to find a nice default value. The x server timestamp is
    # millisecond from last reset, it wraps around in 49 days, half
    # the space is set aside for the past So a value just below 2**31
    # should be considered recent in most situations ?
    timestamp = 2**31 - 1
    screen = wnck.screen_get_default()
    while gtk.events_pending():
        event = gtk.gdk.event_peek()
        if event != None and event.get_time() != 0:
            timestamp = event.get_time()
        gtk.main_iteration()

    recollMain = ""
    recollwins = [];
    for window in screen.get_windows():
        if window.get_class_group().get_name() == "Recoll":
            # print "win name: [%s], class: [%s]" % \
            #      (window.get_name(), window.get_class_group().get_name())
            if window.get_name() == "Recoll":
                recollMain = window
            recollwins.append(window)

    if not recollMain:
        os.system("recoll&")
        sys.exit(0)

    # Check the main window state, and either activate or minimize all
    workspace = screen.get_active_workspace()
    if not recollMain.is_visible_on_workspace(workspace):
        for win in recollwins:
            win.move_to_workspace(workspace)
            win.activate(timestamp)
    else:
        for win in recollwins:
            win.minimize()

if __name__ == '__main__':
  main()
  

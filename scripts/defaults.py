import ob         # base module
import focus      # add some default focus handling and cycling functions
import focusmodel # default focus models
import behavior   # defines default behaviors for interaction with windows
import callbacks  # a lib of functions that can be used as binding callbacks
import windowplacement # use a routine in here to place windows
import historyplacement # history window placement

# try focus something when nothing is focused
focus.FALLBACK = 1

# choose a default focus model
focusmodel.setup_click_focus() # use focusmodel.setup_sloppy_focus() instead to
                               # make focus follow the cursor, or bind things
                               # in some way like these functions do to make
                               # your own custom focus model.
# set up the mouse buttons
behavior.setup_window_clicks()
behavior.setup_window_buttons()
behavior.setup_scroll()

# my window placement algorithm
#ob.ebind(ob.EventAction.PlaceWindow, windowplacement.random)
ob.ebind(ob.EventAction.PlaceWindow, historyplacement.place)
# don't place terminals by history placement (xterm,aterm,rxvt)
def histplace(data):
    if data.client.appClass() == "XTerm": return 0
    return 1
historyplacement.CONFIRM_CALLBACK = histplace


# run xterm from root clicks
ob.mbind("Left", ob.MouseContext.Root, ob.MouseAction.Click,
         lambda(d): ob.execute("xterm", d.screen))

ob.kbind(["A-F4"], ob.KeyContext.All, callbacks.close)

ob.kbind(["W-d"], ob.KeyContext.All, callbacks.toggle_show_desktop)

# focus bindings

from cycle import CycleWindows
ob.kbind(["A-Tab"], ob.KeyContext.All, CycleWindows.next)
ob.kbind(["A-S-Tab"], ob.KeyContext.All, CycleWindows.previous)

# if you want linear cycling instead of stacked cycling, comment out the two
# bindings above, and use these instead.
#from cycle import CycleWindowsLinear
#ob.kbind(["A-Tab"], ob.KeyContext.All, CycleWindows.next)
#ob.kbind(["A-S-Tab"], ob.KeyContext.All, CycleWindows.previous)

from cycle import CycleDesktops
ob.kbind(["C-Tab"], ob.KeyContext.All, CycleDesktops.next)
ob.kbind(["C-S-Tab"], ob.KeyContext.All, CycleDesktops.previous)

# desktop changing bindings
ob.kbind(["C-1"], ob.KeyContext.All, lambda(d): callbacks.change_desktop(d, 0))
ob.kbind(["C-2"], ob.KeyContext.All, lambda(d): callbacks.change_desktop(d, 1))
ob.kbind(["C-3"], ob.KeyContext.All, lambda(d): callbacks.change_desktop(d, 2))
ob.kbind(["C-4"], ob.KeyContext.All, lambda(d): callbacks.change_desktop(d, 3))
ob.kbind(["C-A-Right"], ob.KeyContext.All,
         lambda(d): callbacks.right_desktop(d))
ob.kbind(["C-A-Left"], ob.KeyContext.All,
         lambda(d): callbacks.left_desktop(d))
ob.kbind(["C-A-Up"], ob.KeyContext.All,
         lambda(d): callbacks.up_desktop(d))
ob.kbind(["C-A-Down"], ob.KeyContext.All,
         lambda(d): callbacks.down_desktop(d))

ob.kbind(["C-S-A-Right"], ob.KeyContext.All,
         lambda(d): callbacks.send_to_next_desktop(d))
ob.kbind(["C-S-A-Left"], ob.KeyContext.All,
         lambda(d): callbacks.send_to_prev_desktop(d))

# focus new windows
ob.ebind(ob.EventAction.NewWindow, callbacks.focus)

print "Loaded defaults.py"

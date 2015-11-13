# Introduction #

GNOME MPlayer is a simple GUI for MPlayer. It is intended to be a nice tight player and provide a simple and clean interface to MPlayer. GNOME MPlayer has a rich API that is exposed via DBus. Using DBus you can control a single or multiple instances of GNOME MPlayer from a single command.

The player can be used to play media on websites when used with Gecko Mediaplayer


# Details #
```
0.6.0 	Feb 13, 2008
GUI Layout Changes
Updated Playlist support and layout
Fix several memory leaks and idle callback leaks
Several new Preference options including support for subtitle and audio tracks

0.5.4 	Jan 17, 2008
Enchanced CD and DVD title/track support
Updated translations
Support for Gnome Multimedia keys
Several UI Updates

0.5.3 	Dec 13, 2007
Longer URL support
Updated translations

0.5.2 	Nov 02, 2007
ChangeLog for 0.5.2 is large so this is a link to it.

0.5.1 	Oct 4, 2007
ChangeLog for 0.5.1 is large so this is a link to it.

0.5.0 	Aug 29, 2007
Playlist Support
ChangeLog for 0.5.0 is large so this is a link to it.

0.4.7 	Jul 18, 2007
ChangeLog for 0.4.7 is large so this is a link to it.

0.4.6 	May 20, 2007
Improved UI
Disables gnome-screensaver while playing back video

0.4.5 	Apr 13, 2007
ChangeLog for 0.4.5 is large so this is a link to it.

0.4.4 	Mar 21, 2007
Compatibility with gecko-mediaplayer 0.0.4
ChangeLog for 0.4.4 is large so this is a link to it.

0.4.3 	Mar 12, 2007
Compatibility with gecko-mediaplayer 0.0.3
Fix pause problem when volume is changed
Switch to using control_id for communication between browser and player
Implement DBUS method GetCacheSize
Implement DBUS signal SetCachePercent
Implement more DBUS methods GetTime, GetDuration, GetPercent
Implement more DBUS methods GetFullScreen, GetShowControls
Implement DBUS introspection and several method calls.
Allow setting cache size using slider bar in preferences dialog
Fix update-po process
DBus methods FastForward, FastReverse, Seek
Update volume control so that we can set it before the media starts playing
More GNOMEification
Added Quit to the right click menu
Changed file selector to GNOME selector (Louis-Francis Ratté-Boulianne)

0.4.2 	Feb 22,2007
Compatibility with gecko-mediaplayer 0.0.2
When streaming and stopped... kill the thread
Only show media length when we know it
Implement DBUS Signal
ResizeWindow int32: int32:
Emit "Next" signal when thread is complete and embedded
Hide ff, rew and fs buttons when width < 250 and embedded
Set progress bar value and text correctly at end of stream
Implement progress status (time) in progress bar
Implement seeking by clicking on progress bar
Detect simple playlists of rtsp and http urls
Fix check mark out of sync on Full Screen option
Detect [reference] playlists makes deejay.it work
Emit "RequestById string:" signal when after OpenButton
is clicked and we have to request the id of the href
Implement DBUS Signals
OpenButton string:url string:hrefid to request from caller
SetShowControls boolean:
Fix embedding with metacity window manager

0.4.1 	Compatibility with gecko-mediaplayer 0.0.1
Fix problem with status info appearing when embedded and video playing
When running in embedded mode, emit Ready signal to com.gecko.browser
Change versioning scheme to x.y.z

0.4 	Feb 10,2006
Updated GUI
DBUS interface, documented in DOCS/tech/dbus.txt
Fully embeddedable using XEmbed
International support (en and partial es)

0.3 	Dec 16,2006
Detect PLS files automatically and pass the -playlist arg if needed.
Can force playlist passing by setting -playlist flag on command line
Add Entry (RO) to display the media title/ICY info
ICY Stream work
Put Volume % in tooltip of volume widget
Display message when caching, so we know it is doing something
Flatten code so that only one play_ command is needed
Fix problem with default working directory
Reworked the automake files so that 'make dist' works properly.

0.2 	Nov 6,2006
Add support for CD Audio playing (no track selection yet, just plays disk)
Add support for DVD playing, can hook up to "Removable Devices and Media" and set it to be "gnome-mplayer %d". Replace totem, since it never works
Add "Open..." to right click menu. So we can open files from the GUI
Fix garbage left on screen after window resize (slight flicker due to this, but minimal)

0.1 	Oct 25, 2006
Initial Release
```
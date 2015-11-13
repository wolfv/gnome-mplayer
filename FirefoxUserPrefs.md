# Introduction #

For sites that provide direct mms or rtsp urls, you can these options to firefox and gnome-mplayer will handle them.

# Details #


To set GNOME MPlayer as the default player for mms and rtsp protocols in Mozilla/Firefox add this to your prefs.js file
```
user_pref("network.protocol-handler.app.mms", "/usr/bin/gnome-mplayer");
user_pref("network.protocol-handler.app.rtsp", "/usr/bin/gnome-mplayer");
```

Or open the "about:config" location and create new string settings with the above values.
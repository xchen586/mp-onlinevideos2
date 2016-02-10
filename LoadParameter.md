

# Introduction #

Starting with MediaPortal 1.2 a designer can add a button in the skinfile that calls any Plugin with a parameter (`<hyperlinkParameter>`). This allows calling OnlineVideos showing a given Site, Category or results of a search. You can also use GuiProperties inside the parameter, they are resolved before handed to the plugin.

# Details #

This is the full parameter specification (each term is optional, order is irrelevant):

group:`<groupname>`|site:`<sitename>`|category:`<categoryname>`|search:`<searchstring>`|VKonfail:`<true,false>`|return:`<Locked,Root>`

Settings theses terms (seperated by |) will allow going directly to:

  1. a Group of sites
  1. a Site (not found = stay at 'All Sites view')
  1. a Category of a Site (not found = stay at 'All Categories View')
  1. the searchresults of a site (nothing found =
    * return after showing 'Nothing Found message'
    * open virtual keyboard with given searchstring (default))

with one of these return modes (PreviousMenu action):

  1. Go back up to the all sites view of OnlineVideos before returning to the window you came from (default)
  1. When at the view OnlineVideos was called with, PreviousMenu will return to the window you came from

## Downloading ##

There are 3 more settings that are useful when calling OnlineVideos to search for a trailer (e.g. from the MyFilms plugin) that allow the plugin dev to specify a folder, filename and menuentry for downloading the trailer. These settings are only respected when the return mode is locked.

downloaddir:`<path>`|downloadfilename:`<filename>`|downloadmenuentry:`<menu text>`

# Examples #

A button in myVideos or MovingPictures with the following hyperlinkParameter:<br />
`site:iTunes Movie Trailers|search:#selecteditem|return:Locked`<br />
will go to the searchresults of the 'iTunes Movie Trailers' site for the currently selected movie and return back to myVideos or MovingPictures if PreviousMenu is pressed in the searchresults view. If no result is found it will open the virtual keyboard with the given movie name, so the user can modify it and search again.


A button in myVideos that searches trailers via the IMDB ID (when set) or the title otherwise on the IMDB site:<br />
`site:IMDb Movie Trailers|search:#(iif(neq(#imdbnumber,''),#imdbnumber,#selecteditem))|return:Locked`<br />


A button in myTV with hyperlinkParameter set to:<br />
`site:#TV.View.channel`<br />
will go to a site named like the current channel (if found) and return back to tv only when using PreviousMenu from the list of all sites.
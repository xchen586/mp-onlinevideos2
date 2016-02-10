# Table of Contents #


---

# Introduction #
This plugin integrates websites that host videos or stream live tv into <a href='http://www.team-mediaportal.com/'>MediaPortal</a>.
Examples are <a href='http://www.youtube.com/'>YouTube</a> and <a href='http://www.wwitv.com/'>wwitv</a>.

The videos/streams can be discovered by reading RSS feeds, parsing HTML, programming against provided APIs or directly adding urls of live streams. Only direct links to video files or streams (.flv, .mp4, ...) are supported, because they will be rendered using <a href='http://wikipedia.org/wiki/DirectShow'>directshow</a>. This allows usage of your favorite codecs to control audio and video quality (e.g. using ffdshow you can upscale low quality videos as well as upmix stereo sound and output to spdif) and gives the best integration with MediaPortal.


---

# Features #
  * automatic (by language) and user defined grouping of sites
  * thumbnails for sites, categories and videos/streams
  * pin to protect adult sites
  * search (on sites that support search)
  * paging in video and category lists
  * download videos (concurrent or queued)
  * multiple quality playback choices
  * add videos and categories to a Favorites site
  * play all videos currently shown
  * play with Windows Media Player, <a href='http://www.videolan.org/vlc/'>VLC media player</a>
  * automatic updating of sites (with restarting)
  * integrated SiteManager to add and remove sites
  * configuration of plugin and site settings (without restarting)
  * fully localized interface

---

# User Interface #
The main view of the plugin at first shows a list of languages represented by flags that is build from all sites in your personal list. This automatic grouping of sites can be disabled or customized. Selecting a group will show all sites for it. By selecting a site you can dive into the categories for that site. If a category has subcategories (e.g. Miro), by selecting a category you will dive into the subcategories recursivly, otherwise a list of videos for that category is shown. Each of these 4 views  can be shown as a list, small thumbs or large thumbs. Some sites support a details view that provides more information and a list of clips for a movie.
## groups view ##
The default view, that will be shown upon entering the plugin.<br />
<img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_AutoGroupByLanguage.jpg' />
## sites view ##
You can order the list of sites by Name or as defined in your local xml.<br />
<img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_Sites_SmallIcons.jpg' />
### site context menu ###
When a site has configuration options that the user can change, the context menu (F9) will allow to modify them.<br />
<img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_Sites_ContextMenu.jpg' />
## categories view ##
Shows a list of categories with thumbs. If the number of videos in a category is known, it will be displayed right aligned in the list mode.<br />
<img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_Categories.png' />
## videos view ##
Example for the video view with large icons. In the lower part, the title, a description and the runtime as well as the publication date (if known) of the selected clip are displayed.<br />
<img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_Videos_LargeIcons.jpg' />
### video context menu ###
  * play with ... to chose Internal, WMP or VLC player
  * play all videos currently showing
  * add to favorites / delete from favorites (when displaying _Favorites_)
  * download video (only visible when download folder was set in config)
  * delete file (only when showing _DownloadedVideos_)
  * custom entries depending on the site (e.g. show related videos on _YouTube_)
<img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_Videos_ContextMenu.jpg' />
## details view ##
This screen is only available on sites that have multiple choices for a video (e.g. iTunes Moview Trailers and IMDB)<br /><img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_DetailsView_AppleTrailers.jpg' />
## side menu ##
  * change layout: list, small icon, large icon
  * search (+category to search, if available)
  * max results, sort method, timeframe, refresh
  * Pin (enter the pin to enable adult content)
  * SiteManager (update, add or remove sites)
  * Settings (when the [MPEI plugin](https://code.google.com/p/mpei-plugin/) is installed)
<img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_Videos_SideMenu.jpg' />


---

# Configuration #
The configuration form for the OnlineVideos plugin has four tabs where you can configure all aspects of the plugin. After you have done your changes make sure you close the MediaPortal configuration tool pressing OK, to save all data stored only in memory.
## General ##
  * displayed name of the plugin in MediaPortal classic home screen
  * pin used for protection of sites with adult content
  * folder where downloaded videos should be saved
  * folder where thumbnails are saved
  * timeout for cached data from the internet
  * enable/disable extended search history<br /><img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_Configuration_General.jpg' />
## Groups ##
  * classify your sites into groups that will be shown as top level view when entering OnlineVideos
  * turn automatic grouping by language on/off
## Sites ##
  * change the order of sites
  * delete, create and import or publish/update sites
  * edit settings that the util of that site exposes (e.g. quality for videos on YouTube)<br /><img width='320' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_Configuration.png' />
### How to add live-streams ###
  * Create a new site (<img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/OnlineVideos.MediaPortal1/Resources/Add.png' />) or select the site where you want to add the live streams and press Edit (<img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/OnlineVideos.MediaPortal1/Resources/edit.png' />).
  * Create a new group (<img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/OnlineVideos.MediaPortal1/Resources/NewFolder.png' />) in the lower part and fill the name and optional url of a logo
  * Select that group and add a new stream(<img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/OnlineVideos.MediaPortal1/Resources/tv.png' />)
  * Fill the streamname, streamUrl (remember: this must be the direct url of the stream, and not some url to a page with a flashplayer playing that stream), and optionally the url of the logo.
  * Save, Start Mediaportal and enjoy.
## Codecs ##
On the Codec tab, you can test your system for playback of different video types. If playback succeeds here, it will tell you what filter is used to split the file into audio and video and you should have no problems playing files of that type in MediaPortal.

---

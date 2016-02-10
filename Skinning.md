

# General Info #
The DefaultWide skinfiles that are installed with the plugin are always kept updated and should be used as a reference when creating skinfiles.

The plugin comes with its own localization, so skinners are encouraged to use the provided properties for their labels.


---


# Window IDs #

OnlineVideos requires 4 window skin files:
  * Id: **4755** ( _myonlinevideos.xml_ )<br />The main OnlineVideos window where the user can browse sites and play videos.<br />
  * Id: **4757** ( _myonlinevideosUpdater.xml_ )<br />A secondary window that allows management of sites from the global list.<br />
  * Id: **4758** ( _myonlinevideosFullScreen.xml_ )<br />A copy of the default videoFullScreen, with the TV progress bar used instead of the default progress bar.<br />
  * Id: **4759** ( _myonlinevideosOSD.xml_ )<br />A copy of the default videoOSD, with the TV progress bar used instead of the default progress bar.

and 1 optional dialog window:
  * Id: **4760** ( _myonlinevideos.DialogMenu.xml_ )<br />A copy of the original DialogMenu.xml with an additional label below the list with `#OnlineVideos.DialogSelectedItemDescription` and the `textvisible3` of the list set to no. This dialog is used when configuring site settings (for example when pressing F9 whith focus on the YouTube site).


---


# Window 4755 (OnlineVideos) required controls #
| **Id** | **type** | **default localized label** | **description** |
|:-------|:---------|:----------------------------|:----------------|
| 2      | button   | `#OnlineVideos.Translation.LayoutList.Label` | Change facade viewmode |
| 5      | selectbutton | `#OnlineVideos.Translation.MaxResults.Label` | Max. results when filtering |
| 6      | selectbutton | `#OnlineVideos.Translation.SortOptions.Label` | Set sort criteria |
| 7      | selectbutton | `#OnlineVideos.Translation.Timeframe.Label` | Timeframe when filtering |
| 8      | button   | `#OnlineVideos.Translation.Refresh.Label` | Refresh view when filtering |
| 9      | selectbutton | `#OnlineVideos.Translation.Category.Label` | Category for search |
| 10     | button   | `#OnlineVideos.Translation.Search.Label` | Search videos on current site |
| 12     | button   | `#OnlineVideos.Translation.EnterPin.Label` | Enter Pin for adult sites |
| 50     | facadeview | `label1` - title, `label2` - language or length or number of childs |  main facade (required modes: list, large and small icons) |
| 51     | listcontrol | `label1` - title            | additional list used instead of facade in details view |
|47016   | button   | `#OnlineVideos.currentDownloads` | optional button that will go directly to the current downloads screen |

## facade info ##
The listcontrol of the facade needs to set the PinIcon width, height and offset, because it is used to indicate a favorited category.

The icon shown for each site in the facade can be overridden by the skin if a png in `[CurrentSkinFolder]\Media\OnlineVideos\Icons\` is found.
The same goes for the site's banner (`[CurrentSkinFolder]\Media\OnlineVideos\Banners\`).

A new icon is needed for an item in the facade for retrieving the next page videos: (`[CurrentSkinFolder]\Media\OnlineVideos\NextPage.png`).

The thumbnailpanel of the facade should set `keepaspectratio` to yes and `thumbzoom` to no, because the size and format of thumbnails   is not predictable for images from websites all around the world.


---


# Window 4757 (SiteManager) required controls #
| **Id** | **type** | **default localized label** | **description** |
|:-------|:---------|:----------------------------|:----------------|
| 50     | listcontrol | `label1` - site name, `label2` - language, `label3` - updatedate | main list       |
| 503    | selectbutton | `#OnlineVideos.Translation.Filter.Label: #OnlineVideos.Translation.State.Label` | State filter    |
| 504    | selectbutton | `#OnlineVideos.Translation.SortOptions.Label` | Set sort criteria |
| 506    | selectbutton | `#OnlineVideos.Translation.Filter.Label: #OnlineVideos.Translation.Creator.Label` | Creator filter  |
| 507    | selectbutton | `#OnlineVideos.Translation.Filter.Label: #OnlineVideos.Translation.Language.Label` | Language Filter |
| 508    | button   | `#OnlineVideos.Translation.AutomaticUpdate.Label` | invokes the automatic update |

The listcontrol (id 50) also sets the PinIcon for each listitem to an image file with this path: `[CurrentSkinFolder]\Media\OnlineVideos\[State].png`. The State is one of the three: `Broken, Reported, Working`. In the default skin, these states are represented by a red, yellow and green icon.


---


# Properties set in the code #
  * Global
    * `#OnlineVideos.HeaderLabel` - changes according to the name of the current site/category, ...
    * `#OnlineVideos.HeaderImage` - banner style image (3:1 AR) that changes according to the current site the user is browsing
    * `#OnlineVideos.Translation.ManageSites.Label` - can be used for the button that calls the site manager window
    * `#OnlineVideos.currentDownloads` - number of downloads currently running in the background
    * `#OnlineVideos.HomeScreenName` - user configured name for the OnlineVideos menu item

  * Site Manager Window (4757) only
    * `#OnlineVideos.owner` - Creator of the current selected item (localized label property: `#OnlineVideos.Translation.Creator.Label`)

  * OnlineVideos Window (4755) only
    * `#OnlineVideos.selectedSite` - name of the Site currently selected (e.g. "GameTrailers") that can be used for conditional visibility
    * `#OnlineVideos.selectedSiteUtil` - name of the Util used by the currently selected Site (e.g. "GenericSite") that can be used for conditional visibility
    * `#OnlineVideos.filter` - Current SMS T9 Filter
    * `#OnlineVideos.state` - Current View (values: groups, sites, categories, videos, details)
    * `#OnlineVideos.length` - runtime of selected item if `#OnlineVideos.state` = videos, (localized label property: `#OnlineVideos.Translation.Runtime.Label`)
    * `#OnlineVideos.aired` - airdate of selected item if `#OnlineVideos.state` = videos, (localized label property: `#OnlineVideos.Translation.Airdate.Label`)
    * `#selecteditem2`
      * groups view: number of sites in that group
      * sites view: language of the selected Site (localized label id for "Language": 248)
      * categories view: number of videos in that category (if known)
      * videos view: runtime or airdate (if available) of the current selected item

  * Facade (both Windows)
    * `#itemcount` - total number of items
    * `#itemtype` - localized name of items
    * `#selecteditem` - title of the current selected item
    * `#selectedthumb` - thumb (if available) for the current selected item
    * `#OnlineVideos.desc` - description (if available) of the current selected item

  * Currently Playing Video
    * `#Play.Current.Title` - title as in facade
    * `#Play.Current.Plot` - description
    * `#Play.Current.Thumb` - thumbnail as in facade
    * `#Play.Current.Year` - duration and airdate if available
    * `#Play.Current.OnlineVideos.SiteName` - name of the site that video originates from
    * `#Play.Current.OnlineVideos.SiteIcon` - path to the icon for the site that video originates from
    * `#OnlineVideos.IsBuffering` - "true" while a video is beeing buffered before playback will start
    * `#OnlineVideos.buffered` - float (2 digits + decimal point) showing percentage buffered (localized label property: `#OnlineVideos.Translation.Buffered.Label`)


---


## Videos View - Extended Infos ##
The user can toggle the extended infos by pressing the info key (F3) while in the videos view. Additional properties for the currently selected item should be shown by the skin. This mode is also reset when leaving videos view.
### YouTube ###
detect by (`string.equals(#OnlineVideos.state, videos) + string.equals(#OnlineVideos.ExtendedVideoInfo, True) + string.equals(#OnlineVideos.selectedSiteUtil, YouTube)`)
  * `#OnlineVideos.Details.Uploader`
  * `#OnlineVideos.Details.Rating`
  * `#OnlineVideos.Details.NumRaters`
  * `#OnlineVideos.Details.FavoriteCount`
  * `#OnlineVideos.Details.ViewCount`


---


## Details View ##

  * `#OnlineVideos.Details.Poster` - path to a poster format image

### iTunes Movie Trailers ###
detect by (`string.equals(#OnlineVideos.selectedSiteUtil, ITMovieTrailers)`)
  * `#OnlineVideos.Details.Plot` - (localized label property: `#OnlineVideos.Translation.PlotOutline.Label`)
  * `#OnlineVideos.Details.Genres` - (localized label property: `#OnlineVideos.Translation.Genre.Label`)
  * `#OnlineVideos.Details.ReleaseDate` - (localized label property: `#OnlineVideos.Translation.DateOfRelease.Label`)
  * `#OnlineVideos.Details.Actors` - (localized label property: `#OnlineVideos.Translation.Actors.Label`)
  * `#OnlineVideos.Details.Directors` - (localized label property: `#OnlineVideos.Translation.Directors.Label`)
  * `#OnlineVideos.Details.Certificate` - movie rating(e.g. PG-13, R ...)
  * `#OnlineVideos.Details.Title` - also set in `#header.label`
  * `#OnlineVideos.Details.Studio` - movie studio creating the moview
  * `#OnlineVideos.Details.Year` - release year of the movie
  * `#OnlineVideos.DetailsItem.Date` - release date of a trailer (also set to Label2)

### IMDb Movie Trailers ###
detect by (`string.equals(#OnlineVideos.selectedSiteUtil, IMDb)`)

  * `#OnlineVideos.Details.Plot` - (localized label property: `#OnlineVideos.Translation.PlotOutline.Label`)
  * `#OnlineVideos.Details.Genres` - (localized label property: `#OnlineVideos.Translation.Genre.Label`)
  * `#OnlineVideos.Details.ReleaseDate` - (localized label property: `#OnlineVideos.Translation.DateOfRelease.Label`)
  * `#OnlineVideos.Details.Actors` - (localized label property: `#OnlineVideos.Translation.Actors.Label`)
  * `#OnlineVideos.Details.Directors` - (localized label property: `#OnlineVideos.Translation.Directors.Label`)
  * `#OnlineVideos.Details.Writers` - (localized label property: `#OnlineVideos.Translation.Writers.Label`)
  * `#OnlineVideos.Details.Certificate` - movie rating (e.g. PG-13, R ...)
  * `#OnlineVideos.Details.Title` - also set in `#header.label`
  * `#OnlineVideos.Details.Year` - release year of the title
  * `#OnlineVideos.Details.Rating` - Rating 1-10 by votes
  * `#OnlineVideos.Details.Votes` - total user votes on IMDb the rating i based on
  * `#OnlineVideos.Details.Type` - type of title (Unknown, Movie, Game, Short, TVSeries, TVEpisode)
  * `#OnlineVideos.Details.Tagline` - Tagline
  * `#OnlineVideos.DetailsItem.Title` - titleof the video
  * `#OnlineVideos.DetailsItem.Description` - Description of the video
  * `#OnlineVideos.DetailsItem.Duration` - Duration of the video


---


## Latest videos ##
Latest OnlineVideos GuiProperties are available throughout MediaPortal at any time - they are set by a background thread.
The videos will rotate after a user configurable amount of time.
The user can configure how many videos he wants, but the default is 3 (replace 1 with any number to get the properties of that video).

  * `#OnlineVideos.LatestVideo1.Site` - name of the site the video originates from (e.g. _YouTube_)
  * `#OnlineVideos.LatestVideo1.SiteIcon` - thumbnail of the site the video originates from
  * `#OnlineVideos.LatestVideo1.Title` - title of the video
  * `#OnlineVideos.LatestVideo1.Aired` - date when the video was published
  * `#OnlineVideos.LatestVideo1.Duration` - duration of the clip
  * `#OnlineVideos.LatestVideo1.Thumb` - thumbnail of the video
  * `#OnlineVideos.LatestVideo1.Description` - short description of the video
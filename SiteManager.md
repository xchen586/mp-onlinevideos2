

# Global Sites #
OnlineVideos is an open community project and relies on the creativity of users from around the world to create and maintain sites with the provided API. In order to achieve a quick and easy workflow, we provide a public webservice that allows every user to publish and update sites from the OnlineVideos Configuration window. The server stores the configuration data (xml), required library (dll), icon and banner (png) for every site and keeps track of the current state for each site: working, reported, broken.

# Local Sites #
When you install the plugin for the first time, you will only see a small subset of all the sites that are available - a default list that is saved to your hard drive. This is your personal list of sites (local). If you want to remove or add sites in this list, you can go to the SiteManager that shows a merged list of your sites with all the ones that are available (global). Select and press `Enter` to either remove or add sites to your personal list. It is also possible to do this from the Configuration tool on the sites tab - which shows all your local sites so you can remove and reorder them. On top of the form is a button to "Import from global list".

# Updating sites #
Once you have your personal set of sites, some may become broken when the website changes. The creator of the site can update the code or xml to get it working again and update the site in the global list. In order to keep your local site up to date as well, there are two different methods to achieve this.

## automatic ##
Automatic update will check all sites in your local list when they have been last updated and download newer files/configuration when available on the server. This will not add or remove any sites from your list. Only when a Site has been marked as broken on the global list it is deactivated in your local list and will become visible again once the creator has updated it.
Automatic update can be configured in the plugin configuration screen to:
  * run every few hours without asking
  * ask every few hours if it should run when entering OnlineVideos
  * not run at all (only useful when you are a developer)

## manual ##
Clicking on a site in the SiteManager gives you the possibility to update only that site, if it is already in your local list, or otherwise add it to your list. The menu also gives you a chance to add/update/remove all sites that are currently shown. By using the filter in the hidden menu you can for example quickly add all German sites to your list.

# Broken Sites #
If you have repeated errors on a site, checked that your are not having codec problems (try to download or play different videos with VLC/WMP via context menu (F9)), and the website is actually available in the browser, you can write a small report informing the site creator that it is broken in MediaPortal or Configuration. Please make sure that your provided description is helpfull. Additionally you can post a logfile in the forum.

# Screenshots #
<img width='640' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_SiteManager_SideMenu.jpg' />


<img width='640' src='http://mp-onlinevideos2.googlecode.com/svn/wiki/ScreenShots/Screenshot_B3W_SiteManager_ContextMenu.jpg' />
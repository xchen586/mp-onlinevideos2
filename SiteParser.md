# Table of Contents #


# Introduction #

The SiteParser is a tool that can be used for creating the necessary configuration parameters to add to the OnlineVideoSites.xml

You can start the siteparser by clicking this <img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/OnlineVideos.MediaPortal1/Resources/CreateSite.png' /> button in the Online Videos Configuration, Sites-Tab
Or start the program "Onlinevideos.SiteCreator" in the Mediaportal Installation directory

# Description #

A website in OnlineVideos consists of Categories, optional Subcategories, and Videolists.
For each of these, the way of getting the correct information into OnlineVideos must be configured. Therefore, on the main screen, there are sections on which to create/test their configuration settings.

First of all, and the most important one is the baseurl: That must be the URL that holds the categories you want to have in the OnlineVideos.
Most of the time, this is the URL you type in your browser if you want to watch videos. However, sometimes this is buried a little deeper, and you have to use some other means to get that URL. To find the base URL you could use “Page Source”, normally in the “View” menu of your browser, or more advanced tools like Fiddler2. Fiddler2 is a free debugging program that logs all the HTTP traffic and allows identification of any unseen page calls.

Once you have put the baseurl in its place, you are ready to find the categories.
Click the "Create Regex" button in the Categories section [explanation](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex), and start creating the Regex for it. When creating a Regex you search for a line in the html code from the baseurl that contains the item your looking for, in this case the ‘categories’. You then copy the full line into the editing box and substitute the item your looking for with the code from the dropdown list provided. You can then check the result by pressing the "Test<img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/SiteParser/Resources/Intersect.png' /> " button.  You can remove excess html by using the “skip to char” substitution.  The [video](http://mp-onlinevideos2.googlecode.com/files/SiteParser.wmv) provides a good example of this process.
Once complete, click on the "GetCats" button, and you'll see a "+" appearing at the right side (just left of "-site").
You can also add static categories by clicking the "Static Categories" button.

Expand that "+", and select some categories and see if the results at the bottom of the screen are satisfactory.
If not, you can change the URL-format, and enable URL-decoding if necessary. After changing those, you need to click on "GetCats" to see the updated results.

After the categories are correctly configured, you can do the same for subcategories if present. If not, go to the "VideoList".
For static categeories, you can also add static subcategories by clicking the "Static Subcategories" button.

VideoList:
Under site (at the right side) you have to select a Category or Subcategory.
Then create the regex for the videos.

When that one is created click on the "GetVideoList", and you'll see a "+" appearing at the right side, just left of the category/subcategory selected.

VideoUrl:
This is a 3-step process:

1: "GetVideoUrl" here you can transform the url from the VideoList to a more suitable string (not necessary a valid url) e.g. extract the id from the url
[Explanation of dropdownlist items](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex?ts=1300917626&updated=CreateRegex#For_the_Video_URL)

2: "GetPlayListUrl" to be documented

3: "GetFileUrl" here you can parse the html-result of the previous steps to extract the final url.
[Explanation of dropdownlist items](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex?ts=1300917626&updated=CreateRegex#For_File_Url:)
In that case you can specify the Formatted name at FileUrlNameFormatString.
Also you can enable parsing redirected urls with getRedirectedFileUrl, and fill in data to put in a http-POST message (in fileUrlPostString)

4: "Result" a list of the final urls found.
Next to it is a "play" button, which opens the selected url in Windows Media Player.
Right-click on that button will give more options, like "copy to clipboard" and "check if url is valid"

## Cookies ##
If a website require static cookies, you can enter these in the "Cookies" box. The format is 

&lt;name&gt;

=

&lt;value&gt;

, and if multiple cookies are required, enter each on a separate line

## Force Utf8 Encoding ##
Check this if a website's html isn't recognized as having UTF8 Encoding.

# Menu Items #

  * File, Load Sites: Opens an existing OnlineVideoSites.xml for further testing/editing
  * ChoiceBox: If a file is opened, you can choose the site you want to test/edit
  * Load: loads the configuration of the site selected in the choicebox
  * Copy: copy the current configuration parameters as xml to the clipboard (So that you can easily paste them in the OnlineVideoSites.xml

### Parameters in Regex Dropdown list ###

They are explained in detail [here](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex#Regex-shortcuts)

# Step by step (simple version) #

  1. for baseurl: fill the URL of the page with the categories on it
  1. click on "Create Regex" on the "Category" tab, and create the regex [explanation](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex)
  1. click on "GetCats" on the "Category" tab. You should now see the categories listed under "site" at the right panel
  1. if there are subcategories present then select a category, and goto the subcategories tab, if notL skip next step
  1. click on "Create Regex" and after that on GetSubcats. then select a subcategory
  1. goto the "VideoList" tab, and click on "Create Regex".
  1. create the regex, and after that, click "GetVideoList"
  1. now you should see the video's listed under the selected (sub)category
  1. select a video, and click on the "VideoUrl" tab.
  1. click "GetVideoUrl", click "GetPlaylistUrl" and click "Create Regex" next to "GetFileUrl"
  1. create the regex
  1. choose the URL under "ResultUrl" and hit the play-button to verify
  1. click "Copy" in the main menu, open onlinevideosites.xml (located in either the c:\programdata\team mediaportal\mediaportal, C:\Documents and Settings\All Users\Application Data\Team MediaPortal\MediaPortal, or c:\program files\team mediaportal\mediaportal) in your favourite texteditor, and paste the configuration in it. Then remove the categories (lines starting with <Category xsi:type="RssLink") that are to be dynamically discovered.
  1. Save the file, and enjoy your new creation!
  1. After the final approvement, you can publish the site through the onlinevideos configuration

# Video #

There is also a video where a demonstration is given of the SiteParser.
It's located [here](http://mp-onlinevideos2.googlecode.com/files/SiteParser.wmv)

#summary A guide on how to use the SiteParser to add your site to OnlineVideos.
# Table of Contents #


# Introduction #

The SiteParser is a tool that can be used for creating the necessary configuration parameters to add to the OnlineVideoSites.xml

You can start the siteparser by clicking this <img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/OnlineVideos.MediaPortal1/Resources/CreateSite.png' /> button in the Online Videos Configuration, Sites-Tab
Or start the program "Onlinevideos.SiteCreator" in the Mediaportal Installation directory

# Description #

A website in OnlineVideos consists of Categories, optional Subcategories, and Videolists.
For each of these, the way of getting the correct information into OnlineVideos must be configured. Therefore, on the main screen, there are sections on which to create/test their configuration settings.

First of all, and the most important one is the baseurl: That must be the URL that holds the categories you want to have in the OnlineVideos.
Most of the time, this is the URL you type in your browser if you want to watch videos. However, sometimes this is buried a little deeper, and you have to use some other means to get that URL. To find the base URL you could use “Page Source”, normally in the “View” menu of your browser, or more advanced tools like Fiddler2. Fiddler2 is a free debugging program that logs all the HTTP traffic and allows identification of any unseen page calls.

Once you have put the baseurl in its place, you are ready to find the categories.
Click the "Create Regex" button in the Categories section [explanation](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex), and start creating the Regex for it. When creating a Regex you search for a line in the html code from the baseurl that contains the item your looking for, in this case the ‘categories’. You then copy the full line into the editing box and substitute the item your looking for with the code from the dropdown list provided. You can then check the result by pressing the "Test<img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/SiteParser/Resources/Intersect.png' /> " button.  You can remove excess html by using the “skip to char” substitution.  The [video](http://mp-onlinevideos2.googlecode.com/files/SiteParser.wmv) provides a good example of this process.
Once complete, click on the "GetCats" button, and you'll see a "+" appearing at the right side (just left of "-site").
You can also add static categories by clicking the "Static Categories" button.

Expand that "+", and select some categories and see if the results at the bottom of the screen are satisfactory.
If not, you can change the URL-format, and enable URL-decoding if necessary. After changing those, you need to click on "GetCats" to see the updated results.

After the categories are correctly configured, you can do the same for subcategories if present. If not, go to the "VideoList".
For static categeories, you can also add static subcategories by clicking the "Static Subcategories" button.

VideoList:
Under site (at the right side) you have to select a Category or Subcategory.
Then create the regex for the videos.

When that one is created click on the "GetVideoList", and you'll see a "+" appearing at the right side, just left of the category/subcategory selected.

VideoUrl:
This is a 3-step process:

1: "GetVideoUrl" here you can transform the url from the VideoList to a more suitable string (not necessary a valid url) e.g. extract the id from the url
[Explanation of dropdownlist items](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex?ts=1300917626&updated=CreateRegex#For_the_Video_URL)

2: "GetPlayListUrl" to be documented

3: "GetFileUrl" here you can parse the html-result of the previous steps to extract the final url.
[Explanation of dropdownlist items](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex?ts=1300917626&updated=CreateRegex#For_File_Url:)
In that case you can specify the Formatted name at FileUrlNameFormatString.
Also you can enable parsing redirected urls with getRedirectedFileUrl, and fill in data to put in a http-POST message (in fileUrlPostString)

4: "Result" a list of the final urls found.
Next to it is a "play" button, which opens the selected url in Windows Media Player.
Right-click on that button will give more options, like "copy to clipboard" and "check if url is valid"

# Menu Items #

  * File, Load Sites: Opens an existing OnlineVideoSites.xml for further testing/editing
  * ChoiceBox: If a file is opened, you can choose the site you want to test/edit
  * Load: loads the configuration of the site selected in the choicebox
  * Copy: copy the current configuration parameters as xml to the clipboard (So that you can easily paste them in the OnlineVideoSites.xml

### Parameters in Regex Dropdown list ###

They are explained in detail [here](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex#Regex-shortcuts)

# Step by step (simple version) #

  1. for baseurl: fill the URL of the page with the categories on it
  1. click on "Create Regex" on the "Category" tab, and create the regex [explanation](http://code.google.com/p/mp-onlinevideos2/wiki/CreateRegex)
  1. click on "GetCats" on the "Category" tab. You should now see the categories listed under "site" at the right panel
  1. if there are subcategories present then select a category, and goto the subcategories tab, if notL skip next step
  1. click on "Create Regex" and after that on GetSubcats. then select a subcategory
  1. goto the "VideoList" tab, and click on "Create Regex".
  1. create the regex, and after that, click "GetVideoList"
  1. now you should see the video's listed under the selected (sub)category
  1. select a video, and click on the "VideoUrl" tab.
  1. click "GetVideoUrl", click "GetPlaylistUrl" and click "Create Regex" next to "GetFileUrl"
  1. create the regex
  1. choose the URL under "ResultUrl" and hit the play-button to verify
  1. click "Copy" in the main menu, open onlinevideosites.xml (located in either the c:\programdata\team mediaportal\mediaportal, C:\Documents and Settings\All Users\Application Data\Team MediaPortal\MediaPortal, or c:\program files\team mediaportal\mediaportal) in your favourite texteditor, and paste the configuration in it. Then remove the categories (lines starting with <Category xsi:type="RssLink") that are to be dynamically discovered.
  1. Save the file, and enjoy your new creation!
  1. After the final approvement, you can publish the site through the onlinevideos configuration

# Video #

There is also a video where a demonstration is given of the SiteParser.
It's located [here](http://mp-onlinevideos2.googlecode.com/files/SiteParser.wmv)

# RtmpUtil #

Occasionally, you have a final url which starts with "rtmp://" or "rtmpe://".

At first, you can try if it works as is (Onlinevideos is capable of handling rtmp urls, which does not need extra parameters),
but if that doesn't work, you probably have to add some extras (f.e. playpath, swf verification parameters etc).

The easiest way, is to try to find those parameters on the internet, or you can start trying yourself with [rtmpdump](http://rtmpdump.mplayerhq.hu/).
When you have the correct parameters, you then have to put them into the configuration.

This is done by replacing "GenericSite" by "Rtmp" in the OnlineVideoSites.xml, and provide the extra parameters at "fileUrlFormatString".

f.e. if you need to set the playpath, you use the following:
```
<item key="fileUrlFormatString"><![CDATA[{0}?playpath={1}&app={2}]]></item>
```
There must be a "?" between the url and the added parameters, and between each param=value-pair there must be an "&"

This is in accordance to a [Query string](http://en.wikipedia.org/wiki/Query_string)

Sometimes this doesn't work because there are "?" present in the rtmpurl or in the other parameters. In that case, a " " is also allowed, ex.:
```
<item key="fileUrlFormatString"><![CDATA[{0} playpath={1}&app={2}]]></item>
```


The following parameters are supported:
  * rtmpurl
  * app
  * tcUrl
  * hostname
  * port
  * playpath
  * subscribepath
  * pageurl
  * swfurl
  * swfsize
  * swfhash
  * swfVfy
  * live
  * auth
  * token
  * conn
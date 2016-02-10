# Table of Contents #


# Description #

This part of the program is used as an aid to get the correct regex for the various parts of the configuraion.

When this part is entered (through the "Create Regex" button), the relevant html is already loaded in the pane at the right side. This is the base on which to create the regex.

Make sure you have this page visible in your web-browser, that will make the next steps easier.
First you enter a (part of the) text you are searching for, e.g. one of the Categories you see in your browser. Hit "Find" to go to that particular part of the html.
Then you have to select the relevant part (this requires some experience) and click on the <img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/SiteParser/Resources/curved.png' />. This will overwrite the text selected in the top-left pane with the selected text of the right part.

Then for all the pieces in the top-left pane that you recognize (e.g. the name, the URL etc) you select them, choose the kind of item that it represents, and click "<img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/SiteParser/Resources/arrowDn.png' />Insert"

For all the remaining data, you can select parts, and choose e.g. "skip to" (multiple chars), "skip to single char" or "optional".

After that, you can click "Test<img width='16' src='http://mp-onlinevideos2.googlecode.com/svn/trunk/SiteParser/Resources/Intersect.png' /> " to see the results of your regex.

You have to play with this to get some experience, because if you leave too much data in your regex, there will only be 1 match instead of a list, and if you remove too much, there will be more matches than you actually want.

When manually editing the regex, note that these are the default settings:
**_Compiled, CultureInvariant, Multiline, Singleline, IgnorePatternWhitespace, ExplicitCapture_**

For testing purposes there are two helpful checkboxes:

**Only Selected (Real-time)**

This is useful if you want to investigate a certain category or video isn't listed in the test result list.
When checked, only the selected part of your regex is used to calculate the match, so you can start with a couple of characters and gradually select more until the category/video disappears from the list. Then you have a more accurate location in the regex which is causing the non-match.

**Highlight Matches**

To further investigate if your regex isn't matching exactly what you want, you can highlight the matches in the right pane (the html-text).
Be aware that for this to work, the match is done on on the text as it's visible in the rigt pane, and not on the original html-data.

However, as far as I noticed, the only differences were in whitespace (f.e. if a number of `<tab>`s is followed by a space in the original html-data, that space is not used when calculating the matches).
Technical explanation: For some reason, the RichTextBox changes the contents of the .Text property while the .RTF getter is used


# Regex-shortcuts #

## For Categories/Subcategories: ##

### url ###
This is used to replace the url of the page with (sub)categories to add.

### title ###
This is used to replace the title string of the (sub)category to add.

### thumb ###
This is used to replace the url string of thumbnail image of the (sub)category to add.

### description ###
This is used to replace the description string of the (sub)category to add.

## For videos: ##

### Title ###
This is used to replace the title string of the video stream you're looking to add.

### VideoUrl ###
This is used to replace the url of the video stream you're looking to add.

### ImageUrl ###
This is used to replace the url string of thumbnail image of the video stream you're looking to add.

### Description ###
This is used to replace the description string of the video stream you're looking to add.

### Duration ###
This is used to replace the duration or length string of the video stream you're looking to add.

### Airdate ###
This is used to replace the airdate or upload date string of the video stream you're looking to add.

## For next page: ##

### url ###
This is used to replace the url of the next page of videos.

## For the Video URL ##

### m0..m2 ###

You can use m0,m1 and so on to extract the parts that are needed, and they will be filled into the {0},{1} etc in the VideoUrlFormatString
example:
```
Url from VideoList: http://www.website.com/video/today/706334
VideoUrlRegex: http://.+/(?<m0>\d{4,10}) (this results in m0=706334)
VideoUrlFormatString: http://www.website.com/flash/download?&movieId={0}
VideoUrlResult: http://www.website.com/flash/download?&movieId=706334
```

## For Playlist Url: ##

### url ###
This is used to replace the url of the playlist files.

## For File Url: ##

### m0..m2 ###
You can use m0,m1 and so on to extract the parts that are needed, and they will be filled into the {0},{1} etc in the FileUrlFormatString

### n0..n2 ###
Here you optionally can specify n0,n1 etc for the name (useful in case of multiple urls for 1 video, like youtube).

## For all: ##

### Skip to ###
this will skip to multiple chars.
e.g. if you have

`loads of uninteresting html<img src="http://url_of_thumbnail" />`

select `loads of uninteresting html`, choose "skip to" and click "insert"
Then replace the selected text (@@) with the text after the part that you want to skip, in this case `<img`

### Skip to single char ###
this will skip to a single char.
e.g. if you have

`<img src="http://url_of_thumbnail" width="500" height="500"/>`

select `width="500" height="500"/` choose "skip to single char" and click "insert".

### optional ###
this will make the selected text optional.

### match after ###
This will make sure only matches after "some text" are returned
Choose "match after", click insert.
Then replace the selected text (@@) with "some text"

### match before ###
This will make sure only matches before "some text" are returned
Choose "match after", click insert.
Then replace the selected text (@@) with "some text"

### match not ###
This will prevent a match with the given text. Usefull f.e. for ignoring unwanted entries such as "FAQ" or "Contact"
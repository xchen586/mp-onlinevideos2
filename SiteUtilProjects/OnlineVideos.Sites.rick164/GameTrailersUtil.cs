﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Web;
using System.Xml;
using RssToolkit.Rss;
using Newtonsoft.Json.Linq;

namespace OnlineVideos.Sites
{
    public class GameTrailersUtil : GenericSiteUtil
    {
        public override void Initialize(SiteSettings siteSettings)
        {
            base.Initialize(siteSettings);
        }

        public override string getUrl(VideoInfo video)
        {
            string data = GetWebData(video.VideoUrl);
            string VideoUrl = "";
            string finalDownloadUrl = "";
            string baseDownloadUrl = "http://www.gametrailers.com/feeds/video_download/";
            if (data.Length > 0)
            {
                if (regEx_PlaylistUrl != null)
                {
                    try
                    {
                        Match m = regEx_PlaylistUrl.Match(data);
                        while (m.Success)
                        {
                            string contentid = HttpUtility.HtmlDecode(m.Groups["contentid"].Value);
                            string token = HttpUtility.HtmlDecode(m.Groups["token"].Value);
                            finalDownloadUrl = baseDownloadUrl + contentid + "/" + token;

                            string dataJson = GetWebData(finalDownloadUrl);
                            JObject o = JObject.Parse(dataJson);
                            VideoUrl = o["url"].ToString().Replace("\"", "");
                            break;
                        }
                    }
                    catch (Exception eVideoUrlRetrieval)
                    {
                        Log.Debug("Error while retrieving Video Url: " + eVideoUrlRetrieval.ToString());
                    }
                    return VideoUrl;
                }
            }
            return null;
        }

        public override List<VideoInfo> getVideoList(Category category)
        {
            string url = ((RssLink)category).Url;
            //Log.Debug("CATEGORY: " + category.Name + " | URL: " +url);
            return getVideoList(url);
        }

        public override List<VideoInfo> getNextPageVideos()
        {
            //Log.Debug("NEXT PAGE URL: " + nextPageUrl);
            return getVideoList(nextPageUrl);
        }

        protected List<VideoInfo> getVideoList(string url)
        {
            List<VideoInfo> videoList = new List<VideoInfo>();
            string data = GetWebData(url);

            //Make sure Regexp doesn't get copied over after search so save it in a new Regexp.
            Regex regEx_VideoListTmp = new Regex("");
            Regex regEx_VideoListGameName = new Regex("");

            regEx_VideoListTmp = regEx_VideoList;
            string strRegEx_VideoList = "";
            string strRegEx_VideoListGameName = "";

            string searchBase = "http://www.gametrailers.com/feeds/search/";

            //Replace Regexp with search friendly regexp
            if (url.StartsWith(searchBase))
            {
                strRegEx_VideoList = @"<meta\sitemprop=""url""\scontent=""http://www\.gametrailers\.com/[^/]*/(?<tmpTitle>[^""]*)""/>\s*<meta\sitemprop=""name""\scontent=""(?<Title>[^""]*)""/>\s*<meta\sitemprop=""thumbnail""\scontent=""(?<ImageUrl>[^""]*)""/>\s*<meta\sitemprop=""description""\scontent=""(?<Description>[^""]*)""/>\s*<meta\sitemprop=""uploadDate""\scontent=""(?<Airdate>[^""]*)""/>\s*<meta\sitemprop=""duration""\scontent=""(?<Duration>[^""]*)""/>\s*<a\shref=""(?<VideoUrl>[^""]*)""\sclass=""thumbnail"">";
                regEx_VideoListTmp = new Regex(strRegEx_VideoList);
            }

            //Hardcode regexp as we probably need to add workaround later (when GT changes their per-category layout once again)
            else
            {
                strRegEx_VideoList = @"<meta\sitemprop=""url""\scontent=""http://www\.gametrailers\.com/[^/]*/(?<tmpTitle>[^""]*)""/>\s*<meta\sitemprop=""name""\scontent=""(?<Title>[^""]*)""/>\s*<meta\sitemprop=""thumbnail""\scontent=""(?<ImageUrl>[^""]*)""/>\s*<meta\sitemprop=""description""\scontent=""(?<Description>[^""]*)""/>\s*<meta\sitemprop=""uploadDate""\scontent=""(?<Airdate>[^""]*)""/>\s*<meta\sitemprop=""duration""\scontent=""(?<Duration>[^""]*)""/>\s*<a\shref=""(?<VideoUrl>[^""]*)""\sclass=""thumbnail"">";
                regEx_VideoListTmp = new Regex(strRegEx_VideoList);

                //Full video title regexp
                strRegEx_VideoListGameName = @"<h3><a\shref=""[^""]*"">(?<gameName>[^<]*)</a></h3>\s*<h4><a\shref=""[^""]*""\sclass=""override_promo_font_color"">(?<Title>[^<]*)</a></h4>";
                regEx_VideoListGameName = new Regex(strRegEx_VideoListGameName);
            }
            if (data.Length > 0)
            {
                if (regEx_VideoListTmp != null)
                {
                    try
                    {
                        Log.Debug("USED URL: " + url);
                        Log.Debug("USED REGEX: " +strRegEx_VideoList); 
                        Match m = regEx_VideoListTmp.Match(data);
                        Match m2 = regEx_VideoListGameName.Match(data);
                        int counter = 0;
                        int videoCount = 0;

                        while (m.Success)
                        {
                            VideoInfo videoInfo = CreateVideoInfo();

                            videoInfo.Title = m.Groups["Title"].Value;

                            //Try to retrieve full title (gameName + title) since these are listed differently per category or spread out, couldn't match those easily with one Regexp.
                            if (!url.StartsWith(searchBase))
                            {
                                while (m2.Success)
                                {
                                    if (counter == videoCount)
                                    {
                                        videoInfo.Title = HttpUtility.HtmlDecode(m2.Groups["gameName"].Value) + " - " + HttpUtility.HtmlDecode(m2.Groups["Title"].Value);
                                        counter++;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }

                            videoInfo.VideoUrl = m.Groups["VideoUrl"].Value;
                            videoInfo.ImageUrl = m.Groups["ImageUrl"].Value;
                            videoInfo.Length = Utils.PlainTextFromHtml(m.Groups["Duration"].Value).Replace("M", "M ").Replace("S", "S").Replace("PT0H", "").Replace("PT1H", "1H ").Replace("PT", "").Trim();
                            videoInfo.Description =HttpUtility.HtmlDecode(m.Groups["Description"].Value);

                            videoCount++;
                            videoList.Add(videoInfo);
                            m = m.NextMatch();
                            m2 = m2.NextMatch();
                        }

                    }
                    catch (Exception eVideoListRetrieval)
                    {
                        Log.Debug("Error while retrieving VideoList: " + eVideoListRetrieval.ToString());
                    }
                }
                if (regEx_NextPage != null)
                {
                    try
                    {
                        // check for next page link
                        Match mNext = regEx_NextPage.Match(data);
                        if (mNext.Success)
                        {
                            //Log.Debug("PAGE URL: " + mNext.Groups["url"].Value);
                            //Log.Debug("VIDEO URL: "+ url);
                            nextPageAvailable = true;
                            nextPageUrl = mNext.Groups["url"].Value;
                            if (!string.IsNullOrEmpty(nextPageRegExUrlFormatString)) nextPageUrl = string.Format(nextPageRegExUrlFormatString, nextPageUrl);
                            nextPageUrl = ApplyUrlDecoding(nextPageUrl, nextPageRegExUrlDecoding);
                            nextPageUrl = url + nextPageUrl.Replace("?currentPage=", "&currentPage=");
                        }
                        else
                        {
                            string page = HttpUtility.ParseQueryString(new Uri(url).Query)["currentPage"];
                            nextPageAvailable = true;
                            nextPageUrl = url.Replace("currentPage=" + page.ToString(), "currentPage=" + (int.Parse(page) + 1).ToString());
                        }
                        //Log.Debug("NEXTPAGE URL: " + nextPageUrl);
                        nextPageUrl = nextPageUrl;
                    }
                    catch (Exception eNextPageRetrieval)
                    {
                        Log.Debug("Error while retrieving Next Page Url: " + eNextPageRetrieval.ToString());
                    }
                }
            }
            return videoList;
        }
        public override int DiscoverSubCategories(Category parentCategory)
        {
            string url = ((RssLink)parentCategory).Url;
            parentCategory.SubCategories = new List<Category>();

            //Log.Debug("PARENT CATEGORY: " + url);
            string data = GetWebData(url);

            if (regEx_dynamicSubCategories != null)
           {
                List<string> catNames = new List<string>();

                //Add static categories, no need to fetch them since Gametrailers will most likely keep them as is for a while.
                catNames.Add("Newest Media");
                catNames.Add("Review");
                catNames.Add("Preview");
                catNames.Add("Trailer");
                catNames.Add("Gameplay");
                catNames.Add("Features");
                catNames.Add("Interview");

                Match m = regEx_dynamicSubCategories.Match(data);
                int counter = 0;
                string tmpUrl = "";

                while (m.Success && counter == 0)
                {
                    tmpUrl = m.Groups["url"].Value;

                    foreach (string catName in catNames)
                    {
                        RssLink cat = new RssLink();
                        if (catName == "Newest Media")
                        {
                            cat.Url = tmpUrl + "/?sortBy=most_recent";
                        }
                        else
                        {
                            cat.Url = tmpUrl + "/?sortBy=most_recent&category=" + catName;
                        }

                        cat.Name = catName;
                        cat.ParentCategory = parentCategory;
                        parentCategory.SubCategories.Add(cat);

                        //Log.Debug("CAT NAME: " + cat.Name + " CAT URL: " + cat.Url);
                        counter++;
                    }
                    
                }  
            }

            parentCategory.SubCategoriesDiscovered = parentCategory.SubCategories.Count > 0; // only set to true if actually discovered (forces re-discovery until found)
            return parentCategory.SubCategories.Count;
        }

        [Obsolete]
        public override  List<VideoInfo> Search(string query)
        {
            //
            //First we need to fetch the Promo ID in order to do a search, afterwards we can do the real search
            //
            string strPromoIDRegex = @"<input class=""search"" name=""keywords"" type=""text"" value=""[^""]*"" data-keywords=""[^""]*"" data-promotionId=(?<id>[^/]*)/>";
            Regex regEX_SearchURL = new Regex(strPromoIDRegex);

            string data = GetWebData("http://www.gametrailers.com/search?keywords=");
            string promotionID = "";
            Match m = regEX_SearchURL.Match(data);
            try
            {
                while (m.Success)
                {
                    promotionID = m.Groups["id"].Value;
                    //Log.Debug("PROMO=" + promotionID);
                    break;
                }
            }
            catch (Exception eSearchUrlRetrieval)
            {
                Log.Debug("Error while retrieving Search URL: " + eSearchUrlRetrieval.ToString());
            }

            // if an override Encoding was specified, we need to UrlEncode the search string with that encoding
            if (encodingOverride != null) query = HttpUtility.UrlEncode(encodingOverride.GetBytes(query));

            searchUrl = "http://www.gametrailers.com/feeds/search/child/" + promotionID + "/?tabName=videos&platforms=&sortBy=most_recent&keywords=" + query;

            return getVideoList(searchUrl);
        }
    }
}

﻿using System;
using System.ComponentModel;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Text.RegularExpressions;

namespace OnlineVideos.Sites.DavidCalder
{
    public class WatchOnlineSeriesUtil : DeferredResolveUtil
    {

        public override List<VideoInfo> getVideoList(Category category)
        {
            List<VideoInfo> vidInfo = new List<VideoInfo>(base.getVideoList(category));

            foreach (VideoInfo vid in vidInfo)
            {
                string newName = vid.Title.Replace(".", "");
                vid.Title = string.Format("{0} – {1} – TBA", category.ParentCategory.Name, newName);
            }
            return vidInfo;
        }

        public override string FormatHosterUrl(string name)
        {
            string videoUrl = name;
            if (videoUrl.Contains("adf.ly/254271/"))
            {
                videoUrl = videoUrl.Replace("adf.ly/254271/", "");
            }
            if (!videoUrl.Contains("www."))
            {
                videoUrl = videoUrl.Replace("http://", "http://www.");
            }
            return videoUrl;
        }


        public override List<ISearchResultItem> DoSearch(string query)
        {
            List < ISearchResultItem > searchResults = new List<ISearchResultItem>(base.DoSearch(query.Replace(" ", "-")));
            
            return searchResults;
        }

    }

   
}

﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using OnlineVideos.Hoster.Base;
using OnlineVideos.Sites;
using System.Text.RegularExpressions;

namespace OnlineVideos.Hoster
{
    public class DataUp : HosterBase
    {
        public override string getHosterUrl()
        {
            return "Dataup.to";
        }

        public override string getVideoUrls(string url)
        {
            string page = SiteUtilBase.GetWebData(url);
            if (!string.IsNullOrEmpty(page))
            {
                Match n = Regex.Match(page, @"video/divx""\ssrc=""(?<url>[^""]+)""");
                if (n.Success)
                {
                    videoType = VideoType.divx;
                    return n.Groups["url"].Value;
                }
                else
                {
                    n = Regex.Match(page, @"addVariable\('file','(?<url>[^']+)'");
                    if (n.Success)
                    {
                        videoType = VideoType.flv;
                        return n.Groups["url"].Value;
                    }
                }
            }
            return String.Empty;
        }
    }
}
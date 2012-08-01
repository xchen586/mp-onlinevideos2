﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using OnlineVideos.Hoster.Base;
using OnlineVideos.Sites;
using System.Text.RegularExpressions;

namespace OnlineVideos.Hoster
{
    public class DivxDen : HosterBase
    {
        public override string getHosterUrl()
        {
            return "DivxDen.com";
        }

        public override string getVideoUrls(string url)
        {
            if (url.Contains("embed"))
            {
                string page = SiteUtilBase.GetWebData(url);
                url = Regex.Match(page, @"<div><a\shref=""(?<url>[^""]+)""").Groups["url"].Value;
            }

            string[] urlParts = url.Split('/');

            string postData = @"op=download1&usr_login=&id=" + urlParts[3] + "&fname=" + urlParts[4] + "&referer=&method_free=Free+Stream";
            string webData = GenericSiteUtil.GetWebDataFromPost(url, postData);
            string packed = GetSubString(webData, @"return p}", @"</script>");
            packed = packed.Replace(@"\'", @"'");
            string unpacked = UnPack(packed);
            string res = GetSubString(unpacked, @"'file','", @"'");
            videoType = VideoType.divx;
            if (!String.IsNullOrEmpty(res))
                return res;
            return GetSubString(unpacked, @"name=""src""value=""", @"""");
        }


    }
}
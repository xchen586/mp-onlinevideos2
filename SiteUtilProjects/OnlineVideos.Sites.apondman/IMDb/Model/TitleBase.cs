﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OnlineVideos.Sites.Pondman.IMDb.Model
{
    using System.Web;
    using OnlineVideos.Sites.apondman.IMDb.DTO;
    using OnlineVideos.Sites.Pondman.IMDb.Json;

    public class TitleBase : Reference, IVideoDetails
    {
        #region Public properties

        public virtual TitleType Type { get; internal set; }

        public virtual string Title { get; internal set; }

        public virtual int Year { get; internal set; }

        public virtual double Rating { get; internal set; }

        public virtual int Votes { get; internal set; }

        public virtual string Image { get; internal set; }

        #endregion

        #region Public methods

        public virtual List<VideoReference> GetVideos()
        {
            return IMDbAPI.GetVideos(this.session, this.ID);
        }

        #endregion

        #region Internal methods

        internal virtual void FillFrom(IMDbTitle dto) {
            
            ID = dto.TConst;
            Title = dto.Title;
            Year = dto.Year;
            Rating = dto.Rating;
            Votes = dto.Votes;

            if (dto.Image != null)
            {
                Image = dto.Image.Url;
            }

            switch (dto.Type)
            {
                case "feature":
                    Type = TitleType.Movie;
                    break;
                case "tv_series":
                    Type = TitleType.TVSeries;
                    break;
                case "video_game":
                    Type = TitleType.Game;
                    break;
                case "short":
                    Type = TitleType.Short;
                    break;
                // todo: add more types
                default:
                    Type = TitleType.Unknown;
                    break;
            }
        }


        internal virtual void FillFrom(IMDbTitleMobile dto)
        {
            this.ID = dto.URL.Replace("/title/", "").Replace("/", "").Trim();

            IMDbAPI.ParseDisplayStringToTitleBase(this, HttpUtility.HtmlDecode(dto.Title) + " " + dto.Extra);
            this.Image = IMDbAPI.ParseImageUrl(dto.Image.Url);
        }
        

        internal virtual void FillFrom(Item dto)
        {
            this.ID = dto.display.titleId;
            this.Title = dto.display.text;
            this.Year = int.Parse(dto.display.year);

            if (dto.display.poster != null)
            {
                Image = dto.display.poster.url;
            }

            switch (dto.display.type)
            {
                case "feature":
                    Type = TitleType.Movie;
                    break;
                case "tv_series":
                    Type = TitleType.TVSeries;
                    break;
                case "video_game":
                    Type = TitleType.Game;
                    break;
                case "short":
                    Type = TitleType.Short;
                    break;
                // todo: add more types
                default:
                    Type = TitleType.Unknown;
                    break;
            }
        }

        #endregion

        #region IVideoDetails Members

        public override Dictionary<string, string> GetExtendedProperties()
        {
            Dictionary<string, string> p = base.GetExtendedProperties();

            p.Add("Type", this.Type.ToString());
            p.Add("Title", this.Title);
            p.Add("Year", this.Year.ToString());
            p.Add("Rating", this.Rating.ToString());
            p.Add("Votes", this.Votes.ToString());

            return p;
        }

        #endregion
    }
}

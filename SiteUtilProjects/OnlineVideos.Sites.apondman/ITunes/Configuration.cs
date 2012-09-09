﻿using System;
using System.Text;

namespace OnlineVideos.Sites.Pondman.ITunes {

    public class Configuration {
        
        public string RootTitle {
            get
            {
                return this.rootTitle;
            }
            set
            {
                this.rootTitle = value;
            }
        } string rootTitle = "iTunes Movie Trailers";       

        public string BaseUri {
            get { return _baseUri; }
            set { _baseUri = value; }
        } string _baseUri = "http://trailers.apple.com";

        public string FeedsUri {
            get {
                if (_feedsUri.StartsWith("/"))
                    return _baseUri + _feedsUri;

                return _feedsUri;
            }
            set { _feedsUri = value; }
        } string _feedsUri = "/trailers/home/feeds/";

        public string HomeUri {
            get {
                if (_homeUri.StartsWith("/"))
                    return _baseUri + _homeUri;

                return _homeUri;
            }
            set { _homeUri = value; }
        } string _homeUri = "/moviesxml/h/index.xml";

        public string SearchUri {
            get {
                if (_searchUri.StartsWith("/"))
                    return _baseUri + _searchUri;

                return _searchUri;
            }
            set { _searchUri = value; }
        } string _searchUri = "/trailers/home/scripts/quickfind.php?q=";

        public string XmlNamespace {
            get { return _xmlNamespace; }
            set { _xmlNamespace = value; }
        } string _xmlNamespace = "http://www.apple.com/itms/";

        public string XmlMovieDetailsUri {
            get {
                if (_movieUri.StartsWith("/"))
                    return _baseUri + _movieUri;

                return _movieUri;
            }
            set { _movieUri = value; }
        } string _movieUri = "/moviesxml/s/";

        public string FeaturedJustAddedUri {
            get { return FeedsUri + "just_added.json"; }
        }

        public string FeaturedExclusiveUri {
            get { return FeedsUri + "exclusive.json"; }
        }

        public string FeaturedJustHdUri {
            get { return FeedsUri + "just_hd.json"; }
        }

        public string FeaturedMostPopularUri {
            get { return FeedsUri + "most_pop.json"; }
        }

        public string FeaturedGenresUri {
            get { return FeedsUri + "genres.json"; }
        }

        public string FeaturedStudiosUri {
            get { return FeedsUri + "studios.json"; }
        }

        public string WeekendBoxOfficeUri {
            get { return BaseUri + "/moviesxml/h/boxoffice_include.xml"; }
        }

        public string OpeningThisWeekUri {
            get { return BaseUri + "/moviesxml/h/openings_include.xml"; }
        }

        /// <summary>
        /// Adds the value of BaseUri when the uri is relative
        /// </summary>
        /// <param name="uri"></param>
        /// <returns></returns>
        public string FixUri(string uri)
        {
            if (uri.StartsWith("/"))
            {
                uri = BaseUri + uri;
            }

            return uri;
        }

    }
}
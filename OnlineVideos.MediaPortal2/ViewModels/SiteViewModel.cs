﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MediaPortal.Common.General;
using MediaPortal.UI.SkinEngine.ScreenManagement;
using MediaPortal.UI.Presentation.DataObjects;
using MediaPortal.UiComponents.Media.General;

namespace OnlineVideos.MediaPortal2
{
	public class SiteViewModel : ListItem
    {
		protected AbstractProperty _nameProperty;
		public AbstractProperty NameProperty { get { return _nameProperty; } }
		public string Name
		{
			get { return (string)_nameProperty.GetValue(); }
			set { _nameProperty.SetValue(value); }
		}

		protected AbstractProperty _descriptionProperty;
		public AbstractProperty DescriptionProperty { get { return _descriptionProperty; } }
		public string Description
		{
			get { return (string)_descriptionProperty.GetValue(); }
			set { _descriptionProperty.SetValue(value); }
		}

		protected AbstractProperty _languageProperty;
		public AbstractProperty LanguageProperty { get { return _languageProperty; } }
		public string Language
		{
			get { return (string)_languageProperty.GetValue(); }
			set { _languageProperty.SetValue(value); }
		}

        protected Sites.SiteUtilBase _site;
        public Sites.SiteUtilBase Site
        {
            get { return _site; }
        }
        
        public SiteViewModel(Sites.SiteUtilBase site) 
			: base(Consts.KEY_NAME, site.Settings.Name)
        {
            _site = site;

			_nameProperty = new WProperty(typeof(string), site.Settings.Name);
			_languageProperty = new WProperty(typeof(string), site.Settings.Language);
			_descriptionProperty = new WProperty(typeof(string), site.Settings.Description);
        }
    }
}

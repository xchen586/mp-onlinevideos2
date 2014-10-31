﻿using OnlineVideos.Sites.Base;
using OnlineVideos.Sites.WebAutomation.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Windows.Forms;

namespace OnlineVideos.Sites.WebAutomation.ConnectorImplementations
{
    /// <summary>
    /// Base connector which will show a loading dialog
    /// </summary>
    public abstract class BrowserUtilConnectorBase: BrowserUtilConnector
    {
        protected PictureBox _loadingPicture = new PictureBox();

        protected abstract Sites.Entities.EventResult PerformActualLogin(string username, string password);

        /// <summary>
        /// Show the loading dialog, the implementations will need to remove it when the video is fully ready
        /// </summary>
        /// <param name="username"></param>
        /// <param name="password"></param>
        /// <returns></returns>
        public override Sites.Entities.EventResult PerformLogin(string username, string password)
        {
            // Adjust the timer interval here to something longer as the actual timer interval won't be changed until the next OV release
            var timerCtl = Browser.FindForm().GetType().GetField("tmrKeepOnTop", BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic).GetValue(Browser.FindForm()) as Timer;
            if (timerCtl != null)
                timerCtl.Interval = 30000;
            ShowLoading();
            return PerformActualLogin(username, password);
        }

        /// <summary>
        /// Ahow a loading image
        /// </summary>
        public void ShowLoading()
        {
            _loadingPicture.Image = Resources.loading;
            _loadingPicture.Dock = DockStyle.Fill;
            _loadingPicture.SizeMode = PictureBoxSizeMode.CenterImage;
            _loadingPicture.BackColor = Color.Black;
            if (!Browser.FindForm().Controls.Contains(_loadingPicture))
                Browser.FindForm().Controls.Add(_loadingPicture);
            _loadingPicture.BringToFront();
        }
    }
}
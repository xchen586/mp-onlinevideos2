﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel;
using System.Collections;

namespace OnlineVideos.MPUrlSourceFilter
{
    /// <summary>
    /// Represents class for UDP or RTP url settings.
    /// </summary>
    [TypeConverter(typeof(UdpRtpUrlSettingsConverter))]
    public class UdpRtpUrlSettings : SimpleUrlSettings
    {
        #region Private fields

        private int openConnectionTimeout = OnlineVideoSettings.Instance.UdpRtpOpenConnectionTimeout;
        private int openConnectionSleepTime = OnlineVideoSettings.Instance.UdpRtpOpenConnectionSleepTime;
        private int totalReopenConnectionTimeout = OnlineVideoSettings.Instance.UdpRtpTotalReopenConnectionTimeout;
        private int receiveDataCheckInterval = OnlineVideoSettings.Instance.UdpRtpReceiveDataCheckInterval;

        #endregion

        #region Properties

        /// <summary>
        /// Gets or sets the timeout to open UDP or RTP url in milliseconds.
        /// </summary>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <para>The <see cref="OpenConnectionTimeout"/> is lower than zero.</para>
        /// </exception>
        [Category("OnlineVideosUserConfiguration"), Description("The timeout to open UDP or RTP url in milliseconds. It is applied to first opening of url.")]
        public int OpenConnectionTimeout
        {
            get { return this.openConnectionTimeout; }
            set
            {
                if (value < 0)
                {
                    throw new ArgumentOutOfRangeException("OpenConnectionTimeout", value, "Cannot be less than zero.");
                }

                this.openConnectionTimeout = value;
            }
        }

        /// <summary>
        /// Gets or sets the time in milliseconds to sleep before opening connection.
        /// </summary>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <para>The <see cref="OpenConnectionSleepTime"/> is lower than zero.</para>
        /// </exception>
        [Category("OnlineVideosUserConfiguration"), Description("The time in milliseconds to sleep before opening connection.")]
        public int OpenConnectionSleepTime
        {
            get { return this.openConnectionSleepTime; }
            set
            {
                if (value < 0)
                {
                    throw new ArgumentOutOfRangeException("OpenConnectionSleepTime", value, "Cannot be less than zero.");
                }

                this.openConnectionSleepTime = value;
            }
        }

        /// <summary>
        /// Gets or sets the total timeout to open UDP or RTP url in milliseconds.
        /// </summary>
        /// <remarks>
        /// <para>It is applied when lost connection and trying to open new one. Filter will be trying to open connection until this timeout occurs. This parameter is ignored in case of live stream.</para>
        /// </remarks>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <para>The <see cref="TotalReopenConnectionTimeout"/> is lower than zero.</para>
        /// </exception>
        [Category("OnlineVideosUserConfiguration"), Description("The total timeout to open UDP or RTP url in milliseconds. It is applied when lost connection and trying to open new one. Filter will be trying to open connection until this timeout occurs. This parameter is ignored in case of live stream.")]
        public int TotalReopenConnectionTimeout
        {
            get { return this.totalReopenConnectionTimeout; }
            set
            {
                if (value < 0)
                {
                    throw new ArgumentOutOfRangeException("TotalReopenConnectionTimeout", value, "Cannot be less than zero.");
                }

                this.totalReopenConnectionTimeout = value;
            }
        }


        /// <summary>
        /// Gets or sets the receive data check interval in milliseconds.
        /// </summary>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <para>The <see cref="ReceiveDataCheckInterval"/> is lower than zero.</para>
        /// </exception>
        [Category("OnlineVideosUserConfiguration"), Description("If not data are received in specified amount of time in milliseconds, then connection is assumed to be lost.")]
        public int ReceiveDataCheckInterval
        {
            get { return this.receiveDataCheckInterval; }
            set
            {
                if (value < 0)
                {
                    throw new ArgumentOutOfRangeException("ReceiveDataCheckInterval", value, "Cannot be less than zero.");
                }

                this.receiveDataCheckInterval = value;
            }
        }

        #endregion

        #region Constructors

        /// <summary>
        /// Initializes a new instance of <see cref="UdpRtpUrlSettings" /> class.
        /// </summary>
        public UdpRtpUrlSettings()
            : this(String.Empty)
        {
        }

        /// <summary>
        /// Initializes a new instance of <see cref="UdpRtpUrlSettings" /> class with specified UDP or RTP url parameters.
        /// </summary>
        /// <param name="value">UDP or RTP url parameters.</param>
        /// <exception cref="ArgumentNullException">
        /// <para>The <paramref name="value"/> is <see langword="null"/>.</para>
        /// </exception>
        public UdpRtpUrlSettings(String value)
            : base(value)
        {
            Hashtable parameters = SimpleUrlSettings.GetParameters(value);

            this.OpenConnectionTimeout = int.Parse(SimpleUrlSettings.GetValue(parameters, "OpenConnectionTimeout", OnlineVideoSettings.Instance.UdpRtpOpenConnectionTimeout.ToString()));
            this.OpenConnectionSleepTime = int.Parse(SimpleUrlSettings.GetValue(parameters, "OpenConnectionSleepTime", OnlineVideoSettings.Instance.UdpRtpOpenConnectionSleepTime.ToString()));
            this.TotalReopenConnectionTimeout = int.Parse(SimpleUrlSettings.GetValue(parameters, "TotalReopenConnectionTimeout", OnlineVideoSettings.Instance.UdpRtpTotalReopenConnectionTimeout.ToString()));
            this.ReceiveDataCheckInterval = int.Parse(SimpleUrlSettings.GetValue(parameters, "ReceiveDataCheckInterval", OnlineVideoSettings.Instance.UdpRtpReceiveDataCheckInterval.ToString()));
        }

        #endregion

        #region Methods

        /// <summary>
        /// Gets canonical string representation for the specified instance.
        /// </summary>
        /// <returns>
        /// A <see cref="System.String"/> instance that contains the unescaped canonical representation of the this instance.
        /// </returns>
        public override string ToString()
        {
            StringBuilder builder = new StringBuilder();

            builder.Append(base.ToString());
            builder.Append((this.OpenConnectionTimeout != OnlineVideoSettings.Instance.UdpRtpOpenConnectionTimeout) ? String.Format("OpenConnectionTimeout={0};", this.OpenConnectionTimeout) : String.Empty);
            builder.Append((this.OpenConnectionSleepTime != OnlineVideoSettings.Instance.UdpRtpOpenConnectionSleepTime) ? String.Format("OpenConnectionSleepTime={0};", this.OpenConnectionSleepTime) : String.Empty);
            builder.Append((this.TotalReopenConnectionTimeout != OnlineVideoSettings.Instance.UdpRtpTotalReopenConnectionTimeout) ? String.Format("TotalReopenConnectionTimeout={0};", this.TotalReopenConnectionTimeout) : String.Empty);
            builder.Append((this.ReceiveDataCheckInterval != OnlineVideoSettings.Instance.UdpRtpReceiveDataCheckInterval) ? String.Format("ReceiveDataCheckInterval={0};", this.ReceiveDataCheckInterval) : String.Empty);

            return builder.ToString();
        }

        #endregion
    }

}

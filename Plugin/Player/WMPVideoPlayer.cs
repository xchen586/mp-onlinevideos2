using System;
using System.Drawing;
using System.Windows.Forms;
using MediaPortal.GUI.Library;
using MediaPortal.Player;
using AxWMPLib;
using WMPLib;
using ExternalOSDLibrary;

namespace OnlineVideos.Player
{
    public class WMPVideoPlayer : IPlayer
    {
        public enum PlayState { Init, Playing, Paused, Ended };
        
        private static AxWindowsMediaPlayer _wmp10Player = null;
        private string _currentFile = "";
        private PlayState _graphState = PlayState.Init;
        private bool _isFullScreen = false;
        private int _positionX = 10, _positionY = 10, _videoWidth = 100, _videoHeight = 100;        
        private bool _needUpdate = true;
        private bool _notifyPlaying = true;
        private bool _bufferCompleted = true;
        private OSDController _osd;
        private bool _osdActive = false;

        private static void CreateInstance()
        {
            if (_wmp10Player != null) return; // single instance

            _wmp10Player = new AxWindowsMediaPlayer();
            _wmp10Player.BeginInit();
            GUIGraphicsContext.form.SuspendLayout();
            _wmp10Player.Enabled = true;

            //ComponentResourceManager resources = new ComponentResourceManager(typeof (Resource1));
            _wmp10Player.Location = new Point(8, 16);
            _wmp10Player.Name = "axWindowsMediaPlayer1";
            //_wmp10Player.OcxState = ((AxHost.State) (resources.GetObject("axWindowsMediaPlayer1.OcxState")));
            _wmp10Player.Size = new Size(264, 240);
            _wmp10Player.TabIndex = 0;

            GUIGraphicsContext.form.Controls.Add(_wmp10Player);

            try { _wmp10Player.EndInit(); } catch { }

            _wmp10Player.uiMode = "none";
            _wmp10Player.windowlessVideo = true;
            _wmp10Player.enableContextMenu = false;
            _wmp10Player.Ctlenabled = false;
            _wmp10Player.ClientSize = new Size(0, 0);
            _wmp10Player.Visible = false;
            GUIGraphicsContext.form.ResumeLayout(false);
        }

        public override bool Play(string strFile)
        {
            _graphState = PlayState.Init;
            _currentFile = strFile;

            Log.Info("WMPVideoPlayer: Disabling DX9 exclusive mode");
            GUIMessage msg = new GUIMessage(GUIMessage.MessageType.GUI_MSG_SWITCH_FULL_WINDOWED, 0, 0, 0, 0, 0, null);
            GUIWindowManager.SendMessage(msg);

            _notifyPlaying = true;
            GC.Collect();
            CreateInstance();            

            if (_wmp10Player == null) return false;
            
            VideoRendererStatistics.VideoState = VideoRendererStatistics.State.VideoPresent;
            _wmp10Player.ErrorEvent += OnError;
            _wmp10Player.PlayStateChange += OnPlayStateChange;
            _wmp10Player.Buffering += OnBuffering;

            if (_osd == null)
            {
                _osd = OSDController.getInstance();
                GUIWindowManager.OnNewAction += GUIWindowManager_OnNewAction;
            }

            //_wmp10Player.enableContextMenu = false;
            //_wmp10Player.Ctlenabled = false;
            Log.Info("WMPVideoPlayer:play {0}", strFile);
            _wmp10Player.URL = strFile;

            _wmp10Player.network.bufferingTime = OnlineVideoSettings.Instance.wmpbuffer;
            _wmp10Player.Ctlcontrols.play();
            _wmp10Player.ClientSize = new Size(0, 0);
            _wmp10Player.Visible = false;

            // When file is internetstream
            if (_wmp10Player.URL.ToLower().StartsWith("http") || _wmp10Player.URL.ToLower().StartsWith("mms"))
            {
                _bufferCompleted = false;
                using (WaitCursor waitcursor = new WaitCursor())
                {
                    GUIGraphicsContext.Overlay = false;
                    while (_bufferCompleted != true)
                    {
                        {
                            // if true then could not load stream 
                            if (_wmp10Player.playState.Equals(WMPPlayState.wmppsPlaying))
                            {
                                _bufferCompleted = true;
                            }
                            if (GUIGraphicsContext.Overlay)
                            {
                                GUIGraphicsContext.Overlay = false;
                            }
                            _graphState = PlayState.Playing;
                            GUIWindowManager.Process();
                        }
                    }
                    GUIGraphicsContext.Overlay = true;
                }

                if (_bufferCompleted && _wmp10Player.playState.Equals(WMPPlayState.wmppsReady))
                {
                    Log.Info("WMPVideoPlayer: failed to load {0}", strFile);
                    return false;
                }
            }

            GUIMessage msgPb = new GUIMessage(GUIMessage.MessageType.GUI_MSG_PLAYBACK_STARTED, 0, 0, 0, 0, 0, null);
            msgPb.Label = strFile;

            GUIWindowManager.SendThreadMessage(msgPb);
            _graphState = PlayState.Playing;
            GC.Collect();
            _needUpdate = true;
            _isFullScreen = GUIGraphicsContext.IsFullScreenVideo;
            _positionX = GUIGraphicsContext.VideoWindow.Left;
            _positionY = GUIGraphicsContext.VideoWindow.Top;
            _videoWidth = GUIGraphicsContext.VideoWindow.Width;
            _videoHeight = GUIGraphicsContext.VideoWindow.Height;

            SetVideoWindow();

            return true;
        }

        void GUIWindowManager_OnNewAction(Action action)
        {
            if (_graphState == PlayState.Playing || _graphState == PlayState.Paused)
            {
                switch (action.wID)
                {
                    case Action.ActionType.ACTION_BIG_STEP_FORWARD:
                    case Action.ActionType.ACTION_BIG_STEP_BACK:
                    case Action.ActionType.ACTION_STEP_BACK:
                    case Action.ActionType.ACTION_STEP_FORWARD:
                        if (!_osdActive) { _osd.Activate(); _osdActive = true; }
                        break;
                }
            }
        }

        private void OnPlayStateChange(object sender, _WMPOCXEvents_PlayStateChangeEvent e)
        {
            if (_wmp10Player == null)
            {
                return;
            }
            switch (_wmp10Player.playState)
            {
                case WMPPlayState.wmppsStopped:
                    PlaybackEnded(false);
                    break;
                case WMPPlayState.wmppsMediaEnded:
                    if (_wmp10Player.currentMedia.isMemberOf(_wmp10Player.currentPlaylist))
                    {
                        if (_wmp10Player.currentMedia.get_isIdentical(_wmp10Player.currentPlaylist.get_Item(_wmp10Player.currentPlaylist.count - 1)))
                        {
                            PlaybackEnded(false);
                        }
                    }
                    else
                        PlaybackEnded(false);
                    break;
            }
        }

        private void OnError(object sender, EventArgs e)
        {
            string err = _wmp10Player.Error.get_Item(0).errorDescription;
            Log.Error("WMPVideoPlayer: " + err);
        }

        private void OnBuffering(object sender, _WMPOCXEvents_BufferingEvent e)
        {
            Log.Debug("WMPVideoPlayer: bandWidth: {0}", _wmp10Player.network.bandWidth);
            Log.Debug("WMPVideoPlayer: bitRate: {0}", _wmp10Player.network.bitRate);
            Log.Debug("WMPVideoPlayer: receivedPackets: {0}", _wmp10Player.network.receivedPackets);
            Log.Debug("WMPVideoPlayer: receptionQuality: {0}", _wmp10Player.network.receptionQuality);

            //_wmp10Player.network.bufferingTime = _bufferTime;

            if (e.start)
            {
                _bufferCompleted = false;
                Log.Debug("WMPVideoPlayer: bandWidth: {0}", _wmp10Player.network.bandWidth);
                Log.Debug("WMPVideoPlayer: receptionQuality: {0}", _wmp10Player.network.receptionQuality);
            }
            if (!e.start)
            {
                _bufferCompleted = true;
            }
        }

        private void PlaybackEnded(bool bManualStop)
        {
            // this is triggered only if movie has ended
            // ifso, stop the movie which will trigger MovieStopped

            //if (!Util.Utils.IsAudio(_currentFile))
            //{
            GUIGraphicsContext.IsFullScreenVideo = false;
            //}
            Log.Info("WMPVideoPlayer:ended {0} {1}", _currentFile, bManualStop);
            _currentFile = "";
            if (_osdActive) { _osd.Deactivate(); _osdActive = false; }
            if (_wmp10Player != null)
            {
                _bufferCompleted = true;
                _wmp10Player.ClientSize = new Size(0, 0);
                _wmp10Player.Visible = false;
                _wmp10Player.PlayStateChange -= OnPlayStateChange;
                _wmp10Player.Buffering -= OnBuffering;
            }
            //GUIGraphicsContext.IsFullScreenVideo=false;
            GUIGraphicsContext.IsPlaying = false;
            if (!bManualStop)
            {
                _graphState = PlayState.Ended;
            }
            else
            {
                _graphState = PlayState.Init;
            }
            GC.Collect();
        }


        public override bool Ended
        {
            get { return _graphState == PlayState.Ended; }
        }

        public override double Duration
        {
            get
            {
                if (_graphState != PlayState.Init && _wmp10Player != null)
                {
                    try
                    {
                        return _wmp10Player.currentMedia.duration;
                    }
                    catch (Exception)
                    {
                    }
                }
                return 0.0d;
            }
        }

        public override double CurrentPosition
        {
            get
            {
                try
                {
                    return _wmp10Player.Ctlcontrols.currentPosition;
                }
                catch (Exception)
                {
                }
                return 0.0d;
            }
        }

        public override void Pause()
        {
            if (_wmp10Player == null)
            {
                return;
            }
            if (_graphState == PlayState.Paused)
            {
                if (_osdActive) { _osd.Deactivate(); _osdActive = false; }
                _graphState = PlayState.Playing;
                _wmp10Player.Ctlcontrols.play();
            }
            else if (_graphState == PlayState.Playing)
            {
                if (!_osdActive) { _osd.Activate(); _osdActive = true; }
                _wmp10Player.Ctlcontrols.pause();
                if (_wmp10Player.playState == WMPPlayState.wmppsPaused)
                {
                    _graphState = PlayState.Paused;
                }
            }
        }

        public override bool Paused
        {
            get { return (_graphState == PlayState.Paused); }
        }

        public override bool Playing
        {
            get { return (_graphState == PlayState.Playing || _graphState == PlayState.Paused); }
        }

        public override bool Stopped
        {
            get { return (_graphState == PlayState.Init || _graphState == PlayState.Ended); }
        }

        public override string CurrentFile
        {
            get { return _currentFile; }
        }

        public override void Stop()
        {
            if (_wmp10Player == null)
            {
                return;
            }
            if (_graphState != PlayState.Init)
            {
                _wmp10Player.Ctlcontrols.stop();
                _wmp10Player.ClientSize = new Size(0, 0);
                _wmp10Player.Visible = false;
                PlaybackEnded(true);
            }
        }

        public override int Volume
        {
            get
            {
                if (_wmp10Player == null)
                {
                    return 100;
                }
                return _wmp10Player.settings.volume;
            }
            set
            {
                if (_wmp10Player == null)
                {
                    return;
                }
                if (_wmp10Player.settings.volume != value)
                {
                    _wmp10Player.settings.volume = value;
                }
            }
        }

        public override bool HasVideo
        {
            get { return true; }
        }

        #if !MP102
        public override bool HasViz
        {
            get { return true; }
        }
        #endif

        public override bool IsCDA
        {
            get { return false; }
        }

        #region IDisposable Members

        public override void Release()
        {
            if (_wmp10Player == null)
            {
                return;
            }
            _wmp10Player.ClientSize = new Size(0, 0);
            _wmp10Player.Visible = false;
            /*
            try
            {
              GUIGraphicsContext.form.Controls.Remove(_wmp10Player);
            }
            catch (Exception) { }
            _wmp10Player.Dispose();
            _wmp10Player = null;
             * */
        }

        #endregion

        public override bool FullScreen
        {
            get { return GUIGraphicsContext.IsFullScreenVideo; }
            set
            {
                if (value != _isFullScreen)
                {
                    _isFullScreen = value;
                    _needUpdate = true;
                }
            }
        }

        public override int PositionX
        {
            get { return _positionX; }
            set
            {
                if (value != _positionX)
                {
                    _positionX = value;
                    _needUpdate = true;
                }
            }
        }

        public override int PositionY
        {
            get { return _positionY; }
            set
            {
                if (value != _positionY)
                {
                    _positionY = value;
                    _needUpdate = true;
                }
            }
        }

        public override int RenderWidth
        {
            get { return _videoWidth; }
            set
            {
                if (value != _videoWidth)
                {
                    _videoWidth = value;
                    _needUpdate = true;
                }
            }
        }

        public override int RenderHeight
        {
            get { return _videoHeight; }
            set
            {
                if (value != _videoHeight)
                {
                    _videoHeight = value;
                    _needUpdate = true;
                }
            }
        }

        public override void Process()
        {
            if (!Playing)
            {
                return;
            }
            if (_wmp10Player == null)
            {
                return;
            }

            if (_osd != null) _osd.UpdateGUI();

            if (GUIGraphicsContext.BlankScreen ||
                (GUIGraphicsContext.Overlay == false && GUIGraphicsContext.IsFullScreenVideo == false))
            {
                if (_wmp10Player.Visible)
                {
                    _wmp10Player.ClientSize = new Size(0, 0);
                    _wmp10Player.Visible = false;
                    //_wmp10Player.uiMode = "invisible";
                }
            }
            else if (!_wmp10Player.Visible)
            {
                _needUpdate = true;
                SetVideoWindow();
                //_wmp10Player.uiMode = "none";
                _wmp10Player.Visible = true;
            }

            if (CurrentPosition >= 10.0)
            {
                if (_notifyPlaying)
                {
                    _notifyPlaying = false;
                    GUIMessage msg = new GUIMessage(GUIMessage.MessageType.GUI_MSG_PLAYING_10SEC, 0, 0, 0, 0, 0, null);
                    msg.Label = CurrentFile;
                    GUIWindowManager.SendThreadMessage(msg);
                }
            }
        }

        public delegate void SafeInvoke();

        public override void SetVideoWindow()
        {
            if (_wmp10Player == null)
            {
                return;
            }
            if (GUIGraphicsContext.IsFullScreenVideo != _isFullScreen)
            {
                _isFullScreen = GUIGraphicsContext.IsFullScreenVideo;
                _needUpdate = true;
            }
            if (!_needUpdate)
            {
                return;
            }
            _needUpdate = false;

            if (_isFullScreen)
            {
                Log.Info("WMPVideoPlayer:Fullscreen");

                _positionX = GUIGraphicsContext.OverScanLeft;
                _positionY = GUIGraphicsContext.OverScanTop;
                _videoWidth = GUIGraphicsContext.OverScanWidth;
                _videoHeight = GUIGraphicsContext.OverScanHeight;

                SafeInvoke si = new SafeInvoke(delegate()
                                                 {
                                                     _wmp10Player.Location = new Point(0, 0);
                                                     _wmp10Player.ClientSize = new Size(GUIGraphicsContext.Width,
                                                                                        GUIGraphicsContext.Height);
                                                     _wmp10Player.Size = new Size(GUIGraphicsContext.Width,
                                                                                  GUIGraphicsContext.Height);
                                                     _wmp10Player.stretchToFit = true;
                                                 });

                if (_wmp10Player.InvokeRequired)
                {
                    IAsyncResult iar = _wmp10Player.BeginInvoke(si);
                    iar.AsyncWaitHandle.WaitOne();
                }
                else
                {
                    si();
                }

                _videoRectangle = new Rectangle(0, 0, _wmp10Player.ClientSize.Width, _wmp10Player.ClientSize.Height);
                _sourceRectangle = _videoRectangle;

                //_wmp10Player.fullScreen=true;
                Log.Info("WMPVideoPlayer:done");
                return;
            }
            else
            {
                SafeInvoke si = new SafeInvoke(delegate()
                                                 {
                                                     _wmp10Player.ClientSize = new Size(_videoWidth, _videoHeight);
                                                     _wmp10Player.Location = new Point(_positionX, _positionY);
                                                 });
                if (_wmp10Player.InvokeRequired)
                {
                    IAsyncResult iar = _wmp10Player.BeginInvoke(si);
                    iar.AsyncWaitHandle.WaitOne();
                }
                else
                {
                    si();
                }
                _videoRectangle = new Rectangle(_positionX, _positionY, _wmp10Player.ClientSize.Width,
                                                _wmp10Player.ClientSize.Height);
                _sourceRectangle = _videoRectangle;
                //Log.Info("WMPVideoPlayer:set window:({0},{1})-({2},{3})",_positionX,_positionY,_positionX+_wmp10Player.ClientSize.Width,_positionY+_wmp10Player.ClientSize.Height);
            }
            //_wmp10Player.uiMode = "none";
            //_wmp10Player.windowlessVideo = true;
            //_wmp10Player.enableContextMenu = false;
            //_wmp10Player.Ctlenabled = false;
            GUIGraphicsContext.form.Controls[0].Enabled = false;
        }

        /*
            public override int AudioStreams
            {
              get { return _wmp10Player.Ctlcontrols.audioLanguageCount;}
            }
            public override int CurrentAudioStream
            {
              get { return _wmp10Player.Ctlcontrols.currentAudioLanguage;}
              set { _wmp10Player.Ctlcontrols.currentAudioLanguage=value;}
            }
            public override string AudioLanguage(int iStream)
            {
              return _wmp10Player.controls.getLanguageName(iStream);
            }
        */

        public override void SeekRelative(double dTime)
        {
            if (_wmp10Player == null)
            {
                return;
            }
            if (_graphState != PlayState.Init)
            {
                double dCurTime = CurrentPosition;
                dTime = dCurTime + dTime;
                if (dTime < 0.0d)
                {
                    dTime = 0.0d;
                }
                if (dTime < Duration)
                {
                    _wmp10Player.Ctlcontrols.currentPosition = dTime;
                }
            }
        }

        public override void SeekAbsolute(double dTime)
        {
            if (_wmp10Player == null)
            {
                return;
            }
            if (_graphState != PlayState.Init)
            {
                if (dTime < 0.0d)
                {
                    dTime = 0.0d;
                }
                if (dTime < Duration)
                {
                    _wmp10Player.Ctlcontrols.currentPosition = dTime;
                }
                if (_osdActive) { _osd.Deactivate(); _osdActive = false; }
            }
        }

        public override void SeekRelativePercentage(int iPercentage)
        {
            if (_wmp10Player == null)
            {
                return;
            }
            if (_graphState != PlayState.Init)
            {
                double dCurrentPos = CurrentPosition;
                double dDuration = Duration;

                double fCurPercent = (dCurrentPos / Duration) * 100.0d;
                double fOnePercent = Duration / 100.0d;
                fCurPercent = fCurPercent + (double)iPercentage;
                fCurPercent *= fOnePercent;
                if (fCurPercent < 0.0d)
                {
                    fCurPercent = 0.0d;
                }
                if (fCurPercent < Duration)
                {
                    _wmp10Player.Ctlcontrols.currentPosition = fCurPercent;
                }
            }
        }


        public override void SeekAsolutePercentage(int iPercentage)
        {
            if (_wmp10Player == null)
            {
                return;
            }
            if (_graphState != PlayState.Init)
            {
                if (iPercentage < 0)
                {
                    iPercentage = 0;
                }
                if (iPercentage >= 100)
                {
                    iPercentage = 100;
                }
                double fPercent = Duration / 100.0f;
                fPercent *= (double)iPercentage;
                _wmp10Player.Ctlcontrols.currentPosition = fPercent;
            }
        }

        public override int Speed
        {
            get
            {
                if (_graphState == PlayState.Init)
                {
                    return 1;
                }
                if (_wmp10Player == null)
                {
                    return 1;
                }
                return (int)_wmp10Player.settings.rate;
            }
            set
            {
                if (_wmp10Player == null)
                {
                    return;
                }
                if (_graphState != PlayState.Init)
                {
                    if (value < 0)
                    {
                        _wmp10Player.Ctlcontrols.currentPosition += (double)value;                        
                    }
                    else
                    {
                        try
                        {
                            _wmp10Player.settings.rate = (double)value;
                        }
                        catch (Exception)
                        {
                        }                        
                    }
                }
            }
        }
    }
}
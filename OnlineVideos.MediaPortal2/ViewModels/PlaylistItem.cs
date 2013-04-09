﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MediaPortal.Common.MediaManagement;
using MediaPortal.Common.MediaManagement.DefaultItemAspects;
using MediaPortal.Common;
using MediaPortal.Common.SystemResolver;

namespace OnlineVideos.MediaPortal2
{
	public class PlaylistItem : MediaItem
	{
		public PlaylistItem(VideoViewModel videoInfo, string resolvedPlaybackUrl)
			: base(Guid.Empty, new Dictionary<Guid, MediaItemAspect>()
			{
				{ ProviderResourceAspect.ASPECT_ID, new MediaItemAspect(ProviderResourceAspect.Metadata)},
				{ MediaAspect.ASPECT_ID, new MediaItemAspect(MediaAspect.Metadata) },
				{ VideoAspect.ASPECT_ID, new MediaItemAspect(VideoAspect.Metadata) }
			})
		{
			Aspects[ProviderResourceAspect.ASPECT_ID].SetAttribute(ProviderResourceAspect.ATTR_SYSTEM_ID, ServiceRegistration.Get<ISystemResolver>().LocalSystemId);
			Aspects[ProviderResourceAspect.ASPECT_ID].SetAttribute(ProviderResourceAspect.ATTR_RESOURCE_ACCESSOR_PATH, RawUrlMediaProvider.ToProviderResourcePath(resolvedPlaybackUrl).Serialize());
			
			Aspects[MediaAspect.ASPECT_ID].SetAttribute(MediaAspect.ATTR_MIME_TYPE, OnlineVideosPlayer.ONLINEVIDEOS_MIMETYPE);
			Aspects[MediaAspect.ASPECT_ID].SetAttribute(MediaAspect.ATTR_TITLE, videoInfo.Title);
			
			Aspects[VideoAspect.ASPECT_ID].SetAttribute(VideoAspect.ATTR_STORYPLOT, videoInfo.Description);

			DateTime parsedAirDate;
			if (DateTime.TryParse(videoInfo.VideoInfo.Airdate, out parsedAirDate))
				Aspects[MediaAspect.ASPECT_ID].SetAttribute(MediaAspect.ATTR_RECORDINGTIME, parsedAirDate);

		}
	}
}

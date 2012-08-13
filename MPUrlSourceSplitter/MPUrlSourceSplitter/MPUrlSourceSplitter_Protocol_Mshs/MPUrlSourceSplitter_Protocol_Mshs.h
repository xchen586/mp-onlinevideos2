/*
    Copyright (C) 2007-2010 Team MediaPortal
    http://www.team-mediaportal.com

    This file is part of MediaPortal 2

    MediaPortal 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MediaPortal 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MediaPortal 2.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifndef __MPURLSOURCESPLITTER_PROTOCOL_MSHS_DEFINED
#define __MPURLSOURCESPLITTER_PROTOCOL_AFHS_DEFINED

#include "MPUrlSourceSplitter_Protocol_Mshs_Exports.h"
#include "Logger.h"
#include "IProtocolPlugin.h"
#include "LinearBufferCollection.h"
#include "HttpCurlInstance.h"
#include "StreamFragmentCollection.h"

#include "MSHSSmoothStreamingMedia.h"
#include "Box.h"
#include "FileTypeBox.h"
#include "TrackFragmentHeaderBox.h"
#include "MovieBox.h"
#include "TrackBox.h"
#include "HandlerBox.h"
#include "TrackHeaderBox.h"
#include "MediaBox.h"
#include "MediaHeaderBox.h"
#include "MediaInformationBox.h"
#include "VideoMediaHeaderBox.h"
#include "SoundMediaHeaderBox.h"
#include "DataInformationBox.h"
#include "DataReferenceBox.h"
#include "DataEntryUrlBox.h"
#include "SampleTableBox.h"
#include "SampleDescriptionBox.h"
#include "TimeToSampleBox.h"
#include "SampleToChunkBox.h"
#include "ChunkOffsetBox.h"
#include "MovieExtendsBox.h"
#include "TrackExtendsBox.h"
#include "VisualSampleEntryBox.h"
#include "AudioSampleEntryBox.h"
#include "AVCConfigurationBox.h"
#include "ESDBox.h"

#include <curl/curl.h>

#include <WinSock2.h>

#define PROTOCOL_NAME                                                         L"MSHS"

#define TOTAL_SUPPORTED_PROTOCOLS                                             1
wchar_t *SUPPORTED_PROTOCOLS[TOTAL_SUPPORTED_PROTOCOLS] =                     { L"MSHS" };

#define MINIMUM_RECEIVED_DATA_FOR_SPLITTER                                    1 * 1024 * 1024

// This class is exported from the CMPUrlSourceSplitter_Protocol_Mshs.dll
class MPURLSOURCESPLITTER_PROTOCOL_MSHS_API CMPUrlSourceSplitter_Protocol_Mshs : public IProtocolPlugin
{
public:
  // constructor
  // create instance of CMPUrlSourceSplitter_Protocol_Mshs class
  CMPUrlSourceSplitter_Protocol_Mshs(CParameterCollection *configuration);

  // destructor
  ~CMPUrlSourceSplitter_Protocol_Mshs(void);

  // IProtocol interface

  // test if connection is opened
  // @return : true if connected, false otherwise
  bool IsConnected(void);

  // get protocol maximum open connection attempts
  // @return : maximum attempts of opening connections or UINT_MAX if error
  unsigned int GetOpenConnectionMaximumAttempts(void);

  // parse given url to internal variables for specified protocol
  // errors should be logged to log file
  // @param parameters : the url and connection parameters
  // @return : S_OK if successfull
  HRESULT ParseUrl(const CParameterCollection *parameters);

  // receive data and stores them into internal buffer
  // @param shouldExit : the reference to variable specifying if method have to be finished immediately
  void ReceiveData(bool *shouldExit);

  // ISimpleProtocol interface

  // get timeout (in ms) for receiving data
  // @return : timeout (in ms) for receiving data
  unsigned int GetReceiveDataTimeout(void);

  // starts receiving data from specified url and configuration parameters
  // @param parameters : the url and parameters used for connection
  // @return : S_OK if url is loaded, false otherwise
  HRESULT StartReceivingData(const CParameterCollection *parameters);

  // request protocol implementation to cancel the stream reading operation
  // @return : S_OK if successful
  HRESULT StopReceivingData(void);

  // retrieves the progress of the stream reading operation
  // @param total : reference to a variable that receives the length of the entire stream, in bytes
  // @param current : reference to a variable that receives the length of the downloaded portion of the stream, in bytes
  // @return : S_OK if successful, VFW_S_ESTIMATED if returned values are estimates, E_UNEXPECTED if unexpected error
  HRESULT QueryStreamProgress(LONGLONG *total, LONGLONG *current);
  
  // retrieves available lenght of stream
  // @param available : reference to instance of class that receives the available length of stream, in bytes
  // @return : S_OK if successful, other error codes if error
  HRESULT QueryStreamAvailableLength(CStreamAvailableLength *availableLength);

  // clear current session
  // @return : S_OK if successfull
  HRESULT ClearSession(void);

  // ISeeking interface

  // gets seeking capabilities of protocol
  // @return : bitwise combination of SEEKING_METHOD flags
  unsigned int GetSeekingCapabilities(void);

  // request protocol implementation to receive data from specified time (in ms)
  // @param time : the requested time (zero is start of stream)
  // @return : time (in ms) where seek finished or lower than zero if error
  int64_t SeekToTime(int64_t time);

  // request protocol implementation to receive data from specified position to specified position
  // @param start : the requested start position (zero is start of stream)
  // @param end : the requested end position, if end position is lower or equal to start position than end position is not specified
  // @return : position where seek finished or lower than zero if error
  int64_t SeekToPosition(int64_t start, int64_t end);

  // sets if protocol implementation have to supress sending data to filter
  // @param supressData : true if protocol have to supress sending data to filter, false otherwise
  void SetSupressData(bool supressData);

  // IPlugin interface

  // return reference to null-terminated string which represents plugin name
  // function have to allocate enough memory for plugin name string
  // errors should be logged to log file and returned NULL
  // @return : reference to null-terminated string
  const wchar_t *GetName(void);

  // get plugin instance ID
  // @return : GUID, which represents instance identifier or GUID_NULL if error
  GUID GetInstanceId(void);

  // initialize plugin implementation with configuration parameters
  // @param configuration : the reference to additional configuration parameters (created by plugin's hoster class)
  // @return : S_OK if successfull
  HRESULT Initialize(PluginConfiguration *configuration);

protected:
  CLogger *logger;

  // source filter that created this instance
  IOutputStream *filter;

  // holds various parameters supplied by caller
  CParameterCollection *configurationParameters;

  // holds receive data timeout
  unsigned int receiveDataTimeout;

  // holds open connection maximum attempts
  unsigned int openConnetionMaximumAttempts;

  // the lenght of stream
  LONGLONG streamLength;

  // holds if length of stream was set
  bool setLength;

  // stream time
  int64_t streamTime;

  // specifies position in buffer
  // it is always reset on seek
  int64_t bytePosition;

  // mutex for locking access to file, buffer, ...
  HANDLE lockMutex;

  // main instance of CURL
  CHttpCurlInstance *mainCurlInstance;

  // CURL instance for first video URL
  CHttpCurlInstance *videoCurlInstance;
  // CURL instance for first audio URL
  CHttpCurlInstance *audioCurlInstance;

  // reference to variable that signalize if protocol is requested to exit
  bool shouldExit;
  // specifies if whole stream is downloaded
  bool wholeStreamDownloaded;
  // specifies if seeking (cleared when first data arrive)
  bool seekingActive;
  // specifies if filter requested supressing data
  bool supressData;

  // buffer for processing box data before are send to further processing
  CLinearBufferCollection *bufferForBoxProcessingCollection;

  // buffer for processing data before are send to filter
  CLinearBuffer *bufferForProcessing;

  // holds stream fragments and urls
  CStreamFragmentCollection *streamFragments;

  // holds smooth streaming media for further processing
  CMSHSSmoothStreamingMedia *streamingMedia;

  // specifies if last fragment was downloaded
  bool lastFragmentDownloaded;

  // removes all downloaded stream fragments
  // the last one stream fragment (even downloaded) still preserve
  void RemoveAllDownloadedStreamFragments(void);

  // gets first not downloaded stream fragment
  // @return : first not downloaded stream fragment or NULL if not exists
  CStreamFragment *GetFirstNotDownloadedStreamFragment(void);

  // gets stream fragments collection created from manifest
  // @param logger : the logger for logging purposes
  // @param methodName : the name of method calling GetStreamFragmentsFromManifest()
  // @param configurationParameters : the configuration parameters
  // @param manifest : manifest to create stream fragments collection
  // @param logCollection : specifies if result collection should be logged
  // @return : stream fragments collection created from manifest or NULL if error
  CStreamFragmentCollection *GetStreamFragmentsFromManifest(CLogger *logger, const wchar_t *methodName, CParameterCollection *configurationParameters, CMSHSSmoothStreamingMedia *manifest, bool logCollection);

  // formats url
  // @param baseUrl : the base url (manifest url)
  // @param urlPattern : the url pattern
  // @param track : track for which is url formatted
  // @param fragment : fragment for which is url formatted
  // @return : url or NULL if error
  wchar_t *FormatUrl(const wchar_t *baseUrl, const wchar_t *urlPattern, CMSHSTrack *track, CMSHSStreamFragment *fragment);

  // creates file type box
  // @return : file type box or NULL if error
  CFileTypeBox *CreateFileTypeBox(void);

  // gets track fragment header box from linear buffer (video or audio CURL instance)
  // @param buffer : buffer to get track fragment header box
  // @return : track fragment header box or NULL if error
  CTrackFragmentHeaderBox *GetTrackFragmentHeaderBox(CLinearBuffer *buffer);

  // stores box into buffer
  // @param box : box to store in buffer
  // @param buffer : the buffer
  // @return : true if successful, false otherwise
  bool PutBoxIntoBuffer(CBox *box, CLinearBuffer *buffer);

  // gets movie box
  // @param media : smooth streaming media
  // @param videoFragmentHeaderBox : video fragment header box
  // @param audioFragmentHeaderBox : audio fragment header box
  // @return : movie box or NULL if error
  CMovieBox *GetMovieBox(CMSHSSmoothStreamingMedia *media, CTrackFragmentHeaderBox *videoFragmentHeaderBox, CTrackFragmentHeaderBox *audioFragmentHeaderBox);

  // gets video track box
  // @param media : smooth streaming media
  // @param streamIndex : the index of stream
  // @param trackIndex : the index of track in stream
  // @param fragmentHeaderBox : fragment header box
  // @return : video track box or NULL if error
  CTrackBox *GetVideoTrackBox(CMSHSSmoothStreamingMedia *media, unsigned int streamIndex, unsigned int trackIndex, CTrackFragmentHeaderBox *fragmentHeaderBox);

  // gets audio track box
  // @param media : smooth streaming media
  // @param fragmentHeaderBox : fragment header box
  // @return : audio track box or NULL if error
  CTrackBox *GetAudioTrackBox(CMSHSSmoothStreamingMedia *media, unsigned int streamIndex, unsigned int trackIndex, CTrackFragmentHeaderBox *fragmentHeaderBox);

  CDataInformationBox *GetDataInformationBox(void);

  CHandlerBox *GetHandlerBox(uint32_t handlerType, const wchar_t *handlerName);

  CSampleTableBox *GetVideoSampleTableBox(CMSHSSmoothStreamingMedia *media, unsigned int streamIndex, unsigned int trackIndex, CTrackFragmentHeaderBox *fragmentHeaderBox);

  CSampleTableBox *GetAudioSampleTableBox(CMSHSSmoothStreamingMedia *media, unsigned int streamIndex, unsigned int trackIndex, CTrackFragmentHeaderBox *fragmentHeaderBox);
};

#endif

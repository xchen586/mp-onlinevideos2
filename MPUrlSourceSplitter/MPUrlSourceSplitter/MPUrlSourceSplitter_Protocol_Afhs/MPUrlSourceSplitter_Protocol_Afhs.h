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

#ifndef __MPURLSOURCESPLITTER_PROTOCOL_AFHS_DEFINED
#define __MPURLSOURCESPLITTER_PROTOCOL_AFHS_DEFINED

#include "MPUrlSourceSplitter_Protocol_Afhs_Exports.h"
#include "Logger.h"
#include "IProtocolPlugin.h"
#include "LinearBufferCollection.h"
#include "HttpCurlInstance.h"
#include "BootstrapInfoBox.h"
#include "SegmentFragmentCollection.h"
#include "ParameterCollection.h"
#include "AfhsDecryptionHoster.h"

#include <curl/curl.h>

#include <WinSock2.h>

#define PROTOCOL_NAME                                                         L"AFHS"

#define TOTAL_SUPPORTED_PROTOCOLS                                             1
wchar_t *SUPPORTED_PROTOCOLS[TOTAL_SUPPORTED_PROTOCOLS] =                     { L"AFHS" };

#define FLV_FILE_HEADER_LENGTH                                                13
unsigned char FLV_FILE_HEADER[FLV_FILE_HEADER_LENGTH] =                       { 0x46, 0x4C, 0x56, 0x01, 0x05, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00 };

// defines delay between two bootstrap info requests
#define LAST_REQUEST_BOOTSTRAP_INFO_DELAY                                     10000

// This class is exported from the CMPUrlSourceSplitter_Protocol_Afhs.dll
class MPURLSOURCESPLITTER_PROTOCOL_AFHS_API CMPUrlSourceSplitter_Protocol_Afhs : public IProtocolPlugin
{
public:
  // constructor
  // create instance of CMPUrlSourceSplitter_Protocol_Afhs class
  CMPUrlSourceSplitter_Protocol_Afhs(CParameterCollection *configuration);

  // destructor
  ~CMPUrlSourceSplitter_Protocol_Afhs(void);

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

  // receives data and stores them into receive data parameter
  // @param shouldExit : the reference to variable specifying if method have to be finished immediately
  // @param receiveData : received data
  // @result: S_OK if successful, error code otherwise
  HRESULT ReceiveData(bool *shouldExit, CReceiveData *receiveData);

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
  // holds if end of stream was set
  bool setEndOfStream;

  // stream time
  int64_t streamTime;

  // specifies position in buffer
  // it is always reset on seek
  int64_t bytePosition;

  // mutex for locking access to file, buffer, ...
  HANDLE lockMutex;

  // main instance of CURL
  CHttpCurlInstance *mainCurlInstance;
  // CURL instance for downloading bootstrap info for live session
  CHttpCurlInstance *bootstrapInfoCurlInstance;

  // reference to variable that signalize if protocol is requested to exit
  bool shouldExit;
  // specifies if whole stream is downloaded
  bool wholeStreamDownloaded;
  // specifies if seeking (cleared when first data arrive)
  bool seekingActive;
  // specifies if filter requested supressing data
  bool supressData;

  // holds which segment and fragment is currently downloading (UINT_MAX means none)
  unsigned int segmentFragmentDownloading;
  // holds which segment and fragment is currently processed
  unsigned int segmentFragmentProcessing;
  // holds which segment and fragment have to be downloaded
  // (UINT_MAX means next segment fragment, always reset after started download of segment and fragment)
  unsigned int segmentFragmentToDownload;

  // buffer for processing data before are send to filter
  CLinearBuffer *bufferForProcessing;

  //// holds first FLV packet timestamp for correction of video packet timestamps
  //int firstTimestamp;

  //// holds first video FLV packet timestamp for correction of video packet timestamps
  //int firstVideoTimestamp;

  // holds bootstrap info box - segments, fragments, duration, seek information
  CBootstrapInfoBox *bootstrapInfoBox;
  // holds collection of segments, fragments and urls
  CSegmentFragmentCollection *segmentsFragments;
  // specifies if working with live stream
  bool live;
  // holds last bootstrap info request time for live streaming
  DWORD lastBootstrapInfoRequestTime;

  // gets segment and fragment collection created from bootstrap info box
  // @param logger : the logger for logging purposes
  // @param methodName : the name of method calling GetSegmentsFragmentsFromBootstrapInfoBox()
  // @param configurationParameters : the configuration parameters
  // @param bootstrapInfoBox : bootstrap info box to create segment and fragment collection
  // @param logCollection : specifies if result collection should be logged
  // @return : segment and fragment collection created from bootstrap info box or NULL if error
  CSegmentFragmentCollection *GetSegmentsFragmentsFromBootstrapInfoBox(CLogger *logger, const wchar_t *methodName, CParameterCollection *configurationParameters, CBootstrapInfoBox *bootstrapInfoBox, bool logCollection);

  // gets store file path based on configuration
  // creates folder structure if not created
  // @return : store file or NULL if error
  wchar_t *GetStoreFile(void);

  // holds store file path
  wchar_t *storeFilePath;
  // holds last store time of storing segments and fragments to file
  DWORD lastStoreTime;

  // specifies if we are still connected
  bool isConnected;

  // fills buffer for processing with segment and fragment data (stored in memory or in store file)
  // @param segmentsFragments : segments and fragments collection
  // @param segmentFragmentProcessing : segment and fragment to get data
  // @param storeFile : the name of store file
  // @return : buffer for processing with filled data, NULL otherwise
  CLinearBuffer *FillBufferForProcessing(CSegmentFragmentCollection *segmentsFragments, unsigned int segmentFragmentProcessing, wchar_t *storeFile);

  // holds decryption hoster
  CAfhsDecryptionHoster *decryptionHoster;
};

#endif

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

#include "stdafx.h"

#include "MPUrlSourceSplitter_Protocol_Afhs.h"
#include "MPUrlSourceSplitter_Protocol_Afhs_Parameters.h"
#include "Utilities.h"
#include "LockMutex.h"
#include "VersionInfo.h"
#include "formatUrl.h"
#include "ErrorCodes.h"
#include "Utilities.h"

#include "base64.h"

#include "Box.h"
#include "BootstrapInfoBox.h"
#include "MediaDataBox.h"

#include "FlvPacket.h"

#include "F4MManifest.h"

#include <WinInet.h>
#include <stdio.h>

// protocol implementation name
#ifdef _DEBUG
#define PROTOCOL_IMPLEMENTATION_NAME                                          L"MPUrlSourceSplitter_Protocol_Afhsd"
#else
#define PROTOCOL_IMPLEMENTATION_NAME                                          L"MPUrlSourceSplitter_Protocol_Afhs"
#endif

#define METHOD_FILL_BUFFER_FOR_PROCESSING_NAME                                L"FillBufferForProcessing()"

PIPlugin CreatePluginInstance(CParameterCollection *configuration)
{
  return new CMPUrlSourceSplitter_Protocol_Afhs(configuration);
}

void DestroyPluginInstance(PIPlugin pProtocol)
{
  if (pProtocol != NULL)
  {
    CMPUrlSourceSplitter_Protocol_Afhs *pClass = (CMPUrlSourceSplitter_Protocol_Afhs *)pProtocol;
    delete pClass;
  }
}

CMPUrlSourceSplitter_Protocol_Afhs::CMPUrlSourceSplitter_Protocol_Afhs(CParameterCollection *configuration)
{
  this->configurationParameters = new CParameterCollection();
  if (configuration != NULL)
  {
    this->configurationParameters->Append(configuration);
  }

  this->logger = new CLogger(this->configurationParameters);
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME);

  wchar_t *version = GetVersionInfo(VERSION_INFO_MPURLSOURCESPLITTER_PROTOCOL_AFHS, COMPILE_INFO_MPURLSOURCESPLITTER_PROTOCOL_AFHS);
  if (version != NULL)
  {
    this->logger->Log(LOGGER_INFO, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME, version);
  }
  FREE_MEM(version);

  version = CCurlInstance::GetCurlVersion();
  if (version != NULL)
  {
    this->logger->Log(LOGGER_INFO, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME, version);
  }
  FREE_MEM(version);
  
  this->receiveDataTimeout = AFHS_RECEIVE_DATA_TIMEOUT_DEFAULT;
  this->openConnetionMaximumAttempts = AFHS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS_DEFAULT;
  this->streamLength = 0;
  this->setLength = false;
  this->setEndOfStream = false;
  this->streamTime = 0;
  this->lockMutex = CreateMutex(NULL, FALSE, NULL);
  this->wholeStreamDownloaded = false;
  this->mainCurlInstance = NULL;
  this->bootstrapInfoCurlInstance = NULL;
  this->bytePosition = 0;
  this->seekingActive = false;
  this->supressData = false;
  this->bufferForProcessing = NULL;
  this->shouldExit = false;
  this->bootstrapInfoBox = NULL;
  this->segmentsFragments = NULL;
  this->live = false;
  this->lastBootstrapInfoRequestTime = 0;
  this->storeFilePath = NULL;
  this->lastStoreTime = 0;
  this->isConnected = false;
  this->segmentFragmentDownloading = UINT_MAX;
  this->segmentFragmentProcessing = 0;
  this->segmentFragmentToDownload = UINT_MAX;

  this->decryptionHoster = new CAfhsDecryptionHoster(this->logger, this->configurationParameters);
  if (this->decryptionHoster != NULL)
  {
    this->decryptionHoster->LoadPlugins();
  }

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME);
}

CMPUrlSourceSplitter_Protocol_Afhs::~CMPUrlSourceSplitter_Protocol_Afhs()
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_DESTRUCTOR_NAME);

  if (this->IsConnected())
  {
    this->StopReceivingData();
  }

  FREE_MEM_CLASS(this->mainCurlInstance);
  FREE_MEM_CLASS(this->bootstrapInfoCurlInstance);

  if (this->decryptionHoster != NULL)
  {
    this->decryptionHoster->RemoveAllPlugins();
    FREE_MEM_CLASS(this->decryptionHoster);
  }

  if (this->storeFilePath != NULL)
  {
    DeleteFile(this->storeFilePath);
  }

  FREE_MEM_CLASS(this->bufferForProcessing);
  FREE_MEM_CLASS(this->configurationParameters);

  if (this->lockMutex != NULL)
  {
    CloseHandle(this->lockMutex);
    this->lockMutex = NULL;
  }

  FREE_MEM_CLASS(this->bootstrapInfoBox);
  FREE_MEM_CLASS(this->segmentsFragments);
  FREE_MEM(this->storeFilePath);

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_DESTRUCTOR_NAME);

  FREE_MEM_CLASS(this->logger);
}

// IProtocol interface

bool CMPUrlSourceSplitter_Protocol_Afhs::IsConnected(void)
{
  return ((this->isConnected) || (this->wholeStreamDownloaded));
}

unsigned int CMPUrlSourceSplitter_Protocol_Afhs::GetOpenConnectionMaximumAttempts(void)
{
  return this->openConnetionMaximumAttempts;
}

HRESULT CMPUrlSourceSplitter_Protocol_Afhs::ParseUrl(const CParameterCollection *parameters)
{
  HRESULT result = S_OK;
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_URL_NAME);
  CHECK_POINTER_DEFAULT_HRESULT(result, parameters);

  this->ClearSession();
  if (SUCCEEDED(result))
  {
    this->configurationParameters->Clear();
    ALLOC_MEM_DEFINE_SET(protocolConfiguration, ProtocolPluginConfiguration, 1, 0);
    if (protocolConfiguration != NULL)
    {
      protocolConfiguration->configuration = (CParameterCollection *)parameters;
    }
    this->Initialize(protocolConfiguration);
    FREE_MEM(protocolConfiguration);
  }

  const wchar_t *url = this->configurationParameters->GetValue(PARAMETER_NAME_URL, true, NULL);
  if (SUCCEEDED(result))
  {
    result = (url == NULL) ? E_OUTOFMEMORY : S_OK;
  }

  if (SUCCEEDED(result))
  {
    ALLOC_MEM_DEFINE_SET(urlComponents, URL_COMPONENTS, 1, 0);
    if (urlComponents == NULL)
    {
      this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_URL_NAME, L"cannot allocate memory for 'url components'");
      result = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(result))
    {
      ZeroURL(urlComponents);
      urlComponents->dwStructSize = sizeof(URL_COMPONENTS);

      this->logger->Log(LOGGER_INFO, L"%s: %s: url: %s", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_URL_NAME, url);

      if (!InternetCrackUrl(url, 0, 0, urlComponents))
      {
        this->logger->Log(LOGGER_ERROR, L"%s: %s: InternetCrackUrl() error: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_URL_NAME, GetLastError());
        result = E_FAIL;
      }
    }

    if (SUCCEEDED(result))
    {
      int length = urlComponents->dwSchemeLength + 1;
      ALLOC_MEM_DEFINE_SET(protocol, wchar_t, length, 0);
      if (protocol == NULL) 
      {
        this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_URL_NAME, L"cannot allocate memory for 'protocol'");
        result = E_OUTOFMEMORY;
      }

      if (SUCCEEDED(result))
      {
        wcsncat_s(protocol, length, urlComponents->lpszScheme, urlComponents->dwSchemeLength);

        bool supportedProtocol = false;
        for (int i = 0; i < TOTAL_SUPPORTED_PROTOCOLS; i++)
        {
          if (_wcsnicmp(urlComponents->lpszScheme, SUPPORTED_PROTOCOLS[i], urlComponents->dwSchemeLength) == 0)
          {
            supportedProtocol = true;
            break;
          }
        }

        if (!supportedProtocol)
        {
          // not supported protocol
          this->logger->Log(LOGGER_INFO, L"%s: %s: unsupported protocol '%s'", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_URL_NAME, protocol);
          result = E_FAIL;
        }
      }
      FREE_MEM(protocol);
    }

    FREE_MEM(urlComponents);
  }

  this->logger->Log(LOGGER_INFO, SUCCEEDED(result) ? METHOD_END_FORMAT : METHOD_END_FAIL_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_URL_NAME);
  return result;
}

HRESULT CMPUrlSourceSplitter_Protocol_Afhs::ReceiveData(bool *shouldExit, CReceiveData *receiveData)
{
  HRESULT result = S_OK;
  this->logger->Log(LOGGER_DATA, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME);
  this->shouldExit = *shouldExit;

  CLockMutex lock(this->lockMutex, INFINITE);

  if (this->IsConnected())
  {
    if (!this->wholeStreamDownloaded)
    {
      if (SUCCEEDED(result) && (!this->shouldExit) && (this->segmentFragmentDownloading != UINT_MAX) && (this->mainCurlInstance != NULL))
      {
        unsigned int bytesRead = this->mainCurlInstance->GetReceiveDataBuffer()->GetBufferOccupiedSpace();
        if (bytesRead != 0)
        {
          CSegmentFragment *segmentFragment = this->segmentsFragments->GetItem(this->segmentFragmentDownloading);
          CLinearBuffer *linearBuffer = segmentFragment->GetBuffer();

          if (linearBuffer != NULL)
          {
            unsigned int bufferSize = linearBuffer->GetBufferSize();
            if (bufferSize == 0)
            {
              // initialize buffer
              linearBuffer->InitializeBuffer(MINIMUM_RECEIVED_DATA_FOR_SPLITTER);
              bufferSize = linearBuffer->GetBufferSize();
            }

            unsigned int freeSpace = linearBuffer->GetBufferFreeSpace();

            if (freeSpace < bytesRead)
            {
              unsigned int bufferNewSize = max(bufferSize * 2, bufferSize + bytesRead);
              this->logger->Log(LOGGER_VERBOSE, L"%s: %s: buffer to small, buffer size: %d, new size: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, bufferSize, bufferNewSize);
              if (!linearBuffer->ResizeBuffer(bufferNewSize))
              {
                this->logger->Log(LOGGER_WARNING, L"%s: %s: resizing buffer unsuccessful, dropping received data", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME);
                // error
                bytesRead = 0;
              }
            }

            if (bytesRead != 0)
            {
              ALLOC_MEM_DEFINE_SET(buffer, unsigned char, bytesRead, 0);
              if (buffer != NULL)
              {
                this->mainCurlInstance->GetReceiveDataBuffer()->CopyFromBuffer(buffer, bytesRead, 0, 0);
                linearBuffer->AddToBuffer(buffer, bytesRead);
                this->mainCurlInstance->GetReceiveDataBuffer()->RemoveFromBufferAndMove(bytesRead);
              }
              FREE_MEM(buffer);
            }
          }
        }
      }

      if (SUCCEEDED(result) && (!this->setLength) && (this->bytePosition != 0))
      {
        // adjust total length if not already set
        if (this->streamLength == 0)
        {
          // error occured or stream duration is not set
          // just make guess
          this->streamLength = LONGLONG(MINIMUM_RECEIVED_DATA_FOR_SPLITTER);
          this->logger->Log(LOGGER_VERBOSE, L"%s: %s: setting quess total length: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, this->streamLength);
          receiveData->GetTotalLength()->SetTotalLength(this->streamLength, true);
        }
        else if ((this->bytePosition > (this->streamLength * 3 / 4)))
        {
          // it is time to adjust stream length, we are approaching to end but still we don't know total length
          this->streamLength = this->bytePosition * 2;
          this->logger->Log(LOGGER_VERBOSE, L"%s: %s: adjusting quess total length: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, this->streamLength);
          receiveData->GetTotalLength()->SetTotalLength(this->streamLength, true);
        }
      }

      if (SUCCEEDED(result) && (!this->supressData) && (this->segmentFragmentProcessing < this->segmentsFragments->Count()))
      {
        CLinearBuffer *bufferForBoxProcessing = this->FillBufferForProcessing(this->segmentsFragments, this->segmentFragmentProcessing, this->storeFilePath);
        if (bufferForBoxProcessing != NULL)
        {
          // buffer successfully filled

          bool continueProcessing = false;
          do
          {
            continueProcessing = false;
            CBox *box = new CBox();
            if (box != NULL)
            {
              unsigned int length = bufferForBoxProcessing->GetBufferOccupiedSpace();
              if (length > 0)
              {
                ALLOC_MEM_DEFINE_SET(buffer, unsigned char, length, 0);
                if (buffer != NULL)
                {
                  bufferForBoxProcessing->CopyFromBuffer(buffer, length, 0, 0);
                  if (box->Parse(buffer, length))
                  {
                    unsigned int boxSize = (unsigned int)box->GetSize();
                    if (length >= boxSize)
                    {
                      continueProcessing = true;

                      if (wcscmp(box->GetType(), MEDIA_DATA_BOX_TYPE) == 0)
                      {
                        CMediaDataBox *mediaBox = new CMediaDataBox();
                        if (mediaBox != NULL)
                        {
                          continueProcessing &= mediaBox->Parse(buffer, length);

                          if (continueProcessing)
                          {
                            unsigned int payloadSize = (unsigned int)mediaBox->GetPayloadSize();
                            continueProcessing &= (this->bufferForProcessing->AddToBufferWithResize(mediaBox->GetPayload(), payloadSize) == payloadSize);
                          }
                        }
                        FREE_MEM_CLASS(mediaBox);
                      }

                      //if (wcscmp(box->GetType(), BOOTSTRAP_INFO_BOX_TYPE) == 0)
                      //{
                      //  CBootstrapInfoBox *bootstrapInfoBox = new CBootstrapInfoBox();
                      //  if (bootstrapInfoBox != NULL)
                      //  {
                      //    continueProcessing &= bootstrapInfoBox->Parse(buffer, length);

                      //    if (continueProcessing)
                      //    {
                      //      this->logger->Log(LOGGER_VERBOSE, L"%s: %s: bootstrap info box:\n%s", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, bootstrapInfoBox->GetParsedHumanReadable(L""));
                      //    }

                      //    // ignore errors while processing bootstrap info boxes
                      //    continueProcessing = true;
                      //  }
                      //  FREE_MEM_CLASS(bootstrapInfoBox);
                      //}

                      if (continueProcessing)
                      {
                        bufferForBoxProcessing->RemoveFromBuffer(boxSize);
                        continueProcessing = true;
                      }
                    }
                  }
                }
                FREE_MEM(buffer);
              }
            }
            FREE_MEM_CLASS(box);
          } while (continueProcessing);

          if (bufferForBoxProcessing->GetBufferOccupiedSpace() == 0)
          {
            // all data are processed
            continueProcessing = true;
          }

          if (continueProcessing)
          {
            this->segmentFragmentProcessing++;

            // check if segment and fragment is downloaded
            // if segment and fragment is not downloaded, then schedule it for download

            if (this->segmentFragmentProcessing < this->segmentsFragments->Count())
            {
              CSegmentFragment *segmentFragment = this->segmentsFragments->GetItem(this->segmentFragmentProcessing);
              if ((!segmentFragment->IsDownloaded()) && (this->segmentFragmentProcessing != this->segmentFragmentDownloading))
              {
                // segment and fragment is not downloaded and also is not downloading currently
                this->segmentFragmentToDownload = this->segmentFragmentProcessing;
              }
            }
          }
        }
        FREE_MEM_CLASS(bufferForBoxProcessing);
      }

      if (SUCCEEDED(result) && (!this->supressData) && (this->bufferForProcessing != NULL))
      {
        CFlvPacket *flvPacket = new CFlvPacket();
        if (flvPacket != NULL)
        {
          while (SUCCEEDED(result) && (flvPacket->ParsePacket(this->bufferForProcessing)))
          {
            // FLV packet parsed correctly
            // push FLV packet to filter

            //if ((flvPacket->GetType() != FLV_PACKET_HEADER) && (this->firstTimestamp == (-1)))
            //{
            //  this->firstTimestamp = flvPacket->GetTimestamp();
            //  this->logger->Log(LOGGER_VERBOSE, L"%s: %s: set first timestamp: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, this->firstTimestamp);
            //}

            //if ((flvPacket->GetType() == FLV_PACKET_VIDEO) && (this->firstVideoTimestamp == (-1)))
            //{
            //  this->firstVideoTimestamp = flvPacket->GetTimestamp();
            //  this->logger->Log(LOGGER_VERBOSE, L"%s: %s: set first video timestamp: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, this->firstVideoTimestamp);
            //}

            //if ((flvPacket->GetType() == FLV_PACKET_VIDEO) && (this->firstVideoTimestamp != (-1)) && (this->firstTimestamp != (-1)))
            //{
            //  // correction of video timestamps
            //  flvPacket->SetTimestamp(flvPacket->GetTimestamp() + this->firstTimestamp - this->firstVideoTimestamp);
            //}

            if ((flvPacket->GetType() == FLV_PACKET_AUDIO) ||
              (flvPacket->GetType() == FLV_PACKET_HEADER) ||
              (flvPacket->GetType() == FLV_PACKET_META) ||
              (flvPacket->GetType() == FLV_PACKET_VIDEO))
            {
              // do nothing, known packet types
              CHECK_CONDITION_HRESULT(result, !flvPacket->IsEncrypted(), result, E_DRM_PROTECTED);
            }
            else
            {
              this->logger->Log(LOGGER_WARNING, L"%s: %s: unknown FLV packet: %d, size: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, flvPacket->GetType(), flvPacket->GetSize());
              result = E_UNKNOWN_STREAM_TYPE;
            }

            if ((flvPacket->GetType() != FLV_PACKET_HEADER) || (!this->seekingActive))
            {
              // set or adjust total length (if needed)
              int64_t newBytePosition = this->bytePosition + flvPacket->GetSize();

              if ((!this->setLength) && (newBytePosition != 0))
              {
                // adjust total length if not already set
                if (this->streamLength == 0)
                {
                  // error occured or stream duration is not set
                  // just make guess
                  this->streamLength = LONGLONG(MINIMUM_RECEIVED_DATA_FOR_SPLITTER);
                  this->logger->Log(LOGGER_VERBOSE, L"%s: %s: setting quess total length: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, this->streamLength);
                  receiveData->GetTotalLength()->SetTotalLength(this->streamLength, true);
                }
                else if ((newBytePosition > (this->streamLength * 3 / 4)))
                {
                  // it is time to adjust stream length, we are approaching to end but still we don't know total length
                  this->streamLength = newBytePosition * 2;
                  this->logger->Log(LOGGER_VERBOSE, L"%s: %s: adjusting quess total length: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, this->streamLength);
                  receiveData->GetTotalLength()->SetTotalLength(this->streamLength, true);
                }
              }

              // create media packet
              // set values of media packet
              CMediaPacket *mediaPacket = new CMediaPacket();
              mediaPacket->GetBuffer()->InitializeBuffer(flvPacket->GetSize());
              mediaPacket->GetBuffer()->AddToBuffer(flvPacket->GetData(), flvPacket->GetSize());
              mediaPacket->SetStart(this->bytePosition);
              mediaPacket->SetEnd(this->bytePosition + flvPacket->GetSize() - 1);

              if (!receiveData->GetMediaPacketCollection()->Add(mediaPacket))
              {
                FREE_MEM_CLASS(mediaPacket);
              }
              this->bytePosition += flvPacket->GetSize();
            }
            // we are definitely not seeking
            this->seekingActive = false;
            this->bufferForProcessing->RemoveFromBufferAndMove(flvPacket->GetSize());

            flvPacket->Clear();
          }

          FREE_MEM_CLASS(flvPacket);
        }
      }

      if (SUCCEEDED(result) && (this->segmentFragmentProcessing >= this->segmentsFragments->Count()) &&
        (this->segmentFragmentToDownload == UINT_MAX) &&
        (this->segmentFragmentDownloading == UINT_MAX) &&
        (!this->live))
      {
        // all segments and fragments downloaded and processed
        // whole stream downloaded
        this->wholeStreamDownloaded = true;
        this->isConnected = false;
        FREE_MEM_CLASS(this->mainCurlInstance);
      }

      if (SUCCEEDED(result) && (this->segmentFragmentProcessing >= this->segmentsFragments->Count()) && (!this->live))
      {
        // all segments and fragments processed
        // set stream length and report end of stream
        if (!this->seekingActive)
        {
          // we are not seeking, so we can set total length
          if (!this->setLength)
          {
            this->streamLength = this->bytePosition;
            this->logger->Log(LOGGER_VERBOSE, L"%s: %s: setting total length: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, this->streamLength);
            receiveData->GetTotalLength()->SetTotalLength(this->streamLength, false);
            this->setLength = true;
          }

          if (!this->setEndOfStream)
          {
            // notify filter the we reached end of stream
            receiveData->GetEndOfStreamReached()->SetStreamPosition(max(0, this->bytePosition - 1));
            this->setEndOfStream = true;
          }
        }
      }

      if (SUCCEEDED(result) && (this->mainCurlInstance != NULL) && (this->mainCurlInstance->GetCurlState() == CURL_STATE_RECEIVED_ALL_DATA))
      {
        if (this->mainCurlInstance->GetErrorCode() == CURLE_OK)
        {
          if (this->mainCurlInstance->GetReceiveDataBuffer()->GetBufferOccupiedSpace() == 0)
          {
            // in CURL instance aren't received data, all data are stored in segment and fragment
            // all data received, we're not receiving data
            CSegmentFragment *segmentFragment = this->segmentsFragments->GetItem(this->segmentFragmentDownloading);
            segmentFragment->SetDownloaded(true);
            this->logger->Log(LOGGER_VERBOSE, L"%s: %s: received all data for url '%s'", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, this->mainCurlInstance->GetUrl());

            // if it is live session, then download first not downloaded segment and fragment after currently processed segment and fragment
            // can return UINT_MAX if segment and fragment for download doesn't exist (in that case wait for update of bootstrap info)
            this->segmentFragmentToDownload = (this->live) ? this->segmentsFragments->GetFirstNotDownloadedSegmentFragment(this->segmentFragmentProcessing) : this->segmentFragmentToDownload;
            // if not set segment and fragment to download, then set segment and fragment to download (get next not downloaded segment and fragment after current downloaded segment and fragment)
            this->segmentFragmentToDownload = ((!this->live) && (this->segmentFragmentToDownload == UINT_MAX)) ? this->segmentsFragments->GetFirstNotDownloadedSegmentFragment(this->segmentFragmentDownloading) : this->segmentFragmentToDownload;
            // if not set segment and fragment to download, then set segment and fragment to download (get next not downloaded segment and fragment from first segment and fragment)
            this->segmentFragmentToDownload = ((!this->live) && (this->segmentFragmentToDownload == UINT_MAX)) ? this->segmentsFragments->GetFirstNotDownloadedSegmentFragment(0) : this->segmentFragmentToDownload;

            // segment and fragment to download still can be UINT_MAX = no segment and fragment to download
            this->segmentFragmentDownloading = UINT_MAX;
            FREE_MEM_CLASS(this->mainCurlInstance);
          }
        }
        else
        {
          // error occured while downloading
          // download segment and fragment again or download scheduled segment and fragment
          this->segmentFragmentToDownload = (this->segmentFragmentToDownload != UINT_MAX) ? this->segmentFragmentDownloading : this->segmentFragmentToDownload;
          this->segmentFragmentDownloading = UINT_MAX;
          FREE_MEM_CLASS(this->mainCurlInstance);
        }
      }

      if (SUCCEEDED(result) && (!this->shouldExit) && (this->segmentsFragments->GetFirstNotProcessedSegmentFragment(0) != UINT_MAX))
      {
        result = this->decryptionHoster->ProcessSegmentsAndFragments(this->segmentsFragments);
      }

      if (SUCCEEDED(result) && (this->mainCurlInstance == NULL) && (this->segmentsFragments->GetFirstNotProcessedSegmentFragment(0) == UINT_MAX))
      {
        // do not start any download until all downloaded segments and fragments processed
        // no CURL instance exists, we finished download
        // start another download

        // if it is live session, then download first not downloaded segment and fragment after currently processed segment and fragment
        // can return UINT_MAX if segment and fragment for download doesn't exist (in that case wait for update of bootstrap info)
        this->segmentFragmentToDownload = (this->live) ? this->segmentsFragments->GetFirstNotDownloadedSegmentFragment(this->segmentFragmentProcessing) : this->segmentFragmentToDownload;
        // if not set segment and fragment to download, then set segment and fragment to download (get next not downloaded segment and fragment after current processed segment and fragment)
        this->segmentFragmentToDownload = ((!this->live) && (this->segmentFragmentToDownload == UINT_MAX)) ? this->segmentsFragments->GetFirstNotDownloadedSegmentFragment(this->segmentFragmentProcessing) : this->segmentFragmentToDownload;
        // if not set segment and fragment to download, then set segment and fragment to download (get next not downloaded segment and fragment from first segment and fragment)
        this->segmentFragmentToDownload = ((!this->live) && (this->segmentFragmentToDownload == UINT_MAX)) ? this->segmentsFragments->GetFirstNotDownloadedSegmentFragment(0) : this->segmentFragmentToDownload;
        // segment and fragment to download still can be UINT_MAX = no segment and fragment to download

        if (SUCCEEDED(result) && (this->segmentFragmentToDownload != UINT_MAX))
        {
          // there is specified segment and fragment to download

          CSegmentFragment *segmentFragment = this->segmentsFragments->GetItem(this->segmentFragmentToDownload);
          if (segmentFragment != NULL)
          {
            HRESULT result = S_OK;

            if (SUCCEEDED(result))
            {
              // we need to download for another url
              this->mainCurlInstance = new CHttpCurlInstance(this->logger, this->lockMutex, segmentFragment->GetUrl(), PROTOCOL_IMPLEMENTATION_NAME, L"Main");
              CHECK_POINTER_HRESULT(result, this->mainCurlInstance, result, E_POINTER);

              if (SUCCEEDED(result))
              {
                this->mainCurlInstance->SetReceivedDataTimeout(this->receiveDataTimeout);
                this->mainCurlInstance->SetReferer(this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_REFERER, true, NULL));
                this->mainCurlInstance->SetUserAgent(this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_USER_AGENT, true, NULL));
                this->mainCurlInstance->SetCookie(this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_COOKIE, true, NULL));
                this->mainCurlInstance->SetHttpVersion(this->configurationParameters->GetValueLong(PARAMETER_NAME_AFHS_VERSION, true, HTTP_VERSION_DEFAULT));
                this->mainCurlInstance->SetIgnoreContentLength((this->configurationParameters->GetValueLong(PARAMETER_NAME_AFHS_IGNORE_CONTENT_LENGTH, true, HTTP_IGNORE_CONTENT_LENGTH_DEFAULT) == 1L));

                result = (this->mainCurlInstance->Initialize()) ? S_OK : E_FAIL;

                if (SUCCEEDED(result))
                {
                  // all parameters set
                  // start receiving data

                  result = (this->mainCurlInstance->StartReceivingData()) ? S_OK : E_FAIL;

                  if (SUCCEEDED(result))
                  {
                    this->segmentFragmentDownloading = this->segmentFragmentToDownload;
                    this->segmentFragmentToDownload = UINT_MAX;
                  }
                }
              }
            }
          }
        }
      }

      if (SUCCEEDED(result) && (this->live))
      {
        // in case of live stream we need to download again manifest and parse bootstrap info for new information about stream

        if ((this->bootstrapInfoCurlInstance != NULL) && (this->bootstrapInfoCurlInstance->GetCurlState() == CURL_STATE_RECEIVED_ALL_DATA))
        {
          if (this->bootstrapInfoCurlInstance->GetErrorCode() == CURLE_OK)
          {
            unsigned int length = this->bootstrapInfoCurlInstance->GetReceiveDataBuffer()->GetBufferOccupiedSpace();
            bool continueWithBootstrapInfo = (length > 1);

            if (continueWithBootstrapInfo)
            {
              ALLOC_MEM_DEFINE_SET(buffer, unsigned char, length, 0);
              continueWithBootstrapInfo &= (buffer != NULL);

              if (continueWithBootstrapInfo)
              {
                this->bootstrapInfoCurlInstance->GetReceiveDataBuffer()->CopyFromBuffer(buffer, length, 0, 0);

                CBootstrapInfoBox *bootstrapInfoBox = new CBootstrapInfoBox();
                continueWithBootstrapInfo &= (bootstrapInfoBox != NULL);

                if (continueWithBootstrapInfo)
                {
                  if (bootstrapInfoBox->Parse(buffer, length))
                  {
                    //this->logger->Log(LOGGER_VERBOSE, L"%s: %s: new bootstrap info box:\n%s", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, bootstrapInfoBox->GetParsedHumanReadable(L""));

                    CSegmentFragmentCollection *updateSegmentsFragments = this->GetSegmentsFragmentsFromBootstrapInfoBox(
                      this->logger,
                      METHOD_RECEIVE_DATA_NAME,
                      this->configurationParameters,
                      bootstrapInfoBox,
                      false);
                    continueWithBootstrapInfo &= (updateSegmentsFragments != NULL);

                    if (continueWithBootstrapInfo)
                    {
                      CSegmentFragment *lastSegmentFragment = this->segmentsFragments->GetItem(this->segmentsFragments->Count() - 1);

                      for (unsigned int i = 0; i < updateSegmentsFragments->Count(); i++)
                      {
                        CSegmentFragment *parsedSegmentFragment = updateSegmentsFragments->GetItem(i);
                        if (parsedSegmentFragment->GetFragment() > lastSegmentFragment->GetFragment())
                        {
                          // new segment fragment, add it to be downloaded
                          CSegmentFragment *clone = new CSegmentFragment(
                            parsedSegmentFragment->GetSegment(),
                            parsedSegmentFragment->GetFragment(),
                            parsedSegmentFragment->GetUrl(),
                            parsedSegmentFragment->GetFragmentTimestamp());
                          continueWithBootstrapInfo &= (clone != NULL);

                          if (continueWithBootstrapInfo)
                          {
                            continueWithBootstrapInfo &= this->segmentsFragments->Add(clone);
                            if (continueWithBootstrapInfo)
                            {
                              this->logger->Log(LOGGER_VERBOSE, L"%s: %s: added new segment and fragment, segment %d, fragment %d, url '%s', timestamp: %lld", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, clone->GetSegment(), clone->GetFragment(), clone->GetUrl(), clone->GetFragmentTimestamp());
                            }
                          }

                          if (!continueWithBootstrapInfo)
                          {
                            FREE_MEM_CLASS(clone);
                          }
                        }
                      }
                    }
                    else
                    {
                      this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"cannot create segments and fragments to download");
                    }

                    FREE_MEM_CLASS(updateSegmentsFragments);
                  }
                  else
                  {
                    this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"cannot parse new bootstrap info box");
                  }
                }
                else
                {
                  this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"not enough memory for new bootstrap info box");
                }

                FREE_MEM_CLASS(bootstrapInfoBox);
              }
              else
              {
                this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"not enough memory for new bootstrap info data");
              }

              FREE_MEM(buffer);
            }
            else
            {
              this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"too short data downloaded for new bootstrap info");
            }

            if (continueWithBootstrapInfo)
            {
              // everything correct, download next bootstrap info after LAST_REQUEST_BOOTSTRAP_INFO_DELAY seconds
              this->lastBootstrapInfoRequestTime = GetTickCount();
            }
          }
          else
          {
            // error occured while downloading bootstrap info
            // download bootstrap info again
            this->lastBootstrapInfoRequestTime = 0;
          }

          FREE_MEM_CLASS(this->bootstrapInfoCurlInstance);
        }

        if ((this->bootstrapInfoCurlInstance == NULL) && (GetTickCount() > (this->lastBootstrapInfoRequestTime + LAST_REQUEST_BOOTSTRAP_INFO_DELAY)))
        {
          const wchar_t *url = this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_BOOTSTRAP_INFO_URL, true, NULL);
          if (url != NULL)
          {
            this->logger->Log(LOGGER_VERBOSE, L"%s: %s: live streaming, requesting bootstrap info, url: '%s'", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, url);

            this->bootstrapInfoCurlInstance = new CHttpCurlInstance(this->logger, NULL, url, PROTOCOL_IMPLEMENTATION_NAME, L"BootstrapInfo");
            this->bootstrapInfoCurlInstance->SetReceivedDataTimeout(this->receiveDataTimeout);
            this->bootstrapInfoCurlInstance->SetReferer(this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_REFERER, true, NULL));
            this->bootstrapInfoCurlInstance->SetUserAgent(this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_USER_AGENT, true, NULL));
            this->bootstrapInfoCurlInstance->SetCookie(this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_COOKIE, true, NULL));
            this->bootstrapInfoCurlInstance->SetHttpVersion(this->configurationParameters->GetValueLong(PARAMETER_NAME_AFHS_VERSION, true, HTTP_VERSION_DEFAULT));
            this->bootstrapInfoCurlInstance->SetIgnoreContentLength((this->configurationParameters->GetValueLong(PARAMETER_NAME_AFHS_IGNORE_CONTENT_LENGTH, true, HTTP_IGNORE_CONTENT_LENGTH_DEFAULT) == 1L));

            bool continueWithBootstrapInfo = this->bootstrapInfoCurlInstance->Initialize();
            if (continueWithBootstrapInfo)
            {
              continueWithBootstrapInfo &= this->bootstrapInfoCurlInstance->StartReceivingData();
              if (!continueWithBootstrapInfo)
              {
                this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"cannot start receiving data of new bootstrap info");
              }
            }
            else
            {
              this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"cannot initialize new bootstrap info download");
            }

            if (!continueWithBootstrapInfo)
            {
              FREE_MEM_CLASS(this->bootstrapInfoCurlInstance);
            }
          }
        }
      }
    }
    else
    {
      // set total length (if not set earlier)
      if (!this->setLength)
      {
        this->streamLength = this->bytePosition;
        this->logger->Log(LOGGER_VERBOSE, L"%s: %s: setting total length: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, this->streamLength);
        receiveData->GetTotalLength()->SetTotalLength(this->streamLength, false);
        this->setLength = true;
      }
    }
  }
  else
  {
    this->logger->Log(LOGGER_WARNING, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"connection closed, opening new one");
    // re-open connection if previous is lost
    if (this->StartReceivingData(NULL) != S_OK)
    {
      this->StopReceivingData();
    }
  }

  // store segments and fragments to temporary file
  if (SUCCEEDED(result) && ((GetTickCount() - this->lastStoreTime) > 1000))
  {
    this->lastStoreTime = GetTickCount();

    if (this->segmentsFragments->Count() > 0)
    {
      // store all segments and fragments (which are not stored) to file
      if (this->storeFilePath == NULL)
      {
        this->storeFilePath = this->GetStoreFile();
      }

      if (this->storeFilePath != NULL)
      {
        LARGE_INTEGER size;
        size.QuadPart = 0;

        // open or create file
        HANDLE hTempFile = CreateFile(this->storeFilePath, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hTempFile != INVALID_HANDLE_VALUE)
        {
          if (!GetFileSizeEx(hTempFile, &size))
          {
            this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"error while getting size");
            // error occured while getting file size
            size.QuadPart = -1;
          }

          if (size.QuadPart >= 0)
          {
            unsigned int i = 0;
            while (i < this->segmentsFragments->Count())
            {
              CSegmentFragment *segmentFragment = this->segmentsFragments->GetItem(i);

              if ((!segmentFragment->IsStoredToFile()) && (segmentFragment->IsProcessed()))
              {
                // if segment and fragment is not stored to file
                // store it to file
                unsigned int length = segmentFragment->GetLength();

                ALLOC_MEM_DEFINE_SET(buffer, unsigned char, length, 0);
                if (segmentFragment->GetBuffer()->CopyFromBuffer(buffer, length, 0, 0) == length)
                {
                  DWORD written = 0;
                  if (WriteFile(hTempFile, buffer, length, &written, NULL))
                  {
                    if (length == written)
                    {
                      // mark as stored
                      segmentFragment->SetStoredToFile(size.QuadPart);
                      size.QuadPart += length;
                    }
                  }
                }
                FREE_MEM(buffer);
              }

              i++;
            }
          }

          CloseHandle(hTempFile);
          hTempFile = INVALID_HANDLE_VALUE;
        }
      }
    }
  }

  this->logger->Log(LOGGER_DATA, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME);
  return result;
}

// ISimpleProtocol interface

unsigned int CMPUrlSourceSplitter_Protocol_Afhs::GetReceiveDataTimeout(void)
{
  return this->receiveDataTimeout;
}

HRESULT CMPUrlSourceSplitter_Protocol_Afhs::StartReceivingData(const CParameterCollection *parameters)
{
  HRESULT result = S_OK;
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME);

  CHECK_POINTER_DEFAULT_HRESULT(result, this->configurationParameters);

  // lock access to stream
  CLockMutex lock(this->lockMutex, INFINITE);

  this->wholeStreamDownloaded = false;
  //this->firstTimestamp = -1;
  //this->firstVideoTimestamp = -1;
  this->bytePosition = 0;
  this->streamLength = 0;
  this->setLength = false;
  this->setEndOfStream = false;

  if (this->segmentsFragments == NULL)
  {
    char *bootstrapInfoBase64Encoded = ConvertToMultiByteW(this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_BOOTSTRAP_INFO, true, NULL));
    CHECK_POINTER_HRESULT(result, bootstrapInfoBase64Encoded, result, E_POINTER);

    if (SUCCEEDED(result))
    {
      // bootstrap info is BASE64 encoded
      unsigned char *bootstrapInfo = NULL;
      unsigned int bootstrapInfoLength = 0;

      result = base64_decode(bootstrapInfoBase64Encoded, &bootstrapInfo, &bootstrapInfoLength);

      if (SUCCEEDED(result))
      {
        FREE_MEM_CLASS(this->bootstrapInfoBox);
        this->bootstrapInfoBox = new CBootstrapInfoBox();
        CHECK_POINTER_HRESULT(result, this->bootstrapInfoBox, result, E_OUTOFMEMORY);

        if (SUCCEEDED(result))
        {
          result = (this->bootstrapInfoBox->Parse(bootstrapInfo, bootstrapInfoLength)) ? result : HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

          /*if (SUCCEEDED(result))
          {
            wchar_t *parsedBootstrapInfoBox = this->bootstrapInfoBox->GetParsedHumanReadable(L"");
            this->logger->Log(LOGGER_VERBOSE, L"%s: %s: parsed bootstrap info:\n%s", PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME, parsedBootstrapInfoBox);
            FREE_MEM(parsedBootstrapInfoBox);
          }*/

          if (FAILED(result))
          {
            this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME, L"cannot parse bootstrap info box");
          }
        }
      }
      else
      {
        this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME, L"cannot decode bootstrap info");
      }
    }

    if (SUCCEEDED(result))
    {
      // we have bootstrap info box successfully parsed
      this->live = this->bootstrapInfoBox->IsLive();

      FREE_MEM(this->segmentsFragments);
      this->segmentsFragments = this->GetSegmentsFragmentsFromBootstrapInfoBox(
        this->logger,
        METHOD_START_RECEIVING_DATA_NAME,
        this->configurationParameters,
        this->bootstrapInfoBox,
        false);
      CHECK_POINTER_HRESULT(result, this->segmentsFragments, result, E_POINTER);

      if (this->live)
      {
        // in case of live stream check current media time and choose right segment and fragment

        uint64_t currentMediaTime = (this->bootstrapInfoBox->GetCurrentMediaTime() > 0) ? (this->bootstrapInfoBox->GetCurrentMediaTime() - 1): 0;
        // this download one fragment before current media time

        // find segment and fragment to process
        result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        if (this->segmentsFragments != NULL)
        {
          for (unsigned int i = 0; i < this->segmentsFragments->Count(); i++)
          {
            CSegmentFragment *segFrag = this->segmentsFragments->GetItem(i);

            if (segFrag->GetFragmentTimestamp() <= (uint64_t)time)
            {
              this->segmentFragmentProcessing = i;
              result = S_OK;
            }
          }
        }
      }
    }
  }

  if (SUCCEEDED(result))
  {
    this->bufferForProcessing = new CLinearBuffer();
    CHECK_POINTER_HRESULT(result, this->bufferForProcessing, result, E_OUTOFMEMORY);

    if (SUCCEEDED(result))
    {
      result = (this->bufferForProcessing->InitializeBuffer(MINIMUM_RECEIVED_DATA_FOR_SPLITTER)) ? result : E_FAIL;

      if ((SUCCEEDED(result)) && (!this->seekingActive))
      {
        this->bufferForProcessing->AddToBuffer(FLV_FILE_HEADER, FLV_FILE_HEADER_LENGTH);

        char *mediaMetadataBase64Encoded = ConvertToMultiByteW(this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_MEDIA_METADATA, true, NULL));
        if (mediaMetadataBase64Encoded != NULL)
        {
          // metadata can be in connection parameters, but it is optional
          // metadata is BASE64 encoded
          unsigned char *metadata = NULL;
          unsigned int metadataLength = 0;
          result = base64_decode(mediaMetadataBase64Encoded, &metadata, &metadataLength);

          if (SUCCEEDED(result))
          {
            // create FLV packet from metadata and add its content to buffer for processing
            CFlvPacket *metadataFlvPacket = new CFlvPacket();
            CHECK_POINTER_HRESULT(result, metadataFlvPacket, result, E_OUTOFMEMORY);

            if (SUCCEEDED(result))
            {
              result = metadataFlvPacket->CreatePacket(FLV_PACKET_META, metadata, metadataLength, (unsigned int)this->segmentsFragments->GetItem(0)->GetFragmentTimestamp(), false) ? result : E_FAIL;

              if (SUCCEEDED(result))
              {
                result = (this->bufferForProcessing->AddToBufferWithResize(metadataFlvPacket->GetData(), metadataFlvPacket->GetSize()) == metadataFlvPacket->GetSize()) ? result : E_FAIL;

                if (FAILED(result))
                {
                  this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"cannot add FLV metadata packet to buffer");
                }
              }
              else
              {
                this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"cannot create FLV metadata packet");
              }
            }
            else
            {
              this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"not enough memory for FLV metadata packet");
            }
            FREE_MEM_CLASS(metadataFlvPacket);
          }
          else
          {
            this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"cannot decode metadata");
          }
          FREE_MEM(metadata);
        }
        FREE_MEM(mediaMetadataBase64Encoded);
      }
    }
  }

  if (FAILED(result))
  {
    this->StopReceivingData();
  }

  this->isConnected = SUCCEEDED(result);

  this->logger->Log(LOGGER_INFO, SUCCEEDED(result) ? METHOD_END_FORMAT : METHOD_END_FAIL_HRESULT_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME, result);
  return result;
}

HRESULT CMPUrlSourceSplitter_Protocol_Afhs::StopReceivingData(void)
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_STOP_RECEIVING_DATA_NAME);

  // lock access to stream
  CLockMutex lock(this->lockMutex, INFINITE);

  if (this->mainCurlInstance != NULL)
  {
    this->mainCurlInstance->SetCloseWithoutWaiting(this->seekingActive);
    FREE_MEM_CLASS(this->mainCurlInstance);
  }

  if (this->bootstrapInfoCurlInstance != NULL)
  {
    this->bootstrapInfoCurlInstance->SetCloseWithoutWaiting(this->seekingActive);
    FREE_MEM_CLASS(this->bootstrapInfoCurlInstance);
  }

  FREE_MEM_CLASS(this->bufferForProcessing);
  this->isConnected = false;
  this->segmentFragmentDownloading = UINT_MAX;

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_STOP_RECEIVING_DATA_NAME);
  return S_OK;
}

HRESULT CMPUrlSourceSplitter_Protocol_Afhs::QueryStreamProgress(LONGLONG *total, LONGLONG *current)
{
  this->logger->Log(LOGGER_DATA, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_QUERY_STREAM_PROGRESS_NAME);

  HRESULT result = S_OK;
  CHECK_POINTER_DEFAULT_HRESULT(result, total);
  CHECK_POINTER_DEFAULT_HRESULT(result, current);

  if (SUCCEEDED(result))
  {
    *total = (this->streamLength == 0) ? 1 : this->streamLength;
    *current = (this->streamLength == 0) ? 0 : this->bytePosition;

    if (!this->setLength)
    {
      result = VFW_S_ESTIMATED;
    }
  }

  this->logger->Log(LOGGER_DATA, (SUCCEEDED(result)) ? METHOD_END_HRESULT_FORMAT : METHOD_END_FAIL_HRESULT_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_QUERY_STREAM_PROGRESS_NAME, result);
  return result;
}
  
HRESULT CMPUrlSourceSplitter_Protocol_Afhs::QueryStreamAvailableLength(CStreamAvailableLength *availableLength)
{
  HRESULT result = S_OK;
  CHECK_POINTER_DEFAULT_HRESULT(result, availableLength);

  if (SUCCEEDED(result))
  {
    availableLength->SetQueryResult(S_OK);
    if (!this->setLength)
    {
      availableLength->SetAvailableLength(this->bytePosition);
    }
    else
    {
      availableLength->SetAvailableLength(this->streamLength);
    }
  }

  return result;
}

HRESULT CMPUrlSourceSplitter_Protocol_Afhs::ClearSession(void)
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CLEAR_SESSION_NAME);

  if (this->IsConnected())
  {
    this->StopReceivingData();
  }

  // clear all decryption plugins session
  this->decryptionHoster->ClearSession();

  if (this->storeFilePath != NULL)
  {
    DeleteFile(this->storeFilePath);
  }

  FREE_MEM_CLASS(this->bufferForProcessing);
 
  this->streamLength = 0;
  this->setLength = false;
  this->setEndOfStream = false;
  this->streamTime = 0;
  this->wholeStreamDownloaded = false;
  this->receiveDataTimeout = AFHS_RECEIVE_DATA_TIMEOUT_DEFAULT;
  this->openConnetionMaximumAttempts = AFHS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS_DEFAULT;
  this->bytePosition = 0;
  this->shouldExit = false;
  FREE_MEM_CLASS(this->bootstrapInfoBox);
  FREE_MEM_CLASS(this->segmentsFragments);
  this->live = false;
  this->lastBootstrapInfoRequestTime = 0;
  FREE_MEM(this->storeFilePath);
  this->isConnected = false;
  this->segmentFragmentDownloading = UINT_MAX;
  this->segmentFragmentProcessing = 0;
  this->segmentFragmentToDownload = UINT_MAX;

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CLEAR_SESSION_NAME);
  return S_OK;
}

// ISeeking interface

unsigned int CMPUrlSourceSplitter_Protocol_Afhs::GetSeekingCapabilities(void)
{
  return SEEKING_METHOD_TIME;
}

int64_t CMPUrlSourceSplitter_Protocol_Afhs::SeekToTime(int64_t time)
{
  CLockMutex lock(this->lockMutex, INFINITE);

  this->logger->Log(LOGGER_VERBOSE, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_TIME_NAME);
  this->logger->Log(LOGGER_VERBOSE, L"%s: %s: from time: %llu", PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_TIME_NAME, time);

  int64_t result = -1;

  // AFHS protocol can seek to ms
  // time is in ms

  // find segment and fragment to process
  if (this->segmentsFragments != NULL)
  {
    for (unsigned int i = 0; i < this->segmentsFragments->Count(); i++)
    {
      CSegmentFragment *segFrag = this->segmentsFragments->GetItem(i);

      if (segFrag->GetFragmentTimestamp() <= (uint64_t)time)
      {
        this->segmentFragmentProcessing = i;
        result = segFrag->GetFragmentTimestamp();
      }
    }
  }

  this->seekingActive = true;
  this->bytePosition = 0;
  this->streamLength = 0;
  this->setLength = false;
  this->setEndOfStream = false;
  // reset whole stream downloaded, but IsConnected() must return true to avoid calling StartReceivingData()
  this->isConnected = true;
  this->wholeStreamDownloaded = false;

  // in this->segmentFragmentProcessing is id of segment and fragment to process
  CSegmentFragment *segFrag = this->segmentsFragments->GetItem(this->segmentFragmentProcessing);

  if (segFrag != NULL)
  {
    this->logger->Log(LOGGER_VERBOSE, L"%s: %s: segment %d, fragment %d, url '%s', timestamp %lld", PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_TIME_NAME,
      segFrag->GetSegment(), segFrag->GetFragment(), segFrag->GetUrl(), segFrag->GetFragmentTimestamp());

    if (!segFrag->IsDownloaded())
    {
      // close connection
      this->StopReceivingData();

      // clear segment and fragment to download
      this->segmentFragmentToDownload = UINT_MAX;

      // reopen connection
      // StartReceivingData() reset wholeStreamDownloaded
      this->isConnected = SUCCEEDED(this->StartReceivingData(NULL));
    }
  }
  else
  {
    this->isConnected = false;
  }

  if (!this->IsConnected())
  {
    result = -1;
  }
  else
  {
    this->streamTime = result;
  }

  this->logger->Log(LOGGER_VERBOSE, METHOD_END_INT64_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_TIME_NAME, result);
  return result;
}

int64_t CMPUrlSourceSplitter_Protocol_Afhs::SeekToPosition(int64_t start, int64_t end)
{
  this->logger->Log(LOGGER_VERBOSE, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_POSITION_NAME);
  this->logger->Log(LOGGER_VERBOSE, L"%s: %s: from position: %llu, to position: %llu", PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_POSITION_NAME, start, end);

  int64_t result = -1;

  this->logger->Log(LOGGER_VERBOSE, METHOD_END_INT64_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_POSITION_NAME, result);
  return result;
}

void CMPUrlSourceSplitter_Protocol_Afhs::SetSupressData(bool supressData)
{
  this->supressData = supressData;
}

// IPlugin interface

const wchar_t *CMPUrlSourceSplitter_Protocol_Afhs::GetName(void)
{
  return PROTOCOL_NAME;
}

GUID CMPUrlSourceSplitter_Protocol_Afhs::GetInstanceId(void)
{
  return this->logger->loggerInstance;
}

HRESULT CMPUrlSourceSplitter_Protocol_Afhs::Initialize(PluginConfiguration *configuration)
{
  if (configuration == NULL)
  {
    return E_POINTER;
  }

  ProtocolPluginConfiguration *protocolConfiguration = (ProtocolPluginConfiguration *)configuration;
  this->logger->SetParameters(protocolConfiguration->configuration);

  if (this->lockMutex == NULL)
  {
    return E_FAIL;
  }

  this->configurationParameters->Clear();
  if (protocolConfiguration->configuration != NULL)
  {
    this->configurationParameters->Append(protocolConfiguration->configuration);
  }
  this->configurationParameters->LogCollection(this->logger, LOGGER_VERBOSE, PROTOCOL_IMPLEMENTATION_NAME, METHOD_INITIALIZE_NAME);

  this->receiveDataTimeout = this->configurationParameters->GetValueLong(PARAMETER_NAME_AFHS_RECEIVE_DATA_TIMEOUT, true, AFHS_RECEIVE_DATA_TIMEOUT_DEFAULT);
  this->openConnetionMaximumAttempts = this->configurationParameters->GetValueLong(PARAMETER_NAME_AFHS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS, true, AFHS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS_DEFAULT);

  this->receiveDataTimeout = (this->receiveDataTimeout < 0) ? AFHS_RECEIVE_DATA_TIMEOUT_DEFAULT : this->receiveDataTimeout;
  this->openConnetionMaximumAttempts = (this->openConnetionMaximumAttempts < 0) ? AFHS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS_DEFAULT : this->openConnetionMaximumAttempts;

  return S_OK;
}

// other methods

CSegmentFragmentCollection *CMPUrlSourceSplitter_Protocol_Afhs::GetSegmentsFragmentsFromBootstrapInfoBox(CLogger *logger, const wchar_t *methodName, CParameterCollection *configurationParameters, CBootstrapInfoBox *bootstrapInfoBox, bool logCollection)
{
  HRESULT result = S_OK;
  CSegmentFragmentCollection *segmentsFragments = NULL;

  if (SUCCEEDED(result))
  {
    // now choose from bootstrap info -> QualityEntryTable highest quality (if exists) with segment run
    wchar_t *quality = NULL;
    CSegmentRunEntryCollection *segmentRunEntryTable = NULL;

    for (unsigned int i = 0; ((i <= bootstrapInfoBox->GetQualityEntryTable()->Count()) && (segmentRunEntryTable == NULL)); i++)
    {
      FREE_MEM(quality);

      // choose quality only for valid indexes, in another case is quality NULL
      if (i != bootstrapInfoBox->GetQualityEntryTable()->Count())
      {
        CBootstrapInfoQualityEntry *bootstrapInfoQualityEntry = bootstrapInfoBox->GetQualityEntryTable()->GetItem(0);
        quality = Duplicate(bootstrapInfoQualityEntry->GetQualityEntry());
      }

      // from segment run table choose segment with specifed quality (if exists) or segment with QualityEntryCount equal to zero
      for (unsigned int i = 0; i < bootstrapInfoBox->GetSegmentRunTable()->Count(); i++)
      {
        CSegmentRunTableBox *segmentRunTableBox = bootstrapInfoBox->GetSegmentRunTable()->GetItem(i);

        if (quality != NULL)
        {
          if (segmentRunTableBox->GetQualitySegmentUrlModifiers()->Contains(quality, false))
          {
            segmentRunEntryTable = segmentRunTableBox->GetSegmentRunEntryTable();
          }
        }
        else
        {
          if ((segmentRunTableBox->GetQualitySegmentUrlModifiers()->Count() == 0) || (segmentRunTableBox->GetQualitySegmentUrlModifiers()->Contains(L"", false)))
          {
            segmentRunEntryTable = segmentRunTableBox->GetSegmentRunEntryTable();
          }
        }
      }
    }

    if (segmentRunEntryTable == NULL)
    {
      logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, methodName, L"cannot find any segment run table");
      result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    if (SUCCEEDED(result))
    {
      if (segmentRunEntryTable->Count() == 0)
      {
        logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, methodName, L"cannot find any segment run entry");
        result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
      }
    }

    // from fragment run table choose fragment with specifed quality (if exists) or fragment with QualityEntryCount equal to zero
    CFragmentRunEntryCollection *fragmentRunEntryTableTemp = NULL;
    unsigned int timeScale = 0;
    for (unsigned int i = 0; i < bootstrapInfoBox->GetFragmentRunTable()->Count(); i++)
    {
      CFragmentRunTableBox *fragmentRunTableBox = bootstrapInfoBox->GetFragmentRunTable()->GetItem(i);

      if (quality != NULL)
      {
        if (fragmentRunTableBox->GetQualitySegmentUrlModifiers()->Contains(quality, false))
        {
          fragmentRunEntryTableTemp = fragmentRunTableBox->GetFragmentRunEntryTable();
          timeScale = fragmentRunTableBox->GetTimeScale();
        }
      }
      else
      {
        if ((fragmentRunTableBox->GetQualitySegmentUrlModifiers()->Count() == 0) || (fragmentRunTableBox->GetQualitySegmentUrlModifiers()->Contains(L"", false)))
        {
          fragmentRunEntryTableTemp = fragmentRunTableBox->GetFragmentRunEntryTable();
          timeScale = fragmentRunTableBox->GetTimeScale();
        }
      }
    }

    if (fragmentRunEntryTableTemp == NULL)
    {
      logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, methodName, L"cannot find any fragment run table");
      result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    CFragmentRunEntryCollection *fragmentRunEntryTable = new CFragmentRunEntryCollection();
    CHECK_POINTER_HRESULT(result, fragmentRunEntryTable, result, E_OUTOFMEMORY);

    if (SUCCEEDED(result))
    {
      // convert temporary fragment run table to simplier collection
      for (unsigned int i = 0; i < fragmentRunEntryTableTemp->Count(); i++)
      {
        CFragmentRunEntry *fragmentRunEntryTemp = fragmentRunEntryTableTemp->GetItem(i);
        unsigned int nextItemIndex = i + 1;
        CFragmentRunEntry *fragmentRunEntryTempNext = NULL;

        for (unsigned int j = nextItemIndex; j < fragmentRunEntryTableTemp->Count(); j++)
        {
          CFragmentRunEntry *temp = fragmentRunEntryTableTemp->GetItem(nextItemIndex);
          if (temp->GetFirstFragment() != 0)
          {
            fragmentRunEntryTempNext = temp;
            break;
          }
          else
          {
            nextItemIndex++;
          }
        }

        if (((fragmentRunEntryTemp->GetFirstFragmentTimestamp() == 0) && (i == 0)) ||
          (fragmentRunEntryTemp->GetFirstFragmentTimestamp() != 0))
        {
          uint64_t fragmentTimestamp = fragmentRunEntryTemp->GetFirstFragmentTimestamp();
          unsigned int lastFragment = (fragmentRunEntryTempNext == NULL) ? (fragmentRunEntryTemp->GetFirstFragment() + 1) : fragmentRunEntryTempNext->GetFirstFragment();

          for (unsigned int j = fragmentRunEntryTemp->GetFirstFragment(); j < lastFragment; j++)
          {
            unsigned int diff = j - fragmentRunEntryTemp->GetFirstFragment();
            CFragmentRunEntry *fragmentRunEntry = new CFragmentRunEntry(
              fragmentRunEntryTemp->GetFirstFragment() + diff,
              fragmentTimestamp,
              fragmentRunEntryTemp->GetFragmentDuration(),
              fragmentRunEntryTemp->GetDiscontinuityIndicator());
            fragmentTimestamp += fragmentRunEntryTemp->GetFragmentDuration();

            CHECK_POINTER_HRESULT(result, fragmentRunEntry, result, E_OUTOFMEMORY);

            if (SUCCEEDED(result))
            {
              result = (fragmentRunEntryTable->Add(fragmentRunEntry)) ? result : E_FAIL;
            }

            if (FAILED(result))
            {
              FREE_MEM_CLASS(fragmentRunEntry);
            }
          }
        }
      }
    }

    if (SUCCEEDED(result))
    {
      if (fragmentRunEntryTable->Count() == 0)
      {
        logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, methodName, L"cannot find any fragment run entry");
        result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
      }
    }

    if (SUCCEEDED(result))
    {
      wchar_t *serverBaseUrl = Duplicate(configurationParameters->GetValue(PARAMETER_NAME_AFHS_BASE_URL, true, L""));
      for (unsigned int i = 0; i < bootstrapInfoBox->GetServerEntryTable()->Count(); i++)
      {
        CBootstrapInfoServerEntry *serverEntry = bootstrapInfoBox->GetServerEntryTable()->GetItem(i);
        if (!IsNullOrEmptyOrWhitespace(serverEntry->GetServerEntry()))
        {
          FREE_MEM(serverBaseUrl);
          serverBaseUrl = Duplicate(serverEntry->GetServerEntry());
        }
      }

      CHECK_POINTER_HRESULT(result, serverBaseUrl, result, E_OUTOFMEMORY);

      if (SUCCEEDED(result))
      {
        wchar_t *mediaPartUrl = Duplicate(configurationParameters->GetValue(PARAMETER_NAME_AFHS_MEDIA_PART_URL, true, L""));
        CHECK_POINTER_HRESULT(result, mediaPartUrl, result, E_OUTOFMEMORY);

        if (SUCCEEDED(result))
        {
          wchar_t *baseUrl = FormatAbsoluteUrl(serverBaseUrl, mediaPartUrl);
          CHECK_POINTER_HRESULT(result, baseUrl, result, E_OUTOFMEMORY);

          if (SUCCEEDED(result))
          {
            //wchar_t *movieIdentifierUrl = FormatAbsoluteUrl(baseUrl, this->bootstrapInfoBox->GetMovieIdentifier());
            //CHECK_POINTER_HRESULT(result, movieIdentifierUrl, result, E_OUTOFMEMORY);

            if (SUCCEEDED(result))
            {
              //wchar_t *qualityUrl = FormatString(L"%s%s", movieIdentifierUrl, (quality == NULL) ? L"" : quality);
              wchar_t *qualityUrl = FormatAbsoluteUrl(baseUrl, (quality == NULL) ? L"" : quality);
              CHECK_POINTER_HRESULT(result, qualityUrl, result, E_OUTOFMEMORY);

              if (SUCCEEDED(result))
              {
                // convert segment run entry table to simplier collection
                segmentsFragments = new CSegmentFragmentCollection();
                CHECK_POINTER_HRESULT(result, segmentsFragments, result, E_OUTOFMEMORY);

                if (SUCCEEDED(result))
                {
                  unsigned int fragmentRunEntryTableIndex  = 0;

                  for (unsigned int i = 0; i < segmentRunEntryTable->Count(); i++)
                  {
                    CSegmentRunEntry *segmentRunEntry = segmentRunEntryTable->GetItem(i);
                    unsigned int lastSegment = (i == (segmentRunEntryTable->Count() - 1)) ? (segmentRunEntry->GetFirstSegment() + 1) : segmentRunEntryTable->GetItem(i + 1)->GetFirstSegment();

                    for (unsigned int j = segmentRunEntry->GetFirstSegment(); j < lastSegment; j++)
                    {
                      for (unsigned int k = 0; k < ((segmentRunEntry->GetFragmentsPerSegment() == UINT_MAX) ? fragmentRunEntryTable->Count() : segmentRunEntry->GetFragmentsPerSegment()); k++)
                      {
                        // choose fragment and get its timestamp
                        uint64_t timestamp = fragmentRunEntryTable->GetItem(min(fragmentRunEntryTableIndex, fragmentRunEntryTable->Count() - 1))->GetFirstFragmentTimestamp();
                        unsigned int firstFragment = fragmentRunEntryTable->GetItem(min(fragmentRunEntryTableIndex, fragmentRunEntryTable->Count() - 1))->GetFirstFragment();

                        if (fragmentRunEntryTableIndex >= fragmentRunEntryTable->Count())
                        {
                          // adjust fragment timestamp
                          timestamp += (fragmentRunEntryTableIndex - fragmentRunEntryTable->Count() + 1) * fragmentRunEntryTable->GetItem(fragmentRunEntryTable->Count() - 1)->GetFragmentDuration();
                          firstFragment += (fragmentRunEntryTableIndex - fragmentRunEntryTable->Count() + 1);
                        }
                        fragmentRunEntryTableIndex++;
                        wchar_t *url = FormatString(L"%sSeg%d-Frag%d", qualityUrl, j, firstFragment);
                        CHECK_POINTER_HRESULT(result, url, result, E_OUTOFMEMORY);

                        if (SUCCEEDED(result))
                        {
                          CSegmentFragment *segmentFragment = new CSegmentFragment(j, firstFragment, url, timestamp * 1000 / timeScale);
                          CHECK_POINTER_HRESULT(result, segmentFragment, result, E_OUTOFMEMORY);

                          if (SUCCEEDED(result))
                          {
                            result = (segmentsFragments->Add(segmentFragment)) ? result : E_FAIL;
                          }

                          if (FAILED(result))
                          {
                            FREE_MEM_CLASS(segmentFragment);
                          }
                        }
                        FREE_MEM(url);
                      }
                    }
                  }
                }
              }
              FREE_MEM(qualityUrl);
            }
            //FREE_MEM(movieIdentifierUrl);
          }
          FREE_MEM(baseUrl);
        }
        FREE_MEM(mediaPartUrl);
      }
      FREE_MEM(serverBaseUrl);
    }

    FREE_MEM(quality);
    FREE_MEM_CLASS(fragmentRunEntryTable);

    result = (segmentsFragments->Count() > 0) ? result : E_FAIL;

    if (SUCCEEDED(result) && (logCollection))
    {
      wchar_t *segmentFragmentLog = NULL;
      for (unsigned int i = 0; i < segmentsFragments->Count(); i++)
      {
        CSegmentFragment *segmentFragment = segmentsFragments->GetItem(i);

        wchar_t *temp = FormatString(L"%s%ssegment %u, fragment %u, url '%s', timestamp: %llu", (i == 0) ? L"" : segmentFragmentLog, (i == 0) ? L"" : L"\n", segmentFragment->GetSegment(), segmentFragment->GetFragment(), segmentFragment->GetUrl(), segmentFragment->GetFragmentTimestamp());
        FREE_MEM(segmentFragmentLog);
        segmentFragmentLog = temp;
      }

      if (segmentFragmentLog != NULL)
      {
        logger->Log(LOGGER_VERBOSE, L"%s: %s: segments and fragments:\n%s", PROTOCOL_IMPLEMENTATION_NAME, methodName, segmentFragmentLog);
      }

      FREE_MEM(segmentFragmentLog);
    }
  }

  if (FAILED(result))
  {
    FREE_MEM_CLASS(segmentsFragments);
  }

  return segmentsFragments;
}

wchar_t *CMPUrlSourceSplitter_Protocol_Afhs::GetStoreFile(void)
{
  wchar_t *result = NULL;
  wchar_t *folder = GetStoreFilePath(this->configurationParameters);

  if (folder != NULL)
  {
    wchar_t *guid = ConvertGuidToString(this->logger->loggerInstance);

    if (guid != NULL)
    {
      result = FormatString(L"%smpurlsourcesplitter_protocol_afhs_%s.temp", folder, guid);
    }
  }

  FREE_MEM(folder);
  return result;
}

CLinearBuffer *CMPUrlSourceSplitter_Protocol_Afhs::FillBufferForProcessing(CSegmentFragmentCollection *segmentsFragments, unsigned int segmentFragmentProcessing, wchar_t *storeFile)
{
  CLinearBuffer *result = NULL;

  if (segmentsFragments != NULL)
  {
    if (segmentFragmentProcessing < segmentsFragments->Count())
    {
      CSegmentFragment *segmentFragment = segmentsFragments->GetItem(segmentFragmentProcessing);

      if (segmentFragment->IsProcessed())
      {
        // segment and fragment is downloaded and processed
        // segment and fragment can be stored in memory or in store file

        // temporary buffer for data (from store file or from memory)
        unsigned int bufferLength = segmentFragment->GetLength();
        ALLOC_MEM_DEFINE_SET(buffer, unsigned char, bufferLength, 0);

        if (buffer != NULL)
        {
          if ((segmentFragment->IsStoredToFile()) && (storeFile != NULL))
          {
            // segment and fragment is stored into file and store file is specified

            LARGE_INTEGER size;
            size.QuadPart = 0;

            // open or create file
            HANDLE hTempFile = CreateFile(storeFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (hTempFile != INVALID_HANDLE_VALUE)
            {
              bool error = false;

              LONG distanceToMoveLow = (LONG)(segmentFragment->GetStoreFilePosition());
              LONG distanceToMoveHigh = (LONG)(segmentFragment->GetStoreFilePosition() >> 32);
              LONG distanceToMoveHighResult = distanceToMoveHigh;
              DWORD setFileResult = SetFilePointer(hTempFile, distanceToMoveLow, &distanceToMoveHighResult, FILE_BEGIN);
              if (setFileResult == INVALID_SET_FILE_POINTER)
              {
                DWORD lastError = GetLastError();
                if (lastError != NO_ERROR)
                {
                  this->logger->Log(LOGGER_ERROR, L"%s: %s: error occured while setting position: %lu", PROTOCOL_IMPLEMENTATION_NAME, METHOD_FILL_BUFFER_FOR_PROCESSING_NAME, lastError);
                  error = true;
                }
              }

              if (!error)
              {
                DWORD read = 0;
                if (ReadFile(hTempFile, buffer, bufferLength, &read, NULL) == 0)
                {
                  this->logger->Log(LOGGER_ERROR, L"%s: %s: error occured reading file: %lu", PROTOCOL_IMPLEMENTATION_NAME, METHOD_FILL_BUFFER_FOR_PROCESSING_NAME, GetLastError());
                  FREE_MEM(buffer);
                }
                else if (read != bufferLength)
                {
                  this->logger->Log(LOGGER_WARNING, L"%s: %s: readed data length not same as requested, requested: %u, readed: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_FILL_BUFFER_FOR_PROCESSING_NAME, bufferLength, read);
                  FREE_MEM(buffer);
                }
              }

              CloseHandle(hTempFile);
              hTempFile = INVALID_HANDLE_VALUE;
            }
          }
          else if (!segmentFragment->IsStoredToFile())
          {
            // segment and fragment is stored in memory
            if (segmentFragment->GetBuffer() != NULL)
            {
              if (segmentFragment->GetBuffer()->CopyFromBuffer(buffer, bufferLength, 0, 0) != bufferLength)
              {
                // error occured while copying data
                FREE_MEM(buffer);
              }
            }
          }
        }

        if (buffer != NULL)
        {
          // all data are read
          bool correct = false;
          result = new CLinearBuffer();
          if (result != NULL)
          {
            if (result->InitializeBuffer(bufferLength))
            {
              if (result->AddToBuffer(buffer, bufferLength) == bufferLength)
              {
                // everything correct, data copied successfully
                correct = true;
              }
            }
          }

          if (!correct)
          {
            FREE_MEM_CLASS(result);
          }
        }

        // clean-up buffer
        FREE_MEM(buffer);
      }
    }
  }

  return result;
}
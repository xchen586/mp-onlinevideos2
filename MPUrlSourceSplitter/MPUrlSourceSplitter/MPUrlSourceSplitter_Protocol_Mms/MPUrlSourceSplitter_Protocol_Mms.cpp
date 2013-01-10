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

#include "MPUrlSourceSplitter_Protocol_Mms.h"
#include "Utilities.h"
#include "LockMutex.h"
#include "VersionInfo.h"
#include "MPUrlSourceSplitter_Protocol_Mms_Parameters.h"

#include <WinInet.h>
#include <stdio.h>

// protocol implementation name
#ifdef _DEBUG
#define PROTOCOL_IMPLEMENTATION_NAME                                    L"MPUrlSourceSplitter_Protocol_Mmsd"
#else
#define PROTOCOL_IMPLEMENTATION_NAME                                    L"MPUrlSourceSplitter_Protocol_Mms"
#endif

#ifndef AV_RL16
#   define AV_RL16(x)                           \
    ((((const uint8_t*)(x))[1] << 8) |          \
      ((const uint8_t*)(x))[0])
#endif
#ifndef AV_RL32
#   define AV_RL32(x)                                \
    (((uint32_t)((const uint8_t*)(x))[3] << 24) |    \
               (((const uint8_t*)(x))[2] << 16) |    \
               (((const uint8_t*)(x))[1] <<  8) |    \
                ((const uint8_t*)(x))[0])
#endif
#ifndef AV_RL64
#   define AV_RL64(x)                                   \
    (((uint64_t)((const uint8_t*)(x))[7] << 56) |       \
     ((uint64_t)((const uint8_t*)(x))[6] << 48) |       \
     ((uint64_t)((const uint8_t*)(x))[5] << 40) |       \
     ((uint64_t)((const uint8_t*)(x))[4] << 32) |       \
     ((uint64_t)((const uint8_t*)(x))[3] << 24) |       \
     ((uint64_t)((const uint8_t*)(x))[2] << 16) |       \
     ((uint64_t)((const uint8_t*)(x))[1] <<  8) |       \
      (uint64_t)((const uint8_t*)(x))[0])
#endif

PIPlugin CreatePluginInstance(CParameterCollection *configuration)
{
  return new CMPUrlSourceSplitter_Protocol_Mms(configuration);
}

void DestroyPluginInstance(PIPlugin pProtocol)
{
  if (pProtocol != NULL)
  {
    CMPUrlSourceSplitter_Protocol_Mms *pClass = (CMPUrlSourceSplitter_Protocol_Mms *)pProtocol;
    delete pClass;
  }
}

CMPUrlSourceSplitter_Protocol_Mms::CMPUrlSourceSplitter_Protocol_Mms(CParameterCollection *configuration)
{
  this->configurationParameters = new CParameterCollection();
  if (configuration != NULL)
  {
    this->configurationParameters->Append(configuration);
  }

  this->logger = new CLogger(this->configurationParameters);
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME);

  wchar_t *version = GetVersionInfo(VERSION_INFO_MPURLSOURCESPLITTER_PROTOCOL_MMS, COMPILE_INFO_MPURLSOURCESPLITTER_PROTOCOL_MMS);
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
  
  this->receiveDataTimeout = MMS_RECEIVE_DATA_TIMEOUT_DEFAULT;
  this->openConnetionMaximumAttempts = MMS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS_DEFAULT;
  this->streamLength = 0;
  this->setLength = false;
  this->lengthCanBeSet = false;
  this->streamTime = 0;
  this->bytePosition = 0;
  this->lockMutex = CreateMutex(NULL, FALSE, NULL);
  this->lockCurlMutex = CreateMutex(NULL, FALSE, NULL);
  this->internalExitRequest = false;
  this->wholeStreamDownloaded = false;
  this->mainCurlInstance = NULL;
  this->seekingActive = false;
  this->supressData = false;
  this->sequenceNumber = 1;
  this->receivingData = false;
  this->mmsContext = NULL;
  this->shouldExit = false;
  this->streamEndedLogged = false;
  this->clientGuid = GUID_NULL;

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME);
}

CMPUrlSourceSplitter_Protocol_Mms::~CMPUrlSourceSplitter_Protocol_Mms()
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_DESTRUCTOR_NAME);

  if (this->IsConnected())
  {
    this->StopReceivingData();
  }

  FREE_MEM_CLASS(this->mainCurlInstance);
  FREE_MEM_CLASS(this->configurationParameters);
  FREE_MEM_CLASS(this->mmsContext);

  delete this->configurationParameters;

  if (this->lockMutex != NULL)
  {
    CloseHandle(this->lockMutex);
    this->lockMutex = NULL;
  }
  if (this->lockCurlMutex != NULL)
  {
    CloseHandle(this->lockCurlMutex);
    this->lockCurlMutex = NULL;
  }

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_DESTRUCTOR_NAME);
  FREE_MEM_CLASS(this->logger);
}

// IProtocol interface

bool CMPUrlSourceSplitter_Protocol_Mms::IsConnected(void)
{
  return ((this->mainCurlInstance != NULL) || (this->wholeStreamDownloaded) || (this->seekingActive));
}

unsigned int CMPUrlSourceSplitter_Protocol_Mms::GetOpenConnectionMaximumAttempts(void)
{
  return this->openConnetionMaximumAttempts;
}

HRESULT CMPUrlSourceSplitter_Protocol_Mms::ParseUrl(const CParameterCollection *parameters)
{
  HRESULT result = S_OK;
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_URL_NAME);

  this->ClearSession();
  if (parameters != NULL)
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

HRESULT CMPUrlSourceSplitter_Protocol_Mms::ReceiveData(bool *shouldExit, CReceiveData *receiveData)
{
  this->logger->Log(LOGGER_DATA, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME);
  this->shouldExit = *shouldExit;

  CLockMutex lock(this->lockMutex, INFINITE);

  if (this->internalExitRequest)
  {
    // there is internal exit request pending == changed timestamp
    // close connection
    this->StopReceivingData();

    // reopen connection
    // OpenConnection() reset wholeStreamDownloaded
    this->StartReceivingData(NULL);

    this->internalExitRequest = false;
  }

  bool changingStream = false;
  if ((!this->supressData) && (!this->seekingActive))
  {
    if (this->IsConnected())
    {
      if (!this->wholeStreamDownloaded)
      {
        if (!(this->shouldExit))
        {
          long responseCode = this->mainCurlInstance->GetHttpDownloadResponse()->GetResponseCode();
          if ((responseCode != 0) && ((responseCode < 200) || (responseCode >= 400)))
          {
            // response code 200 - 299 = OK
            // response code 300 - 399 = redirect (OK)
            this->logger->Log(LOGGER_VERBOSE, L"%s: %s: error response code: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, responseCode);
          }

          if ((responseCode >= 200) && (responseCode < 400))
          {
            if (this->receivingData)
            {
              if ((!this->setLength) && (this->lengthCanBeSet))
              {
                double streamSize = this->mainCurlInstance->GetDownloadContentLength();
                if ((streamSize > 0) && (this->streamTime < streamSize))
                {
                  LONGLONG total = LONGLONG(streamSize);
                  this->streamLength = total;
                  this->logger->Log(LOGGER_VERBOSE, L"%s: %s: setting total length: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, total);
                  receiveData->GetTotalLength()->SetTotalLength(total, false);
                  this->setLength = true;
                }
                else
                {
                  if ((this->streamLength == 0) || (this->bytePosition > (this->streamLength * 3 / 4)))
                  {
                    if (this->bytePosition != 0)
                    {
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
                  }
                }
              }
            }

            {
              CLockMutex lockData(this->lockCurlMutex, INFINITE);

              unsigned int bytesRead = this->mainCurlInstance->GetHttpDownloadResponse()->GetReceivedData()->GetBufferOccupiedSpace();
              if (bytesRead != 0)
              {
                unsigned int bufferSize = this->mmsContext->GetBuffer()->GetBufferSize();
                unsigned int freeSpace = this->mmsContext->GetBuffer()->GetBufferFreeSpace();
                unsigned int newBufferSize = max(bufferSize * 2, bufferSize + bytesRead);

                if (freeSpace < bytesRead)
                {
                  this->logger->Log(LOGGER_INFO, L"%s: %s: not enough free space in buffer for received data, buffer size: %d, free size: %d, received data: %d, new buffer size: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, bufferSize, freeSpace, bytesRead, newBufferSize);
                  if (!this->mmsContext->GetBuffer()->ResizeBuffer(newBufferSize))
                  {
                    this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, L"resizing of buffer unsuccessful");
                    // it indicates error
                    bytesRead = 0;
                  }
                }

                if (bytesRead != 0)
                {
                  ALLOC_MEM_DEFINE_SET(buffer, unsigned char, bytesRead, 0);
                  if (buffer != NULL)
                  {
                    this->mainCurlInstance->GetHttpDownloadResponse()->GetReceivedData()->CopyFromBuffer(buffer, bytesRead, 0, 0);
                    this->mmsContext->GetBuffer()->AddToBuffer(buffer, bytesRead);
                    this->mainCurlInstance->GetHttpDownloadResponse()->GetReceivedData()->RemoveFromBufferAndMove(bytesRead);
                  }
                  FREE_MEM(buffer);
                }
              }
            }
          }
        }

        if (this->receivingData)
        {
          if (this->mmsContext->GetAsfHeaderLength() != 0)
          {
            // ASF header was not send to filter

            if (this->sendAsfHeader)
            {
              // create media packet
              // set values of media packet
              CMediaPacket *mediaPacket = new CMediaPacket();
              mediaPacket->GetBuffer()->InitializeBuffer(this->mmsContext->GetAsfHeaderLength());
              mediaPacket->GetBuffer()->AddToBuffer(this->mmsContext->GetAsfHeader(), this->mmsContext->GetAsfHeaderLength());
              mediaPacket->SetStart(this->bytePosition);
              mediaPacket->SetEnd(this->bytePosition + this->mmsContext->GetAsfHeaderLength() - 1);

              if (!receiveData->GetMediaPacketCollection()->Add(mediaPacket))
              {
                FREE_MEM_CLASS(mediaPacket);
              }

              this->bytePosition += this->mmsContext->GetAsfHeaderLength();
            }

            this->mmsContext->ClearAsfHeader();
          }

          bool finish = false;
          while ((!this->shouldExit) && (!finish))
          {
            MMSChunk *chunk = new MMSChunk();
            if (chunk != NULL)
            {
              if (SUCCEEDED(this->GetMmsChunk(this->mmsContext, chunk)))
              {
                switch (chunk->GetChunkType())
                {
                case CHUNK_TYPE_END:
                  {
                    this->mmsContext->SetChunkSequence(0);
                    if (!this->streamEndedLogged)
                    {
                      this->logger->Log(LOGGER_INFO, L"%s: %s: stream ended", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME);
                      this->streamEndedLogged = true;
                    }
                  }
                  break;
                case CHUNK_TYPE_STREAM_CHANGE:
                  {
                    /*this->receivingData = false;
                    this->mmsContext->SetHeaderParsed(false);
                    HRESULT result = this->GetMmsHeaderData(this->mmsContext, chunk);
                    if (FAILED(result))
                    {
                      this->logger->Log(LOGGER_ERROR, L"%s: %s: get MMS header data failed with error: 0x%08X", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, result);
                      this->StopReceivingData();
                    }*/
                    changingStream = true;
                  }
                  break;
                case CHUNK_TYPE_DATA:
                  {
                    // create media packet
                    // set values of media packet
                    unsigned int packetLength = this->mmsContext->GetAsfPacketLength();
                    if (packetLength < chunk->GetChunkDataLength())
                    {
                      // error : ASF packet length is less than parsed chunk
                      this->logger->Log(LOGGER_WARNING, L"%s: %s: parsed chunk is bigger than ASF packet length, chunk length: %d, ASF packet length: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, chunk->GetChunkDataLength(), packetLength);
                    }
                    else
                    {
                      // packet must be padded with zeros until packetLength
                      CMediaPacket *mediaPacket = new CMediaPacket();
                      mediaPacket->GetBuffer()->InitializeBuffer(packetLength, 0);
                      mediaPacket->GetBuffer()->AddToBuffer(chunk->GetChunkData(), chunk->GetChunkDataLength());

                      if (packetLength > chunk->GetChunkDataLength())
                      {
                        unsigned int paddingLength = packetLength - chunk->GetChunkDataLength();
                        ALLOC_MEM_DEFINE_SET(paddingBuffer, unsigned char, paddingLength, 0);
                        if (paddingBuffer != NULL)
                        {
                          mediaPacket->GetBuffer()->AddToBuffer(paddingBuffer, paddingLength);
                        }
                        else
                        {
                          this->logger->Log(LOGGER_ERROR, L"%s: %s: cannot create padding buffer", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME);
                        }
                        FREE_MEM(paddingBuffer);
                      }

                      mediaPacket->SetStart(this->bytePosition);
                      mediaPacket->SetEnd(this->bytePosition + packetLength - 1);

                      if (!receiveData->GetMediaPacketCollection()->Add(mediaPacket))
                      {
                        FREE_MEM_CLASS(mediaPacket);
                      }

                      this->bytePosition += packetLength;
                    }
                  }
                  break;
                default:
                  // ignore unknown packets
                  break;
                }
              }
              else
              {
                finish = true;
              }

              delete chunk;
              chunk = NULL;
            }
            else
            {
              finish = true;
            }
          }
        }

        if ((changingStream) || (this->mainCurlInstance->GetCurlState() == CURL_STATE_RECEIVED_ALL_DATA))
        {
          // all data received, we're not receiving data
          this->logger->Log(LOGGER_VERBOSE, L"%s: %s: received all data", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME);

          // whole stream downloaded
          this->wholeStreamDownloaded = true;
          FREE_MEM_CLASS(this->mainCurlInstance);

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

            // notify filter the we reached end of stream
            receiveData->GetEndOfStreamReached()->SetStreamPosition(max(0, this->bytePosition - 1));
          }
        }

        if (changingStream)
        {
          // stop receiving data, we are done with this video
          this->StopReceivingData();
        }
      }
      else
      {
        // set total length (if not set earlier)
        if ((!this->setLength) && (this->lengthCanBeSet))
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
  }

  this->logger->Log(LOGGER_DATA, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME);
  return S_OK;
}

CParameterCollection *CMPUrlSourceSplitter_Protocol_Mms::GetConnectionParameters(void)
{
  CParameterCollection *result = new CParameterCollection();

  if (result != NULL)
  {
    if (!result->Append(this->configurationParameters))
    {
      FREE_MEM_CLASS(result);
    }
  }
  
  return result;
}

// ISimpleProtocol interface

unsigned int CMPUrlSourceSplitter_Protocol_Mms::GetReceiveDataTimeout(void)
{
  return this->receiveDataTimeout;
}

HRESULT CMPUrlSourceSplitter_Protocol_Mms::StartReceivingData(const CParameterCollection *parameters)
{
  HRESULT result = S_OK;
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME);

  CHECK_POINTER_DEFAULT_HRESULT(result, this->configurationParameters);

  wchar_t *url = NULL;
  if (SUCCEEDED(result))
  {
    url = ReplaceString(this->configurationParameters->GetValue(PARAMETER_NAME_URL, true, NULL), L"mms://", L"http://");
    if (url == NULL)
    {
      this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME, L"cannot allocate memory for RTMP url");
      result = E_OUTOFMEMORY;
    }
  }

  wchar_t *clientGuidString = NULL;

  if (SUCCEEDED(result))
  {
    result = CoCreateGuid(&this->clientGuid);
    if (FAILED(result))
    {
      this->clientGuid = GUID_NULL;
    }
  }

  if (SUCCEEDED(result))
  {
    wchar_t *guidTemp = ConvertGuidToString(this->clientGuid);
    CHECK_POINTER_HRESULT(result, guidTemp, result, E_OUTOFMEMORY);

    if (SUCCEEDED(result))
    {
      clientGuidString = FormatString(CLIENTGUID_FORMAT_VALUE, guidTemp);
      CHECK_POINTER_HRESULT(result, clientGuidString, result, E_OUTOFMEMORY);
    }

    FREE_MEM(guidTemp);
  }

  // open connection and send first command
  if (SUCCEEDED(result))
  {
    // lock access to stream
    CLockMutex lock(this->lockMutex, INFINITE);

    this->wholeStreamDownloaded = false;

    if (SUCCEEDED(result))
    {
      this->mainCurlInstance = new CHttpCurlInstance(this->logger, this->lockCurlMutex, PROTOCOL_IMPLEMENTATION_NAME, L"Main");
      CHECK_POINTER_HRESULT(result, this->mainCurlInstance, result, E_OUTOFMEMORY);

      if (SUCCEEDED(result))
      {
        CHttpDownloadRequest *request = new CHttpDownloadRequest();
        CHECK_POINTER_HRESULT(result, request, result, E_OUTOFMEMORY);

        if (SUCCEEDED(result))
        {
          request->SetUrl(url);
          request->SetReferer(this->configurationParameters->GetValue(PARAMETER_NAME_MMS_REFERER, true, NULL));
          request->SetUserAgent(this->configurationParameters->GetValue(PARAMETER_NAME_MMS_USER_AGENT, true, NULL));
          request->SetCookie(this->configurationParameters->GetValue(PARAMETER_NAME_MMS_COOKIE, true, NULL));
          request->SetHttpVersion(this->configurationParameters->GetValueLong(PARAMETER_NAME_MMS_VERSION, true, HTTP_VERSION_DEFAULT));
          request->SetIgnoreContentLength((this->configurationParameters->GetValueLong(PARAMETER_NAME_MMS_IGNORE_CONTENT_LENGTH, true, HTTP_IGNORE_CONTENT_LENGTH_DEFAULT) == 1L));

          this->sequenceNumber = 1;
          result = request->GetHeaders()->Add(L"Accept", L"*/*") ? S_OK : E_FAIL;
          result = request->GetHeaders()->Add(USERAGENT_NAME, USERAGENT_VALUE) ? result : E_FAIL;

          wchar_t *pragma = FormatString(L"no-cache,rate=1.000000,stream-time=%u,request-context=%u,max-duration=0", this->streamTime, this->sequenceNumber++);
          result = request->GetHeaders()->Add(L"Pragma", pragma) ? result : E_FAIL;
          FREE_MEM(pragma);

          result = request->GetHeaders()->Add(L"Pragma", clientGuidString) ? result : E_FAIL;
          result = request->GetHeaders()->Add(L"Connection", L"Close") ? result : E_FAIL;

          this->mainCurlInstance->SetReceivedDataTimeout(this->receiveDataTimeout);

          if (SUCCEEDED(result))
          {
            result = (this->mainCurlInstance->Initialize(request)) ? S_OK : E_FAIL;
          }

        }
        FREE_MEM_CLASS(request);
      }
    }

    if (SUCCEEDED(result))
    {
      FREE_MEM_CLASS(this->mmsContext);

      this->mmsContext = new MMSContext();
      result = (this->mmsContext != NULL) ? S_OK : E_OUTOFMEMORY;
    }

    if (SUCCEEDED(result))
    {
      this->mmsContext->SetTimeout(this->GetReceiveDataTimeout() / 2);

      // all parameters set
      // start receiving data

      result = (this->mainCurlInstance->StartReceivingData()) ? S_OK : E_FAIL;
    }

    if (SUCCEEDED(result))
    {
      // wait for HTTP status code

      long responseCode = this->mainCurlInstance->GetHttpDownloadResponse()->GetResponseCode();
      while (responseCode == 0)
      {
        responseCode = this->mainCurlInstance->GetHttpDownloadResponse()->GetResponseCode();
        if ((responseCode != 0) && ((responseCode < 200) || (responseCode >= 400)))
        {
          // response code 200 - 299 = OK
          // response code 300 - 399 = redirect (OK)
          this->logger->Log(LOGGER_VERBOSE, L"%s: %s: HTTP status code: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, responseCode);
          result = E_FAIL;
        }

        if ((responseCode == 0) && (this->mainCurlInstance->GetCurlState() == CURL_STATE_RECEIVED_ALL_DATA))
        {
          // we received all data, but no response code
          break;
        }

        // wait some time
        Sleep(10);
      }
    }
  }

  if (SUCCEEDED(result))
  {
    // get MMS header data and parse it
    result = this->GetMmsHeaderData(this->mmsContext, NULL);

    if (FAILED(result))
    {
      this->logger->Log(LOGGER_ERROR, L"%s: %s: (DESCRIBE) get MMS header data failed with error: 0x%08X", PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME, result);
    }
    else
    {
      {
        // lock access to stream
        CLockMutex lock(this->lockMutex, INFINITE);

        this->StopReceivingData();
      }
    }
  }

  if (SUCCEEDED(result))
  {
    // send play command
    {
      // lock access to stream
      CLockMutex lock(this->lockMutex, INFINITE);

      this->wholeStreamDownloaded = false;

      this->mainCurlInstance = new CHttpCurlInstance(this->logger, this->lockCurlMutex, PROTOCOL_IMPLEMENTATION_NAME, L"Main");
      CHECK_POINTER_HRESULT(result, this->mainCurlInstance, result, E_OUTOFMEMORY);

      if (SUCCEEDED(result))
      {
        CHttpDownloadRequest *request = new CHttpDownloadRequest();
        CHECK_POINTER_HRESULT(result, request, result, E_OUTOFMEMORY);

        if (SUCCEEDED(result))
        {
          request->SetUrl(url);
          request->SetReferer(this->configurationParameters->GetValue(PARAMETER_NAME_MMS_REFERER, true, NULL));
          request->SetUserAgent(this->configurationParameters->GetValue(PARAMETER_NAME_MMS_USER_AGENT, true, NULL));
          request->SetCookie(this->configurationParameters->GetValue(PARAMETER_NAME_MMS_COOKIE, true, NULL));
          request->SetHttpVersion(this->configurationParameters->GetValueLong(PARAMETER_NAME_MMS_VERSION, true, HTTP_VERSION_DEFAULT));
          request->SetIgnoreContentLength((this->configurationParameters->GetValueLong(PARAMETER_NAME_MMS_IGNORE_CONTENT_LENGTH, true, HTTP_IGNORE_CONTENT_LENGTH_DEFAULT) == 1L));

          this->sequenceNumber = 1;
          result = request->GetHeaders()->Add(L"Accept", L"*/*") ? S_OK : E_FAIL;
          result = request->GetHeaders()->Add(USERAGENT_NAME, USERAGENT_VALUE) ? result : E_FAIL;

          wchar_t *pragma = FormatString(L"no-cache,rate=1.000000,request-context=%u,stream-time=%u", this->sequenceNumber++, this->streamTime);
          result = request->GetHeaders()->Add(L"Pragma", pragma) ? result : E_FAIL;
          FREE_MEM(pragma);

          result = request->GetHeaders()->Add(L"Pragma", L"xPlayStrm=1") ? result : E_FAIL;
          result = request->GetHeaders()->Add(L"Pragma", clientGuidString) ? result : E_FAIL;

          pragma = FormatString(L"stream-switch-count=%d", this->mmsContext->GetStreams()->Count());
          result = request->GetHeaders()->Add(L"Pragma", pragma) ? result : E_FAIL;
          FREE_MEM(pragma);

          for (unsigned int i = 0; i < this->mmsContext->GetStreams()->Count(); i++)
          {
            MMSStream *stream = mmsContext->GetStreams()->GetItem(i);
            wchar_t *temp = FormatString(L"%sffff:%d:0 ", (pragma == NULL) ? L"stream-switch-entry=" : pragma, stream->GetId());
            FREE_MEM(pragma);
            pragma = temp;
          }
          result = request->GetHeaders()->Add(L"Pragma", pragma) ? result : E_FAIL;
          FREE_MEM(pragma);

          result = request->GetHeaders()->Add(L"Connection", L"Close") ? result : E_FAIL;

          this->mainCurlInstance->SetReceivedDataTimeout(this->receiveDataTimeout);

          if (SUCCEEDED(result))
          {
            result = (this->mainCurlInstance->Initialize(request)) ? S_OK : E_FAIL;
          }
        }
        FREE_MEM_CLASS(request);
      }

      if (SUCCEEDED(result))
      {
        // all parameters set
        // start receiving data

        result = (this->mainCurlInstance->StartReceivingData()) ? S_OK : E_FAIL;
      }

      if (SUCCEEDED(result))
      {
        // wait for HTTP status code

        long responseCode = this->mainCurlInstance->GetHttpDownloadResponse()->GetResponseCode();
        while (responseCode == 0)
        {
          responseCode = this->mainCurlInstance->GetHttpDownloadResponse()->GetResponseCode();
          if ((responseCode != 0) && ((responseCode < 200) || (responseCode >= 400)))
          {
            // response code 200 - 299 = OK
            // response code 300 - 399 = redirect (OK)
            this->logger->Log(LOGGER_VERBOSE, L"%s: %s: HTTP status code: %u", PROTOCOL_IMPLEMENTATION_NAME, METHOD_RECEIVE_DATA_NAME, responseCode);
            result = E_FAIL;
          }

          if ((responseCode == 0) && (this->mainCurlInstance->GetCurlState() == CURL_STATE_RECEIVED_ALL_DATA))
          {
            // we received all data, but no response code
            break;
          }

          // wait some time
          Sleep(10);
        }
      }
    }
  }

  if (SUCCEEDED(result))
  {
    // get MMS header data and parse it
    result = this->GetMmsHeaderData(this->mmsContext, NULL);

    if (FAILED(result))
    {
      this->logger->Log(LOGGER_ERROR, L"%s: %s: (PLAY) get MMS header data failed with error: 0x%08X", PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME, result);
    }
    else
    {
      {
        // lock access to stream
        CLockMutex lock(this->lockMutex, INFINITE);

        // set that we are receiving data
        this->receivingData = true;
      }
    }
  }

  if (FAILED(result))
  {
    this->StopReceivingData();
  }

  FREE_MEM(clientGuidString);
  FREE_MEM(url);

  // if seeking than not send ASF header
  this->sendAsfHeader = !this->seekingActive;

  // we are definitely not seeking
  this->seekingActive = false;

  // length can be set now
  this->lengthCanBeSet = true;

  this->logger->Log(LOGGER_INFO, SUCCEEDED(result) ? METHOD_END_FORMAT : METHOD_END_FAIL_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_START_RECEIVING_DATA_NAME);
  return result;
}

HRESULT CMPUrlSourceSplitter_Protocol_Mms::StopReceivingData(void)
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_STOP_RECEIVING_DATA_NAME);

  // lock access to stream
  CLockMutex lock(this->lockMutex, INFINITE);

  FREE_MEM_CLASS(this->mainCurlInstance);

  this->lengthCanBeSet = false;

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_STOP_RECEIVING_DATA_NAME);
  return S_OK;
}

HRESULT CMPUrlSourceSplitter_Protocol_Mms::QueryStreamProgress(LONGLONG *total, LONGLONG *current)
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
  
HRESULT CMPUrlSourceSplitter_Protocol_Mms::QueryStreamAvailableLength(CStreamAvailableLength *availableLength)
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

HRESULT CMPUrlSourceSplitter_Protocol_Mms::ClearSession(void)
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CLEAR_SESSION_NAME);

  if (this->IsConnected())
  {
    this->StopReceivingData();
  }
 
  this->internalExitRequest = false;
  this->streamLength = 0;
  this->setLength = false;
  this->lengthCanBeSet = false;
  this->streamTime = 0;
  this->bytePosition = 0;
  this->wholeStreamDownloaded = false;
  this->receiveDataTimeout = MMS_RECEIVE_DATA_TIMEOUT_DEFAULT;
  this->openConnetionMaximumAttempts = MMS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS_DEFAULT;
  this->sequenceNumber = 1;
  this->receivingData = false;
  this->shouldExit = false;
  this->streamEndedLogged = false;

  FREE_MEM_CLASS(this->mmsContext);

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_CLEAR_SESSION_NAME);
  return S_OK;
}

// ISeeking interface

unsigned int CMPUrlSourceSplitter_Protocol_Mms::GetSeekingCapabilities(void)
{
  return SEEKING_METHOD_TIME;
}

int64_t CMPUrlSourceSplitter_Protocol_Mms::SeekToTime(int64_t time)
{
  int64_t result = -1;

  {
    CLockMutex lock(this->lockMutex, INFINITE);

    this->logger->Log(LOGGER_VERBOSE, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_TIME_NAME);
    this->logger->Log(LOGGER_VERBOSE, L"%s: %s: from time: %llu", PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_TIME_NAME, time);

    // this block ReceiveData() method from calling any method
    this->seekingActive = true;

    // close connection
    this->StopReceivingData();

    this->bytePosition = 0;
  }

  // MMS protocol can seek to ms
  // time is in ms

  // 1 second back
  this->streamTime = max(0, time - 1000);

  // do not lock mutex, because we need to receive data in StartReceivingData() method
  // reopen connection
  // StartReceivingData() reset wholeStreamDownloaded

  if (SUCCEEDED(this->StartReceivingData(NULL)))
  {
    result = max(0, time - 1000);
  }

  this->logger->Log(LOGGER_VERBOSE, METHOD_END_INT64_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_TIME_NAME, result);
  return result;
}

int64_t CMPUrlSourceSplitter_Protocol_Mms::SeekToPosition(int64_t start, int64_t end)
{
  this->logger->Log(LOGGER_VERBOSE, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_POSITION_NAME);
  this->logger->Log(LOGGER_VERBOSE, L"%s: %s: from time: %llu, to time: %llu", PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_POSITION_NAME, start, end);

  int64_t result = -1;

  this->logger->Log(LOGGER_VERBOSE, METHOD_END_INT64_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_SEEK_TO_POSITION_NAME, result);
  return result;
}

void CMPUrlSourceSplitter_Protocol_Mms::SetSupressData(bool supressData)
{
  this->supressData = supressData;
}

// IPlugin interface

const wchar_t *CMPUrlSourceSplitter_Protocol_Mms::GetName(void)
{
  return PROTOCOL_NAME;
}

GUID CMPUrlSourceSplitter_Protocol_Mms::GetInstanceId(void)
{
  return this->logger->loggerInstance;
}

HRESULT CMPUrlSourceSplitter_Protocol_Mms::Initialize(PluginConfiguration *configuration)
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

  this->receiveDataTimeout = this->configurationParameters->GetValueLong(PARAMETER_NAME_MMS_RECEIVE_DATA_TIMEOUT, true, MMS_RECEIVE_DATA_TIMEOUT_DEFAULT);
  this->openConnetionMaximumAttempts = this->configurationParameters->GetValueLong(PARAMETER_NAME_MMS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS, true, MMS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS_DEFAULT);

  this->receiveDataTimeout = (this->receiveDataTimeout < 0) ? MMS_RECEIVE_DATA_TIMEOUT_DEFAULT : this->receiveDataTimeout;
  this->openConnetionMaximumAttempts = (this->openConnetionMaximumAttempts < 0) ? MMS_OPEN_CONNECTION_MAXIMUM_ATTEMPTS_DEFAULT : this->openConnetionMaximumAttempts;

  return S_OK;
}

// other methods

HRESULT CMPUrlSourceSplitter_Protocol_Mms::GetMmsChunk(MMSContext *mmsContext, MMSChunk *mmsChunk)
{
  HRESULT result = S_OK;
  CHECK_POINTER_DEFAULT_HRESULT(result, mmsContext);
  CHECK_POINTER_DEFAULT_HRESULT(result, mmsChunk);

  if (SUCCEEDED(result))
  {
    result = mmsContext->IsValid() ? S_OK : E_INVALIDARG;
    if (FAILED(result))
    {
      this->logger->Log(LOGGER_ERROR, METHOD_MESSAGE_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_GET_MMS_CHUNK_NAME, L"MMS context is not valid");
    }
  }

  if (SUCCEEDED(result))
  {
    ALLOC_MEM_DEFINE_SET(chunkHeader, unsigned char, CHUNK_HEADER_LENGTH, 0);
    ALLOC_MEM_DEFINE_SET(extHeader, unsigned char, EXT_HEADER_LENGTH, 0);

    CHECK_POINTER_HRESULT(result, chunkHeader, S_OK, E_OUTOFMEMORY);
    CHECK_POINTER_HRESULT(result, extHeader, S_OK, E_OUTOFMEMORY);
    result = (mmsContext->GetBuffer()->GetBufferOccupiedSpace() >= CHUNK_HEADER_LENGTH) ? result : HRESULT_FROM_WIN32(ERROR_NO_DATA);

    if (SUCCEEDED(result))
    {
      unsigned int chunkType = 0;
      unsigned int chunkDataLength = 0;
      unsigned int chunkPacketLength = 0;
      unsigned int extHeaderLength = 0;

      mmsContext->GetBuffer()->CopyFromBuffer(chunkHeader, CHUNK_HEADER_LENGTH, 0, 0);

      chunkType = AV_RL16(chunkHeader);
      chunkDataLength  = AV_RL16(chunkHeader + 2);
      chunkPacketLength = chunkDataLength + 4;

      result = (mmsContext->GetBuffer()->GetBufferOccupiedSpace() >= chunkPacketLength) ? result : HRESULT_FROM_WIN32(ERROR_NO_DATA);

      if (SUCCEEDED(result))
      {
        switch (chunkType)
        {
        case CHUNK_TYPE_END:
        case CHUNK_TYPE_STREAM_CHANGE:
          extHeaderLength = 4;
          break;
        case CHUNK_TYPE_ASF_HEADER:
        case CHUNK_TYPE_DATA:
          extHeaderLength = 8;
          break;
        default:
          this->logger->Log(LOGGER_WARNING, L"%s: %s: unknown chunk type: %d, length: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_GET_MMS_CHUNK_NAME, chunkType, chunkDataLength);
          result = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
      }

      if (SUCCEEDED(result))
      {
        mmsContext->GetBuffer()->CopyFromBuffer(extHeader, extHeaderLength, 0, CHUNK_HEADER_LENGTH);

        mmsChunk->SetChunkType(chunkType);
        if (SUCCEEDED(result))
        {
          ALLOC_MEM_DEFINE_SET(temp, unsigned char, (chunkDataLength - extHeaderLength), 0);
          CHECK_POINTER(result, temp, result, E_OUTOFMEMORY);
          if (SUCCEEDED(result))
          {
            mmsContext->GetBuffer()->CopyFromBuffer(temp, chunkDataLength - extHeaderLength, 0, CHUNK_HEADER_LENGTH + extHeaderLength);
            result = mmsChunk->SetChunkData(temp, chunkDataLength - extHeaderLength) ? result : E_OUTOFMEMORY;
          }
          FREE_MEM(temp);
        }
        if (SUCCEEDED(result))
        {
          ALLOC_MEM_DEFINE_SET(temp, unsigned char, extHeaderLength, 0);
          CHECK_POINTER(result, temp, result, E_OUTOFMEMORY);
          if (SUCCEEDED(result))
          {
            mmsContext->GetBuffer()->CopyFromBuffer(temp, extHeaderLength, 0, CHUNK_HEADER_LENGTH);
            result = mmsChunk->SetExtraHeaderData(temp, extHeaderLength) ? result : E_OUTOFMEMORY;
          }
          FREE_MEM(temp);
        }

        if (SUCCEEDED(result))
        {
          if ((chunkType == CHUNK_TYPE_END) || (chunkType == CHUNK_TYPE_DATA))
          {
            mmsContext->SetChunkSequence(AV_RL32(extHeader));
          }

          mmsContext->GetBuffer()->RemoveFromBufferAndMove(chunkPacketLength);
        }
        else
        {
          mmsChunk->Clear();
        }
      }
    }

    FREE_MEM(chunkHeader);
    FREE_MEM(extHeader);
  }
  return result;
}

HRESULT CMPUrlSourceSplitter_Protocol_Mms::ParseMmsAsfHeader(MMSContext *mmsContext, MMSChunk *mmsChunk)
{
  HRESULT result = S_OK;
  CHECK_POINTER_DEFAULT_HRESULT(result, mmsContext);
  CHECK_POINTER_DEFAULT_HRESULT(result, mmsChunk);

  if (FAILED(result))
  {
    return result;
  }

  mmsContext->GetStreams()->Clear();

  if ((mmsChunk->GetChunkDataLength() < (sizeof(ASF_GUID) * 2 + 22)) ||
    (memcmp(mmsChunk->GetChunkData(), ASF_HEADER, sizeof(ASF_GUID))))
  {
    this->logger->Log(LOGGER_ERROR, L"%s: %s: corrupt stream, invalid ASF header, size: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME, mmsChunk->GetChunkDataLength());
    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
  }

  const unsigned char *start = mmsChunk->GetChunkData();
  const unsigned char *end = mmsChunk->GetChunkData() + mmsChunk->GetChunkDataLength();

  start += sizeof(ASF_GUID) + 14;
  int flags;
  int stream_id;

  while ((SUCCEEDED(result)) && ((end - start) >= (sizeof(ASF_GUID) + 8)))
  {
    uint64_t chunksize;

    if (!memcmp(start, ASF_DATA_HEADER, sizeof(ASF_GUID)))
    {
      chunksize = 50; // see Reference [2] section 5.1
    }
    else
    {
      chunksize = AV_RL64(start + sizeof(ASF_GUID));
    }

    if (!chunksize || (chunksize > (end - start)))
    {
      this->logger->Log(LOGGER_ERROR, L"%s: %s: header chunk size is invalid: %I64d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME, chunksize);
      return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

    if (!memcmp(start, ASF_FILE_HEADER, sizeof(ASF_GUID)))
    {
      /* read packet size */
      if ((end - start) > (sizeof(ASF_GUID) * 2 + 68))
      {
        mmsContext->SetAsfPacketLength(AV_RL32(start + sizeof(ASF_GUID) * 2 + 64));
        this->logger->Log(LOGGER_INFO, L"%s: %s: ASF packet length: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME, mmsContext->GetAsfPacketLength());
        if (mmsContext->GetAsfPacketLength() > HEADER_BUFFER_SIZE)
        {
          this->logger->Log(LOGGER_ERROR, L"%s: %s: corrupted stream, too large ASF packet length: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME, mmsContext->GetAsfPacketLength());
          return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }
      }
    }
    else if (!memcmp(start, ASF_STREAM_HEADER, sizeof(ASF_GUID)))
    {
      flags = AV_RL16(start + sizeof(ASF_GUID) * 3 + 24);
      stream_id = flags & 0x7F;

      if (mmsContext->GetStreams()->Count() < MMS_MAX_STREAMS)
      {
        MMSStream *stream = new MMSStream();
        if (stream == NULL)
        {
          this->logger->Log(LOGGER_ERROR, L"%s: %s: not enough memory to create stream", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME);
          return E_OUTOFMEMORY;
        }

        stream->SetId(stream_id);
        if (!mmsContext->GetStreams()->Add(stream))
        {
          // cannot add MMS stream to MMS context
          // delete stream and return error
          delete stream;
          stream = NULL;
          this->logger->Log(LOGGER_ERROR, L"%s: %s: cannot add stream to MMS context", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME);
          return E_FAIL;
        }
        else
        {
          this->logger->Log(LOGGER_VERBOSE, L"%s: %s: added stream with id: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME, stream->GetId());
        }
      }
      else
      {
        this->logger->Log(LOGGER_ERROR, L"%s: %s: corrupted stream, too many audio/video streams", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME);
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
      }
    }
    else if (!memcmp(start, ASF_EXT_STREAM_HEADER, sizeof(ASF_GUID)))
    {
      if ((end - start) >= 88)
      {
        int stream_count = AV_RL16(start + 84);
        int ext_len_count = AV_RL16(start + 86);
        uint64_t skip_bytes = 88;

        while (stream_count--)
        {
          if ((end - start) < (skip_bytes + 4))
          {
            this->logger->Log(LOGGER_ERROR, L"%s: %s: corrupted stream, next stream name length is not in the buffer", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME);
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
          }
          skip_bytes += 4 + AV_RL16(start + skip_bytes + 2);
        }

        while (ext_len_count--)
        {
          if ((end - start) < (skip_bytes + 22))
          {
            this->logger->Log(LOGGER_ERROR, L"%s: %s: corrupted stream, next extension system info length is not in the buffer", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME);
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
          }
          skip_bytes += 22 + AV_RL32(start + skip_bytes + 18);
        }

        if ((end - start) < skip_bytes)
        {
          this->logger->Log(LOGGER_ERROR, L"%s: %s: corrupted stream, the last extension system info length is invalid", PROTOCOL_IMPLEMENTATION_NAME, METHOD_PARSE_MMS_ASF_HEADER_NAME);
          return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        if (chunksize - skip_bytes > 24)
        {
          chunksize = skip_bytes;
        }
      }
    }
    else if (!memcmp(start, ASF_HEAD1_GUID, sizeof(ASF_GUID)))
    {
      chunksize = 46; // see references [2] section 3.4. This should be set 46.
    }

    start += chunksize;
  }

  return S_OK;
}

HRESULT CMPUrlSourceSplitter_Protocol_Mms::GetMmsHeaderData(MMSContext *mmsContext, MMSChunk *mmsChunk)
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_GET_MMS_HEADER_DATA_NAME);

  HRESULT result = S_OK;
  CHECK_POINTER_DEFAULT_HRESULT(result, mmsContext);

  if (SUCCEEDED(result))
  {
    result = mmsContext->IsValid() ? S_OK : E_INVALIDARG;
  }

  if (SUCCEEDED(result))
  {
    bool finish = false;
    DWORD startTime = GetTickCount();
    MMSChunk *tempMmsChunk = new MMSChunk(mmsChunk);
    result = (tempMmsChunk != NULL) ? S_OK : E_OUTOFMEMORY;

    while (SUCCEEDED(result) && (!finish))
    {
      // fill data to buffer
      if (SUCCEEDED(result))
      {
        CLockMutex(this->lockCurlMutex, INFINITE);

        unsigned int bytesRead = this->mainCurlInstance->GetHttpDownloadResponse()->GetReceivedData()->GetBufferOccupiedSpace();
        if (bytesRead > 0)
        {
          ALLOC_MEM_DEFINE_SET(buffer, unsigned char, bytesRead, 0);
          if (buffer != 0)
          {
            this->mainCurlInstance->GetHttpDownloadResponse()->GetReceivedData()->CopyFromBuffer(buffer, bytesRead, 0, 0);
            result = (this->mmsContext->GetBuffer()->AddToBufferWithResize(buffer, bytesRead) == bytesRead) ? S_OK : E_OUTOFMEMORY;
            this->mainCurlInstance->GetHttpDownloadResponse()->GetReceivedData()->RemoveFromBufferAndMove(bytesRead);
          }
          FREE_MEM(buffer);
        }
      }

      if (tempMmsChunk->IsCleared())
      {
        result = this->GetMmsChunk(mmsContext, tempMmsChunk);
      }

      if (SUCCEEDED(result) || (result == HRESULT_FROM_WIN32(ERROR_INVALID_DATA)))
      {
        switch (tempMmsChunk->GetChunkType())
        {
        case CHUNK_TYPE_END:
          this->logger->Log(LOGGER_VERBOSE, L"%s: %s: chunk type: CHUNK_TYPE_END, length: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_GET_MMS_HEADER_DATA_NAME, tempMmsChunk->GetChunkDataLength());
          break;
        case CHUNK_TYPE_STREAM_CHANGE:
          this->logger->Log(LOGGER_VERBOSE, L"%s: %s: chunk type: CHUNK_TYPE_STREAM_CHANGE, length: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_GET_MMS_HEADER_DATA_NAME, tempMmsChunk->GetChunkDataLength());
          break;
        case CHUNK_TYPE_ASF_HEADER:
          this->logger->Log(LOGGER_VERBOSE, L"%s: %s: chunk type: CHUNK_TYPE_ASF_HEADER, length: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_GET_MMS_HEADER_DATA_NAME, tempMmsChunk->GetChunkDataLength());
          break;
        case CHUNK_TYPE_DATA:
          this->logger->Log(LOGGER_VERBOSE, L"%s: %s: chunk type: CHUNK_TYPE_DATA, length: %d", PROTOCOL_IMPLEMENTATION_NAME, METHOD_GET_MMS_HEADER_DATA_NAME, tempMmsChunk->GetChunkDataLength());
          break;
        }

        switch(tempMmsChunk->GetChunkType())
        {
        case CHUNK_TYPE_ASF_HEADER:
          // get asf header and stored it
          {
            if (!mmsContext->GetHeaderParsed())
            {
              result = this->ParseMmsAsfHeader(this->mmsContext, tempMmsChunk);

              if (SUCCEEDED(result))
              {
                this->logger->Log(LOGGER_VERBOSE, L"%s: %s: ASF header successfully parsed", PROTOCOL_IMPLEMENTATION_NAME, METHOD_GET_MMS_HEADER_DATA_NAME);
                mmsContext->SetHeaderParsed(true);
                if (!mmsContext->SetAsfHeader(tempMmsChunk->GetChunkData(), tempMmsChunk->GetChunkDataLength()))
                {
                  result = E_OUTOFMEMORY;
                }
                finish = true;
              }
            }
            else
            {
              finish = true;
            }
          }
          break;

        case CHUNK_TYPE_DATA:
          finish = true;
          break;

        default:
          result = S_OK;
          break;
        }
      }

      tempMmsChunk->Clear();

      // test for HRESULT_FROM_WIN32(ERROR_NO_DATA)
      if (result == HRESULT_FROM_WIN32(ERROR_NO_DATA))
      {
        if ((GetTickCount() - startTime) <= mmsContext->GetTimeout())
        {
          // reset error, wait some time
          result = S_OK;
          Sleep(1);
        }
      }
    }

    // free MMS chunk
    if (tempMmsChunk != NULL)
    {
      delete tempMmsChunk;
      tempMmsChunk = NULL;
    }
  }

  this->logger->Log(LOGGER_INFO, METHOD_END_HRESULT_FORMAT, PROTOCOL_IMPLEMENTATION_NAME, METHOD_GET_MMS_HEADER_DATA_NAME, result);
  return result;
}
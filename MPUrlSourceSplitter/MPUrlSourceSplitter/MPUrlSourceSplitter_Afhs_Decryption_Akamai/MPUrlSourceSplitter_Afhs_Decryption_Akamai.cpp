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

#include "MPUrlSourceSplitter_Afhs_Decryption_Akamai.h"
#include "MPUrlSourceSplitter_Afhs_Decryption_Akamai_Parameters.h"
#include "VersionInfo.h"
#include "formatUrl.h"
#include "BoxFactory.h"
#include "MediaDataBox.h"
#include "base64.h"

// AFHS decryption plugin implementation name
#ifdef _DEBUG
#define PLUGIN_IMPLEMENTATION_NAME                                            L"MPUrlSourceSplitter_Afhs_Decryption_Akamaid"
#else
#define PLUGIN_IMPLEMENTATION_NAME                                            L"MPUrlSourceSplitter_Afhs_Decryption_Akamai"
#endif

PIPlugin CreatePluginInstance(CParameterCollection *configuration)
{
  return new CMPUrlSourceSplitter_Afhs_Decryption_Akamai(configuration);
}

void DestroyPluginInstance(PIPlugin pProtocol)
{
  if (pProtocol != NULL)
  {
    CMPUrlSourceSplitter_Afhs_Decryption_Akamai *pClass = (CMPUrlSourceSplitter_Afhs_Decryption_Akamai *)pProtocol;
    delete pClass;
  }
}

CMPUrlSourceSplitter_Afhs_Decryption_Akamai::CMPUrlSourceSplitter_Afhs_Decryption_Akamai(CParameterCollection *configuration)
{
  this->configurationParameters = new CParameterCollection();
  if (configuration != NULL)
  {
    this->configurationParameters->Append(configuration);
  }

  this->logger = new CLogger(this->configurationParameters);
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PLUGIN_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME);

  wchar_t *version = GetVersionInfo(VERSION_INFO_MPURLSOURCESPLITTER_AFHS_DECRYPTION_AKAMAI, COMPILE_INFO_MPURLSOURCESPLITTER_AFHS_DECRYPTION_AKAMAI);
  if (version != NULL)
  {
    this->logger->Log(LOGGER_INFO, METHOD_MESSAGE_FORMAT, PLUGIN_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME, version);
  }
  FREE_MEM(version);

  version = CCurlInstance::GetCurlVersion();
  if (version != NULL)
  {
    this->logger->Log(LOGGER_INFO, METHOD_MESSAGE_FORMAT, PLUGIN_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME, version);
  }
  FREE_MEM(version);
  
  this->receivedDataAnalysed = false;
  this->receivedDataCanBeDecrypted = false;
  this->initializeAkamaiFlashInstanceResult = E_NOT_VALID_STATE;
  this->akamaiFlashInstance = NULL;
  this->lastKeyUrl = NULL;
  this->lastKeyBase64 = NULL;
  this->akamaiGuid = NULL;
  this->keyRequestPending = false;
  this->akamaiSwfFile = NULL;
  this->lastTimestamp = 0;
  this->sessionID = NULL;

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PLUGIN_IMPLEMENTATION_NAME, METHOD_CONSTRUCTOR_NAME);
}

CMPUrlSourceSplitter_Afhs_Decryption_Akamai::~CMPUrlSourceSplitter_Afhs_Decryption_Akamai()
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PLUGIN_IMPLEMENTATION_NAME, METHOD_DESTRUCTOR_NAME);

  FREE_MEM_CLASS(this->akamaiFlashInstance);

  if (this->akamaiSwfFile != NULL)
  {
    DeleteFile(this->akamaiSwfFile);
  }

  FREE_MEM(this->lastKeyUrl);
  FREE_MEM(this->lastKeyBase64);
  FREE_MEM(this->akamaiGuid);
  FREE_MEM_CLASS(this->configurationParameters);
  FREE_MEM(this->akamaiSwfFile);
  FREE_MEM(this->sessionID);

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, PLUGIN_IMPLEMENTATION_NAME, METHOD_DESTRUCTOR_NAME);

  FREE_MEM_CLASS(this->logger);
}

// IPlugin interface

const wchar_t *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetName(void)
{
  return DECRYPTION_NAME;
}

GUID CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetInstanceId(void)
{
  return this->logger->loggerInstance;
}

HRESULT CMPUrlSourceSplitter_Afhs_Decryption_Akamai::Initialize(PluginConfiguration *configuration)
{
  if (configuration == NULL)
  {
    return E_POINTER;
  }
  
  AfhsDecryptionPluginConfiguration *pluginConfiguration = (AfhsDecryptionPluginConfiguration *)configuration;
  this->logger->SetParameters(pluginConfiguration->configuration);

  this->configurationParameters->Clear();
  if (pluginConfiguration->configuration != NULL)
  {
    this->configurationParameters->Append(pluginConfiguration->configuration);
  }
  this->configurationParameters->LogCollection(this->logger, LOGGER_VERBOSE, PLUGIN_IMPLEMENTATION_NAME, METHOD_INITIALIZE_NAME);

  return S_OK;
}

// IAfhsSimpleDecryptionPlugin interface

HRESULT CMPUrlSourceSplitter_Afhs_Decryption_Akamai::ClearSession(void)
{
  FREE_MEM_CLASS(this->akamaiFlashInstance);

  if (this->akamaiSwfFile != NULL)
  {
    DeleteFile(this->akamaiSwfFile);
  }

  this->receivedDataAnalysed = false;
  this->receivedDataCanBeDecrypted = false;
  this->keyRequestPending = false;
  this->initializeAkamaiFlashInstanceResult = E_NOT_VALID_STATE;
  FREE_MEM(this->lastKeyUrl);
  FREE_MEM(this->lastKeyBase64);
  FREE_MEM(this->akamaiGuid);
  FREE_MEM(this->akamaiSwfFile);
  this->lastTimestamp = 0;
  FREE_MEM(this->sessionID);

  return S_OK;
}

HRESULT CMPUrlSourceSplitter_Afhs_Decryption_Akamai::ProcessSegmentsAndFragments(CAfhsDecryptionContext *context)
{
  HRESULT result = S_OK;
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, PLUGIN_IMPLEMENTATION_NAME, METHOD_PROCESS_SEGMENTS_AND_FRAGMENTS_NAME);

  // if key request is pending then check last segment and fragment
  if (this->keyRequestPending)
  {
    CSegmentFragment *segmentFragment = context->GetSegmentsFragments()->GetItem(context->GetSegmentsFragments()->Count() - 1);
    if (segmentFragment->IsDownloaded())
    {
      // decryption key received
      FREE_MEM(this->lastKeyBase64);
      this->lastKeyBase64 = this->GetDecryptionKeyFromSegmentFragment(segmentFragment);
      CHECK_POINTER_HRESULT(result, this->lastKeyBase64, result, E_FAIL);

      this->keyRequestPending = false;
      context->GetSegmentsFragments()->Remove(context->GetSegmentsFragments()->Count() - 1);
    }
  }

  for (unsigned int i = 0; (SUCCEEDED(result) && (!this->keyRequestPending) && (i < context->GetSegmentsFragments()->Count())); i++)
  {
    CSegmentFragment *segmentFragment = context->GetSegmentsFragments()->GetItem(i);

    if (segmentFragment->IsDownloaded() && (!segmentFragment->IsProcessed()))
    {
      // if segment and fragment is downloaded, but it's not processed

      // extract all boxes from segment and fragment
      // get media data box, parse each akamai FLV packet
      // reconstruct segment and fragment back
      CBoxCollection *boxes = this->GetBoxes(segmentFragment);
      CHECK_POINTER_HRESULT(result, boxes, result, E_FAIL);

      CParsedMediaDataBox *parsedMediaDataBox = this->ParseMediaDataBox(context, segmentFragment);
      CHECK_POINTER_HRESULT(result, parsedMediaDataBox, result, E_FAIL);

      if ((!this->keyRequestPending) && (parsedMediaDataBox->IsMediaDataBox()) && (parsedMediaDataBox->GetAkamaiGuid() != NULL) && (parsedMediaDataBox->GetAkamaiFlvPackets()->Count() > 0))
      {
        // it is media data box
        // specified akamai GUID
        // at least one akamai FLV packet

        FREE_MEM(this->akamaiGuid);
        this->akamaiGuid = Duplicate(parsedMediaDataBox->GetAkamaiGuid());
        if (this->akamaiGuid != NULL)
        {
          // key url have to be in first FLV packet
          CAkamaiFlvPacket *packet = parsedMediaDataBox->GetAkamaiFlvPackets()->GetItem(0);

          if (packet->HasKey())
          {
            if ((CompareWithNull(this->lastKeyUrl, packet->GetKeyUrl()) != 0) || (this->lastKeyBase64 == NULL))
            {
              // download new decryption key from url
              CSegmentFragment *keyDecryptionSegmentFragment = new CSegmentFragment(UINT_MAX, UINT_MAX, UINT64_MAX);
              bool continueKeyRequest = (keyDecryptionSegmentFragment != NULL);

              if (continueKeyRequest)
              {
                FREE_MEM(this->lastKeyUrl);
                FREE_MEM(this->sessionID);
                this->sessionID = Duplicate(packet->GetSessionId());
                this->lastKeyUrl = Duplicate(packet->GetKeyUrl());

                wchar_t *url = this->GetKeyUrlFromUrl(context->GetSegmentsFragments()->GetItem(0)->GetHttpDownloadRequest()->GetUrl(), packet->GetKeyUrl(), this->akamaiGuid);                
                continueKeyRequest &= (url != NULL) && (this->sessionID != NULL) && (this->lastKeyUrl != NULL);
                if (continueKeyRequest)
                {
                  continueKeyRequest &= keyDecryptionSegmentFragment->GetHttpDownloadRequest()->SetUrl(url);
                }
                FREE_MEM(url);

                if (continueKeyRequest)
                {
                  continueKeyRequest &= context->GetSegmentsFragments()->Add(keyDecryptionSegmentFragment);

                  if (!continueKeyRequest)
                  {
                    FREE_MEM_CLASS(keyDecryptionSegmentFragment);
                  }
                }
              }

              if (continueKeyRequest)
              {
                context->SetSegmentFragmentToDownload(context->GetSegmentsFragments()->Count() - 1);
                context->SetForceDownload(true);
                this->keyRequestPending = true;
                result = S_FALSE;
              }
            }
          }
        }
      }

      FREE_MEM_CLASS(parsedMediaDataBox);

      if (SUCCEEDED(result) && (!this->keyRequestPending))
      {
        CLinearBuffer *encryptedMediaDataBoxPayload = this->GetMediaDataBox(segmentFragment);
        CHECK_POINTER_HRESULT(result, encryptedMediaDataBoxPayload, result, E_FAIL);

        if (SUCCEEDED(result))
        {
          // we have media data box payload
          // we can parse it to akamai FLV packets and decrypt them

          CEncryptedDataCollection *encryptedDataCollection = new CEncryptedDataCollection();
          CHECK_POINTER_HRESULT(result, encryptedDataCollection, result, E_OUTOFMEMORY);

          while (SUCCEEDED(result) && (encryptedMediaDataBoxPayload->GetBufferOccupiedSpace() != 0))
          {
            CAkamaiFlvPacket *akamaiFlvPacket = this->GetAkamaiFlvPacket(encryptedMediaDataBoxPayload);
            CHECK_POINTER_HRESULT(result, akamaiFlvPacket, result, E_FAIL);

            if (SUCCEEDED(result))
            {
              // we have akamai FLV packet, decrypt content
              // FLV packet content is 0x0F bytes smaller then whole FLV packet size
              // akamai content is from 11th byte of FLV packet
              unsigned int contentSize = akamaiFlvPacket->GetSize() - 0x0F;
              ALLOC_MEM_DEFINE_SET(packetBuffer, uint8_t, contentSize, 0);
              CHECK_POINTER_HRESULT(result, packetBuffer, result, E_OUTOFMEMORY);

              if (SUCCEEDED(result))
              {
                memcpy(packetBuffer, akamaiFlvPacket->GetData() + 11, contentSize);

                // decryptor needs data and key in BASE64 encoding
                // key in BASE64 encoding is in this->lastKeyBase64
                wchar_t *encryptedPacketBuffer = NULL;
                unsigned int encryptedPacketBufferLength = 0;
                result = base64_encode(packetBuffer, contentSize, &encryptedPacketBuffer, &encryptedPacketBufferLength);

                if (SUCCEEDED(result))
                {
                  result = encryptedDataCollection->Add(encryptedPacketBuffer, encryptedPacketBufferLength, akamaiFlvPacket) ? S_OK : E_FAIL;
                }

                FREE_MEM(encryptedPacketBuffer);
              }

              FREE_MEM(packetBuffer);

              // remove processed akamai FLV packet from media data box
              encryptedMediaDataBoxPayload->RemoveFromBuffer(akamaiFlvPacket->GetSize());
              // continue processing with next akamai FLV packet
            }
          }

          // call decryption methods
          CDecryptedDataCollection *decryptedDataCollection = this->akamaiFlashInstance->GetDecryptedData(this->lastKeyBase64, encryptedDataCollection);

          if (encryptedDataCollection->Count() != decryptedDataCollection->Count())
          {
            this->logger->Log(LOGGER_ERROR, L"%s: %s: decrypted data not equal to encrypted data, decrypted: %u, encrypted: %u", PLUGIN_IMPLEMENTATION_NAME, METHOD_PROCESS_SEGMENTS_AND_FRAGMENTS_NAME, decryptedDataCollection->Count(), encryptedDataCollection->Count());
            result = E_FAIL;
          }

          // get total length of result media data box
          unsigned int decryptedMediaDataBoxSize = 0;
          if (SUCCEEDED(result))
          {
            for (unsigned int i = 0; (SUCCEEDED(result) && (i < decryptedDataCollection->Count())); i++)
            {
              CDecryptedData *decryptedData = decryptedDataCollection->GetItem(i);

              decryptedMediaDataBoxSize += decryptedData->GetDecryptedLength() + 0x0F;

              switch(decryptedData->GetErrorCode())
              {
              case AKAMAI_DECRYPTOR_ERROR_CODE_SUCCESS:
                // everything correct
                break;
              default:
                // unknown error code, log it and return error
                this->logger->Log(LOGGER_INFO, L"%s: %s: unknown error code: %u, error: '%s'", PLUGIN_IMPLEMENTATION_NAME, METHOD_PROCESS_SEGMENTS_AND_FRAGMENTS_NAME, decryptedData->GetErrorCode(), decryptedData->GetError());
                result = E_FAIL;
                break;
              }
            }
          }

          if (SUCCEEDED(result))
          {
            // allocate enough memory to hold payload of media data box
            ALLOC_MEM_DEFINE_SET(decryptedMediaDataBoxPayload, uint8_t, decryptedMediaDataBoxSize, 0);
            CHECK_POINTER_HRESULT(result, decryptedMediaDataBoxPayload, result, E_OUTOFMEMORY);

            if (SUCCEEDED(result))
            {
              unsigned int position = 0;
              // from each decrypted data create FLV packet and add it to payload of media data box

              for (unsigned int i = 0; (SUCCEEDED(result) && (i < decryptedDataCollection->Count())); i++)
              {
                CDecryptedData *decryptedData = decryptedDataCollection->GetItem(i);
                CEncryptedData *encryptedData = encryptedDataCollection->GetItem(i);
                
                const uint8_t *akamaiFlvPacketData = encryptedData->GetAkamaiFlvPacket()->GetData();
                unsigned int tempTimestamp = akamaiFlvPacketData[7] & 0x000000FF;
                tempTimestamp <<= 8;
                tempTimestamp |= akamaiFlvPacketData[4] & 0x000000FF;
                tempTimestamp <<= 8;
                tempTimestamp |= akamaiFlvPacketData[5] & 0x000000FF;
                tempTimestamp <<= 8;
                tempTimestamp |= akamaiFlvPacketData[6] & 0x000000FF;

                // next commented part was causing error when seeking to start of video
                /*if ((tempTimestamp == this->lastTimestamp) || ((tempTimestamp == 0) && (this->lastTimestamp > 0)))
                {
                  this->lastTimestamp += 5;
                  tempTimestamp = this->lastTimestamp;
                }
                else*/
                {
                  this->lastTimestamp = tempTimestamp;
                }
                // in this->lastTimestamp is FLV packet timestamp

                CFlvPacket *decryptedFlvPacket = new CFlvPacket();
                CHECK_POINTER_HRESULT(result, decryptedFlvPacket, result, E_OUTOFMEMORY);

                if (SUCCEEDED(result))
                {
                  result = (decryptedFlvPacket->CreatePacket(encryptedData->GetAkamaiFlvPacket()->GetType(),
                    decryptedData->GetDecryptedData(), decryptedData->GetDecryptedLength(), this->lastTimestamp, false)) ? result : E_FAIL;
                }

                if (SUCCEEDED(result))
                {
                  memcpy(decryptedMediaDataBoxPayload + position, decryptedFlvPacket->GetData(), decryptedFlvPacket->GetSize());
                  position += decryptedFlvPacket->GetSize();
                }

                FREE_MEM_CLASS(decryptedFlvPacket);
              }
            }

            CMediaDataBox *decryptedMediaDataBox = new CMediaDataBox();
            CHECK_POINTER_HRESULT(result, decryptedMediaDataBox, result, E_OUTOFMEMORY);

            if (SUCCEEDED(result))
            {
              result = (decryptedMediaDataBox->SetPayload(decryptedMediaDataBoxPayload, decryptedMediaDataBoxSize)) ? result : E_FAIL;
            }
            if (SUCCEEDED(result))
            {
              // get total length of new received data (all boxes except media data box + new media data box)
              unsigned int decryptedDataSize = (uint32_t)decryptedMediaDataBox->GetSize();
              for (unsigned int i = 0; (SUCCEEDED(result) && (i < boxes->Count())); i++)
              {
                if (wcscmp(boxes->GetItem(i)->GetType(), MEDIA_DATA_BOX_TYPE) != 0)
                {
                  decryptedDataSize += (uint32_t)boxes->GetItem(i)->GetSize();
                }
              }

              // clear received data buffer and allocate new memory space
              CLinearBuffer *receivedData = segmentFragment->GetHttpDownloadResponse()->GetReceivedData()->Clone();
              CHECK_POINTER_HRESULT(result, receivedData, result, E_OUTOFMEMORY);

              if (SUCCEEDED(result))
              {
                segmentFragment->GetHttpDownloadResponse()->GetReceivedData()->DeleteBuffer();
                result = (segmentFragment->GetHttpDownloadResponse()->GetReceivedData()->InitializeBuffer(decryptedDataSize)) ? result : E_OUTOFMEMORY;

                if (SUCCEEDED(result))
                {
                  unsigned int position = 0;
                  for (unsigned int i = 0; (SUCCEEDED(result) && (i < boxes->Count())); i++)
                  {
                    CBox *box = boxes->GetItem(i);
                    if (wcscmp(box->GetType(), MEDIA_DATA_BOX_TYPE) != 0)
                    {
                      // another box than media data box, just copy it
                      ALLOC_MEM_DEFINE_SET(buffer, uint8_t, ((uint32_t)box->GetSize()), 0);
                      CHECK_POINTER_HRESULT(result, buffer, result, E_OUTOFMEMORY);

                      if (SUCCEEDED(result))
                      {
                        result = (receivedData->CopyFromBuffer(buffer, (uint32_t)box->GetSize(), 0, position)) ? result : E_FAIL;
                      }
                      if (SUCCEEDED(result))
                      {
                        result = (segmentFragment->GetHttpDownloadResponse()->GetReceivedData()->AddToBufferWithResize(buffer, (uint32_t)box->GetSize()) == (uint32_t)box->GetSize()) ? result : E_FAIL;
                      }

                      FREE_MEM(buffer);
                    }
                    else
                    {
                      // media data box, replace it with decrypted media data box
                      ALLOC_MEM_DEFINE_SET(buffer, uint8_t, ((uint32_t)decryptedMediaDataBox->GetSize()), 0);
                      CHECK_POINTER_HRESULT(result, buffer, result, E_OUTOFMEMORY);

                      if (SUCCEEDED(result))
                      {
                        result = (decryptedMediaDataBox->GetBox(buffer, (uint32_t)decryptedMediaDataBox->GetSize())) ? result : E_FAIL;
                      }
                      if (SUCCEEDED(result))
                      {
                        result = (segmentFragment->GetHttpDownloadResponse()->GetReceivedData()->AddToBufferWithResize(buffer, (uint32_t)decryptedMediaDataBox->GetSize()) == (uint32_t)decryptedMediaDataBox->GetSize()) ? result : E_FAIL;
                      }

                      FREE_MEM(buffer);
                    }

                    position += (uint32_t)box->GetSize();
                  }
                }
              }

              FREE_MEM_CLASS(receivedData);
            }

            FREE_MEM_CLASS(decryptedMediaDataBox);
            FREE_MEM(decryptedMediaDataBoxPayload);
          }

          FREE_MEM_CLASS(encryptedDataCollection);
          FREE_MEM_CLASS(decryptedDataCollection);
        }

        FREE_MEM_CLASS(encryptedMediaDataBoxPayload);
      }

      // release all boxes
      FREE_MEM_CLASS(boxes);

      segmentFragment->SetProcessed(SUCCEEDED(result) && (!this->keyRequestPending));
    }
  }

  this->logger->Log(LOGGER_INFO, SUCCEEDED(result) ? METHOD_END_FORMAT : METHOD_END_FAIL_HRESULT_FORMAT, PLUGIN_IMPLEMENTATION_NAME, METHOD_PROCESS_SEGMENTS_AND_FRAGMENTS_NAME, result);
  return result;
}

// IAfhsDecryptionPlugin interface

CParsedMediaDataBox *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::ParseMediaDataBox(CAfhsDecryptionContext *context, CSegmentFragment *segmentFragment)
{
  CParsedMediaDataBox *result = new CParsedMediaDataBox();
  if (result != NULL)
  {
    bool continueParsing = true;

    int index = IndexOf(context->GetManifestUrl(), AKAMAI_GUID_URL_PART);
    if (index != (-1))
    {
      wchar_t *akamaiGuid = Substring(context->GetManifestUrl(), index + AKAMAI_GUID_URL_PART_LENGTH);
      continueParsing &= (akamaiGuid != NULL);
      if (continueParsing)
      {
        continueParsing &= result->SetAkamaiGuid(akamaiGuid);
      }
      FREE_MEM(akamaiGuid);
    }

    if (continueParsing)
    {
      // parse segment and fragment for media data box
      CLinearBuffer *mediaDataBox = this->GetMediaDataBox(segmentFragment);
      continueParsing &= (mediaDataBox != NULL);
      if (continueParsing)
      {
        result->SetMediaDataBox(true);

        while (continueParsing && (mediaDataBox->GetBufferOccupiedSpace() != 0))
        {
          // parse media data box for known akamai pattern
          CAkamaiFlvPacket *flvPacket = this->GetAkamaiFlvPacket(mediaDataBox);
          continueParsing &= (flvPacket != NULL);
          if (continueParsing)
          {
            continueParsing &= result->GetAkamaiFlvPackets()->Add(flvPacket);

            mediaDataBox->RemoveFromBuffer(flvPacket->GetSize());

            if (!continueParsing)
            {
              FREE_MEM_CLASS(flvPacket);
            }
          }
        }
      }

      FREE_MEM_CLASS(mediaDataBox);
    }

    if (!continueParsing)
    {
      FREE_MEM_CLASS(result);
    }
  }
  return result;
}

wchar_t *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetDecryptionKeyFromSegmentFragment(CSegmentFragment *segmentFragment)
{
  wchar_t *result = NULL;

  bool continueExtracting = (segmentFragment->GetHttpDownloadResponse()->GetResultCode() == CURLE_OK);
  if (continueExtracting)
  {
    // extract key, remove key segment and fragment and set appropriate segment and fragment to download
    unsigned int lastKeyUsedLength = 0;

    lastKeyUsedLength = segmentFragment->GetHttpDownloadResponse()->GetReceivedData()->GetBufferOccupiedSpace();
    continueExtracting &= (lastKeyUsedLength != 0);

    if (continueExtracting)
    {
      ALLOC_MEM_DEFINE_SET(lastKeyUsed, uint8_t, lastKeyUsedLength, 0);
      continueExtracting &= (lastKeyUsed != NULL);

      if (continueExtracting)
      {
        continueExtracting &= (segmentFragment->GetHttpDownloadResponse()->GetReceivedData()->CopyFromBuffer(lastKeyUsed, lastKeyUsedLength, 0, 0) == lastKeyUsedLength);

        if (continueExtracting)
        {
          // convert key to BASE64 encoding

          continueExtracting &= SUCCEEDED(base64_encode(lastKeyUsed, lastKeyUsedLength, &result));
        }
      }
      FREE_MEM(lastKeyUsed);
    }
  }

  return result;
}

DecryptionResult CMPUrlSourceSplitter_Afhs_Decryption_Akamai::Supported(CAfhsDecryptionContext *context)
{
  DecryptionResult result = DecryptionResult_NotKnown;

  if (this->receivedDataAnalysed && this->keyRequestPending)
  {
    // data were analysed and key request is pending
    result = DecryptionResult_Pending;
    
    CSegmentFragment *keySegmentFragment = context->GetSegmentsFragments()->GetItem(context->GetSegmentsFragments()->Count() - 1);
    if (keySegmentFragment->IsDownloaded())
    {
      // key request is on the end of segments and fragments collection
      // check status of download
      // if success then extract key, in another case report error

      FREE_MEM(this->lastKeyBase64);
      this->lastKeyBase64 = this->GetDecryptionKeyFromSegmentFragment(keySegmentFragment);

      if (this->lastKeyBase64 == NULL)
      {
        FREE_MEM(this->lastKeyBase64);
        result = DecryptionResult_Error;
      }
      else
      {
        context->SetSegmentFragmentToDownload(1);
        this->receivedDataCanBeDecrypted = true;
        result = DecryptionResult_Known;
      }

      context->GetSegmentsFragments()->Remove(context->GetSegmentsFragments()->Count() - 1);
      this->keyRequestPending = false;
    }
  }

  // first check segments and fragments for known akamai pattern in received data
  if (!this->receivedDataAnalysed)
  {
    // data are analysed
    this->receivedDataAnalysed = true;
    // by default we have error
    result = DecryptionResult_Error;

    CParsedMediaDataBox *parsedMediaDataBox = this->ParseMediaDataBox(context, context->GetSegmentsFragments()->GetItem(0));
    if (parsedMediaDataBox != NULL)
    {
      if ((parsedMediaDataBox->IsMediaDataBox()) && (parsedMediaDataBox->GetAkamaiGuid() != NULL) && (parsedMediaDataBox->GetAkamaiFlvPackets()->Count() > 0))
      {
        // it is media data box
        // specified akamai GUID
        // at least one akamai FLV packet

        FREE_MEM(this->akamaiGuid);
        this->akamaiGuid = Duplicate(parsedMediaDataBox->GetAkamaiGuid());
        if (this->akamaiGuid != NULL)
        {
          // key url have to be in first FLV packet
          CAkamaiFlvPacket *packet = parsedMediaDataBox->GetAkamaiFlvPackets()->GetItem(0);

          if (packet->HasKey())
          {
            if ((CompareWithNull(this->lastKeyUrl, packet->GetKeyUrl()) != 0) || (this->lastKeyBase64 == NULL))
            {
              // download new decryption key from url
              CSegmentFragment *keyDecryptionSegmentFragment = new CSegmentFragment(UINT_MAX, UINT_MAX, UINT64_MAX);
              bool continueKeyRequest = (keyDecryptionSegmentFragment != NULL);

              if (continueKeyRequest)
              {
                FREE_MEM(this->lastKeyUrl);
                FREE_MEM(this->sessionID);
                this->sessionID = Duplicate(packet->GetSessionId());
                this->lastKeyUrl = Duplicate(packet->GetKeyUrl());

                wchar_t *url = this->GetKeyUrlFromUrl(context->GetSegmentsFragments()->GetItem(0)->GetHttpDownloadRequest()->GetUrl(), packet->GetKeyUrl(), this->akamaiGuid);                
                continueKeyRequest &= (url != NULL) && (this->sessionID != NULL) && (this->lastKeyUrl != NULL);
                if (continueKeyRequest)
                {
                  continueKeyRequest &= keyDecryptionSegmentFragment->GetHttpDownloadRequest()->SetUrl(url);
                }
                FREE_MEM(url);

                if (continueKeyRequest)
                {
                  continueKeyRequest &= context->GetSegmentsFragments()->Add(keyDecryptionSegmentFragment);

                  if (!continueKeyRequest)
                  {
                    FREE_MEM_CLASS(keyDecryptionSegmentFragment);
                  }
                }
              }

              if (continueKeyRequest)
              {
                context->SetSegmentFragmentToDownload(context->GetSegmentsFragments()->Count() - 1);
                context->SetForceDownload(true);
                result = DecryptionResult_Pending;
                this->keyRequestPending = true;
              }
            }
          }
        }
      }

    }
    FREE_MEM_CLASS(parsedMediaDataBox);
  }

  // check if we have valid and initialized flash instance
  if ((this->receivedDataAnalysed) && (this->receivedDataCanBeDecrypted) && (this->initializeAkamaiFlashInstanceResult == E_NOT_VALID_STATE))
  {
    // flash instance is not initialized
    CLinearBuffer *akamaiResource = this->GetResource(L"AKAMAI_DECRYPTOR", L"DATA");
    bool savedFile = false;
    if (akamaiResource != NULL)
    {
      // save akamai decryptor to filesystem and create flash instance
      this->akamaiSwfFile = this->GetAkamaiSwfFile();
      if (this->akamaiSwfFile != NULL)
      {
        // open or create file
        HANDLE hAkamaiSwfFile = CreateFile(this->akamaiSwfFile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);

        if (hAkamaiSwfFile != INVALID_HANDLE_VALUE)
        {
          ALLOC_MEM_DEFINE_SET(buffer, uint8_t, akamaiResource->GetBufferOccupiedSpace(), 0);
          if (buffer != NULL)
          {
            if (akamaiResource->CopyFromBuffer(buffer, akamaiResource->GetBufferOccupiedSpace(), 0, 0) == akamaiResource->GetBufferOccupiedSpace())
            {
              // write data to swf file file
              DWORD written = 0;

              if (WriteFile(hAkamaiSwfFile, buffer, akamaiResource->GetBufferOccupiedSpace(), &written, NULL))
              {
                savedFile = (written == akamaiResource->GetBufferOccupiedSpace());
              }
            }
          }
          FREE_MEM(buffer);

          CloseHandle(hAkamaiSwfFile);
          hAkamaiSwfFile = INVALID_HANDLE_VALUE;
        }
      }
    }

    if (!savedFile)
    {
      this->logger->Log(LOGGER_INFO, L"%s: %s: cannot load akamai decryption wrapper", PLUGIN_IMPLEMENTATION_NAME, METHOD_SUPPORTED_NAME);
      result = DecryptionResult_Error;
    }
    FREE_MEM_CLASS(akamaiResource);

    if (savedFile)
    {
      this->akamaiFlashInstance = new CAkamaiFlashInstance(this->logger, PLUGIN_IMPLEMENTATION_NAME, this->akamaiSwfFile);
      this->initializeAkamaiFlashInstanceResult = this->akamaiFlashInstance->Initialize();
      if (FAILED(this->initializeAkamaiFlashInstanceResult))
      {
        this->logger->Log(LOGGER_INFO, L"%s: %s: cannot initialize flash instance, error: 0x%08X", PLUGIN_IMPLEMENTATION_NAME, METHOD_SUPPORTED_NAME, this->initializeAkamaiFlashInstanceResult);
        result = DecryptionResult_Error;
      }
      else
      {
        result = DecryptionResult_Pending;
      }
    }
  }

  if (SUCCEEDED(this->initializeAkamaiFlashInstanceResult) && (this->receivedDataAnalysed) && (this->receivedDataCanBeDecrypted))
  {
    AkamaiDecryptorState state = this->akamaiFlashInstance->GetState();
    if (state == AkamaiDecryptorState_NotInitialized)
    {
      this->akamaiFlashInstance->SetDecryptionModuleUrl(this->configurationParameters->GetValue(PARAMETER_NAME_AFHS_DECRYPTION_AKAMAI_MODULE_URL, true, PARAMETER_NAME_AFHS_DECRYPTION_AKAMAI_MODULE_URL_DEFAULT));
      result = DecryptionResult_Pending;
    }
    else if (state == AkamaiDecryptorState_Pending)
    {
      result = DecryptionResult_Pending;
    }
    else if (state == AkamaiDecryptorState_Error)
    {
      // decryptor in error state, decryption impossible
      this->logger->Log(LOGGER_ERROR, L"%s: %s: decryption plugin '%s' in error state, error: '%s'", PLUGIN_IMPLEMENTATION_NAME, METHOD_SUPPORTED_NAME, this->GetName(), this->akamaiFlashInstance->GetError());
      result = DecryptionResult_Error;
    }
    else if (state == AkamaiDecryptorState_Ready)
    {
      this->logger->Log(LOGGER_ERROR, L"%s: %s: decryption plugin '%s' in ready state", PLUGIN_IMPLEMENTATION_NAME, METHOD_SUPPORTED_NAME, this->GetName());
      result = DecryptionResult_Known;
    }
  }

  return result;
}

CBoxCollection *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetBoxes(CSegmentFragment *segmentFragment)
{
  CBoxCollection *result = new CBoxCollection();
  CLinearBuffer *bufferForBoxProcessing = segmentFragment->GetHttpDownloadResponse()->GetReceivedData();

  CBoxFactory *factory = new CBoxFactory();
  bool continueProcessing = (factory != NULL);

  // for box factory we need buffer, position and length
  uint32_t position = 0;
  uint32_t length = bufferForBoxProcessing->GetBufferOccupiedSpace();

  ALLOC_MEM_DEFINE_SET(buffer, uint8_t, length, 0);
  continueProcessing &= (buffer != NULL);

  if (continueProcessing)
  {
    continueProcessing &= (bufferForBoxProcessing->CopyFromBuffer(buffer, length, 0, 0) == length);
  }

  if (continueProcessing)
  {
    do
    {
      CBox *box = factory->CreateBox(buffer + position, length - position);
      continueProcessing &= (box != NULL);

      if (continueProcessing)
      {
        position += (uint32_t)box->GetSize();
        continueProcessing &= result->Add(box);
      }

      if (!continueProcessing)
      {
        FREE_MEM_CLASS(box);
      }
    } while ((continueProcessing) && (position != length));
  }

  FREE_MEM(buffer);
  FREE_MEM_CLASS(factory);

  if (!continueProcessing)
  {
    FREE_MEM_CLASS(result);
  }

  return result;
}

CLinearBuffer *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetMediaDataBox(CSegmentFragment *segmentFragment)
{
  CLinearBuffer *result = NULL;
  CBoxCollection *boxes = this->GetBoxes(segmentFragment);
  if (boxes != NULL)
  {
    // find media data box
    CBox *box = boxes->GetBox(MEDIA_DATA_BOX_TYPE, true);
    if (box != NULL)
    {
      CMediaDataBox *mediaDataBox = dynamic_cast<CMediaDataBox *>(box);
      if (mediaDataBox != NULL)
      {
        unsigned int payloadSize = (unsigned int)mediaDataBox->GetPayloadSize();
        result = new CLinearBuffer();
        bool continueProcessing = (result != NULL);

        if (continueProcessing)
        {
          continueProcessing &= (result->AddToBufferWithResize(mediaDataBox->GetPayload(), payloadSize) == payloadSize);

          if (!continueProcessing)
          {
            FREE_MEM_CLASS(result);
          }
        }                    
      }
    }
  }
  FREE_MEM_CLASS(boxes);
  return result;
}

CAkamaiFlvPacket *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetAkamaiFlvPacket(CLinearBuffer *buffer)
{
  CAkamaiFlvPacket *flvPacket = new CAkamaiFlvPacket();
  if (flvPacket != NULL)
  {
    if (flvPacket->ParsePacket(buffer))
    {
      if (!flvPacket->IsAkamaiFlvPacket())
      {
        FREE_MEM_CLASS(flvPacket);
      }
    }
    else
    {
      FREE_MEM_CLASS(flvPacket);
    }
  }

  return flvPacket;
}

wchar_t *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetAkamaiGuid(void)
{
  ALLOC_MEM_DEFINE_SET(temp, char, (AKAMAI_GUID_LENGTH + 1), 0);
  if (temp != NULL)
  {
    for (unsigned int i = 0; i < AKAMAI_GUID_LENGTH; i++)
    {
      temp[i] = 65 + (rand() % 25);
    }
  }
  wchar_t *result = ConvertToUnicodeA(temp);
  FREE_MEM(temp);
  return result;
}

wchar_t *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetKeyUrlFromUrl(const wchar_t *segmentFragmentUrl, const wchar_t *packetUrl, const wchar_t *akamaiGuid)
{
  wchar_t *result = NULL;

  int index = IndexOf(packetUrl, L"://");
  if (index == (-1))
  {
    // no host in packet url
    wchar_t *host = GetHost(segmentFragmentUrl);
    if (host != NULL)
    {
      result = FormatAbsoluteUrl(host, packetUrl);
    }
    FREE_MEM(host);
  }
  else
  {
    // host in packet url
    result = Duplicate(packetUrl);
  }

  // add to result akamai GUID
  wchar_t *tempResult = FormatString(L"%s?guid=%s", result, akamaiGuid);
  FREE_MEM(result);
  result = tempResult;

  return result;
}

CLinearBuffer *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetResource(const wchar_t *name, const wchar_t *type)
{
  bool correct = false;
  CLinearBuffer *result = NULL;
  HMODULE module = GetModuleHandle(L"MPUrlSourceSplitter_Afhs_Decryption_Akamai.dll");
  HRSRC resourceBlockHandle = FindResource(module, name, type);

  if (resourceBlockHandle != NULL)
  {
    HGLOBAL dataHandle = LoadResource(module, resourceBlockHandle);
    if (dataHandle != NULL)
    {
      DWORD size = SizeofResource(module, resourceBlockHandle);
      if (size != 0)
      {
        LPVOID data = LockResource(dataHandle);
        if (data != NULL)
        {
          result = new CLinearBuffer();
          if (result != NULL)
          {
            if (result->InitializeBuffer(size))
            {
              correct = (result->AddToBuffer((const unsigned char*)data, size) == size);
            }
          }
        }
      }
    }
  }

  if (!correct)
  {
    FREE_MEM_CLASS(result);
  }

  return result;
}

wchar_t *CMPUrlSourceSplitter_Afhs_Decryption_Akamai::GetAkamaiSwfFile(void)
{
  wchar_t *result = NULL;
  wchar_t *folder = GetStoreFilePath(this->configurationParameters);

  if (folder != NULL)
  {
    GUID temp = GUID_NULL;
    if (SUCCEEDED(CoCreateGuid(&temp)))
    {
      wchar_t *guid = ConvertGuidToString(temp);
      if (guid != NULL)
      {
        result = FormatString(L"%smpurlsourcesplitter_afhs_decryption_akamai_%s.swf", folder, guid);
      }
      FREE_MEM(guid);
    }
  }

  FREE_MEM(folder);
  return result;
}
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

#include "ProtocolHoster.h"

CProtocolHoster::CProtocolHoster(CLogger *logger, CParameterCollection *configuration, IOutputStream *outputStream)
  : COutputStreamHoster(logger, configuration, L"ProtocolHoster", L"mpurlsourcesplitter_protocol_*.dll", outputStream)
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, MODULE_PROTOCOL_HOSTER_NAME, METHOD_CONSTRUCTOR_NAME);

  this->activeProtocol = NULL;

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, MODULE_PROTOCOL_HOSTER_NAME, METHOD_CONSTRUCTOR_NAME);
}

CProtocolHoster::~CProtocolHoster(void)
{
  this->logger->Log(LOGGER_INFO, METHOD_START_FORMAT, MODULE_PROTOCOL_HOSTER_NAME, METHOD_DESTRUCTOR_NAME);

  this->logger->Log(LOGGER_INFO, METHOD_END_FORMAT, MODULE_PROTOCOL_HOSTER_NAME, METHOD_DESTRUCTOR_NAME);
}

// hoster methods
PluginImplementation *CProtocolHoster::AllocatePluginsMemory(unsigned int maxPlugins)
{
  return ALLOC_MEM(ProtocolImplementation, maxPlugins);
}

PluginImplementation *CProtocolHoster::GetPluginImplementation(unsigned int position)
{
  if ((this->pluginImplementations != NULL) && (position < this->pluginImplementationsCount))
  {
    return (((ProtocolImplementation *)this->pluginImplementations) + position);
  }

  return NULL;
}

bool CProtocolHoster::AppendPluginImplementation(HINSTANCE hLibrary, DESTROYPLUGININSTANCE destroyPluginInstance, PIPlugin plugin)
{
  bool result = __super::AppendPluginImplementation(hLibrary, destroyPluginInstance, plugin);
  if (result)
  {
    ProtocolImplementation *implementation = (ProtocolImplementation *)this->GetPluginImplementation(this->pluginImplementationsCount - 1);
    implementation->supported = false;
  }

  return result;
}

void CProtocolHoster::RemovePluginImplementation(void)
{
  __super::RemovePluginImplementation();
}

PluginConfiguration *CProtocolHoster::GetPluginConfiguration(void)
{
  ALLOC_MEM_DEFINE_SET(pluginConfiguration, ProtocolPluginConfiguration, 1, 0);
  if (pluginConfiguration != NULL)
  {
    pluginConfiguration->configuration = this->configuration;
    pluginConfiguration->outputStream = this;
  }

  return pluginConfiguration;
}

// IProtocol interface implementation

bool CProtocolHoster::IsConnected(void)
{
  return (this->activeProtocol != NULL) ? this->activeProtocol->IsConnected() : false;
}

unsigned int CProtocolHoster::GetOpenConnectionMaximumAttempts(void)
{
  return (this->activeProtocol != NULL) ? this->activeProtocol->GetOpenConnectionMaximumAttempts() : UINT_MAX;
}

HRESULT CProtocolHoster::ParseUrl(const CParameterCollection *parameters)
{
  HRESULT retval = (this->pluginImplementationsCount == 0) ? E_NOT_VALID_STATE : S_OK;
  CHECK_POINTER_DEFAULT_HRESULT(retval, parameters);

  if (SUCCEEDED(retval))
  {
    for(unsigned int i = 0; i < this->pluginImplementationsCount; i++)
    {
      ProtocolImplementation *implementation = (ProtocolImplementation *)this->GetPluginImplementation(i);
      if (implementation != NULL)
      {
        IProtocolPlugin *protocol = (IProtocolPlugin *)implementation->pImplementation;

        implementation->supported = (protocol->ParseUrl(parameters) == S_OK);
        if ((implementation->supported) && (this->activeProtocol == NULL))
        {
          // active protocol wasn't set yet
          this->activeProtocol = protocol;
        }

        retval = implementation->supported ? S_OK : retval;
      }
    }
  }

  return retval;
}

void CProtocolHoster::ReceiveData(bool *shouldExit)
{
  if (this->activeProtocol != NULL)
  {
    this->activeProtocol->ReceiveData(shouldExit);
  }
}

// ISimpleProtocol interface implementation

unsigned int CProtocolHoster::GetReceiveDataTimeout(void)
{
  return (this->activeProtocol != NULL) ? this->activeProtocol->GetReceiveDataTimeout() : 0;
}

HRESULT CProtocolHoster::StartReceivingData(const CParameterCollection *parameters)
{
  HRESULT result = S_OK;
  CHECK_POINTER_DEFAULT_HRESULT(result, this->activeProtocol);

  if (SUCCEEDED(result))
  {
    result = this->activeProtocol->StartReceivingData(NULL);
  }

  return result;
}

HRESULT CProtocolHoster::StopReceivingData(void)
{
  // close active protocol connection
  if (this->activeProtocol != NULL)
  {
    if (this->activeProtocol->IsConnected())
    {
      this->activeProtocol->StopReceivingData();
    }
  }

  return S_OK;
}

HRESULT CProtocolHoster::QueryStreamProgress(LONGLONG *total, LONGLONG *current)
{
  return (this->activeProtocol != NULL) ? this->activeProtocol->QueryStreamProgress(total, current) : E_NOT_VALID_STATE;
}
  
HRESULT CProtocolHoster::QueryStreamAvailableLength(CStreamAvailableLength *availableLength)
{
  return (this->activeProtocol != NULL) ? this->activeProtocol->QueryStreamAvailableLength(availableLength) : E_NOT_VALID_STATE;
}

HRESULT CProtocolHoster::ClearSession(void)
{
  if (this->pluginImplementations != NULL)
  {
    for(unsigned int i = 0; i < this->pluginImplementationsCount; i++)
    {
      ProtocolImplementation *protocolImplementation = (ProtocolImplementation *)this->GetPluginImplementation(i);
      this->logger->Log(LOGGER_INFO, L"%s: %s: reseting protocol: %s", this->moduleName, METHOD_CLEAR_SESSION_NAME, protocolImplementation->name);

      if (protocolImplementation->pImplementation != NULL)
      {
        IProtocolPlugin *protocol = (IProtocolPlugin *)protocolImplementation->pImplementation;
        protocol->ClearSession();
      }
    }
  }

  this->activeProtocol = NULL;

  return S_OK;
}

// ISeeking interface implementation

unsigned int CProtocolHoster::GetSeekingCapabilities(void)
{
  return (this->activeProtocol != NULL) ? this->activeProtocol->GetSeekingCapabilities() : SEEKING_METHOD_NONE;
}

int64_t CProtocolHoster::SeekToTime(int64_t time)
{
  return (this->activeProtocol != NULL) ? this->activeProtocol->SeekToTime(time) : (-1);
}

int64_t CProtocolHoster::SeekToPosition(int64_t start, int64_t end)
{
  return (this->activeProtocol != NULL) ? this->activeProtocol->SeekToPosition(start, end) : (-1);
}

void CProtocolHoster::SetSupressData(bool supressData)
{
  if (this->activeProtocol != NULL)
  {
    this->activeProtocol->SetSupressData(supressData);
  }
}

// IPlugin interface implementation

const wchar_t *CProtocolHoster::GetName(void)
{
  return (this->activeProtocol != NULL) ? this->activeProtocol->GetName() : NULL;
}

GUID CProtocolHoster::GetInstanceId(void)
{
  return GUID_NULL;
}

HRESULT CProtocolHoster::Initialize(PluginConfiguration *configuration)
{
  return E_NOTIMPL;
}
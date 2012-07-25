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

#ifndef __MPURLSOURCESPLITTER_PARSER_ASX_DEFINED
#define __MPURLSOURCESPLITTER_PARSER_ASX_DEFINED

#include "MPUrlSourceSplitter_Parser_Asx_Exports.h"
#include "IParserPlugin.h"

#define PARSER_NAME                                               L"ASX"

// This class is exported from the MPUrlSourceSplitter_Parser_Asx.dll
class MPURLSOURCESPLITTER_PARSER_ASX_API CMPUrlSourceSplitter_Parser_Asx : public IParserPlugin
{
public:
  // constructor
  // create instance of CMPUrlSourceSplitter_Parser_Asx class
  CMPUrlSourceSplitter_Parser_Asx(CParameterCollection *configuration);

  // destructor
  ~CMPUrlSourceSplitter_Parser_Asx(void);

  // IParser interface

  // clears current parser session
  // @return : S_OK if successfull
  HRESULT ClearSession(void);

  // parses media packet
  // @param mediaPacket : media packet to parse
  // @return : one of IParserPlugin::ParseResult values
  ParseResult ParseMediaPacket(CMediaPacket *mediaPacket);

  // sets current connection url and parameters
  // @param parameters : the collection of url and connection parameters
  // @return : S_OK if successful
  HRESULT SetConnectionParameters(const CParameterCollection *parameters);

  // gets parser action after parser recognizes pattern in stream
  // @return : one of Action values
  Action GetAction(void);

  // gets (fills) connection url and parameters
  // @param parameters : the collection of url and connection parameters to fill
  // @return : S_OK if successful
  HRESULT GetConnectionParameters(CParameterCollection *parameters);

  // IPlugin interface

  // return reference to null-terminated string which represents plugin name
  // function have to allocate enough memory for plugin name string
  // errors should be logged to log file and returned NULL
  // @return : reference to null-terminated string
  wchar_t *GetName(void);

  // get plugin instance ID
  // @return : GUID, which represents instance identifier or GUID_NULL if error
  GUID GetInstanceId(void);

  // initialize plugin implementation with configuration parameters
  // @param configuration : the reference to additional configuration parameters (created by plugin's hoster class)
  // @return : S_OK if successfull
  HRESULT Initialize(PluginConfiguration *configuration);

protected:
  CLogger *logger;

  // holds connection parameters
  CParameterCollection *connectionParameters;
};

#endif

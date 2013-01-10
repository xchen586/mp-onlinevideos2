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

#ifndef __PARAMETER_COLLECTION_DEFINED
#define __PARAMETER_COLLECTION_DEFINED

#define PARAMETER_NAME_INTERFACE                                              L"Interface"
#define PARAMETER_NAME_URL                                                    L"Url"
#define PARAMETER_NAME_MAX_LOG_SIZE                                           L"MaxLogSize"
#define PARAMETER_NAME_LOG_VERBOSITY                                          L"LogVerbosity"
#define PARAMETER_NAME_MAX_PLUGINS                                            L"MaxPlugins"
#define PARAMETER_NAME_DOWNLOAD_FILE_NAME                                     L"DownloadFileName"
#define PARAMETER_NAME_CACHE_FOLDER                                           L"CacheFolder"

#include "Parameter.h"
#include "Logger.h"
#include "Collection.h"

#include <stdint.h>

class CLogger;

class CParameterCollection : public CCollection<CParameter, const wchar_t *>
{
public:
  CParameterCollection(void);
  ~CParameterCollection(void);

  // test if parameter exists in collection
  // @param name : the name of parameter to find
  // @param invariant : specifies if parameter name shoud be find with invariant casing
  // @return : true if parameter exists, false otherwise
  bool Contains(const wchar_t *name, bool invariant);

  // updates value of parameter in collection
  // if parameter doesn't exist, then is added to collection
  // @param name : the name of parameter to update (add)
  // @param invariant : specifies if parameter name shoud be find with invariant casing
  // @param parameter : new parameter value
  // @return : true if successful, false otherwise
  bool Update(const wchar_t *name, bool invariant, CParameter *parameter);

  // get the parameter from collection with specified index
  // @param index : the index of parameter to find
  // @return : the reference to parameter or NULL if not find
  CParameter *GetParameter(unsigned int index);

  // get the parameter from collection with specified name
  // @param name : the name of parameter to find
  // @param invariant : specifies if parameter name shoud be find with invariant casing
  // @return : the reference to parameter or NULL if not find
  CParameter *GetParameter(const wchar_t *name, bool invariant);

  // get the string value of parameter with specified name
  // @param name : the name of parameter to find
  // @param invariant : specifies if parameter name shoud be find with invariant casing
  // @param defaultValue : the default value to return
  // @return : the value of parameter or default value if not found
  const wchar_t *GetValue(const wchar_t *name, bool invariant, const wchar_t *defaultValue);

  // get the integer value of parameter with specified name
  // @param name : the name of parameter to find
  // @param invariant : specifies if parameter name shoud be find with invariant casing
  // @param defaultValue : the default value to return
  // @return : the value of parameter or default value if not found
  long GetValueLong(const wchar_t *name, bool invariant, long defaultValue);

  // get the unsigned integer value of parameter with specified name
  // @param name : the name of parameter to find
  // @param invariant : specifies if parameter name shoud be find with invariant casing
  // @param defaultValue : the default value to return
  // @return : the value of parameter or default value if not found
  long GetValueUnsignedInt(const wchar_t *name, bool invariant, unsigned int defaultValue);

  // get 64-bit integer value of parameter with specified name
  // @param name : the name of parameter to find
  // @param invariant : specifies if parameter name shoud be find with invariant casing
  // @param defaultValue : the default value to return
  // @return : the value of parameter or default value if not found
  int64_t GetValueInt64(const wchar_t *name, bool invariant, int64_t defaultValue);

  // get the boolean value of parameter with specified name
  // @param name : the name of parameter to find
  // @param invariant : specifies if parameter name shoud be find with invariant casing
  // @param defaultValue : the default value to return
  // @return : the value of parameter or default value if not found
  bool GetValueBool(const wchar_t *name, bool invariant, bool defaultValue);

  // log all parameters to log file
  // @param logger : the logger
  // @param loggerLevel : the logger level of messages
  // @param protocolName : name of protocol calling LogCollection()
  // @param functionName : name of function calling LogCollection()
  void LogCollection(CLogger *logger, unsigned int loggerLevel, const wchar_t *protocolName, const wchar_t *functionName);

  // copies parameter in collection
  // @param parameterName : the name of parameter to copy
  // @param invariant : specifies if parameter name shoud be find with invariant casing
  // @param newParameterName : the name of new parameter to create
  // @return : true if parameter created, false otherwise
  bool CopyParameter(const wchar_t *parameterName, bool invariant, const wchar_t *newParameterName);

protected:
  // compare two item keys
  // @param firstKey : the first item key to compare
  // @param secondKey : the second item key to compare
  // @param context : the reference to user defined context
  // @return : 0 if keys are equal, lower than zero if firstKey is lower than secondKey, greater than zero if firstKey is greater than secondKey
  int CompareItemKeys(const wchar_t *firstKey, const wchar_t *secondKey, void *context);

  // gets key for item
  // @param item : the item to get key
  // @return : the key of item
  const wchar_t *GetKey(CParameter *item);

  // clones specified item
  // @param item : the item to clone
  // @return : deep clone of item or NULL if not implemented
  CParameter *Clone(CParameter *item);
};

#endif
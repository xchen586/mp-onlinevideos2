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

#ifndef __HTTP_CURL_INSTANCE_DEFINED
#define __HTTP_CURL_INSTANCE_DEFINED

#include "CurlInstance.h"
#include "HttpDownloadRequest.h"
#include "HttpDownloadResponse.h"

#define HTTP_VERSION_NONE                                                     0
#define HTTP_VERSION_FORCE_HTTP10                                             1
#define HTTP_VERSION_FORCE_HTTP11                                             2

#define HTTP_VERSION_DEFAULT                                                  HTTP_VERSION_NONE
#define HTTP_IGNORE_CONTENT_LENGTH_DEFAULT                                    false

class CHttpCurlInstance :
  public CCurlInstance
{
public:
  // initializes a new instance of CHttpCurlInstance class
  // @param logger : logger for logging purposes
  // @param mutex : mutex for locking access to receive data buffer
  // @param protocolName : the protocol name instantiating
  // @param instanceName : the name of CURL instance
  CHttpCurlInstance(CLogger *logger, HANDLE mutex, const wchar_t *protocolName, const wchar_t *instanceName);

  ~CHttpCurlInstance(void);

  /* get methods */

  // gets download request
  // @return : download request
  virtual CHttpDownloadRequest *GetHttpDownloadRequest(void);

  // gets download response
  // @return : download respose
  virtual CHttpDownloadResponse *GetHttpDownloadResponse(void);

  // gets download content length
  // @return : download content length or -1 if error or unknown
  virtual double GetDownloadContentLength(void);

  /* set methods */

  /* other methods */

  // initializes CURL instance
  // @param downloadRequest : download request
  // @return : true if successful, false otherwise
  virtual bool Initialize(CDownloadRequest *downloadRequest);

protected:
  // holds HTTP download request
  // never created and never destroyed
  // initialized in constructor by deep cloning
  CHttpDownloadRequest *httpDownloadRequest;

  // holds HTTP download response
  CHttpDownloadResponse *httpDownloadResponse;

  curl_slist *httpHeaders;

  // called when CURL debug message arives
  // @param type : CURL message type
  // @param data : received CURL message data
  virtual void CurlDebug(curl_infotype type, const wchar_t *data);

  // process received data
  // @param buffer : buffer with received data
  // @param length : the length of buffer
  // @return : the length of processed data (lower value than length means error)
  virtual size_t CurlReceiveData(const unsigned char *buffer, size_t length);

  // appends header to HTTP headers
  // @param header : HTTP header to append
  // @return : true if successful, false otherwise
  bool AppendToHeaders(CHttpHeader *header);

  // clears headers
  void ClearHeaders(void);

  // gets new instance of download response
  // @return : new download response or NULL if error
  virtual CDownloadResponse *GetNewDownloadResponse(void);
};

#endif
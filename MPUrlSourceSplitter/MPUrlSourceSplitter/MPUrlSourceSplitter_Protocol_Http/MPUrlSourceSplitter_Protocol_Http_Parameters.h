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

#ifndef __MPURLSOURCESPLITTER_PROTOCOL_HTTP_PARAMETERS_DEFINED
#define __MPURLSOURCESPLITTER_PROTOCOL_HTTP_PARAMETERS_DEFINED

#define PARAMETER_NAME_HTTP_RECEIVE_DATA_TIMEOUT                  L"HttpReceiveDataTimeout"
#define PARAMETER_NAME_HTTP_OPEN_CONNECTION_MAXIMUM_ATTEMPTS      L"HttpOpenConnectionMaximumAttempts"
#define PARAMETER_NAME_HTTP_REFERER                               L"HttpReferer"
#define PARAMETER_NAME_HTTP_USER_AGENT                            L"HttpUserAgent"
#define PARAMETER_NAME_HTTP_COOKIE                                L"HttpCookie"
#define PARAMETER_NAME_HTTP_VERSION                               L"HttpVersion"
#define PARAMETER_NAME_HTTP_IGNORE_CONTENT_LENGTH                 L"HttpIgnoreContentLength"

// we should get data in twenty seconds
#define HTTP_RECEIVE_DATA_TIMEOUT_DEFAULT                         20000
#define HTTP_OPEN_CONNECTION_MAXIMUM_ATTEMPTS_DEFAULT             3

#endif

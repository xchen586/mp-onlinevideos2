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

#ifndef __URL_DEFINED
#define __URL_DEFINED

// gets base URL without last '/'
// @param url : URL to get base url
// @return : base URL or NULL if error
wchar_t *GetBaseUrl(const wchar_t *url);

// tests if URL is absolute
// @param url : URL to test
// @return : true if URL is absolute, false otherwise or if error
bool IsAbsoluteUrl(const wchar_t *url);

// gets absolute URL combined from base URL and relative URL
// if relative URL is absolute, then duplicate of relative URL is returned
// @param baseUrl : base URL for combining, URL have to be without last '/'
// @param relativeUrl : relative URL for combinig
// @return : absolute URL or NULL if error
wchar_t *FormatAbsoluteUrl(const wchar_t *baseUrl, const wchar_t *relativeUrl);

// gets absolute base URL combined from base URL and relative URL
// @param baseUrl : base URL for combining, URL have to be without last '/'
// @param relativeUrl : relative URL for combinig, URL have to be without start '/'
// @return : absolute base URL or NULL if error
wchar_t *FormatAbsoluteBaseUrl(const wchar_t *baseUrl, const wchar_t *relativeUrl);

#endif
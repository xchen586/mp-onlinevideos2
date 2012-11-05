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

#include "StdAfx.h"

#include "HttpHeader.h"

CHttpHeader::CHttpHeader(void)
{
  this->name = NULL;
  this->value = NULL;
}

CHttpHeader::~CHttpHeader(void)
{
  FREE_MEM(this->name);
  FREE_MEM(this->value);
}

/* get methods */

const wchar_t *CHttpHeader::GetName(void)
{
  return this->name;
}

unsigned int CHttpHeader::GetNameLength(void)
{
  return (this->name == NULL) ? UINT_MAX : wcslen(this->name);
}

const wchar_t *CHttpHeader::GetValue(void)
{
  return this->value;
}

unsigned int CHttpHeader::GetValueLength(void)
{
  return (this->value == NULL) ? UINT_MAX : wcslen(this->value);
}

/* set methods */

bool CHttpHeader::SetName(const wchar_t *name)
{
  SET_STRING_RETURN_WITH_NULL(this->name, name);
}

bool CHttpHeader::SetValue(const wchar_t *value)
{
  SET_STRING_RETURN_WITH_NULL(this->value, value);
}

/* other methods */

void CHttpHeader::Clear(void)
{
  FREE_MEM(this->name);
  FREE_MEM(this->value);
}

bool CHttpHeader::IsValid(void)
{
  return ((this->name != NULL) && (this->value != NULL));
}

CHttpHeader *CHttpHeader::Clone(void)
{
  CHttpHeader *result = new CHttpHeader();
  if (result != NULL)
  {
    if (!this->CloneInternal(result))
    {
      FREE_MEM_CLASS(result);
    }
  }
  return result;
}

bool CHttpHeader::CloneInternal(CHttpHeader *clonedHeader)
{
  bool result = true;

  clonedHeader->Clear();
  clonedHeader->name = Duplicate(this->name);
  clonedHeader->value = Duplicate(this->value);

  result &= TEST_STRING_WITH_NULL(this->name, clonedHeader->name);
  result &= TEST_STRING_WITH_NULL(this->value, clonedHeader->value);

  return result;
}
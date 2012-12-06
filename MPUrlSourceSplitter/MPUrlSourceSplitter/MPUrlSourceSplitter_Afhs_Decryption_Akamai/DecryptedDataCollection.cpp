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

#include "DecryptedDataCollection.h"

CDecryptedDataCollection::CDecryptedDataCollection(void)
  : CCollection()
{
}

CDecryptedDataCollection::~CDecryptedDataCollection(void)
{
}

bool CDecryptedDataCollection::Add(uint8_t *decryptedData, unsigned int decryptedLength, uint32_t errorCode, char *error)
{
  CDecryptedData *data = new CDecryptedData();
  bool result = (data != NULL);
  if (result)
  {
    data->SetDecryptedData(decryptedData, decryptedLength);
    data->SetErrorCode(errorCode);
    data->SetError(error);

    if (result)
    {
      result = __super::Add(data);
    }
  }

  if (!result)
  {
    data->SetDecryptedData(NULL, 0);
    data->SetError(NULL);

    FREE_MEM_CLASS(data);
  }
  return result;
}

int CDecryptedDataCollection::CompareItemKeys(const wchar_t *firstKey, const wchar_t *secondKey, void *context)
{
  bool invariant = (*(bool *)context);

  if (invariant)
  {
    return _wcsicmp(firstKey, secondKey);
  }
  else
  {
    return wcscmp(firstKey, secondKey);
  }
}

const wchar_t *CDecryptedDataCollection::GetKey(CDecryptedData *item)
{
  return L"";
}

CDecryptedData *CDecryptedDataCollection::Clone(CDecryptedData *item)
{
  return NULL;
}
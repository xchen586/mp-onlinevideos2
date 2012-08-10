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

#include "DataEntryUrlBox.h"

CDataEntryUrlBox::CDataEntryUrlBox(void)
  : CDataEntryBox()
{
  this->type = Duplicate(DATA_ENTRY_URL_BOX_TYPE);
  this->location = NULL;
}

CDataEntryUrlBox::~CDataEntryUrlBox(void)
{
  FREE_MEM(this->location);
}

/* get methods */

bool CDataEntryUrlBox::GetBox(uint8_t *buffer, uint32_t length)
{
  bool result = __super::GetBox(buffer, length);

  if (result)
  {
    uint32_t position = this->HasExtendedHeader() ? BOX_HEADER_LENGTH_SIZE64 : BOX_HEADER_LENGTH;
  }

  return result;
}

const wchar_t *CDataEntryUrlBox::GetLocation(void)
{
  return this->location;
}

/* set methods */

/* other methods */

bool CDataEntryUrlBox::Parse(const uint8_t *buffer, uint32_t length)
{
  return this->ParseInternal(buffer, length, true);
}

wchar_t *CDataEntryUrlBox::GetParsedHumanReadable(const wchar_t *indent)
{
  wchar_t *result = NULL;
  wchar_t *previousResult = __super::GetParsedHumanReadable(indent);

  if ((previousResult != NULL) && (this->IsParsed()))
  {
    // prepare finally human readable representation
    result = FormatString(
      L"%s\n" \
      L"%sLocation: '%s'"
      ,
      
      previousResult,
      indent, (this->IsSelfContained()) ? L"" : this->GetLocation()

      );
  }

  FREE_MEM(previousResult);

  return result;
}

uint64_t CDataEntryUrlBox::GetBoxSize(void)
{
  return __super::GetBoxSize();
}

bool CDataEntryUrlBox::ParseInternal(const unsigned char *buffer, uint32_t length, bool processAdditionalBoxes)
{
  FREE_MEM(this->location);

  bool result = __super::ParseInternal(buffer, length, false);

  if (result)
  {
    if (wcscmp(this->type, DATA_ENTRY_URL_BOX_TYPE) != 0)
    {
      // incorect box type
      this->parsed = false;
    }
    else
    {
      // box is file type box, parse all values
      uint32_t position = this->HasExtendedHeader() ? FULL_BOX_HEADER_LENGTH_SIZE64 : FULL_BOX_HEADER_LENGTH;
      bool continueParsing = (this->GetSize() <= (uint64_t)length);

      if (!this->IsSelfContained())
      {
        if (continueParsing)
        {
          uint32_t positionAfter = position;
          continueParsing &= SUCCEEDED(this->GetString(buffer, length, position, &this->location, &positionAfter));

          if (continueParsing)
          {
            position = positionAfter;
          }
        }
      }

      if (continueParsing && processAdditionalBoxes)
      {
        this->ProcessAdditionalBoxes(buffer, length, position);
      }

      this->parsed = continueParsing;
    }
  }

  result = this->parsed;

  return result;
}
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

#include "FullBox.h"

CFullBox::CFullBox(void)
  : CBox()
{
  this->version = 0;
  this->flags = 0;
}

CFullBox::~CFullBox(void)
{
}

/* get methods */

uint8_t CFullBox::GetVersion(void)
{
  return this->version;
}

uint32_t CFullBox::GetFlags(void)
{
  return this->flags;
}

bool CFullBox::GetBox(uint8_t **buffer, uint32_t *length)
{
  bool result = __super::GetBox(buffer, length);

  if (result)
  {
    uint32_t position = this->HasExtendedHeader() ? BOX_HEADER_LENGTH_SIZE64 : BOX_HEADER_LENGTH;

    WBE8INC(*buffer, position, this->GetVersion());
    WBE24INC(*buffer, position, this->GetFlags());

    if (!result)
    {
      FREE_MEM(*buffer);
      *length = 0;
    }
  }

  return result;
}

/* set methods */

/* other methods */

bool CFullBox::Parse(const uint8_t *buffer, uint32_t length)
{
  return this->ParseInternal(buffer, length, true);
}

wchar_t *CFullBox::GetParsedHumanReadable(const wchar_t *indent)
{
  wchar_t *result = NULL;
  wchar_t *previousResult = __super::GetParsedHumanReadable(indent);

  if ((previousResult != NULL) && (this->IsParsed()))
  {
    // prepare finally human readable representation
    result = FormatString(
      L"%s\n" \
      L"%sVersion: %u\n" \
      L"%sFlags: 0x%06X",
      
      previousResult,
      indent, this->GetVersion(),
      indent, this->GetFlags()
      );
  }

  FREE_MEM(previousResult);

  return result;
}

uint64_t CFullBox::GetBoxSize(uint64_t size)
{
  return __super::GetBoxSize(size + FULL_BOX_DATA_SIZE);
}

bool CFullBox::ParseInternal(const unsigned char *buffer, uint32_t length, bool processAdditionalBoxes)
{
  this->version = 0;
  this->flags = 0;

  bool result = __super::ParseInternal(buffer, length, false);

  if (result)
  {
    // box is file type box, parse all values
    uint32_t position = this->HasExtendedHeader() ? BOX_HEADER_LENGTH_SIZE64 : BOX_HEADER_LENGTH;
    bool continueParsing = ((position + FULL_BOX_DATA_SIZE) <= min(length, (uint32_t)this->GetSize()));

    if (continueParsing)
    {
      RBE8INC(buffer, position, this->version);
      RBE24INC(buffer, position, this->flags);
    }

    if (continueParsing && processAdditionalBoxes)
    {
      this->ProcessAdditionalBoxes(buffer, length, position);
    }

    this->parsed = continueParsing;
  }

  result = this->parsed;

  return result;
}
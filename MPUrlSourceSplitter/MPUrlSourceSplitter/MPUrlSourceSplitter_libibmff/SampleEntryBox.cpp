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

#include "SampleEntryBox.h"

CSampleEntryBox::CSampleEntryBox(void)
  : CBox()
{
  this->dataReferenceIndex = 0;
}

CSampleEntryBox::~CSampleEntryBox(void)
{
}

/* get methods */

bool CSampleEntryBox::GetBox(uint8_t **buffer, uint32_t *length)
{
  bool result = __super::GetBox(buffer, length);

  if (result)
  {
    uint32_t position = this->HasExtendedHeader() ? BOX_HEADER_LENGTH_SIZE64 : BOX_HEADER_LENGTH;

    if (!result)
    {
      FREE_MEM(*buffer);
      *length = 0;
    }
  }

  return result;
}

uint16_t CSampleEntryBox::GetDataReferenceIndex(void)
{
  return this->dataReferenceIndex;
}

/* set methods */

/* other methods */

bool CSampleEntryBox::Parse(const uint8_t *buffer, uint32_t length)
{
  return this->ParseInternal(buffer, length, true);
}

wchar_t *CSampleEntryBox::GetParsedHumanReadable(const wchar_t *indent)
{
  wchar_t *result = NULL;
  wchar_t *previousResult = __super::GetParsedHumanReadable(indent);

  if ((previousResult != NULL) && (this->IsParsed()))
  {
    // prepare finally human readable representation
    result = FormatString(
      L"%s\n" \
      L"%sData reference index: %u"
      ,
      
      previousResult,
      indent, this->GetDataReferenceIndex()
      );
  }

  FREE_MEM(previousResult);

  return result;
}

uint64_t CSampleEntryBox::GetBoxSize(uint64_t size)
{
  return __super::GetBoxSize(size + SAMPLE_ENTRY_BOX_DATA_SIZE);
}

bool CSampleEntryBox::ParseInternal(const unsigned char *buffer, uint32_t length, bool processAdditionalBoxes)
{
  this->dataReferenceIndex = 0;

  bool result = __super::ParseInternal(buffer, length, false);

  if (result)
  {
    // box is file type box, parse all values
    uint32_t position = this->HasExtendedHeader() ? BOX_HEADER_LENGTH_SIZE64 : BOX_HEADER_LENGTH;
    bool continueParsing = ((position + SAMPLE_ENTRY_BOX_DATA_SIZE) <= min(length, (uint32_t)this->GetSize()));

    if (continueParsing)
    {
      // skip 6 x uint(8) reserved
      position += 6;

      RBE16INC(buffer, position, this->dataReferenceIndex);
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
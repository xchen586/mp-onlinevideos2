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

#include "AudioSampleEntryBox.h"

CAudioSampleEntryBox::CAudioSampleEntryBox(void)
  : CSampleEntryBox()
{
  this->channelCount = 0;
  this->sampleSize = 0;
  this->sampleRate = new CFixedPointNumber(16, 16);
}

CAudioSampleEntryBox::~CAudioSampleEntryBox(void)
{
  FREE_MEM_CLASS(this->sampleRate);
}

/* get methods */

bool CAudioSampleEntryBox::GetBox(uint8_t *buffer, uint32_t length)
{
  bool result = __super::GetBox(buffer, length);

  if (result)
  {
    uint32_t position = this->HasExtendedHeader() ? BOX_HEADER_LENGTH_SIZE64 : BOX_HEADER_LENGTH;
  }

  return result;
}

const wchar_t *CAudioSampleEntryBox::GetCodingName(void)
{
  return this->GetType();
}

uint16_t CAudioSampleEntryBox::GetChannelCount(void)
{
  return this->channelCount;
}

uint16_t CAudioSampleEntryBox::GetSampleSize(void)
{
  return this->sampleSize;
}

CFixedPointNumber *CAudioSampleEntryBox::GetSampleRate(void)
{
  return this->sampleRate;
}

/* set methods */

/* other methods */

bool CAudioSampleEntryBox::Parse(const uint8_t *buffer, uint32_t length)
{
  return this->ParseInternal(buffer, length, true);
}

wchar_t *CAudioSampleEntryBox::GetParsedHumanReadable(const wchar_t *indent)
{
  wchar_t *result = NULL;
  wchar_t *previousResult = __super::GetParsedHumanReadable(indent);

  if ((previousResult != NULL) && (this->IsParsed()))
  {
    // prepare finally human readable representation
    result = FormatString(
      L"%s\n" \
      L"%sCoding name: '%s'\n" \
      L"%sChannel count: %u\n" \
      L"%sSample size: %u\n" \
      L"%sSample rate: %u.%u"
      ,
      
      previousResult,
      indent, this->GetCodingName(),
      indent, this->GetChannelCount(),
      indent, this->GetSampleSize(),
      indent, this->GetSampleRate()->GetIntegerPart(), this->GetSampleRate()->GetFractionPart()
      );
  }

  FREE_MEM(previousResult);

  return result;
}

uint64_t CAudioSampleEntryBox::GetBoxSize(void)
{
  return __super::GetBoxSize();
}

bool CAudioSampleEntryBox::ParseInternal(const unsigned char *buffer, uint32_t length, bool processAdditionalBoxes)
{
  this->dataReferenceIndex = 0;

  bool result = __super::ParseInternal(buffer, length, false);

  if (result)
  {
    // box is file type box, parse all values
    uint32_t position = this->HasExtendedHeader() ? SAMPLE_ENTRY_BOX_HEADER_LENGTH_SIZE64 : SAMPLE_ENTRY_BOX_HEADER_LENGTH;
    bool continueParsing = (this->GetSize() <= (uint64_t)length);

    if (continueParsing)
    {
      // skip 2 x uint(32) reserved
      position += 8;

      RBE16INC(buffer, position, this->channelCount);
      RBE16INC(buffer, position, this->sampleSize);

      // skip 1 x uint(16) pre-defined and 1 x uint(16) reserved
      position += 4;

      continueParsing &= this->sampleRate->SetNumber(RBE32(buffer, position));
      position += 4;
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
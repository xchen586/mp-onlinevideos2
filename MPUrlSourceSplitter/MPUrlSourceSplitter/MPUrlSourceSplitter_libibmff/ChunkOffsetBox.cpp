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

#include "ChunkOffsetBox.h"

CChunkOffsetBox::CChunkOffsetBox(void)
  : CFullBox()
{
  this->type = Duplicate(CHUNK_OFFSET_BOX_TYPE);
  this->chunkOffsets = new CChunkOffsetCollection();
}

CChunkOffsetBox::~CChunkOffsetBox(void)
{
  FREE_MEM_CLASS(this->chunkOffsets);
}

/* get methods */

bool CChunkOffsetBox::GetBox(uint8_t *buffer, uint32_t length)
{
  bool result = __super::GetBox(buffer, length);

  if (result)
  {
    uint32_t position = this->HasExtendedHeader() ? BOX_HEADER_LENGTH_SIZE64 : BOX_HEADER_LENGTH;
  }

  return result;
}

CChunkOffsetCollection *CChunkOffsetBox::GetChunkOffsets(void)
{
  return this->chunkOffsets;
}

/* set methods */

/* other methods */

bool CChunkOffsetBox::Parse(const uint8_t *buffer, uint32_t length)
{
  return this->ParseInternal(buffer, length, true);
}

wchar_t *CChunkOffsetBox::GetParsedHumanReadable(const wchar_t *indent)
{
  wchar_t *result = NULL;
  wchar_t *previousResult = __super::GetParsedHumanReadable(indent);

  if ((previousResult != NULL) && (this->IsParsed()))
  {
    // prepare sample entries collection
    wchar_t *offsets = NULL;
    wchar_t *tempIndent = FormatString(L"%s\t", indent);
    for (unsigned int i = 0; i < this->GetChunkOffsets()->Count(); i++)
    {
      CChunkOffset *chunkOffset = this->GetChunkOffsets()->GetItem(i);
      wchar_t *tempOffsets = FormatString(
        L"%s%s%s%llu",
        (i == 0) ? L"" : offsets,
        (i == 0) ? L"" : L"\n",
        tempIndent,
        chunkOffset->GetChunkOffset());
      FREE_MEM(offsets);

      offsets = tempOffsets;
    }

    // prepare finally human readable representation
    result = FormatString(
      L"%s\n" \
      L"%sChunk offsets:%s" \
      L"%s"
      ,
      
      previousResult,
      indent, (this->GetChunkOffsets()->Count() == 0) ? L"" : L"\n",
      (this->GetChunkOffsets()->Count() == 0) ? L"" : offsets

      );

    FREE_MEM(offsets);
    FREE_MEM(tempIndent);
  }

  FREE_MEM(previousResult);

  return result;
}

uint64_t CChunkOffsetBox::GetBoxSize(void)
{
  return __super::GetBoxSize();
}

bool CChunkOffsetBox::ParseInternal(const unsigned char *buffer, uint32_t length, bool processAdditionalBoxes)
{
  if (this->chunkOffsets != NULL)
  {
    this->chunkOffsets->Clear();
  }

  bool result (this->chunkOffsets != NULL);
  result &= __super::ParseInternal(buffer, length, false);

  if (result)
  {
    if (wcscmp(this->type, CHUNK_OFFSET_BOX_TYPE) != 0)
    {
      // incorect box type
      this->parsed = false;
    }
    else
    {
      // box is file type box, parse all values
      uint32_t position = this->HasExtendedHeader() ? FULL_BOX_HEADER_LENGTH_SIZE64 : FULL_BOX_HEADER_LENGTH;
      bool continueParsing = (this->GetSize() <= (uint64_t)length);

      if (continueParsing)
      {
        RBE32INC_DEFINE(buffer, position, chunkOffsetCount, uint32_t);

        for (uint32_t i = 0; (continueParsing && (i < chunkOffsetCount)); i++)
        {
          CChunkOffset *chunkOffset = new CChunkOffset();
          continueParsing &= (chunkOffset != NULL);

          if (continueParsing)
          {
            chunkOffset->SetChunkOffset(RBE32(buffer, position));
            position += 4;

            continueParsing &= this->chunkOffsets->Add(chunkOffset);
          }

          if (!continueParsing)
          {
            FREE_MEM_CLASS(chunkOffset);
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
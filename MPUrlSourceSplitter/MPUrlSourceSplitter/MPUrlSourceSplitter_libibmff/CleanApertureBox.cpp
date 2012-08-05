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

#include "CleanApertureBox.h"

CCleanApertureBox::CCleanApertureBox(void)
  : CBox()
{
  this->type = Duplicate(CLEAN_APERTURE_BOX_TYPE);
  this->cleanApertureWidthN = 0;
  this->cleanApertureWidthD = 0;
  this->cleanApertureHeightN = 0;
  this->cleanApertureHeightD = 0;
  this->horizOffN = 0;
  this->horizOffD = 0;
  this->vertOffN = 0;
  this->vertOffD = 0;
}

CCleanApertureBox::~CCleanApertureBox(void)
{
}

/* get methods */

bool CCleanApertureBox::GetBox(uint8_t **buffer, uint32_t *length)
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

uint32_t CCleanApertureBox::GetCleanApertureWidthN(void)
{
  return this->cleanApertureWidthN;
}

uint32_t CCleanApertureBox::GetCleanApertureWidthD(void)
{
  return this->cleanApertureWidthD;
}

uint32_t CCleanApertureBox::GetCleanApertureHeightN(void)
{
  return this->cleanApertureHeightN;
}

uint32_t CCleanApertureBox::GetCleanApertureHeightD(void)
{
  return this->cleanApertureHeightD;
}

uint32_t CCleanApertureBox::GetHorizontalOffsetN(void)
{
  return this->horizOffN;
}

uint32_t CCleanApertureBox::GetHorizontalOffsetD(void)
{
  return this->horizOffD;
}

uint32_t CCleanApertureBox::GetVerticalOffsetN(void)
{
  return this->vertOffN;
}

 uint32_t CCleanApertureBox::GetVerticalOffsetD(void)
 {
   return this->vertOffD;
 }

/* set methods */

/* other methods */

bool CCleanApertureBox::Parse(const uint8_t *buffer, uint32_t length)
{
  return this->ParseInternal(buffer, length, true);
}

wchar_t *CCleanApertureBox::GetParsedHumanReadable(const wchar_t *indent)
{
  wchar_t *result = NULL;
  wchar_t *previousResult = __super::GetParsedHumanReadable(indent);

  if ((previousResult != NULL) && (this->IsParsed()))
  {
    // prepare finally human readable representation
    result = FormatString(
      L"%s\n" \
      L"%sClean aperture width N: %u\n" \
      L"%sClean aperture width D: %u\n" \
      L"%sClean aperture height N: %u\n" \
      L"%sClean aperture height D: %u\n" \
      L"%sHorizontal offset N: %u\n" \
      L"%sHorizontal offset D: %u\n" \
      L"%sVetical offset N: %u\n" \
      L"%sVertical offset D: %u"
      ,
      
      previousResult,
      indent, this->GetCleanApertureWidthN(),
      indent, this->GetCleanApertureWidthD(),
      indent, this->GetCleanApertureHeightN(),
      indent, this->GetCleanApertureHeightD(),
      indent, this->GetHorizontalOffsetN(),
      indent, this->GetHorizontalOffsetD(),
      indent, this->GetVerticalOffsetN(),
      indent, this->GetVerticalOffsetD()

      );
  }

  FREE_MEM(previousResult);

  return result;
}

uint64_t CCleanApertureBox::GetBoxSize(uint64_t size)
{
  return __super::GetBoxSize(size);
}

bool CCleanApertureBox::ParseInternal(const unsigned char *buffer, uint32_t length, bool processAdditionalBoxes)
{
  this->cleanApertureWidthN = 0;
  this->cleanApertureWidthD = 0;
  this->cleanApertureHeightN = 0;
  this->cleanApertureHeightD = 0;
  this->horizOffN = 0;
  this->horizOffD = 0;
  this->vertOffN = 0;
  this->vertOffD = 0;

  bool result = __super::ParseInternal(buffer, length, false);

  if (result)
  {
    if (wcscmp(this->type, CLEAN_APERTURE_BOX_TYPE) != 0)
    {
      // incorect box type
      this->parsed = false;
    }
    else
    {
      // box is file type box, parse all values
      uint32_t position = this->HasExtendedHeader() ? BOX_HEADER_LENGTH_SIZE64 : BOX_HEADER_LENGTH;
      bool continueParsing = (this->GetSize() <= (uint64_t)length);

      if (continueParsing)
      {
        RBE32INC(buffer, position, this->cleanApertureWidthN);
        RBE32INC(buffer, position, this->cleanApertureWidthD);
        RBE32INC(buffer, position, this->cleanApertureHeightN);
        RBE32INC(buffer, position, this->cleanApertureHeightD);
        RBE32INC(buffer, position, this->horizOffN);
        RBE32INC(buffer, position, this->horizOffD);
        RBE32INC(buffer, position, this->vertOffN);
        RBE32INC(buffer, position, this->vertOffD);
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
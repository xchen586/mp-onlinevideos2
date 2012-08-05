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

#include "DataReferenceBox.h"
#include "BoxFactory.h"

CDataReferenceBox::CDataReferenceBox(void)
  : CFullBox()
{
  this->type = Duplicate(DATA_REFERENCE_BOX_TYPE);
  this->dataEntryBoxCollection = new CDataEntryBoxCollection();
}

CDataReferenceBox::~CDataReferenceBox(void)
{
  FREE_MEM_CLASS(this->dataEntryBoxCollection);
}

/* get methods */

bool CDataReferenceBox::GetBox(uint8_t **buffer, uint32_t *length)
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

CDataEntryBoxCollection *CDataReferenceBox::GetDataEntryBoxCollection(void)
{
  return this->dataEntryBoxCollection;
}

/* set methods */

/* other methods */

bool CDataReferenceBox::Parse(const uint8_t *buffer, uint32_t length)
{
  return this->ParseInternal(buffer, length, true);
}

wchar_t *CDataReferenceBox::GetParsedHumanReadable(const wchar_t *indent)
{
  wchar_t *result = NULL;
  wchar_t *previousResult = __super::GetParsedHumanReadable(indent);

  if ((previousResult != NULL) && (this->IsParsed()))
  {
    // prepare server entry table
    wchar_t *dataEntryBoxes = NULL;
    wchar_t *tempIndent = FormatString(L"%s\t", indent);
    for (unsigned int i = 0; i < this->GetDataEntryBoxCollection()->Count(); i++)
    {
      CDataEntryBox *dataEntryBox = this->GetDataEntryBoxCollection()->GetItem(i);
      wchar_t *tempDataEntryBoxes = FormatString(
        L"%s%s%s--- data entry box %d start ---\n%s\n%s--- data entry box %d end ---",
        (i == 0) ? L"" : dataEntryBoxes,
        (i == 0) ? L"" : L"\n",
        tempIndent, i + 1,
        dataEntryBox->GetParsedHumanReadable(tempIndent),
        tempIndent, i + 1);
      FREE_MEM(dataEntryBoxes);
      dataEntryBoxes = tempDataEntryBoxes;
    }

    // prepare finally human readable representation
    result = FormatString(
      L"%s\n" \
      L"%sData reference box count: %d" \
      L"%s%s"
      ,
      
      previousResult,
      indent, this->GetDataEntryBoxCollection()->Count(),
      (dataEntryBoxes == NULL) ? L"" : L"\n", (dataEntryBoxes == NULL) ? L"" : dataEntryBoxes

      );

    FREE_MEM(dataEntryBoxes);
    FREE_MEM(tempIndent);
  }

  FREE_MEM(previousResult);

  return result;
}

uint64_t CDataReferenceBox::GetBoxSize(uint64_t size)
{
  return __super::GetBoxSize(size);
}

bool CDataReferenceBox::ParseInternal(const unsigned char *buffer, uint32_t length, bool processAdditionalBoxes)
{
  if (this->dataEntryBoxCollection != NULL)
  {
    this->dataEntryBoxCollection->Clear();
  }

  bool result = (this->dataEntryBoxCollection != NULL);
  result &= __super::ParseInternal(buffer, length, false);

  if (result)
  {
    if (wcscmp(this->type, DATA_REFERENCE_BOX_TYPE) != 0)
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
        CBoxFactory *factory = new CBoxFactory();
        continueParsing &= (factory != NULL);

        if (continueParsing)
        {
          RBE32INC_DEFINE(buffer, position, dataEntryBoxCount, uint32_t);

          for (uint32_t i = 0; (continueParsing && (i < dataEntryBoxCount)); i++)
          {
            CBox *box = factory->CreateBox(buffer + position, length - position);
            continueParsing &= (box != NULL);

            if (continueParsing)
            {
              continueParsing &= ((dynamic_cast<CDataEntryBox *>(box)) != NULL);
              if (continueParsing)
              {
                continueParsing &= this->dataEntryBoxCollection->Add(dynamic_cast<CDataEntryBox *>(box));
                position += (uint32_t)box->GetSize();
              }
            }

            if (!continueParsing)
            {
              FREE_MEM_CLASS(box);
            }
          }
        }

        FREE_MEM_CLASS(factory);
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
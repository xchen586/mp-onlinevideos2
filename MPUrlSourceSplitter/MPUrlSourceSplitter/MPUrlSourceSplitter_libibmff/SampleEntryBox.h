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

#ifndef __SAMPLE_ENTRY_BOX_DEFINED
#define __SAMPLE_ENTRY_BOX_DEFINED

#include "Box.h"

#define SAMPLE_ENTRY_BOX_DATA_SIZE                                            8

#define SAMPLE_ENTRY_BOX_HEADER_LENGTH                                        BOX_HEADER_LENGTH + SAMPLE_ENTRY_BOX_DATA_SIZE
#define SAMPLE_ENTRY_BOX_HEADER_LENGTH_SIZE64                                 BOX_HEADER_LENGTH_SIZE64 + SAMPLE_ENTRY_BOX_DATA_SIZE

class CSampleEntryBox :
  public CBox
{
public:
  // initializes a new instance of CSampleEntryBox class
  CSampleEntryBox(void);

  // destructor
  virtual ~CSampleEntryBox(void);

  /* get methods */

  // gets whole box into buffer (buffer must be allocated before)
  // @param buffer : the buffer for box data
  // @param length : the length of buffer for data
  // @return : true if all data were successfully stored into buffer, false otherwise
  virtual bool GetBox(uint8_t *buffer, uint32_t length);

  // gets whole box size
  // method is called to determine whole box size for storing box into buffer
  // @return : size of box 
  virtual uint64_t GetBoxSize(void);

  // gets the index of the data reference to use to retrieve data associated with samples
  // @return : the index of the data reference to use to retrieve data associated with samples
  virtual uint16_t GetDataReferenceIndex(void);

  /* set methods */

  /* other methods */

  // parses data in buffer
  // @param buffer : buffer with box data for parsing
  // @param length : the length of data in buffer
  // @return : true if parsed successfully, false otherwise
  virtual bool Parse(const uint8_t *buffer, uint32_t length);

  // gets box data in human readable format
  // @param indent : string to insert before each line
  // @return : box data in human readable format or NULL if error
  virtual wchar_t *GetParsedHumanReadable(const wchar_t *indent);

protected:

  // integer that contains the index of the data reference to use to retrieve data associated with samples
  // that use this sample description
  // data references are stored in Data Reference Boxes
  // the index ranges from 1 to the number of data references
  uint16_t dataReferenceIndex;

  // parses data in buffer
  // @param buffer : buffer with box data for parsing
  // @param length : the length of data in buffer
  // @param processAdditionalBoxes : specifies if additional boxes have to be processed
  // @return : true if parsed successfully, false otherwise
  virtual bool ParseInternal(const unsigned char *buffer, uint32_t length, bool processAdditionalBoxes);
};

#endif
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

#include "stdafx.h"

#include "LinearBuffer.h"

CLinearBuffer::CLinearBuffer(void)
{
  this->buffer = NULL;
  this->DeleteBuffer();
}

CLinearBuffer::CLinearBuffer(unsigned int size)
{
  this->buffer = NULL;
  this->DeleteBuffer();

  // create internal buffer
  this->InitializeBuffer(size);
}

CLinearBuffer::~CLinearBuffer(void)
{
  this->DeleteBuffer();
}

CLinearBuffer *CLinearBuffer::Clone(void)
{
  CLinearBuffer *clone = new CLinearBuffer();
  bool success = false;

  success = clone->InitializeBuffer(this->GetBufferSize());

  if (success)
  {
    // we got some data to copy from current instance buffer to clone buffer
    success = (clone->AddToBuffer(this->dataStart, this->GetBufferOccupiedSpace()) == this->GetBufferOccupiedSpace());
  }
  
  if (!success)
  {
    delete clone;
    clone = NULL;
  }

  return clone;
}

bool CLinearBuffer::InitializeBuffer(unsigned int size)
{
  // remove current buffer (if any)
  this->DeleteBuffer();

  this->buffer = ALLOC_MEM(unsigned char, size);

  if (this->buffer != NULL)
  {
    this->bufferSize = size;
    this->ClearBuffer();
  }

  return (this->buffer != NULL);
}

bool CLinearBuffer::InitializeBuffer(unsigned int size, char value)
{
  bool result = this->InitializeBuffer(size);
  if (result)
  {
    memset(this->buffer, value, this->bufferSize);
  }
  return result;
}

void CLinearBuffer::ClearBuffer(void)
{
  this->dataStart = this->buffer;
  this->dataEnd = this->buffer;
}

void CLinearBuffer::DeleteBuffer(void)
{
  FREE_MEM(this->buffer);
  this->bufferSize = 0;
  this->ClearBuffer();
}

unsigned int CLinearBuffer::GetBufferSize()
{
  return this->bufferSize;
}

unsigned int CLinearBuffer::GetBufferFreeSpace()
{
  return ((long)this->bufferSize - (long)this->dataEnd + (long)this->buffer);
}

unsigned int CLinearBuffer::GetBufferOccupiedSpace(void)
{
  return ((long)this->dataEnd - (long)this->dataStart);
}

void CLinearBuffer::RemoveFromBuffer(unsigned int length)
{
  // the length to remove from buffer cannot be greater than occupied space
  length = min(length, this->GetBufferOccupiedSpace());

  if (length > 0)
  {
    if (length == this->GetBufferOccupiedSpace())
    {
      // removing all data from buffer
      this->ClearBuffer();
    }
    else
    {
      this->dataStart += length;
    }
  }
}

void CLinearBuffer::RemoveFromBufferAndMove(unsigned int length)
{
  // the length to remove from buffer cannot be greater than occupied space
  length = min(length, this->GetBufferOccupiedSpace());

  if (length > 0)
  {
    unsigned int occupiedSpace = this->GetBufferOccupiedSpace();
    if (length == occupiedSpace)
    {
      // removing all data from buffer
      this->ClearBuffer();
    }
    else
    {
      this->dataStart += length;

      int remainingDataLength = occupiedSpace - length;
      memmove(this->buffer, this->dataStart, remainingDataLength);
      this->dataStart = this->buffer;
      this->dataEnd = this->buffer + remainingDataLength;
    }
  }
}

unsigned int CLinearBuffer::AddToBuffer(const unsigned char *source, unsigned int length)
{
  unsigned int returnValue = 0;
  if ((length > 0) && (length <= this->GetBufferFreeSpace()))
  {
    memcpy(this->dataEnd, source, length);
    this->dataEnd += length;
    returnValue = length;
  }

  return returnValue;
}

unsigned int CLinearBuffer::AddToBufferWithResize(const unsigned char *source, unsigned int length)
{
  unsigned int returnValue = 0;

  if (this->GetBufferFreeSpace() < length)
  {
    if (this->ResizeBuffer(this->GetBufferSize() + length - this->GetBufferFreeSpace()))
    {
      returnValue = this->AddToBuffer(source, length);
    }
  }
  else
  {
    returnValue = this->AddToBuffer(source, length);
  }

  return returnValue;
}

unsigned int CLinearBuffer::CopyFromBuffer(unsigned char *destination, unsigned int length, unsigned int packetSize)
{
  return this->CopyFromBuffer(destination, length, packetSize, 0);
}

unsigned int CLinearBuffer::CopyFromBuffer(unsigned char *destination, unsigned int length, unsigned int packetSize, unsigned int start)
{
  int copiedBytes = 0;

  // length cannot be greater than buffer occupied space
  length = min(length, this->GetBufferOccupiedSpace() - start);
  if (length > 0)
  {
    if (packetSize > 0)
    {
      copiedBytes = (length / packetSize) * packetSize;
    }
    else
    {
      copiedBytes = length;
    }

    // copy to destination only if length is greater than zero
    if (copiedBytes > 0)
    {
      memcpy(destination, this->dataStart + start, copiedBytes);
    }
  }

  return copiedBytes;
}

unsigned int CLinearBuffer::GetFirstPosition(unsigned int start, char c)
{
  unsigned int result = UINT_MAX;

  for(unsigned int i = start; i < this->GetBufferOccupiedSpace(); i++)
  {
    if (this->buffer[i] == c)
    {
      result = i;
      break;
    }
  }

  return result;
}

bool CLinearBuffer::ResizeBuffer(unsigned int size)
{
  unsigned int occupiedSize = this->GetBufferOccupiedSpace();
  bool result = (size >= occupiedSize);

  if (result)
  {
    // requested buffer size is bigger than current occupied space
    // create new buffer
    unsigned char *tempBuffer = ALLOC_MEM(unsigned char, size);
    result = (tempBuffer != NULL);

    if (result)
    {
      // copy content from current buffer to new buffer
      this->CopyFromBuffer(tempBuffer, occupiedSize, 0);
      // delete current buffer
      this->DeleteBuffer();
      // set new buffer pointer
      this->buffer = tempBuffer;
      // set new start, end of buffer
      this->dataStart = tempBuffer;
      this->dataEnd = tempBuffer + occupiedSize;
      this->bufferSize = size;
    }
  }

  return result;
}
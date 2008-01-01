/*----------------------------------------------------------------------------*/
/*                                                                            */
/* Copyright (c) 1995, 2004 IBM Corporation. All rights reserved.             */
/* Copyright (c) 2005-2006 Rexx Language Association. All rights reserved.    */
/*                                                                            */
/* This program and the accompanying materials are made available under       */
/* the terms of the Common Public License v1.0 which accompanies this         */
/* distribution. A copy is also available at the following address:           */
/* http://www.oorexx.org/license.html                          */
/*                                                                            */
/* Redistribution and use in source and binary forms, with or                 */
/* without modification, are permitted provided that the following            */
/* conditions are met:                                                        */
/*                                                                            */
/* Redistributions of source code must retain the above copyright             */
/* notice, this list of conditions and the following disclaimer.              */
/* Redistributions in binary form must reproduce the above copyright          */
/* notice, this list of conditions and the following disclaimer in            */
/* the documentation and/or other materials provided with the distribution.   */
/*                                                                            */
/* Neither the name of Rexx Language Association nor the names                */
/* of its contributors may be used to endorse or promote products             */
/* derived from this software without specific prior written permission.      */
/*                                                                            */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS        */
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT          */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS          */
/* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT   */
/* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,      */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,        */
/* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY     */
/* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING    */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS         */
/* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/******************************************************************************/
/* REXX Kernel                                        MutableBufferClass.c    */
/*                                                                            */
/* Primitive MutableBuffer Class                                              */
/*                                                                            */
/******************************************************************************/
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "RexxCore.h"
#include "StringClass.hpp"
#include "MutableBufferClass.hpp"
#include "RexxBuiltinFunctions.h"                          /* Gneral purpose BIF Header file       */
#include "ProtectedObject.hpp"
#include "StringUtil.hpp"


// singleton class instance
RexxClass *RexxMutableBuffer::classInstance = OREF_NULL;

#define DEFAULT_BUFFER_LENGTH 256

RexxMutableBuffer *RexxMutableBufferClass::newRexx(RexxObject **args, size_t argc)
/******************************************************************************/
/* Function:  Allocate (and initialize) a string object                       */
/******************************************************************************/
{
  RexxString        *string;
  RexxMutableBuffer *newBuffer;         /* new mutable buffer object         */
  size_t            bufferLength = DEFAULT_BUFFER_LENGTH;
  size_t            defaultSize;
  if (argc >= 1) {
    if (args[0] != NULL) {
                                        /* force argument to string value    */
      string = (RexxString *)get_string(args[0], ARG_ONE);
    }
    else
    {
      string = OREF_NULLSTRING;           /* default to empty content          */
    }
  }
  else                                      /* minimum buffer size given?        */
  {
     string = OREF_NULLSTRING;
  }

  if (argc >= 2)
  {
    bufferLength = optional_length(args[1], DEFAULT_BUFFER_LENGTH, ARG_TWO);
  }

  defaultSize = bufferLength;           /* remember initial default size     */

                                        /* input string longer than demanded */
                                        /* minimum size? expand accordingly  */
  if (string->getLength() > bufferLength)
  {
      bufferLength = string->getLength();
  }
  /* allocate the new object           */
  newBuffer = new ((RexxClass *)this) RexxMutableBuffer(bufferLength, defaultSize);
  newBuffer->dataLength = string->getLength();
  /* copy the content                  */
  newBuffer->data->copyData(0, string->getStringData(), string->getLength());

  ProtectedObject p(newBuffer);
  newBuffer->sendMessage(OREF_INIT, args, argc > 2 ? argc - 2 : 0);
  return newBuffer;
}


/**
 * Default constructor.
 */
RexxMutableBuffer::RexxMutableBuffer()
{
    bufferLength = DEFAULT_BUFFER_LENGTH;   /* save the length of the buffer    */
    defaultSize  = bufferLength;            /* store the default buffer size    */

    data = new_buffer(bufferLength);
}


/**
 * Constructor with explicitly set size and default.
 *
 * @param l      Initial length.
 * @param d      The explicit default size.
 */
RexxMutableBuffer::RexxMutableBuffer(size_t l, size_t d)
{
    bufferLength = l;               /* save the length of the buffer    */
    defaultSize  = d;               /* store the default buffer size    */

    data = new_buffer(bufferLength);
}


/**
 * Create a new mutable buffer object from a potential subclass.
 *
 * @param size   The size of the buffer object.
 *
 * @return A new instance of a mutable buffer, with the default class
 *         behaviour.
 */
void *RexxMutableBuffer::operator new(size_t size)
{
    return new_object(size, T_MutableBuffer);
}

/**
 * Create a new mutable buffer object from a potential subclass.
 *
 * @param size   The size of the buffer object.
 * @param bufferClass
 *               The class of the buffer object.
 *
 * @return A new instance of a mutable buffer, with the target class
 *         behaviour.
 */
void *RexxMutableBuffer::operator new(size_t size, RexxClass *bufferClass)
{
    RexxObject * newObj = new_object(size, T_MutableBuffer);
    newObj->setBehaviour(bufferClass->getInstanceBehaviour());
    return newObj;
}


void RexxMutableBuffer::live(size_t liveMark)
/******************************************************************************/
/* Function:  Normal garbage collection live marking                          */
/******************************************************************************/
{
    memory_mark(this->objectVariables);
    memory_mark(this->data);
}

void RexxMutableBuffer::liveGeneral(int reason)
/******************************************************************************/
/* Function:  Generalized object marking                                      */
/******************************************************************************/
{
    memory_mark_general(this->objectVariables);
    memory_mark_general(this->data);
}


void RexxMutableBuffer::flatten(RexxEnvelope *envelope)
/******************************************************************************/
/* Function:  Flatten a mutable buffer                                        */
/******************************************************************************/
{
  setUpFlatten(RexxMutableBuffer)

  flatten_reference(newThis->data, envelope);
  flatten_reference(newThis->objectVariables, envelope);

  cleanUpFlatten
}

RexxObject *RexxMutableBuffer::copy()
/******************************************************************************/
/* Function:  copy an object                                                  */
/******************************************************************************/
{

    RexxMutableBuffer *newObj = (RexxMutableBuffer *)this->clone();

                                           /* see the comments in ::newRexx()!! */
    newObj->data = new_buffer(bufferLength);
    newObj->dataLength = this->dataLength;
    newObj->data->copyData(0, data->address(), bufferLength);

    newObj->defaultSize = this->defaultSize;
    newObj->bufferLength = this->bufferLength;
    return newObj;
}

void RexxMutableBuffer::ensureCapacity(size_t addedLength)
/******************************************************************************/
/* Function:  append to the mutable buffer                                    */
/******************************************************************************/
{
    size_t resultLength = this->dataLength + addedLength;

    if (resultLength > bufferLength)
    {   /* need to enlarge?                  */
        bufferLength *= 2;                   /* double the buffer                 */
        if (bufferLength < resultLength)
        {   /* still too small? use new length   */
            bufferLength = resultLength;
        }

        RexxBuffer *newBuffer = new_buffer(bufferLength);
        // copy the data into the new buffer
        newBuffer->copyData(0, data->address(), dataLength);
        // replace the old data buffer
        OrefSet(this, this->data, newBuffer);
    }
}


/**
 * Return the length of the data in the buffer currently.
 *
 * @return The current length, as an Integer object.
 */
RexxObject *RexxMutableBuffer::lengthRexx()
{
    return new_integer(dataLength);
}


RexxMutableBuffer *RexxMutableBuffer::append(RexxObject *obj)
/******************************************************************************/
/* Function:  append to the mutable buffer                                    */
/******************************************************************************/
{
    RexxString *string = REQUIRED_STRING(obj, ARG_ONE);
    ProtectedObject p(string);
    // make sure we have enough room
    ensureCapacity(string->getLength());

    data->copyData(dataLength, string->getStringData(), string->getLength());
    this->dataLength += string->getLength();
    return this;
}


RexxMutableBuffer *RexxMutableBuffer::insert(RexxObject *str, RexxObject *pos, RexxObject *len, RexxObject *pad)
/******************************************************************************/
/* Function:  insert string at given position                                 */
/******************************************************************************/
{
    // force this into string form
    RexxString * string = REQUIRED_STRING(str, ARG_ONE);

    // we're using optional length because 0 is valid for insert.
    size_t begin = optionalNonNegative(pos, 0, ARG_TWO);
    size_t insertLength = optional_length(len, string->getLength(), ARG_THREE);

    char padChar = get_pad(pad, ' ', ARG_FOUR);

    // if inserting a zero length string, this is simple!
    if (insertLength == 0)
    {
        return this;                            /* do nothing                   */
    }

    // if inserting within the current bounds, we only need to add the length
    // if inserting beyond the end, we need to make sure we add space for the gap too
    if (begin < dataLength)
    {
        ensureCapacity(insertLength);
    }
    else
    {
        ensureCapacity(insertLength = (begin - dataLength));
    }


    /* create space in the buffer   */
    if (begin < dataLength)
    {
        data->openGap(begin, insertLength, dataLength - begin);
    }
    else if (begin > this->dataLength)
    {
        /* pad before insertion         */
        data->setData(dataLength, padChar, begin - dataLength);
    }
    /* insert string contents       */
    data->copyData(begin, string->getStringData(), string->getLength());
    // do we need data padding?
    if (insertLength > string->getLength())
    {
        data->setData(begin + string->getLength(), padChar, insertLength - string->getLength());
    }
    // inserting after the end? the resulting length is measured from the insertion point
    if (begin > this->dataLength)
    {
        this->dataLength = begin + insertLength;
    }
    else
    {
        // just add in the inserted length
        this->dataLength += insertLength;
    }
    return this;
}


RexxMutableBuffer *RexxMutableBuffer::overlay(RexxObject *str, RexxObject *pos, RexxObject *len, RexxObject *pad)
/******************************************************************************/
/* Function:  replace characters in buffer contents                           */
/******************************************************************************/
{
    RexxString *string = get_string(str, ARG_ONE);
    size_t begin = optional_position(pos, 1, ARG_TWO) - 1;
    size_t replaceLength = optional_length(len, string->getLength(), ARG_THREE);

    char padChar = get_pad(pad, ' ', ARG_FOUR);

    // if nothing to replace, we can just return immediately.
    if (replaceLength == 0)
    {
        return this;
    }

    // make sure we have room for this
    ensureCapacity(begin + replaceLength);

    // is our start position beyond the current data end?
    if (begin > dataLength)
    {
        // add padding to the gap
        data->setData(dataLength, padChar, begin - dataLength);
    }

    // now overlay the string data
    data->copyData(begin, string->getStringData(), replaceLength);
    // do we need additional padding?
    if (replaceLength > string->getLength())
    {
        // pad the section after the overlay
        data->setData(begin + string->getLength(), padChar, replaceLength - string->getLength());
    }

    // did this add to the size?
    if (begin + replaceLength > dataLength)
    {
        //adjust upward
        dataLength = begin + replaceLength;
    }
    return this;
}


RexxMutableBuffer *RexxMutableBuffer::mydelete(RexxObject *_start, RexxObject *len)
/******************************************************************************/
/* Function:  delete character range in buffer                                */
/******************************************************************************/
{
    size_t begin = get_position(_start, ARG_ONE) - 1;
    size_t range = optional_length(len, this->data->getLength() - begin, ARG_TWO);

    // is the begin point actually within the string?
    if (begin < dataLength)
    {           /* got some work to do?         */
        // deleting from the middle?
        if (begin + range < dataLength)
        {
            // shift everything over
            data->closeGap(begin, range, dataLength - (begin + range));
            dataLength -= range;
        }
        else
        {
            // we're just truncating
            dataLength = begin;
        }
    }
    return this;
}


RexxObject *RexxMutableBuffer::setBufferSize(RexxInteger *size)
/******************************************************************************/
/* Function:  set the size of the buffer                                      */
/******************************************************************************/
{
    size_t newsize = get_length(size, ARG_ONE);
    // has a reset to zero been requested?
    if (newsize == 0)
    {
        // have we increased the buffer size?
        if (bufferLength > defaultSize)
        {
            // reallocate the buffer
            OrefSet(this, this->data, new_buffer(defaultSize));
            // reset the size to the default
            bufferLength = defaultSize;
        }
        dataLength = 0;
    }
    // an actual resize?
    else if (newsize != bufferLength)
    {
        // reallocate the buffer
        RexxBuffer *newBuffer = new_buffer(newsize);
        // if we're shrinking this, it truncates.
        dataLength = Numerics::minVal(dataLength, newsize);
        newBuffer->copyData(0, data->address(), dataLength);
        // replace the old buffer
        OrefSet(this, this->data, newBuffer);
        // and update the size....
        bufferLength = newsize;
    }
    return this;
}


RexxString *RexxMutableBuffer::makeString()
/******************************************************************************/
/* Function:  Handle a REQUEST('STRING') request for a mutablebuffer object   */
/******************************************************************************/
{
    return new_string(data->address(), dataLength);
}


/******************************************************************************/
/* Arguments:  String position for substr                                     */
/*             requested length of new string                                 */
/*             pad character to use, if necessary                             */
/*                                                                            */
/*  Returned:  string, sub string of original.                                */
/******************************************************************************/
RexxString *RexxMutableBuffer::substr(RexxInteger *argposition,
                                      RexxInteger *arglength,
                                      RexxString  *pad)
{
    return StringUtil::substr(getStringData(), getLength(), argposition, arglength, pad);
}


RexxInteger *RexxMutableBuffer::posRexx(RexxString  *needle, RexxInteger *pstart)
{
    return StringUtil::posRexx(getStringData(), getLength(), needle, pstart);
}


RexxInteger *RexxMutableBuffer::lastPos(RexxString  *needle, RexxInteger *_start)
{
    return StringUtil::lastPosRexx(getStringData(), getLength(), needle, _start);
}


/**
 * Extract a single character from a string object.
 * Returns a null string if the specified position is
 * beyond the bounds of the string.
 *
 * @param positionArg
 *               The position of the target  character.  Must be a positive
 *               whole number.
 *
 * @return Returns the single character at the target position.
 *         Returns a null string if the position is beyond the end
 *         of the string.
 */
RexxString *RexxMutableBuffer::subchar(RexxInteger *positionArg)
{
    return StringUtil::subchar(getStringData(), getLength(), positionArg);
}


RexxArray *RexxMutableBuffer::makearray(RexxString *div)
/******************************************************************************/
/* Function:  Split string into an array                                      */
/******************************************************************************/
{
    return StringUtil::makearray(getStringData(), getLength(), div);
}

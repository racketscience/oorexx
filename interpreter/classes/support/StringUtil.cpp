/*----------------------------------------------------------------------------*/
/*                                                                            */
/* Copyright (c) 1995, 2004 IBM Corporation. All rights reserved.             */
/* Copyright (c) 2005-2014 Rexx Language Association. All rights reserved.    */
/*                                                                            */
/* This program and the accompanying materials are made available under       */
/* the terms of the Common Public License v1.0 which accompanies this         */
/* distribution. A copy is also available at the following address:           */
/* http://www.oorexx.org/license.html                                         */
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
/* REXX Kernel                                                                */
/*                                                                            */
/* String Utilities shared between String class and MutableBuffer class       */
/*                                                                            */
/******************************************************************************/

#include "RexxCore.h"
#include "StringClass.hpp"
#include "ProtectedObject.hpp"
#include "StringUtil.hpp"
#include "QueueClass.hpp"
#include "MethodArguments.hpp"
#include "NumberStringClass.hpp"


/**
 * Extract a substring from a data buffer.
 *
 * @param string    The data buffer.
 * @param stringLength
 *                  The length of the buffer.
 * @param _position The position argument for the starting position.
 * @param _length   The substring length argument.
 * @param pad       The padding argument.
 *
 * @return The extracted substring.
 */
RexxString *StringUtil::substr(const char *string, size_t stringLength, RexxInteger *_position,
    RexxInteger *_length, RexxString  *pad)
{
    size_t position = positionArgument(_position, ARG_ONE) - 1;
    // assume nothing is pulled from this string
    size_t length = 0;
    // is the position within the string bounds?
    if (stringLength >= position)
    {
        // we extract everything from the position to the end (potentially)
        length = stringLength - position;
    }
    // now we process any overrides on this
    length = optionalLengthArgument(_length, length, ARG_TWO);
    // get a padding character (blank is default)
    char padChar = optionalPadArgument(pad, ' ', ARG_THREE);

    // if our target length is zero, we can just return the null string singleton
    if (length == 0)
    {
        return GlobalNames::NULLSTRING;
    }

    size_t substrLength = 0;
    size_t padCount = 0;

    // starting past the end of the string?
    // this will be all pad characters
    if (position > stringLength)
    {
        padCount = length;
    }
    else
    {
        // we have a combination of source string and pad characters
        substrLength = Numerics::minVal(length, stringLength - position);
        padCount = length - substrLength;
    }
    RexxString *retval = raw_string(length);
    RexxString::StringBuilder builder(retval);

    // copy the substring data
    builder.append(string + position, substrLength);
    // add any needed padding characters
    builder.pad(padChar, padCount);
    // and return the final result
    return retval;
}


/**
 * Extract a substring from a data buffer, using the "[]"
 * semantics
 *
 * @param string    The data buffer.
 * @param stringLength
 *                  The length of the buffer.
 * @param _position The position argument for the starting position.
 * @param _length   The substring length argument.
 *
 * @return The extracted substring.
 */
RexxString *StringUtil::substr(const char *string, size_t stringLength, RexxInteger *_position,
    RexxInteger *_length)
{
    // position is required
    size_t position = positionArgument(_position, ARG_ONE) - 1;
    // The default length is a single character
    size_t length = optionalLengthArgument(_length, 1, ARG_TWO);

    // if our target length is zero, or this is beyond the bounds of
    // the result is a null string
    if (length == 0 || position >= stringLength)
    {
        return GlobalNames::NULLSTRING;
    }

    // need to cap the length to the remainder of the string
    length = Numerics::minVal(length, stringLength - position);

    // just extract a new string from the data
    return new_string(string + position, length);
}


/**
 * Locate a string within the designated string buffer.
 *
 * @param stringData The stringData to search within.
 * @param length     The length of the string data.
 * @param needle     The needle to search for.
 * @param pstart     The starting position.
 * @param range      The length of the string to search within.
 *
 * @return .true if the string is found, .false otherwise.
 */
RexxObject *StringUtil::containsRexx(const char *stringData, size_t length, RexxString *needle, RexxInteger *pstart, RexxInteger *range)
{
    needle = stringArgument(needle, ARG_ONE);
    size_t _start = optionalPositionArgument(pstart, 1, ARG_TWO);
    size_t _range = optionalLengthArgument(range, length - _start + 1, ARG_THREE);

    // pass on to the primitive function and return as a boolean object
    return booleanObject(pos(stringData, length, needle, _start - 1, _range) > 0);
}


/**
 * Test if a given string contains the designated substring
 *
 * @param stringData The stringData to search within.
 * @param length     The length of the string data.
 * @param needle     The needle to search for.
 * @param pstart     The starting position.
 * @param range      The length of the string to search within.
 *
 * @return An integer object giving the located position.
 */
RexxInteger *StringUtil::posRexx(const char *stringData, size_t length, RexxString *needle, RexxInteger *pstart, RexxInteger *range)
{
    needle = stringArgument(needle, ARG_ONE);
    size_t _start = optionalPositionArgument(pstart, 1, ARG_TWO);
    size_t _range = optionalLengthArgument(range, length - _start + 1, ARG_THREE);

    return new_integer(pos(stringData, length, needle, _start - 1, _range));
}


/**
 * Primitive level search withint a string buffer.
 *
 * @param stringData The maystack buffer.
 * @param haystack_length
 *                   The length of the haystack.
 * @param needle     The search needle.
 * @param _start     The starting position.
 *
 * @return The offset of the located needle, or 0 if the needle doesn't exist.
 */
size_t StringUtil::pos(const char *stringData, size_t haystack_length, RexxString *needle, size_t _start, size_t _range)
{
    size_t needle_length = needle->getLength();
    // make sure the range is capped
    _range = Numerics::minVal(_range, haystack_length - _start);

    // ok, there are a few quick checks we can perform.  If the needle is
    // bigger than the haystack, or the needle is a null string or
    // our haystack length after adjusting to the starting position
    // zero, then we can quickly return zero.
    if (_start >= haystack_length || needle_length > _range || needle_length == 0)
    {
        return 0;
    }

    // address the string value
    const char *haypointer = stringData + _start;
    const char *needlepointer = needle->getStringData();
    size_t location = _start + 1;         // this is the match location as an index
    // calculate the number of probes we can make in this string
    size_t count = _range - needle_length + 1;

    // now scan
    while (count--)
    {
        // get a match at this position?  return that location
        if (memcmp(haypointer, needlepointer, needle_length) == 0)
        {
            return location;
        }
        // step our pointers accordingly
        location++;
        haypointer++;
    }
    return 0;  // we got nothing...
}


/**
 * Primitive level search withint a string buffer.
 *
 * @param stringData The maystack buffer.
 * @param haystack_length
 *                   The length of the haystack.
 * @param needle     The search needle.
 * @param _start     The starting position.
 *
 * @return The offset of the located needle, or 0 if the needle doesn't exist.
 */
size_t StringUtil::caselessPos(const char *stringData, size_t haystack_length, RexxString *needle, size_t _start, size_t _range)
{
    // get the two working lengths
    size_t needle_length = needle->getLength();
    // make sure the range is capped
    _range = Numerics::minVal(_range, haystack_length - _start + 1);

    // ok, there are a few quick checks we can perform.  If the needle is
    // bigger than the haystack, or the needle is a null string or
    // our haystack length after adjusting to the starting position
    // zero, then we can quickly return zero.
    if (_start > haystack_length || needle_length > _range || needle_length == 0)
    {
        return 0;
    }

    // address the string value
    const char *haypointer = stringData + _start;
    const char *needlepointer = needle->getStringData();
    size_t location = _start + 1;         // this is the match location as an index
    // calculate the number of probes we can make in this string
    size_t count = _range - needle_length + 1;

    // now scan
    while (count--)
    {
        // this is a caseless compare
        if (caselessCompare(haypointer, needlepointer, needle_length) == 0)
        {
            return location;
        }
        // step our pointers accordingly
        location++;
        haypointer++;
    }
    return 0;  // we got nothing...
}


/**
 * Locate the last positon of a string within the designated
 * string buffer.
 *
 * @param stringData The stringData to search within.
 * @param length     The length of the string data.
 * @param needle     The needle to search for.
 * @param pstart     The starting position.
 *
 * @return An integer object giving the located position.
 */
RexxInteger *StringUtil::lastPosRexx(const char *stringData, size_t haystackLen, RexxString  *needle, RexxInteger *_start, RexxInteger *_range)
{
    needle = stringArgument(needle, ARG_ONE);
    // find out where to start the search. The default is at the very end.
    size_t startPos = optionalPositionArgument(_start, haystackLen, ARG_TWO);
    size_t range = optionalLengthArgument(_range, haystackLen, ARG_THREE);

    // now perform the actual search.
    return new_integer(lastPos(stringData, haystackLen, needle, startPos, range));
}


/**
 * Primitive level lastpos search within a string buffer.
 *
 * @param stringData The maystack buffer.
 * @param haystack_length
 *                   The length of the haystack.
 * @param needle     The search needle.
 * @param _start     The starting position.
 *
 * @return The offset of the located needle, or 0 if the needle doesn't exist.
 */
size_t StringUtil::lastPos(const char *stringData, size_t haystackLen, RexxString  *needle, size_t _start, size_t range)
{
    size_t needleLen = needle->getLength();

    // no match possible if either string is null
    if (needleLen == 0 || haystackLen == 0 || needleLen > range)
    {
        return 0;
    }
    else
    {
        // get the start position for the search.
        haystackLen = Numerics::minVal(_start, haystackLen);
        range = Numerics::minVal(range, haystackLen);
        // adjust the starting point by pretending this is smaller than the original string
        const char *startPoint = stringData + haystackLen - range;

        // if nothing found,
        const char *matchLocation = lastPos(needle->getStringData(), needleLen, startPoint, range);
        // this is either 0 or the offset of the match position
        return matchLocation == NULL ? 0 : matchLocation - stringData + 1;
    }
}


/**
 * Absolutely most primitive version of a lastpos search.  This
 * version searches directly in a buffer rather than a Rexx
 * String.
 *
 * @param needle    Pointer to the needle string.
 * @param needleLen Length of the needle string.
 * @param haystack  The pointer to the haystack string.
 * @param haystackLen
 *                  The length of the haystack string.
 *
 * @return A pointer to the match location or NULL if there is no match.
 */
const char *StringUtil::lastPos(const char *needle, size_t needleLen, const char *haystack, size_t haystackLen)
{
    // if the needle's longer than the haystack, no chance of a match
    if (needleLen > haystackLen)
    {
        return NULL;
    }
    // set the search startpoing point relative to the end of the search string
    haystack = haystack + haystackLen - needleLen;
    // this is the possible number of compares we might need to perform
    size_t count = haystackLen - needleLen + 1;
    // now scan backward
    while (count > 0)
    {
        // got a match at this position, return it directly
        if (memcmp(haystack, needle, needleLen) == 0)
        {
            return haystack;
        }
        // decrement count and position
        count--;
        haystack--;
    }
    return NULL;   // nothing to see here folks, move along
}


/**
 * Primitive level caseless lastpos search within a string
 * buffer.
 *
 * @param stringData The maystack buffer.
 * @param haystack_length
 *                   The length of the haystack.
 * @param needle     The search needle.
 * @param _start     The starting position.
 *
 * @return The offset of the located needle, or 0 if the needle doesn't exist.
 */
size_t StringUtil::caselessLastPos(const char *stringData, size_t haystackLen, RexxString *needle, size_t _start, size_t range)
{
    size_t needleLen = needle->getLength();          /* and get the length too            */

    // no match possible if either string is null
    if (needleLen == 0 || haystackLen == 0 || needleLen > range)
    {
        return 0;
    }
    else
    {
        // get the start position for the search.
        haystackLen = Numerics::minVal(_start, haystackLen);
        range = Numerics::minVal(range, haystackLen);
        // adjust the starting point
        const char *startPoint = stringData + haystackLen - range;
                                         /* do the search                     */
        const char *matchLocation = caselessLastPos(needle->getStringData(), needleLen, startPoint, range);
        // this is either 0 or the offset of the match position
        return matchLocation == NULL ? 0 : matchLocation - stringData + 1;
    }
}


/**
 * Absolutely most primitive version of a caseless lastpos
 * search. This version searches directly in a buffer rather
 * than a Rexx String.
 *
 * @param needle    Pointer to the needle string.
 * @param needleLen Length of the needle string.
 * @param haystack  The pointer to the haystack string.
 * @param haystackLen
 *                  The length of the haystack string.
 *
 * @return A pointer to the match location or NULL if there is no match.
 */
const char *StringUtil::caselessLastPos(const char *needle, size_t needleLen, const char *haystack, size_t haystackLen)
{
    // if the needle's longer than the haystack, no chance of a match
    if (needleLen > haystackLen)
    {
        return NULL;
    }
    // set the search startpoing point relative to the end of the search string
    haystack = haystack + haystackLen - needleLen;
    // this is the possible number of compares we might need to perform
    size_t count = haystackLen - needleLen + 1;
    // now scan backward
    while (count > 0)
    {
        // got a match at this position, return it directly
        if (caselessCompare(haystack, needle, needleLen) == 0)
        {
            return haystack;
        }
        // decrement count and position
        count--;
        haystack--;
    }
    return NULL;   // nothing to see here folks, move along
}


/**
 * Extract an individual character from a string buffer, returned
 * as a string object.
 *
 * @param stringData The string buffer.
 * @param stringLength
 *                   The length of the buffer.
 * @param positionArg
 *                   The target position.
 *
 * @return The target character, as a string value.
 */
RexxString *StringUtil::subchar(const char *stringData, size_t stringLength, RexxInteger *positionArg)
{
    // the starting position isn't optional
    size_t position = positionArgument(positionArg, ARG_ONE) - 1;

    // beyond the bounds, this is a null string
    if (position >= stringLength)
    {
        return GlobalNames::NULLSTRING;
    }
    // return the single character
    return new_string(stringData + position, 1);
}

/**
 * Search for a separator within a string segment.
 *
 * @param start     The start position for the scan.
 * @param end       The last possible position for a scan (taking the length
 *                  of the separator into account).
 * @param sepData   The separator data
 * @param sepLength the length of the separator.
 *
 * @return The next match position, or null for no match.
 */
const char *StringUtil::locateSeparator(const char *start, const char *end, const char *sepData, size_t sepLength)
{
    // search for separator character
    while (start < end)
    {
        if (memcmp(start, sepData, sepLength) == 0)
        {
            return start;
        }
        start++;
    }
    // not found
    return NULL;
}


/**
 * Carve the string buffer up into an array of string values.
 *
 * @param start     The starting position of the buffer.
 * @param length    The length of the buffer.
 * @param separator The optional separator character.
 *
 * @return An array of all strings within the buffer, with the target
 *         delimiter removed.
 */
ArrayClass *StringUtil::makearray(const char *start, size_t length, RexxString *separator)
{
    const char *sepData = "\n";          // set our default separator
    size_t sepSize = 1;
    bool checkCR = true;                 // by default, we look for either separator

    // if we have an explicit separator, use it instead
    if (separator != OREF_NULL)
    {
        // make sure this is really a string value
        separator = stringArgument(separator, ARG_ONE);
        sepData = separator->getStringData();
        sepSize = separator->getLength();
        checkCR = false;                 // if explicitly given, only use the given one
    }


    // the Null string gets special handling
    if (sepSize == 0)
    {
        // we need an array the size of the string
        ArrayClass *array = new_array(length);
        ProtectedObject p1(array);
        // create a string for each character and poke into the array
        for (size_t i = 0; i < length; i++, start++)
        {
            array->put(new_string(start, 1), i + 1);
        }
        return array;
    }

    // we need to get a count of the strings that will be in the result.
    // this saves a lot of overhead that can result from continually expanding
    // the size of the result array

    size_t count = 0;

    // this is our current scan pointer
    const char *scan = start;

    // this is the end of the string
    const char *stringEnd = start + length;

    // this is where we stop scanning
    const char *end = start + length - sepSize + 1;

    while (scan < end)
    {
        // search for the next separator, if not found, we're done
        const char *tmp = locateSeparator(scan, end, sepData, sepSize);
        if (tmp == NULL)
        {
            break;
        }

        count++;
        // step to the next scan position
        scan = tmp + sepSize;
    }
    // we might have a tail piece here
    if (scan < stringEnd)
    {
        count++;
    }


    // create an array of strings to return, then repeat the scan
    Protected<ArrayClass> strings = new_array(count);
    // scan the string again, but this time, do the chop
    while (start < end)
    {
        // search for the next separator, if not found, we're done
        const char *tmp = locateSeparator(start, end, sepData, sepSize);
        if (tmp == NULL)
        {
            break;
        }
        size_t stringLen = tmp - start;
        // if checking for either linend possibility, reduce the length if we had
        // a leading CR character
        if (checkCR && *(tmp - 1) == '\r')
        {
            stringLen--;
        }
        strings->append(new_string(start, stringLen));
        // step to the next scan position
        start = tmp + sepSize;
    }
    // we might have a tail piece here
    if (start < stringEnd)
    {
        size_t stringLen = stringEnd - start;
        strings->append(new_string(start, stringLen));
    }
    // just return the string array.
    return strings;
}


/**
 * Perform a caseless comparison of two strings
 *
 * @param string1 The first string to compare.
 * @param string2 The second string.
 * @param length  The length to compare.
 *
 * @return 0 if the two strings are equal, -1 if the first is less than the
 *         second, and 1 if the first string is the greater.
 */
int  StringUtil::caselessCompare(const char *string1, const char *string2, size_t length)
{
    // totally equal within the case?  That is easy
    if (!memcmp(string1, string2, length))
    {
        return 0;
    }

    // need to do this the hard way
    while (length--)
    {
        int rc = toupper(*string1++) - toupper(*string2++);
        if (rc != 0)
        {
            return rc < 0 ? -1 : 1;
        }
    }
    // these are equal with caselessness taken into consideration.
    return 0;
}



/**
 * Convert a hex digit to it's integer value equivalent.
 *
 * @param ch     The input character.
 *
 * @return the integer value of the digit.
 */
int StringUtil::hexDigitToInt(char  ch)
{
    // for digits, just subtract the character zero
    if (isdigit(ch))
    {
        return ch - '0';
    }
    // for the alpha digits, subtract A to get the relative value, then
    // add 10 to that result
    else
    {
        return toupper(ch) - 'A' + 10;
    }
}


/**
 * The value of the buffer contents
 * interpreted as the binary expansion
 * of a byte, with most significant
 * bit in s[0] and least significant
 * bit in s[7].
 *
 * @param string The string to pack (must be 8 digits)
 *
 * @return The single packed character.
 */
char StringUtil::packByte(const char *string )
{
    char result = 0;
    // loop through all 8 characters
    for (int i = 0; i < 8; i++)
    {
        // if this "bit" is set, then set the same bit in the binary version
        if (string[i] == '1')
        {
            result |= (1<<(7-i));
        }
    }
    return result;
}

/**
 * The value of the buffer contents
 * interpreted as the binary expansion
 * of a byte, with most significant
 * bit in s[0] and least significant
 * bit in s[7].
 *
 * @param string Pack 4 characters into a hex string value.
 *
 * @return The hex character representing the nibble value.
 */
char StringUtil::packNibble(const char *string)
{
    char  buf[8];

    memset(buf, '0', 4);                 // set first 4 bytes to zero
    memcpy(buf+4, string, 4);            // copy next 4 bytes from the string
    int i = packByte(buf);               // pack to a single byte
    return "0123456789ABCDEF"[i];        // convert to a printable character
}


/**
 * Pack 2 0123456789ABCDEFabcdef chars into
 * byte
 *
 * The value of the buffer contents
 * interpreted as the hex expansion
 * of a byte, with most significant
 * nibble in s[0] and least significant
 * nibble in s[2].
 *
 * @param Byte   The pointer to the hex digit pair to pack.
 *
 * @return The single byte encoding of the pair of digits.
 */
char StringUtil::packByte2(const char *bytes)
{
    // covert each hex digit and combind into a single value
    int nibble1 = hexDigitToInt(bytes[0]);
    int nibble2 = hexDigitToInt(bytes[1]);
    /* combine the two digits            */

    return ((nibble1 << 4) | nibble2);
}


/**
 * Validate blocks in string
 *
 * A string is considered valid if consists
 * of zero or more characters belonging to
 * the null-terminated C string set in
 * groups of size modulus.  The first group
 * may have fewer than modulus characters.
 * The groups are optionally separated by
 * one or more blanks.
 *
 * @param String  The string to validate.
 * @param Length  The string length.
 * @param Set     The valid characters in the set.
 * @param Modulus The size of the smallest allowed grouping.
 * @param Hex     Indicates this is a hex or binary string.  Used for issuing
 *                the correct error type.
 *
 * @return The number of valid digits found.
 */
size_t StringUtil::validateSet(const char *string, size_t length, const char *set, int modulus, bool hex)
{
    // leading whitespace not permitted
    if (*string == RexxString::ch_SPACE || *string == RexxString::ch_TAB)
    {
        reportException(hex ? Error_Incorrect_method_hexblank : Error_Incorrect_method_binblank, IntegerOne);
    }

    bool spaceFound = false;
    size_t count = 0;
    size_t residue = 0;
    const char *current = string;
    const char *spaceLocation = NULL;
    char ch;

    // scan the entire string section
    for (; length; length--)
    {
        ch = *current++;

        // if this is in the set, then add in the count of digits
        if (ch != '\0' && strchr(set, ch) != NULL)
        {
            count++;
        }
        else
        {
            // need to handle white space
            if (ch == RexxString::ch_SPACE || ch == RexxString::ch_TAB)
            {
                // save the location for back checking
                spaceLocation = current;
                // if this is the first space location,
                // this section is permitted to have fewer than the modulus
                // count of characters.
                if (!spaceFound)
                {
                    // get the remainder count and mark that we have seen the
                    // first gap
                    residue = (count % modulus);
                    spaceFound = 1;
                }
                // count is still the whole count.  Each time we see a whitespace gap,
                // the residue needs to remain the same as the first gap
                else if (residue != (count % modulus))
                {
                    reportException(hex ? Error_Incorrect_method_hexblank : Error_Incorrect_method_binblank, spaceLocation - string);
                }
            }
            // the remaining possibility is an invalid character
            else
            {

                reportException(hex ? Error_Incorrect_method_invhex : Error_Incorrect_method_invbin, new_string(ch));
            }
        }
    }

    // we've hit the end.  We could have ended on whitespace, which is an error, or the final grouping
    // has the wrong number of characters
    if ((ch == RexxString::ch_SPACE || ch == RexxString::ch_TAB) || (spaceFound && ((count % modulus) != residue)))
    {
        reportException(hex ? Error_Incorrect_method_hexblank : Error_Incorrect_method_binblank, spaceLocation - string);
    }
    return count;
}


/**
 * Scan string for next members of
 * character set
 *
 * @param Destination
 *               The string where the characters are packed.
 * @param Source The source for the string data.
 * @param Length The length of the input string.
 * @param Count  The number of valid characters in the string.
 * @param Set    The set of allowed characters.
 * @param ScannedSize
 *               The returned scan size.
 *
 * @return
 */
size_t  StringUtil::chGetSm(char *destination, const char *source, size_t length, size_t count, const char *characterSet, size_t &scannedSize)
{
    // make sure the scanned size is initialized
    scannedSize = 0;
    size_t found = 0;                    // number of characters located
    const char *current = source;        // scan pointer

    for (; length > 0; length--)
    {
        char ch = *current++;
        scannedSize++;

        // if this is one of our target characters, copy it to the destination
        if (ch != '\0' && strchr(characterSet, ch) != NULL)
        {
            *destination++ = ch;
            // if we've copied the desired number of characters, we're finished
            if (++found == count)
            {
                break;
            }
        }
    }

    // return the number found (which might be less than the count if
    // we ran out of string.
    return found;
}


/**
 * pack a string of 'hex' digits in place
 *
 * take two alpha chars and make into one byte
 *
 * @param String The string to pack
 * @param StringLength
 *               The length of the string.
 *
 * @return The resulting packed string.
 */
RexxString *StringUtil::packHex(const char *string, size_t stringLength)
{
    // if the string we're packing is a null string, the result is also a null string
    if (stringLength == 0)
    {
        return GlobalNames::NULLSTRING;
    }

    const char *source = string;
    // perform the validation and get a character cound
    size_t nibbles = validateSet(source, stringLength, "0123456789ABCDEFabcdef", 2, true);
    // get a result string, with rounding in case we have an odd number of digits
    RexxString *retval = raw_string((nibbles + 1) / 2);

    char *destination = retval->getWritableData();

    // process all of the nibbles
    while (nibbles > 0)
    {
        char     buf[8];
        // we do this two characters at a time, but the first group might
        // only be one digit if we have an odd number
        size_t b = nibbles % 2 == 0 ? 2 : 1;
        // if we have an odd number, pad the buffer with zeros
        if (b == 1)
        {
            memset(buf, '0', 2);
        }
        size_t scanned;
        // copy the digits into out buffer...we're copying either 1 or 2 characters
        chGetSm(buf + 2 - b, source, stringLength, b, "0123456789ABCDEFabcdef", scanned);
        // now convert this into a single character and insert into the destination string
        *destination++ = packByte2(buf);
        source += scanned;
        stringLength -= scanned;
        nibbles -= b;
    }
    return retval;
}


/**
 * convert nibble to 4 '0'/'1' chars
 *
 * p[0], ..., p[3]: the four '0'/'1'
 * chars representing the nibble
 *
 * No terminating null character is
 * produced
 *
 * @param Val    The nibble to unpack.
 * @param p      The location to unpack into.
 */
void StringUtil::unpackNibble(int Val, char *p)
{
    p[0] = (Val & 0x08) != 0 ?'1':'0';
    p[1] = (Val & 0x04) != 0 ?'1':'0';
    p[2] = (Val & 0x02) != 0 ?'1':'0';
    p[3] = (Val & 0x01) != 0 ?'1':'0';
}


/**
 * Find the first occurrence of the set non-member in a string.
 *
 * @param String The string to search.
 * @param Set    The character set.
 * @param Length The length to search.
 *
 * @return The position of a match.
 */
const char *StringUtil::memcpbrk(const char *string, const char *set, size_t length)
{
    while (length--)
    {
        // a null character or one in the set will terminate
        if (*string == '\0' || !strchr(set, *string))
        {
            return string;
        }
        string++;
    }
    // no offenders found
    return NULL;
}


/**
 * Validate blocks in string
 *
 * A string is considered valid if consists
 * of zero or more characters belonging to
 * the null-terminated C string set in
 * groups of size modulus.  The first group
 * may have fewer than modulus characters.
 * The groups are optionally separated by
 * one or more blanks.  This version does not raise errors.
 *
 * @param String     The string to validate.
 * @param Length     The string length.
 * @param Set        The validation set.
 * @param Modulus    The set modulus
 * @param PackedSize The final packed size.
 *
 * @return The count of located characters.
 */
bool StringUtil::validateCharacterSet(const char *string, size_t length, const char *set, int modulus, size_t &count)
{
    // leading whitespace not permitted
    if (*string == RexxString::ch_SPACE || *string == RexxString::ch_TAB)
    {
        return false;
    }

    bool spaceFound = false;
    count = 0;
    size_t residue = 0;
    const char *current = string;
    const char *spaceLocation = NULL;
    char ch;

    // scan the entire string section
    for (; length; length--)
    {
        ch = *current++;

        // if this is in the set, then add in the count of digits
        if (ch != '\0' && strchr(set, ch) != NULL)
        {
            count++;
        }
        else
        {
            // need to handle white space
            if (ch == RexxString::ch_SPACE || ch == RexxString::ch_TAB)
            {
                // save the location for back checking
                spaceLocation = current;
                // if this is the first space location,
                // this section is permitted to have fewer than the modulus
                // count of characters.
                if (!spaceFound)
                {
                    // get the remainder count and mark that we have seen the
                    // first gap
                    residue = (count % modulus);
                    spaceFound = 1;
                }
                // count is still the whole count.  Each time we see a whitespace gap,
                // the residue needs to remain the same as the first gap
                else if (residue != (count % modulus))
                {
                    return false;
                }
            }
            // the remaining possibility is an invalid character
            else
            {
                return false;
            }
        }
    }

    // we've hit the end.  We could have ended on whitespace, which is an error, or the final grouping
    // has the wrong number of characters
    if ((ch == RexxString::ch_SPACE || ch == RexxString::ch_TAB) || (spaceFound && ((count % modulus) != residue)))
    {
        return false;
    }
    // everything validated
    return true;
}


/**
 * Perform primitive datatype validation.
 *
 * @param String The target string.
 * @param Option The type of data to validate.
 *
 * @return True if this is of the indicated type, false for any mismatch.
 */
RexxObject *StringUtil::dataType(RexxString *string, char option )
{
    size_t len = string->getLength();
    const char *scanp = string->getStringData();

    // no process each type option
    switch (toupper(option))
    {
        case RexxString::DATATYPE_ALPHANUMERIC:
            return booleanObject(len != 0 && !memcpbrk(scanp, RexxString::ALPHANUM, len));

        case RexxString::DATATYPE_BINARY:
        {
            size_t count;
            return booleanObject(len == 0 || validateCharacterSet(scanp, len, RexxString::BINARY, 4, count));
        }

        case RexxString::DATATYPE_LOWERCASE:
            return booleanObject(len != 0 && !memcpbrk(scanp, RexxString::LOWER_ALPHA, len));

        case RexxString::DATATYPE_UPPERCASE:
            return booleanObject(len != 0 && !memcpbrk(scanp, RexxString::UPPER_ALPHA, len));

        case RexxString::DATATYPE_MIXEDCASE:
            return booleanObject(len != 0 && !memcpbrk(scanp, RexxString::MIXED_ALPHA, len));

        case RexxString::DATATYPE_WHOLE_NUMBER:
        {
            // validate as a number
            NumberString *tempNum = string->numberString();
            // if a valid string, then force rounding and see if this
            // is a valid integer
            if (tempNum != OREF_NULL)
            {
                tempNum = (NumberString *)tempNum->plus(IntegerZero);
                return booleanObject(tempNum->isInteger());
            }
            return TheFalseObject;
        }

        case RexxString::DATATYPE_NUMBER:
        {
            // validate as a number
            NumberString *tempNum = string->numberString();
            return booleanObject(tempNum != OREF_NULL);
        }

        case RexxString::DATATYPE_9DIGITS:
        {
            wholenumber_t temp;
            return booleanObject(string->numberValue(temp));
        }

        case RexxString::DATATYPE_HEX:
        {
            size_t count;
            return booleanObject(len == 0 || validateCharacterSet(scanp, len, RexxString::HEX_CHAR_STR, 2, count));
        }

        case RexxString::DATATYPE_SYMBOL:
            return booleanObject(string->isSymbol() != STRING_BAD_VARIABLE);

        case RexxString::DATATYPE_VARIABLE:
        {
            // validate the symbol
            StringSymbolType type = string->isSymbol();
            // a valid variable type?
            return booleanObject(type == STRING_NAME || type == STRING_STEM || type == STRING_COMPOUND_NAME);
        }

        case RexxString::DATATYPE_LOGICAL:
            return booleanObject(!(len != 1 || (*scanp != '1' && *scanp != '0')));

        default  :
            reportException(Error_Incorrect_method_option, "ABCDLMNOSUVWX9", new_string(option));
    }
    return TheFalseObject;
}


/**
 * Count the number of words in a string.
 *
 * @param string The string to count.
 * @param stringLength
 *               The length of the string.
 *
 * @return The count of white-space delimited words.
 */
size_t  StringUtil::wordCount(const char *string, size_t stringLength )
{
    // no string, no words....
    if (stringLength == 0)
    {
        return 0;
    }

    size_t count = 0;
    RexxString::WordIterator counter(string, stringLength);

    // count the words in the string
    while (counter.next())
    {
        count++;
    }

    return count;
}


/**
 * Count the occurences of a string within another string.
 *
 * @param hayStack Pointer to the haystack data.
 * @param hayStackLength
 *                 Length of the haystack data.
 * @param needle   The needle we're searching for
 *
 * @return The count of needle occurrences located in the string.
 */
size_t StringUtil::countStr(const char *hayStack, size_t hayStackLength, RexxString *needle)
{
    size_t count = 0;

    // get the first match position
    size_t matchPos = pos(hayStack, hayStackLength, needle, 0, hayStackLength);
    while (matchPos != 0)
    {
        count = count + 1;
        // step to the new position and search
        matchPos = pos(hayStack, hayStackLength, needle, matchPos + needle->getLength() - 1, hayStackLength);
    }
    return count;
}


/**
 * Count the occurences of a string within another string.
 *
 * @param hayStack Pointer to the haystack data.
 * @param hayStackLength
 *                 Length of the haystack data.
 * @param needle   The needle we're searching for
 *
 * @return The count of needle occurrences located in the string.
 */
size_t StringUtil::caselessCountStr(const char *hayStack, size_t hayStackLength, RexxString *needle)
{
    size_t count = 0;

    size_t matchPos = caselessPos(hayStack, hayStackLength, needle, 0, hayStackLength);
    while (matchPos != 0)
    {
        count++;
        // step to the new position and search
        matchPos = caselessPos(hayStack, hayStackLength, needle, matchPos + needle->getLength() - 1, hayStackLength);
    }
    return count;
}


/**
 * Locate the first occurrence of a character in a string.
 *
 * @param string The search string.
 * @param length The length of the search string.
 * @param target The target character.
 *
 * @return The match offset, or SIZE_MAX if this is not found.
 */
size_t StringUtil::memPos(const char *string, size_t length, char target)
{
    for (const char *scan = string; length; scan++, length--)
    {
        // if we have a match, return the offset
        if (*scan == target)
        {
            return scan - string;
        }
    }
    return SIZE_MAX;             // no match position
}


/**
 * Perform a verify operation on a section of data.
 *
 * @param data      The data pointer
 * @param stringLen The length of the string to match
 * @param ref       The reference search string.
 * @param option    The match/nomatch option.
 * @param _start    The starting offset for the match.
 *
 * @return The match/nomatch position, or 0 if nothing was found.
 */
RexxInteger *StringUtil::verify(const char *data, size_t stringLen, RexxString  *ref, RexxString  *option, RexxInteger *_start, RexxInteger *range)
{
    // get the reference string information
    ref = stringArgument(ref, ARG_ONE);
    size_t referenceLen = ref->getLength();
    const char *refSet = ref->getStringData();

    char opt = optionalOptionArgument(option, "MN", RexxString::VERIFY_NOMATCH, ARG_TWO);

    size_t startPos = optionalPositionArgument(_start, 1, ARG_THREE);
    size_t stringRange = optionalLengthArgument(range, stringLen - startPos + 1, ARG_FOUR);

    if (startPos > stringLen)
    {
        return IntegerZero;
    }

    // adjust the range for seaching
    stringRange = Numerics::minVal(stringRange, stringLen - startPos + 1);

    const char *current = data + startPos - 1;

    if (referenceLen == 0)
    {
        if (opt == RexxString::VERIFY_MATCH)
        {
            return IntegerZero;
        }
        else
        {
            return new_integer(startPos);/* non-match at start position       */
        }
    }
    else
    {
        // we're verifying that all characters are members of the reference set, so
        // return the first non-matching character
        if (opt == RexxString::VERIFY_NOMATCH)
        {
            while (stringRange-- != 0)
            {
                // if no match at this position, return this position
                if (!StringUtil::matchCharacter(*current++, refSet, referenceLen))
                {
                    return new_integer(current - data);
                }
            }
            // this is always a non matching situation to get here
            return IntegerZero;
        }
        else
        {
            while (stringRange-- != 0)
            {
                // if we have a match at this position, trigger this
                if (StringUtil::matchCharacter(*current++, refSet, referenceLen))
                {
                    return new_integer(current - data);
                }
            }
            // this is always a non matching situation to get here
            return IntegerZero;
        }
    }
}


/**
 * Do a subword operation on a buffer of data
 *
 * @param data     The start of the data buffer.
 * @param length   The length of the buffer
 * @param position The starting word position.
 * @param plength  the count of words to return.
 *
 * @return The string containing the indicated subwords.
 */
RexxString *StringUtil::subWord(const char *data, size_t length, RexxInteger *position, RexxInteger *plength)
{
    size_t wordPos = positionArgument(position, ARG_ONE);
    // get num of words to extract.  The default is a "very large number
    size_t count = optionalLengthArgument(plength, Numerics::MAX_WHOLENUMBER, ARG_TWO);

    // handle cases that will always result in a null string
    if (length == 0 || count == 0)
    {
        return GlobalNames::NULLSTRING;
    }

    // get an iterator
    RexxString::WordIterator iterator(data, length);

    // try to skip ahead to the target word...if we don't have that many words,
    // return a null string
    if (!iterator.skipWords(wordPos))
    {
        return GlobalNames::NULLSTRING;
    }

    const char *wordStart = iterator.wordPointer();

    // skip ahead the count number of words (we have the first word already, so
    // skip one fewer)
    iterator.skipWords(count - 1);

    const char *wordEnd = iterator.wordEndPointer();

    // get the substring
    return new_string(wordStart, wordEnd - wordStart);
}


/**
 * Do a wordList operation on a buffer of data
 *
 * @param data     The start of the data buffer.
 * @param length   The length of the buffer
 * @param position The starting word position.
 * @param plength  the count of words to return.
 *
 * @return The array containing the indicated subwords.
 */
ArrayClass *StringUtil::subWords(const char *data, size_t length, RexxInteger *position, RexxInteger *plength)
{
    size_t wordPos = optionalPositionArgument(position, 1, ARG_ONE);
    // get num of words to extract.  The default is a "very large number
    size_t count = optionalLengthArgument(plength, Numerics::MAX_WHOLENUMBER, ARG_TWO);

    // handle cases that will always result an empty array
    if (length == 0 || count == 0)
    {
        return new_array();
    }

    // get an iterator
    RexxString::WordIterator iterator(data, length);
    // try to skip ahead to the target word...if we don't have that many words,
    // return an empty array
    if (!iterator.skipWords(wordPos))
    {
        return new_array();
    }

    // we make this size zero so the size and the items count will match
    Protected<ArrayClass> result = new_array((size_t)0);

    // we're positioned at the first word, iterate and insert the new word values
    while (count--)
    {
        result->append(new_string(iterator.wordPointer(), iterator.wordLength()));
        // if that was the last word, exit
        if (!iterator.next())
        {
            break;
        }
    }
    return result;                     // return the populated array
}


/**
 * Extract a word from a buffer
 *
 * @param data     The data pointer
 * @param length   the length of the data buffer.
 * @param position the target word position.
 *
 * @return The string value of the word at the indicated position.
 */
RexxString *StringUtil::word(const char *data, size_t length, RexxInteger *position)
{
    size_t wordPos = positionArgument(position, ARG_ONE);

    if (length == 0)
    {
        return GlobalNames::NULLSTRING;
    }

    // get an iterator
    RexxString::WordIterator iterator(data, length);
    // try to skip ahead to the target word...if we don't have that many words,
    // return a null string
    if (!iterator.skipWords(wordPos))
    {
        return GlobalNames::NULLSTRING;
    }

    return new_string(iterator.wordPointer(), iterator.wordLength());
}


/**
 * Extract all words from a buffer
 *
 * @param data     The data pointer
 * @param length   the length of the data buffer.
 * @param position the target word position.
 *
 * @return The string value of the word at the indicated position.
 */
ArrayClass *StringUtil::words(const char *data, size_t length)
{
    // get an iterator
    RexxString::WordIterator iterator(data, length);

    // we make this size zero so the size and the items count will match
    Protected<ArrayClass> result = new_array((size_t)0);

    // step through this until we run out of words
    while (iterator.next())
    {
        result->append(new_string(iterator.wordPointer(), iterator.wordLength()));
    }
    return result;      // return whatever we've accumulated
}


/**
 * Return the index position for a given word
 *
 * @param data     The data containing the words
 * @param length   The length of the data buffer
 * @param position The target word position
 *
 * @return The offset of the start of the indicated word.
 */
RexxInteger *StringUtil::wordIndex(const char *data, size_t length, RexxInteger *position)
{
    size_t wordPos = positionArgument(position, ARG_ONE);

    // get an iterator
    RexxString::WordIterator iterator(data, length);
    // try to skip ahead to the target word...if we don't have that many words,
    // return zero
    if (!iterator.skipWords(wordPos))
    {
        return IntegerZero;
    }

    // return as an integer object
    return new_integer(iterator.wordPointer() - data + 1);
}


/**
 * Return the length of the word located at a given index.
 *
 * @param data     The data containing the word list.
 * @param length   The length of the data buffer
 * @param position The target word position.
 *
 * @return The length of the given word at the target index.  Returns
 *         0 if no word is found.
 */
RexxInteger *StringUtil::wordLength(const char *data, size_t length, RexxInteger *position)
{
    size_t wordPos = positionArgument(position , ARG_ONE);

    // get an iterator
    RexxString::WordIterator iterator(data, length);
    // try to skip ahead to the target word...if we don't have that many words,
    // return zero
    if (!iterator.skipWords(wordPos))
    {
        return IntegerZero;
    }

    // return as an integer object
    return new_integer(iterator.wordLength());
}


/**
 * Execute a wordpos search on a buffer of data.
 *
 * @param data   the source data buffer.
 * @param length the length of the buffer
 * @param phrase the search phrase.
 * @param pstart the starting position.
 *
 * @return the location of the start of the search phrase.
 */
size_t StringUtil::wordPos(const char *data, size_t length, RexxString  *phrase, RexxInteger *pstart)
{
    phrase = stringArgument(phrase, ARG_ONE);
    size_t needleLength = phrase->getLength();
    size_t count = optionalPositionArgument(pstart, 1, ARG_TWO);

    const char *needle = phrase->getStringData();
    const char *haystack = data;
    size_t haystackLength = length;

    // cound the words in both the needle and the haystack
    size_t needleWords = wordCount(needle, needleLength);

    size_t haystackWords = wordCount(haystack, haystackLength);

    // if search phrase is longer or no words in search
    // or count is longer, then this is a failure
    if (needleWords > (haystackWords - count + 1) || needleWords == 0 || count > haystackWords)
    {
        return 0;
    }

    // we know how many potential search attempts we can make.
    size_t searchCount = (haystackWords - needleWords - count) + 2;

    RexxString::WordIterator haystackIterator(haystack, haystackLength);

    // skip the haystack ahead to the target word.  We know we have at least
    // count words already
    haystackIterator.skipWords(count);

    // now get an iterator for the needle and position at the first word.
    RexxString::WordIterator needleIterator(needle, needleLength);
    // this will work because we know the needle has at least one word
    needleIterator.next();

    // These two iterators are our masters.  We perform the comparisons at each
    // position by copying the current master state and iterating the copies for
    // the comparisons.

    for (; searchCount; searchCount--)
    {
        // copy the current iterator positions.  This allows us to advance
        // without changing the originals
        RexxString::WordIterator tempHaystack = haystackIterator;
        RexxString::WordIterator tempNeedle = needleIterator;

        // now compare each word in turn
        size_t i;
        for (i = 0; i < needleWords; i++)
        {
            // if the current words don't compare, get out now
            if (!tempHaystack.compare(tempNeedle))
            {
                break;
            }

            // step both iterators to the next word (we already know they have one)
            tempHaystack.next();
            tempNeedle.next();
        }

        // if all of the words have matched, then we can return the current count position.
        if (i == needleWords)
        {
            return count;
        }
        // step to the next haystack position
        haystackIterator.next();
        count++;
    }

    return 0;                          // not found
}


/**
 * Execute a caseless wordpos search on a buffer of data.
 *
 * @param data   the source data buffer.
 * @param length the length of the buffer
 * @param phrase the search phrase.
 * @param pstart the starting position.
 *
 * @return the location of the start of the search phrase.
 */
size_t StringUtil::caselessWordPos(const char *data, size_t length, RexxString  *phrase, RexxInteger *pstart)
{
    phrase = stringArgument(phrase, ARG_ONE);
    size_t needleLength = phrase->getLength();
    size_t count = optionalPositionArgument(pstart, 1, ARG_TWO);

    const char *needle = phrase->getStringData();
    const char *haystack = data;
    size_t haystackLength = length;

    // cound the words in both the needle and the haystack
    size_t needleWords = wordCount(needle, needleLength);

    size_t haystackWords = wordCount(haystack, haystackLength);

    // if search phrase is longer or no words in search
    // or count is longer, then this is a failure
    if (needleWords > (haystackWords - count + 1) || needleWords == 0 || count > haystackWords)
    {
        return 0;
    }

    // we know how many potential search attempts we can make.
    size_t searchCount = (haystackWords - needleWords - count) + 2;

    RexxString::WordIterator haystackIterator(haystack, haystackLength);

    // skip the haystack ahead to the target word.  We know we have at least
    // count words already
    haystackIterator.skipWords(count);

    // now get an iterator for the needle and position at the first word.
    RexxString::WordIterator needleIterator(needle, needleLength);
    // this will work because we know the needle has at least one word
    needleIterator.next();

    // These two iterators are our masters.  We perform the comparisons at each
    // position by copying the current master state and iterating the copies for
    // the comparisons.

    for (; searchCount; searchCount--)
    {
        // copy the current iterator positions.  This allows us to advance
        // without changing the originals
        RexxString::WordIterator tempHaystack = haystackIterator;
        RexxString::WordIterator tempNeedle = needleIterator;

        // now compare each word in turn
        size_t i;
        for (i = 0; i < needleWords; i++)
        {
            // if the current words don't compare, get out now
            if (!tempHaystack.caselessCompare(tempNeedle))
            {
                break;
            }

            // step both iterators to the next word (we already know they have one)
            tempHaystack.next();
            tempNeedle.next();
        }

        // if all of the words have matched, then we can return the current count position.
        if (i == needleWords)
        {
            return count;
        }
        // step to the next haystack position so that we
        // compare from the next location on the next pass.
        haystackIterator.next();
        count++;
    }
    return 0;                          // not found
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* Copyright (c) 1995, 2004 IBM Corporation. All rights reserved.             */
/* Copyright (c) 2005-2015 Rexx Language Association. All rights reserved.    */
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
/* Primitive Translator Token Class                                           */
/*                                                                            */
/******************************************************************************/
#include <ctype.h>
#include <string.h>
#include "RexxCore.h"
#include "StringClass.hpp"
#include "Token.hpp"
#include "SourceFile.hpp"



/**
 * Create a new Token object.
 *
 * @param size   The size of the object.
 *
 * @return Storage for a new instance of a Token.
 */
void *RexxToken::operator new(size_t size)
{
    return new_object(size, T_Token);
}


/**
 * Perform garbage collection on a live object.
 *
 * @param liveMark The current live mark.
 */
void RexxToken::live(size_t liveMark)
{
    memory_mark(this->value);
}


/**
 * Perform generalized live marking on an object.  This is
 * used when mark-and-sweep processing is needed for purposes
 * other than garbage collection.
 *
 * @param reason The reason for the marking call.
 */
void RexxToken::liveGeneral(int reason)
{
    memory_mark_general(this->value);
}


/**
 * Check and update this token for the special assignment forms
 * (+=, -=, etc.).
 *
 * @param source The source for the original operator token.
 */
void RexxToken::checkAssignment(RexxSource *source, RexxString *newValue)
{
    // check if the next character is a special assignment shortcut
    if (source->nextSpecial('=', tokenLocation))
    {
        // this is a special type, which uses the same subtype.
        classId = TOKEN_ASSIGNMENT;
        // this is the new string value of the token
        value = newValue;
    }
}


/**
 * Determine a Token's operator precedence.
 *
 * @return A numeric ranking for operator characters.
 */
int RexxToken::precedence()
{
    // the subclass determines what type of operator this is.
    switch (subclass)
    {
        default:
            return 0;                         // this is the bottom of the heap
            break;

        case OPERATOR_OR:
        case OPERATOR_XOR:
            return 1;                         // various OR types are next
            break;

        case OPERATOR_AND:
            return 2;                         // AND operator ahead of ORs
            break;

        case OPERATOR_EQUAL:                  // comparisons are all together
        case OPERATOR_BACKSLASH_EQUAL:
        case OPERATOR_GREATERTHAN:
        case OPERATOR_BACKSLASH_GREATERTHAN:
        case OPERATOR_LESSTHAN:
        case OPERATOR_BACKSLASH_LESSTHAN:
        case OPERATOR_GREATERTHAN_EQUAL:
        case OPERATOR_LESSTHAN_EQUAL:
        case OPERATOR_STRICT_EQUAL:
        case OPERATOR_STRICT_BACKSLASH_EQUAL:
        case OPERATOR_STRICT_GREATERTHAN:
        case OPERATOR_STRICT_BACKSLASH_GREATERTHAN:
        case OPERATOR_STRICT_LESSTHAN:
        case OPERATOR_STRICT_BACKSLASH_LESSTHAN:
        case OPERATOR_STRICT_GREATERTHAN_EQUAL:
        case OPERATOR_STRICT_LESSTHAN_EQUAL:
        case OPERATOR_LESSTHAN_GREATERTHAN:
        case OPERATOR_GREATERTHAN_LESSTHAN:
            return 3;
            break;

        case OPERATOR_ABUTTAL:                // concatentates
        case OPERATOR_CONCATENATE:
        case OPERATOR_BLANK:
            return 4;
            break;

        case OPERATOR_PLUS:                   // plus and minus
        case OPERATOR_SUBTRACT:
            return 5;
            break;

        case OPERATOR_MULTIPLY:               // multiply and divide versions
        case OPERATOR_DIVIDE:
        case OPERATOR_INTDIV:
        case OPERATOR_REMAINDER:
            return 6;
            break;

        case OPERATOR_POWER:
            return 7;                         // almost the top of the heap
            break;

        case OPERATOR_BACKSLASH:
            return 8;                         // NOT is the top honcho
            break;
    }
}

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
/* REXX Translator                                                            */
/*                                                                            */
/* Primitive Leave Parse Class                                                */
/*                                                                            */
/******************************************************************************/
#include <stdlib.h>
#include "RexxCore.h"
#include "StringClass.hpp"
#include "RexxActivation.hpp"
#include "LeaveInstruction.hpp"
#include "Token.hpp"

/**
 * Constructor for a LEAVE/ITERATE instruction.
 *
 * @param _name  The name of the target loop.
 */
RexxInstructionLeave::RexxInstructionLeave(RexxString *_name)
{
    name = _name;
}


/**
 * Perform garbage collection on a live object.
 *
 * @param liveMark The current live mark.
 */
void RexxInstructionLeave::live(size_t liveMark)
{
    // must be first object marked
    memory_mark(nextInstruction);
    memory_mark(name);
}


/**
 * Perform generalized live marking on an object.  This is
 * used when mark-and-sweep processing is needed for purposes
 * other than garbage collection.
 *
 * @param reason The reason for the marking call.
 */
void RexxInstructionLeave::liveGeneral(MarkReason reason)
{
    // must be first object marked
    memory_mark_general(nextInstruction);
    memory_mark_general(name);
}


/**
 * Flatten a source object.
 *
 * @param envelope The envelope that will hold the flattened object.
 */
void RexxInstructionLeave::flatten(RexxEnvelope *envelope)
{
    setUpFlatten(RexxInstructionLeave)

    flattenRef(nextInstruction);
    flattenRef(name);

    cleanUpFlatten
}

/**
 * Execute a LEAVE or ITERATE instruction.
 *
 * @param context The current excution context.
 * @param stack   The current evaluation stack.
 */
void RexxInstructionLeave::execute(RexxActivation *context, RexxExpressionStack *stack )
{
    context->traceInstruction(this);

    // if this is a LEAVE, we tell the context to leave the appropriate loop.
    if (instructionType == KEYWORD_LEAVE)
    {
        context->leaveLoop(name);
    }
    // we are iterating the loop
    else
    {
        context->iterate(name);
    }

    context->pauseInstruction();
}


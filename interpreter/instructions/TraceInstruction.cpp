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
/* Primitive Trace Parse Class                                                */
/*                                                                            */
/******************************************************************************/
#include "RexxCore.h"
#include "RexxActivation.hpp"
#include "TraceInstruction.hpp"
#include "SourceFile.hpp"


/**
 * Initialize a Trace instruction.
 *
 * @param _expression
 *                   A potential expression to evaluate.
 * @param trace      The new trace setting (can be zero if numeric or dynamic form).
 * @param flags      The translated trace flags for setting-based forms.
 * @param debug_skip A potential debug_skip value.
 */
RexxInstructionTrace::RexxInstructionTrace(RexxObject *_expression, size_t trace, size_t flags, wholenumber_t debug_skip )
{
    // this is an expression for TRACE VALUE forms
    expression = _expression;
    // this is also optional, used for numeric stuff
    debugskip = debug_skip;
    // a trace setting and some optimized flags
    traceSetting = trace;
    traceFlags = flags;
}


/**
 * Perform garbage collection on a live object.
 *
 * @param liveMark The current live mark.
 */
void RexxInstructionTrace::live(size_t liveMark)
{
    // this must be the first object marked
    memory_mark(nextInstruction);
    memory_mark(expression);
}


/**
 * Perform generalized live marking on an object.  This is
 * used when mark-and-sweep processing is needed for purposes
 * other than garbage collection.
 *
 * @param reason The reason for the marking call.
 */
void RexxInstructionTrace::liveGeneral(MarkReason reason)
{
    // this must be the first object marked
    memory_mark_general(nextInstruction);
    memory_mark_general(expression);
}


/**
 * Perform generalized live marking on an object.  This is
 * used when mark-and-sweep processing is needed for purposes
 * other than garbage collection.
 *
 * @param reason The reason for the marking call.
 */
void RexxInstructionTrace::flatten(RexxEnvelope *envelope)
{
    setUpFlatten(RexxInstructionTrace)

    flattenRef(nextInstruction);
    flattenRef(expression);

    cleanUpFlatten
}

/**
 * Execute a TRACE instruction
 *
 * @param context The current execution context.
 * @param stack   The current evaluation stack.
 */
void RexxInstructionTrace::execute(RexxActivation *context, RexxExpressionStack *stack)
{
    // trace if needed.
    context->traceInstruction(this);
    // is this a debug skip request (the setting value is zero in that case)
    if ((traceSetting&TRACE_SETTING_MASK) == 0)
    {
        // turn on the skip mode in the context.
        context->debugSkip(debugskip, (traceSetting&DEBUG_NOTRACE) != 0);
    }
    // non-dynamic form?
    else if (expression == OREF_NULL)
    {
        // if not in debug mode, we just do the setting.  The TRACE instruction
        // is ignored in debug mode, although we do trace everything
        if (!context->inDebug())
        {
            context->setTrace(traceSetting, traceFlags);
        }
        else
        {
            // we're in debug, so do the pause
            context->pauseInstruction();
        }
    }
    // dynamic form, requiring an expression evaluation.
    else
    {
        // evaluate, and get as a string value
        RexxObject *result = expression->evaluate(context, stack);
        RexxString *value = REQUEST_STRING(result);
        // Even trace gets traced :-)
        context->traceResult(result);
        // again, we don't change anything if we're already in debug mode.
        if (!context->inDebug())
        {
            context->setTrace(value);
        }
        else
        {
            // in debug mode means we do need to pause.
            context->pauseInstruction();
        }
    }
}


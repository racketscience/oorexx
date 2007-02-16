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
/* REXX Translator                                              SelectInstruction.c      */
/*                                                                            */
/* Primitive Select Parse Class                                               */
/*                                                                            */
/******************************************************************************/
#include <stdlib.h>
#include "RexxCore.h"
#include "RexxActivity.hpp"
#include "RexxActivation.hpp"
#include "QueueClass.hpp"
#include "IntegerClass.hpp"
#include "SelectInstruction.hpp"
#include "EndInstruction.hpp"
#include "IfInstruction.hpp"
#include "OtherwiseInstruction.hpp"

RexxInstructionSelect::RexxInstructionSelect()
/******************************************************************************/
/* Function:  Initialize a SELECT instruction object                          */
/******************************************************************************/
{
                                       /* create a list of WHEN targets     */
  OrefSet(this, this->when_list, new_queue());
}

void RexxInstructionSelect::live()
/******************************************************************************/
/* Function:  Normal garbage collection live marking                          */
/******************************************************************************/
{
  setUpMemoryMark
  memory_mark(this->nextInstruction);  /* must be first one marked          */
  memory_mark(this->when_list);
  memory_mark(this->end);
  memory_mark(this->otherwise);
  cleanUpMemoryMark
}

void RexxInstructionSelect::liveGeneral()
/******************************************************************************/
/* Function:  Generalized object marking                                      */
/******************************************************************************/
{
  setUpMemoryMarkGeneral
                                       /* must be first one marked          */
  memory_mark_general(this->nextInstruction);
  memory_mark_general(this->when_list);
  memory_mark_general(this->end);
  memory_mark_general(this->otherwise);
  cleanUpMemoryMarkGeneral
}

void RexxInstructionSelect::flatten(RexxEnvelope *envelope)
/******************************************************************************/
/* Function:  Flatten an object                                               */
/******************************************************************************/
{
  setUpFlatten(RexxInstructionSelect)

  flatten_reference(newThis->nextInstruction, envelope);
  flatten_reference(newThis->when_list, envelope);
  flatten_reference(newThis->end, envelope);
  flatten_reference(newThis->otherwise, envelope);

  cleanUpFlatten
}

void RexxInstructionSelect::execute(
    RexxActivation      *context,      /* current activation context        */
    RexxExpressionStack *stack )       /* evaluation stack                  */
/****************************************************************************/
/* Function:  Execute a REXX SELECT instruction                             */
/****************************************************************************/
{
  context->traceInstruction(this);     /* trace if necessary                */
  context->indent();                   /* indent on tracing                 */
  context->addBlock();                 /* add to the loop nesting           */
                                       /* do debug pause if necessary       */

                                       /* have to re-execute?               */
  if (context->conditionalPauseInstruction()) {
    context->removeBlock();            /* cause termination cleanup         */
    context->unindent();               /* step back trace indentation       */
  }
}

void RexxInstructionSelect::matchEnd(
     RexxInstructionEnd *partner,      /* end to match up                   */
     RexxSource         *source )      /* parsed source file (for errors)   */
/******************************************************************************/
/* Function:  Match an END instruction up with a SELECT                       */
/******************************************************************************/
{
  RexxInstructionIf    *when;          /* target WHEN clause                */
  LOCATIONINFO          location;      /* location of the end               */
  LONG                  lineNum;       /* Instruction line number           */

  partner->getLocation(&location);     /* get location of END instruction   */
  lineNum = this->lineNumber;          /* get the instruction line number   */
  if (partner->name != OREF_NULL)      /* END had a name specified?         */
                                       /* misplaced END instruction         */
    CurrentActivity->raiseException(Error_Unexpected_end_select, &location, source, OREF_NULL, new_array2(partner->name, new_integer(lineNum)), OREF_NULL);
  OrefSet(this, this->end, partner);   /* match up with the END instruction */
                                       /* get first item off of WHEN list   */
  when = (RexxInstructionIf *)(this->when_list->pullRexx());
                                       /* nothing there?                    */
  if (when == (RexxInstructionIf *)TheNilObject) {
    this->getLocation(&location);      /* get the location info             */
                                       /* need at least one WHEN here       */
    CurrentActivity->raiseException(Error_When_expected_when, &location, source, OREF_NULL, new_array1(new_integer(lineNum)), OREF_NULL);
  }
                                       /* link up each WHEN with the END    */
  while (when != (RexxInstructionIf *)TheNilObject) {
                                       /* hook up with the partner END      */
    when->fixWhen((RexxInstructionEndIf *)partner);
                                       /* get the next list item            */
    when = (RexxInstructionIf *)(this->when_list->pullRexx());
  }
                                       /* get rid of the lists              */
  OrefSet(this, this->when_list, OREF_NULL);
  if (this->otherwise != OREF_NULL)    /* an other wise block?              */
    partner->setStyle(OTHERWISE_BLOCK);/* END closes the OTHERWISE          */
  else
    partner->setStyle(SELECT_BLOCK);   /* SELECT with no otherwise          */
}

void RexxInstructionSelect::addWhen(
    RexxInstructionIf *when)           /* associated WHEN instruction       */
/******************************************************************************/
/* Function:  Associate a WHEN instruction with its surrounding SELECT        */
/******************************************************************************/
{
                                       /* add to the WHEN list queue        */
  this->when_list->pushRexx((RexxObject *)when);
}

void RexxInstructionSelect::setOtherwise(
    RexxInstructionOtherWise *otherwise) /* partner OTHERWISE for SELECT      */
/******************************************************************************/
/* Function:  Associate an OTHERSISE instruction with its surrounding SELECT  */
/******************************************************************************/
{
                                       /* save the otherwise partner        */
  OrefSet(this, this->otherwise, otherwise);
}


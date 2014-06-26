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
/* REXX Kernel                                                    DoBlock.hpp */
/*                                                                            */
/* Runtime processing of DO/LOOP variations.  This holds the state of active  */
/* loops and performs loop test operations.                                   */
/*                                                                            */
/******************************************************************************/
#ifndef Included_RexxDoBlock
#define Included_RexxDoBlock

class RexxBlockInstruction;

class RexxDoBlock : public RexxInternalObject {
 public:

    void *operator new(size_t);
    inline void *operator new(size_t size, void *ptr) {return ptr;};
    inline void  operator delete(void *) { ; }
    inline void  operator delete(void *, void *) { ; }

    RexxDoBlock(RexxBlockInstruction *, size_t);
    inline RexxDoBlock(RESTORETYPE restoreType) { ; };

    void live(size_t);
    void liveGeneral(int reason);
    void flatten(RexxEnvelope *);

    inline RexxBlockInstruction * getParent() {return this->parent;};
    inline void setPrevious(RexxDoBlock *block) { previous = block; }
    inline RexxDoBlock *getPrevious() { return previous; }

    inline void setControl(RexxVariableBase *v) { control = v; }
    inline void setTo(RexxObject * value) {to = value;};
    inline void setBy(RexxObject * value) {by = value;};
    inline void setFor(wholenumber_t value) {forVount = value;};
    inline void setCompare(int value) {compare = (unsigned short)value;};
    inline size_t getIndent() { return indent; };
    inline bool checkFor() {return (forCount--) <= 0;};
           bool checkControl(RexxActivation *context, RexxExpressionStack *stack, bool increment);
           bool checkOver(RexxActivation *context, RexxExpressionStack *stack);


protected:

    RexxDoBlock       *previous;         // previous stacked Do Block
    RexxBlockInstruction *parent;        // parent instruction
    size_t             indent;           // base indentation

    // start of variables representing control state.  This get
    // initialized by the loop instruction and are used by block test
    // operations.
    RexxVariableBase  *control;          // control variable for controlled loop
    RexxObject        *to;               // final target TO value
    RexxObject        *by;               // control increment value
    wholenumber_t      forCount;         // number of iterations
    int                compare;          // type of comparison
};
#endif

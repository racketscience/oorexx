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
/* REXX Kernel                                          RexxInternalStack.hpp   */
/*                                                                            */
/* Primitive Expression Stack Class Definitions                               */
/*                                                                            */
/******************************************************************************/

#ifndef Included_RexxInternalStack
#define Included_RexxInternalStack

class RexxInternalStack : public RexxInternalObject {
 public:
  inline void *operator new(size_t size, void *ptr) { return ptr;};
  RexxInternalStack() { ; }
  inline RexxInternalStack(RESTORETYPE restoreType) { ; }
  void live();
  void liveGeneral();
  void flatten(RexxEnvelope *);

  inline void         push(RexxObject *value) { *(++this->top) = value; };
  inline RexxObject * pop() { return *(this->top--); };
  inline RexxObject * fastPop() { return *(this->top--); };
  inline void         replace(size_t offset, RexxObject *value) { *(this->top - offset) = value; };
  inline size_t       getSize() {return this->size;};
  inline RexxObject * getTop()  {return *(this->top);};
  inline void         popn(size_t c) {this->top -= c;};
  inline void         clear() {this->top = this->stack;};
  inline RexxObject * peek(size_t v) {return *(this->top - v);};
  inline RexxObject **pointer(size_t v) {return (this->top - v); };
  inline size_t       location() {return this->top - this->stack;};
  inline void         setTop(size_t v) {this->top = this->stack + v;};
  inline void         toss() { this->top--; };
  inline BOOL         isEmpty() { return top == stack; }
  inline BOOL         isFull() { return top >= stack + size; }

  size_t size;                         /* size of the expstack              */
  RexxObject **top;                    /* current expstack top location     */
  RexxObject *stack[1];                /* actual stack values               */
};
#endif

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
/* REXX Kernel                                               StackClass.hpp   */
/*                                                                            */
/* Primitive Stack Class Definitions                                          */
/*                                                                            */
/******************************************************************************/
#ifndef Included_RexxStack
#define Included_RexxStack

 class RexxStack : public RexxInternalObject {
  public:
   inline void *operator new(size_t size, void *ptr) { return ptr; }
   void        *operator new(size_t, size_t, bool temporary = false);
   inline void  operator delete(void *) { }
   inline void  operator delete(void *, void *) { }
   inline void  operator delete(void *, size_t, bool temporary) { };

   inline RexxStack(RESTORETYPE restoreType) { ; };
   RexxStack(size_t size);

   void        init(size_t);
   void        live(size_t);
   void        liveGeneral(int reason);
   void        flatten(RexxEnvelope *);
   RexxObject *get(size_t pos);
   inline RexxObject *push(RexxObject *obj)
                   { incrementTop();
                     return *(this->stack + this->top) = obj;
                   }
   RexxObject *pop();
   RexxObject *fpop();

   inline void        fastPush(RexxObject *element) { this->stack[++(this->top)] = element; };
   inline bool        checkRoom() { return this->top < this->size-1; }
   inline RexxObject *fastPop() { return this->stack[(this->top)--]; };
   inline size_t      stackSize() { return this->size; };
   inline RexxObject *stackTop() { return (*(this->stack + this->top)); };
   inline void        decrementTop() { top = (top == 0) ? size - 1 : top - 1; }
   inline void        incrementTop() { if (++top >= size) top = 0; }
                                                                                                                                                          /* (other->size + 1) was wrong !? */
   inline void        copyEntries(RexxStack *other) { memcpy((char *)this->stack, other->stack, other->size * sizeof(RexxObject *)); this->top = other->top; }

   size_t   size;                      // the stack size
   size_t   top;                       /* top position on the stack         */
   RexxObject *stack[1];               /* stack entries                     */
 };

 class RexxSaveStack : public RexxStack {
  public:
   void       *operator new(size_t, size_t);
   inline void operator delete(void *, size_t) { }

   RexxSaveStack(size_t, size_t);
   void        live(size_t);
   void        init(size_t, size_t);
   void        extend(size_t);
   void        remove(RexxObject *, bool search = false);

   size_t allocSize;
 };

#endif

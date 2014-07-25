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
/* REXX Kernel                                             NativeMethod.hpp   */
/*                                                                            */
/* Primitive Native Code Class Definitions                                    */
/*                                                                            */
/******************************************************************************/
#ifndef Included_NativeCode
#define Included_NativeCode

#include "MethodClass.hpp"


/**
 * Base class for external methods and routines written in C++
 */
class NativeCode : public BaseCode
{
  public:

    inline NativeCode() { }
    NativeCode(RexxString *p, RexxString *n);

    virtual void live(size_t);
    virtual void liveGeneral(MarkReason reason);
    virtual void flatten(Envelope *envelope);

    virtual PackageClass *getPackageObject();
    virtual RexxClass *findClass(RexxString *className);
    virtual BaseCode *setPackageObject(PackageClass *s);
    SecurityManager *getSecurityManager();

protected:

    RexxString *packageName;           // the package name
    RexxString *name;                  // the mapped method name
    PackageClass *package;             // source this is attached to
};


/**
 * An external method written in C++
 */
class NativeMethod : public NativeCode
{
  public:
    inline void *operator new(size_t size, void *ptr) { return ptr; }
    void        *operator new(size_t size);
    inline void  operator delete(void *) { ; }
    inline void  operator delete(void *, void *) { ; }

    inline NativeMethod(RESTORETYPE restoreType) { ; };
    inline NativeMethod(RexxString *p, RexxString *n, PNATIVEMETHOD e) : NativeCode(p, n), entry(e) { }

    virtual void liveGeneral(MarkReason reason);
    virtual void flatten(Envelope *envelope);

    inline PNATIVEMETHOD getEntry() { return entry; }

    virtual void run(Activity *activity, MethodClass *method, RexxObject *receiver, RexxString *messageName,
        RexxObject **argPtr, size_t count, ProtectedObject &result);

protected:

    PNATIVEMETHOD entry;               // method entry point.
};


class RexxRoutine : public NativeCode
{
  public:

    inline RexxRoutine() { }
    inline RexxRoutine(RexxString *p, RexxString *n) : NativeCode(p, n) { }

    virtual void call(Activity *, RoutineClass *, RexxString *, RexxObject **, size_t, ProtectedObject &) = 0;
};


/**
 * An external routine written in C++
 */
class NativeRoutine : public RexxRoutine
{
  public:
    inline void *operator new(size_t size, void *ptr) { return ptr; }
    void        *operator new(size_t size);
    inline void  operator delete(void *) { ; }
    inline void  operator delete(void *, void *) { ; }

    inline NativeRoutine(RESTORETYPE restoreType) { ; };
    inline NativeRoutine(RexxString *p, RexxString *n, PNATIVEROUTINE e) : RexxRoutine(p, n), entry(e) { }

    virtual void liveGeneral(MarkReason reason);
    virtual void flatten(Envelope *envelope);

    inline PNATIVEROUTINE getEntry() { return entry; }

    virtual void call(Activity *, RoutineClass *, RexxString *, RexxObject **, size_t, ProtectedObject &);

protected:

    PNATIVEROUTINE entry;               // method entry point.
};


/**
 * A legacy-style external routine.
 */
class RegisteredRoutine : public RexxRoutine
{
  public:
    inline void *operator new(size_t size, void *ptr) { return ptr; }
    void        *operator new(size_t size);
    inline void  operator delete(void *) { ; }
    inline void  operator delete(void *, void *) { ; }

    virtual void liveGeneral(MarkReason reason);
    virtual void flatten(Envelope *envelope);

    inline RegisteredRoutine(RESTORETYPE restoreType) { ; };
    RegisteredRoutine(RexxString *n, RexxRoutineHandler *e)  : RexxRoutine(OREF_NULL, n), entry(e) { }
    RegisteredRoutine(RexxString *p, RexxString *n, RexxRoutineHandler *e)  : RexxRoutine(p, n), entry(e) { }

    virtual void call(Activity *, RoutineClass *, RexxString *, RexxObject **, size_t, ProtectedObject &);

    inline RexxRoutineHandler *getEntry() { return entry; }

protected:

    RexxRoutineHandler *entry;          // method entry point.
};

#endif

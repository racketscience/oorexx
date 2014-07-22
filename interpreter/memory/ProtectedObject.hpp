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
#ifndef ProtectedObject_Included
#define ProtectedObject_Included

#include "Activity.hpp"
#include "ActivityManager.hpp"

class RexxInstruction;

/**
 * Base class for a protected object.
 */
class ProtectedBase
{
friend class Activity;
public:
    inline ProtectedBase()
    {
        // save the activity
        activity = ActivityManager::currentActivity;

        // it would be better to have the activity class do this, but because
        // we're doing this with inline methods, we run into a bit of a
        // circular reference problem

        // NOTE:  ProtectedObject gets used in a few places during image
        // restore before we have a valid activity.  If we don't have
        // one, then just assume this will be safe.
        if (activity != OREF_NULL)
        {
            next = activity->protectedObjects;
            activity->protectedObjects = this;
        }
    }

    inline ProtectedBase(Activity *a) : activity(a)
    {
        // it would be better to have the activity class do this, but because
        // we're doing this with inline methods, we run into a bit of a
        // circular reference problem

        // NOTE:  ProtectedObject gets used in a few places during image
        // restore before we have a valid activity.  If we don't have
        // one, then just assume this will be safe.
        if (activity != OREF_NULL)
        {
            next = activity->protectedObjects;
            activity->protectedObjects = this;
        }
    }

    inline ~ProtectedBase()
    {
        // remove ourselves from the list.

        // NOTE:  ProtectedObject gets used in a few places during image
        // restore before we have a valid activity.  If we don't have
        // one, then just assume this will be safe.
        if (activity != OREF_NULL)
        {
            activity->protectedObjects = next;
        }
    }

    virtual void mark(size_t liveMark) = 0;
    virtual void markGeneral(int reason) = 0;

protected:

    ProtectedBase  *next;                // the pointer protected by the object
    Activity   *activity;            // the activity we're running on
};


/**
 * Normal untyped ProtectedObject class.  This protects
 * an object reference from garbage collection.
 */
class ProtectedObject : public ProtectedBase
{
 public:
    inline ProtectedObject() : protectedObject(OREF_NULL), ProtectedBase() { }
    inline ProtectedObject(Activity *a) : protectedObject(OREF_NULL), ProtectedBase(a) { }
    inline ProtectedObject(RexxInternalObject *o) : protectedObject(o), ProtectedBase() { }
    inline ProtectedObject(RexxInternalObject *o, Activity *a) : protectedObject(o), ProtectedBase(a) { }
    inline ~ProtectedObject()
    {
        if (protectedObject != OREF_NULL)
        {
            holdObject((RexxObject *)protectedObject);
        }
    }

    virtual void mark(size_t liveMark) { memory_mark(protectedObject); }
    virtual void markGeneral(int reason) { memory_mark_general(protectedObject); }

    inline ProtectedBase & operator=(RexxInternalObject *o)
    {
        protectedObject = o;
        return *this;
    }

    inline bool operator == (RexxInternalObject *o)
    {
        return protectedObject == o;
    }

    inline bool operator != (RexxInternalObject *o)
    {
        return protectedObject != o;
    }

    // cast conversion operators for some very common uses of protected object.
    inline operator RexxObject *()
    {
        return (RexxObject *)protectedObject;
    }

    // cast conversion operators for some very common uses of protected object.
    inline operator RexxInternalObject *()
    {
        return protectedObject;
    }

    inline operator RexxObjectPtr ()
    {
        return (RexxObjectPtr)protectedObject;
    }

    inline operator RexxString *()
    {
        return (RexxString *)protectedObject;
    }

    inline operator MethodClass *()
    {
        return (MethodClass *)protectedObject;
    }

    inline operator ArrayClass *()
    {
        return (ArrayClass *)protectedObject;
    }

    // this conversion helps the parsing process protect objects
    inline operator RexxInstruction *()
    {
        return (RexxInstruction *)protectedObject;
    }

    inline operator void *()
    {
        return (void *)protectedObject;
    }

    inline bool isNull()
    {
        return protectedObject == OREF_NULL;
    }

protected:
    RexxInternalObject *protectedObject;   // The object we're protecting
};


/**
 * Typed version of a protected object.  Because this
 * uses templates, it is possible to use these like
 * normal pointers to invoke methods.  More useful where
 * operations need to be performed on a protected object
 * since it avoids lots of cast operations.
 */
class ProtectedSet : public ProtectedObject
{
public:
    inline ProtectedSet() : ProtectedObject() { }
    inline ProtectedSet(Activity *a) : ProtectedObject(a) { }
    inline ~ProtectedSet() { }

    void add(RexxObject *);
};


template <class objType> class Protected : public ProtectedBase
{
public:
    inline Protected() : protectedObject(OREF_NULL), ProtectedBase() { }
    inline Protected(Activity *a) : protectedObject(OREF_NULL), ProtectedBase(a) { }
    inline Protected(objType *o) : protectedObject(o), ProtectedBase() { }
    inline Protected(objType *o, Activity *a) : protectedObject(o), ProtectedBase(a) { }

    inline ~Protected()
    {
        // give the protected object some hold protection.
        if (protectedObject != OREF_NULL)
        {
            holdObject(protectedObject);
        }
    }

    virtual void mark(size_t liveMark) { memory_mark(protectedObject); }
    virtual void markGeneral(int reason) { memory_mark_general(protectedObject); }

    inline ProtectedBase & operator=(objType *o)
    {
        protectedObject = o;
        return *this;
    }

    inline bool operator == (objType *o)
    {
        return protectedObject == o;
    }

    // cast conversion operators for some very common uses of protected object.
    inline operator RexxObject *()
    {
        return (RexxObject *)protectedObject;
    }

    inline operator RexxObjectPtr ()
    {
        return (RexxObjectPtr)protectedObject;
    }

    inline operator objType *()
    {
        return protectedObject;
    }

    inline operator void *()
    {
        return (void *)protectedObject;
    }

    inline bool isNull()
    {
        return protectedObject == OREF_NULL;
    }

    // pointer access
    inline objType * operator->()
    {
        return protectedObject;
    }

protected:

    objType *protectedObject;          // the protected object.
};


#endif

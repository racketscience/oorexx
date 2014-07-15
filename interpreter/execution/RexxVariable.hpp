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
/* REXX Kernel                                           RexxVariable.hpp     */
/*                                                                            */
/* Primitive Variable Class Definition                                        */
/*                                                                            */
/******************************************************************************/
#ifndef Included_RexxVariable
#define Included_RexxVariable

#include "StringClass.hpp"
#include "ObjectClass.hpp"

class RexxVariable : public RexxInternalObject
{
 public:
    void *operator new(size_t);
    inline void *operator new(size_t size, void *objectPtr) { return objectPtr; };
    inline void  operator delete(void *, void *) {;}
    inline void  operator delete(void *) { }

    inline RexxVariable() : variable_name(OREF_NULL), variableValue(OREF_NULL), creator(OREF_NULL), dependents(OREF_NULL) {;};
    inline RexxVariable(RexxString *n) : variable_name(n), variableValue(OREF_NULL), creator(OREF_NULL), dependents(OREF_NULL) {;};
    inline RexxVariable(RESTORETYPE restoreType) { ; };

    virtual void live(size_t);
    virtual void liveGeneral(MarkReason reason);
    virtual void flatten(RexxEnvelope *);

    void         inform(RexxActivity *);
    void         drop();
    void         notify();
    void         uninform(RexxActivity *);
    void         setStem(RexxObject *);

    inline void set(RexxObject *value)
    {
        setField(variableValue, value);
        if (dependents != OREF_NULL)
        {
            notify();
        }
    };

    inline RexxObject *getVariableValue() { return variableValue; };
    inline RexxObject *getResolvedValue() { return variableValue != OREF_NULL ? variableValue : (RexxObject *)variable_name; };
    inline RexxString *getName() { return variable_name; }
    inline void setName(RexxString *name) { OrefSet(this, variable_name, name); }

    inline void reset(RexxString *name)
    {
        creator       = OREF_NULL;        /* this is unowned                   */
        variableValue = OREF_NULL;        /* clear out the hash value          */
        variable_name = name;             /* fill in the name                  */
        dependents = OREF_NULL;           /* and the dependents                */
    }

    // Note:  This does not use OrefSet since it will only occur with
    // local variables that can never be part of oldspace;
    inline void setCreator(RexxActivation *creatorActivation) { creator = creatorActivation; }
    inline RexxVariable *getNext() { return (RexxVariable *)variableValue; }
    inline void cache(RexxVariable *next) { reset(OREF_NULL); variableValue = (RexxObject *)next; }
    inline bool isLocal(RexxActivation *act) { return act == creator; }
    inline bool isStem() { return variable_name->endsWith('.'); }

protected:

    RexxString *variable_name;           // the name of the variable
    RexxObject *variableValue;           // the assigned value of the variable.
    RexxActivation *creator;             // the activation that created this variable
    IdentityTable  *dependents;      // guard expression dependents
};


inline RexxVariable *new_variable(RexxString *n) { return new RexxVariable(n); }

#endif

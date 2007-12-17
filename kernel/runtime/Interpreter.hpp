/*----------------------------------------------------------------------------*/
/*                                                                            */
/* Copyright (c) 1995, 2004 IBM Corporation. All rights reserved.             */
/* Copyright (c) 2005-2006 Rexx Language Association. All rights reserved.    */
/*                                                                            */
/* This program and the accompanying materials are made available under       */
/* the terms of the Common Public License v1.0 which accompanies this         */
/* distribution. A copy is also available at the following address:           */
/* http://www.ibm.com/developerworks/oss/CPLv1.0.htm                          */
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
/*****************************************************************************/
/* REXX Windows Support                                                      */
/*                                                                           */
/* Main interpreter control.  This is the preferred location for all         */
/* platform independent global variables.                                    */
/* The interpreter does not instantiate an instance of this                  */
/* class, so most variables and methods should be static.                    */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

#ifndef RexxInterpreter_Included
#define RexxInterpreter_Included

#include "RexxCore.h"

class Interpreter
{
public:
    static inline void getResourceLock() { MTXRQ(resourceLock); }
    static inline void releaseResourceLock() { MTXRL(resourceLock); }
    static inline void createLocks()
    {
        MTXCROPEN(resourceLock, "OBJREXXRESSEM");
        // this needs to be created and set
        EVCR(terminationSem);
        EVSET(terminationSem);
    }

    static inline void closeLocks()
    {
        MTXCL(resourceLock);
        EVCLOSE(terminationSem);
        terminationSem = 0;
    }

    static inline void signalTermination()
    {
        EVPOST(terminationSem);              /* let anyone who cares know we're done*/
    }

    static void terminate();
    static bool isTerminated();


protected:
    static SMTX   resourceLock;      // use to lock resources accessed outside of kernel global lock
    static SEV    terminationSem;    // used to signal that everything has shutdown
};


/**
 * Block control for access to the resource lock.
 */
class ResourceSection
{
public:
    inline ResourceSection()
    {
        Interpreter::getResourceLock();
        terminated = false;
    }

    inline ~ResourceSection()
    {
        if (!terminated)
        {
            Interpreter::releaseResourceLock();
        }
    }

    inline void release()
    {
        if (!terminated)
        {
            Interpreter::releaseResourceLock();
            terminated = true;
        }
    }


    inline void reacquire()
    {
        if (terminated)
        {
            Interpreter::getResourceLock();
            terminated = false;
        }
    }

private:

    bool terminated;       // we can release these as needed
};


#endif

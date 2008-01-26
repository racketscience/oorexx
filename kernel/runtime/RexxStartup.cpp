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
/* REXX Kernel                                                                */
/*                                                                            */
/* Startup                                                                    */
/*                                                                            */
/******************************************************************************/
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include "RexxCore.h"
#include "RexxMemory.hpp"
#include "StringClass.hpp"
#include "DirectoryClass.hpp"
#include "RexxActivity.hpp"
#include "ActivityManager.hpp"
#include "MethodClass.hpp"
#include "RexxNativeAPI.h"
#include "StackClass.hpp"
#include "Interpreter.hpp"
#include "TranslateDispatcher.hpp"
#include "RexxStartDispatcher.hpp"
#include "CreateMethodDispatcher.hpp"
#include "InterpreterInstance.hpp"
#include "RexxNativeActivation.hpp"
#include "RexxInternalApis.h"


int REXXENTRY RexxTerminate()
/******************************************************************************/
/* Function:  Terminate the REXX interpreter...will only terminate if the     */
/*            call nesting level has reached zero.                            */
/******************************************************************************/
{
    // terminate and clean up the interpreter runtime.  This only works
    // if there are no active instances
    return Interpreter::terminateInterpreter() ? 0 : 1;
}

int REXXENTRY RexxInitialize ()
/******************************************************************************/
/* Function:  Perform main kernel initializations                             */
/******************************************************************************/
{
    // start this up for normal execution
    Interpreter::startInterpreter(Interpreter::RUN_MODE);
    // this always returns true
    return true;
}

int REXXENTRY RexxQuery ()
/******************************************************************************/
/* Function:  Determine if the REXX interpreter is initialized and active     */
/******************************************************************************/
{
    // see if we have an active environment
    return Interpreter::isActive();
}


/**
 * Create the Rexx saved image during build processing.
 *
 * @return Nothing
 */
void REXXENTRY RexxCreateInterpreterImage()
{
    // start this up and save the image.  This never returns to here
    Interpreter::startInterpreter(Interpreter::SAVE_IMAGE_MODE);
}



/******************************************************************************/
/* Name:       RexxMain                                                       */
/*                                                                            */
/* Arguments:  argcount - Number of args in arglist                           */
/*             arglist - Array of args (array of RXSTRINGs)                   */
/*             programname - REXX program to run                              */
/*             instore - Instore array (array of 2 RXSTRINGs)                 */
/*             envname - Initial cmd environment                              */
/*             calltype - How the program is called                           */
/*             exits - Array of system exit names (array of RXSTRINGs)        */
/*                                                                            */
/* Returned:   result - Result returned from program                          */
/*             rc - Return code from program                                  */
/*                                                                            */
/* Notes:  Primary path into Object REXX.  Makes sure Object REXX is up       */
/*   and runs the requested program.                                          */
/*                                                                            */
/******************************************************************************/
int REXXENTRY RexxStart(
  size_t argcount,                     /* Number of args in arglist         */
  PCONSTRXSTRING arglist,              /* Array of args                     */
  const char *programname,             /* REXX program to run               */
  PRXSTRING instore,                   /* Instore array                     */
  const char *envname,                 /* Initial cmd environment           */
  int   calltype,                      /* How the program is called         */
  PRXSYSEXIT exits,                    /* Array of system exit names        */
  short * retcode,                     /* Integer form of result            */
  PRXSTRING result)                    /* Result returned from program      */
{
    if (calltype == RXCOMMAND && argcount == 1 && arglist[0].strptr != NULL && StringUtil::caselessCompare(arglist[0].strptr, "//T", arglist[0].strlength) == 0)
    {
        TranslateDispatcher arguments(exits);
        arguments.programName = programname;
        arguments.instore = instore;
        // this just translates and gives the error, potentially returning
        // the instore image
        arguments.outputName = NULL;
        // go run this program
        arguments.invoke();

        return (int)arguments.rc;      /* return the error code (negated)   */
    }


    // this is the dispatcher that handles the actual
    // interpreter call.  This gets all of the RexxStart arguments, then
    // gets dispatched on the other side of the interpreter boundary
    RexxStartDispatcher arguments(exits, envname);
                                       /* copy all of the arguments into    */
                                       /* the info control block, which is  */
                                       /* passed across the kernel boundary */
                                       /* into the real RexxStart method    */
                                       /* this is a real execution          */
    arguments.argcount = argcount;
    arguments.arglist = arglist;
    arguments.programName = programname;
    arguments.instore = instore;
    arguments.calltype = calltype;
    arguments.retcode = 0;
    arguments.result = result;

    // go run this program
    arguments.invoke();
    *retcode = arguments.retcode;

    return (int)arguments.rc;          /* return the error code (negated)   */
}



/**
 * Translate a program and store the translated results in an
 * external file.
 *
 * @param inFile  The input source file.
 * @param outFile The output source.
 * @param exits   The exits to use during the translation process.
 *
 * @return The error return code (if any).
 */
APIRET REXXENTRY RexxTranslateProgram(const char *inFile, const char *outFile, PRXSYSEXIT exits)
{
    TranslateDispatcher arguments(exits);
    // this gets processed from disk, always.
    arguments.programName = inFile;
    arguments.instore = NULL;
    // this just translates and gives the error, potentially returning
    // the instore image
    arguments.outputName = outFile;
    // go run this program
    arguments.invoke();

    return (APIRET)arguments.rc;       /* return the error code (negated)   */
}


/**
 * Create a new stripting context used for RexxRunMethod, et. al.
 *
 * @param contextName
 *               The name of the scripting engine context.
 */
void REXXENTRY RexxCreateScriptContext(const char *contextName)
{
    // Get an instance.  This also gives the root activity of the instance
    // the kernel lock.
    InstanceBlock instance;
    // now create a directory and hang it off of the local environment.  This
    // will persist in the environment until the entire interpreter environment is
    // terminated.
    RexxString *context = new_string(contextName);
    ActivityManager::localEnvironment->put(new_directory(), context);
}


/**
 * Destroy a script context created using RexxCreateScriptContext.
 *
 * @param contextName
 *               The name of the target context.
 *
 * @return nothing.
 */
void REXXENTRY RexxDestroyScriptContext(const char *contextName)
{
    // Get an instance.  This also gives the root activity of the instance
    // the kernel lock.
    InstanceBlock instance;

    // delete the named context from the local environment.
    RexxString *context = new_string(contextName);
    ActivityManager::localEnvironment->remove(context);
}


/**
 * Release an object reference stored within the named scripting
 * context.
 *
 * @param contextName
 *               The name of the context.
 * @param obj    The object reference to release.
 *
 * @return true if this object was held in the target context, false if
 *         it was not found.
 */
int REXXENTRY RexxReleaseScriptReference(const char *contextName, REXXOBJECT obj)
{
    // Get an instance.  This also gives the root activity of the instance
    // the kernel lock.
    InstanceBlock instance;

    // delete the named context from the local environment.
    RexxString *context = new_string(contextName);
    RexxDirectory *locked_objects = (RexxDirectory *)ActivityManager::localEnvironment->at(context);
    // the value used in our directory is the string value of the
    // method pointer
    char buffer[32];
    sprintf(buffer, "0x%p", obj);
    REXXOBJECT oldObject = locked_objects->remove(new_string(buffer));

    // the return code indicates the removed object matches the input object
    return oldObject == obj;
}


/**
 * Create a new invokable method within an given scripting
 * context.
 *
 * @param context    The scripting context name.
 * @param sourceData The source for the program to transform into a method.
 * @param pmethod    The returned method object reference.
 * @param pRexxCondData
 *                   Any error condition data returned for a translation failure.
 *
 * @return The translation return code.
 */
APIRET REXXENTRY RexxCreateMethod(const char *context, PCONSTRXSTRING sourceData,
  REXXOBJECT   *pmethod, RexxConditionData *pRexxCondData)
{
    // create the dispatcher
    CreateMethodDispatcher arguments(pRexxCondData);

    arguments.contextName = context;
    arguments.programBuffer = *sourceData;
    arguments.translatedMethod = NULL;

    // go perform the operation
    arguments.invoke();
    // fill in the return value
    *pmethod = arguments.translatedMethod;

    return (APIRET)arguments.rc;    /* return the error code             */
}


/**
 * Run a method in a scripting context.
 *
 * @param context   The name of the script context (used to anchor returned results).
 * @param method    The method to run.
 * @param callbackArgs
 *                  Opaque argument function used by the argument callback for
 *                  marshaling arguments into ooRexx objects.
 * @param callbackFunction
 *                  The callback function used for argument marshalling.
 * @param exit_list The set of exits used to run this method.
 * @param presult   The return result value.
 * @param securityManager
 *                  The security manager to use for running this method.
 * @param pRexxCondData
 *                  Any error condition information returned to the caller.
 *
 * @return The return code from running this program (0 indicates success).
 */
APIRET REXXENTRY RexxRunMethod(const char * context, REXXOBJECT method, void * callbackArgs,
  REXXOBJECT (REXXENTRY *callbackFunction)(void *), PRXSYSEXIT exit_list, REXXOBJECT *presult,
  REXXOBJECT securityManager, RexxConditionData *pRexxCondData)        /* returned condition data           */
{
    RunMethodDispatcher arguments(exit_list, pRexxCondData);

    arguments.contextName = context;
    arguments.method = method;
    arguments.callbackArguments = callbackArgs;
    arguments.argumentCallback = callbackFunction;
    arguments.securityManager = securityManager;

    // make the call
    arguments.invoke();
    // fill in the return value
    *presult = arguments.result;

    return (APIRET)arguments.rc;          /* return the error code             */
}


/**
 * Retrieve the interpreter version information.
 *
 * @return
 */
char *REXXENTRY RexxGetVersionInformation()
{
    char ver[100];
    sprintf( ver, " %d.%d.%d", ORX_VER, ORX_REL, ORX_MOD );
    char vbuf0[] = "Open Object Rexx %s Version";
  #ifdef _DEBUG
    char vbuf1[] = " - Internal Test Version\nBuild date: ";
  #else
    char vbuf1[] = "\nBuild date: ";
  #endif
    char vbuf2[] = "\nCopyright (c) IBM Corporation 1995, 2004.\nCopyright (c) RexxLA 2005-2008.\nAll Rights Reserved.";
    char vbuf3[] = "\nThis program and the accompanying materials";
    char vbuf4[] = "\nare made available under the terms of the Common Public License v1.0";
    char vbuf5[] = "\nwhich accompanies this distribution.";
    char vbuf6[] = "\nhttp://www.oorexx.org/license.html";
    size_t s0 = strlen(vbuf0);
    size_t s1 = strlen(vbuf1);
    size_t s2 = strlen(vbuf2);
    size_t s3 = strlen(vbuf3);
    size_t s4 = strlen(vbuf4);
    size_t s5 = strlen(vbuf5);
    size_t s6 = strlen(vbuf6);
    size_t sd = strlen(__DATE__);
    size_t sv = strlen(ver);
    char *ptr = (char *)SysAllocateResultMemory(sv+s0+s1+s2+s3+s4+s5+s6+sd+1);
    if (ptr)
    {
        sprintf(ptr, "%s%s%s%s%s%s%s%s%s", vbuf0, ver, vbuf1, __DATE__, vbuf2, vbuf3, vbuf4, vbuf5, vbuf6);
    }
    return ptr;
}


/**
 * Raise a halt condition for a target thread.
 *
 * @param threadid The target threadid.
 *
 * @return RXARI_OK if this worked, RXARI_NOT_FOUND if the thread isn't
 *         active.
 */
APIRET REXXENTRY RexxHaltThread(thread_id_t threadid)
{
    if (RexxQuery())
    {                        /* Are we up?                     */
       if (!ActivityManager::haltActivity(threadid, OREF_NULL))
       {
           return (RXARI_NOT_FOUND);             /* Couldn't find threadid         */
       }
       return (RXARI_OK);
    }
    return RXARI_NOT_FOUND;     /* REXX not running, error...     */
}


/**
 * Compatibility function for doing a RexxHaltThread().
 *
 * @param procid   The process id (ignored).
 * @param threadid The target threadid
 *
 * @return the success/failure return code.
 */
APIRET REXXENTRY RexxSetHalt(process_id_t procid, thread_id_t threadid)
{
    return RexxHaltThread(threadid);
}


/**
 * Turn on tracing for a given interpreter thread.
 *
 * @param threadid The target thread identifier.
 *
 * @return the success/failure return code.
 */
APIRET REXXENTRY RexxSetThreadTrace(thread_id_t threadid)
{
    if (RexxQuery())
    {
       if (!ActivityManager::setActivityTrace(threadid, true))
       {
           return (RXARI_NOT_FOUND);             /* Couldn't find threadid         */
       }
       return (RXARI_OK);
    }
    return RXARI_NOT_FOUND;     /* REXX not running, error...     */
}


/**
 * Reset the external trace for a target thread.
 *
 * @param threadid The target thread id.
 *
 * @return The success/failure indicator.
 */
APIRET REXXENTRY RexxResetThreadTrace(thread_id_t threadid)
{
    if (RexxQuery())
    {
       if (!ActivityManager::setActivityTrace(threadid, false))
       {
           return (RXARI_NOT_FOUND);             /* Couldn't find threadid         */
       }
       return (RXARI_OK);
    }
    return RXARI_NOT_FOUND;     /* REXX not running, error...     */
}


/**
 * Compatibility stub for the old signature of RexxSetTrace.
 *
 * @param procid   The process id (ignored).
 * @param threadid The target thread identifier.
 *
 * @return the success/failure return code.
 */
APIRET REXXENTRY RexxSetTrace(process_id_t procid, thread_id_t threadid)
{
    return RexxSetThreadTrace(threadid);
}


/**
 * The compatibility stub for the reset trace API.
 *
 * @param procid   The target process id (ignored).
 * @param threadid The thread id of the target thread.
 *
 * @return The success/failure indicator.
 */
APIRET REXXENTRY RexxResetTrace(process_id_t procid, thread_id_t threadid)
{
    return RexxResetThreadTrace(threadid);
}


/**
 * Retrieve the current digits setting for an external context.
 *
 * @param precision The current precision.
 */
size_t REXXENTRY RexxGetCurrentPrecision()
{
    NativeContextBlock context;
    // get the digits setting from the current context.
    return context.self->digits();
}

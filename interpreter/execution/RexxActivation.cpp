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
/* REXX Kernel                                                                */
/*                                                                            */
/* Primitive Activation Class                                                 */
/*                                                                            */
/* NOTE:  activations are an execution time only object.  They are never      */
/*        flattened or saved in the image, and hence will never be in old     */
/*        space.  Because of this, activations "cheat" and do not use         */
/*        OrefSet to assign values to get better performance.  Care must be   */
/*        used to maintain this situation.                                    */
/*                                                                            */
/******************************************************************************/
#include "RexxCore.h"
#include "StringClass.hpp"
#include "BufferClass.hpp"
#include "DirectoryClass.hpp"
#include "VariableDictionary.hpp"
#include "RexxActivation.hpp"
#include "Activity.hpp"
#include "MethodClass.hpp"
#include "MessageClass.hpp"
#include "RexxCode.hpp"
#include "RexxInstruction.hpp"
#include "CallInstruction.hpp"
#include "DoBlock.hpp"
#include "DoInstruction.hpp"
#include "ProtectedObject.hpp"
#include "ActivityManager.hpp"
#include "Interpreter.hpp"
#include "SystemInterpreter.hpp"
#include "RexxInternalApis.h"
#include "PackageManager.hpp"
#include "CompoundVariableTail.hpp"
#include "CommandHandler.hpp"
#include "ActivationFrame.hpp"
#include "StackFrameClass.hpp"
#include "InterpreterInstance.hpp"
#include "PackageClass.hpp"
#include "RoutineClass.hpp"
#include "LanguageParser.hpp"

// max instructions without a yield
const size_t MAX_INSTRUCTIONS = 100;

/**
 * Create a new activation object
 *
 * @param size   Base size of the object.
 *
 * @return Storage for building an activation object.
 */
void * RexxActivation::operator new(size_t size)
{
    return new_object(size, T_Activation);
}


/**
 * Initialize an activation for direct caching in the activation
 * cache.  At this time, this is not an executable activation
 */
RexxActivation::RexxActivation()
{
    setHasNoReferences();          // nothing referenced from this either
}


/**
 * Initialize an activation for a method invocation.
 *
 * @param _activity The activity we're running under.
 * @param _method   The method being invoked.
 * @param _code     The code to execute.
 */
RexxActivation::RexxActivation(Activity* _activity, MethodClass * _method, RexxCode *_code)
{
    clearObject();                 // start with a fresh object
    activity = _activity;          // save the activity pointer
    scope = _method->getScope();   // save the scope
    code = _code;                  // the code we're executing
    executable = _method;          // save this as the base executable
                                   // save the source object reference also
    packageObject = _method->getPackageObject();
    settings.intermediateTrace = false;
    activationContext = METHODCALL;  // the context is a method call
    parent = OREF_NULL;              // we don't have a parent stack frame when invoked as a method
    executionState = ACTIVE;         // we are now in active execution
    objectScope = SCOPE_RELEASED;    // scope not reserved yet

    // create a new evaluation stack.  This must be done before a
    // local variable frame is created.

    // get our evaluation stack
    allocateStackFrame();

    // initialize from the package-defined settings
    inheritPackageSettings();

    if (_method->isGuarded())            // make sure we set the appropriate guarded state
    {
        setGuarded();
    }

    settings.parentCode = code;

    // allocate a frame for the local variables from activity stack
    allocateLocalVariables();

    // set the initial and initial alternate address settings
    settings.currentAddress = activity->getInstance()->getDefaultEnvironment();
    settings.alternateAddress = settings.currentAddress;
    // get initial random seed value
    randomSeed = activity->getRandomSeed();
    // copy the security manager, and pick up the instance one if nothing is set.
    settings.securityManager = code->getSecurityManager();
    if (settings.securityManager == OREF_NULL)
    {
        settings.securityManager = activity->getInstanceSecurityManager();
    }
    // and the call type is METHOD
    settings.calltype = OREF_METHODNAME;
}


/**
 * Create a new Rexx activation for an internal level call.
 * An internal level call is an internal call, a call trap,
 * an Interpret statement, or a debug pause execution.
 *
 * @param _activity The current activity.
 * @param _parent   The parent activation.
 * @param _code     The code to be executed.  For interpret and debug pauses, this
 *                  is a new code object.  For call activations, this is the
 *                  parent code object.
 * @param context   The type of call being made.
 */
RexxActivation::RexxActivation(Activity *_activity, RexxActivation *_parent, RexxCode *_code, ActivationContext context)
{
    clearObject();                 // start with a fresh object
    activity = _activity;          // save the activity pointer
    code = _code;                  // get the code we're going to execute

    // if this is a debug pause, then flip on the debug pause flag
    // so we know we're executing the pause, but execute this as an
    // INTERPRET call.
    if (context == DEBUGPAUSE)
    {
        debugPause = true;
        context = INTERPRET;
    }

    activationContext = context;
    settings.intermediateTrace = false;

    // the sender is our parent activity
    parent = _parent;
    executionState = ACTIVE;      // we are now in active execution
    objectScope = SCOPE_RELEASED; // internal calls don't have a scope

    // get our evaluation stack
    allocateStackFrame();

    // inherit parents settings
    _parent->putSettings(settings);
    // step the trace indentation level for this internal nesting
    settings.traceIndent++;
    // the random seed is copied from the calling activity, this led
    // to reproducable random sequences even though no specific seed was given!
    adjustRandomSeed();

    // if we are doing an internal call, we've inherited our
    // caller's trap state, but if we change anything, then we need
    // to create a new set of tables so that we don't change the caller's
    // settings as well.
    if (context == INTERNALCALL)
    {
        settings.stateFlags[trapsCopied] = false;
        settings.stateFlags[replyIssued] = false;
        // invalidate the timestamp...interpret or debug pauses use the old timestamp.
        settings.timestamp.valid = false;
    }

    // this is a nested call until we issue a procedure *
    settings.localVariables.setNested();
    // get the executable from the parent.
    executable = _parent->getExecutable();
    // for internal calls, this is the same source object as the parent
    if (isInterpret())
    {
        // use the source object for the interpret so error tracebacks are correct.
        packageObject = code->getPackageObject();
    }
    else
    {
        // save the source object reference also
        packageObject = executable->getPackageObject();
    }
}


/**
 * Create a top-level activation of Rexx code.  This will
 * either a toplevel program or an external call.
 *
 * @param _activity The current thread we're running on.
 * @param _routine  The routine to invoke.
 * @param _code     The code object to be executed.
 * @param calltype  Type type of call being made (function or subroutine)
 * @param env       The default address environment
 * @param context   The type of call context.
 */
RexxActivation::RexxActivation(Activity *_activity, RoutineClass *_routine, RexxCode *_code,
    RexxString *calltype, RexxString *env, ActivationContext context)
{
    clearObject();
    activity = _activity;
    code = _code;                  // the code comes from the routine object...
    executable = _routine;         // and the routine is our executable
                                   // save the source object reference also
    packageObject = _routine->getPackageObject();

    activationContext = context;
    settings.intermediateTrace = false;
    parent = OREF_NULL;            // there's no parent for a top level call
    executionState = ACTIVE;       // we are now in active execution
    objectScope = SCOPE_RELEASED;  // scope not reserved yet

    // a live marking can happen without a properly set up stack (::live()
    // is called). Setting the NoRefBit when creating the stack avoids it.
    setHasNoReferences();
    _activity->allocateStackFrame(&stack, code->getMaxStackSize());
    setHasReferences();

    // initialize with the package-defined settings
    inheritPackageSettings();

    // save the source also
    settings.parentCode = code;

    // allocate a frame for the local variables from activity stack
    allocateLocalVariables();

    // we use default address settings
    settings.currentAddress = activity->getInstance()->getDefaultEnvironment();
    settings.alternateAddress = settings.currentAddress;

    // this is a top level call, so we get a fresh random seed
    randomSeed = activity->getRandomSeed();

    // the random seed is copied from the calling activity, this led
    // to reproducable random sequences even though no specific seed was given!
    adjustRandomSeed();

    // copy the source security manager
    settings.securityManager = code->getSecurityManager();
    // but use the default if not set
    if (settings.securityManager == OREF_NULL)
    {
        settings.securityManager = activity->getInstanceSecurityManager();
    }

    // if we have a default environment specified, apply the override.
    if (env != OREF_NULL)
    {
        setDefaultAddress(env);
    }
    // set the call type
    if (calltype != OREF_NULL)
    {
        settings.calltype = calltype;
    }
}


/**
 * Allocate a stack frame for this activation to use
 * for the evaluation stack.
 */
void RexxActivation::allocateStackFrame()
{
    // a live marking can happen without a properly set up stack (::live()
    // is called). Setting the NoRefBit when creating the stack avoids it.
    setHasNoReferences();
    _activity->allocateStackFrame(&stack, code->getMaxStackSize());
    setHasReferences();
}


/**
 * Allocate a new local variable frame.
 */
void RexxActivation::allocateLocalVariables()
{
    // allocate a frame for the local variables from activity stack
    settings.localVariables.init(this, code->getLocalVariableSize());
    activity->allocateLocalVariableFrame(&settings.localVariables);
}


/**
 * Inherit the settings from our package object.
 */
void RexxActivation::inheritPackageSettings()
{
    // and override with the package-defined settings

    // TODO:  Create a NumericSettings class and a package settings class that includes the
    // numeric settings and the trace settings.
    settings.numericSettings.digits = packageObject->getDigits();
    settings.numericSettings.fuzz = packageObject->getFuzz();
    settings.numericSettings.form = packageObject->getForm();
    setTrace(packageObject->getTraceSetting());
}


/**
 * Re-dispatch an activation after a REPLY
 *
 * @return The result object.
 */
RexxObject * RexxActivation::dispatch()
{
    ProtectedObject r;
    // we just resume running at the old location, reusing the intial values.
    return run(receiver, settings.messageName, argList, argCount, OREF_NULL, r);
}


/**
 * Run some Rexx code...this is it!  This is the heart of the
 * interpreter that makes the whole thing run!
 *
 * @param _receiver The target receiver object (if a method call)
 * @param name      The message, routine, or program name.
 * @param _arglist  The argument list.
 * @param _argcount The count of arguments.
 * @param start     The starting instruction.
 * @param resultObj A protected object for returning a result.
 *
 * @return The execution result (also returned via the protected object.)
 */
RexxObject * RexxActivation::run(RexxObject *_receiver, RexxString *name, RexxObject **_arglist,
     size_t _argcount, RexxInstruction * start, ProtectedObject &resultObj)
{
    // add the frame to the execution stack
    RexxActivationFrame frame(activity, this);

#ifndef FIXEDTIMERS
    // this is the number of instructions to run without yielding
    size_t instructionCount = 0;
#endif
    receiver = _receiver;
    // the "msgname" can also be the name of an external routine, the label
    // name of an internal routine.
    settings.messageName = msgname;

    // not a reply restart situation?  We need to do the full
    // initial setup
    if (executionState != REPLIED)
    {
        // exits possible?  We don't use exits for methods in the image
        // we try not to check stuff between clauses unless we have to...the
        // clause boundary flag tells us we need to perform checks.
        if (!code->isOldSpace() && activity->isClauseExitUsed())
        {
            // check at the end of each clause
            clauseBoundary = true;
            // remember that we have sys exits
            settings.stateFlags[clauseExits] = true;
        }
        // save the argument information
        argList = _arglist;
        argCount = _argcount;
        // is this the top-level program call?  We need to dummy up our
        // parent information.
        if (isTopLevelCall())
        {
            // save entry argument list forvariable pool fetch private access
            settings.parent_arglist = arglist;
            settings.parent_argcount = argcount;
            // make sure the code has resolved any class definitions, requireds, or libraries
            code->install(this);
            // set our starting code position (and the instruction used for error reporting)
            next = code->getFirstInstruction();
            current = next;
            // if this is a program invocation, then we need to potentially
            // run the initialization exit.
            if (isProgramLevelCall())
            {
                activity->callInitializationExit(this);
                activity->getInstance()->setupProgram(this);
            }
            // this is a method call.  We need to do some method setup.
            else
            {
                // guarded methods need to reserve the object scope
                if (isGuarded())
                {
                    // get the object variables and reserve these
                    settings.objectVariables = receiver->getObjectVariables(scope);
                    settings.objectVariables->reserve(activity);
                    objectScope = SCOPE_RESERVED;
                }
                // initialize the SELF and SUPER variables
                setLocalVariable(OREF_SELF, VARIABLE_SELF, receiver);
                setLocalVariable(OREF_SUPER, VARIABLE_SUPER, receiver->superScope(scope));
            }
        }
        // Internal call, interpret, or a debug pause
        else
        {
            // for an internal call, we're handed the starting instruction.  This
            // is probably an interpret, so we need to get it from the code object.
            if (start != OREF_NULL)
            {
                next = start;
            }
            else
            {
                next = code->getFirstInstruction();
            }
            // current and next are always the same at the start in case
            // we encounter any errors.
            current = next;
        }
    }
    // resuming on a new thread after a reply
    else
    {
        // if we could not keep the guard lock when we were spun off, then
        // we need to reaquire (and potentially wait) for the lock now.
        if (settings.stateFlags[transferFailed])
        {
            settings.objectVariabless->reserve(activity);
            // turn off the failure flag in case we spin off again.
            settings.stateFlags[transferFailed];
        }
    }

    // if this is an internal call, then we need to scan a little
    // bit ahead to see if we have a procedure at the start of the
    // code section.
    if (isInternalCall())
    {
        start = next;
        while (start != OREF_NULL && start->isType(KEYWORD_LABEL))
        {
            start = start->nextInstruction;
        }
        // if we only have labels and then a PROCEDURE, this is valid
        // and we allow it when issued.
        if (start != OREF_NULL && start->isType(KEYWORD_PROCEDURE))
        {
            settings.stateFlags[procedureValid] = true;
        }
    }

    // we are now live
    executionState = ACTIVE;

    // we might have a package option that turned on tracing.  If this
    // is a routine or method invocation in one of those packages, give the
    // initial entry trace so the user knows where we are.
    if (tracingAll() && isMethodOrRoutine())
    {
        traceEntry();
    }

    // this is the main execution loop...continue until we get a terminating
    // condition, such as a RETURN or EXIT, or just reaching the end of the code stream.
    while (true)
    {
        try
        {
#ifndef FIXEDTIMERS
            // reset the instruction counter
            instructionCount = 0;
#endif
            RexxInstruction *nextInst = next;
            // loop until we no longer have a next instruction to process
            while (nextInst != OREF_NULL)
            {

                // if we're time slicing, do a quick timeslice check and give
                // up the kernel lock if has pinged.
#ifdef FIXEDTIMERS
                if (Interpreter::hasTimeSliceElapsed())
                {
                    activity->relinquish();
                }
#else
                // not doing time slicing, so just relinquish every so often.
                if (++instructionCount > MAX_INSTRUCTIONS)
                {
                    // TODO:  make sure this is optimized for single-thread execution.
                    activity->relinquish();
                    instructionCount = 0;
                }
#endif
                // set the current instruction and prefetch the next one.  Control
                // instructions may change next on us.
                current = nextInst;
                next = nextInst->nextInstruction;
                // execute the current instruction
                nextInst->execute(this, &stack);

                // make sure the stack is cleared after each instruction
                stack.clear();
                // the time stamp is no longer current
                settings.timeStamp.valid = false;

                // do we need to check clause_boundary stuff?  Go do those
                // checks.
                if (clauseBoundary)
                {
                    processClauseBoundary();
                }
                // get our next instrucion and loop around
                nextInst = next;
            }

            // if we've fallen off the end of the code, our state is still
            // active.  Handle this as an implicit exit
            if (executionState == ACTIVE)
            {
                implicitExit();              /* treat this like an EXIT           */
            }

            // had a real RETURN?
            if (executionState == RETURNED)
            {
                // perform the normal termination
                termination();
                // if this was an interpret, then any state changes need to
                // be pushed back to the parent
                if (isInterpret())
                {
                    // save the nested setting
                    bool nested = parent->settings.localVariables.isNested();
                    // propagate parent's settings back
                    parent->getSettings(settings);
                    if (!nested)
                    {
                        // if our calling variable context was not nested, we
                        // need to clear it.
                        parent->settings.localVariables.clearNested();
                    }
                    // merge any pending conditions
                    parent->mergeTraps(conditionQueue);
                }
                resultObj = result;  // save the result
                // pop this off of the activity stack and
                activity->popStackFrame(false);
                // see if there are any objects waiting to run uninits.
                memoryObject.checkUninitQueue();
            }
            // This is a REPLIED state
            else
            {
                // save the reply result for handing back to the caller.
                resultObj = result;
                // reset the next instruction
                next = current->nextInstruction;
                // now we need to create a new activity and
                // attach this activation to it.
                Activity *oldActivity = activity;

                activity = oldActivity->spawnReply();

                // save the pointer to the start of our stack frame.  We're
                // going to need to release this after we migrate everything
                // over.
                RexxInternalObject **framePtr = stack.getFrame();
                // migrate the local variables and the expression stack to the
                // new activity.  NOTE:  these must be done in this order to
                // get them allocated from the new activity in the correct
                // order.
                stack.migrate(activity);
                settings.localVariables.migrate(activity);
                // if we have arguments, we need to migrate those also, as they are
                // subject to overwriting once we return to the parent activation.
                if (argcount > 0)
                {
                    RexxObject **newArguments = activity->allocateFrame(argcount);
                    memcpy(newArguments, arglist, sizeof(RexxObject *) * argcount);
                    arglist = newArguments;  // must be set on "this"
                    settings.parent_arglist = newArguments;
                }

                // return our stack frame space back to the old activity.
                oldActivity->releaseStackFrame(framePtr);

                // now push this activation on to the new activity
                activity->pushStackFrame(this);
                // pop the old one off of the stack frame (but without returning it to
                // the activation cache)
                oldActivity->popStackFrame(true);
                // if we have the scope lock, try to transfer it to the new activity.  If
                // the lock is nested on this activity, then we will need to
                // obtain it when we start running on the new thread.
                if (objectScope == SCOPE_RESERVED)
                {
                    if (!settings.objectVariabless->transfer(activity))
                    {
                        // this will tell us that we need to try grabbing this again.
                        settings.stateFlags[transferFailed] = true;
                    }
                }
                // now start the new activity running and give up control on this
                // thread before returning.
                activity->run();
                oldActivity->relinquish();
            }
            return resultObj;
        }
        // an error has occurred.  The thrown object is an activation pointer,
        // which tells us when to stop unwinding
        catch (RexxActivation *t)
        {
            // if we're not the target of this throw, we've already been unwound
            // keep throwing this until it reaches the target activation.
            if (t != this )
            {
                throw;
            }
            // unwind the activation stack back to our frame
            activity->unwindToFrame(this);

            // do the normal between clause clean up.
            stack.clear();
            settings.timestamp.valid = false;

            // if we were in a debug pause, we had a error interpreting the
            // line typed at the pause.  We're just going to terminate this
            // like it was a return
            if (debugPause)
            {
                executionState = RETURNED;
                next = OREF_NULL;
            }

            // we might have caught a condition.  See if we have something to do.
            if (conditionQueue != OREF_NULL)
            {
                // if we have pending traps, process them now
                if (!conditionQueue->isEmpty())
                {
                    processTraps();
                    // processing the traps might have deferred handling until clause
                    // termination (CALL ON conditions)...turn on the clause_boundary
                    // flag to check for them after instruction completiong.
                    if (!conditionQueue->isEmpty())
                    {
                        clauseBoundary = true;
                    }
                }
            }
        }
    }
}


/**
 * process pending condition traps before going on to execute a
 * clause
 */
void RexxActivation::processTraps()
{
    size_t i = conditionQueue->items();

    // process each item currently in the queue
    while (i--)
    {
        // get the next handler off the queue
        TrapHandler *trapHandler = (TrapHandler *)conditionQueue->pull();
        // condition in DELAY state?
        if (trapHandler->isDelayed())
        {
            // add to the end of the queue
            conditionQueue->append(trapHandler);
        }
        else
        {
            // get the condition object from the current trap handler
            DirectoryClass *conditionObj = trapHandler->getConditionObject();
            // see if we have something to assign to the RC variable
            RexxInternalObject *rc = conditionObj->get(OREF_RC);
            if (rc != OREF_NULL)
            {
                setLocalVariable(OREF_RC, VARIABLE_RC, rc);
            }

            // it's possible that the condition can raise an error because of a
            // missing label, so we need to catch any conditions that might be thrown
            try
            {
                // process the trap
                trapHandler->trap(this);
            }
            catch (RexxActivation *t)
            {
                // if we're not the target of this throw, we've already been unwound
                // keep throwing this until it reaches the target activation.
                if (t != this )
                {
                    throw;
                }
            }
        }
    }
}


/**
 * Process a numeric "debug skip" TRACE instruction to suppress
 * pauses or tracing for a given number of instructions.
 *
 * @param skipCount The number of clauses to skip.
 * @param noTrace   Indicates whether we are skipping tracing as well as the pauses.
 */
void RexxActivation::debugSkip(wholenumber_t skipCount, bool noTrace )
{
    // this is only allowed from a debug pause, not normal code execution
    if (!debugPause)
    {
        reportException(Error_Invalid_trace_debug);
    }

    // mark the count to skip
    settings.traceSkip = skipCount;
    // turn on the skip flag to suppress the tracing.
    settings.stateFlags[traceSuppress] = noTrace;
    settings.stateFlags[debugBpass] = true;
}


/**
 * Generate a string version of the current trace setting.
 *
 * @return The current trace setting formatted into a human-readable
 *         string.
 */
RexxString * RexxActivation::traceSetting()
{
    // have the source file process this
    return LanguageParser::formatTraceSetting(settings.traceOption);
}


/**
 * Set the trace using a dynamically evaluated string.
 *
 * @param setting The new trace setting.
 */
void RexxActivation::setTrace(RexxString *setting)
{
    size_t newsetting;                   /* new trace setting                 */
    size_t traceFlags;                   // the optimized trace flags

    char   traceOption = 0;              // a potential bad character

    if (!LanguageParser::parseTraceSetting(setting, newsetting, traceFlags, traceOption))
    {
        reportException(Error_Invalid_trace_trace, new_string(&traceOption, 1));
    }
                                       /* now change the setting            */
    setTrace(newsetting, traceFlags);
}


/**
 * Set a new trace setting for the context.
 *
 * @param source The trace setting source.
 */
void RexxActivation::setTrace(const TraceSetting &source)
{
    // turn off any trace suppression
    settings.stateFlags[traceSuppress] = false;
    settings.traceSkip = 0;

    // this might just be a debug toggle request.  All other trace
    // settings remain the same, but we flip the debug state to the
    // other mode.
    if (source.isDebugToggle())
    {
        // just flip the debug state
        settings.traceSettings.toggleDebug();
        // if no longer in debug mode, we need to reset the prompt issued flag
        if (!settings.traceSettings.isDebug())
        {
            // flipping out of debug mode.  Reissue the debug prompt when
            // turned back on again
            settings.stateFlags[debugPromptIssued] = false;
        }
    }
    // are we in debug mode already?  A trace setting with no "?" maintains the
    // debug setting, unless it is Trace Off
    else if (settings.traceSettings.isDebug())
    {
        // merge the flag settings
        settings.traceSettings.merge(source);
        // flipped out of debug mode.  Reissue the debug prompt when
        // turned back on again
        if (!settings.traceSettings.inDebug())
        {
            settings.stateFlags[debugPromptIssued] = false;
        }
    }
    else
    {
        // set the new flags
        settings.traceSettings.set(source);

    }

    // if tracing intermediates, turn on the special fast check flag
    settings.intermediateTrace = settings.traceFlags.tracingIntermediates();
    // if we issued this from a debug prompt, let the pause handler know this changes.
    if (debugPause)
    {
        settings.stateFlags[debugBypass] = true;
    }
}

void RexxActivation::live(size_t liveMark)
/******************************************************************************/
/* Function:  Normal garbage collection live marking                          */
/******************************************************************************/
{
    memory_mark(previous);
    memory_mark(executable);
    memory_mark(scope);
    memory_mark(code);
    memory_mark(settings.securityManager);
    memory_mark(receiver);
    memory_mark(activity);
    memory_mark(parent);
    memory_mark(dostack);
    // the stack and the local variables handle their own marking.
    stack.live(liveMark);
    settings.localVariables.live(liveMark);
    memory_mark(current);
    memory_mark(next);
    memory_mark(result);
    memory_mark(trapinfo);
    memory_mark(objnotify);
    memory_mark(environmentList);
    memory_mark(handler_queue);
    memory_mark(condition_queue);
    memory_mark(settings.traps);
    memory_mark(settings.conditionObj);
    memory_mark(settings.parentCode);
    memory_mark(settings.currentAddress);
    memory_mark(settings.alternateAddress);
    memory_mark(settings.msgname);
    memory_mark(settings.objectVariabless);
    memory_mark(settings.calltype);
    memory_mark(settings.streams);
    memory_mark(settings.halt_description);
    memory_mark(contextObject);

    // We're hold a pointer back to our arguments directly where they
    // are created.  Since in some places, this argument list comes
    // from the C stack, we need to handle the marker ourselves.
    memory_mark_array(argcount, arglist);
    memory_mark_array(settings.parent_argcount, settings.parent_arglist);
}

void RexxActivation::liveGeneral(MarkReason reason)
/******************************************************************************/
/* Function:  Generalized object marking                                      */
/******************************************************************************/
{
    memory_mark_general(previous);
    memory_mark_general(executable);
    memory_mark_general(code);
    memory_mark_general(settings.securityManager);
    memory_mark_general(receiver);
    memory_mark_general(activity);
    memory_mark_general(parent);
    memory_mark_general(dostack);
    // the stack and the local variables handle their own marking.
    stack.liveGeneral(reason);
    settings.localVariables.liveGeneral(reason);
    memory_mark_general(current);
    memory_mark_general(next);
    memory_mark_general(result);
    memory_mark_general(trapinfo);
    memory_mark_general(objnotify);
    memory_mark_general(environmentList);
    memory_mark_general(handler_queue);
    memory_mark_general(condition_queue);
    memory_mark_general(settings.traps);
    memory_mark_general(settings.conditionObj);
    memory_mark_general(settings.parentCode);
    memory_mark_general(settings.currentAddress);
    memory_mark_general(settings.alternateAddress);
    memory_mark_general(settings.msgname);
    memory_mark_general(settings.objectVariabless);
    memory_mark_general(settings.calltype);
    memory_mark_general(settings.streams);
    memory_mark_general(settings.halt_description);
    memory_mark_general(contextObject);

    // We're hold a pointer back to our arguments directly where they
    // are created.  Since in some places, this argument list comes
    // from the C stack, we need to handle the marking ourselves.
    memory_mark_general_array(argcount, arglist);
    memory_mark_general_array(settings.parent_argcount, settings.parent_arglist);
}


void RexxActivation::reply(
     RexxObject * resultObj)           /* returned REPLY result             */
/******************************************************************************/
/* Function:  Process a REXX REPLY instruction                                */
/******************************************************************************/
{
    /* already had a reply issued?       */
    if (settings.stateFlags[replyIssued])
    {
        reportException(Error_Execution_reply);
    }
    settings.stateFlags[replyIssued] = true;/* turn on the replied flag          */
                                         /* change execution state to         */
    executionState = REPLIED;     /* terminate the main loop           */
    next = OREF_NULL;              /* turn off execution engine         */
    result = resultObj;            /* save the result value             */
}


void RexxActivation::returnFrom(
     RexxObject * resultObj)           /* returned RETURN/EXIT result       */
/******************************************************************************/
/* Function:  process a REXX RETURN instruction                               */
/******************************************************************************/
{
    // already had a reply and trying to return a result?  There is nobody
    // to receive this result, so this is an error.
    if (settings.stateFlags[replyIssued] && resultObj != OREF_NULL)
    {
        reportException(Error_Execution_reply_return);
    }
    /* processing an Interpret           */
    if (isInterpret())
    {
        executionState = RETURNED;  /* this is a returned state          */
        next = OREF_NULL;            /* turn off execution engine         */
                                           /* cause a return in the parent      */
        parent->returnFrom(resultObj); /* activity                          */
    }
    else
    {
        executionState = RETURNED;  /* the state is returned             */
        next = OREF_NULL;            /* turn off execution engine         */
        result = resultObj;          /* save the return result            */
                                           /* real program call?                */
        if (isProgramLevelCall())
        {
            /* run termination exit              */
            activity->callTerminationExit(this);
        }
    }
    /* switch debug off to avoid debug   */
    /* pause after exit entered from an  */
    resetDebug();
}


void RexxActivation::iterate(
     RexxString * name )               /* name specified on iterate         */
/******************************************************************************/
/* Function:  Process a REXX ITERATE instruction                              */
/******************************************************************************/
{
    DoBlock *doblock = topBlock();          /* get the first stack item          */

    while (doblock != OREF_NULL)
    {       /* while still DO blocks to process  */
        RexxBlockInstruction *loop = doblock->getParent();       /* get the actual loop instruction   */
        if (name == OREF_NULL)             // leaving the inner-most loop?
        {
            // we only recognize LOOP constructs for this.
            if (loop->isLoop())
            {
                /* reset the indentation             */
                setIndent(doblock->getIndent());
                ((RexxInstructionBaseDo *)loop)->reExecute(this, &stack, doblock);
                return;                          /* we're finished                    */
            }

        }
        // a named LEAVE can be either a labeled block or a loop.
        else if (loop->isLabel(name))
        {
            if (!loop->isLoop())
            {
                reportException(Error_Invalid_leave_iterate_name, name);
            }
            /* reset the indentation             */
            setIndent(doblock->getIndent());
            ((RexxInstructionBaseDo *)loop)->reExecute(this, &stack, doblock);
            return;                          /* we're finished                    */
        }
        popBlock();                  /* cause termination cleanup         */
        removeBlock();               /* remove the execution nest         */
        doblock = topBlock();        /* get the new stack top             */
    }
    if (name != OREF_NULL)               /* have a name?                      */
    {
        /* report exception with the name    */
        reportException(Error_Invalid_leave_iteratevar, name);
    }
    else
    {
        /* have a misplaced ITERATE          */
        reportException(Error_Invalid_leave_iterate);
    }
}


void RexxActivation::leaveLoop(
     RexxString * name )               /* name specified on leave           */
/******************************************************************************/
/* Function:  Process a REXX LEAVE instruction                                */
/******************************************************************************/
{
    DoBlock *doblock = topBlock();          /* get the first stack item          */

    while (doblock != OREF_NULL)
    {       /* while still DO blocks to process  */
        RexxBlockInstruction *loop = doblock->getParent();       /* get the actual loop instruction   */
        if (name == OREF_NULL)             // leaving the inner-most loop?
        {
            // we only recognize LOOP constructs for this.
            if (loop->isLoop())
            {
                loop->terminate(this, doblock);  /* terminate the loop                */
                return;                          /* we're finished                    */
            }

        }
        // a named LEAVE can be either a labeled block or a loop.
        else if (loop->isLabel(name))
        {
            loop->terminate(this, doblock);  /* terminate the loop                */
            return;                          /* we're finished                    */
        }
        popBlock();                  /* cause termination cleanup         */
        removeBlock();               /* remove the execution nest         */
                                           /* get the first stack item again    */
        doblock = topBlock();        /* get the new stack top             */
    }
    if (name != OREF_NULL)               /* have a name?                      */
    {
        /* report exception with the name    */
        reportException(Error_Invalid_leave_leavevar, name);
    }
    else
    {
        /* have a misplaced LEAVE            */
        reportException(Error_Invalid_leave_leave);
    }
}

size_t RexxActivation::currentLine()
/******************************************************************************/
/* Function:  Return the line number of the current instruction               */
/******************************************************************************/
{
    if (current != OREF_NULL)      /* have a current line?              */
    {
        return current->getLineNumber(); /* return the line number            */
    }
    else
    {
        return 1;                          /* error on the loading              */
    }
}


void RexxActivation::procedureExpose(
    RexxVariableBase **variables, size_t count)
/******************************************************************************/
/* Function:  Expose variables for a PROCEDURE instruction                    */
/******************************************************************************/
{
    /* procedure not allowed here?       */
    if (!(settings.stateFlags[procedureValid]))
    {
        reportException(Error_Unexpected_procedure_call);
    }
    /* disable further procedures        */
    settings.stateFlags[procedureValid] = false;

    /* get a new  */
    activity->allocateLocalVariableFrame(&settings.localVariables);
    /* make sure we clear out the dictionary, otherwise we'll see the */
    /* dynamic entries from the previous level. */
    settings.localVariables.procedure(this);

    /* now expose each individual variable */
    for (size_t i = 0; i < count; i++)
    {
        variables[i]->procedureExpose(this, parent);
    }
}


void RexxActivation::expose(
    RexxVariableBase **variables, size_t count)
/******************************************************************************/
/* Function:  Expose variables for an EXPOSE instruction                      */
/******************************************************************************/
{
    /* get the variable set for this object */
    VariableDictionary * objectVariabless = getObjectVariables();

    /* now expose each individual variable */
    for (size_t i = 0; i < count; i++)
    {
        variables[i]->expose(this, objectVariabless);
    }
}


RexxObject *RexxActivation::forward(
    RexxObject  * target,              /* target object                     */
    RexxString  * message,             /* message to send                   */
    RexxObject  * superClass,          /* class over ride                   */
    RexxObject ** _arguments,          /* message arguments                 */
    size_t        _argcount,           /* count of message arguments        */
    bool          continuing)          /* return/continue flag              */
/******************************************************************************/
/* Function:  Process a REXX FORWARD instruction                              */
/******************************************************************************/
{
    if (target == OREF_NULL)             /* no target?                        */
    {
        target = receiver;           /* use this                          */
    }
    if (message == OREF_NULL)            /* no message override?              */
    {
        message = settings.msgname;  /* use same message name             */
    }
    if (_arguments == OREF_NULL)
    {       /* no arguments given?               */
        _arguments = arglist;        /* use the same arguments            */
        _argcount = argcount;
    }
    if (continuing)
    {                    /* just processing the message?      */
        ProtectedObject r;
        if (superClass == OREF_NULL)       /* no override?                      */
        {
            /* issue the message and return      */
            target->messageSend(message, _arguments, _argcount, r);
        }
        else
        {
            /* issue the message with override   */
            target->messageSend(message, _arguments, _argcount, superClass, r);
        }
        return(RexxObject *)r;
    }
    else
    {                               /* got to shut down and issue        */
        settings.stateFlags[forwarded] = true; /* we are now a phantom activation   */
        // cannot return a result if a reply has already been issued.
        if (settings.stateFlags[replyIssued]  && result != OREF_NULL)
        {
            reportException(Error_Execution_reply_exit);
        }
        executionState = RETURNED;  /* this is an EXIT for real          */
        next = OREF_NULL;            /* turn off execution engine         */
                                           /* switch debug off to avoid debug   */
                                           /* pause after exit entered from an  */
                                           /* interactive debug prompt          */
        resetDebug();
        ProtectedObject r;
        if (superClass == OREF_NULL)       /* no over ride?                     */
        {
            /* issue the simple message          */
            target->messageSend(message, _arguments, _argcount, r);
        }
        else
        {
            /* use the full override             */
            target->messageSend(message, _arguments, _argcount, superClass, r);
        }
        result = (RexxObject *)r;    /* save the result value             */
                                           /* already had a reply issued?       */
        if (settings.stateFlags[replyIssued] && result != OREF_NULL)
        {
            reportException(Error_Execution_reply_exit);
        }
        termination();               /* run "program" termination method  */
                                           /* if there are stream objects       */
        return OREF_NULL;                  /* just return nothing               */
    }
}

void RexxActivation::exitFrom(
     RexxObject * resultObj)           /* EXIT result                       */
/******************************************************************************/
/* Function:  Process a REXX exit instruction                                 */
/******************************************************************************/
{
    RexxActivation *activation;          /* unwound activation                */

    executionState = RETURNED;    /* this is an EXIT for real          */
    next = OREF_NULL;              /* turn off execution engine         */
    result = resultObj;            /* save the result value             */
    // switch off debug pausing
    resetDebug();
                                         /* at a main program level?          */
    if (isTopLevelCall())
    {
        /* already had a reply issued?       */
        if (settings.stateFlags[replyIssued] && result != OREF_NULL)
        {
            reportException(Error_Execution_reply_exit);
        }
        /* real program call?                */
        if (isProgramLevelCall())
        {
            /* run termination exit              */
            activity->callTerminationExit(this);
        }
    }
    else
    {                               /* internal routine or Interpret     */
        /* start terminating with this level */
        activation = this;
        do
        {
            activation->termination();       /* make sure this level cleans up    */
            ActivityManager::currentActivity->popStackFrame(false);     /* pop this level off                */
                                             /* get the next level                */
            activation = ActivityManager::currentActivity->getCurrentRexxFrame();
        } while (!activation->isTopLevel());

        activation->exitFrom(resultObj);   /* tell this level to terminate      */
                                           /* unwind and process the termination*/
        throw activation;                  // throw this as an exception to start the unwinding
    }
}


/**
 * Process a "fall off the end" exit condition
 */
void RexxActivation::implicitExit()
{
    // at a main program level or completing an INTERPRET instruction?
    if (isTopLevelCall() || isInterpret())
    {
        // real program call?  we might have a termination exit to call
        if (isProgramLevelCall())
        {
            activity->callTerminationExit(this);
        }
        // we are terminating
        executionState = RETURNED;
        return;
    }
    // we've had a nested exit, we need to process this more fully
    exitFrom(OREF_NULL);
}

void RexxActivation::termination()
/******************************************************************************/
/* Function: do any cleanup due to a program terminating.                     */
/******************************************************************************/
{
    guardOff();                    /* Remove any guards for this activatio*/

                                         /* were there any SETLOCAL calls for */
                                         /* this method?  And are there any   */
                                         /* that didn't have a matching ENDLOC*/
    if (environmentList != OREF_NULL && environmentList->getSize() != 0)
    {
        /* Yes, then restore the environment */
        /*  to the ist on added.             */
        SystemInterpreter::restoreEnvironment(((BufferClass *)environmentList->lastItem())->getData());
    }
    environmentList = OREF_NULL;   /* Clear out the env list            */
    closeStreams();                /* close any open streams            */
    /* release the stack frame, which also releases the frame for the */
    /* variable cache. */
    activity->releaseStackFrame(stack.getFrame());
    /* do the variable termination       */
    cleanupLocalVariables();
    // deactivate the context object if we created one.
    if (contextObject != OREF_NULL)
    {
        contextObject->detach();
    }
}


/**
 * Create/copy a trap table as needed
 */
void RexxActivation::checkTrapTable()
{
    // no trap table created yet?  just create a new collection
    if (settings.traps == OREF_NULL)
    {
        settings.traps = new_string_table();
    }
    // have to copy the trap table for an internal routine call?
    else if (isInternalCall() && !(settings.stateFlags[trapsCopied]))
    {
        // copy the table and remember that we've done that
        settings.traps = (StringTable *)settings.traps->copy();
        settings.stateFlags[trapsCopied] = true;
    }
}


/**
 * Activate a condition trap.
 *
 * @param condition The condition name being trapped.
 * @param handler   The instruction handling the trap.
 */
void RexxActivation::trapOn(RexxString *condition, RexxInstructionCallBase *handler)
{
    // make sure we have a trap table (we create on demand)
    checkTrapTable();

    // add a state block to our current trap list
    settings.traps->put(new TrapHandler(condition, handler), condition);
    // if this is NOVALUE or ANY, then we need to flip on the switch in the
    // local variables indicating we're interested in NOVALUE events.
    if (condition->strCompare(CHAR_NOVALUE) || condition->strCompare(CHAR_ANY))
    {
        settings.localVariables.setNovalueOn();
    }
}


/**
 * Disable a condition trap
 *
 * @param condition The name of the condition we're turning off.
 */
void RexxActivation::trapOff(RexxString *condition)
{
    checkTrapTable();
    // remove our existing trap.
    settings.traps->remove(condition);
    // if we no longer have NOVALUE or ANY enabled, then we can turn
    // off novalue processing in the variable pool
    if (!settings.traps->hasIndex(OREF_NOVALUE) && !settings.traps->hasIndex(OREF_ANY))
    {
        settings.localVariables.setNovalueOff();
    }
}


RexxActivation * RexxActivation::external()
/******************************************************************************/
/* Function:  Return the top level external activation                        */
/******************************************************************************/
{
    /* if an internal call or an         */
    /* interpret, we need to pass this   */
    /* along                             */
    if (isInternalLevelCall())
    {
        return parent->external();   /* get our sender method             */
    }
    else
    {
        return this;                       /* already at the top level          */
    }
}


void RexxActivation::raiseExit(
     RexxString    * condition,        /* condition to raise                */
     RexxObject    * rc,               /* information assigned to RC        */
     RexxString    * description,      /* description of the condition      */
     RexxObject    * additional,       /* extra descriptive information     */
     RexxObject    * resultObj,        /* return result                     */
     DirectoryClass * conditionobj )    /* propagated condition object       */
/******************************************************************************/
/* Function:  Raise a condition using exit semantics for the returned value.  */
/******************************************************************************/
{
    /* not internal routine or Interpret */
    /* instruction activation?           */
    if (isTopLevelCall())
    {
        /* do the real condition raise       */
        raise(condition, rc, description, additional, resultObj, conditionobj);
        return;                            /* return if processed               */
    }

    /* reached the top level?            */
    if (parent == OREF_NULL)
    {
        exitFrom(resultObj);         /* turn into an exit instruction     */
    }
    else
    {
        /* real program call?                */
        if (isProgramLevelCall())
        {
            /* run termination exit              */
            activity->callTerminationExit(this);
        }
        ProtectedObject p(this);
        termination();               /* remove guarded status on object   */
        activity->popStackFrame(false); /* pop ourselves off active list     */
        /* propogate the condition backward  */
        parent->raiseExit(condition, rc, description, additional, resultObj, conditionobj);
    }
}


void RexxActivation::raise(
     RexxString    * condition,        /* condition to raise                */
     RexxObject    * rc,               /* information assigned to RC        */
     RexxString    * description,      /* description of the condition      */
     RexxObject    * additional,       /* extra descriptive information     */
     RexxObject    * resultObj,        /* return result                     */
     DirectoryClass * conditionobj )    /* propagated condition object       */
/******************************************************************************/
/* Function:  Raise a give REXX condition                                     */
/******************************************************************************/
{
    bool            propagated;          /* propagated syntax condition       */

                                         /* propagating an existing condition?*/
    if (condition->strCompare(CHAR_PROPAGATE))
    {
        /* get the original condition name   */
        condition = (RexxString *)conditionobj->at(OREF_CONDITION);
        propagated = true;                 /* this is propagated                */
                                           /* fill in the propagation status    */
        conditionobj->put(TheTrueObject, OREF_PROPAGATED);
        if (resultObj == OREF_NULL)        /* no result specified?              */
        {
            resultObj = conditionobj->at(OREF_RESULT);
        }
    }
    else
    {                               /* build a condition object          */
        conditionobj = new_directory();    /* get a new directory               */
                                           /* put in the condition name         */
        conditionobj->put(condition, OREF_CONDITION);
        /* fill in default description       */
        conditionobj->put(OREF_NULLSTRING, OREF_DESCRIPTION);
        /* fill in the propagation status    */
        conditionobj->put(TheFalseObject, OREF_PROPAGATED);
        propagated = false;                /* remember for later                */
    }
    if (rc != OREF_NULL)                 /* have an RC value?                 */
    {
        conditionobj->put(rc, OREF_RC);    /* add to the condition argument     */
    }
    if (description != OREF_NULL)        /* any description to add?           */
    {
        conditionobj->put(description, OREF_DESCRIPTION);
    }
    if (additional != OREF_NULL)         /* or additional information         */
    {
        conditionobj->put(additional, OREF_ADDITIONAL);
    }
    if (resultObj != OREF_NULL)          /* or a result object                */
    {
        conditionobj->put(resultObj, OREF_RESULT);
    }

    /* fatal SYNTAX error?               */
    if (condition->strCompare(CHAR_SYNTAX))
    {
        ProtectedObject p(this);
        if (propagated)
        {                  /* reraising a condition?            */
            termination();             /* do the termination cleanup on ourselves */
            activity->popStackFrame(false);  /* pop ourselves off active list     */
                                             /* go propagate the condition        */
            ActivityManager::currentActivity->reraiseException(conditionobj);
        }
        else
        {
            /* go raise the error                */
            ActivityManager::currentActivity->raiseException(((RexxInteger *)rc)->getValue(), description, (ArrayClass *)additional, resultObj);
        }
    }
    else
    {                               /* normal condition trapping         */
                                    /* get the sender object (if any)    */
        // find a predecessor Rexx activation
        RexxActivation *_sender = senderActivation();
        /* do we have a sender that is       */
        /* trapping this condition?          */
        /* do we have a sender?              */
        bool trapped = false;
        if (_sender != OREF_NULL)
        {
            /* "tickle them" with this           */
            trapped = _sender->trap(condition, conditionobj);
        }

        /* is this an untrapped halt condition?  Need to transform into a SYNTAX error */
        if (!trapped && condition->strCompare(CHAR_HALT))
        {
                                               /* raise as a syntax error           */
            reportException(Error_Program_interrupted_condition, OREF_HALT);
        }

        returnFrom(resultObj);       /* process the return part           */
        throw this;                        /* unwind and process the termination*/
    }
}


VariableDictionary * RexxActivation::getObjectVariables()
/******************************************************************************/
/* Function:  Return the associated object variables vdict                    */
/******************************************************************************/
{
    /* no retrieved yet?                 */
    if (settings.objectVariabless == OREF_NULL)
    {
        /* get the object variables          */
        settings.objectVariabless = receiver->getObjectVariables(scope);
        if (isGuarded())                   /* guarded method?                   */
        {
            /* reserve the variable scope        */
            settings.objectVariabless->reserve(activity);
            /* and remember for later            */
            objectScope = SCOPE_RESERVED;
        }
    }
    /* return the vdict                  */
    return settings.objectVariabless;
}


/**
 * Resolve a stream name for a BIF call.
 *
 * @param name     The name of the stream.
 * @param stack    The expression stack.
 * @param input    The input/output flag.
 * @param fullName The returned full name of the stream.
 * @param added    A flag indicating we added this.
 *
 * @return The backing stream object for the name.
 */
RexxObject *RexxActivation::resolveStream(RexxString *name, bool input, RexxString **fullName, bool *added)
{
    if (added != NULL)
    {
        *added = false;           /* when caller requires stream table entry then initialize */
    }
    DirectoryClass *streamTable = getStreams(); /* get the current stream set        */
    if (fullName)                        /* fullName requested?               */
    {
        *fullName = name;                  /* initialize to name                */
    }
    /* if length of name is 0, then it's the same as omitted */
    if (name == OREF_NULL || name->getLength() == 0)   /* no name?                 */
    {
        if (input)                         /* input operation?                  */
        {
            /* get the default output stream     */
            return getLocalEnvironment(OREF_INPUT);
        }
        else
        {
            /* get the default output stream     */
            return getLocalEnvironment(OREF_OUTPUT);
        }
    }
    /* standard input stream?            */
    else if (name->strCaselessCompare(CHAR_STDIN) || name->strCaselessCompare(CHAR_CSTDIN))
    {
        /* get the default output stream     */
        return getLocalEnvironment(OREF_INPUT);
    }
    /* standard output stream?           */
    else if (name->strCaselessCompare(CHAR_STDOUT) || name->strCaselessCompare(CHAR_CSTDOUT))
    {
        /* get the default output stream     */
        return getLocalEnvironment(OREF_OUTPUT);
    }
    /* standard error stream?            */
    else if (name->strCaselessCompare(CHAR_STDERR) || name->strCaselessCompare(CHAR_CSTDERR))
    {
        /* get the default output stream     */
        return getLocalEnvironment(OREF_ERRORNAME);
    }
    else
    {
        /* go get the qualified name         */
        RexxString *qualifiedName = SystemInterpreter::qualifyFileSystemName(name);
        if (fullName)                      /* fullName requested?               */
        {
            *fullName = qualifiedName;       /* provide qualified name            */
        }
        // protect from GC
        ProtectedObject p(qualifiedName);
        /* Note: stream name is pushed to the stack to be protected from GC;    */
        /* e.g. it is used by the caller to remove stream from stream table.    */
        /* The stack will be reset after the function was executed and the      */
        /* protection is released                                               */
        /* see if we've already opened this  */
        RexxObject *stream = streamTable->at(qualifiedName);
        if (stream == OREF_NULL)           /* not open                          */
        {
            SecurityManager *manager = getEffectiveSecurityManager();
            stream = manager->checkStreamAccess(qualifiedName);
            if (stream != OREF_NULL)
            {
                streamTable->put(stream, qualifiedName);
                return stream;               /* return the stream object          */
            }
            /* get the stream class              */
            RexxObject *streamClass = TheEnvironment->at(OREF_STREAM);
            /* create a new stream object        */
            stream = streamClass->sendMessage(OREF_NEW, name);

            if (added)                       /* open the stream?   begin          */
            {
                /* add to the streams table          */
                streamTable->put(stream, qualifiedName);
                *added = true;                 /* mark it as added to stream table  */
            }
        }
        return stream;                       /* return the stream object          */
    }
}

DirectoryClass *RexxActivation::getStreams()
/******************************************************************************/
/* Function:  Return the associated object variables stream table             */
/******************************************************************************/
{
    /* not created yet?                  */
    if (settings.streams == OREF_NULL)
    {
        /* first entry into here?            */
        if (isProgramOrMethod())
        {
            /* always use a new directory        */
            settings.streams = new_directory();
        }
        else
        {
            // get the caller frame.  If it is not a Rexx one, then
            // we use a fresh stream table
            RexxActivationBase *callerFrame = getPreviousStackFrame();
            if (callerFrame == OREF_NULL || !callerFrame->isRexxContext())
            {
                settings.streams = new_directory();
            }
            else
            {

                /* alway's use caller's for internal */
                /* call, external call or interpret  */
                settings.streams = ((RexxActivation *)callerFrame)->getStreams();
            }
        }
    }
    return settings.streams;       /* return the stream table           */
}

void RexxActivation::signalTo(
     RexxInstruction * target )        /* target instruction                */
/******************************************************************************/
/* Function:  Signal to a targer instruction                                  */
/******************************************************************************/
{
    /* internal routine or Interpret     */
    /* instruction activation?           */
    if (isInterpret())
    {
        executionState = RETURNED;  /* signal interpret termination      */
        next = OREF_NULL;            /* turn off execution engine         */
        parent->signalTo(target);    /* propogate the signal backward     */
    }
    else
    {
        /* initialize the SIGL variable      */
        size_t lineNum = current->getLineNumber();/* get the instruction line number   */
        setLocalVariable(OREF_SIGL, VARIABLE_SIGL, new_integer(lineNum));
        next = target;               /* set the new target location       */
        dostack = OREF_NULL;         /* clear the do loop stack           */
        blockNest = 0;               /* no more active blocks             */
        settings.traceindent = 0;    /* reset trace indentation           */
    }
}

void RexxActivation::toggleAddress()
/******************************************************************************/
/* Function:  Toggle the address setting between the current and alternate    */
/******************************************************************************/
{
    RexxString *temp = settings.currentAddress;   /* save the current environment      */
    /* make the alternate the current    */
    settings.currentAddress = settings.alternateAddress;
    settings.alternateAddress = temp; /* old current is now the alternate  */
}


void RexxActivation::setAddress(
                               RexxString * address )             /* new address environment           */
/******************************************************************************/
/* Function:  Set the new current address, moving the current one to the      */
/*            alternate address                                               */
/******************************************************************************/
{
    /* old current is now the alternate  */
    settings.alternateAddress = settings.currentAddress;
    settings.currentAddress = address;/* set new current environment       */
}


void RexxActivation::setDefaultAddress(
                                      RexxString * address )             /* new address environment           */
/******************************************************************************/
/* Function:  Set up a default address environment so that both the primary   */
/*            and the alternate address are the same value                    */
/******************************************************************************/
{
    /* old current is the new one        */
    settings.alternateAddress = address;
    settings.currentAddress = address;/* set new current environment       */
}


void RexxActivation::signalValue(
    RexxString * name )                /* target label name                 */
/******************************************************************************/
/* Function:  Signal to a computed label target                               */
/******************************************************************************/
{
    RexxInstruction *target = OREF_NULL;                  /* no target yet                     */
    DirectoryClass *labels = getLabels();          /* get the labels                    */
    if (labels != OREF_NULL)             /* have labels?                      */
    {
        /* look up label and go to normal    */
        /* signal processing                 */
        target = (RexxInstruction *)labels->at(name);
    }
    if (target == OREF_NULL)             /* unknown label?                    */
    {
        /* raise an error                    */
        reportException(Error_Label_not_found_name, name);
    }
    signalTo(target);              /* now switch to the label location  */
}


void RexxActivation::guardOn()
/******************************************************************************/
/* Function:  Turn on the activations guarded state                           */
/******************************************************************************/
{
    /* currently in unguarded state?     */
    if (objectScope == SCOPE_RELEASED)
    {
        /* not retrieved yet?                */
        if (settings.objectVariabless == OREF_NULL)
        {
            /* get the object variables          */
            settings.objectVariabless = receiver->getObjectVariables(scope);
        }
        /* lock the variable dictionary      */
        settings.objectVariabless->reserve(activity);
        /* set the state here also           */
        objectScope = SCOPE_RESERVED;
    }
}

size_t RexxActivation::digits()
/******************************************************************************/
/* Function:  Return the current digits setting                               */
/******************************************************************************/
{
    return settings.numericSettings.digits;
}

size_t RexxActivation::fuzz()
/******************************************************************************/
/* Function:  Return the current fuzz setting                                 */
/******************************************************************************/
{
    return settings.numericSettings.fuzz;
}

bool RexxActivation::form()
/******************************************************************************/
/* Function:  Return the current FORM setting                                 */
/******************************************************************************/
{
    return settings.numericSettings.form;
}

/**
 * Set the digits setting to the package-defined default
 */
void RexxActivation::setDigits()
{
    setDigits(packageObject->getDigits());
}

void RexxActivation::setDigits(size_t digitsVal)
/******************************************************************************/
/* Function:  Set a new digits setting                                        */
/******************************************************************************/
{
    settings.numericSettings.digits = digitsVal;
}

/**
 * Set the fuzz setting to the package-defined default
 */
void RexxActivation::setFuzz()
{
    setFuzz(packageObject->getFuzz());
}



void RexxActivation::setFuzz(size_t fuzzVal)
/******************************************************************************/
/* Function:  Set a new FUZZ setting                                          */
/******************************************************************************/
{
    settings.numericSettings.fuzz = fuzzVal;
}

/**
 * Set the form setting to the package-defined default
 */
void RexxActivation::setForm()
{
    setForm(packageObject->getForm());
}



void RexxActivation::setForm(bool formVal)
/******************************************************************************/
/* Function:  Set the new current NUMERIC FORM setting                        */
/******************************************************************************/
{
    settings.numericSettings.form = formVal;
}


/**
 * Return the Rexx context this operates under.  Depending on the
 * context, this could be null.
 *
 * @return The parent Rexx context.
 */
RexxActivation *RexxActivation::getRexxContext()
{
    return this;          // I am my own grampa...I mean Rexx context.
}


/**
 * Return the Rexx context this operates under.  Depending on the
 * context, this could be null.
 *
 * @return The parent Rexx context.
 */
RexxActivation *RexxActivation::findRexxContext()
{
    return this;          // I am my own grampa...I mean Rexx context.
}


/**
 * Indicate whether this activation is a Rexx context or not.
 *
 * @return true if this is a Rexx context, false otherwise.
 */
bool RexxActivation::isRexxContext()
{
    return true;
}


/**
 * Get the numeric settings for the current context.
 *
 * @return The new numeric settings.
 */
NumericSettings *RexxActivation::getNumericSettings()
{
    return &(settings.numericSettings);
}


/**
 * Get the message receiver
 *
 * @return The message receiver.  Returns OREF_NULL if this is not
 *         a message activation.
 */
RexxObject *RexxActivation::getReceiver()
{
    if (isInterpret())
    {
        return parent->getReceiver();
    }
    return receiver;
}


/**
 * Return the current state for a trap as either ON, OFF, or DELAY
 *
 * @param condition The condition name.
 *
 * @return The trap state, either ON, OFF, or DELAY.
 */
RexxString *RexxActivation::trapState(RexxString *condition)
{
    // default to OFF
    RexxString *state = OREF_OFF;
    // no enabled traps?  must be off
    if (settings.traps != OREF_NULL)
    {
        // see if the trap is enabled, if we have this, query the state
        TrapHandler *trapHandler = settings.traps->get(condition);
        if (trapHandler != OREF_NULL)
        {
            return trapHandler->getState();
        }
    }
    return state;
}


/**
 * Put a trap into the delay state while executing a CALL
 * ON.
 *
 * @param condition The condition name.
 */
void RexxActivation::trapDelay(RexxString * condition)
{
    checkTrapTable();
    // if we have a trap, put it into the delay state
    TrapHandler *traphandler = (TrapHandler *)settings.traps->get(condition);
    if (traphandler != OREF_NULL)
    {
        trapHandler->disable();
    }
}


/**
 * Remove a trap from the DELAY state
 *
 * @param condition The condition being processed.
 */
void RexxActivation::trapUndelay(RexxString * condition)
{
    checkTrapTable();
    // if we have a trap, put it into the delay state
    TrapHandler *traphandler = (TrapHandler *)settings.traps->get(condition);
    if (traphandler != OREF_NULL)
    {
        trapHandler->enable();
    }
}


/**
 * Check the activation to see if this is trapping a condition.
 * For SIGNAL traps, control goes back to the point of the trap
 * via throw.  For CALL ON traps, the condition is saved, and
 * the method returns true to indicate the trap was handled.
 *
 * @param condition The name of the raised condition.
 * @param exceptionObject
 *                  The exception object associated with the condition.
 *
 * @return true if the condition was trapped and handled, false otherwise.
 */
bool RexxActivation::trap(RexxString *condition, DirectoryClass *exceptionObject)
{
    // TODO:  See if it is possible to check if a condition will be trapped before building
    // the condition object.

    // if we're in the act of processing a FORWARD instruction, then this
    // stack frame doesn't really exist any more.  We need to check the previous
    // stack frame to see if it can handle this.
    if (settings.stateFlags[forwarded])
    {
        RexxActivationBase *activation = getPreviousStackFrame();
        // we can have multiple forwardings in process, so keep drilling until we
        // find a non-forwarded frame
        while (activation != OREF_NULL && isOfClass(Activation, activation))
        {
            // we've found a non-ghost frame, so have it try to handle this.
            if (!activation->isForwarded())
            {
                return activation->trap(condition, exception_object);
            }
            activation = activation->getPreviousStackFrame();
        }
        // we are not really here, so we can't handle this
        return false;
    }
    // do we need to notify a message object of a syntax error?
    // send it the notification message.
    if (objnotify != OREF_NULL && condition->strCompare(CHAR_SYNTAX))
    {
        objnotify->error(exception_object);
    }

    // are we in a debug pause?  ignore any condition other than a syntax error.
    if (debugPause)
    {
        if (!condition->strCompare(CHAR_SYNTAX))
        {
            return false;
        }
        // display the errors encountered during debug, then do the
        // error unwind to terminate the debug pause activation.
        activity->displayDebug(exception_object);
        throw this;
    }
    // no trap table set yet?  can't handle this
    if (settings.traps == OREF_NULL)
    {
        return false;
    }

    // see if we have a handler for this condition
    TrapHandler *trapHandler = (TrapHandler *)settings.traps->get(condition);

    // nothing there for the specific condition.  We could have an ANY
    // trap enabled, so check that.
    if (trapHandler == OREF_NULL)
    {

        trapHandler = (TrapHandler *)settings.traps->get(OREF_ANY);
        // if we have a handler, but this can't handle this can condition, return false
        if (trapHandler != OREF_NULL && !trapHandler->canHandle(condition))
        {
            return false;
        }
    }
    // if the condition is being trapped, do the CALL or SIGNAL
    if (traphandler != OREF_NULL)
    {
        // if this is a halt condition, we might need to call the system exit.
        if (condition->strCompare(CHAR_HALT))
        {
            activity->callHaltClearExit(this);
        }

        /* get the handler info              */
        RexxInstructionCallBase *handler = (RexxInstructionCallBase *)traphandler->get(1);
        // no condition queue yet?           */
        if (conditionQueue == OREF_NULL)
        {
            // create a pending queue
            conditionQueue = new_queue();
        }

        RexxString *instruction = OREF_CALL;
        if (handler->isType(KEYWORD_SIGNAL_ON))
        {
            instruction = OREF_SIGNAL;       /* this is trapped by a SIGNAL       */
        }
                                             /* add the instruction trap info     */
        exceptionObject->put(trapHandler->instructionName(), OREF_INSTRUCTION);
        // set the condition object into the traphandler
        trapHandler->setConditionObject(exceptionObject);
        // add the handler to the condition queue
        conditionQueue->append(trapHandler);
        pendingConditions++;
        // clear this from the activity if we're trapping this here
        activity->clearCurrentCondition();

        // if the handler is a SIGNAL, then we unwind everything now without returning
        if (trapHandler->isSignal())
        {
            // if not an interpret, then we can just throw this and unwind the
            // stack.
            if (!isInterpret())
            {
                throw this;
            }
            // if we're interpreted, this needs to be handled in the parent
            // activaiton.
            else
            {
                parent->mergeTraps(conditionQueue);
                parent->unwindTrap(this);
            }
        }
        else
        {
            // we're going to need to process this trap at the clause boundary.
            clauseBoundary = true;
            // we've handled this
            return true;
        }
    }
    // not something we can handle.
    return false;
}


/**
 * Process a NOVALUE event for a variable.
 *
 * @param name     The variable name triggering the event.
 * @param variable The resolved variable object for the variable.
 *
 * @return A value for that variable.
 */
RexxInternalObject *RexxActivation::handleNovalueEvent(RexxString *name, RexxInternalObject *defaultValue, RexxVariable *variable)
{
    RexxInternalObject *value = novalueHandler(name);
    // If the handler returns anything other than .nil, this is a
    // value
    if (value != TheNilObject)
    {
        return value;
    }
    // give any external novalue handler a chance at this
    if (!activity->callNovalueExit(this, name, value))
    {
        // set this variable to the object found in the engine
        variable->set(value);
        return value;
    }
    // raise novalue?
    if (novalueEnabled())
    {
        reportNovalue(name);
    }

    // the provided default value is the returned value
    return defaultValue;
}


/**
 * Merge a list of trapped conditions from an interpret into the
 * parent activation's queues.
 *
 * @param sourceConditionQueue
 *               The interpret activations condition queue.
 */
void RexxActivation::mergeTraps(QueueClass *sourceConditionQueue)
{
    if (sourceConditionQueue != OREF_NULL)
    {
        // if we don't have a condition queue at this level yet, then
        // just inherit the interpret one
        if (conditionQueue == OREF_NULL)
        {
            conditionQueue = sourceConditionQueue;
        }
        else
        {
            // copy all of the items over.
            size_t items = sourcConditionQueue->item();
            while (items--)
            {
                conditionQueue->append(sourceConditionQueue->pull());
            }
        }
    }
}


/**
 * Unwind a chain of interpret activations to process a SIGNAL ON
 * or PROPAGATE condition trap.  This ensures that the SIGNAL
 * or PROPAGATE returns to the correct condition level
 *
 * @param child  The child activaty.
 */
void RexxActivation::unwindTrap(RexxActivation * child )
{
    //* still an interpret level?  Merge, and try again
    if (isInterpret())
    {
        parent->mergeTraps(conditionQueue);
        parent->unwindTrap(child);
    }
    // reached the base non-interpret level
    else
    {
        // pull back the settings from the child
        child->putSettings(settings);
        // unwind and process the trap
        throw this;
    }
}


RexxActivation * RexxActivation::senderActivation()
/******************************************************************************/
/* Function:  Retrieve the activation that activated this activation (whew)   */
/******************************************************************************/
{
    /* get the sender from the activity  */
    RexxActivationBase *_sender = getPreviousStackFrame();
    /* spin down to non-native activation*/
    while (_sender != OREF_NULL && isOfClass(NativeActivation, _sender))
    {
        _sender = _sender->getPreviousStackFrame();
    }
    return(RexxActivation *)_sender;    /* return that activation            */
}

/**
 * Translate and interpret a string of data as a piece
 * of Rexx code within the current program context.
 *
 * @param codestring The source code string.
 */
void RexxActivation::interpret(RexxString * codestring)
/******************************************************************************/
/* Function:  Translate and interpret a string of data as a piece of REXX     */
/*            code within the current program context.                        */
/******************************************************************************/
{
    // check the stack space to see if we have room.
    ActivityManager::currentActivity->checkStackSpace();
    // translate the code as if it was located here.
    RexxCode * newCode = code->interpret(codestring, current->getLineNumber());
    // create a new activation to run this code
    RexxActivation *newActivation = ActivityManager::newActivation(activity, this, newCode, INTERPRET);
    activity->pushStackFrame(newActivation);
    ProtectedObject r;
    // run this compiled code on the new activation
    newActivation->run(OREF_NULL, OREF_NULL, arglist, argcount, OREF_NULL, r);
}


/**
 * Interpret a string of debug input.
 *
 * @param codestring The code string to interpret
 */
void RexxActivation::debugInterpret(RexxString * codestring)
{
    // mark that this is debug mode
    debugPause = true;
    try
    {
        // translate the code
        RexxCode *newCode = code->interpret(codestring, current->getLineNumber());
        // get a new activation to execute this
        RexxActivation *newActivation = ActivityManager::newActivation(activity, this, newCode, DEBUGPAUSE);
        activity->pushStackFrame(newActivation);
        ProtectedObject r;
        // go run the code
        newActivation->run(receiver, settings.msgname, arglist, argcount, OREF_NULL, r);
        // turn this off when done executing
        debugPause = false;
    }
    catch (RexxActivation *t)
    {
        // turn this off unconditionally for any errors
        // if we're not the target of this throw, we've already been unwound
        // keep throwing this until it reaches the target activation.
        if (t != this )
        {
            throw;
        }
    }
}

/**
 * Return a Rexx-defined "dot" variable na.e
 *
 * @param name   The target variable name.
 *
 * @return The variable value or OREF_NULL if this is not
 *         one of the special variables.
 */
RexxObject * RexxActivation::rexxVariable(RexxString * name )
{
    // .RS happens in our context, so process here.
    if (name->strCompare(CHAR_RS))
    {
        if (settings.stateFlags[returnStatusSet])
        {
            /* returned as an integer object     */
            return new_integer(settings.returnStatus);
        }
        else                               /* just return the name              */
        {
            return name->concatToCstring(".");
        }
    }
    // all other should be handled by the parent context
    if (isInterpret())
    {
        return parent->rexxVariable(name);
    }

    if (name->strCompare(CHAR_METHODS))  /* is this ".methods"                */
    {
        /* get the methods directory         */
        return(RexxObject *)settings.parentCode->getMethods();
    }
    else if (name->strCompare(CHAR_ROUTINES))  /* is this ".routines"                */
    {
        /* get the methods directory         */
        return(RexxObject *)settings.parentCode->getRoutines();
    }
    else if (name->strCompare(CHAR_LINE))  /* current line (".line")?    */
    {
        return new_integer(current->getLineNumber());
    }
    else if (name->strCompare(CHAR_CONTEXT))
    {
        // retrieve the context object (potentially creating it on the first request)
        return getContextObject();
    }
    return OREF_NULL;                    // not recognized
}


/**
 * Get the context object for this activation.
 *
 * @return The created context object.
 */
RexxObject *RexxActivation::getContextObject()
{
    // the context object is created on demand...much of the time, this
    // is not needed for an actvation
    if (contextObject == OREF_NULL)
    {
        contextObject = new RexxContext(this);
    }
    return contextObject;
}


/**
 * Return the line context information for a context.
 *
 * @return The current execution line.
 */
RexxObject *RexxActivation::getContextLine()
{
    // if this is an interpret, we need to report the line number of
    // the context that calls the interpret.
    if (isInterpret())
    {
        return parent->getContextLine();
    }
    else
    {

        return new_integer(current->getLineNumber());
    }
}


/**
 * Return the line context information for a context.
 *
 * @return The current execution line.
 */
size_t RexxActivation::getContextLineNumber()
{
    // if this is an interpret, we need to report the line number of
    // the context that calls the interpret.
    if (isInterpret())
    {
        return parent->getContextLineNumber();
    }
    else
    {

        return current->getLineNumber();
    }
}


/**
 * Return the RS context information for a activation.
 *
 * @return The current execution line.
 */
RexxObject *RexxActivation::getContextReturnStatus()
{
    if (settings.stateFlags&returnStatusSet)
    {
        /* returned as an integer object     */
        return new_integer(settings.returnStatus);
    }
    else
    {
        return TheNilObject;
    }
}


/**
 * Attempt to call a function stored in the macrospace.
 *
 * @param target    The target function name.
 * @param arguments The argument pointer.
 * @param argcount  The count of arguments,
 * @param calltype  The type of call (FUNCTION or SUBROUTINE)
 * @param order     The macrospace order flag.
 * @param result    The function result.
 *
 * @return true if the macrospace function was located and called.
 */
bool RexxActivation::callMacroSpaceFunction(RexxString * target, RexxObject **_arguments,
    size_t _argcount, RexxString * calltype, int order, ProtectedObject &_result)
{
    unsigned short position;             /* located macro search position     */
    const char *macroName = target->getStringData();  /* point to the string data          */
    /* did we find this one?             */
    if (RexxQueryMacro(macroName, &position) == 0)
    {
        /* but not at the right time?        */
        if (order == MS_PREORDER && position == RXMACRO_SEARCH_AFTER)
        {
            return false;                    /* didn't really find this           */
        }
        // unflatten the code now
        Protected<RoutineClass> routine = getMacroCode(target);

        // not restoreable is a call failure
        if (routine == OREF_NULL)
        {
            return false;
        }
        // run as a call
        routine->call(activity, target, _arguments, _argcount, calltype, OREF_NULL, EXTERNALCALL, _result);
        // merge (class) definitions from macro with current settings
        getSourceObject()->mergeRequired(routine->getSourceObject());
        return true;                       /* return success we found it flag   */
    }
    return false;                        /* nope, nothing to find here        */
}


/**
 * Main method for performing an external routine call.  This
 * orchestrates the search order for locating an external routine.
 *
 * @param target    The target function name.
 * @param _argcount The count of arguments for the call.
 * @param _stack    The expression stack holding the arguments.
 * @param calltype  The type of call (FUNCTION or SUBROUTINE)
 * @param resultObj The returned result.
 *
 * @return The function result (also returned in the resultObj protected
 *         object reference.
 */
RexxObject *RexxActivation::externalCall(RexxString *target, size_t _argcount, ExpressionStack *_stack,
    RexxString *calltype, ProtectedObject &resultObj)
{
    /* get the arguments array           */
    RexxObject **_arguments = _stack->arguments(_argcount);
    // Step 1:  Check the global functions directory
    // this is actually considered part of the built-in functions, but these are
    // written in ooRexx.  The names are also case sensitive
    RoutineClass *routine = (RoutineClass *)TheFunctionsDirectory->get(target);
    if (routine != OREF_NULL)        /* not found yet?                    */
    {
        // call and return the result
        routine->call(this->activity, target, _arguments, _argcount, calltype, OREF_NULL, EXTERNALCALL, resultObj);
        return(RexxObject *)resultObj;
    }

    // Step 2:  Check for a ::ROUTINE definition in the local context
    routine = settings.parentCode->findRoutine(target);
    if (routine != OREF_NULL)
    {
        // call and return the result
        routine->call(activity, target, _arguments, _argcount, calltype, OREF_NULL, EXTERNALCALL, resultObj);
        return(RexxObject *)resultObj;
    }
    // Step 2a:  See if the function call exit fields this one
    if (!activity->callObjectFunctionExit(this, target, calltype, resultObj, _arguments, _argcount))
    {
        return(RexxObject *)resultObj;
    }

    // Step 2b:  See if the function call exit fields this one
    if (!activity->callFunctionExit(this, target, calltype, resultObj, _arguments, _argcount))
    {
        return(RexxObject *)resultObj;
    }

    // Step 3:  Perform all platform-specific searches
    if (SystemInterpreter::invokeExternalFunction(this, activity, target, _arguments, _argcount, calltype, resultObj))
    {
        return(RexxObject *)resultObj;
    }

    // Step 4:  Check scripting exit, which is after most of the checks
    if (!activity->callScriptingExit(this, target, calltype, resultObj, _arguments, _argcount))
    {
        return(RexxObject *)resultObj;
    }

    // if it's made it through all of these steps without finding anything, we
    // finally have a routine non found situation
    reportException(Error_Routine_not_found_name, target);
    return OREF_NULL;     // prevent compile error
}




/**
 * Call an external program as a function or subroutine.
 *
 * @param target     The target function name.
 * @param parent     The name of the parent program (used for resolving extensions).
 * @param _arguments The arguments to the call.
 * @param _argcount  The count of arguments for the call.
 * @param calltype   The type of call (FUNCTION or SUBROUTINE)
 * @param resultObj  The returned result.
 *
 * @return True if an external program was located and called.  false for
 *         any failures.
 */
bool RexxActivation::callExternalRexx(RexxString *target, RexxObject **_arguments,
    size_t _argcount, RexxString *calltype, ProtectedObject  &resultObj)
{
    // Get full name including path
    RexxString *filename = resolveProgramName(target);
    if (filename != OREF_NULL)
    {
        // protect the file name on stack
        stack.push(filename);
        // try for a saved program or translate a anew

        RoutineClass *routine = LanguageParser::createProgramFromFile(filename);
        // remove the protected name
        stack.pop();
        // do we have something?  return not found
        if (routine == OREF_NULL)
        {
            return false;
        }
        else
        {
            ProtectedObject p(routine);
            // run as a call
            routine->call(activity, target, _arguments, _argcount, calltype, settings.currentAddress, EXTERNALCALL, resultObj);
            // merge all of the public info
            settings.parentCode->mergeRequired(routine->getSourceObject());
            return true;
        }
    }
    // the external routine wasn't found
    else
    {
        return false;
    }
}


/**
 * Retrieve a macro image file from the macro space.
 *
 * @param macroName The name of the macro to retrieve.
 *
 * @return If available, the unflattened method image.
 */
RoutineClass *RexxActivation::getMacroCode(RexxString *macroName)
{
    RXSTRING       macroImage;
    RoutineClass * macroRoutine = OREF_NULL;

    macroImage.strptr = NULL;
    const char *name = macroName->getStringData();
    int rc;
    {
        UnsafeBlock releaser;

        rc = RexxResolveMacroFunction(name, &macroImage);
    }

    if (rc == 0)
    {
        macroRoutine = RoutineClass::restore(&macroImage, macroName);
        // return the allocated buffer
        if (macroImage.strptr != NULL)
        {
            SystemInterpreter::releaseResultMemory(macroImage.strptr);
        }
    }
    return macroRoutine;
}


/**
 * This is resolved in the context of the calling program.
 *
 * @param name   The name to resolve.
 *
 * @return The fully resolved program name, or OREF_NULL if this can't be
 *         located.
 */
RexxString *RexxActivation::resolveProgramName(RexxString *name)
{
    return code->resolveProgramName(activity, name);
}


/**
 * Resolve a class in this activation's context.
 *
 * @param name   The name to resolve.
 *
 * @return The resolved class, or OREF_NULL if not found.
 */
RexxClass *RexxActivation::findClass(RexxString *name)
{
    RexxClass *classObject = getSourceObject()->findClass(name);
    // we need to filter this to always return a class object
    if (classObject != OREF_NULL && classObject->isInstanceOf(TheClassClass))
    {
        return classObject;
    }
    return OREF_NULL;
}


/**
 * Resolve a class in this activation's context.
 *
 * @param name   The name to resolve.
 *
 * @return The resolved class, or OREF_NULL if not found.
 */
RexxObject *RexxActivation::resolveDotVariable(RexxString *name)
{
    // if not an interpret, then resolve directly.
    if (activationContext != INTERPRET)
    {
        return getSourceObject()->findClass(name);
    }
    else
    {
        // otherwise, send this up the call chain and resolve in the
        // original source context
        return parent->resolveDotVariable(name);
    }
}


/**
 * Load a ::REQUIRES directive when the source file is first
 * invoked.
 *
 * @param target The name of the ::REQUIRES
 * @param instruction
 *               The directive instruction being processed.
 */
PackageClass *RexxActivation::loadRequires(RexxString *target, RexxInstruction *instruction)
{
    // this will cause the correct location to be used for error reporting
    current = instruction;

    // the loading/merging is done by the source object
    return getSourceObject()->loadRequires(activity, target);
}


/**
 * Load a package defined by a ::REQUIRES name LIBRARY
 * directive.
 *
 * @param target The name of the package.
 * @param instruction
 *               The ::REQUIRES directive being loaded.
 */
void RexxActivation::loadLibrary(RexxString *target, RexxInstruction *instruction)
{
    // this will cause the correct location to be used for error reporting
    current = instruction;
    // have the package manager resolve the package
    PackageManager::getLibrary(target);
}


/**
 * Process an internal function or subroutine call.
 *
 * @param name      The name of the target label.
 * @param target    The target instruction where we start executing (this is the label)
 * @param _argcount The count of arguments
 * @param _stack    The context stack holding the arguments
 * @param returnObject
 *                  A holder for the return value
 *
 * @return The return value object
 */
RexxObject * RexxActivation::internalCall(RexxString *name, RexxInstruction *target,
    size_t _argcount, ExpressionStack *_stack, ProtectedObject &returnObject)
{
    RexxActivation * newActivation;      /* new activation for call           */
    size_t           lineNum;            /* line number of the call           */
    RexxObject **    _arguments = _stack->arguments(_argcount);

    lineNum = current->getLineNumber();  /* get the current line number       */
    /* initialize the SIGL variable      */
    setLocalVariable(OREF_SIGL, VARIABLE_SIGL, new_integer(lineNum));
    /* create a new activation           */
    newActivation = ActivityManager::newActivation(activity, this, settings.parentCode, INTERNALCALL);

    activity->pushStackFrame(newActivation); /* push on the activity stack        */
    /* run the internal routine on the   */
    /* new activation                    */
    return newActivation->run(receiver, name, _arguments, _argcount, target, returnObject);
}

/**
 * Processing a call to an internal trap subroutine.
 *
 * @param name      The label name of the internal call.
 * @param target    The target instruction for the call (the label)
 * @param conditionObj
 *                  The associated condition object
 * @param resultObj A holder for a result object
 *
 * @return Any return result
 */
RexxObject * RexxActivation::internalCallTrap(RexxString *name, RexxInstruction * target,
    DirectoryClass *conditionObj, ProtectedObject &resultObj)
{

    stack.push(conditionObj);      /* protect the condition object      */
    size_t lineNum = current->getLineNumber();  /* get the current line number       */
    /* initialize the SIGL variable      */
    setLocalVariable(OREF_SIGL, VARIABLE_SIGL, new_integer(lineNum));
    /* create a new activation           */
    RexxActivation *newActivation = ActivityManager::newActivation(activity, this, settings.parentCode, INTERNALCALL);
    /* set the new condition object      */
    newActivation->setConditionObj(conditionObj);
    activity->pushStackFrame(newActivation); /* push on the activity stack        */
    /* run the internal routine on the   */
    /* new activation                    */
    return newActivation->run(OREF_NULL, name, NULL, 0, target, resultObj);
}



void RexxActivation::guardWait()
/******************************************************************************/
/* Function:  Wait for a variable in a guard expression to get updated.       */
/******************************************************************************/
{
    int initial_state = objectScope;  /* save the initial state            */
                                         /* have the scope reserved?          */
    if (objectScope == SCOPE_RESERVED)
    {
        /* tell the receiver to release this */
        settings.objectVariabless->release(activity);
        /* and change our local state        */
        objectScope = SCOPE_RELEASED;    /* do an assignment! */
    }
    activity->guardWait();         /* wait on a variable inform event   */
                                         /* did we release the scope?         */
    if (initial_state == SCOPE_RESERVED)
    {
        /* tell the receiver to reserve this */
        settings.objectVariabless->reserve(activity);
        /* and change our local state        */
        objectScope = SCOPE_RESERVED;    /* do an assignment! */
    }
}


/**
 * Get a traceback line for the current instruction.
 *
 * @return The formatted string traceback.
 */
RexxString *RexxActivation::getTraceBack()
{
    return formatTrace(current, getSourceObject());
}


RexxDateTime RexxActivation::getTime()
/******************************************************************************/
/* Function:  Retrieve the current activation timestamp, retrieving a new     */
/*            timestamp if this is the first call for a clause                */
/******************************************************************************/
{
    /* not a valid time stamp?           */
    if (!settings.timestamp.valid)
    {
        // IMPORTANT:  If a time call resets the elapsed time clock, we don't
        // clear the value out.  The time needs to stay valid until the clause is
        // complete.  The time stamp value that needs to be used for the next
        // elapsed time call is the timstamp that was valid at the point of the
        // last call, which is our current old invalid one.  So, we need to grab
        // that value and set the elapsed time start point, then clear the flag
        // so that it will remain current.
        if (isElapsedTimerReset())
        {
            settings.elapsed_time = settings.timestamp.getUTCBaseTime();
            setElapsedTimerValid();
        }
        /* get a fresh time stamp            */
        SystemInterpreter::getCurrentTime(&settings.timestamp);
        /* got a new one                     */
        settings.timestamp.valid = true;
    }
    /* return the current time           */
    return settings.timestamp;
}


int64_t RexxActivation::getElapsed()
/******************************************************************************/
/* Function:  Retrieve the current elapsed time counter start time, starting  */
/*            the counter from the current time stamp if this is the first    */
/*            call                                                            */
/******************************************************************************/
{
    // no active elapsed time clock yet?
    if (settings.elapsed_time == 0)
    {

        settings.elapsed_time = settings.timestamp.getUTCBaseTime();
    }
    return settings.elapsed_time;
}


void RexxActivation::resetElapsed()     /* reset activation elapsed time     */
/******************************************************************************/
/* Function:  Retrieve the current elapsed time counter start time, starting  */
/*            the counter from the current time stamp if this is the first    */
/*            call                                                            */
/******************************************************************************/
{
    // Just invalidate the flat so that we'll refresh this the next time we
    // obtain a new timestamp value.
    setElapsedTimerInvalid();
}


#define DEFAULT_MIN 0                  /* default random minimum value      */
#define DEFAULT_MAX 999                /* default random maximum value      */
#define MAX_DIFFERENCE 999999999       /* max spread between min and max    */


uint64_t RexxActivation::getRandomSeed(RexxInteger * seed )
{
    /* currently in an internal routine  */
    /* or interpret instruction?         */
    if (isInternalLevelCall())
    {
        /* forward along                     */
        return parent->getRandomSeed(seed);
    }
    /* have a seed supplied?             */
    if (seed != OREF_NULL)
    {
        wholenumber_t seed_value = seed->getValue();     /* get the value                     */
        if (seed_value < 0)                /* negative value?                   */
        {
            /* got an error                      */
            reportException(Error_Incorrect_call_nonnegative, CHAR_RANDOM, IntegerThree, seed);
        }
        /* set the saved seed value          */
        randomSeed = seed_value;
        /* flip all of the bits              */
        randomSeed = ~randomSeed;
        /* randomize the seed number a bit   */
        for (size_t i = 0; i < 13; i++)
        {
            /* scramble the seed a bit           */
            randomSeed = RANDOMIZE(randomSeed);
        }
    }
    /* step the randomization            */
    randomSeed = RANDOMIZE(randomSeed);
    /* set the seed at the activity level*/
    activity->setRandomSeed(randomSeed);
    return randomSeed;            /* return the seed value             */
}


RexxInteger * RexxActivation::random(
  RexxInteger * randmin,               /* RANDOM minimum range              */
  RexxInteger * randmax,               /* RANDOM maximum range              */
  RexxInteger * randseed )             /* RANDOM seed                       */
/******************************************************************************/
/* Function:  Process the random function, using the current activation       */
/*            seed value.                                                     */
/******************************************************************************/
{
    size_t i;                           /* loop counter                      */

                                        /* go get the seed value             */
    uint64_t seed = getRandomSeed(randseed);

    wholenumber_t minimum = DEFAULT_MIN;  /* get the default MIN value         */
    wholenumber_t maximum = DEFAULT_MAX;  /* get the default MAX value         */
    /* minimum specified?                */
    if (randmin != OREF_NULL)
    {
        /* no maximum value specified        */
        /* and no seed specified             */
        if ((randmax == OREF_NULL) && (randseed == OREF_NULL))
        {
            maximum = randmin->getValue();  /* this is actually a max value      */
        }
        /* minimum value specified           */
        /* maximum value not specified       */
        /* seed specified                    */
        else if ((randmin != OREF_NULL) && (randmax == OREF_NULL) && (randseed != OREF_NULL))
        {
            minimum = randmin->getValue();
        }
        else
        {
            minimum = randmin->getValue();  /* give both max and min values      */
            maximum = randmax->getValue();
        }
    }
    else if (randmax != OREF_NULL)      /* only given a maximum?             */
    {
        maximum = randmax->getValue();    /* use the supplied maximum          */
    }

    if (maximum < minimum)              /* range problem?                    */
    {
        /* this is an error                  */
        reportException(Error_Incorrect_call_random, randmin, randmax);
    }
    /* too big of a spread ?              */
    if (maximum - minimum > MAX_DIFFERENCE)
    {
        /* this is an error                  */
        reportException(Error_Incorrect_call_random_range, randmin, randmax);
    }

    /* have real work to do?             */
    if (minimum != maximum)
    {
        // this will invert the bits of the value
        uint64_t work = 0;                  /* start with zero                   */
        for (i = 0; i < sizeof(uint64_t) * 8; i++)
        {
            work <<= 1;                     /* shift working num left one        */
                                            /* add in next seed bit value        */
            work = work | (seed & 0x01LL);
            seed >>= 1;                     /* shift off the right most seed bit */
        }
        /* adjust for requested range        */
        minimum += (wholenumber_t)(work % (uint64_t)(maximum - minimum + 1));
    }
    return new_integer(minimum);        /* return the random number          */
}

// table of trace prefixes
static const char * trace_prefix_table[] =
{
  "*-*",                               // TRACE_PREFIX_CLAUSE
  "+++",                               // TRACE_PREFIX_ERROR
  ">>>",                               // TRACE_PREFIX_RESULT
  ">.>",                               // TRACE_PREFIX_DUMMY
  ">V>",                               // TRACE_PREFIX_VARIABLE
  ">E>",                               // TRACE_PREFIX_DOTVARIABLE
  ">L>",                               // TRACE_PREFIX_LITERAL
  ">F>",                               // TRACE_PREFIX_FUNCTION
  ">P>",                               // TRACE_PREFIX_PREFIX
  ">O>",                               // TRACE_PREFIX_OPERATOR
  ">C>",                               // TRACE_PREFIX_COMPOUND
  ">M>",                               // TRACE_PREFIX_MESSAGE
  ">A>",                               // TRACE_PREFIX_ARGUMENT
  ">=>",                               // TRACE_PREFIX_ASSIGNMENT
  ">I>",                               // TRACE_PREFIX_INVOCATION
};


// TODO get rid of these defines
                                       /* extra space required to format a  */
                                       /* result line.  This overhead is    */
                                       /* 6 leading spaces for the line     */
                                       /* number, + 1 space + length of the */
                                       /* message prefix (3) + 1 space +    */
                                       /* 2 for an indent + 2 for the       */
                                       /* quotes surrounding the value      */
#define TRACE_OVERHEAD 15
                                       /* overhead for a traced instruction */
                                       /* (6 digit line number, blank,      */
                                       /* 3 character prefix, and a blank   */
#define INSTRUCTION_OVERHEAD 11
#define LINENUMBER 6                   /* size of a line number             */
#define PREFIX_OFFSET (LINENUMBER + 1) /* location of the prefix field      */
#define PREFIX_LENGTH 3                /* length of the prefix flag         */
#define INDENT_SPACING 2               /* spaces per indentation amount     */
// over head for adding quotes
#define QUOTES_OVERHEAD 2

/**
 * Trace program entry for a method or routine
 */
void RexxActivation::traceEntry()
{
    // since we're advertising the entry location up front, we want to disable
    // the normal trace-turn on notice.  We'll get one or the other, but not
    // both
    settings.stateFlags[sourceTraced] = true;

    ArrayClass *info = OREF_NULL;

    if (isMethod())
    {
        info = new_array(getMessageName(), scope->getId(), getPackage()->getName());
    }
    else
    {
        info = new_array(getExecutable()->getName(), getPackage()->getName());
    }
    ProtectedObject p(info);

    RexxString *message = activity->buildMessage(isRoutine() ? Message_Translations_routine_invocation : Message_Translations_method_invocation, info);
    p = message;

    /* get a string large enough to      */
    size_t outlength = message->getLength() + INSTRUCTION_OVERHEAD;
    RexxString *buffer = raw_string(outlength);      /* get an output string              */
    /* insert the leading blanks         */
    buffer->set(0, ' ', INSTRUCTION_OVERHEAD);
    /* add the trace prefix              */
    buffer->put(PREFIX_OFFSET, trace_prefix_table[TRACE_PREFIX_INVOCATION], PREFIX_LENGTH);
    /* copy the string value             */
    buffer->put(INSTRUCTION_OVERHEAD, message->getStringData(), message->getLength());
                                         /* write out the line                */
    activity->traceOutput(this, buffer);
}


void RexxActivation::traceValue(       /* trace an intermediate value       */
     RexxObject * value,               /* value to trace                    */
     int          prefix )             /* traced result type                */
/******************************************************************************/
/* Function:  Trace an intermediate value or instruction result value         */
/******************************************************************************/
{
    /* tracing currently suppressed or   */
    /* no value was received?            */
    if (noTracing(value))
    {
        return;                            /* just ignore this call             */
    }

    if (!code->isTraceable())      /* if we don't have real source      */
    {
        return;                            /* just ignore for this              */
    }
                                           /* get the string version            */
    RexxString *stringvalue = value->stringValue();
                                           /* get a string large enough to      */
    size_t outlength = stringvalue->getLength() + TRACE_OVERHEAD + settings.traceindent * INDENT_SPACING;
    RexxString *buffer = raw_string(outlength);      /* get an output string              */
    ProtectedObject p(buffer);
    /* insert the leading blanks         */
    buffer->set(0, ' ', TRACE_OVERHEAD + settings.traceindent * INDENT_SPACING);
    /* add the trace prefix              */
    buffer->put(PREFIX_OFFSET, trace_prefix_table[prefix], PREFIX_LENGTH);
    /* add a quotation mark              */
    buffer->putChar(TRACE_OVERHEAD - 2 + settings.traceindent * INDENT_SPACING, '\"');
    /* copy the string value             */
    buffer->put(TRACE_OVERHEAD - 1 + settings.traceindent * INDENT_SPACING, stringvalue->getStringData(), stringvalue->getLength());
    buffer->putChar(outlength - 1, '\"');/* add the trailing quotation mark   */
                                         /* write out the line                */
    activity->traceOutput(this, buffer);
}


/**
 * Trace an entry that's of the form 'tag => "value"'.
 *
 * @param prefix    The trace prefix tag to use.
 * @param tagPrefix Any prefix string added to the tag.  Use mostly for adding
 *                  the "." to traced environment variables.
 * @param quoteTag  Indicates whether the tag should be quoted or not.  Operator
 *                  names are quoted.
 * @param tag       The tag name.
 * @param value     The associated trace value.
 */
void RexxActivation::traceTaggedValue(int prefix, const char *tagPrefix, bool quoteTag,
     RexxString *tag, const char *marker, RexxObject * value)
{
    // the trace settings would normally require us to trace this, but there are conditions
    // where we just skip doing this anyway.
    if (noTracing(value))
    {
        return;
    }

    // get the string value from the traced object.
    RexxString *stringVal = value->stringValue();

    // now calculate the length of the traced string
    stringsize_t outLength = tag->getLength() + stringVal->getLength();
    // these are fixed overheads
    outLength += TRACE_OVERHEAD + strlen(marker);
    // now the indent spacing
    outLength += settings.traceindent * INDENT_SPACING;
    // now other conditionals
    outLength += quoteTag ? QUOTES_OVERHEAD : 0;
    // this is usually null, but dot variables add a "." to the tag.
    outLength += tagPrefix == NULL ? 0 : strlen(tagPrefix);

    // now get a buffer to write this out into
    RexxString *buffer = raw_string(outLength);
    ProtectedObject p(buffer);

    // get a cursor for filling in the formatted data
    stringsize_t dataOffset = TRACE_OVERHEAD + settings.traceindent * INDENT_SPACING - 2;
                                       /* insert the leading blanks         */
    buffer->set(0, ' ', TRACE_OVERHEAD + settings.traceindent * INDENT_SPACING);
                                       /* add the trace prefix              */
    buffer->put(PREFIX_OFFSET, trace_prefix_table[prefix], PREFIX_LENGTH);

    // if this is a quoted tag (operators do this), add quotes before coping the tag
    if (quoteTag)
    {
        buffer->putChar(dataOffset, '\"');
        dataOffset++;
    }
    // is the tag prefixed?  Add this before the name
    if (tagPrefix != NULL)
    {
        stringsize_t prefixLength = strlen(tagPrefix);
        buffer->put(dataOffset, tagPrefix, prefixLength);
        dataOffset += prefixLength;
    }

    // add in the tag name
    buffer->put(dataOffset, tag);
    dataOffset += tag->getLength();

    // might need a closing quote.
    if (quoteTag)
    {
        buffer->putChar(dataOffset, '\"');
        dataOffset++;
    }

    // now add the data marker
    buffer->put(dataOffset, marker, strlen(marker));
    dataOffset += strlen(marker);

    // the leading quote around the value
    buffer->putChar(dataOffset, '\"');
    dataOffset++;

    // the traced value
    buffer->put(dataOffset, stringVal);
    dataOffset += stringVal->getLength();

    // and finally, the trailing quote
    buffer->putChar(dataOffset, '\"');
    dataOffset++;
                                       /* write out the line                */
    activity->traceOutput(this, buffer);
}


/**
 * Trace an entry that's of the form 'tag => "value"'.
 *
 * @param prefix    The trace prefix tag to use.
 * @param tagPrefix Any prefix string added to the tag.  Use mostly for adding
 *                  the "." to traced environment variables.
 * @param quoteTag  Indicates whether the tag should be quoted or not.  Operator
 *                  names are quoted.
 * @param tag       The tag name.
 * @param value     The associated trace value.
 */
void RexxActivation::traceOperatorValue(int prefix, const char *tag, RexxObject *value)
{
    // the trace settings would normally require us to trace this, but there are conditions
    // where we just skip doing this anyway.
    if (noTracing(value))
    {
        return;
    }

    // get the string value from the traced object.
    RexxString *stringVal = value->stringValue();

    // now calculate the length of the traced string
    stringsize_t outLength = strlen(tag) + stringVal->getLength();
    // these are fixed overheads
    outLength += TRACE_OVERHEAD + strlen(VALUE_MARKER);
    // now the indent spacing
    outLength += settings.traceindent * INDENT_SPACING;
    // now other conditionals
    outLength += QUOTES_OVERHEAD;

    // now get a buffer to write this out into
    RexxString *buffer = raw_string(outLength);
    ProtectedObject p(buffer);

    // get a cursor for filling in the formatted data
    stringsize_t dataOffset = TRACE_OVERHEAD + settings.traceindent * INDENT_SPACING - 2;
                                       /* insert the leading blanks         */
    buffer->set(0, ' ', TRACE_OVERHEAD + settings.traceindent * INDENT_SPACING);
                                       /* add the trace prefix              */
    buffer->put(PREFIX_OFFSET, trace_prefix_table[prefix], PREFIX_LENGTH);

    // operators are quoted.
    buffer->putChar(dataOffset, '\"');
    dataOffset++;

    // add in the tag name
    buffer->put(dataOffset, tag, strlen(tag));
    dataOffset += strlen(tag);

    // need a closing quote.
    buffer->putChar(dataOffset, '\"');
    dataOffset++;

    // now add the data marker
    buffer->put(dataOffset, VALUE_MARKER, strlen(VALUE_MARKER));
    dataOffset += strlen(VALUE_MARKER);

    // the leading quote around the value
    buffer->putChar(dataOffset, '\"');
    dataOffset++;

    // the traced value
    buffer->put(dataOffset, stringVal);
    dataOffset += stringVal->getLength();

    // and finally, the trailing quote
    buffer->putChar(dataOffset, '\"');
    dataOffset++;
                                       /* write out the line                */
    activity->traceOutput(this, buffer);
}


/**
 * Trace a compound variable entry that's of the form 'tag =>
 * "value"'.
 *
 * @param prefix    The trace prefix tag to use.
 * @param stem      The stem name of the compound.
 * @param tails     The array of tail elements (unresolved).
 * @param tailCount The count of tail elements.
 * @param value     The resolved tail element
 */
void RexxActivation::traceCompoundValue(int prefix, RexxString *stemName, RexxObject **tails, size_t tailCount,
     CompoundVariableTail *tail)
{
    traceCompoundValue(TRACE_PREFIX_COMPOUND, stemName, tails, tailCount, VALUE_MARKER, tail->createCompoundName(stemName));
}


/**
 * Trace a compound variable entry that's of the form 'tag =>
 * "value"'.
 *
 * @param prefix    The trace prefix tag to use.
 * @param stem      The stem name of the compound.
 * @param tails     The array of tail elements (unresolved).
 * @param tailCount The count of tail elements.
 * @param value     The associated trace value.
 */
void RexxActivation::traceCompoundValue(int prefix, RexxString *stemName, RexxObject **tails, size_t tailCount, const char *marker,
     RexxObject * value)
{
    // the trace settings would normally require us to trace this, but there are conditions
    // where we just skip doing this anyway.
    if (noTracing(value))
    {
        return;
    }

    // get the string value from the traced object.
    RexxString *stringVal = value->stringValue();

    // now calculate the length of the traced string
    stringsize_t outLength = stemName->getLength() + stringVal->getLength();

    // build an unresolved tail name
    CompoundVariableTail tail(tails, tailCount, false);

    outLength += tail.getLength();

    // add in the number of added dots
    outLength += tailCount - 1;

    // these are fixed overheads
    outLength += TRACE_OVERHEAD + strlen(marker);
    // now the indent spacing
    outLength += settings.traceindent * INDENT_SPACING;

    // now get a buffer to write this out into
    RexxString *buffer = raw_string(outLength);
    ProtectedObject p(buffer);

    // get a cursor for filling in the formatted data
    stringsize_t dataOffset = TRACE_OVERHEAD + settings.traceindent * INDENT_SPACING - 2;
                                       /* insert the leading blanks         */
    buffer->set(0, ' ', TRACE_OVERHEAD + settings.traceindent * INDENT_SPACING);
                                       /* add the trace prefix              */
    buffer->put(PREFIX_OFFSET, trace_prefix_table[prefix], PREFIX_LENGTH);

    // add in the stem name
    buffer->put(dataOffset, stemName);
    dataOffset += stemName->getLength();

    // copy the tail portion of the compound name
    buffer->put(dataOffset, tail.getTail(), tail.getLength());
    dataOffset += tail.getLength();

    // now add the data marker
    buffer->put(dataOffset, marker, strlen(marker));
    dataOffset += strlen(marker);

    // the leading quote around the value
    buffer->putChar(dataOffset, '\"');
    dataOffset++;

    // the traced value
    buffer->put(dataOffset, stringVal);
    dataOffset += stringVal->getLength();

    // and finally, the trailing quote
    buffer->putChar(dataOffset, '\"');
    dataOffset++;
                                       /* write out the line                */
    activity->traceOutput(this, buffer);
}


void RexxActivation::traceSourceString()
/******************************************************************************/
/* Function:  Trace the source string at debug mode start                     */
/******************************************************************************/
{
    /* already traced?                   */
    if (settings.stateFlags[sourceTraced])
    {
        return;                            /* don't do it again                 */
    }
                                           /* tag this as traced                */
    settings.stateFlags[sourceTraced] = true;
    /* get the string version            */
    RexxString *string = sourceString();       /* get the source string             */
    /* get a string large enough to      */
    size_t outlength = string->getLength() + INSTRUCTION_OVERHEAD + 2;
    RexxString *buffer = raw_string(outlength);      /* get an output string              */
    /* insert the leading blanks         */
    buffer->set(0, ' ', INSTRUCTION_OVERHEAD);
    /* add the trace prefix              */
    buffer->put(PREFIX_OFFSET, trace_prefix_table[TRACE_PREFIX_ERROR], PREFIX_LENGTH);
    /* add a quotation mark              */
    buffer->putChar(INSTRUCTION_OVERHEAD, '\"');
    /* copy the string value             */
    buffer->put(INSTRUCTION_OVERHEAD + 1, string->getStringData(), string->getLength());
    buffer->putChar(outlength - 1, '\"');/* add the trailing quotation mark   */
                                         /* write out the line                */
    activity->traceOutput(this, buffer);
}


RexxString *RexxActivation::formatTrace(
   RexxInstruction *  instruction,     /* instruction to trace              */
   PackageClass    *  package )        /* program source                    */
/******************************************************************************/
/* Function:  Format a source line for traceback or tracing                   */
/******************************************************************************/
{
    if (instruction == OREF_NULL)        /* no current instruction?           */
    {
        return OREF_NULL;                  /* nothing to trace here             */
    }
    // get the instruction location
    SourceLocation location = instruction->getLocation();
                                           /* extract the source string         */
                                           /* (formatted for tracing)           */
    if (settings.traceindent < MAX_TRACEBACK_INDENT)
    {
        return package->traceBack(this, location, settings.traceindent, true);
    }
    else
    {
        return package->traceBack(this, location, MAX_TRACEBACK_INDENT, true);
    }
}


/**
 * Handle all clause boundary processing (raising of halt
 * conditions, turning on of external traces, and calling of halt
 * and trace clause boundary exits
 */
void RexxActivation::processClauseBoundary()
{
    // do we have any pending CALL ON conditions?  Dispatch those
    // now.
    if (conditionQueue != OREF_NULL && !conditionQueue->isEmpty())
    {
        processTraps();
    }

    // test any halt exit wants to raise a halt condition.
    activity->callHaltTestExit(this);
    // check for external traces
    if (!activity->callTraceTestExit(this, isExternalTraceOn()))
    {
        // remember how this flipped
        if (isExternalTraceOn())
        {
            setExternalTraceOff();
        }
        else
        {
            setExternalTraceOn();
        }
    }

    // asked to yield control?
    if (settings.stateFlags[externalYield)
    {
        // turn off the flag and give up control
        settings.stateFlags[externalYield] = false;
        activity->relinquish();
    }

    // have a halt condition?
    if (settings.stateFlags[haltCondition])
    {
        // flip this off and raise the condition
        // if not handled as a condition, turn into a syntax error
        settings.stateFlags[haltCondition] = false;
        if (!activity->raiseCondition(OREF_HALT, OREF_NULL, settings.halt_description, OREF_NULL, OREF_NULL))
        {
            reportException(Error_Program_interrupted_condition, OREF_HALT);
        }
    }

    // been asked to turn on tracing?
    if (settings.stateFlags[setTraceOn])
    {
        settings.stateFlags[setTraceOn] = false;
        setExternalTraceOn();
        setTrace(LanguageParser::TRACE_RESULTS | LanguageParser::DEBUG_ON, trace_results_flags | trace_debug);
    }

    // maybe turing tracing off?
    if (settings.stateFlags[setTraceOff])
    {
        settings.stateFlags[setTraceOff] = false;
        setExternalTraceOff();
        setTrace(LanguageParser::TRACE_OFF | LanguageParser::DEBUG_OFF, trace_off);
    }

    // now set the boundary flag based on whether we still have pending stuff for
    // next go around.
    claseBoundary = (settings.stateFlags[clauseExits] && (conditionQueue == OREF_NULL || conditionQueue->isEmpty());
}


/**
 * Turn on external trace at program startup (e.g, because
 * RXTRACE is set)
 */
void RexxActivation::enableExternalTrace()
{
    setTrace(LanguageParser::TRACE_RESULTS | LanguageParser::DEBUG_ON, trace_results_flags | trace_debug);
}


/**
 * Halt the activation
 *
 * @param description
 *               The description for the halt condition (if any).
 *
 * @return true if this halt was recognized, false if there is a
 *         previous halt condition still to be processed.
 */
bool RexxActivation::halt(RexxString *description )
{
    // if there's no halt condition pending, set this
    if ((settings.stateFlags[haltCondition])
    {
        // store the description
        settings.haltDescription = description;
        // turn on the halt flag and also the clause boundary
        // flag so that this gets processed.
        settings.stateFlags[halt_condition] = true;
        clauseBoundary = true;
        return true;
    }
    else
    {
        // we're not in a good position to process this
        return false;
    }
}


/**
 * Flip ON the externally activated TRACE bit.
 */
void RexxActivation::yield()
{
    settings.stateFlags[externalYield] = true;
    clauseBoundary = true;
}


/**
 * Flip ON the externally activated TRACE bit.
 */
void RexxActivation::externalTraceOn()
{
    // turn on the tracing flags and have this checked at the next clause boundary
    settings.stateFlags[setTraceOn] = true;
    clauseBoundary = true;
}


/**
 * Flip OFF the externally activated TRACE bit.
 */
void RexxActivation::externalTraceOff()
{
    // turn on the tracing flags and have this checked at the next clause boundary
    settings.stateFlags[setTraceOff] = true;
    clauseBoundary = true;
}


/**
 * Process an individual debug pause for an instruction
 *
 * @return true if the instruction should re-execute, false otherwise.
 */
bool RexxActivation::doDebugPause()
{
    // already in debug pause?  just skip pausing
    if (debugPause)
    {
        return false;
    }

    // asked to bypass...turn this off for the next time.
    if (settings.stateFlags[debugBypass])
    {
        settings.stateFlags[debugBypass] = false;
    }
    // debug pauses suppressed?  Reduce the count and turn debug pausing
    // back on for the next time.
    else if (settings.traceSkip > 0)
    {
        settings.traceSkip--;
        if (settings.traceSkip == 0)
        {
            // turn tracing back on again (this
            // ensures the next pause also has
            // the instruction traced
            settings.traceFlags[traceSuppress] = false;
        }
    }
    // normal pause
    else
    {
        // if we don't have real source code for this instruction, we can't pause.
        if (!code->isTraceable())
        {
            return false;
        }
        // first time paused?
        if (!settings.stateFlags[debugPromptIssued])
        {
            // write the initial prompt and turn off for the next time.
            activity->traceOutput(this, SystemInterpreter::getMessageText(Message_Translations_debug_prompt));
            settings.stateFlags[debugPromptIssued] = true;
        }
        // save the next instruction in case we're asked to re-execute
        RexxInstruction *currentInst = next;
        for (;;)
        {
            RexxString *response = activity->traceInput(this);
            // a null line just advances
            if (response->getLength() == 0)
            {
                break;
            }
            // a re-execute request ("=")?
            // we reset the next instruction and return an indicator that
            // we need to re-execute.  Some instructions (e.g., block instructions)
            // need to undo some side effects of execution.
            else if (response->getLength() == 1 && response->getChar(0) == '=')
            {
                next = current;
                return true;
            }
            else
            {
                // interpret the instruction
                debugInterpret(response);
                // if we've had a flow of control change, we're done.
                if (currentInst != next)
                {
                    break;
                }
                // the trace setting may have changed on us.
                else if (settings.stateFlags[debugBypass])
                {
                    // turn off the bypass setting.  Is for situations where a
                    // trace in normal code turns on debug.  The debug pause is
                    // skipped until the next instruction
                    settings.stateFlags[debugBypass] = false;
                    break;
                }
            }
        }
    }
    // no re-execution needed
    return false;
}

/**
 * Trace an individual line of a source file
 *
 * @param clause the clause to trace.
 * @param prefix
 */
// TODO:  used a enum type for the prefix argument.
void RexxActivation::traceClause(RexxInstruction *clause, int prefix)
{
    // we might be in a state where tracing is suppressed, or there's no source available.
    if (!canTrace())
    {
        return;
    }
    // format the trace line
    RexxString *line = formatTrace(clause, code->getPackageObject());
    // do we have a real source line we can trace, go output it.
    if (line != OREF_NULL)
    {
        // if we've just dropped into debug mode, we need to put out the extra context line.
        if ((settings.traceFlags[traceDebug] && !settings.stateFlags[sourceTraced]))
        {
            traceSourceString();
        }
        activity->traceOutput(this, line);
    }
}

/**
 * Issue a command to a named host evironment
 *
 * @param commandString
 *                The command to issue
 * @param address The target address
 *
 * @return The return code object
 */
void RexxActivation::command(RexxString *address, RexxString *commandString)
{
    // if we are tracing command instructions, then we need to add some
    // additional trace information afterward.  Also, if we're tracing errors or
    // failures, we similarly need to know if the command has already been traced.
    bool instruction_traced = tracingAll() || tracingCommands();
    ProtectedObject condition;
    ProtectedObject commandResult;

    // give the command exit first pass at this.
    if (activity->callCommandExit(this, address, commandString, commandResult, condition))
    {
        // first check for registered command handlers.  If we have a handler, then
        // call it.  This includes the default system command handlers.
        CommandHandler *handler = activity->resolveCommandHandler(address);
        if (handler != OREF_NULL)
        {
            handler->call(activity, this, address, commandString, commandResult, condition);
        }
        else
        {
            // No handler for this environment.  Give a default return code and
            // raise a failure condition.
            commandResult = new_integer(RXSUBCOM_NOTREG);   // just use the not registered return code
            condition = activity->createConditionObject(OREF_FAILURENAME, (RexxObject *)commandResult, commandString, OREF_NULL, OREF_NULL);
        }
    }

    // now process the command result.
    RexxObject *rc = (RexxObject *)commandResult;
    DirectoryClass *conditionObj = (DirectoryClass *)(RexxObject *)condition;

    bool failureCondition = false;    // don't have a failure condition yet

    int returnStatus = RETURN_STATUS_NORMAL;
    // did a handler raise a condition?  We need to pull the rc value from the
    // condition object
    if (conditionObj != OREF_NULL)
    {
        RexxObject *temp = conditionObj->get(OREF_RC);
        if (temp == OREF_NULL)
        {
            // see if we have a result and make sure the condition object
            // fills this as the RC value
            temp = conditionObj->get(OREF_RESULT);
            if (temp != OREF_NULL)
            {
                conditionObj->put(temp, OREF_RC);
            }
        }
        // replace the RC value
        if (temp != OREF_NULL)
        {
            rc = temp;
        }

        // now check for ERROR or FAILURE conditions
        RexxString *conditionName = (RexxString *)conditionObj->get(OREF_CONDITION);
        // check for an error or failure condition, since these get special handling
        if (conditionName->strCompare(CHAR_FAILURENAME))
        {
            // unconditionally update the RC value
            conditionObj->put(temp, OREF_RC);
            // failure conditions require special handling when raising the condition
            // we'll need to reraise this as an ERROR condition if not trapped.
            failureCondition = true;
            // set the appropriate return status
            returnStatus = RETURN_STATUS_FAILURE;
        }
        if (conditionName->strCompare(CHAR_ERROR))
        {
            // unconditionally update the RC value
            conditionObj->put(temp, OREF_RC);
            // set the appropriate return status
            returnStatus = RETURN_STATUS_ERROR;
        }
    }

    // a handler might not return a value, so default the return code to zero
    // if nothing is received.
    if (rc == OREF_NULL)
    {
        rc = TheFalseObject;
    }

    // if this was done during a debug pause, we don't update RC
    // and .RS.
    if (!debugPause)
    {
        // set the RC value before anything
        setLocalVariable(OREF_RC, VARIABLE_RC, rc);
        // tracing command errors or fails?
        if ((returnStatus == RETURN_STATUS_ERROR && tracingErrors()) ||
            (returnStatus == RETURN_STATUS_FAILURE && (tracingFailures())))
        {
            // trace the current instruction
            traceClause(current, TRACE_PREFIX_CLAUSE);
            // then we always trace full command
            traceValue(commandString, TRACE_PREFIX_RESULT);
            // this has been traced
            instruction_traced = true;
        }

        wholenumber_t rcValue;
        // need to trace the RC info too?
        if (instruction_traced && rc->numberValue(rcValue) && rcValue != 0)
        {
            // this has a special RC(val) format
            RexxString *rc_trace = rc->stringValue();
            rc_trace = rc_trace->concatToCstring("RC(");
            rc_trace = rc_trace->concatWithCstring(")");
            traceValue(rc_trace, TRACE_PREFIX_ERROR);
        }
        // set the return status
        setReturnStatus(returnStatus);

        // now handle any conditions we might need to raise
        // these are also not raised if it's a debug pause.
        if (conditionObj != OREF_NULL)
        {
            // try to raise the condition, and if it isn't handled, we might
            // munge this into an ERROR condition
            if (!activity->raiseCondition(conditionObj))
            {
                // untrapped failure condition?  Turn into an ERROR condition and
                // reraise
                if (failureCondition)
                {
                    // just change the condition name
                    conditionObj->put(OREF_ERRORNAME, OREF_CONDITION);
                    activity->raiseCondition(conditionObj);
                }
            }
        }

        // do debug pause if necessary.  necessary is defined by:  we are
        // tracing ALL or COMMANDS, OR, we are using TRACE NORMAL and a FAILURE
        // return code was received OR we receive an ERROR return code and
        // have TRACE ERROR in effect.
        if (instruction_traced && inDebug())
        {
            debugPause();
        }
    }
}


/**
 * Set the return status flag for an activation context.
 *
 * @param status The new status value.
 */
// TODO:  use enum type here
void RexxActivation::setReturnStatus(int status)
{
    settings.returnStatus = status;
    settings.statusFlags[returnStatusSet] = true;
}


RexxString * RexxActivation::getProgramName()
/******************************************************************************/
/* Function:  Return the name of the current program file                     */
/******************************************************************************/
{
  return code->getProgramName(); /* get the name from the code        */
}

DirectoryClass * RexxActivation::getLabels()
/******************************************************************************/
/* Function:  Return the directory of labels for this method                  */
/******************************************************************************/
{
  return code->getLabels();      /* get the labels from the code      */
}

RexxString * RexxActivation::sourceString()
/******************************************************************************/
/* Function:  Create the source string returned by parse source               */
/******************************************************************************/
{
                                       /* produce the system specific string*/
  return SystemInterpreter::getSourceString(settings.calltype, code->getProgramName());
}


/**
 * Add a local routine to the current activation's routine set.
 *
 * @param name   The name to add this under.
 * @param method The method associated with the name.
 */
void RexxActivation::addLocalRoutine(RexxString *name, MethodClass *_method)
{
    // get the directory of external functions
    DirectoryClass *routines = settings.parentCode->getLocalRoutines();

    // if it does not exist, it will be created
    if (routines == OREF_NULL)
    {

        settings.parentCode->getSourceObject()->setLocalRoutines(new_directory());
        routines = settings.parentCode->getLocalRoutines();
    }
    // if a method by that name exists, it will be OVERWRITTEN!
    routines->setEntry(name, _method);
}


/**
 * Retrieve the directory of public routines associated with the
 * current activation.
 *
 * @return A directory of the public routines.
 */
DirectoryClass *RexxActivation::getPublicRoutines()
{
    return code->getPublicRoutines();
}



void RexxActivation::setObjNotify(
     MessageClass    * notify)          /* activation to notify              */
/******************************************************************************/
/* Function:  Set an error notification tag on the activation.                */
/******************************************************************************/
{
  objnotify = notify;
}


void RexxActivation::pushEnvironment(
     RexxObject * environment)         /* new local environment buffer        */
/******************************************************************************/
/* Function:  Push the new environment buffer onto the EnvLIst                */
/******************************************************************************/
{
    /* internal call or interpret?         */
    if (isTopLevelCall())
    {
        /* nope, push environment here.        */
        /* DO we have a environment list?      */
        if (!environmentList)
        {
            /* nope, create one                    */
            environmentList = new_list();
        }
        environmentList->addFirst(environment);
    }
    else                                 /* nope, process up the chain.         */
    {
        /* Yes, forward on the message.        */
        parent->pushEnvironment(environment);
    }
}

RexxObject * RexxActivation::popEnvironment()
/******************************************************************************/
/* Function:  return the top level local Environemnt                          */
/******************************************************************************/
{
    /* internal call or interpret?         */
    if (isTopLevelCall())
    {
        /* nope, we puop Environemnt here      */
        /* DO we have a environment list?      */
        if (environmentList)
        {
            /* yup, return first element           */
            return  environmentList->removeFirst();

        }
        else                               /* nope, return .nil                   */
        {
            return TheNilObject;
        }
    }
    else
    {                               /* nope, pass on up the chain.         */
                                    /* Yes, forward on the message.        */
        return parent->popEnvironment();
    }
}

void RexxActivation::closeStreams()
/******************************************************************************/
/* Function:  Close any streams opened by the I/O builtin functions           */
/******************************************************************************/
{
    RexxString    *index;                /* index for stream directory        */

                                         /* exiting a bottom level?           */
    if (isProgramOrMethod())
    {
        DirectoryClass *streams = settings.streams;  /* get the streams directory         */
        /* actually have a table?            */
        if (streams != OREF_NULL)
        {
            /* traverse this                     */
            for (HashLink j = streams->first(); (index = (RexxString *)streams->index(j)) != OREF_NULL; j = streams->next(j))
            {
                /* closing each stream               */
                streams->at(index)->sendMessage(OREF_CLOSE);
            }
        }
    }
}


RexxObject  *RexxActivation::novalueHandler(
     RexxString *name )                /* name to retrieve                  */
/******************************************************************************/
/* Function:  process unitialized variable over rides                         */
/******************************************************************************/
{
    /* get the handler from .local       */
    RexxObject *novalue_handler = getLocalEnvironment(OREF_NOVALUE);
    if (novalue_handler != OREF_NULL)    /* have a novalue handler?           */
    {
        /* ask it to process this            */
        return novalue_handler->sendMessage(OREF_NOVALUE, name);
    }
    return TheNilObject;                 /* return the handled result         */
}

/**
 * Retrieve the package for the current execution context.
 *
 * @return The Package holding the code for the current execution
 *         context.
 */
PackageClass *RexxActivation::getPackage()
{
    return executable->getPackage();
}


RexxObject *RexxActivation::evaluateLocalCompoundVariable(RexxString *stemName, size_t index, RexxObject **tail, size_t tailCount)
{
                                         /* new tail for compound             */
    CompoundVariableTail resolved_tail(this, tail, tailCount);

    StemClass *stem_table = getLocalStem(stemName, index);   /* get the stem entry from this dictionary */
    RexxObject *value = stem_table->evaluateCompoundVariableValue(this, stemName, &resolved_tail);
    /* need to trace?                    */
    if (tracingIntermediates())
    {
        traceCompoundName(stemName, tail, tailCount, &resolved_tail);
        /* trace variable value              */
        traceCompound(stemName, tail, tailCount, value);
    }
    return value;
}


RexxObject *RexxActivation::getLocalCompoundVariableValue(RexxString *stemName, size_t index, RexxObject **tail, size_t tailCount)
{
                                         /* new tail for compound             */
    CompoundVariableTail resolved_tail(this, tail, tailCount);

    StemClass *stem_table = getLocalStem(stemName, index);   /* get the stem entry from this dictionary */
    return stem_table->getCompoundVariableValue(&resolved_tail);
}


RexxObject *RexxActivation::getLocalCompoundVariableRealValue(RexxString *localstem, size_t index, RexxObject **tail, size_t tailCount)
{
                                         /* new tail for compound             */
    CompoundVariableTail resolved_tail(this, tail, tailCount);

    StemClass *stem_table = getLocalStem(localstem, index);   /* get the stem entry from this dictionary */
    return stem_table->getCompoundVariableRealValue(&resolved_tail);
}


CompoundTableElement *RexxActivation::getLocalCompoundVariable(RexxString *stemName, size_t index, RexxObject **tail, size_t tailCount)
{
                                         /* new tail for compound             */
    CompoundVariableTail resolved_tail(this, tail, tailCount);

    StemClass *stem_table = getLocalStem(stemName, index);   /* get the stem entry from this dictionary */
    return stem_table->getCompoundVariable(&resolved_tail);
}


CompoundTableElement *RexxActivation::exposeLocalCompoundVariable(RexxString *stemName, size_t index, RexxObject **tail, size_t tailCount)
{
                                         /* new tail for compound             */
    CompoundVariableTail resolved_tail(this, tail, tailCount);

    StemClass *stem_table = getLocalStem(stemName, index);   /* get the stem entry from this dictionary */
    return stem_table->exposeCompoundVariable(&resolved_tail);
}


bool RexxActivation::localCompoundVariableExists(RexxString *stemName, size_t index, RexxObject **tail, size_t tailCount)
{
                                         /* new tail for compound             */
    CompoundVariableTail resolved_tail(this, tail, tailCount);

    StemClass *stem_table = getLocalStem(stemName, index);   /* get the stem entry from this dictionary */
    return stem_table->compoundVariableExists(&resolved_tail);
}


void RexxActivation::assignLocalCompoundVariable(RexxString *stemName, size_t index, RexxObject **tail, size_t tailCount, RexxObject *value)
{
                                              /* new tail for compound             */
    CompoundVariableTail resolved_tail(this, tail, tailCount);

    StemClass *stem_table = getLocalStem(stemName, index);   /* get the stem entry from this dictionary */
    /* and set the value                 */
    stem_table->setCompoundVariable(&resolved_tail, value);
    /* trace resolved compound name */
    if (tracingIntermediates())
    {
        traceCompoundName(stemName, tail, tailCount, &resolved_tail);
        /* trace variable value              */
        traceCompoundAssignment(stemName, tail, tailCount, value);
    }
}


void RexxActivation::setLocalCompoundVariable(RexxString *stemName, size_t index, RexxObject **tail, size_t tailCount, RexxObject *value)
{
                                              /* new tail for compound             */
    CompoundVariableTail resolved_tail(this, tail, tailCount);

    StemClass *stem_table = getLocalStem(stemName, index);   /* get the stem entry from this dictionary */
    /* and set the value                 */
    stem_table->setCompoundVariable(&resolved_tail, value);
}


void RexxActivation::dropLocalCompoundVariable(RexxString *stemName, size_t index, RexxObject **tail, size_t tailCount)
{
                                              /* new tail for compound             */
    CompoundVariableTail resolved_tail(this, tail, tailCount);

    StemClass *stem_table = getLocalStem(stemName, index);   /* get the stem entry from this dictionary */
    /* and set the value                 */
    stem_table->dropCompoundVariable(&resolved_tail);
}


/**
 * Get the security manager in effect for a given context.
 *
 * @return The security manager defined for this activation
 *         context.
 */
SecurityManager *RexxActivation::getSecurityManager()
{
    return settings.securityManager;
}


/**
 * Get the security manager in used by this activation.
 *
 * @return Either the defined security manager or the instance-global security
 *         manager.
 */
SecurityManager *RexxActivation::getEffectiveSecurityManager()
{
    SecurityManager *manager = settings.securityManager;
    if (manager != OREF_NULL)
    {
        return manager;
    }
    return activity->getInstanceSecurityManager();
}


/**
 * Retrieve a value from the instance local environment.
 *
 * @param name   The name of the .local object.
 *
 * @return The object stored at the given name.
 */
RexxObject *RexxActivation::getLocalEnvironment(RexxString *name)
{
    return activity->getLocalEnvironment(name);
}


/**
 * Create a stack frame for exception tracebacks.
 *
 * @return A StackFrame instance for this activation.
 */
StackFrameClass *RexxActivation::createStackFrame()
{
    const char *type = StackFrameClass::FRAME_METHOD;
    ArrayClass *arguments = OREF_NULL;
    RexxObject *target = OREF_NULL;

    if (isInterpret())
    {
        type = StackFrameClass::FRAME_INTERPRET;
    }
    else if (isInternalCall())
    {
        type = StackFrameClass::FRAME_INTERNAL_CALL;
        arguments = getArguments();
    }
    else if (isMethod())
    {
        type = StackFrameClass::FRAME_METHOD;
        arguments = getArguments();
        target = receiver;
    }
    else if (isProgram())
    {
        type = StackFrameClass::FRAME_PROGRAM;
        arguments = getArguments();
    }
    else if (isRoutine())
    {
        type = StackFrameClass::FRAME_ROUTINE;
        arguments = getArguments();
    }

    // construct the traceback line before we allocate the stack frame object.
    // calling this in the constructor argument list can cause the stack frame instance
    // to be inadvertently reclaimed if a GC is triggered while evaluating the constructor
    // arguments.
    RexxString *traceback = getTraceBack();
    return new StackFrameClass(type, getMessageName(), getExecutableObject(), target, arguments, traceback, getContextLineNumber());
}

/**
 * Format a more informative trace line when giving
 * traceback information for code when no source code is
 * available.
 *
 * @param packageName
 *               The package name to use (could be "REXX" for internal code)
 *
 * @return A formatted descriptive string for the invocation.
 */
RexxString *RexxActivation::formatSourcelessTraceLine(RexxString *packageName)
{
    // if this is a method invocation, then we can give the method name and scope.
    if (isMethod())
    {
        ArrayClass *info = new_array(getMessageName(), scope->getId(), packageName);
        ProtectedObject p(info);

        return activity->buildMessage(Message_Translations_sourceless_method_invocation, info);
    }
    else if (isRoutine())
    {
        ArrayClass *info = new_array(getMessageName(), packageName);
        ProtectedObject p(info);

        return activity->buildMessage(Message_Translations_sourceless_routine_invocation, info);
    }
    else
    {
        ArrayClass *info = new_array(packageName);
        ProtectedObject p(info);

        return activity->buildMessage(Message_Translations_sourceless_program_invocation, info);
    }
}


/**
 * Generate the stack frames for the current context.
 *
 * @return A list of the stackframes.
 */
ArrayClass *RexxActivation::getStackFrames(bool skipFirst)
{
    return activity->generateStackFrames(skipFirst);
}

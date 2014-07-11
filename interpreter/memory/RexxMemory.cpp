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
/* Kernel                                                     RexxMemory.cpp  */
/*                                                                            */
/* Memory Object                                                              */
/*                                                                            */
/******************************************************************************/
#include "RexxCore.h"
#include "RexxMemory.hpp"
#include "MemoryStack.hpp"
#include "StringClass.hpp"
#include "MutableBufferClass.hpp"
#include "DirectoryClass.hpp"
#include "RexxActivity.hpp"
#include "IntegerClass.hpp"
#include "ArrayClass.hpp"
#include "TableClass.hpp"
#include "RexxActivation.hpp"
#include "ActivityManager.hpp"
#include "MessageClass.hpp"
#include "MethodClass.hpp"
#include "RelationClass.hpp"
#include "SupplierClass.hpp"
#include "PointerClass.hpp"
#include "BufferClass.hpp"
#include "PackageClass.hpp"
#include "WeakReferenceClass.hpp"
#include "StackFrameClass.hpp"
#include "Interpreter.hpp"
#include "SystemInterpreter.hpp"
#include "Interpreter.hpp"
#include "PackageManager.hpp"
#include "SysFileSystem.hpp"
#include "UninitDispatcher.hpp"
#include "GlobalProtectedObject.hpp"
#include "MapTable.hpp"

// restore a class from its
// associated primitive behaviour
// (already restored by memory_init)
#define RESTORE_CLASS(name, className) The##name##Class = (className *)RexxBehaviour::getPrimitiveBehaviour(T_##name)->restoreClass();


bool SysAccessPool(MemorySegmentPool **pool);
// NOTE:  There is just a single memory object in global storage.  We'll define
// memobj to be the direct address of this memory object.
MemoryObject memoryObject;

static void logMemoryCheck(FILE *outfile, const char *message, ...)
{
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    if (outfile != NULL) {
        vfprintf(outfile, message, args);
    }
    va_end(args);
}


MemoryObject::MemoryObject()
/******************************************************************************/
/* Function: Main Constructor for Rexxmemory, called once during main         */
/*  initialization.  Will create the initial memory Pool(s), etc.             */
/******************************************************************************/
{
    // we need to set a valid size for this object.  We round it up
    // to the minimum allocation boundary, even though that might be
    // a lie.  Since this never participates in a sweep operation,
    // this works ok in the end.
    setObjectSize(Memory::roundObjectBoundary(sizeof(MemoryObject)));
    // our first pool is the current one
    currentPool = firstPool;

    // OR'ed into object headers to mark during gc
    markWord = 1;
    saveStack = NULL;

    // we always start out with an empty list.  WeakReferences that are in the
    // saved image will (MUST) never be set to a new value, so it's not necessary
    // to hook those back up again.
    weakReferenceList = OREF_NULL;
}


void MemoryObject::initialize(bool _restoringImage)
/******************************************************************************/
/* Function:  Gain access to all Pools                                        */
/******************************************************************************/
{
    /* access 1st pool directly. SysCall */
    /* Did the pool exist?               */

    firstPool = MemorySegmentPool::createPool();
    currentPool = firstPool;

    new (this) MemoryObject;
    new (&newSpaceNormalSegments) NormalSegmentSet(this);
    new (&newSpaceLargeSegments) LargeSegmentSet(this);

    /* and the new/old Space segments    */
    new (&oldSpaceSegments) OldSpaceSegmentSet(this);

    collections = 0;
    allocations = 0;
    variableCache = OREF_NULL;
    globalStrings = OREF_NULL;

    // get our table of virtual functions setup first thing.
    buildVirtualFunctionTable();

    liveStack = (LiveStack *)oldSpaceSegments.allocateObject(MemorySegmentSet::SegmentDeadSpace);
    // remember the original one
    originalLiveStack = liveStack;

    // if we're restoring, load everything from the imae file now.
    if (_restoringImage)
    {
        restoreImage();
    }

    // set the object behaviour
    memoryObject.setBehaviour(TheMemoryBehaviour);

    // make sure we have an inital segment set to allocate from.
    newSpaceNormalSegments.getInitialSet();

    // get the initial uninit table
    uninitTable = new_identity_table();

    // is this image creation?  This will build and save the image, then
    // terminate
    if (!_restoringImage)
    {
        createImage();
    }
    restore();                           // go restore the state of the memory object
}


void MemoryObject::logVerboseOutput(const char *message, void *sub1, void *sub2)
/******************************************************************************/
/* Function:  Log verbose output events                                       */
/******************************************************************************/
{
    logMemoryCheck(NULL, message, sub1, sub2);
}


void MemoryObject::markObjectsMain(RexxObject *rootObject)
/******************************************************************************/
/* Function:  Main memory_mark driving loop                                   */
/******************************************************************************/
{
    // for some of the root objects, we get called to mark them before they get allocated.
    // make sure we don't process any null references.
    if (rootObject == OREF_NULL)
    {
        return;
    }

    RexxObject *markObject;

    // set up the live marking word passed to the live() routines
    size_t liveMark = markWord | OldSpaceBit;

    allocations = 0;
    pushLiveStack(OREF_NULL);            /* push a unique terminator          */
    mark(rootObject);                    /* OREF_ENV or old2new               */
    for (markObject = popLiveStack();
        markObject != OREF_NULL;        /* test for unique terminator        */
        markObject = popLiveStack())
    {
        /* mark behaviour live               */
        memory_mark((RexxObject *)markObject->behaviour);
        /* Mark other referenced obj.  We can do this without checking */
        /* the references flag because we only push the object on to */
        /* the stack if it has references. */
        allocations++;
        markObject->live(liveMark);
    }
}


void MemoryObject::checkUninit()
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
{
    /* we might not actually have a table yet, so make sure we check */
    /* before using it. */
    if (uninitTable == NULL)
    {
        return;
    }

    RexxObject *uninitObject;
    /* table and any object is isn't   */
    /* alive, we indicate it should be */
    /* sent unInit.  We indiacte this  */
    /* by setting the value to 1,      */
    /* instead of NIL (the default)    */
    for (HashLink i = uninitTable->first(); (uninitObject = uninitTable->index(i)) != OREF_NULL; i = uninitTable->next(i))
    {
        /* is this object now dead?        */
        if (uninitObject->isObjectDead(markWord))
        {
            /* yes, indicate object is to be   */
            /*  sent uninit.                   */
            uninitTable->replace(TheTrueObject, i);
            pendingUninits++;
        }
    }
}


/**
 * Force a last-gasp garbage collection and running of the
 * uninits during interpreter instance shutdown.  This is an
 * attempt to ensure that all objects with uninit methods get
 * a chance to clean up prior to termination.
 */
void MemoryObject::collectAndUninit(bool clearStack)
{
    // clear the save stack if we're working with a single instance
    if (clearStack)
    {
        clearSaveStack();
    }
    collect();
    runUninits();
}


/**
 * Force a last-gasp garbage collection and running of the
 * uninits during interpreter instance shutdown.  This is an
 * attempt to ensure that all objects with uninit methods get
 * a chance to clean up prior to termination.
 */
void MemoryObject::lastChanceUninit()
{
    // collect and run any uninits still pending
    collectAndUninit(true);
    // we're about to start releasing libraries, so it is critical
    // we don't run any more uninits after this
    uninitTable->empty();
}


void  MemoryObject::runUninits()
/******************************************************************************/
/* Function:  Run any UNINIT methods for this activity                        */
/******************************************************************************/
/* NOTE: The routine to iterate across uninit Table isn't quite right, since  */
/*  the removal of zombieObj may move another zombieObj and then doing        */
/*  the next will skip this zombie, we should however catch it next time      */
/*  through.                                                                  */
/*                                                                            */
/******************************************************************************/
{
    RexxObject * zombieObj;              /* obj that needs uninit run.        */
    HashLink iterTable;                  /* iterator for table.               */

    /* if we're already processing this, don't try to do this */
    /* recursively. */
    if (processingUninits)
    {
        return;
    }

    /* turn on the recursion flag, and also zero out the count of */
    /* pending uninits to run */
    processingUninits = true;
    pendingUninits = 0;

    // get the current activity for running the uninits
    RexxActivity *activity = ActivityManager::currentActivity;

    /* uninitTabe exists, run UNINIT     */
    for (iterTable = uninitTable->first();
        (zombieObj = uninitTable->index(iterTable)) != OREF_NULL;)
    {
        // TODO:  Ther's a bug here.  Removing the object can cause the
        // iterator to skip over an entry....something should be done to
        // prevent this.

        /* is this object readyfor UNINIT?   */
        if (uninitTable->value(iterTable) == TheTrueObject)
        {
            /* make sure we don't recurse        */
            uninitTable->put(TheFalseObject, zombieObj);
            {
                // run this method with appropriate error trapping
                UninitDispatcher dispatcher(zombieObj);
                activity->run(dispatcher);
            }
                                           /* remove zombie from uninit table   */
            uninitTable->remove(zombieObj);


            // because we just did a remove operation, this will effect the iteration
            // process. There are two possibilities here.  Either A) we were at the end of the
            // chain and this is now an empty slot or B) the removal process moved an new item
            // into this slot.  If it is case A), then we need to search for the next item.  If
            // it is case B) we'll just leave the index alone and process this position again.
            if (uninitTable->index(iterTable) == OREF_NULL)
            {
                iterTable = uninitTable->next(iterTable);
            }
        }
        else
        {
            iterTable = uninitTable->next(iterTable);
        }
    }                                  /* now go check next object in table */
    /* make sure we remove the recursion protection */
    processingUninits = false;
}


void  MemoryObject::removeUninitObject(
    RexxObject *obj)                   /* object to remove                  */
/******************************************************************************/
/* Function:  Remove an object from the uninit tables                         */
/******************************************************************************/
{
    // just remove this object from the table
    uninitTable->remove(obj);
}


void MemoryObject::addUninitObject(
    RexxObject *obj)                   /* object to add                     */
/******************************************************************************/
/* Function:  Add an object with an uninit method to the uninit table for     */
/*            a process                                                       */
/******************************************************************************/
{
                                       /* is object already in table?       */
   if (uninitTable->get(obj) == OREF_NULL)
   {
                                       /* nope, add obj to uninitTable,     */
                                       /*  initial value is NIL             */
       uninitTable->put(TheNilObject, obj);
   }

}

bool MemoryObject::isPendingUninit(RexxObject *obj)
/******************************************************************************/
/* Function:  Test if an object is going to require its uninit method run.    */
/******************************************************************************/
{
    return uninitTable->get(obj) != OREF_NULL;
}


void MemoryObject::markObjects()
/******************************************************************************/
/* Function:   Main mark routine for garbage collection.  This reoutine       */
/*  Determines which mark routine to call and does all additional processing  */
/******************************************************************************/
{
    verboseMessage("Beginning mark operation\n");

    /* call normal,speedy,efficient mark */
    markObjectsMain((RexxObject *)this);
    // now process the weak reference queue...We check this before the
    // uninit list is processed so that the uninit list doesn't mark any of the
    // weakly referenced items.  We don't want an object placed on the uninit queue
    // to end up strongly referenced later.
    checkWeakReferences();

    checkUninit();               /* flag all objects about to be dead */
                                       /* now mark the unInit table and the */
    markObjectsMain(uninitTable);

    // if we had to expand the live stack previously, we allocated a temporary
    // one from malloc() storage rather than the object heap.  We need to
    // explicitly free this version when that happens.
    if (liveStack != originalLiveStack)
    {
        free((void *)liveStack);
        liveStack = originalLiveStack;
    }
    verboseMessage("Mark operation completed\n");
}


/******************************************************************************/
/* Function:  Scan the weak reference queue looking for either dead weak      */
/* objects or weak references that refer to objects that have gone out of     */
/* scope.  Objects with registered notification objects (that are still in    */
/* scope) will be moved to a notification queue, which is processed after     */
/* everything is scanned.                                                     */
/******************************************************************************/
void MemoryObject::checkWeakReferences()
{
    WeakReference *current = weakReferenceList;
    // list of "live" weak references...built while scanning
    WeakReference *newList = OREF_NULL;

    // loop through the list
    while (current != OREF_NULL)
    {
        // we have to save the next one in the list
        WeakReference *next = current->nextReferenceList;
        // this reference still in scope?
        if (current->isObjectLive(markWord))
        {
            // keep this one in the list
            current->nextReferenceList = newList;
            newList = current;
            // have a reference?
            if (current->referentObject != OREF_NULL)
            {
                // if the object is not alive, null out the reference
                if (!current->referentObject->isObjectLive(markWord))
                {
                    current->referentObject = OREF_NULL;
                }
            }
        }
        // step to the new nest item
        current = next;
    }

    // update the list
    weakReferenceList = newList;
}


void MemoryObject::addWeakReference(WeakReference *ref)
/******************************************************************************/
/* Function:  Add a new weak reference to the tracking table                  */
/******************************************************************************/
{
    // just add this to the front of the list
    ref->nextReferenceList = weakReferenceList;
    weakReferenceList = ref;
}


MemorySegment *MemoryObject::newSegment(size_t requestedBytes, size_t minBytes)
/******************************************************************************/
/* Function:  Allocate a segment of the requested size.  The requested size   */
/* is the desired size, while the minimum is the absolute minimum we can      */
/* handle.  This takes care of the overhead accounting and additional         */
/* rounding.  The requested size is assumed to have been rounded up to the    */
/* next "appropriate" boundary already, and the segment overhead will be      */
/* allocated from that part, if possible.  Otherwise, and additional page is  */
/* added.                                                                     */
/******************************************************************************/
{
    MemorySegment *segment;

#ifdef MEMPROFILE
    printf("Allocating a new segment of %d bytes\n", requestedBytes);
#endif
    /* first make sure we've got enough space for the control */
    /* information, and round this to a proper boundary */
    requestedBytes = MemorySegment::roundSegmentBoundary(requestedBytes + MemorySegment::MemorySegmentOverhead);
#ifdef MEMPROFILE
    printf("Allocating boundary a new segment of %d bytes\n", requestedBytes);
#endif
    /*Get a new segment                  */
    segment = currentPool->newSegment(requestedBytes);
    /* Did we get a segment              */
    if (segment == NULL)
    {
        /* Segmentsize is the minimum size request we handle.  If */
        /* minbytes is small, then we're just adding a segment to the */
        /* small pool.  Reduce the request to SegmentSize and try again. */
        /* For all other requests, try once more with the minimum. */
        minBytes = MemorySegment::roundSegmentBoundary(minBytes + MemorySegment::MemorySegmentOverhead);
        /* try to allocate once more...if this fails, the caller will */
        /* have to handle it. */
        segment = currentPool->newSegment(minBytes);
    }
    return segment;                      /* return the allocated segment      */
}


MemorySegment *MemoryObject::newLargeSegment(size_t requestedBytes, size_t minBytes)
/******************************************************************************/
/* Function:  Allocate a segment of the requested size.  The requested size   */
/* is the desired size, while the minimum is the absolute minimum we can      */
/* handle.  This takes care of the overhead accounting and additional         */
/* rounding.  The requested size is assumed to have been rounded up to the    */
/* next "appropriate" boundary already, and the segment overhead will be      */
/* allocated from that part, if possible.  Otherwise, and additional page is  */
/* added.                                                                     */
/******************************************************************************/
{
    MemorySegment *segment;

    /* first make sure we've got enough space for the control */
    /* information, and round this to a proper boundary */
    size_t allocationBytes = MemorySegment::roundSegmentBoundary(requestedBytes + MemorySegment::MemorySegmentOverhead);
#ifdef MEMPROFILE
    printf("Allocating large boundary new segment of %d bytes for request of %d\n", allocationBytes, requestedBytes);
#endif
    /*Get a new segment                  */
    segment = currentPool->newLargeSegment(allocationBytes);
    /* Did we get a segment              */
    if (segment == NULL)
    {
        /* Segmentsize is the minimum size request we handle.  If */
        /* minbytes is small, then we're just adding a segment to the */
        /* small pool.  Reduce the request to SegmentSize and try again. */
        /* For all other requests, try once more with the minimum. */
        // TODO: this operation is done a lot...consider adding a method to MemorySegment
        minBytes = MemorySegment::roundSegmentBoundary(minBytes + MemorySegment::MemorySegmentOverhead);
        /* try to allocate once more...if this fails, the caller will */
        /* have to handle it. */
        segment = currentPool->newLargeSegment(minBytes);
    }
    return segment;                      /* return the allocated segment      */
}


void MemoryObject::restoreImage()
/******************************************************************************/
/* Function:  Restore a saved image to usefulness.                            */
/******************************************************************************/
{
    // Nothing to restore if we have a buffer already
    if (restoredImage != NULL)
    {
        return;
    }

    size_t imageSize;

    // load the image file
    SystemInterpreter::loadImage(&restoredImage, &imageSize);
    // we write a size to the start of the image when the image is created.
    // the restoredImage buffer does not include that image size, so we
    // need to pretend the buffer is slightly before the start.
    // image data is just past that information.
    char *relocation = restoredImage - sizeof(size_t);

    // create a handler for fixing up reference addresses.
    ImageRestoreMarkHandler markHandler(relocation);
    setMarkHandler(&markHandler);

    // address the start and end of the image.
    RexxObject *objectPointer = (RexxObject *)restoredImage;
    RexxObject *endPointer = (RexxObject *)(restoredImage + imageSize);

    // the save array is the 1st object in the buffer
    RexxArray *saveArray = (RexxArray *)objectPointer;

    // now loop through the image buffer fixing up the objects.
    while (objectPointer < endPointer)
    {
        size_t primitiveTypeNum;

        // Fixup the Behaviour pointer for the object.

        // If this is a primitive object, we can just pick up the hard coded
        // behaviour.
        if (objectPointer->isNonPrimitive())
        {
            // Working with a copy, so don't use the static table version.
            // the behaviour will have been packed into the image, so we need to
            // pick up the reference.
            RexxBehaviour *imageBehav = (RexxBehaviour *)(relocation + (uintptr_t)objectPointer->behaviour);
            // and rewrite the offset version with the resolved pointer
            objectPointer->behaviour = imageBehav;
            // get the type number from the behaviour.  This tells us what
            // virtual function pointer to use.
            primitiveTypeNum = imageBehav->getClassType();
        }
        else
        {
            // the original behaviour pointer has been encoded as a type number and class
            // category to allow us to convert back to the appropriate type.
            objectPointer->behaviour = RexxBehaviour::restoreSavedPrimitiveBehaviour(objectPointer->behaviour);
            primitiveTypeNum = objectPointer->behaviour->getClassType();
        }
        // This will be an OldSpace object.  We delay setting this
        // until now, because the oldspace bit is overloaded with the
        // NonPrimitive bit.  Since the we are done with the
        // non-primitive bit now, we can use this for the oldspace
        // flag too.
        objectPointer->setOldSpace();
        // now fix up the virtual function table for the object using the type number
        objectPointer->setVirtualFunctions(virtualFunctionTable[primitiveTypeNum]);

        // if the object has references, call liveGeneral() to cause these references to
        // be converted back into real pointers
        if (objectPointer->hasReferences())
        {
            objectPointer->liveGeneral(RESTORINGIMAGE);
        }
        // advance to the next object in the image
        objectPointer = objectPointer->nextObject();
    }

    // now start restoring the critical objects from the save array
    TheEnvironment = (RexxDirectory *)saveArray->get(saveArray_ENV);
    // restore all of the primitive behaviour data...right now, these are
    // all default values.
    RexxArray *primitiveBehaviours = (RexxArray *)saveArray->get(saveArray_PBEHAV);
    for (size_t i = 0; i <= T_Last_Exported_Class; i++)
    {
        /* behaviours into this array        */
        RexxBehaviour::primitiveBehaviours[i].restore((RexxBehaviour *)primitiveBehaviours->get(i + 1));
    }

    TheKernel      = (RexxDirectory *)saveArray->get(saveArray_KERNEL);
    TheSystem      = (RexxDirectory *)saveArray->get(saveArray_SYSTEM);
    TheFunctionsDirectory = (RexxDirectory *)saveArray->get(saveArray_FUNCTIONS);
    TheTrueObject  = (RexxInteger *)saveArray->get(saveArray_TRUE);
    TheFalseObject = (RexxInteger *)saveArray->get(saveArray_FALSE);
    TheNilObject   = saveArray->get(saveArray_NIL);
    TheNullArray   = (RexxArray *)saveArray->get(saveArray_NULLA);
    TheNullPointer   = (RexxPointer *)saveArray->get(saveArray_NULLPOINTER);
    TheClassClass  = (RexxClass *)saveArray->get(saveArray_CLASS);
    TheCommonRetrievers = (RexxDirectory *)saveArray->get(saveArray_COMMON_RETRIEVERS);

    // restore the global strings
    memoryObject.restoreStrings((RexxArray *)saveArray->get(saveArray_NAME_STRINGS));
    // make sure we have a working thread context
    RexxActivity::initializeThreadContext();
    PackageManager::restore((RexxArray *)saveArray->get(saveArray_PACKAGES));
}


void MemoryObject::live(size_t liveMark)
/******************************************************************************/
/* Arguments:  None                                                           */
/*                                                                            */
/*  Returned:  Nothing                                                        */
/*                                                                            */
/******************************************************************************/
{
    // Mark the save stack first, since it will be pulled off of
    // the stack after everything else.  This will give other
    // objects a chance to be marked before we remove them from
    // the savestack.
    memory_mark(saveStack);
    memory_mark(old2new);
    memory_mark(variableCache);
    memory_mark(globalStrings);
    memory_mark(environment);
    memory_mark(commonRetrievers);
    memory_mark(kernel);
    memory_mark(system);

    // now call the various subsystem managers to mark their references
    Interpreter::live(liveMark);
    SystemInterpreter::live(liveMark);
    ActivityManager::live(liveMark);
    PackageManager::live(liveMark);
    // mark any protected objects we've been watching over

    GlobalProtectedObject *p = protectedObjects;
    while (p != NULL)
    {
        memory_mark(p->protectedObject);
        p = p->next;
    }
}

void MemoryObject::liveGeneral(MarkReason reason)
/******************************************************************************/
/* Arguments:  None                                                           */
/*                                                                            */
/*  Returned:  Nothing                                                        */
/*                                                                            */
/******************************************************************************/
{
    memory_mark_general(saveStack);/* Mark the save stack last, to give it a chance to clear out entries */
    memory_mark_general(old2new);
    memory_mark_general(variableCache);
    memory_mark_general(globalStrings);
    memory_mark_general(environment);
    memory_mark_general(commonRetrievers);
    memory_mark_general(kernel);
    memory_mark_general(system);

    // now call the various subsystem managers to mark their references
    Interpreter::liveGeneral(reason);
    SystemInterpreter::liveGeneral(reason);
    ActivityManager::liveGeneral(reason);
    PackageManager::liveGeneral(reason);
    // mark any protected objects we've been watching over

    GlobalProtectedObject *p = protectedObjects;
    while (p != NULL)
    {
        memory_mark_general(p->protectedObject);
        p = p->next;
    }
}

void MemoryObject::collect()
/******************************************************************************/
/* Function:  Collect all dead memory in the Rexx object space.  The          */
/* collection process performs a mark operation to mark all of the live       */
/* objects, followed by sweep of each of the segment sets.                    */
/******************************************************************************/
{
    collections++;
    verboseMessage("Begin collecting memory, cycle #%d after %d allocations.\n", collections, allocations);
    allocations = 0;

    /* change our marker to the next value so we can distinguish */
    /* between objects marked on this cycle from the objects marked */
    /* in the pervious cycles. */
    bumpMarkWord();

    /* do the object marking now...followed by a sweep of all of the */
    /* segments. */
    markObjects();
    newSpaceNormalSegments.sweep();
    newSpaceLargeSegments.sweep();

    /* The space segments are now in a known, completely clean state. */
    /* Now based on the context that caused garbage collection to be */
    /* initiated, the segment sets may be expanded to add additional */
    /* free memory.  The decision to expand the object space requires */
    /* the usage statistics collected by the mark-and-sweep */
    /* operation. */

    verboseMessage("End collecting memory\n");
}

RexxObject *MemoryObject::oldObject(size_t requestLength)
/******************************************************************************/
/* Arguments:  Requested length                                               */
/*                                                                            */
/*  Returned:  New object, or OREF_NULL if requested length was too large     */
/******************************************************************************/
{
    /* Compute size of new object and determine where we should */
    /* allocate from */
    requestLength = Memory::roundObjectBoundary(requestLength);
    RexxObject *newObj = oldSpaceSegments.allocateObject(requestLength);

    /* if we got a new object, then perform the final setup steps. */
    /* Since the oldspace objects are special, we don't push them on */
    /* to the save stack.  Also, we don't set the oldspace flag, as */
    /* those are a separate category of object. */
    if (newObj != OREF_NULL)
    {
        // initialize the hash table object
        newObj->initializeNewObject(requestLength, markWord, virtualFunctionTable[T_Object], TheObjectBehaviour);
    }

    /* return the newly allocated object to our caller */
    return newObj;
}

char *MemoryObject::allocateImageBuffer(size_t imageSize)
/******************************************************************************/
/* Function:  Allocate an image buffer for the system code.  The image buffer */
/* is allocated in the oldspace segment set.  We create an object from that   */
/* space, then return this object as a character pointer.  We eventually will */
/* get that pointer passed back to us as the image address.                   */
/******************************************************************************/
{
    return (char *)oldObject(imageSize);
}


RexxObject *MemoryObject::newObject(size_t requestLength, size_t type)
/******************************************************************************/
/* Arguments:  Requested length                                               */
/*                                                                            */
/*  Returned:  New object, or OREF_NULL if requested length was too large     */
/******************************************************************************/
{
    RexxObject *newObj;

    allocations++;

    /* Compute size of new object and determine where we should */
    /* allocate from */
    requestLength = Memory::roundObjectBoundary(requestLength);

    /* is this a typical small object? */
    if (requestLength <= MemorySegment::LargeBlockThreshold)
    {
        /* make sure we don't go below our minimum size. */
        if (requestLength < Memory::MinimumObjectSize)
        {
            requestLength = Memory::MinimumObjectSize;
        }
        newObj = newSpaceNormalSegments.allocateObject(requestLength);
        /* feat. 1061 moves the handleAllocationFailure code into the initial */
        /* allocation parts; this way an "if" instruction can be saved.       */
        if (newObj == NULL)
        {
            newObj = newSpaceNormalSegments.handleAllocationFailure(requestLength);
        }
    }
    /* we keep the big objects in a separate cage */
    else
    {
        /* round this allocation up to the appropriate large boundary */
        requestLength = Memory::roundLargeObjectAllocation(requestLength);
        newObj = newSpaceLargeSegments.allocateObject(requestLength);
        if (newObj == NULL)
        {
            newObj = newSpaceLargeSegments.handleAllocationFailure(requestLength);
        }
    }

    newObj->initializeNewObject(markWord, virtualFunctionTable[type], RexxBehaviour::getPrimitiveBehaviour(type));

    if (saveStack != OREF_NULL)
    {
        // saveobj doesn't get turned on until the system is initialized
        //far enough but once its on, push this new obj on the save stack to
        //keep it from being garbage collected before it can be used
        //and safely anchored by caller.
        pushSaveStack(newObj);
    }
    // return the newly allocated object to our caller
    return newObj;
}


RexxArray  *MemoryObject::newObjects(
                size_t         size,
                size_t         count,
                size_t         objectType)
/******************************************************************************/
/* Arguments:  size of individual objects,                                    */
/*             number of objects to get                                       */
/*             behaviour of the new objects.                                  */
/*                                                                            */
/* Function : Return an  Array of count objects of size size, with the given  */
/*             behaviour.  Each objects will be cleared                       */
/*  Returned: Array object                                                    */
/*                                                                            */
/******************************************************************************/
{
    size_t i;
    size_t objSize = Memory::roundObjectBoundary(size);
    size_t totalSize;                      /* total size allocated              */
    RexxObject *prototype;                 /* our first prototype object        */

    RexxArray  *arrayOfObjects;
    RexxObject *largeObject;

    /* Get array object to contain all the objects.. */
    arrayOfObjects = (RexxArray *)new_array(count);

    /* Get one LARGE object, that we will parcel up into the smaller */
    /* objects over allocate by the size of one minimum object so we */
    /* can handle any potential overallocations */
    totalSize = objSize * count;         /* first get the total object size   */
    /* We make the decision on which heap this should be allocated */
    /* from based on the size of the object request rather than the */
    /* total size of the aggregate object.  Since our normal usage of */
    /* this is for allocating large collections of small objects, we */
    /* don't want those objects coming from the large block heap, */
    /* even if the aggregate size would suggest this should happen. */
    if (objSize <= MemorySegment::LargeBlockThreshold)
    {
        largeObject = newSpaceNormalSegments.allocateObject(totalSize);
        if (largeObject == OREF_NULL)
        {
            largeObject = newSpaceNormalSegments.handleAllocationFailure(totalSize);
        }
    }
    /* we keep the big objects in a separate cage */
    else
    {
        largeObject = newSpaceLargeSegments.allocateObject(totalSize);
        if (largeObject == OREF_NULL)
        {
            largeObject = newSpaceLargeSegments.handleAllocationFailure(totalSize);
        }
    }

    largeObject->initializeNewObject(markWord, virtualFunctionTable[T_Object], TheObjectBehaviour);

    if (saveStack != OREF_NULL)
    {
        /* saveobj doesn't get turned on     */
        /*until the system is initialized    */
        /*far enough but once its on, push   */
        /*this new obj on the save stack to  */
        /*keep him from being garbage        */
        /*collected, before it can be used   */
        /*and safely anchored by caller.     */
        pushSaveStack(largeObject);
    }

    /* Description of defect 318:  IH:

    The problem is caused by the constructor of RexxClause which is calling newObjects.
    NewObjects allocates one large Object. Immediately after this large Object is allocated,
    an array is allocated as well. It then can happen that while allocating the array
    the largeObject shall be marked. This causes a trap when objectVariables is != NULL.

    Solution: Set objectVariables to NULL before allocating the array. In order to make
    OrefOK (called if CHECKOREF is defined) work properly, the largeObject has to be
    set to a valid object state before calling new_array. Therefore the behaviour assignement
    and the virtual functions assignement has to be done in advance. */

    /* get the remainder object size...this is used to manage the */
    /* dangling piece on the end of the allocation. */
    totalSize = largeObject->getObjectSize() - totalSize;

    /* Clear out the entire object... */
    largeObject->clearObject();

    prototype = largeObject;

    /* IH: Object gets a valid state for the mark and sweep process. */
    /* Otherwise OrefOK (CHECKOREFS) will fail */

    // initialize the hash table object
    largeObject->initializeNewObject(objSize, markWord, virtualFunctionTable[objectType], RexxBehaviour::getPrimitiveBehaviour(objectType));

    for (i=1 ;i < count ; i++ )
    {
        /* IH: Loop one time less than before because first object is initialized
           outside of the loop. I had to move the following 2 statements
           in front of the object initialization */
        /* add this object to the array of objs */
        arrayOfObjects->put(largeObject, i);
        /* point to the next object space. */
        largeObject = (RexxObject *)((char *)largeObject + objSize);
        /* copy the information from the prototype */
        memcpy((void *)largeObject, (void *)prototype, sizeof(RexxInternalObject));
    }
    arrayOfObjects->put(largeObject, i);  /* put the last Object */

    /* adjust the size of the last one to account for any remainder */
    largeObject->setObjectSize(totalSize + objSize);

    return arrayOfObjects;               /* Return our array of objects.      */
}


void MemoryObject::reSize(RexxObject *shrinkObj, size_t requestSize)
/******************************************************************************/
/* Function:  The object shrinkObj only needs to be the size of newSize       */
/*             If the left over space is big enough to become a dead object   */
/*             we will shrink the object to the specified size.               */
/*            NOTE: Since memory knows nothing about any objects that are     */
/*             in the extra space, it cannot do anything about them if this   */
/*             is an OldSpace objetc, therefore the caller must have already  */
/*             take care of this.                                             */
/*                                                                            */
/******************************************************************************/
{
    DeadObject *newDeadObj;

    size_t newSize = Memory::roundObjectResize(requestSize);

    /* is the rounded size smaller and is remainder at least the size */
    /* of the smallest OBJ MINOBJSIZE */
    if (newSize < requestSize && (shrinkObj->getObjectSize() - newSize) >= Memory::MinimumObjectSize)
    {
        size_t deadObjectSize = shrinkObj->getObjectSize() - newSize;
        /* Yes, then we can shrink the object.  Get starting point of */
        /* the extra, this will be the new Dead obj */
        newDeadObj = new ((void *)((char *)shrinkObj + newSize)) DeadObject (deadObjectSize);
        /* if an object is larger than 16 MB, the last 8 bits (256) are */
        /* truncated and therefore the object must have a size */
        /* dividable by 256 and the rest must be put to the dead chain. */
        /* If the resulting dead object is smaller than the size we */
        /* gave, then we've got a truncated remainder we need to turn */
        /* into a dead object. */
        deadObjectSize -= newDeadObj->getObjectSize();
        if (deadObjectSize != 0)
        {
            /* create difference object.  Note:  We don't bother */
            /* putting this back on the dead chain.  It will be */
            /* picked up during the next GC cycle. */
            new ((char *)newDeadObj + newDeadObj->getObjectSize()) DeadObject (deadObjectSize);
        }
        /* Adjust size of original object */
        shrinkObj->setObjectSize(newSize);
    }
}


void MemoryObject::scavengeSegmentSets(
    MemorySegmentSet *requestor,           /* the requesting segment set */
    size_t allocationLength)               /* the size required          */
/******************************************************************************/
/* Function:  Orchestrate sharing of sharing of storage between the segment   */
/* sets in low storage conditions.  We do this only as a last ditch, as we'll */
/* end up fragmenting the large heap with small blocks, messing up the        */
/* benefits of keeping the heaps separate.  We first look a set to donate a   */
/* segment to the requesting set.  If that doesn't work, we'll borrow a block */
/* of storage from the other set so that we can satisfy this request.         */
/******************************************************************************/
{
    MemorySegmentSet *donor;

    /* first determine the donor/requester relationships. */
    if (requestor->is(MemorySegmentSet::SET_NORMAL))
    {
        donor = &newSpaceLargeSegments;
    }
    else
    {
        donor = &newSpaceNormalSegments;
    }

    /* first look for an unused segment.  We might be able to steal */
    /* one and just move it over. */
    MemorySegment *newSeg = donor->donateSegment(allocationLength);
    if (newSeg != NULL)
    {
        requestor->addSegment(newSeg);
        return;
    }

    /* we can't just move a segment over.  Find the smallest block */
    /* we can find that will satisfy this allocation.  If found, we */
    /* can insert it into the normal deadchains. */
    DeadObject *largeObject = donor->donateObject(allocationLength);
    if (largeObject != NULL)
    {
        /* we need to insert this into the normal dead chain */
        /* locations. */
        requestor->addDeadObject(largeObject);
    }
}


void MemoryObject::liveStackFull()
/******************************************************************************/
/* Function:  Process a live-stack overflow situation                         */
/******************************************************************************/
{
    // create a new stack that is double in size
    LiveStack *newLiveStack = liveStack->reallocate(2);

    /* has this already been expanded?   */
    // TODO:  Why is this calling free?
    if (liveStack != originalLiveStack)
    {
        free((void *)liveStack);
    }
    // we can set the new stack
    liveStack = newLiveStack;
}

void MemoryObject::mark(RexxObject *markObject)
/******************************************************************************/
/* Function:  Perform a memory management mark operation                      */
/******************************************************************************/
{
    size_t liveMark = markWord | OldSpaceBit;

    markObject->setObjectLive(markWord); /* Then Mark this object as live.    */
                                         /* object have any references?       */
                                         /* if there are no references, we don't */
                                         /* need to push this on the stack, but */
                                         /* we might need to push the behavior */
                                         /* on the stack.  Since behaviors are */
                                         /* we can frequently skip that step as well */
    if (markObject->hasNoReferences())
    {
        if (ObjectNeedsMarking(markObject->behaviour))
        {
            /* mark the behaviour now to keep us from processing this */
            /* more than once. */
            markObject->behaviour->setObjectLive(markWord);
            /* push him to livestack, to mark    */
            /* later.                            */
            pushLiveStack((RexxObject *)markObject->behaviour);
        }
    }
    else
    {
        /* push him to livestack, to mark    */
        /* later.                            */
        pushLiveStack(markObject);
    }
}

RexxObject *MemoryObject::temporaryObject(size_t requestLength)
/******************************************************************************/
/* Function:  Allocate and setup a temporary object obtained via malloc       */
/*            storage.  This is used currently only by the mark routine to    */
/*            expand the size of the live stack during a garbage collection.  */
/******************************************************************************/
{
    /* get the rounded size of the object*/
    size_t allocationLength = Memory::roundObjectBoundary(requestLength);
    /* allocate a new object             */
    RexxObject *newObj = (RexxObject *)malloc(allocationLength);
    if (newObj == OREF_NULL)             /* unable to allocate a new one?     */
    {
        /* can't allocate, report resource   */
        /* error.                            */
        reportException(Error_System_resources);
    }
    /* setup the new object header for   */
    /*use                                */
    // initialize the hash table object
    newObj->initializeNewObject(allocationLength, markWord, virtualFunctionTable[T_Object], TheObjectBehaviour);
    return newObj;                       /* and return it                     */
}


/**
 * When we are doing a general marking operation, all
 * objects call markGeneral for all of its object references.
 * We perform different functions based on what the
 * current marking reason is.
 *
 * @param obj    A pointer to the field being marked.  This is
 *               passed as a void * because there are lots of
 *               issues with passing this as a object type.  We
 *               just fake things out and avoid the compile
 *               problems this way.
 *
 */
void MemoryObject::markGeneral(void *obj)
{
    // OK, convert this to a pointer to an Object field, then
    // get the object reference stored there.
    RexxObject **pMarkObject = (RexxObject **)obj;
    RexxObject *markObject = *pMarkObject;

    // NULL references require no processing for any operation.
    if (markObject == OREF_NULL)
    {
        return;
    }

    // have our current marking handler take care of this
    currentMarkHandler->mark(pMarkObject, markObject);
}


RexxObject *MemoryObject::holdObject(RexxInternalObject *obj)
/******************************************************************************/
/* Function:  Place an object on the hold stack                               */
/******************************************************************************/
{
   /* Push object onto saveStack */
   saveStack->push((RexxObject *)obj);
   /* return held  object               */
   return (RexxObject *)obj;
}


void MemoryObject::saveImage(void)
/******************************************************************************/
/* Function:  Save the memory image as part of interpreter build              */
/******************************************************************************/
{
    MemoryStats _imageStats;

    imageStats = &_imageStats;     // set the pointer to the current collector
    _imageStats.clear();

    // this has been protecting every thing critical
    // from GC events thus far, but now we remove it because
    // it contains things we don't want to save in the image.
    TheEnvironment->remove(getGlobalName(CHAR_KERNEL));

    // get an array to hold all special objects.  This will be the first object
    // copied into the image buffer and allows us to recover all of the important
    // image objects at restore time.
    RexxArray *saveArray = new_array(saveArray_highest);
    // Note:  A this point, we don't have an activity we can use ProtectedObject to save
    // this with, so we need to use GlobalProtectedObject();
    GlobalProtectedObject p(saveArray);

    // Add all elements needed in
    saveArray->put((RexxObject *)TheEnvironment,   saveArray_ENV);
    saveArray->put((RexxObject *)TheKernel,        saveArray_KERNEL);
    saveArray->put((RexxObject *)TheTrueObject,    saveArray_TRUE);
    saveArray->put((RexxObject *)TheFalseObject,   saveArray_FALSE);
    saveArray->put((RexxObject *)TheNilObject,     saveArray_NIL);
    saveArray->put((RexxObject *)TheNullArray,     saveArray_NULLA);
    saveArray->put((RexxObject *)TheNullPointer,   saveArray_NULLPOINTER);
    saveArray->put((RexxObject *)TheClassClass,    saveArray_CLASS);
    saveArray->put((RexxObject *)PackageManager::getImageData(), saveArray_PACKAGES);
    saveArray->put((RexxObject *)TheSystem,       saveArray_SYSTEM);
    saveArray->put((RexxObject *)TheFunctionsDirectory,  saveArray_FUNCTIONS);
    saveArray->put((RexxObject *)TheCommonRetrievers,    saveArray_COMMON_RETRIEVERS);
    saveArray->put((RexxObject *)saveStrings(), saveArray_NAME_STRINGS);

    // create an array for all of the primitive behaviours and fill it with
    // pointers to our primitive behaviours (only the exported ones need this).
    RexxArray *primitive_behaviours= (RexxArray *)new_array(T_Last_Exported_Class + 1);
    for (size_t i = 0; i <= T_Last_Exported_Class; i++)
    {
        primitive_behaviours->put((RexxObject *)RexxBehaviour::getPrimitiveBehaviour(i), i + 1);
    }

    // add these to the save array
    saveArray->put(primitive_behaviours, saveArray_PBEHAV);

    // this is make sure we're getting the new set
    bumpMarkWord();

    // create a generic mark handler for this.  We really just
    // want to trace all of the live objects alerting them to the pending
    // image save.
    TracingMarkHandler markHandler(this, markWord);
    setMarkHandler(&markHandler);

    // go to any pre-save pruning/replacement needed by image objects first
    tracingMark(saveArray, PREPARINGIMAGE);

    // reset the marking handler
    resetMarkHandler();

    // now allocate an image buffer and flatten everything hung off of the
    // save array into it.
    char *imageBuffer = (char *)malloc(Memory::MaxImageSize);
    // we save the size of this image at the beginning, so we start
    // the flattening process after that size location.
    size_t imageOffset = sizeof(size_t);
    // bump the mark word to ensure we're going to hit everthing
    bumpMarkWord();

    ImageSaveMarkHandler saveHandler(this, markWord, imageBuffer, imageOffset);
    setMarkHandler(&saveHandler);

    // push a marker on the stack and start the process from the root
    // save object.  This will copy this object to the start of the image
    // buffer so we know where to find it at restore time.
    pushLiveStack(OREF_NULL);

    // this starts the process by marking everything in the save array.
    memory_mark_general(saveArray);

    // now keep popping objects from the stack until we hit the null terminator.
    for (RexxObject *markObject = popLiveStack(); markObject != OREF_NULL; markObject = popLiveStack())
    {
        // The mark of this object moved it to the image buffer.  Its behaviour
        // no contains its offset in the image.  We don't want to mark the original
        // object, but rather the save image copy.
        RexxObject *copyObject = (RexxObject *)(imageBuffer + (uintptr_t)markObject->behaviour);

        // mark any other referenced objects in the copy.
        copyObject->liveGeneral(SAVINGIMAGE);
        // if this is a non-primitive behaviour, we need to mark that also
        // so that it is copied into the buffer.  The primitive behaviours have already
        // been handled as part of the savearray.
        if (copyObject->isNonPrimitive())
        {
            memory_mark_general(copyObject->behaviour);
        }
    }

    resetMarkHandler();

    FILE *image = fopen(BASEIMAGE,"wb");
    // place the real size at the beginning of the buffer
    memcpy(imageBuffer, &saveHandler.imageOffset, sizeof(size_t));
    // and finally write this entire image out.
    fwrite(imageBuffer, 1, saveHandler.imageOffset, image);
    fclose(image);
    free(imageBuffer);

#ifdef MEMPROFILE
    printf("Object stats for this image save are \n");
    _imageStats.printSavedImageStats();
    printf("\n\n Total bytes for this image %lu bytes \n", saveHandler.imageOffset);
#endif
}


/**
 * Perform a general marking operation with a specified
 * reason.  This performs no real operation, but touches
 * all of the objects and traverses the live object tree.
 * Usually used during image save preparation.
 *
 * @param root   The root object to mark from.
 * @param reason The marking reason.
 */
void MemoryObject::tracingMark(RexxObject *root, MarkReason reason)
{
    // push a unique terminator
    pushLiveStack(OREF_NULL);
    // push the live root, and process until we run out of stacked objects.
    memory_mark_general(root);

    for (RexxObject *markObject = popLiveStack();
        markObject != OREF_NULL;        /*   test for unique terminator      */
        markObject = popLiveStack())
    {
        // mark the behaviour first
        memory_mark_general(markObject->behaviour);
        // just call the liveGeneral method on the popped object
        markObject->liveGeneral(reason);
    }
}


/**
 * Perform an in-place unflatten operation on an object
 * in a buffer.
 *
 * @param sourceBuffer
 *                   The source buffer that contains this data (will need fixups at the end)
 * @param startPointer
 *                   The starting data location in the buffer.
 * @param dataLength The length of the data to unflatten
 *
 * @return The first "real" object in the buffer.
 */
RexxObject *MemoryObject::unflattenObjectBuffer(RexxBuffer *sourceBuffer, char *startPointer, size_t dataLength)
{
    // get an end pointer
    RexxObject *endPointer = (RexxObject *)(startPointer + dataLength);

    // create the handler that will process the markGeneral calls.
    UnflatteningMarkHandler markHandler(startPointer, markWord);
    setMarkHandler(&markHandler);

    // pointer for addressing a location as an object.  This will also
    // give use the last object we've processed at the end.
    RexxObject *puffObject = (RexxObject *)startPointer;

    // now traverse the buffer fixing all of the behaviour pointers and having the object
    // mark and fix up their references.
    while (puffObject < endPointer)
    {
        size_t primitiveTypeNum = 0;

        // a non-primitive behaviour.  Then are flattened with the referencing objects.
        if (puffObject->isNonPrimitive())
        {
            // Yes, lets get the behaviour Object...this is stored as an offset at this
            // point that we can turn into a real reference.
            RexxBehaviour *objBehav = (RexxBehaviour *)(startPointer + ((uintptr_t)puffObject->behaviour));
            // Resolve the static behaviour info
            objBehav->resolveNonPrimitiveBehaviour();
            // Set this object's behaviour to a real object.
            puffObject->behaviour = objBehav;
            // get the behaviour's type number
            primitiveTypeNum = objBehav->getClassType();
        }
        else
        {
            // convert this from a type number to the actual class.  This will unnormalize the
            // type number to the different object classes.
            puffObject->behaviour = RexxBehaviour::restoreSavedPrimitiveBehaviour(puffObject->behaviour);
            primitiveTypeNum = puffObject->behaviour->getClassType();
        }

        // Force fix-up of VirtualFunctionTable,
        puffObject->setVirtualFunctions(MemoryObject::virtualFunctionTable[primitiveTypeNum]);
        // mark this object as live
        puffObject->setObjectLive(memoryObject.markWord);
        // Mark other referenced objs
        // Note that this flavor of mark_general should update the
        // mark fields in the objects.
        puffObject->liveGeneral(UNFLATTENINGOBJECT);
        // Point to next object in image.
        puffObject = puffObject->nextObject();
    }

    // reset the mark handler to the default
    resetMarkHandler();

    // Prepare to reveal the objects in  the buffer.
    // the first object in the buffer is a dummy added
    // for padding.  We need to step past that one to the
    // beginning of the real unflattened objects
    RexxObject *firstObject = ((RexxObject *)startPointer)->nextObject();

    // this is the location of the next object after the buffer
    char *nextObject = (char *)sourceBuffer->nextObject();
    // this is the size of any tailing buffer portion after the last unflattened object.
    size_t tailSize = nextObject - (char *)endPointer;

    // puffObject is the last object we processed.  Add any tail data size on to that object
    // so we don't create an invalid gap in the heap.
    puffObject->setObjectSize(puffObject->getObjectSize() + tailSize);
    // now adjust the front portion of the buffer object to reveal all of the
    // unflattened data.  There is a dummy object at the front of the buffer...we want to
    // step to the the first real object
    sourceBuffer->setObjectSize((char *)firstObject - (char *)sourceBuffer);
    // and return the first object
    return firstObject;
}


/**
 * Run the list of objects in an unflattened buffer calling
 * the unflatten() method to perform any proxy/collection
 * table processing.
 *
 * @param envelope  The envelope for the unflatten operation.
 * @param firstObject
 *                  The first object of the buffer.
 * @param endObject The end location for the buffer (actually the first object past the end of the buffer).
 */
void MemoryObject::unflattenProxyObjects(RexxEnvelope *envelope, RexxObject *firstObject, RexxObject *endObject)
{
    // switch to an unflattening mark handler.
    EnvelopeMarkHandler markHandler(envelope);
    setMarkHandler(&markHandler);

    // Now traverse the buffer running any proxies.
    while (firstObject < endObject)
    {
        // Since a GC could happen at anytime we need to check to make sure the object
        //  we are going now unflatten is still alive, since all who reference it may have already
        //  run and gotten the info from it and no longer reference it.

        // In theory, this should not be an issue because all of the objects were
        // in the protected set, but unflatten might have cast off some references.
        if (firstObject->isObjectLive(memoryObject.markWord))
        {
            // Note that this flavor of  liveGeneral will run any proxies
            // created by unflatten and fixup  the refs to them.
            firstObject->liveGeneral(UNFLATTENINGOBJECT);
        }

        // Point to next object in image.
        firstObject = firstObject->nextObject();
    }

    // and switch back the mark handler
    resetMarkHandler();
}


/**
 * Update a reference to an object when the target
 * object is a member of the OldSpace.
 *
 * @param oldValue The old field needing updating.
 * @param value    The new value being assigned.
 *
 * @return The assigned object value.
 */
void MemoryObject::setOref(RexxInternalObject *oldValue, RexxInternalObject *value)
{
    // if there is no old2new table, we're doing an image build.  No tracking
    // required then.
    if (old2new != OREF_NULL)
    {
        // the index value is the one assigned there currently.  If this
        // is a newspace value, we should have a table entry with a reference
        // count for it in our table.
        if (oldValue != OREF_NULL && oldValue->isNewSpace())
        {
            // get the old reference count, which
            // *should* be non-zero
            size_t refcount = old2new->get(oldValue);
            if (refcount != 0)
            {
                // TODO:  We can optimize this with a special method

                // decrement the value.  If the new value
                // is zero, then we remove the reference object.
                refcount--;
                if (refcount == 0)
                {
                    old2new->remove(oldValue);
                }
                // update with the new count
                else
                {
                    old2new->put(refcount, oldValue);
                }
            }
            else
            {
                /* naughty, naughty, someone didn't use SetOref */
                printf("******** error in memory_setoref, unable to decrement refcount\n");
                printf("Naughty object reference is at:  %p\n", oldValue);
                printf("Naughty object reference type is:  %lu\n", (oldValue)->behaviour->getClassType());
            }
        }
        // now we have to do this for the new value.
        if (value != OREF_NULL && value->isNewSpace())
        {
            // will return 0 if not in there already
            size_t refCount = old2new->get(value);
            // increment and put back
            refCount ++;
            old2new->put(refCount, value);
        }
    }
}

RexxObject *MemoryObject::dumpImageStats(void)
/******************************************************************************/
/*                                                                            */
/******************************************************************************/
{
    MemoryStats _imageStats;

    /* clear all of the statistics */
    _imageStats.clear();
    /* gather a fresh set of stats for all of the segments */
    newSpaceNormalSegments.gatherStats(&_imageStats, &_imageStats.normalStats);
    newSpaceLargeSegments.gatherStats(&_imageStats, &_imageStats.largeStats);

    /* print out the memory statistics */
    _imageStats.printMemoryStats();
    return TheNilObject;
}


/**
 *
 * Add a new pool to the memory set.
 *
 * @param pool   The new pool.
 */
void MemoryObject::memoryPoolAdded(MemorySegmentPool *pool)
{
    currentPool = pool;
}


void MemoryObject::shutdown()
/******************************************************************************/
/* Function:  Free all the memory pools currently accessed by this process    */
/*    If not already released.  Then set process Pool to NULL, indicate       */
/*    pools have been released.                                               */
/******************************************************************************/
{
    MemorySegmentPool *pool = firstPool;
    while (pool != NULL)
    {
        // save the one we're about to release, and get the next one.
        MemorySegmentPool *releasedPool = pool;
        pool = pool->nextPool();
        // go free this pool up
        releasedPool->freePool();
    }
    // clear out the memory pool information
    firstPool = NULL;
    currentPool = NULL;
}


void MemoryObject::setUpMemoryTables(MapTable *old2newTable)
/******************************************************************************/
/* Function:  Set up the initial memory table.                                */
/******************************************************************************/
{
    // fix up the previously allocated live stack to have the correct
    // characteristics...we're almost ready to go on the air.
    liveStack->setBehaviour(TheLiveStackBehaviour);
    liveStack = new ((void *)liveStack) LiveStack(Memory::LiveStackSize);
    // set up the old 2 new table provided for us
    old2new = old2newTable;
    // Now get our savestack
    saveStack = new (Memory::SaveStackSize) PushThroughStack(Memory::SaveStackSize);
}


/**
 * Add a string to the global name table.
 *
 * @param value  The new value to add.
 *
 * @return The single instance of this string.
 */
RexxString *MemoryObject::getGlobalName(const char *value)
{
    // see if we have a global table.  If not collecting currently,
    // just return the non-unique value

    RexxString *stringValue = new_string(value);
    if (globalStrings == OREF_NULL)
    {
        return stringValue;                /* just return the string            */
    }

    // now see if we have this string in the table already
    RexxString *result = (RexxString *)globalStrings->at(stringValue);
    if (result != OREF_NULL)
    {
        return result;                       // return the previously created one
    }
    /* add this to the table             */
    globalStrings->put((RexxObject *)stringValue, stringValue);
    return stringValue;              // return the newly created one
}


void MemoryObject::create()
/******************************************************************************/
/* Function:  Initial memory setup during image build                         */
/******************************************************************************/
{
    RexxClass::createInstance();         /* get the CLASS class created       */
    RexxInteger::createInstance();
    // Now get our savestack
    memoryObject.setUpMemoryTables(OREF_NULL);
}


void MemoryObject::restore()
/******************************************************************************/
/* Function:  Memory management image restore functions                       */
/******************************************************************************/
{
    /* Retrieve special saved objects    */
    /* OREF_ENV and primitive behaviours */
    /* are already restored              */
                                         /* start restoring class OREF_s      */
    RESTORE_CLASS(Object, RexxClass);
    RESTORE_CLASS(Class, RexxClass);
                                         /* (CLASS is already restored)       */
    RESTORE_CLASS(String, RexxClass);
    RESTORE_CLASS(Array, RexxClass);
    RESTORE_CLASS(Directory, RexxClass);
    RESTORE_CLASS(Integer, RexxIntegerClass);
    RESTORE_CLASS(List, RexxClass);
    RESTORE_CLASS(Message, RexxClass);
    RESTORE_CLASS(Method, RexxClass);
    RESTORE_CLASS(Routine, RexxClass);
    RESTORE_CLASS(Package, RexxClass);
    RESTORE_CLASS(RexxContext, RexxClass);
    RESTORE_CLASS(NumberString, RexxClass);
    RESTORE_CLASS(Queue, RexxClass);
    RESTORE_CLASS(Stem, RexxClass);
    RESTORE_CLASS(Supplier, RexxClass);
    RESTORE_CLASS(Table, RexxClass);
    RESTORE_CLASS(IdentityTable, RexxClass);
    RESTORE_CLASS(Relation, RexxClass);
    RESTORE_CLASS(MutableBuffer, RexxClass);
    RESTORE_CLASS(Pointer, RexxClass);
    RESTORE_CLASS(Buffer, RexxClass);
    RESTORE_CLASS(WeakReference, RexxClass);
    RESTORE_CLASS(StackFrame, RexxClass);

    memoryObject.setOldSpace();          /* Mark Memory Object as OldSpace    */
    /* initialize the tables used for garbage collection. */
    memoryObject.setUpMemoryTables(new MapTable(Memory::DefaultOld2NewSize));
                                         /* If first one through, generate all*/
    IntegerZero   = new_integer(0);      /*  static integers we want to use...*/
    IntegerOne    = new_integer(1);      /* This will allow us to use static  */
    IntegerTwo    = new_integer(2);      /* integers instead of having to do a*/
    IntegerThree  = new_integer(3);      /* new_integer evrytime....          */
    IntegerFour   = new_integer(4);
    IntegerFive   = new_integer(5);
    IntegerSix    = new_integer(6);
    IntegerSeven  = new_integer(7);
    IntegerEight  = new_integer(8);
    IntegerNine   = new_integer(9);
    IntegerMinusOne = new_integer(-1);

    // the activity manager will create the local server, which will use the
    // stream classes.  We need to get the external libraries reloaded before
    // that happens.
    Interpreter::init();
    ActivityManager::init();             /* do activity restores              */
    PackageManager::restore();           // finish restoration of the packages.
}


/**
 * Default mark handler mark method.
 *
 * @param field  The field being marked.
 * @param object the object value in the field.
 */
void MarkHandler::mark(RexxObject **field, RexxObject *object)
{
    Interpreter::logicError("Wrong mark routine called");
}


/**
 * pure virtual method for handling the mark operation during an image save.
 *
 * @param pMarkObject
 *                   The pointer to the field being marked.
 * @param markObject The object being marked.
 */
void ImageSaveMarkHandler::mark(RexxObject **pMarkObject, RexxObject *markObject)
{
    // Save image processing.  We only handle this if the object has not
    // already been marked.
    if (!markObject->isObjectLive(markWord))
    {
        // now immediately mark this
        markObject->setObjectLive(markWord);
        // push this object on to the live stack so it's references can be marked later.
        memory->pushLiveStack(markObject);
        // get the size of this object.
        size_t size = markObject->getObjectSize();
        // add this to our image statistics
        memory->logObjectStats(markObject);

        // ok, this is our target copy address
        RexxObject *bufferReference = (RexxObject *)(imageBuffer + imageOffset);
        // we allocated a hard coded buffer, so we need to make sure we don't blow
        // the buffer size.
        if (imageOffset + size > Memory::MaxImageSize)
        {
            Interpreter::logicError("Rexx saved image exceeds expected maximum");
        }

        // copy the object to the image buffer
        memcpy((void *)bufferReference, (void *)markObject, size);
        // clear the mark in the copy so we're clean in restore
        bufferReference->clearObjectMark();
        // now get the behaviour object
        RexxBehaviour *behaviour = bufferReference->behaviour;
        // if this is a non primitive behaviour, we need to mark the
        // copy object apprpriately (this uses the live mark)
        if (behaviour->isNonPrimitive())
        {
            bufferReference->setNonPrimitive();
        }
        else
        {
            // double check that we're not accidentally saving a transient class.
            if (behaviour->isTransientClass())
            {
                Interpreter::logicError("Transient class included in image buffer");
            }

            // clear this out, as this is overloaded with the oldspace flag.
            bufferReference->setPrimitive();
            // replace behaviour with normalized type number
            bufferReference->behaviour = behaviour->getSavedPrimitiveBehaviour();
        }

        // replace the behaviour in the original object with the image offset.
        // this is a destructive operation, but we're not going to be running any
        // Rexx code at this point, so this is fine.
        markObject->behaviour = (RexxBehaviour *)imageOffset;
        // now update our image offset with the next object location.
        imageOffset += size;
    }

    // now update the field reference with the moved offset location.
    *pMarkObject = (RexxObject *)markObject->behaviour;
}

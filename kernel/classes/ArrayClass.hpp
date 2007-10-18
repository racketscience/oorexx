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
/* REXX Kernel                                                  ArrayClass.hpp   */
/*                                                                            */
/* Primitive Array Class Definitions                                          */
/*                                                                            */
/******************************************************************************/
#ifndef Included_RexxArray
#define Included_RexxArray

#define MAX_FIXEDARRAY_SIZE 100000000L
#define ARRAY_EXTEND_EXTRA_SIZE  10
#define ARRAY_EXTEND_EXTRA_LARGE_SIZE  100
#define ARRAY_MIN_SIZE          4

#define RaiseBoundsNone     0x00000000
#define RaiseBoundsUpper    0x00000001
#define RaiseBoundsInvalid  0x00000002
#define RaiseBoundsTooMany  0x00000004
#define RaiseBoundsAll      0x0000000F
#define ExtendUpper         0x00000010


typedef struct copyElelmentParm {
   size_t firstChangedDimension;
   RexxArray *newArray;
   RexxArray *newDimArray;
   RexxArray *oldDimArray;
   size_t deltaDimSize;
   size_t copyElements;
   size_t skipElements;
   RexxObject **startNew;
   RexxObject **startOld;
} COPYELEMENTPARM;


 class RexxArray : public RexxObject {
  public:

   inline void * operator new(size_t size, void *objectPtr) { return objectPtr; };
   void * operator new(size_t, RexxObject **, size_t, RexxClass *);
   void * operator new(size_t, RexxObject *);
   void * operator new(size_t, RexxObject *, RexxObject *);
   void * operator new(size_t, RexxObject *, RexxObject *, RexxObject *);
   void * operator new(size_t, RexxObject *, RexxObject *, RexxObject *, RexxObject *);
   void * operator new(size_t, size_t, RexxObject **);
   void * operator new(size_t, size_t, RexxClass *cls = TheArrayClass);
   inline RexxArray(RESTORETYPE restoreType) { ; };
   inline RexxArray() { ; };
   inline ~RexxArray() { ; };

   void         init(size_t, size_t);
   void         live();
   void         liveGeneral();
   void         flatten(RexxEnvelope *);
   RexxObject  *copy();
   RexxArray   *makeArray();
   RexxArray   *allItems();
   RexxArray   *allIndexes();
   RexxString  *toString(RexxString *, RexxString *);
   RexxString  *makeString(RexxString *, RexxString *);
// Temporary bypass for BUG #1700606
#if 0
   RexxString  *primitiveMakeString();
#endif
   RexxObject  *getRexx(RexxObject **, size_t);
   void         put(RexxObject * eref, size_t pos);
   RexxObject  *putRexx(RexxObject **, size_t);
   RexxObject  *remove(size_t);
   RexxObject  *removeRexx(RexxObject **, size_t);
   RexxObject  *append(RexxObject *);
   void         setExpansion(RexxObject * expansion);
   RexxInteger *available(size_t position);
   size_t       validateIndex(RexxObject **, size_t, size_t, size_t);
   RexxInteger *sizeRexx();
   RexxObject  *firstRexx();
   RexxObject  *lastRexx();
   RexxObject  *nextRexx(RexxObject **, size_t);
   RexxObject  *previousRexx(RexxObject **, size_t);
   RexxArray   *section(size_t, size_t);
   RexxObject  *sectionRexx(RexxObject *, RexxObject *);
   RexxObject  *sectionSubclass(size_t, size_t);
   RexxInteger *hasIndex(RexxObject *);
   size_t       hasIndexNative(size_t);
   RexxObject  *hasIndexRexx(RexxObject **, size_t);
   size_t       numItems();
   RexxObject  *items();
   RexxObject  *dimension(RexxObject *);
   size_t       getDimension();
   RexxObject  *supplier();
   RexxObject  *join(RexxArray *);
   RexxObject  *insert(RexxObject *, size_t);
   RexxArray   *extend(size_t);
   void         shrink(size_t);
   size_t       indexOf(RexxObject *);
   void         deleteItem(size_t);
   void         insertItem(RexxObject *, size_t);
   RexxArray   *extendMulti(RexxObject **, size_t, size_t);
   void         resize();
   void         ensureSpace(size_t newSize);
   RexxObject  *newRexx(RexxObject **, size_t);
   RexxObject  *of(RexxObject **, size_t);
   RexxObject  *empty();
   RexxObject  *isEmpty();
   RexxObject  *index(RexxObject *);
   RexxObject  *hasItem(RexxObject *);
   RexxObject  *removeItem(RexxObject *);
   void         mergeSort(RexxArray *working, size_t left, size_t right);
   void         merge(RexxArray *working, size_t left, size_t mid, size_t right);
   void         mergeSort(RexxObject *comparator, RexxArray *working, size_t left, size_t right);
   void         merge(RexxObject *comparator, RexxArray *working, size_t left, size_t mid, size_t right);
   void         quickSort(size_t left, size_t right);
   void         quickSort(RexxObject *comparator, size_t left, size_t right);
   wholenumber_t sortCompare(RexxObject *comparator, RexxObject *left, RexxObject *right);
   RexxArray   *sortRexx();
   RexxArray   *sortWithRexx(RexxObject *comparator);
   RexxArray   *stableSortRexx();
   RexxArray   *stableSortWithRexx(RexxObject *comparator);

   inline void         addLast(RexxObject *item) { this->insertItem(item, this->size() + 1); }
   inline void         addFirst(RexxObject *item) { this->insertItem(item, 1); }
   inline RexxArray   *array() { return this->makeArray(); }
   inline size_t       size() { return this->expansionArray->arraySize; }
   inline RexxObject  *get(size_t pos) { return (this->data())[pos-1];}
   inline RexxObject **data() { return this->expansionArray->objects; }
   inline RexxObject **data(size_t pos) { return &((this->data())[pos-1]);}
   inline RexxArray   *getExpansion() { return this->expansionArray; }
   arraysize_t         findSingleIndexItem(RexxObject *item);
   RexxObject *        indexToArray(size_t idx);
   RexxObject *        convertIndex(size_t idx);


 protected:

   size_t arraySize;                   /* current size of array         */
   size_t maximumSize;                 /* Maximum size array can grow   */
   RexxArray *dimensions;              /* Array containing dimensions - null if 1-dimensional */
   RexxArray *expansionArray;          /* actual array containing data  */
   RexxObject  *objects[1];            /* Data.                         */
 };
 #endif

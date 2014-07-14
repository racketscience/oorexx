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
/* Backing contents for all hash-based collections                            */
/*                                                                            */
/******************************************************************************/
#ifndef Included_HashContents
#define Included_HashContents

/**
 * Base class for storing the contents of a hash-based
 * collection.  This version bases all index and item
 * searches on object equality.  Subclasses can override these
 * default rules.  This is the backing contents only,
 * there is a separate collection class that implements
 * the collection interface and allocates a backing
 * context.
 */
class HashContents : public RexxInternalObject
{
public:
    // The type for the reference links
    typedef size_t ItemLink;

    // link terminator
    static const ItemLink NoMore = 0;
    // indicates not linked
    static const ItemLink NoLink = SIZE_MAX;

    /**
     * Small helper class for an entry stored in the contents.
     */
    class ContentEntry
    {
    public:
        inline bool isAvailable() { return index == OREF_NULL; }
        // these can only be used when object identity matches are called for...generally
        // just some special purpose things.
        inline bool matches(RexxInternalObject *i, RexxInternalObject v) { return index == i && value == v; }
        inline bool matches(RexxInternalObject *i) { return index == i; }

        RexxInternalObject *index;           // item index object
        RexxInternalObject *value;           // item value object
        ItemLink next;                       // next item in overflow bucket
    };

    // minimum bucket size we'll work with
    static const MinimumBucketSize = 17;

           void * operator new(size_t size, size_t capacity);
    inline void * operator new(size_t size, void *objectPtr) { return objectPtr; };
    inline void  operator delete(void *, void *) { ; }
    inline void  operator delete(void *, size_t) { ; }

    inline HashContents(RESTORETYPE restoreType) { ; };
           HashContents(size_t entries, size_t total);

    virtual void live(size_t);
    virtual void liveGeneral(MarkReason reason);
    virtual void flatten(RexxEnvelope *);

    // default index comparison method
    virtual bool isIndexEqual(RexxInternalObject *target, RexxInternalObject *entryIndex)
    {
        // default comparison is object identity
        return target == entryIndex;
    }

    // default item comparison method
    virtual bool isItemEqual(RexxInternalObject *target, RexxInternalObject *entryItem)
    {
        // default comparison is object identity
        return target == entryItem;
    }

    // default index hashing method.  bypass the hash() method and directly use the hash value
    virtual ItemLink hashIndex(RexxInternalObject *index)
    {
        return (ItemLink)(obj->getHashValue() % bucketSize);
    }

    void initializeFreeChain();

    // set the entry values for a position
    inline void setEntry(ItemLink position, RexxInternalObject *value, RexxInternalObject *index)
    {
        setField(entries[position].value, value);
        setField(entries[position].index, index);
    }

    // clear and entry in the chain
    inline void clearEntry(ItemLink position)
    {
        // clear out the value/index fields
        setField(entries[position].value, OREF_NULL);
        setField(entries[position].index, OREF_NULL);
        // clear the link also.
        next = NoMore;
    }

    // copy an entry contents into another entry
    inline void copyEntry(ItemLink target, ItemLink source)
    {
        // copy all of the information
        setEntry(target, entryValue(source), entryIndex(source));
        entries[target].next = entries[source].next;
    }

    // remove a non-anchor entry from a hash chain
    inline void closeChain(ItemLink position, ItemLink previous)
    {
        entries[previous].next = entries[position].next;
        // move the removed item back to the free chain and
        // clear it out
        returnToFreeChain(position);
    }

    // return an entry to the free chain
    inline void  returnToFreeChain(ItemLink position)
    {
        // clear the entry, then place at the head of the
        // free chain.
        clearEntry(position);
        entries[position].next = freeChain;
        freeChain = position;
    }

    // set the value in an existing entry
    inline void setValue(ItemLink position, RexxInternalObject *value)
    {
        setField(entries[position].value, value);
    }

    // perform an index comparison for a position
    inline bool isIndex(ItemLink position, RexxInternalObject *index)
    {
        return isIndexEqual(index, entries[position].index);
    }

    // perform an item comparison for a position
    inline bool isItem(ItemLink position, RexxInternalObject *item)
    {
        return isItemEqual(item, entries[position].item);
    }

    // perform an entry comparison for a position using both index and item value
    inline bool isItem(ItemLink position, RexxInternalObject *index, RexxInternalObject *item)
    {
        return isIndexEqual(index, entries[position].index) && isItemEqual(item, entries[position].item);
    }

    // check if an entry is availabe
    inline bool isAvailable(ItemLink postion)
    {
        return entries[postion].isAvailable();
    }

    // check if an entry is availabe
    inline bool isInUse(ItemLink postion)
    {
        return !entries[postion].isAvailable();
    }

    // step to the next position in the chain
    inline ItemLink nextEntry(ItemLink position)
    {
        return entries[position].next;
    }

    // get the value for an entry
    inline RexxInternalObject *entryValue(ItemLink position)
    {
        return entries[position].value;
    }

    // get the index for an entry
    inline RexxInternalObject *entryIndex(ItemLink position)
    {
        return entries[position].index;
    }

    // test if the table is full
    inline bool isFull()
    {
        return freeChain != NoMore;
    }

    // check if this table can hold an additional number of items (usually used on merge operations)
    inline bool hasCapacity(size_t count)
    {
        return totalSize - itemCount > count;
    }

    /**
     * Return the total current capacity.
     *
     * @return The total number of items this table can hold.
     */
    inline size_t capacity()
    {
        return totalSize;
    }


    inline size_t items() { return itemCount; }
    inline bool isEmpty() { return itemCount == 0; }
    bool put(RexxInternalObject *value, RexxInternalObject *index);
    bool append(RexxInternalObject *value, RexxInternalObject * index, ItemLink position);
    RexxInternalObject *remove(RexxInternalObject *index);
    void removeChainLink(ItemLink &position, ItemLink previous);
    bool locateEntry(RexxInternalObject *index, ItemLink &position, ItemLink &previous);
    bool locateEntry(RexxInternalObject *index, RexxInternalObject *item, ItemLink &position, ItemLink &previous);
    bool locateItem(RexxInternalObject *item, ItemLink &position, ItemLink &previous);
    RexxArray *removeAll(RexxInternalObject *index);
    RexxInternalObject *removeItem(RexxInternalObject *value, RexxInternalObject *index);
    bool hasItem(RexxInternalObject *value, RexxInternalObject *index );
    bool hasItem(RexxInternalObject *item);
    RexxInternalObject *removeItem(RexxObject *item);
    RexxObject *nextItem(RexxObject *value, RexxObject *index);
    RexxInternalObject *get(RexxInternalObject *index);
    RexxArray  *getAll(RexxInternalObject *index);
    size_t countAllIndex(RexxInternalObject *index, ItemLink &anchorPosition);
    size_t countAllItem(RexxInternalObject *item);
    RexxArray  *allIndex(RexxInternalObject *item);
    RexxInternalObject *getIndex(RexxInternalObject *item);
    void merge(HashContents *target);
    void reMerge(HashContents *newHash);
    bool mergeItem(RexxInternalObject *, RexxInternalObject *index);
    bool RexxHashTable::mergePut(RexxInternalObject *item, RexxInternalObject *index);
    RexxArray  *allItems();
    void empty();
    RexxArray *RexxHashTable::makeArray();
    RexxArray *allIndexes();
    RexxArray *uniqueIndexes();
    RexxSupplier *supplier();
    void reHash(HashContents *newHash);
    bool add(RexxInternalObject *item, RexxInternalObject *index);

protected:

    size_t   bucketSize;                // size of the hash table
    size_t   totalSize;                 // total size of the table, including the overflow area
    size_t   itemCount;                 // total number of items in the table
    ItemLink freeChain;                 // first free element
    ContentEntry entries[1];            // hash table entries
};

#endif

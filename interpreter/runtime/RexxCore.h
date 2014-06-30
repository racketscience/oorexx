/*----------------------------------------------------------------------------*/
/*                                                                            */
/* Copyright (c) 1995, 2004 IBM Corporation. All rights reserved.             */
/* Copyright (c) 2005-2014 Rexx Language Association. All rights reserved.    */
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
/* REXX Kernel                                                RexxCore.h      */
/*                                                                            */
/* Global Declarations                                                        */
/******************************************************************************/

/******************************************************************************/
/* Globally required include files                                            */
/******************************************************************************/
#ifndef RexxCore_INCLUDED
#define RexxCore_INCLUDED

#include "oorexxapi.h"                 // this is the core to everything

/* ANSI C definitions */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <bitset>

// a useful type-safe bit flag class where the allowed settings are based
// on enum values.

// this is defined as a template so we can specify what enums are stored here.
template < typename TEnum, int TMaxFlags = 8 >
class FlagSet
{
public:
    // set a flag value to true
    inline void set(const TEnum flag)
    {
        flags.set(flag);
    }

    // turn a flag value off
    inline void reset(const TEnum flag)
    {
        flags.reset(flag);
    }

    // flip the value of a bit
    inline void reset(const TEnum flag)
    {
        flags.flip(flag);
    }

    // turn a flag value off
    inline bool test(const TEnum flag)
    {
        return flags.test(flag);
    }

    // access the value of a flag
    inline bool operator[] (const TEnum flag) const
    {
        return flags[flag];
    }

    // test if any flags are set
    inline bool any() const
    {
        return flags.any();
    }

    // test for any of the specified flags being set
    inline bool any(const TEnum flag1, const TEnum flag2)
    {
        return flags.test(flag) || flags.test(flag2);
    }

    // test for any of the specified flags being set
    inline bool any(const TEnum flag1, const TEnum flag2, const TEnum flag3)
    {
        return flags.test(flag) || flags.test(flag2) || flags.test(flag3);
    }

    // test if all flags are set
    inline bool all() const
    {
        return flags.all();
    }

    // test for all of the specified flags being set
    inline bool all(const TEnum flag1, const TEnum flag2)
    {
        return flags.test(flag) && flags.test(flag2);
    }

    // test for any of the specified flags being set
    inline bool all(const TEnum flag1, const TEnum flag2, const TEnum flag3)
    {
        return flags.test(flag) && flags.test(flag2) && flags.test(flag3);
    }

    // test if no flags are set
    inline bool none() const
    {
        return flags.none();
    }

private:
    std:bitset < 32 > flags;
};


/* REXX Library definitions */
#define OREF_NULL NULL                 /* definition of a NULL REXX object  */

#include "RexxPlatformDefinitions.h"

/******************************************************************************/
/* Literal definitions                                                        */
/******************************************************************************/
#include "RexxConstants.hpp"

/******************************************************************************/
/* Kernel Internal Limits                                                     */
/******************************************************************************/

const int MAX_ERROR_NUMBER = 99999;        /* maximum error code number         */
const int MAX_SYMBOL_LENGTH = 250;         /* length of a symbol name           */

/******************************************************************************/
/* Defines for argument error reporting                                       */
/******************************************************************************/

const int ARG_ONE    = 1;
const int ARG_TWO    = 2;
const int ARG_THREE  = 3;
const int ARG_FOUR   = 4;
const int ARG_FIVE   = 5;
const int ARG_SIX    = 6;
const int ARG_SEVEN  = 7;
const int ARG_EIGHT  = 8;
const int ARG_NINE   = 9;
const int ARG_TEN    = 10;


// Object Reference Assignment
// OrefSet handles reference assignment for situations where an
// object exists in the oldspace (rexx image) area and the fields is being updated
// to point to an object in the normal Rexx heap.  Since oldspace objects do
// not participate in the mark-and-sweep operation, we need to keep track of these
// references in a special table.
//
// OrefSet (or the setField() shorter version) needs to be used to set values in any object that
// a) might be part of the saved imaged (transient objects like the LanguageParser, RexxActivation,
// and RexxActivity are examples of classes that are not...any class that is visible to the Rexx programmer
// are classes that will be part of the image, as well as any of the instruction/expresson objects
// created by the LanguageParser).  Note that as a general rule, fields that are set in an object's constructor
// do not need this...the object, by definition, is being newly created and cannot be part of the saved image.
// Other notible exceptions are the instruction/expression objects.  These object, once created, are immutable.
// Therefore, any fields that are set in these objects can only occur while a program is getting translated.  Once
// the translation is complete, all of the references are set and these can be safely included in the image
// without needing to worry about oldspace issues.  If you are uncertain how a given set should be happen,
// use OrefSet().  It is never an error to use in places where it is not required, but it certainly can be an
// error to use in places where it is required.

#ifndef CHECKOREFS
#define OrefSet(o,r,v) ((o)->isOldSpace() ? memoryObject.setOref((void *)&(r),(RexxObject *)v) : (RexxObject *)(r=v))
#else
#define OrefSet(o,r,v) memoryObject.checkSetOref((RexxObject *)o, (RexxObject **)&(r), (RexxObject *)v, __FILE__, __LINE__)
#endif

// short cut version of OrefSet().  99% of the uses specify this as the object pointer...this version
// saves a little typing :-)
#define setField(r, v)  OrefSet(this, this->r, v)


// forward declaration of commonly used classes
class RexxExpressionStack;
class RexxActivation;
class RexxObject;
class RexxClass;
class RexxDirectory;
class RexxIntegerClass;
class RexxArray;
class RexxMemory;
class RexxString;


/******************************************************************************/
/* Change EXTERN definition if not already created by GDATA                   */
/******************************************************************************/

#ifndef INITGLOBALPTR                  // if not the global, this is a NOP.
#define INITGLOBALPTR
#endif
#ifndef EXTERN
#define EXTERN extern                  /* turn into external definition     */
#endif

#ifndef EXTERNMEM
#define EXTERNMEM extern               /* turn into external definition     */
#endif

/******************************************************************************/
/* Primitive Method Type Definition Macros                                    */
/******************************************************************************/
                                       /* following two are used by OKINIT  */
                                       /*  to build the VFT Array.          */
#define CLASS_EXTERNAL(b,c)
#define CLASS_INTERNAL(b,c)

#define koper(name) RexxObject *name(RexxObject *);


/******************************************************************************/
/* Global Objects - General                                                   */
/******************************************************************************/


// this one is special, and is truly global.
EXTERNMEM RexxMemory  memoryObject;   /* memory object                     */

// TODO:  make these into statics inside classes.

#define TheArrayClass RexxArray::classInstance
#define TheClassClass RexxClass::classInstance
#define TheDirectoryClass RexxDirectory::classInstance
#define TheIntegerClass RexxInteger::classInstance
#define TheListClass RexxList::classInstance
#define TheMessageClass RexxMessage::classInstance
#define TheMethodClass RexxMethod::classInstance
#define TheRoutineClass RoutineClass::classInstance
#define ThePackageClass PackageClass::classInstance
#define TheRexxContextClass RexxContext::classInstance
#define TheNumberStringClass RexxNumberString::classInstance
#define TheObjectClass RexxObject::classInstance
#define TheQueueClass RexxQueue::classInstance
#define TheStemClass RexxStem::classInstance
#define TheStringClass RexxString::classInstance
#define TheMutableBufferClass RexxMutableBuffer::classInstance
#define TheSupplierClass RexxSupplier::classInstance
#define TheTableClass RexxTable::classInstance
#define TheIdentityTableClass RexxIdentityTable::classInstance
#define TheRelationClass RexxRelation::classInstance
#define ThePointerClass RexxPointer::classInstance
#define TheBufferClass RexxBuffer::classInstance
#define TheWeakReferenceClass WeakReference::classInstance
#define TheStackFrameClass StackFrameClass::classInstance

#define TheEnvironment RexxMemory::environment
#define TheStaticRequires RexxMemory::staticRequires
#define TheFunctionsDirectory RexxMemory::functionsDir
#define TheCommonRetrievers RexxMemory::commonRetrievers
#define TheKernel RexxMemory::kernel
#define TheSystem RexxMemory::system

#define TheNilObject RexxNilObject::nilObject

#define TheNullArray RexxArray::nullArray

#define TheFalseObject RexxInteger::falseObject
#define TheTrueObject RexxInteger::trueObject
#define TheNullPointer RexxPointer::nullPointer

#define IntegerZero RexxInteger::integerZero
#define IntegerOne RexxInteger::integerOne
#define IntegerTwo RexxInteger::integerTwo
#define IntegerThree RexxInteger::integerThree
#define IntegerFour RexxInteger::integerFour
#define IntegerFive RexxInteger::integerFive
#define IntegerSix RexxInteger::integerSix
#define IntegerSeven RexxInteger::integerSeven
#define IntegerEight RexxInteger::integerEight
#define IntegerNine RexxInteger::integerNine
#define IntegerMinusOne RexxInteger::integerMinusOne

#include "ClassTypeCodes.h"



/******************************************************************************/
/* Utility Macros                                                             */
/******************************************************************************/

#define RXROUNDUP(n,to)  ((((n)+(to-1))/(to))*to)
#define rounddown(n,to)  (((n)/(to))*to)

#define isOfClass(t,r) (r)->isObjectType(The##t##Behaviour)
#define isOfClassType(t,r) (r)->isObjectType(T_##t)

/******************************************************************************/
/* Utility Functions                                                          */
/******************************************************************************/

                                       /* find an environment symbol        */
#define env_find(s) (TheEnvironment->entry(s))

/******************************************************************************/
/* Thread constants                                                           */
/******************************************************************************/

#define NO_THREAD       -1

/******************************************************************************/
/* Global Objects - Names                                                     */
/******************************************************************************/
#undef GLOBAL_NAME
#define GLOBAL_NAME(name, value) EXTERN RexxString * OREF_##name INITGLOBALPTR;
#include "GlobalNames.h"

#include "ObjectClass.hpp"               // get real definition of Object

 #include "TableClass.hpp"               // memory has inline methods to these
 #include "StackClass.hpp"               // classes, so pull them in next.
 #include "RexxMemory.hpp"               // memory next, to get OrefSet
 #include "RexxBehaviour.hpp"            // now behaviours and
 #include "ClassClass.hpp"               // classes, which everything needs
 #include "RexxEnvelope.hpp"             // envelope is needed for flattens
 #include "RexxActivity.hpp"             // activity is needed for errors

/******************************************************************************/
/* Method arguments special codes                                             */
/******************************************************************************/

const size_t A_COUNT   = 127;            // pass arguments as pointer/count pair

/******************************************************************************/
/* Return codes                                                               */
/******************************************************************************/

const int RC_OK         = 0;
const int RC_LOGIC_ERROR  = 2;

const int POSITIVE    = 1;             // integer must be positive
const int NONNEGATIVE = 2;             // integer must be non-negative
const int WHOLE       = 3;             // integer must be whole


// some very common class tests
inline bool isString(RexxObject *o) { return isOfClass(String, o); }
inline bool isInteger(RexxObject *o) { return isOfClass(Integer, o); }
inline bool isArray(RexxObject *o) { return isOfClass(Array, o); }
inline bool isStem(RexxObject *o) { return isOfClass(Stem, o); }
inline bool isActivation(RexxObject *o) { return isOfClass(Activation, o); }
inline bool isMethod(RexxObject *o) { return isOfClass(Method, o); }

#include "ActivityManager.hpp"

/**
 * A function specifically for REQUESTing a STRING, since there are
 * four primitive classes that are equivalents for strings.  It will trap on
 * OREF_NULL.  This always returns a string value, going all the
 * way down the various methods of providing a string value.
 * Will also raise NOSTRING conditions.
 *
 * @param object The object we need a string value from.
 *
 * @return The string value of the object.
 */
inline RexxString *REQUEST_STRING(RexxObject *object)
{
  return (isOfClass(String, object) ? (RexxString *)object : (object)->requestString());
}


/**
 * Check for required arguments and raise a missing argument
 * error for the given position.
 *
 * @param object   The reference to check.
 * @param position the position of the argument for the error message.
 */
inline void requiredArgument(RexxObject *object, size_t position)
{
    if (object == OREF_NULL)
    {
        missingArgument(position);
    }
}


/**
 * REQUEST a STRING needed as a method
 * argument.  This raises an error if the object cannot be converted to a
 * string value.
 *
 * @param object   The object argument to check.
 * @param position The argument position, used for any error messages.
 *
 * @return The String value of the object, if it really has a string value.
 */
inline RexxString * stringArgument(RexxObject *object, size_t position)
{
    if (object == OREF_NULL)
    {
        missingArgument(position);
    }
    return object->requiredString(position);
}


/**
 * REQUEST a STRING needed as a method
 * argument.  This raises an error if the object cannot be converted to a
 * string value.
 *
 * @param object The object to check.
 * @param name   The parameter name of the argument (used for error reporting.)
 *
 * @return The string value of the object if it truely has a string value.
 */
inline RexxString * stringArgument(RexxObject *object, const char *name)
{
    if (object == OREF_NULL)
    {
        reportException(Error_Invalid_argument_noarg, name);
    }

    return object->requiredString(name);
}

// handle an option string argument where a default argument value is provided.
inline RexxString *optionalStringArgument(RexxObject *o, RexxString *d, size_t p)
{
    return (o == OREF_NULL ? d : stringArgument(o, p));
}


// handle an option string argument where a default argument value is provided.
inline RexxString *optionalStringArgument(RexxObject *o, RexxString *d, const char *p)
{
    return (o == OREF_NULL ? d : stringArgument(o, p));
}


/**
 * Parse a length method argument.  this must be a non-negative
 * whole number.  Raises a number if the argument was omitted or
 * is not a length numeric value.
 *
 * @param o      The object to check.
 * @param p      The argument position.
 *
 * @return The converted numeric value of the object.
 */
size_t lengthArgument(RexxObject *o, size_t p);


/**
 * Parse an optional length method argument.  this must be a
 * non-negative whole number.  Raises a number if the argument
 * not a length numeric value.
 *
 * @param o      The object to check.
 * @param d      The default value to return if the argument was omitted.
 * @param p      The argument position.
 *
 * @return The converted numeric value of the object.
 */
inline size_t optionalLengthArgument(RexxObject *o, size_t d, size_t p)
{
    return (o == OREF_NULL ? d : lengthArgument(o, p));
}


/**
 * Parse a position method argument.  this must be a positive
 * whole number.  Raises a number if the argument was omitted or
 * is not a length numeric value.
 *
 * @param o      The object to check.
 * @param p      The argument position.
 *
 * @return The converted numeric value of the object.
 */
size_t positionArgument(RexxObject *o, size_t p);


/**
 * Parse an optional position method argument.  this must be a
 * positive whole number.  Raises a number if the argument not a
 * length numeric value.
 *
 * @param o      The object to check.
 * @param d      The default value to return if the argument was omitted.
 * @param p      The argument position.
 *
 * @return The converted numeric value of the object.
 */
inline size_t optionalPositionArgument(RexxObject *o, size_t d, size_t p)
{
    return (o == OREF_NULL ? d : positionArgument(o, p));
}


/**
 * Parse a pad argument.  This must a string object and
 * only a single character long.
 *
 * @param o      The object argument to check.  Raises an error if this was
 *               omitted.
 * @param p      The argument position, for error reporting.
 *
 * @return The pad character from the argument string.
 */
char padArgument(RexxObject *o, size_t p);



/**
 * Parse an optional pad argument.  This must a string object
 * and only a single character long.
 *
 * @param o      The object argument to check.
 * @param d      The default pad character if the argument was omitted.
 * @param p      The argument position, for error reporting.
 *
 * @return The pad character from the argument string.
 */
inline char optionalPadArgument(RexxObject *o, char d, size_t p)
{
    return (o == OREF_NULL ? d : padArgument(o, p));
}


/**
 * Parse an option argument.  This must be a non-zero length string.
 *
 * @param o      The object to check.
 * @param p      The argument position for error messages.
 *
 * @return The first character of the option string.
 */
char optionArgument(RexxObject *o, size_t p);


/**
 * Parse an optional option argument.  This must be a non-zero
 * length string.
 *
 * @param o      The object to check.
 * @param d      The default option if this was an omitted argument.
 * @param p      The argument position for error messages.
 *
 * @return The first character of the option string.
 */
inline char optionalOptionArgument(RexxObject *o, char d, size_t p)
{
    return (o == OREF_NULL ? d : optionArgument(o, p));
}


/**
 * Handle an optional non-negative numeric option.
 *
 * @param o      The object to check.
 * @param d      The default value to return if the argument was omitted.
 * @param p      The argument position used for error reporting.
 *
 * @return The converted numeric value.
 */
inline size_t optionalNonNegative(RexxObject *o, size_t d, size_t p)
{
    return (o == OREF_NULL ? d : o->requiredNonNegative(p));
}


/**
 * Handle an optional positive numeric option.
 *
 * @param o      The object to check.
 * @param d      The default value to return if the argument was omitted.
 * @param p      The argument position used for error reporting.
 *
 * @return The converted numeric value.
 */
inline size_t optionalPositive(RexxObject *o, size_t d, size_t p)
{
    return (o == OREF_NULL ? d : o->requiredPositive(p));
}

/**
 * REQUEST an ARRAY needed as a method
 * argument.  This raises an error if the object cannot be converted to a
 * single dimensional array item.
 *
 * @param object   The argument object.
 * @param position The argument position (used for error reporting.)
 *
 * @return A converted single-dimension array.
 */
inline RexxArray *arrayArgument(RexxObject *object, size_t position)
{
    // this is required.
    if (object == OREF_NULL)
    {
        missingArgument(position);
    }
    // force to array form
    RexxArray *array = object->requestArray();
    // not an array or not single dimension?  Error!
    if (array == TheNilObject || array->getDimension() != 1)
    {
        reportException(Error_Execution_noarray, object);
    }
    return array;
}


/**
 * REQUEST an ARRAY needed as a method
 * argument.  This raises an error if the object cannot be converted to a
 * single dimensional array item.
 *
 * @param object   The argument object.
 * @param position The argument name (used for error reporting.)
 *
 * @return A converted single-dimension array.
 */
inline RexxArray * arrayArgument(RexxObject *object, const char *name)
{
    if (object == OREF_NULL)
    {
        reportException(Error_Invalid_argument_noarg, name);
    }

    // get the array form and verify we got a single-dimension array back.
    RexxArray *array = object->requestArray();
    if (array == TheNilObject || array->getDimension() != 1)
    {
        /* raise an error                    */
        reportException(Error_Invalid_argument_noarray, name);
    }
    return array;
}


/**
 * Validate that an argument is an instance of a specific class.
 *
 * @param object The argument to test.
 * @param clazz  The class type to check it against.
 * @param name   The argument name for error reporting.
 */
inline void classArgument(RexxObject *object, RexxClass *clazz, const char *name)
{
    if (object == OREF_NULL)             /* missing argument?                 */
    {
        reportException(Error_Invalid_argument_noarg, name);
    }

    if (!object->isInstanceOf(clazz))
    {
        reportException(Error_Invalid_argument_noclass, name, clazz->getId());
    }
}


/**
 * Request an array version for an argument.  Will perform
 * makearray processing on the object, if needed.
 *
 * @param obj    The object to request.
 *
 * @return The converted array value of the object or TheNilObject if
 *         if did not convert.
 */
inline RexxArray * REQUEST_ARRAY(RexxObject *obj) { return ((obj)->requestArray()); }

/**
 * Request an object to be converted to a RexxInteger
 * object.  Return TheNilObject if it could not be converted.
 *
 * @param obj    The object to convert.
 *
 * @return An Integer object instance representing this object or
 *         .nil if it cannot be converted.
 */
inline RexxInteger * REQUEST_INTEGER(RexxObject *obj) { return ((obj)->requestInteger(Numerics::ARGUMENT_DIGITS));}

#endif

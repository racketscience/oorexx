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
/*                                                                            */
/* Parsing methods for processing directive instructions.                     */
/*                                                                            */
/******************************************************************************/
#include "RexxCore.h"
#include "StringClass.hpp"
#include "ArrayClass.hpp"
#include "RexxActivation.hpp"
#include "LanguageParser.hpp"


/**
 * Parse a directive instruction.
 */
void LanguageParser::directive()
{
    // we are in a context where everthing we check is expected to be a
    // directive.

    // step to the next clause
    nextClause();

    // if we've hit the end, we're done.
    if (noClauseAvailable())
    {
        return;
    }

    RexxToken *token = nextReal();
    // we expect anything found here to be the start of a directive
    if (!token->isType(TOKEN_DCOLON))
    {
        syntaxError(Error_Translation_bad_directive);
    }

    // and this needs to be followed by a symbol keyword
    token = nextReal();
    if (!token->isSymbol())
    {
        syntaxError(Error_Symbol_expected_directive);
    }

    // and process each of the named directives
    switch (token->keyDirective())
    {
        // define a class
        case DIRECTIVE_CLASS:
            classDirective();
            break;

        // create a method
        case DIRECTIVE_METHOD:
            methodDirective();
            break;

        // create a routine
        case DIRECTIVE_ROUTINE:
            routineDirective();
            break;

        // require a package file or library
        case DIRECTIVE_REQUIRES:
            requiresDirective();
            break;

        // define an attribute
        case DIRECTIVE_ATTRIBUTE:
            attributeDirective();
            break;

        // define a constant
        case DIRECTIVE_CONSTANT:
            constantDirective();
            break;

        // define package-wide options
        case DIRECTIVE_OPTIONS:
            optionsDirective();
            break;

        // an unknown directive
        default:
            syntaxError(Error_Translation_bad_directive);
            break;
    }
}


/**
 * Verify that no code follows a directive except for more
 * directive instructions.
 *
 * @param errorCode The error code to issue if this condition was violated.
 */
void LanguageParser::checkDirective(int errorCode)
{
    // save the clause location so we can reset for errors
    SourceLocation location = clauseLocation;

    // step to the next clause...if there is one, it must
    // be a directive.
    nextClause();
    if (clauseAvailable())
    {
        // The first token of the next instruction must be a ::
        RexxToken *token = nextReal();

        if (!token->isType(TOKEN_DCOLON))
        {
            syntaxError(errorCode);
        }
        // back up and push the clause back
        firstToken();
        reclaimClause();
    }
    // this resets the current clause location so that any errors on the current
    // clause detected after the clause check reports this on the correct line
    // number
    clauseLocation = location;
}


/**
 * Test if a directive is followed by a body of Rexx code
 * instead of another directive or the end of the source.
 *
 * @return True if there is a non-directive clause following the current
 *         clause.
 */
bool LanguageParser::hasBody()
{
    // assume there's no body here
    bool result = false;

    // if we have anything to look at, see if it is a directive or not.
    nextClause();
    if (clauseAvailable())
    {
        // we have a clause, now check if this is a directive or not
        RexxToken *token = nextReal();
        // not a "::", not a directive, which means we have real code to deal with
        result = token->isType(TOKEN_DCOLON);
        // reset this clause entirely so we can start parsing for real.
        firstToken();
        reclaimClause();
    }
    return result;
}

enum
{
    DEFAULT_GUARD,                 // default guard
    GUARDED_METHOD,                // guard specified
    UNGUARDED_METHOD,              // unguarded specified
} GuardFlag;

enum
{
    DEFAULT_PROTECTION,            // using defualt protection
    PROTECTED_METHOD,              // security manager permission needed
    UNPROTECTED_METHOD,            // no protection.
} ProtectedFlag;

enum
{
    DEFAULT_ACCESS_SCOPE,          // using defualt scope
    PUBLIC_SCOPE,                  // publicly accessible
    PRIVATE_SCOPE,                 // private scope
} AccessFlag;


/**
 * Test if a class directive is defining a duplicate class.
 *
 * @param name   The name from the ::class directive.
 *
 * @return true if a class with this name has already been encounterd.
 */
bool LanguageParser::isDuplicateClass(RexxString *name)
{
    return classDependencies->hasEntry(name);
}


/**
 * Test if a routine directive is defining a duplicate routine.
 *
 * @param name   The name from the ::routine directive.
 *
 * @return true if routine with this name has already been
 *         encounterd.
 */
bool LanguageParser::isDuplicateRoutine(RexxString *name)
{
    return routines->hasEntry(name);
}


void LanguageParser::addClassDirective(RexxString *name, ClassDirective *directive)
{
    classDependencies->put(name, directive);
}


/**
 * Process a ::CLASS directive for a source file.
 */
void LanguageParser::classDirective()
{
    // first token is the name, which must be a symbol or string name
    RexxToken *token = nextReal();
    if (!token->isSymbolOrLiteral())
    {
        syntaxError(Error_Symbol_or_string_class);
    }

    // get the class name
    RexxString *name = token->value();
    // and we export this name in uppercase

    RexxString *public_name = commonString(name->upper());
    // check for a duplicate class
    if (isDuplicateClass(public_name))
    {
        syntaxError(Error_Translation_duplicate_class);
    }

    // TODO:  figure this out at the end.
//  this->flags |= _install;         /* have information to install       */

    // create a class directive and add this to the dependency list
    activeClass = new ClassDirective(name, pulic_name, clause));
    // add this to our directives list.
    addClassDirective(public_name, activeClass);

    // we're using default scope.
    AccessFlag accessFlag = DEFAULT_ACCESS_SCOPE;

    // now we have a bunch of option keywords to handle, which can
    // only be specified once each.
    for (;;)
    {
        // real simple class definition.
        token = nextReal();
        if (token->isEndOfClause())
        {
            break;                       /* get out of here                   */
        }
        // all options are symbols
        else if (!token->isSymbol())
        {
            syntaxError(Error_Invalid_subkeyword_class, token);
        }
        else
        {
            // directive sub keywords are also table based
            DirectiveSubKeyword type = token->subDirective();
            switch (type)
            {
                // ::CLASS name METACLASS metaclass
                case SUBDIRECTIVE_METACLASS:
                    // can't be a duplicate
                    if (activeClass->getMetaClass() != OREF_NULL)
                    {
                        syntaxError(Error_Invalid_subkeyword_class, token);
                    }

                    // this is a required string or symbol value
                    token = nextReal();
                    if (!token->isSymbolOrLiteral())
                    {
                        syntaxError(Error_Symbol_or_string_metaclass, token);
                    }
                    // set the meta class...use the upper case name
                    activeClass->setMetaClass(commonString(token->upperValue());
                    break;

                // ::CLASS name PUBLIC
                case SUBDIRECTIVE_PUBLIC:
                    // have we already seen an ACCESS flag?  This is an error
                    if (accessFlag != DEFAULT_ACCESS_SCOPE)
                    {
                        syntaxError(Error_Invalid_subkeyword_class, token);
                    }
                    accessflag = PUBLIC_SCOPE;
                    // set the access in the active class.
                    activeClass->setPublic();
                    break;

                // ::CLASS name PRIVATE
                case SUBDIRECTIVE_PRIVATE:
                    if (accessFlag != DEFAULT_ACCESS_SCOPE)
                    {
                        syntaxError(Error_Invalid_subkeyword_class, token);
                    }
                    accessFlag = PRIVATE_SCOPE;

                    // don't need to set anything in the directive...this is the default
                    break;

                // ::CLASS name SUBCLASS sub
                case SUBDIRECTIVE_SUBCLASS:
                    // If we have a subclass set already, this is an error
                    if (activeClass->getSubClass() != OREF_NULL)
                    {
                        syntaxError(Error_Invalid_subkeyword_class, token);
                    }

                    // the subclass must be a symbol or string
                    token = nextReal();
                    if (!token->isSymbolOrLiteral())
                    {
                        syntaxError(Error_Symbol_or_string_subclass);
                    }
                    // set the subclass
                    activeClass->setSubClass(commonString(token->upperValue()));
                    break;

                // ::CLASS name MIXINCLASS mclass
                case SUBDIRECTIVE_MIXINCLASS:
                    // If we have a subclass set already, this is an error
                    // NOTE:  setting a mixin class also defines the subclass...
                    if (activeClass->getSubClass() != OREF_NULL)
                    {
                        syntaxError(Error_Invalid_subkeyword_class, token);
                    }
                    token = nextReal();
                    if (!token->isSymbolOrLiteral())
                    {
                        syntaxError(Error_Symbol_or_string_mixinclass);
                    }

                    // set the subclass information      */
                    activeClass->setMixinClass(commonString(token->upperValue()));
                    break;

                // ::CLASS name INHERIT classes
                case SUBDIRECTIVE_INHERIT:
                    // all tokens after the keyword will be consumed by the INHERIT keyword.
                    token = nextReal();
                    if (token->isEndOfClause())
                    {
                        syntaxError(Error_Symbol_or_string_inherit, token);
                    }

                    while (!token->isEndOfClause())
                    {
                        // must be a symbol or string        */
                        if (!token->isSymbolOrLiteral())
                        {
                            syntaxError(Error_Symbol_or_string_inherit, token);
                        }
                        // add to the inherit list
                        activeClass->addInherits(commonString(token->upperValue()));
                        token = nextReal();
                    }
                    // step back a token for final completion checks
                    previousToken();
                    break;

                // invalid keyword
                default:
                    syntaxError(Error_Invalid_subkeyword_class, token);
                    break;
            }
        }
    }
}


/**
 * check for a duplicate method.
 *
 * @param name   The name to check.
 * @param classMethod
 *               Indicates whether this is a check for a CLASS or INSTANCE method.
 * @param errorMsg
 *               The error code to use if there is a duplicate.
 */
void LanguageParser::checkDuplicateMethod(RexxString *name, bool classMethod, int errorMsg)
{
    // no previous ::CLASS directive?
    if (activeClass == OREF_NULL)
    {
        // cannot create unattached class methods.
        if (classMethod)
        {
            syntaxError(Error_Translation_missing_class);
        }
        // duplicate method name?
        if (unattachedMethods->entry(name) != OREF_NULL)
        {
            syntaxError(errorMsg);
        }
    }
    else
    {                                /* add the method to the active class*/
        // adding the method to the active class
        if (activeClass->checkDuplicateMethod(name, classMethod))
        {
            syntaxError(errorMsg);
        }
    }
}


/**
 * Add a new method to this compilation.
 *
 * @param name   The directory name of the method.
 * @param method The method object.
 * @param classMethod
 *               The class/instance method indicator.
 */
void LanguageParser::addMethod(RexxString *name, MethodClass *method, bool classMethod)
{
    // if no active class yet, these are unattached methods.
    if (activeClass == OREF_NULL)
    {
        unattachedMethods->setEntry(name, method);
    }
    else
    {
        activeClass->addMethod(name, method, classMethod);
    }
}


/**
 * Process a ::METHOD directive in a source file.
 */
void LanguageParser::methodDirective()
{
    // set default modifiers
    AccessFlag accessFlag = DEFAULT_ACCESS_SCOPE;
    ProtectedFlag protectedFlag = DEFAULT_PROTECTION;
    GuardFlag guardFlag = DEFAULT_GUARD;
    // other attributes of methods
    bool isClass = false;
    bool isAttribute = false;
    bool isAbstract = false;
    RexxString *externalname = OREF_NULL;       // not an external method yet

    // method name must be a symbol or string
    RexxToken *token = nextReal();
    if (!token->isSymbolOrLiteral())
    {
        syntaxError(Error_Symbol_or_string_method, token);
    }
    // this is the method internal name
    RexxString *name = token->value();
    // this is the look up name
    RexxString *internalname = commonString(name->upper());

    // now process any additional option keywords
    for (;;)
    {
        // finished on EOC
        token = nextReal();
        if (token->isEndOfClause())
        {
            break;
        }
        // option keywords must be symbols
        else if (!token->isSymbol())
        {
            syntaxError(Error_Invalid_subkeyword_method, token);
        }
        else
        {
            // potential option keyword
            switch (token->subDirective())
            {
                // ::METHOD name CLASS
                case SUBDIRECTIVE_CLASS:
                    // no dups
                    if (isClass)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }
                    isClass = true;
                    break;

                // ::METHOD name EXTERNAL extname
                case SUBDIRECTIVE_EXTERNAL:
                    // no dup on external and abstract is mutually exclusive
                    if (externalname != OREF_NULL || isAbstract)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }
                    token = nextReal();
                    // external name must be a string value
                    if (!token->isLiteral())
                    {
                        syntaxError(Error_Symbol_or_string_external, token);
                    }
                    // this will be parsed at install time
                    externalname = token->value();
                    break;

                // ::METHOD name PRIVATE
                case SUBDIRECTIVE_PRIVATE:
                    // has an access flag already been specified?
                    if (accessFlag != DEFAULT_ACCESS_SCOPE)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }
                    accessFlag = PRIVATE_SCOPE;
                    break;

                // ::METHOD name PUBLIC
                case SUBDIRECTIVE_PUBLIC:
                    // has an access flag already been specified?
                    if (accessFlag != DEFAULT_ACCESS_SCOPE)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }

                    accessFlag = PUBLIC_SCOPE;
                    break;

                // ::METHOD name PROTECTED
                case SUBDIRECTIVE_PROTECTED:
                    // had a protection flag specified already?
                    if (protectedFlag != DEFAULT_PROTECTION)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }
                    protectedFlag = PROTECTED_METHOD;
                    break;

                // ::METHOD name UNPROTECTED
                case SUBDIRECTIVE_UNPROTECTED:
                    // had a protection flag specified already?
                    if (protectedFlag != DEFAULT_PROTECTION)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }
                    protectedFlag = UNPROTECTED_METHOD;
                    break;

                // ::METHOD name UNGUARDED
                case SUBDIRECTIVE_UNGUARDED:
                    // already had a guard specification?
                    if (guardFlag != DEFAULT_GUARD)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }

                    guardFlag = UNGUARDED_METHOD;
                    break;

                // ::METHOD name GUARDED
                case SUBDIRECTIVE_GUARDED:
                    // already had a guard specification?
                    if (guardFlag != DEFAULT_GUARD)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }

                    guardFlag = GUARDED_METHOD;
                    break;

                // ::METHOD name ATTRIBUTE
                case SUBDIRECTIVE_ATTRIBUTE:
                    // check for duplicates
                    if (isAttribute)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }

                    // cannot have an abstract attribute here...this can
                    // be defined with ::ATTRIBUTE.
                    if (isAbstract)
                    {
                        // ABSTRACT and ATTRIBUTE are mutually exclusive
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }
                    isAttribute = true;
                    break;

                // ::METHOD name ABSTRACT
                case SUBDIRECTIVE_ABSTRACT:
                    // can't have dups or external name or be an attributed
                    if (isAbstract || externalname != OREF_NULL || isAttribute)
                    {
                        syntaxError(Error_Invalid_subkeyword_method, token);
                    }

                    isAbstract = true;
                    break;

                // something invalid
                default:
                    syntaxError(Error_Invalid_subkeyword_method, token);
                    break;
            }
        }
    }

    // go check for a duplicate and validate the use of the CLASS modifier
    checkDuplicateMethod(internalname, isClass, Error_Translation_duplicate_method);

    MethodClass *_method = OREF_NULL;
    // is this an attribute method?
    if (isAttribute)
    {
        // now get this as the setter method.
        RexxString *setterName = commonString(internalname->concatWithCstring("="));
        // need to check for duplicates on that too
        checkDuplicateMethod(setterName, Class, Error_Translation_duplicate_method);

        // cannot have code following an method with the attribute keyword
        checkDirective(Error_Translation_attribute_method);
        // this might be externally defined setters and getters.
        if (externalname != OREF_NULL)
        {
            RexxString *library = OREF_NULL;
            RexxString *procedure = OREF_NULL;
            decodeExternalMethod(internalname, externalname, library, procedure);
            // now create both getter and setting methods from the information.
            _method = createNativeMethod(internalname, library, procedure->concatToCstring("GET"));
            _method->setAttributes(accessFlag == PRIVATE_SCOPE, protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
            // add to the compilation
            addMethod(internalname, _method, isClass);

            _method = createNativeMethod(setterName, library, procedure->concatToCstring("SET"));
            _method->setAttributes(accessFlag == PRIVATE_SCOPE, protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
            // add to the compilation
            addMethod(setterName, _method, isClass);
        }
        else
        {
            // now get a variable retriever to get the property
            RexxVariableBase *retriever = getRetriever(name);

            // create the method pair and quit.
            createAttributeGetterMethod(internalname, retriever, isClass, accessFlag == PRIVATE_SCOPE,
                protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
            createAttributeSetterMethod(setterName, retriever, isClass, accessFlag == PRIVATE_SCOPE,
                protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
        }
        return;
    }
    // abstract method?
    else if (isAbstract)
    {
        // check that there is no code following this method.
        checkDirective(Error_Translation_abstract_method);
        // this uses a special code block
        BaseCode *code = new AbstractCode();
        _method = new MethodClass(name, code);
    }
    // regular Rexx code method?
    else if (externalname == OREF_NULL)
    {
        // NOTE:  It is necessary to translate the block and protect the code
        // before allocating the MethodClass object.  The new operator allocates the
        // the object first, then evaluates the constructor arguments after the allocation.
        // Since the translateBlock() call will allocate a lot of new objects before returning,
        // there's a high probability that the method object can get garbage collected before
        // there is any opportunity to protect the object.
        RexxCode *code = translateBlock(OREF_NULL);
        ProtectedObject p(code);

        // go do the next block of code
        _method = new MethodClass(name, code);
    }
    // external method
    else
    {
        RexxString *library = OREF_NULL;
        RexxString *procedure = OREF_NULL;
        decodeExternalMethod(internalname, externalname, library, procedure);

        // check that there this is only followed by other directives.
        checkDirective(Error_Translation_external_method);
        // and make this into a method object.
        _method = createNativeMethod(name, library, procedure);
    }
    _method->setAttributes(accessFlag == PRIVATE_SCOPE, protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
    // add to the compilation
    addMethod(internalname, _method, isClass);
}


/**
 * Process a ::OPTIONS directive in a source file.
 */
void LanguageParser::optionsDirective()
{
    // all options are of a keyword/value pattern
    for (;;)
    {

        RexxToken *token = nextReal();
        // finish up if we've hit EOC
        if (token->isEndOfClause())
        {
            break;
        }
        // non-symbol is an error
        else if (!token->isSymbol())
        {
            syntaxError(Error_Invalid_subkeyword_options, token);
        }
        else
        {
            // potential options keyword
            switch (token->subDirective(token))
            {
                // ::OPTIONS DIGITS nnnn
                case SUBDIRECTIVE_DIGITS:
                {
                    token = nextReal();
                    // we'll accept this as a symbol or a string...as long as it's a number
                    if (!token->isSymbolOrLiteral())
                    {
                        syntaxError(Error_Symbol_or_string_digits_value, token);
                    }

                    size_t digits;

                    // convert to a binary number
                    if (!token->value()->requestUnsignedNumber(digits, number_digits()) || digits < 1)
                    {
                        syntaxError(Error_Invalid_whole_number_digits, value);
                    }
                    // problem with the fuzz setting?
                    if (digits <= package->getFuzz())
                    {
                        reportException(Error_Expression_result_digits, digits, package->getFuzz());
                    }
                    // set this in the package object
                    package->setDigits(digits);
                    break;
                }
                // ::OPTIONS FORM ENGINEERING/SCIENTIFIC
                case SUBDIRECTIVE_FORM:
                {
                    token = nextReal();
                    if (!token->isSymbol())
                    {
                        syntaxError(Error_Invalid_subkeyword_form, token);
                    }

                    switch (token->subKeyword())
                    {
                        // FORM SCIENTIFIC
                        case SUBKEY_SCIENTIFIC:
                            package->setForm(Numerics::FORM_SCIENTIFIC);
                            break;

                        // FORM ENGINEERING
                        case SUBKEY_ENGINEERING:     /* NUMERIC FORM ENGINEERING          */
                            package->setForm(Numerics::FORM_ENGINEERING);
                            break;

                        // bad keyword
                        default:
                            syntaxError(Error_Invalid_subkeyword_form, token);
                            break;
                    }
                    break;
                }

                // ::OPTIONS FUZZ nnnn
                case SUBDIRECTIVE_FUZZ:
                {
                    token = nextReal();
                    if (!token->isSymbolOrLiteral())
                    {
                        syntaxError(Error_Symbol_or_string_fuzz_value, token);
                    }

                    RexxString *value = token->value();          /* get the string value              */

                    size_t fuzz;

                    if (!value->requestUnsignedNumber(fuzz, number_digits()))
                    {
                        syntaxError(Error_Invalid_whole_number_fuzz, value);
                    }
                    // validate with the digits setting
                    if (fuzz >= package->getDigits())
                    {
                        reportException(Error_Expression_result_digits, package->getDigits(), fuzz);
                    }
                    package->setFuzz(fuzz);
                    break;
                }

                // ::OPTIONS TRACE setting
                case SUBDIRECTIVE_TRACE:
                {
                    token = nextReal();
                    if (!token->isSymbolOrLiteral())
                    {
                        syntaxError(Error_Symbol_or_string_trace_value, token);
                    }

                    RexxString *value = token->value();
                    char badOption = 0;
                    size_t traceSetting;
                    size_t traceFlags;

                    // validate the setting
                    if (!parseTraceSetting(value, traceSetting, traceFlags, badOption))
                    {
                        syntaxError(Error_Invalid_trace_trace, new_string(&badOption, 1));
                    }
                    // poke into the package
                    package->setTraceSetting(traceSetting);
                    package->setTraceFlags(traceFlags);
                    break;
                }

                // invalid keyword
                default:
                    syntaxError(Error_Invalid_subkeyword_options, token);
                    break;
            }
        }
    }
}


/**
 * Create a native method from a specification.
 *
 * @param name      The method name.
 * @param library   The library containing the method.
 * @param procedure The name of the procedure within the package.
 *
 * @return A method object representing this method.
 */
MethodClass *LanguageParser::createNativeMethod(RexxString *name, RexxString *library, RexxString *procedure)
{
    RexxNativeCode *nmethod = PackageManager::resolveMethod(library, procedure);
    // raise an exception if this entry point is not found.
    if (nmethod == OREF_NULL)
    {
         syntaxError(Error_External_name_not_found_method, procedure);
    }
    // this might return a different object if this has been used already
    nmethod = (RexxNativeCode *)nmethod->setSourceObject(package);
    // turn into a real method object
    return new MethodClass(name, nmethod);
}


/**
 * Decode an external library method specification.
 *
 * @param methodName The internal name of the method (uppercased).
 * @param externalSpec
 *                   The external specification string.
 * @param library    The returned library name.
 * @param procedure  The returned package name.
 */
void LanguageParser::decodeExternalMethod(RexxString *methodName, RexxString *externalSpec, RexxString *&library, RexxString *&procedure)
{
    // this is the default
    procedure = methodName;
    library = OREF_NULL;

    // convert into an array of words
    // NOTE:  This method makes all of the words part of the
    // common string pool
    RexxArray *_words = words(externalSpec);
    // not 'LIBRARY library [entry]' form?
    if (((RexxString *)(_words->get(1)))->strCompare(CHAR_LIBRARY))
    {
        // full library with entry name version?
        if (_words->size() == 3)
        {
            library = (RexxString *)_words->get(2);
            procedure = (RexxString *)_words->get(3);

        }
        else if (_words->size() == 2)
        {
            library = (RexxString *)_words->get(2);
        }
        else  // wrong number of tokens
        {
            syntaxError(Error_Translation_bad_external, externalSpec);
        }
    }
    else
    {
        syntaxError(Error_Translation_bad_external, externalSpec);
    }
}

enum
{
    ATTRIBUTE_BOTH,
    ATTRIBUTE_GET,
    ATTRIBUTE_SET,
} AttributeType;


/**
 * Process a ::ATTRIBUTE directive in a source file.
 */
void LanguageParser::attributeDirective()
{
    // set the default attributes
    AccessFlag accessFlag = DEFAULT_ACCESS_SCOPE;
    ProtectedFlag  protectedFlag = DEFAULT_PROTECTION;
    GuardFlag guardFlag = DEFAULT_GUARD;
    // by default, we create both methods for the attribute.
    AttributeType style = ATTRIBUTE_BOTH;
    bool isClass = false;            // default is an instance method
    bool isAbstract = false;         // by default, creating a concrete method

    RexxToken *token = nextReal();
    // the name must be a string or a symbol
    if (!token->isSymbolOrLiteral())
    {
        syntaxError(Error_Symbol_or_string_attribute, token);
    }

    // get the attribute name and the internal value that we create the method names with
    RexxString *name = token->value();
    RexxString *internalname = commonString(name->upper());
    RexxString *externalname = OREF_NULL;

    // process options
    for (;;)
    {
        // if last token we're out of here.
        token = nextReal();
        if (token->isEndOfClause())
        {
            break;
        }
        // all options must be symbols
        else if (!token->isSymbol())
        {
            syntaxError(Error_Invalid_subkeyword_attribute, token);
        }
        else
        {
            switch (token->subDirective())
            {
                // GET define this as a attribute get method
                case SUBDIRECTIVE_GET:
                    // only one per customer
                    if (style != ATTRIBUTE_BOTH)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    style = ATTRIBUTE_GET;
                    break;

                // SET create an attribute assignment method
                case SUBDIRECTIVE_SET:
                    // only one of GET/SET allowed
                    if (style != ATTRIBUTE_BOTH)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    style = ATTRIBUTE_SET;
                    break;


                // ::ATTRIBUTE name CLASS  creating class methods
                case SUBDIRECTIVE_CLASS:
                    // no dups allowed
                    if (isClass)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    isClass = true;
                    break;

                // private access?
                case SUBDIRECTIVE_PRIVATE:
                    // can have just one of PUBLIC or PRIVATE
                    if (accessFlag != DEFAULT_ACCESS_SCOPE)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    accessFlag = PRIVATE_SCOPE;
                    break;

                // define with public access (the default)
                case SUBDIRECTIVE_PUBLIC:
                    // must be first access specifier
                    if (accessFlag != DEFAULT_ACCESS_SCOPE)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    accessFlag = PUBLIC_SCOPE;
                    break;

                // ::METHOD name PROTECTED
                case SUBDIRECTIVE_PROTECTED:
                    // only one of PROTECTED UNPROTECTED
                    if (protectedFlag != DEFAULT_PROTECTION)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    protectedFlag = PROTECTED_METHOD;
                    break;

                // unprotected method (the default)
                case SUBDIRECTIVE_UNPROTECTED:
                    // only one of PROTECTED UNPROTECTED
                    if (protectedFlag != DEFAULT_PROTECTION)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    protectedFlag = UNPROTECTED_METHOD;
                    break;

                // unguarded access to an object
                case SUBDIRECTIVE_UNGUARDED:
                    if (guardFlag != DEFAULT_GUARD)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    guardFlag = UNGUARDED_METHOD;
                    break;

                // guarded access to a method (the default)
                case SUBDIRECTIVE_GUARDED:
                    if (guardFlag != DEFAULT_GUARD)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    guardFlag = GUARDED_METHOD;
                    break;

                // external attributes?
                case SUBDIRECTIVE_EXTERNAL:
                    // can't be abstract and external
                    if (externalname != OREF_NULL || isAbstract)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }

                    // the external specifier must be a string
                    token = nextReal();
                    if (!token->isLiteral())
                    {
                        syntaxError(Error_Symbol_or_string_external, token);
                    }

                    externalname = token->value();
                    break;

                // abstract method
                case SUBDIRECTIVE_ABSTRACT:
                    // abstract and external conflict
                    if (isAbstract || externalname != OREF_NULL)
                    {
                        syntaxError(Error_Invalid_subkeyword_attribute, token);
                    }
                    isAbstract = true;
                    break;

                // some invalid keyword
                default:
                    syntaxError(Error_Invalid_subkeyword_attribute, token);
                    break;
            }
        }
    }

    // both attributes same default properties?

    // now get a variable retriever to get the property (do this before checking the body
    // so errors get diagnosed on the correct line),
    RexxVariableBase *retriever = getRetriever(name);

    switch (style)
    {
        // creating both a setter and getter.  These cannot have
        // following bodies
        case ATTRIBUTE_BOTH:
        {
            checkDuplicateMethod(internalname, isClass, Error_Translation_duplicate_attribute);
            // now get this as the setter method.
            RexxString *setterName = commonString(internalname->concatWithCstring("="));
            checkDuplicateMethod(setterName, isClass, Error_Translation_duplicate_attribute);

            // no code can follow the automatically generated methods
            checkDirective(Error_Translation_body_error);
            if (externalname != OREF_NULL)
            {
                RexxString *library = OREF_NULL;
                RexxString *procedure = OREF_NULL;
                decodeExternalMethod(internalname, externalname, library, procedure);
                // now create both getter and setting methods from the information.
                MethodClass *_method = createNativeMethod(internalname, library, procedure->concatToCstring("GET"));
                _method->setAttributes(accessFlag == PRIVATE_SCOPE, protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                // add to the compilation
                addMethod(internalname, _method, Class);

                _method = createNativeMethod(setterName, library, procedure->concatToCstring("SET"));
                _method->setAttributes(accessFlag == PRIVATE_SCOPE, protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                // add to the compilation
                addMethod(setterName, _method, Class);
            }
            // abstract method?
            else if (isAbstract)
            {
                // create the method pair and quit.
                createAbstractMethod(internalname, isClass, accessFlag == PRIVATE_SCOPE,
                    protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                createAbstractMethod(setterName, isClass, accessFlag == PRIVATE_SCOPE,
                    protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
            }
            else
            {
                // create the method pair and quit.
                createAttributeGetterMethod(internalname, retriever, isClass, accessFlag == PRIVATE_SCOPE,
                    protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                createAttributeSetterMethod(setterName, retriever, Class, Private == PRIVATE_SCOPE,
                    protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
            }
            break;

        }
        // just need a getter method
        case ATTRIBUTE_GET:
        {
            checkDuplicateMethod(internalname, isClass, Error_Translation_duplicate_attribute);
            // external?  resolve the method
            if (externalname != OREF_NULL)
            {
                // no code can follow external methods
                checkDirective(Error_Translation_external_attribute);
                RexxString *library = OREF_NULL;
                RexxString *procedure = OREF_NULL;
                decodeExternalMethod(internalname, externalname, library, procedure);
                // if there was no procedure explicitly given, create one using the GET/SET convention
                if (internalname == procedure)
                {
                    procedure = procedure->concatToCstring("GET");
                }
                // now create both getter and setting methods from the information.
                MethodClass *_method = createNativeMethod(internalname, library, procedure);
                _method->setAttributes(accessFlag == PRIVATE_SCOPE, protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                // add to the compilation
                addMethod(internalname, _method, Class);
            }
            // abstract method?
            else if (isAbstract)
            {
                // no code can follow abstract methods
                checkDirective(Error_Translation_abstract_attribute);
                // create the method pair and quit.
                createAbstractMethod(internalname, isClass, accessFlag == PRIVATE_SCOPE,
                    protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
            }
            // either written in ooRexx or is automatically generated.
            else
            {
                // written in Rexx?  go create
                if (hasBody())
                {
                    createMethod(internalname, isClass, accessFlags == PRIVATE_SCOPE,
                        protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                }
                else
                {
                    createAttributeGetterMethod(internalname, retriever, isClass, accessFlags == PRIVATE_SCOPE,
                        protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                }
            }
            break;
        }

        // just a setter method
        case ATTRIBUTE_SET:
        {
            // now get this as the setter method.
            RexxString *setterName = commonString(internalname->concatWithCstring("="));
            checkDuplicateMethod(setterName, isClass, Error_Translation_duplicate_attribute);
            // external?  resolve the method
            if (externalname != OREF_NULL)
            {
                // no code can follow external methods
                checkDirective(Error_Translation_external_attribute);
                RexxString *library = OREF_NULL;
                RexxString *procedure = OREF_NULL;
                decodeExternalMethod(internalname, externalname, library, procedure);
                // if there was no procedure explicitly given, create one using the GET/SET convention
                if (internalname == procedure)
                {
                    procedure = procedure->concatToCstring("SET");
                }
                // now create both getter and setting methods from the information.
                MethodClass *_method = createNativeMethod(setterName, library, procedure);
                _method->setAttributes(accessFlag == PRIVATE_SCOPE, protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                // add to the compilation
                addMethod(setterName, _method, isClass);
            }
            // abstract method?
            else if (isAbstract)
            {
                // no code can follow abstract methods
                checkDirective(Error_Translation_abstract_attribute);
                // create the method pair and quit.
                createAbstractMethod(setterName, isClass, accessFlag == PRIVATE_SCOPE,
                    protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
            }
            else
            {
                if (hasBody())        // just the getter method
                {
                    createMethod(setterName, isClass, accessFlag == PRIVATE_SCOPE,
                        protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                }
                else
                {
                    createAttributeSetterMethod(setterName, retriever, isClass, accessFlag == PRIVATE_SCOPE,
                        protectedFlag == PROTECTED_METHOD, guardFlag != UNGUARDED_METHOD);
                }
            }
            break;
        }
    }
}


/**
 * Process a ::CONSTANT directive in a source file.
 */
void LanguageParser::constantDirective()
{

    RexxToken *token = nextReal();
    if (!token->isSymbolOrLiteral())
    {
        syntaxError(Error_Symbol_or_string_constant, token);
    }

    // get the expressed name and the name we use for the methods
    RexxString *name = token->value();
    RexxString *internalname = commonString(name->upper());

    // we only expect just a single value token here
    token = nextReal();
    RexxObject *value;

    // the value omitted?  Just use the literal name of the constant.
    if (token->isEndOfClause())
    {
        value = name;
        // push the EOC token back
        previousToken();
    }
    // no a symbol or literal...we have special checks for signed numbers
    else if (!token->isSymbolOrLiteral())
    {
        // if not a "+" or "-" operator, this is an error
        if (!token->isOperator() || (!token->isSubtype(OPERATOR_SUBTRACT, OPERATOR_PLUS)))
        {
            syntaxError(Error_Symbol_or_string_constant_value, token);
        }
        RexxToken *second = nextReal();
        // this needs to be a constant symbol...we check for
        // numeric below
        if (!second->isSymbol() || !second->isSubtype(SYMBOL_CONSTANT))
        {
            syntaxError(Error_Symbol_or_string_constant_value, token);
        }
        // concat with the sign operator
        value = token->value()->concat(second->value());
        // and validate that this a valid number
        if (value->numberString() == OREF_NULL)
        {
            syntaxError(Error_Symbol_or_string_constant_value, token);
        }
    }
    else
    {
        // this will be some sort of literal value
        value = token->value;
    }

    token = nextReal();
    // No other options on this instruction
    if (!token->isEndOfClause())
    {
        /* report an error                   */
        syntaxError(Error_Invalid_data_constant_dir, token);
    }
    // this directive does not allow a body
    checkDirective(Error_Translation_constant_body);

    // check for duplicates.  We only do the class duplicate check if there
    // is an active class, otherwise we'll get a syntax error
    checkDuplicateMethod(internalname, false, Error_Translation_duplicate_constant);
    if (activeClass != OREF_NULL)
    {
        checkDuplicateMethod(internalname, true, Error_Translation_duplicate_constant);
    }

    // create the method pair and quit.
    createConstantGetterMethod(internalname, value);
}


/**
 * Create a Rexx method body.
 *
 * @param name   The name of the attribute.
 * @param classMethod
 *               Indicates whether we are creating a class or instance method.
 * @param privateMethod
 *               The method's private attribute.
 * @param protectedMethod
 *               The method's protected attribute.
 * @param guardedMethod
 *               The method's guarded attribute.
 */
void LanguageParser::createMethod(RexxString *name, bool classMethod,
    bool privateMethod, bool protectedMethod, bool guardedMethod)
{
    // NOTE:  It is necessary to translate the block and protect the code
    // before allocating the MethodClass object.  The new operator allocates the
    // the object first, then evaluates the constructor arguments after the allocation.
    // Since the translateBlock() call will allocate a lot of new objects before returning,
    // there's a high probability that the method object can get garbage collected before
    // there is any opportunity to protect the object.
    RexxCode *code = translateBlock(OREF_NULL);
    ProtectedObject p(code);

    // convert into a method object
    MethodClass *_method = new MethodClass(name, code);
    _method->setAttributes(privateMethod, protectedMethod, guardedMethod);
    // go add the method to the accumulator
    addMethod(name, _method, classMethod);
}


/**
 * Create an ATTRIBUTE "get" method.
 *
 * @param name      The name of the attribute.
 * @param retriever
 * @param classMethod
 *                  Indicates we're adding a class or instance method.
 * @param privateMethod
 *                  The method's private attribute.
 * @param protectedMethod
 *                  The method's protected attribute.
 * @param guardedMethod
 *                  The method's guarded attribute.
 */
void LanguageParser::createAttributeGetterMethod(RexxString *name, RexxVariableBase *retriever,
    bool classMethod, bool privateMethod, bool protectedMethod, bool guardedMethod)
{
    // create the kernel method for the accessor
    BaseCode *code = new AttributeGetterCode(retriever);
    MethodClass *_method = new MethodClass(name, code);
    _method->setAttributes(privateMethod, protectedMethod, guardedMethod);
    // add this to the target
    addMethod(name, _method, classMethod);
}


/**
 * Create an ATTRIBUTE "set" method.
 *
 * @param name   The name of the attribute.
 * @param classMethod
 *                  Indicates we're adding a class or instance method.
 * @param privateMethod
 *               The method's private attribute.
 * @param protectedMethod
 *               The method's protected attribute.
 * @param guardedMethod
 *               The method's guarded attribute.
 */
void LanguageParser::createAttributeSetterMethod(RexxString *name, RexxVariableBase *retriever,
    bool classMethod, bool privateMethod, bool protectedMethod, bool guardedMethod)
{
    // create the kernel method for the accessor
    BaseCode *code = new AttributeSetterCode(retriever);
    MethodClass *_method = new MethodClass(name, code);
    _method->setAttributes(privateMethod, protectedMethod, guardedMethod);
    // add this to the target
    addMethod(name, _method, classMethod);
}


/**
 * Create an abstract method.
 *
 * @param name   The name of the method.
 * @param classMethod
 *                  Indicates we're adding a class or instance method.
 * @param privateMethod
 *               The method's private attribute.
 * @param protectedMethod
 *               The method's protected attribute.
 * @param guardedMethod
 *               The method's guarded attribute.
 */
void LanguageParser::createAbstractMethod(RexxString *name,
    bool classMethod, bool privateMethod, bool protectedMethod, bool guardedMethod)
{
    // create the kernel method for the accessor
    // this uses a special code block
    BaseCode *code = new AbstractCode();
    MethodClass * _method = new MethodClass(name, code);
    _method->setAttributes(privateMethod, protectedMethod, guardedMethod);
    // add this to the target
    addMethod(name, _method, classMethod);
}


/**
 * Create a CONSTANT "get" method.
 *
 * @param target The target method directory.
 * @param name   The name of the attribute.
 */
void LanguageParser::createConstantGetterMethod(RexxString *name, RexxObject *value)
{
    ConstantGetterCode *code = new ConstantGetterCode(value);
    // add this as an unguarded method
    MethodClass *method = new MethodClass(name, code);
    method->setUnguarded();
    if (activeClass == OREF_NULL)
    {
        addMethod(name, method, false);
    }
    else
    {
        activeClass->addConstantMethod(name, method);
    }
}


/**
 * Process a ::routine directive in a source file.
 */
void LanguageParser::routineDirective()
{
    // must start with a name token
    RexxToken *token = nextReal();

    if (!token->isSymbolOrLiteral())
    {
        syntaxError(Error_Symbol_or_string_routine, token);
    }

    // NOTE:  routine lookups are case sensitive if the name
    // is quoted, so we don't uppercase this name.
    RexxString *name = token->value();
    if (isDuplicateRoutine(name))
    {
        syntaxError(Error_Translation_duplicate_routine);
    }

    // setup option defaults and then process any remaining options.
    RexxString *externalname = OREF_NULL;
    AccessFlag accessFlag = DEFAULT_ACCESS_SCOPE;
    for (;;)
    {
        token = nextReal();
        // finished?
        if (token->isEndOfClause())
        {
            break;
        }
        // all options must be symbols
        else if (!token->isSymbol())
        {
            syntaxError(Error_Invalid_subkeyword_routine, token);
        }

        switch (token->subDirective())
        {
            // written in native code backed by an external library
            case SUBDIRECTIVE_EXTERNAL:
                if (externalname != OREF_NULL)
                {
                    syntaxError(Error_Invalid_subkeyword_class, token);
                }
                token = nextReal();
                // this is a compound string descriptor, so it must be a literal
                if (!token->isLiteral())
                {
                    syntaxError(Error_Symbol_or_string_requires, token);
                }

                externalname = token->value();
                break;

            // publicly available to programs that require this
            case SUBDIRECTIVE_PUBLIC:
                if (accessFlag != DEFAULT_ACCESS_SCOPE)
                {
                    syntaxError(Error_Invalid_subkeyword_routine, token);

                }
                accessFlag = PUBLIC_SCOPE;
                break;

            // this has private scope (the default)
            case SUBDIRECTIVE_PRIVATE:
                if (accessFlag != DEFAULT_ACCESS_SCOPE)
                {
                    syntaxError(Error_Invalid_subkeyword_routine, token);
                }
                accessFlag = PRIVATE_SCOPE;
                break;

            // something invalid
            default:
                syntaxError(Error_Invalid_subkeyword_routine, token);
                break;
        }
    }
    {
        // is this mapped to an external library?
        if (externalname != OREF_NULL)
        {
            // convert external into words (this also adds the strings
            // to the common string pool)
            RexxArray *_words = words(externalname);
            // ::ROUTINE foo EXTERNAL "LIBRARY libbar [foo]"
            // NOTE:  decodeMethodLibrary doesn't really work for routines
            // because we have a second form.  Not really worth writing
            // a second version just for one use.
            if (((RexxString *)(_words->get(1)))->strCompare(CHAR_LIBRARY))
            {
                RexxString *library = OREF_NULL;

                // the default entry point name is the internal name
                RexxString *entry = name;

                // full library with entry name version?
                if (_words->size() == 3)
                {
                    library = (RexxString *)_words->get(2);
                    entry = (RexxString *)_words->get(3);
                }
                else if (_words->size() == 2)
                {
                    library = (RexxString *)_words->get(2);
                }
                else  // wrong number of tokens
                {
                    syntaxError(Error_Translation_bad_external, externalname);
                }

                // make sure this is followed by a directive or end of file
                checkDirective(Error_Translation_external_routine);
                // create a new native method.
                RoutineClass *routine = PackageManager::resolveRoutine(library, entry);
                // raise an exception if this entry point is not found.
                if (routine == OREF_NULL)
                {
                     syntaxError(Error_External_name_not_found_routine, entry);
                }
                // make sure this is attached to the source object for context information
                routine->setSourceObject(package);
                // add to the routine directory
                routines->setEntry(name, routine);
                // if this is a public routine, add to the public directory as well.
                if (accessFlag == PUBLIC_SCOPE)
                {
                    // add to the public directory too
                    publicRoutines->setEntry(name, routine);
                }
            }

            // ::ROUTINE foo EXTERNAL "REGISTERED libbar [foo]"
            // this is an old-style external function.
            else if (((RexxString *)(_words->get(1)))->strCompare(CHAR_REGISTERED))
            {
                RexxString *library = OREF_NULL;
                // the default entry point name is the internal name
                RexxString *entry = name;

                // full library with entry name version?
                if (_words->size() == 3)
                {
                    library = (RexxString *)_words->get(2);
                    entry = (RexxString *)_words->get(3);
                }
                else if (_words->size() == 2)
                {
                    library = (RexxString *)_words->get(2);
                }
                else  // wrong number of tokens
                {
                    syntaxError(Error_Translation_bad_external, externalname);
                }

                // go check the next clause to make
                checkDirective(Error_Translation_external_routine);
                // resolve the native routine
                RoutineClass *routine = PackageManager::resolveRoutine(name, library, entry);
                // raise an exception if this entry point is not found.
                if (routine == OREF_NULL)
                {
                     syntaxError(Error_External_name_not_found_routine, entry);
                }
                // make sure this is attached to the source object for context information
                routine->setSourceObject(package);
                // add to the routine directory
                routines->setEntry(name, routine);
                // if this is a public routine, add to the public directory as well.
                if (accessFlag == PUBLIC_SCOPE)
                {
                    // add to the public directory too
                    publicRoutines->setEntry(name, routine);
                }
            }
            else
            {
                // unknown external type
                syntaxError(Error_Translation_bad_external, externalname);
            }
        }
        else
        {
            // NOTE:  It is necessary to translate the block and protect the code
            // before allocating the MethodClass object.  The new operator allocates the
            // the object first, then evaluates the constructor arguments after the allocation.
            // Since the translateBlock() call will allocate a lot of new objects before returning,
            // there's a high probability that the method object can get garbage collected before
            // there is any opportunity to protect the object.
            RexxCode *code = this->translateBlock(OREF_NULL);
            ProtectedObject p(code);
            RoutineClass *routine = new RoutineClass(name, code);
            // add to the routine directory
            routines->setEntry(name, routine);
            // if this is a public routine, add to the public directory as well.
            if (accessFlag == PUBLIC_SCOPE)
            {
                // add to the public directory too
                publicRoutines->setEntry(name, routine);
            }
        }
    }
}


/**
 * Process a ::REQUIRES directive.
 */
void LanguageParser::requiresDirective()
{
    bool isLibrary = false;
    RexxString *label = OREF_NULL;

    // the required entity name is a string or symbol
    RexxToken *token = nextReal();
    if (!token->isSymbolOrLiteral())
    {
        syntaxError(Error_Symbol_or_string_requires, token);
    }
    // we have the name, and we have a couple options we can process.
    RexxString *name = token->value();
    for (;;)
    {
        token = nextReal();
        // finished?
        if (token->isEndOfClause())
        {
            break;
        }
        // all options must be symbols
        else if (!token->isSymbol())
        {
            syntaxError(Error_Invalid_subkeyword_requires, token);
        }

        switch (token->subDirective())
        {
            // this identifies a library
            case SUBDIRECTIVE_LIBRARY:
                // can only specify library once and for now, at least,
                // the LABEL keyword is not allowed on a LIBRARY requires.
                // this might have some meaning eventually for resolving
                // external routines, but for now, this is a restriction.
                if (isLibrary || label != OREF_NULL)
                {
                    syntaxError(Error_Invalid_subkeyword_requires, token);
                }
                isLibrary = true;
                break:

            case SUBDIRECTIVE_LABEL:
                // can only have one of these and cannot have this with the LIBRARY option
                if (isLibrary || label != OREF_NULL)
                {
                    syntaxError(Error_Invalid_subkeyword_requires, token);
                }
                // get the label token
                token = nextReal();
                if (!token->isSymbol())
                {
                    syntaxError(Error_Symbol_expected_LABEL);
                }
                // NOTE:  since this is a symbol, the label will be an
                // uppercase name.
                label = token->value();
                break:

            default:
                syntaxError(Error_Invalid_subkeyword_requires, token);
                break;
        }
    }

    // this is either a library directive or a requires directive
    if (isLibrary)
    {
        libraries->append((RexxObject *)new LibraryDirective(name, clause));
    }
    else
    {
        requires->append((RexxObject *)new RequiresDirective(name, label, clause));
    }
}



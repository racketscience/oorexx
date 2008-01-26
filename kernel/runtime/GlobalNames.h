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
/* REXX Kernel                                                  GlobalNames.h     */
/*                                                                            */
/* Definitions of all name objects created at startup time.  All these        */
/* Name objects are addressible via OREF_ global names                        */
/*                                                                            */
/******************************************************************************/

  GLOBAL_NAME(ADDCLASS, CHAR_ADDCLASS)
  GLOBAL_NAME(ADDITIONAL, CHAR_ADDITIONAL)
  GLOBAL_NAME(ADDRESS, CHAR_ADDRESS)
  GLOBAL_NAME(AND, CHAR_AND)
  GLOBAL_NAME(ASSIGNMENT_AND, CHAR_ASSIGNMENT_AND)
  GLOBAL_NAME(ANY, CHAR_ANY)
  GLOBAL_NAME(ARGUMENTS, CHAR_ARGUMENTS)
  GLOBAL_NAME(ARRAYSYM, CHAR_ARRAY)
  GLOBAL_NAME(BACKSLASH, CHAR_BACKSLASH)
  GLOBAL_NAME(BACKSLASH_EQUAL, CHAR_BACKSLASH_EQUAL)
  GLOBAL_NAME(BACKSLASH_GREATERTHAN, CHAR_BACKSLASH_GREATERTHAN)
  GLOBAL_NAME(BACKSLASH_LESSTHAN, CHAR_BACKSLASH_LESSTHAN)
  GLOBAL_NAME(BLANK, CHAR_BLANK)
  GLOBAL_NAME(BRACKETS, CHAR_BRACKETS)
  GLOBAL_NAME(CALL, CHAR_CALL)
  GLOBAL_NAME(CHARIN, CHAR_CHARIN)
  GLOBAL_NAME(CHAROUT, CHAR_CHAROUT)
  GLOBAL_NAME(CHARS, CHAR_CHARS)
  GLOBAL_NAME(CLASSSYM, CHAR_CLASS)
  GLOBAL_NAME(CLOSE, CHAR_CLOSE)
  GLOBAL_NAME(CODE, CHAR_CODE)
  GLOBAL_NAME(COMMAND, CHAR_COMMAND)
  GLOBAL_NAME(COMPARE, CHAR_COMPARE)
  GLOBAL_NAME(COMPARETO, CHAR_COMPARETO)
  GLOBAL_NAME(CONCATENATE, CHAR_CONCATENATE)
  GLOBAL_NAME(ASSIGNMENT_CONCATENATE, CHAR_ASSIGNMENT_CONCATENATE)
  GLOBAL_NAME(CONDITION, CHAR_CONDITION)
  GLOBAL_NAME(CSELF, CHAR_CSELF)
  GLOBAL_NAME(DEFAULTNAME, CHAR_DEFAULTNAME)
  GLOBAL_NAME(DELAY, CHAR_DELAY)
  GLOBAL_NAME(DESCRIPTION, CHAR_DESCRIPTION)
  GLOBAL_NAME(DIVIDE, CHAR_DIVIDE)
  GLOBAL_NAME(ASSIGNMENT_DIVIDE, CHAR_ASSIGNMENT_DIVIDE)
  GLOBAL_NAME(ENGINEERING, CHAR_ENGINEERING)
  GLOBAL_NAME(ENVIRONMENT, CHAR_ENVIRONMENT)
  GLOBAL_NAME(EQUAL, CHAR_EQUAL)
  GLOBAL_NAME(ERRORNAME, CHAR_ERROR)
  GLOBAL_NAME(ERRORTEXT, CHAR_ERRORTEXT)
  GLOBAL_NAME(FAILURENAME, CHAR_FAILURENAME)
  GLOBAL_NAME(FILE, CHAR_FILE)
  GLOBAL_NAME(FILESYSTEM, CHAR_FILESYSTEM)
  GLOBAL_NAME(FUNCTIONNAME, CHAR_FUNCTIONNAME)
  GLOBAL_NAME(GET, CHAR_GET)
  GLOBAL_NAME(GREATERTHAN, CHAR_GREATERTHAN)
  GLOBAL_NAME(GREATERTHAN_EQUAL, CHAR_GREATERTHAN_EQUAL)
  GLOBAL_NAME(GREATERTHAN_LESSTHAN, CHAR_GREATERTHAN_LESSTHAN)
  GLOBAL_NAME(HALT, CHAR_HALT)
  GLOBAL_NAME(HASMETHOD, CHAR_HASMETHOD)
  GLOBAL_NAME(HASHCODE, CHAR_HASHCODE)
  GLOBAL_NAME(INHERIT, CHAR_INHERIT)
  GLOBAL_NAME(INIT, CHAR_INIT)
  GLOBAL_NAME(INITIALADDRESS, CHAR_INITIALADDRESS)
  GLOBAL_NAME(INPUT, CHAR_INPUT)
  GLOBAL_NAME(INSERT, CHAR_INSERT)
  GLOBAL_NAME(INSTRUCTION, CHAR_INSTRUCTION)
  GLOBAL_NAME(INTDIV, CHAR_INTDIV)
  GLOBAL_NAME(ASSIGNMENT_INTDIV, CHAR_ASSIGNMENT_INTDIV)
  GLOBAL_NAME(LESSTHAN, CHAR_LESSTHAN)
  GLOBAL_NAME(LESSTHAN_EQUAL, CHAR_LESSTHAN_EQUAL)
  GLOBAL_NAME(LESSTHAN_GREATERTHAN, CHAR_LESSTHAN_GREATERTHAN)
  GLOBAL_NAME(LINEIN, CHAR_LINEIN)
  GLOBAL_NAME(LINEOUT, CHAR_LINEOUT)
  GLOBAL_NAME(LINES, CHAR_LINES)
  GLOBAL_NAME(LOCAL, CHAR_LOCAL)
  GLOBAL_NAME(LOSTDIGITS, CHAR_LOSTDIGITS)
  GLOBAL_NAME(MAKEARRAY, CHAR_MAKEARRAY)
  GLOBAL_NAME(MAKESTRING, CHAR_MAKESTRING)
  GLOBAL_NAME(METHODNAME, CHAR_METHODNAME)
  GLOBAL_NAME(METHODS, CHAR_METHODS)
  GLOBAL_NAME(MULTIPLY, CHAR_MULTIPLY)
  GLOBAL_NAME(ASSIGNMENT_MULTIPLY, CHAR_ASSIGNMENT_MULTIPLY)
  GLOBAL_NAME(NAME, CHAR_NAME)
  GLOBAL_NAME(NAME_MESSAGE, CHAR_MESSAGE)
  GLOBAL_NAME(NEW, CHAR_NEW)
  GLOBAL_NAME(NOMETHOD, CHAR_NOMETHOD)
  GLOBAL_NAME(NONE, CHAR_NONE)
  GLOBAL_NAME(NORMAL, CHAR_NORMAL)
  GLOBAL_NAME(NOSTRING, CHAR_NOSTRING)
  GLOBAL_NAME(NOVALUE, CHAR_NOVALUE)
  GLOBAL_NAME(NULLSTRING, CHAR_NULLSTRING)
  GLOBAL_NAME(OBJECTSYM, CHAR_OBJECT)
  GLOBAL_NAME(OBJECTNAME, CHAR_OBJECTNAME)
  GLOBAL_NAME(OFF, CHAR_OFF)
  GLOBAL_NAME(ON, CHAR_ON)
  GLOBAL_NAME(OR, CHAR_OR)
  GLOBAL_NAME(ASSIGNMENT_OR, CHAR_ASSIGNMENT_OR)
  GLOBAL_NAME(OUTPUT, CHAR_OUTPUT)
  GLOBAL_NAME(PERIOD, CHAR_PERIOD)
  GLOBAL_NAME(PLUS, CHAR_PLUS)
  GLOBAL_NAME(ASSIGNMENT_PLUS, CHAR_ASSIGNMENT_PLUS)
  GLOBAL_NAME(POSITION, CHAR_POSITION)
  GLOBAL_NAME(POWER, CHAR_POWER)
  GLOBAL_NAME(ASSIGNMENT_POWER, CHAR_ASSIGNMENT_POWER)
  GLOBAL_NAME(PROGRAM, CHAR_PROGRAM)
  GLOBAL_NAME(PROPAGATE, CHAR_PROPAGATE)
  GLOBAL_NAME(PROPAGATED, CHAR_PROPAGATED)
  GLOBAL_NAME(PULL, CHAR_PULL)
  GLOBAL_NAME(PUSH, CHAR_PUSH)
  GLOBAL_NAME(PUT, CHAR_PUT)
  GLOBAL_NAME(QUEUED, CHAR_QUEUED)
  GLOBAL_NAME(QUEUENAME, CHAR_QUEUE)
  GLOBAL_NAME(QUERY, CHAR_QUERY)
  GLOBAL_NAME(RC, CHAR_RC)
  GLOBAL_NAME(REMAINDER, CHAR_REMAINDER)
  GLOBAL_NAME(ASSIGNMENT_REMAINDER, CHAR_ASSIGNMENT_REMAINDER)
  GLOBAL_NAME(REQUEST, CHAR_REQUEST)
  GLOBAL_NAME(REQUIRES, CHAR_REQUIRES)
  GLOBAL_NAME(RESULT, CHAR_RESULT)
  GLOBAL_NAME(REXXQUEUE, CHAR_REXXQUEUE)
  GLOBAL_NAME(ROUTINENAME, CHAR_ROUTINENAME)
  GLOBAL_NAME(RUN, CHAR_RUN)
  GLOBAL_NAME(SAY, CHAR_SAY)
  GLOBAL_NAME(SCIENTIFIC, CHAR_SCIENTIFIC)
  GLOBAL_NAME(SCRIPT, CHAR_SCRIPT)
  GLOBAL_NAME(SELF, CHAR_SELF)
  GLOBAL_NAME(SEND, CHAR_SEND)
  GLOBAL_NAME(SERVER, CHAR_SERVER)
  GLOBAL_NAME(SESSION, CHAR_SESSION)
  GLOBAL_NAME(SET, CHAR_SET)
  GLOBAL_NAME(SIGL, CHAR_SIGL)
  GLOBAL_NAME(SIGNAL, CHAR_SIGNAL)
  GLOBAL_NAME(SOURCENAME, CHAR_SOURCE)
  GLOBAL_NAME(STDERR, CHAR_STDERR)
  GLOBAL_NAME(STDIN,  CHAR_STDIN)
  GLOBAL_NAME(STDOUT, CHAR_STDOUT)
  GLOBAL_NAME(CSTDERR, CHAR_CSTDERR)/* standard streams with colon */
  GLOBAL_NAME(CSTDIN,  CHAR_CSTDIN) /* standard streams with colon */
  GLOBAL_NAME(CSTDOUT, CHAR_CSTDOUT)/* standard streams with colon */
  GLOBAL_NAME(STREAM, CHAR_STREAM)
  GLOBAL_NAME(STREAMS, CHAR_STREAMS)
  GLOBAL_NAME(STATE, CHAR_STATE)
  GLOBAL_NAME(STRICT_BACKSLASH_EQUAL, CHAR_STRICT_BACKSLASH_EQUAL)
  GLOBAL_NAME(STRICT_BACKSLASH_GREATERTHAN, CHAR_STRICT_BACKSLASH_GREATERTHAN)
  GLOBAL_NAME(STRICT_BACKSLASH_LESSTHAN, CHAR_STRICT_BACKSLASH_LESSTHAN)
  GLOBAL_NAME(STRICT_EQUAL, CHAR_STRICT_EQUAL)
  GLOBAL_NAME(STRICT_GREATERTHAN, CHAR_STRICT_GREATERTHAN)
  GLOBAL_NAME(STRICT_GREATERTHAN_EQUAL, CHAR_STRICT_GREATERTHAN_EQUAL)
  GLOBAL_NAME(STRICT_LESSTHAN, CHAR_STRICT_LESSTHAN)
  GLOBAL_NAME(STRICT_LESSTHAN_EQUAL, CHAR_STRICT_LESSTHAN_EQUAL)
  GLOBAL_NAME(STRINGSYM, CHAR_STRINGSYM)
  GLOBAL_NAME(SUBROUTINE, CHAR_SUBROUTINE)
  GLOBAL_NAME(SUBTRACT, CHAR_SUBTRACT)
  GLOBAL_NAME(ASSIGNMENT_SUBTRACT, CHAR_ASSIGNMENT_SUBTRACT)
  GLOBAL_NAME(SUPER, CHAR_SUPER)
  GLOBAL_NAME(SUPPLIERSYM, CHAR_SUPPLIER)
  GLOBAL_NAME(SYNTAX, CHAR_SYNTAX)
  GLOBAL_NAME(TOKENIZE_ONLY, CHAR_TOKENIZE_ONLY)
  GLOBAL_NAME(TRACEBACK, CHAR_TRACEBACK)
  GLOBAL_NAME(UNINIT, CHAR_UNINIT)
  GLOBAL_NAME(UNKNOWN, CHAR_UNKNOWN)
  GLOBAL_NAME(VALUE, CHAR_VALUE)
  GLOBAL_NAME(VERSION, CHAR_VERSION)
  GLOBAL_NAME(XOR, CHAR_XOR)
  GLOBAL_NAME(ASSIGNMENT_XOR, CHAR_ASSIGNMENT_XOR)
  GLOBAL_NAME(ZERO_STRING, CHAR_ZERO)


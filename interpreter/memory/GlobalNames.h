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
/* REXX Kernel                                              GlobalNames.h     */
/*                                                                            */
/* Definitions of all name objects created at startup time.  All these        */
/* Name objects are addressible via OREF_ global names.  Changes to this file */
/* also require recompilation of GlobalNames.cpp and GlobalData.cpp for things*/
/* to link correctly.                                                         */
/*                                                                            */
/* NOTE:  The string values of these constants do not need to be a symbolic   */
/* CHAR_* name.  These values can be directly coded as a literal string.  If  */
/* an existing CHAR_* constant is available for the value to be defined, use  */
/* the symbol.  Otherwise, it is perfectly acceptable to just use a literal   */
/* string value here.                                                         */
/*                                                                            */
/******************************************************************************/

  GLOBAL_NAME(ACTIVATE, "ACTIVATE")
  GLOBAL_NAME(ADDITIONAL, "ADDITIONAL")
  GLOBAL_NAME(ADDRESS, "ADDRESS")
  GLOBAL_NAME(AND, "&")
  GLOBAL_NAME(ASSIGNMENT_AND, "&=")
  GLOBAL_NAME(ANY, "ANY")
  GLOBAL_NAME(ARGUMENTS, "ARGUMENTS")
  GLOBAL_NAME(ARRAY, "ARRAY")
  GLOBAL_NAME(BACKSLASH, "\\")
  GLOBAL_NAME(BACKSLASH_EQUAL, "\\=")
  GLOBAL_NAME(BACKSLASH_GREATERTHAN, "\\>")
  GLOBAL_NAME(BACKSLASH_LESSTHAN, "\\<")
  GLOBAL_NAME(BLANK, " ")
  GLOBAL_NAME(BRACKETS, "[]")
  GLOBAL_NAME(CALL, "CALL")
  GLOBAL_NAME(CHARIN, "CHARIN")
  GLOBAL_NAME(CHAROUT, "CHAROUT")
  GLOBAL_NAME(CHARS, "CHARS")
  GLOBAL_NAME(CLOSE, "CLOSE")
  GLOBAL_NAME(CODE, "CODE")
  GLOBAL_NAME(COMMAND, "COMMAND")
  GLOBAL_NAME(COMPARE, "COMPARE")
  GLOBAL_NAME(COMPARETO, "COMPARETO")
  GLOBAL_NAME(CONCATENATE, "||")
  GLOBAL_NAME(ASSIGNMENT_CONCATENATE, "||=")
  GLOBAL_NAME(CONDITION, "CONDITION")
  GLOBAL_NAME(CSELF, "CSELF")
  GLOBAL_NAME(DEBUGINPUT, "DEBUGINPUT")
  GLOBAL_NAME(DEFAULTNAME, "DEFAULTNAME")
  GLOBAL_NAME(DELAY, "DELAY")
  GLOBAL_NAME(DESCRIPTION, "DESCRIPTION")
  GLOBAL_NAME(DIVIDE, "/")
  GLOBAL_NAME(ASSIGNMENT_DIVIDE, "/=")
  GLOBAL_NAME(ENGINEERING, "ENGINEERING")
  GLOBAL_NAME(ENVIRONMENT, "ENVIRONMENT")
  GLOBAL_NAME(EQUAL, "=")
  GLOBAL_NAME(ERRORNAME, "ERROR")
  GLOBAL_NAME(ERRORTEXT, "ERRORTEXT")
  GLOBAL_NAME(FAILURE, "FAILURE")
  GLOBAL_NAME(FUNCTION, "FUNCTION")
  GLOBAL_NAME(GET, "GET")
  GLOBAL_NAME(GREATERTHAN, ">")
  GLOBAL_NAME(GREATERTHAN_EQUAL, ">=")
  GLOBAL_NAME(GREATERTHAN_LESSTHAN, "><")
  GLOBAL_NAME(HALT, "HALT")
  GLOBAL_NAME(HASHCODE, "HASHCODE")
  GLOBAL_NAME(INHERIT, "INHERIT")
  GLOBAL_NAME(INIT, "INIT")
  // this is defined in the platform definitions.
  GLOBAL_NAME(INITIALADDRESS, SYSINITIALADDRESS)
  GLOBAL_NAME(INPUT, "INPUT")
  GLOBAL_NAME(INSTRUCTION, "INSTRUCTION")
  GLOBAL_NAME(INTDIV, "%")
  GLOBAL_NAME(ASSIGNMENT_INTDIV, "%=")
  GLOBAL_NAME(LESSTHAN, "<")
  GLOBAL_NAME(LESSTHAN_EQUAL, "<=")
  GLOBAL_NAME(LESSTHAN_GREATERTHAN, "<>")
  GLOBAL_NAME(LINEIN, "LINEIN")
  GLOBAL_NAME(LINEOUT, "LINEOUT")
  GLOBAL_NAME(LINES, "LINES")
  GLOBAL_NAME(LOCAL, "LOCAL")
  GLOBAL_NAME(LOSTDIGITS, "LOSTDIGITS")
  GLOBAL_NAME(MAKEARRAY, "MAKEARRAY")
  GLOBAL_NAME(MAKESTRING, "MAKESTRING")
  GLOBAL_NAME(MessageComplete, "MESSAGECOMPLETE")
  GLOBAL_NAME(MessageNotification, "MESSAGENOTIFICATION")
  GLOBAL_NAME(METHOD, "METHOD")
  GLOBAL_NAME(MULTIPLY, "*")
  GLOBAL_NAME(ASSIGNMENT_MULTIPLY, "*=")
  GLOBAL_NAME(NAME, "NAME")
  GLOBAL_NAME(MESSAGE, "MESSAGE")
  GLOBAL_NAME(NEW, "NEW")
  GLOBAL_NAME(NOMETHOD, "NOMETHOD")
  GLOBAL_NAME(NONE, "NONE")
  GLOBAL_NAME(NOSTRING, "NOSTRING")
  GLOBAL_NAME(NOVALUE, "NOVALUE")
  GLOBAL_NAME(NULLSTRING, "")
  GLOBAL_NAME(OBJECT, "OBJECT")
  GLOBAL_NAME(OBJECTNAME, "OBJECTNAME")
  GLOBAL_NAME(OFF, "OFF")
  GLOBAL_NAME(ON, "ON")
  GLOBAL_NAME(OR, "|")
  GLOBAL_NAME(ASSIGNMENT_OR, "|=")
  GLOBAL_NAME(OUTPUT, "OUTPUT")
  GLOBAL_NAME(PLUS, "+")
  GLOBAL_NAME(ASSIGNMENT_PLUS, "+=")
  GLOBAL_NAME(POSITION, "POSITION")
  GLOBAL_NAME(POWER, "**")
  GLOBAL_NAME(ASSIGNMENT_POWER, "**=")
  GLOBAL_NAME(PROGRAM, "PROGRAM")
  // there is a define conflict with the package name on the Mac
  GLOBAL_NAME(PACKAGE_REF, "PACKAGE")
  GLOBAL_NAME(PROPAGATE, "PROPAGATE")
  GLOBAL_NAME(PROPAGATED, "PROPAGATED")
  GLOBAL_NAME(PULL, "PULL")
  GLOBAL_NAME(PUSH, "PUSH")
  GLOBAL_NAME(QUEUED, "QUEUED")
  GLOBAL_NAME(QUEUE, "QUEUE")
  GLOBAL_NAME(RC, "RC")
  GLOBAL_NAME(REMAINDER, "//")
  GLOBAL_NAME(REXX, "REXX")
  GLOBAL_NAME(ASSIGNMENT_REMAINDER, "//=")
  GLOBAL_NAME(REQUEST, "REQUEST")
  GLOBAL_NAME(REQUIRES, "REQUIRES")
  GLOBAL_NAME(RESULT, "RESULT")
  GLOBAL_NAME(STDQUE, "STDQUE")
  GLOBAL_NAME(REXXQUEUE, "REXXQUEUE")
  GLOBAL_NAME(REXXUTIL, "REXXUTIL")
  GLOBAL_NAME(ROUTINE, "ROUTINE")
  GLOBAL_NAME(RUN, "RUN")
  GLOBAL_NAME(SAY, "SAY")
  GLOBAL_NAME(SCIENTIFIC, "SCIENTIFIC")
  GLOBAL_NAME(SECURITYMANAGER, "SECURITYMANAGER")
  GLOBAL_NAME(SELF, "SELF")
  GLOBAL_NAME(SEND, "SEND")
  GLOBAL_NAME(SESSION, "SESSION")
  GLOBAL_NAME(SIGL, "SIGL")
  GLOBAL_NAME(SIGNAL, "SIGNAL")
  GLOBAL_NAME(STREAM, "STREAM")
  GLOBAL_NAME(STATE, "STATE")
  GLOBAL_NAME(STRICT_BACKSLASH_EQUAL, "\\==")
  GLOBAL_NAME(STRICT_BACKSLASH_GREATERTHAN, "\\>>")
  GLOBAL_NAME(STRICT_BACKSLASH_LESSTHAN, "\\<<")
  GLOBAL_NAME(STRICT_EQUAL, "==")
  GLOBAL_NAME(STRICT_GREATERTHAN, ">>")
  GLOBAL_NAME(STRICT_GREATERTHAN_EQUAL, ">>=")
  GLOBAL_NAME(STRICT_LESSTHAN, "<<")
  GLOBAL_NAME(STRICT_LESSTHAN_EQUAL, "<<=")
  GLOBAL_NAME(STRING, "STRING")
  GLOBAL_NAME(SUBROUTINE, "SUBROUTINE")
  GLOBAL_NAME(SUBTRACT, "-")
  GLOBAL_NAME(ASSIGNMENT_SUBTRACT, "-=")
  GLOBAL_NAME(SUPER, "SUPER")
  GLOBAL_NAME(SUPPLIER, "SUPPLIER")
  GLOBAL_NAME(SYNTAX, "SYNTAX")
  GLOBAL_NAME(TRACEBACK, "TRACEBACK")
  GLOBAL_NAME(TRACEOUTPUT, "TRACEOUTPUT")
  GLOBAL_NAME(STACKFRAMES, "STACKFRAMES")
  GLOBAL_NAME(UNINIT, "UNINIT")
  GLOBAL_NAME(UNKNOWN, "UNKNOWN")
  GLOBAL_NAME(UNNAMED_METHOD, "*UNNAMED*")
  GLOBAL_NAME(XOR, "&&")
  GLOBAL_NAME(ASSIGNMENT_XOR, "&&=")
  GLOBAL_NAME(ZERO, "0")
  GLOBAL_NAME(SIGINT_STRING, "SIGINT")
  GLOBAL_NAME(SIGTERM_STRING, "SIGTERM")
  GLOBAL_NAME(SIGHUP_STRING, "SIGHUP")
  GLOBAL_NAME(DEFAULT_RESOURCE_END, "::END")


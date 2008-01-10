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

#ifndef ENG2REXX
#define ENG2REXX

#include "rexx.h"
#include "RexxNativeAPI.h"

/************ DEFINED IN WINMAIN.C *****************/
typedef struct _ConditionData {
  int    code;
  int    rc;
  RXSTRING message;
  RXSTRING errortext;
  size_t position;
  RXSTRING program;
} ConditionData;
/************ DEFINED IN WINMAIN.C *****************/

#include "orxscrpt.hpp"
#include "OrxScrptError.hpp"
class OrxScript;

extern HANDLE mutex;
extern Index *thread2EngineList;

LONG APIENTRY RexxCatchExit(LONG, LONG, PEXIT);
LONG APIENTRY RexxCatchExternalFunc(LONG, LONG, PEXIT);
int __stdcall scriptSecurity(CLSID,IUnknown*);
REXXOBJECT Create_securityObject(OrxScript *, FILE *);
void __stdcall parseText(void*);
void __stdcall createCode(void*);
void __stdcall runMethod(void*);

// these functions are not documented to the outside world,
// so we have to give the prototypes here, they're not in rexx.h:
void REXXENTRY RexxCreateDirectory(const char *);
void REXXENTRY RexxRemoveDirectory(const char *);
APIRET REXXENTRY RexxCreateMethod(const char *, PRXSTRING, REXXOBJECT *, ConditionData *);
APIRET REXXENTRY RexxLoadMethod(const char *, PRXSTRING, REXXOBJECT *);
APIRET REXXENTRY RexxStoreMethod(REXXOBJECT, PRXSTRING);
void WinGetVariables(void (__stdcall *callback)(void*));
void WinEnterKernel(bool);
void WinLeaveKernel(bool);
extern "C" {
APIRET REXXENTRY RexxRunMethod(const char *, REXXOBJECT, void *, REXXOBJECT (__stdcall *)(void*), PRXSYSEXIT, REXXOBJECT *, REXXOBJECT, ConditionData *);
}
// these three come from orexxole.c
REXXOBJECT Variant2Rexx(VARIANT *);
void Rexx2Variant(REXXOBJECT, VARIANT *, VARTYPE, size_t);
void setCreationCallback(int (__stdcall *f)(CLSID, IUnknown*));

#endif

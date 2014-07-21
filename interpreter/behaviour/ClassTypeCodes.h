

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
/* REXX  Support                                                              */
/*                                                                            */
/* Rexx primitive class id codes                                              */
/*                                                                            */
/*        -- DO NOT CHANGE THIS FILE, ALL CHANGES WILL BE LOST! --            */
/*                  -- FILE WAS GENERATED BY ClassTypeIds.xsl --              */
/******************************************************************************/

#ifndef ClassTypeCodes_Included
#define ClassTypeCodes_Included

typedef enum
{
    T_First_Primitive_Class = 0,
    T_First_Exported_Class = 0,

    T_Object = 0,
    T_ObjectClass = 1,
    T_Class = 2,
    T_ClassClass = 3,
    T_Array = 4,
    T_ArrayClass = 5,
    T_Directory = 6,
    T_DirectoryClass = 7,
    T_Integer = 8,
    T_IntegerClass = 9,
    T_List = 10,
    T_ListClass = 11,
    T_Message = 12,
    T_MessageClass = 13,
    T_Method = 14,
    T_MethodClass = 15,
    T_NumberString = 16,
    T_NumberStringClass = 17,
    T_Queue = 18,
    T_QueueClass = 19,
    T_Stem = 20,
    T_StemClass = 21,
    T_String = 22,
    T_StringClass = 23,
    T_Supplier = 24,
    T_SupplierClass = 25,
    T_Table = 26,
    T_TableClass = 27,
    T_StringTable = 28,
    T_StringTableClass = 29,
    T_Relation = 30,
    T_RelationClass = 31,
    T_MutableBuffer = 32,
    T_MutableBufferClass = 33,
    T_Pointer = 34,
    T_PointerClass = 35,
    T_Buffer = 36,
    T_BufferClass = 37,
    T_WeakReference = 38,
    T_WeakReferenceClass = 39,
    T_Routine = 40,
    T_RoutineClass = 41,
    T_Package = 42,
    T_PackageClass = 43,
    T_RexxContext = 44,
    T_RexxContextClass = 45,
    T_IdentityTable = 46,
    T_IdentityTableClass = 47,
    T_StackFrame = 48,
    T_StackFrameClass = 49,
    T_SetClass = 50,
    T_SetClassClass = 51,
    T_BagClass = 52,
    T_BagClassClass = 53,

    T_Last_Exported_Class = 53,
    
    T_First_Internal_Class = 54,

    T_NilObject = 54,
    T_Behaviour = 55,
    T_MethodDictionary = 56,
    T_ScopeTable = 57,
    T_RexxSource = 58,
    T_LibraryPackage = 59,
    T_RexxCode = 60,
    T_NativeMethod = 61,
    T_NativeRoutine = 62,
    T_RegisteredRoutine = 63,
    T_CPPCode = 64,
    T_AttributeGetterCode = 65,
    T_AttributeSetterCode = 66,
    T_ConstantGetterCode = 67,
    T_AbstractCode = 68,
    T_SmartBuffer = 69,
    T_IdentityHashContents = 70,
    T_EqualityHashContents = 71,
    T_Variable = 72,
    T_VariableDictionary = 73,
    T_VariableTerm = 74,
    T_CompoundVariableTerm = 75,
    T_StemVariableTerm = 76,
    T_DotVariableTerm = 77,
    T_IndirectVariableTerm = 78,
    T_FunctionCallTerm = 79,
    T_MessageSendTerm = 80,
    T_UnaryOperatorTerm = 81,
    T_BinaryOperatorTerm = 82,
    T_LogicalTerm = 83,
    T_Instruction = 84,
    T_AddressInstruction = 85,
    T_AssignmentInstruction = 86,
    T_CallInstruction = 87,
    T_DynamicCallInstruction = 88,
    T_CallOnInstruction = 89,
    T_CommandInstruction = 90,
    T_SimpleDoInstruction = 91,
    T_DoForeverInstruction = 92,
    T_DoOverInstruction = 93,
    T_DoOverUntilInstruction = 94,
    T_DoOverWhileInstruction = 95,
    T_ControlledDoInstruction = 96,
    T_ControlledDoUntilInstruction = 97,
    T_ControlledDoWhileInstruction = 98,
    T_DoWhileInstruction = 99,
    T_DoUntilInstruction = 100,
    T_DoCountInstruction = 101,
    T_DoCountUntilInstruction = 102,
    T_DoCountWhileInstruction = 103,
    T_DropInstruction = 104,
    T_ElseInstruction = 105,
    T_EndInstruction = 106,
    T_EndIfInstruction = 107,
    T_ExitInstruction = 108,
    T_ExposeInstruction = 109,
    T_ForwardInstruction = 110,
    T_GuardInstruction = 111,
    T_IfInstruction = 112,
    T_CaseWhenInstruction = 113,
    T_InterpretInstruction = 114,
    T_LabelInstruction = 115,
    T_LeaveInstruction = 116,
    T_MessageInstruction = 117,
    T_NopInstruction = 118,
    T_NumericInstruction = 119,
    T_OptionsInstruction = 120,
    T_OtherwiseInstruction = 121,
    T_ParseInstruction = 122,
    T_ProcedureInstruction = 123,
    T_QueueInstruction = 124,
    T_RaiseInstruction = 125,
    T_ReplyInstruction = 126,
    T_ReturnInstruction = 127,
    T_SayInstruction = 128,
    T_SelectInstruction = 129,
    T_SelectCaseInstruction = 130,
    T_SignalInstruction = 131,
    T_DynamicSignalInstruction = 132,
    T_SignalOnInstruction = 133,
    T_ThenInstruction = 134,
    T_TraceInstruction = 135,
    T_UseInstruction = 136,
    T_ClassDirective = 137,
    T_LibraryDirective = 138,
    T_RequiresDirective = 139,
    T_CompoundElement = 140,
    T_ParseTrigger = 141,
    T_ProgramSource = 142,
    T_ArrayProgramSource = 143,
    T_BufferProgramSource = 144,
    T_FileProgramSource = 145,
    T_NumberArray = 146,

    T_Last_Internal_Class = 146,
    
    T_First_Transient_Class = 147,

    T_Memory = 147,
    T_InternalStack = 148,
    T_LiveStack = 149,
    T_PushThroughStack = 150,
    T_Activity = 151,
    T_Activation = 152,
    T_NativeActivation = 153,
    T_ActivationFrameBuffer = 154,
    T_Envelope = 155,
    T_LanguageParser = 156,
    T_Clause = 157,
    T_Token = 158,
    T_DoBlock = 159,
    T_InterpreterInstance = 160,
    T_SecurityManager = 161,
    T_CommandHandler = 162,
    T_MapBucket = 163,
    T_MapTable = 164,

    T_Last_Transient_Class = 164,
    T_Last_Primitive_Class = 164,
    T_Last_Class_Type = 164,
    
} ClassTypeCode;

/* -------------------------------------------------------------------------- */
/* --            ==================================================        -- */
/* --            DO NOT CHANGE THIS FILE, ALL CHANGES WILL BE LOST!        -- */
/* --            ==================================================        -- */
/* -------------------------------------------------------------------------- */
#endif


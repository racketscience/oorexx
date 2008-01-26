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
/*********************************************************************/
/*                                                                   */
/*  Module Name:        WINMETA.C                                    */
/*                                                                   */
/* Unflatten saved methods from various sources.                     */
/*                                                                   */
/*********************************************************************/

#include "RexxCore.h"
#include "StringClass.hpp"
#include "BufferClass.hpp"
#include "RexxSmartBuffer.hpp"
#include "MethodClass.hpp"
#include "RexxCode.hpp"
#include "RexxActivity.hpp"
#include "SourceFile.hpp"
#include "ActivityManager.hpp"
#include "ProtectedObject.hpp"

/*********************************************************************/
/*         Definitions for use by the Meta I/O functionality         */
/*********************************************************************/

#define  METAVERSION       34
                                       /* to ::class instruction errors     */
                                       /* 29 => 31 bump for change in hash  */
                                       /* algorithm                         */
                                       /* 31 => 34 bump for new kernel and  */
                                       /* USERID BIF                        */
#define  MAGIC          11111          /* function                          */


#define  VERPRE         "WINDOWS  "    /* 8 chars + 1 blank          */
#define  LENPRE         9
                                       /* size of control structure         */
#define  CONTROLSZ      sizeof(FILE_CONTROL)

#define  compiledHeader "/**/@REXX"


// OS2 FILESTATUS Stuctures duplicated for win32
typedef struct _FDATE {
   unsigned short day:5;
   unsigned short month:4;
   unsigned short year:7;
} FDATE;

typedef struct _FTIME{
   unsigned short twosecs:5;
   unsigned short minutes:6;
   unsigned short hours:5;
} FTIME;


typedef struct _FILESTATUS{
   FDATE fdataCreation;
   FTIME ftimeCreation;
   FDATE fdateLastAccess;
   FTIME ftimeLastAccess;
   FDATE fdateLastWrite;
   FTIME ftimeLastWrite;
   ULONG cbFile;
   ULONG cbFileAlloc;
   unsigned short attrFile;
} FILESTATUS;

typedef struct _control {              /* meta date control info            */
    unsigned short Magic;              /* identifies as 'meta' prog         */
    unsigned short MetaVersion;        /* version of the meta prog          */
    char           RexxVersion[40];    /* version of rexx intrpreter        */
    FILESTATUS     FileStatus;         /* file information                  */
    size_t         ImageSize;          /* size of the method info           */
} FILE_CONTROL;                        /* saved control info                */


typedef FILE_CONTROL *PFILE_CONTROL;   /* pointer to file info              */

RexxMethod *SysRestoreTranslatedProgram(RexxString *, FILE *Handle);

/*********************************************************************/
/*                                                                   */
/*  Function:    SysRestoreProgram                                   */
/*                                                                   */
/*  Description: This function is used to load the meta data from a  */
/*               file into the proper interpreter variables.         */
/*                                                                   */
/*********************************************************************/

RexxMethod *SysRestoreProgram(
  RexxString *FileName )               /* name of file to process           */

{
  FILE         *Handle;                /* output file handle                */
  const char   *File;                  /* ASCII-Z file name                 */

  RexxMethod  * Method;                /* unflattened method                */
  FILE_CONTROL  Control;               /* control information               */
  char        * StartPointer;          /* start of buffered method          */
  RexxBuffer  * Buffer;                /* Buffer to unflatten               */
  size_t        BufferSize;            /* size of the buffer                */
  size_t        BytesRead;             /* actual bytes read                 */
  RexxSource  * Source;                /* REXX source object                */
  bool          badMeta = false;
  void         *MethodInfo;
                                       /* temporary read buffer             */
  char          fileTag[sizeof(compiledHeader)];

  // if we're in save image mode, we never used a saved image
  if (memoryObject.savingImage())
  {
      return OREF_NULL;
  }

  File = FileName->getStringData();    /* get the file name pointer         */

  Handle = fopen(File, "rb");          /* open the input file               */
  if (Handle == NULL)                  /* get an open error?                */
    return OREF_NULL;                  /* no restored image                 */

                                       /* see if this is a "sourceless" one */
  Method = SysRestoreTranslatedProgram(FileName, Handle);
  if (Method != OREF_NULL)             /* get a method out of this?         */
    return Method;                     /* this process is finished          */
/* this is to load the tokenized form that is eventually stored behind the script */
  Handle = fopen(File, "rb");          /* open the input file               */
  if (Handle == NULL)                  /* get an open error?                */
    return OREF_NULL;                  /* no restored image                 */

  {
                                           /* read the first file part          */
      if (fseek(Handle, 0-sizeof(compiledHeader), SEEK_END) == 0)
         BytesRead = fread(fileTag, 1 ,sizeof(compiledHeader), Handle);
                                           /* not a compiled file?              */
      if ((BytesRead != sizeof(compiledHeader)) || (strcmp(fileTag, compiledHeader) != 0)) {
        fclose(Handle);                    /* close the file                    */
        return OREF_NULL;                  /* not a saved program               */
      }
                                           /* now read the control info         */
      if (fseek(Handle, 0-sizeof(compiledHeader)-sizeof(Control), SEEK_END) == 0)
         BytesRead = fread((char *)&Control, 1, sizeof(Control), Handle);

                                           /* check the control info            */
      if ((BytesRead != sizeof(Control)) || (Control.MetaVersion != METAVERSION) || (Control.Magic != MAGIC)) {
        fclose(Handle);                    /* close the file                    */
        badMeta = true;
                                           /* got an error here                 */
      }
      else
      {
                                               /* read the file size                */
          BufferSize = Control.ImageSize;      /* get the method info size          */
          if (fseek(Handle, (long)(0-sizeof(compiledHeader)-sizeof(Control)-BufferSize), SEEK_END) != 0) {
            fclose(Handle);                    /* close the file                    */
            return OREF_NULL;                  /* not a saved program               */
          }
          MethodInfo = GlobalAlloc(GMEM_FIXED, BufferSize);  /* allocate a temp buffer */
          if (!MethodInfo) {
            fclose(Handle);                    /* close the file                    */
            return OREF_NULL;                  /* not a saved program               */
          }
          /* read the tokenized program */
          BytesRead = fread(MethodInfo, 1, BufferSize, Handle);
          fclose(Handle);                      /* close the file                    */
      }
  }
  if (badMeta)
  {
      reportException(Error_Program_unreadable_version, FileName);
  }

  Buffer = new_buffer(BufferSize);     /* get a new buffer                  */
  ProtectedObject p(Buffer);
                                       /* position relative to the end      */
  StartPointer = ((char *)Buffer + Buffer->getObjectSize()) - BufferSize;
  memcpy(StartPointer, MethodInfo, BufferSize);
  GlobalFree(MethodInfo);              /* done with the tokenize buffer     */
                                       /* "puff" this out usable form       */
  Method = TheMethodClass->restore(Buffer, StartPointer);
  ProtectedObject p1(Method);
  Source = ((RexxCode*)Method->getCode())->getSourceObject();   /* and now the source object         */
                                       /* switch the file name (this might  */
                                       /* be different than the name        */
  Source->setProgramName(FileName);    /* originally saved under            */
  Source->setReconnect();              /* allow source file reconnection    */
  return Method;                       /* return the unflattened method     */
}

/*********************************************************************/
/*                                                                   */
/*  Function:    SysSaveProgram                                      */
/*                                                                   */
/*  Description: This function saves a flattened method to a file    */
/*                                                                   */
/*********************************************************************/
// Macro replaces this in Winrexx.h
// Not implemented for Windows because,
// unable to save a program image in Windows, no EA's

void SysSaveProgram(
  RexxString * FileName,               /* name of file to process           */
  RexxMethod * Method )                /* method to save                    */
{
  FILE          *Handle;                /* file handle                      */
  FILE_CONTROL  Control;               /* control information               */
  RexxBuffer *  MethodBuffer;          /* flattened method                  */
  RexxSmartBuffer *FlatBuffer;         /* fully flattened buffer            */
  char         *BufferAddress;         /* address of flattened method data  */
  size_t        BufferLength;          /* length of the flattened method    */
  RexxString *  Version;               /* REXX version string               */
  size_t        BytesRead;             /* actual bytes read                 */
  const char   *File;                  /* ASCII-Z file name                 */
  RexxActivity *activity;              /* current activity                  */
  char          savetok[65];

  // if we're in save image mode, we never used a saved image
  if (memoryObject.savingImage())
  {
      return;
  }

  /* don't save tokens if environment variable isn't set */
  if ((GetEnvironmentVariable("RXSAVETOKENS", savetok, 64) < 1) || (strcmp("YES",savetok) != 0))
    return;

  ProtectedObject p1(FileName);
  ProtectedObject p2(Method);
  File = FileName->getStringData();    /* get the file name pointer         */
                                       /* open the file                     */
  activity = ActivityManager::currentActivity;          /* save the activity                 */


  Handle = fopen(File, "a+b");         /* open the output file  */
  if (Handle == NULL)                  /* get an open error?                */
     return;

  FlatBuffer = Method->saveMethod();   /* flatten the method                */
  ProtectedObject p3(FlatBuffer);
                                       /* retrieve the length of the buffer */
  BufferLength = FlatBuffer->current;
  MethodBuffer = FlatBuffer->buffer;   /* get to the actual data buffer     */
  BufferAddress = MethodBuffer->address();  /* retrieve buffer starting address  */
                                       /* clear out the cntrol info         */
  memset((void *)&Control, 0, sizeof(Control));
                                       /* fill in version info              */
  memcpy(Control.RexxVersion, VERPRE, LENPRE);
  Version = version_number();          /* get the version string            */
  memcpy((char *)Control.RexxVersion + LENPRE, Version->getStringData(), Version->getLength() > (40-LENPRE)?(40-LENPRE):Version->getLength());
  Control.MetaVersion = METAVERSION;   /* current meta version              */
  Control.Magic = MAGIC;               /* magic signature number            */
  Control.ImageSize = BufferLength;    /* add the buffer length             */

  {
      UnsafeBlock releaser;
                                           /* write out the REXX signature      */
      BytesRead = putc(0x1a, Handle);   /* Ctrl Z */
      fwrite(BufferAddress, 1, BufferLength, Handle);

      fwrite(&Control, 1, sizeof(Control), Handle);

      fwrite(compiledHeader, 1, sizeof(compiledHeader), Handle);
                                           /* now the control info              */
                                           /* and finally the flattened method  */
      fclose(Handle);                      /* done saving                       */
  }
}


/*********************************************************************/
/*                                                                   */
/*  Function:    SysRestoreProgramBuffer                             */
/*                                                                   */
/*  Description: This function is used to unflatten a REXX method    */
/*               from a buffer into a method.                        */
/*                                                                   */
/*********************************************************************/

RexxMethod *SysRestoreProgramBuffer(
  PRXSTRING    InBuffer,               /* pointer to stored method          */
  RexxString  *Name)                   /* name associated with the program  */
{
  PFILE_CONTROL Control;               /* control information               */
  char         *MethodInfo;            /* buffered flattened method         */
  char         *StartPointer;          /* start of buffered information     */
  RexxBuffer   *Buffer;                /* Buffer to unflatten               */
  size_t        BufferSize;            /* size of the buffer                */
  RexxMethod   *Method;                /* unflattened method                */
  RexxSource   *Source;                /* REXX source object                */

                                       /* address the control information   */
  Control = (PFILE_CONTROL)InBuffer->strptr;
                                       /* check the control info            */
  if ((Control->MetaVersion != METAVERSION) ||
      (Control->Magic != MAGIC)) {
    return OREF_NULL;                  /* can't load it                     */
  }
  MethodInfo = InBuffer->strptr + CONTROLSZ;
                                       /* get the buffer size               */
  BufferSize = InBuffer->strlength - CONTROLSZ;
  Buffer = new_buffer(BufferSize);     /* get a new buffer                  */
                                       /* position relative to the end      */
  StartPointer = ((char *)Buffer + Buffer->getObjectSize()) - BufferSize;
                                       /* fill in the buffer                */
  memcpy(StartPointer, MethodInfo, BufferSize);
  ProtectedObject p(Buffer);
                                       /* "puff" this out usable form       */
  Method = TheMethodClass->restore(Buffer, StartPointer);
  Source = ((RexxCode*)Method->getCode())->getSourceObject();   /* and now the source object         */
                                       /* switch the file name (this might  */
                                       /* be different than the name        */
  Source->setProgramName(Name);        /* originally saved under            */
                                       /* NOTE:  no source file reconnect   */
                                       /* is possible here                  */
  return Method;                       /* return the unflattened method     */
}
                                       /* point to the flattened method     */
/*********************************************************************/
/*                                                                   */
/*  Function:    SysRestoreInstoreProgram                            */
/*                                                                   */
/*  Description: This function is used to load the meta data from an */
/*               instorage buffer.                                   */
/*                                                                   */
/*********************************************************************/
RexxMethod *SysRestoreInstoreProgram(
  PRXSTRING    InBuffer,               /* pointer to stored method          */
  RexxString * Name)                   /* name associated with the program  */
{
  RXSTRING     ImageData;              /* actual image part of this         */

                                       /* not a compiled file?              */
  if (strcmp(InBuffer->strptr, compiledHeader) != 0)
    return OREF_NULL;                  /* not a saved program               */
                                       /* point to the image start          */
  ImageData.strptr = InBuffer->strptr + sizeof(compiledHeader);
                                       /* and adjust the length too         */
  ImageData.strlength = InBuffer->strlength - sizeof(compiledHeader);
                                       /* now go unflatten this             */
  return SysRestoreProgramBuffer(&ImageData, Name);
}

/*********************************************************************/
/*                                                                   */
/*  Function:    SysSaveProgramBuffer                                */
/*                                                                   */
/*  Description: This function saves a flattened method to an        */
/*               RXSTRING buffer                                     */
/*                                                                   */
/*********************************************************************/
void SysSaveProgramBuffer(
  PRXSTRING    OutBuffer,              /* location to save the program      */
  RexxMethod  *Method )                /* method to save                    */
{
  PFILE_CONTROL Control;               /* control information               */
  char         *Buffer;                /* buffer pointer                    */
  RexxBuffer   *MethodBuffer;          /* flattened method                  */
  RexxSmartBuffer *FlatBuffer;         /* flattened smart buffer            */
  char         *BufferAddress;         /* address of flattened method data  */
  LONG          BufferLength;          /* length of the flattened method    */
  RexxString   *Version;               /* REXX version string               */

  ProtectedObject p(Method);
  FlatBuffer = Method->saveMethod();   /* flatten the method                */
                                       /* retrieve the length of the buffer */
  BufferLength = (LONG)FlatBuffer->current;
  MethodBuffer = FlatBuffer->buffer;   /* get to the actual data buffer     */
  BufferAddress = MethodBuffer->address();  /* retrieve buffer starting address  */
                                       /* get the final buffer              */
  Buffer = (char *)SysAllocateResultMemory(BufferLength + CONTROLSZ);
  OutBuffer->strptr = Buffer;          /* fill in the result pointer        */
                                       /* and the result length             */
  OutBuffer->strlength = BufferLength + CONTROLSZ;
  Control = (PFILE_CONTROL)Buffer;     /* set pointer to control info       */
                                       /* fill in version info              */
  memcpy(Control->RexxVersion, VERPRE, LENPRE);
  Version = version_number();          /* get the version string            */
                                       /* copy in the version string        */
  memcpy((Control->RexxVersion) + LENPRE, Version->getStringData(), Version->getLength() + 1);

  Control->MetaVersion = METAVERSION;  /* current meta version              */
  Control->Magic = MAGIC;              /* magic signature number            */
  Control->ImageSize = BufferLength;   /* save the buffer size              */
  Buffer = Buffer + CONTROLSZ;         /* step the buffer pointer           */
                                       /* Copy the method buffer            */
  memcpy(Buffer, BufferAddress, BufferLength);
}

/*********************************************************************/
/*                                                                   */
/*  Function:    SysSaveTranslatedProgram                            */
/*                                                                   */
/*  Description: This function saves a flattened method to a file    */
/*                                                                   */
/*********************************************************************/

// retrofit by IH
void SysSaveTranslatedProgram(
  const char  *File,                   /* name of file to process           */
  RexxMethod  *Method )                /* method to save                    */
{
  FILE         *Handle;                /* output file handle                */
  FILE_CONTROL  Control;               /* control information               */
  RexxBuffer   *MethodBuffer;          /* flattened method                  */
  RexxSmartBuffer *FlatBuffer;         /* flattened smart buffer            */
  char         *BufferAddress;         /* address of flattened method data  */
  LONG          BufferLength;          /* length of the flattened method    */
  RexxString   *Version;               /* REXX version string               */

  Handle = fopen(File, "wb");          /* open the output file              */
  if (Handle == NULL)                  /* get an open error?                */
                                       /* got an error here                 */
    reportException(Error_Program_unreadable_output_error, File);
  ProtectedObject p(Method);
  FlatBuffer = Method->saveMethod();   /* flatten the method                */
  ProtectedObject p2(FlatBuffer);
                                       /* retrieve the length of the buffer */
  BufferLength = (LONG)FlatBuffer->current;
  MethodBuffer = FlatBuffer->buffer;   /* get to the actual data buffer     */
  BufferAddress = MethodBuffer->address();  /* retrieve buffer starting address  */
                                       /* clear out the cntrol info         */
  memset((void *)&Control, 0, sizeof(Control));
                                       /* fill in version info              */
  memcpy(Control.RexxVersion, VERPRE, LENPRE);
  Version = version_number();          /* get the version string            */
  strcpy((char *)Control.RexxVersion + LENPRE, Version->getStringData());
  Control.MetaVersion = METAVERSION;   /* current meta version              */
  Control.Magic = MAGIC;               /* magic signature number            */
  Control.ImageSize = BufferLength;    /* add the buffer length             */
  {
      UnsafeBlock;

      fwrite(compiledHeader, 1, sizeof(compiledHeader), Handle);
                                           /* now the control info              */
      fwrite(&Control, 1, sizeof(Control), Handle);
                                           /* and finally the flattened method  */
      fwrite(BufferAddress, 1, BufferLength, Handle);
      fclose(Handle);                      /* done saving                       */
  }
}
/*********************************************************************/
/*                                                                   */
/*  Function:    SysRestoreTranslatedProgram                         */
/*                                                                   */
/*  Description: This function is used to load a flattened method    */
/*               from a file into the proper interpreter variables.  */
/*                                                                   */
/*********************************************************************/

RexxMethod *SysRestoreTranslatedProgram(
  RexxString *FileName,                /* name of file to process           */
  FILE       *Handle )                 /* handle of the file to process     */
{
  FILE_CONTROL  Control;               /* control information               */
  char         *StartPointer;          /* start of buffered method          */
  RexxBuffer   *Buffer;                /* Buffer to unflatten               */
  size_t        BufferSize;            /* size of the buffer                */
  size_t        BytesRead;             /* actual bytes read                 */
  RexxMethod   *Method;                /* unflattened method                */
  RexxSource   *Source;                /* REXX source object                */
                                       /* temporary read buffer             */
  char          fileTag[sizeof(compiledHeader)];

  {
      UnsafeBlock releaser;
                                           /* read the first file part          */
      BytesRead = fread(fileTag, 1, sizeof(compiledHeader), Handle);
                                           /* not a compiled file?              */
      if (strcmp(fileTag, compiledHeader) != 0) {
        fclose(Handle);                    /* close the file                    */
        return OREF_NULL;                  /* not a saved program               */
      }
                                           /* now read the control info         */
      BytesRead = fread((char *)&Control, 1, sizeof(Control), Handle);
  }
                                       /* check the control info            */
  if ((Control.MetaVersion != METAVERSION) || (Control.Magic != MAGIC)) {
    fclose(Handle);                    /* close the file                    */
                                       /* got an error here                 */
    reportException(Error_Program_unreadable_version, FileName);
  }
                                       /* read the file size                */
  BufferSize = Control.ImageSize;      /* get the method info size          */
  Buffer = new_buffer(BufferSize);     /* get a new buffer                  */
  ProtectedObject p1(Buffer);
                                       /* position relative to the end      */
  StartPointer = ((char *)Buffer + Buffer->getObjectSize()) - BufferSize;
  {
      UnsafeBlock releaser;
                                           /* read the flattened method         */
      BytesRead = fread(StartPointer, 1, BufferSize, Handle);
      fclose(Handle);                      /* close the file                    */
  }
                                       /* "puff" this out usable form       */
  Method = TheMethodClass->restore(Buffer, StartPointer);
  ProtectedObject p2(Method);
  Source = ((RexxCode*)Method->getCode())->getSourceObject();   /* and now the source object         */
                                       /* switch the file name (this might  */
                                       /* be different than the name        */
  Source->setProgramName(FileName);    /* originally saved under            */
  return Method;                       /* return the unflattened method     */
}



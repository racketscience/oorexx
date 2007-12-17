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
#include <windows.h>
#include <rexx.h>
#include <stdio.h>
#include <dlgs.h>
#include <malloc.h>
#include "oovutil.h"


ULONG APIENTRY FindTheWindow(
  PUCHAR funcname,
  ULONG argc,
  RXSTRING argv[],
  PUCHAR qname,
  PRXSTRING retstr )
{
   ULONG hW;

   CHECKARG(1);

   hW = (ULONG) FindWindow(NULL, argv[0].strptr);
   if (hW)
   {
      ltoa(hW, retstr->strptr, 10);
      retstr->strlength = strlen(retstr->strptr);
      return 0;
   }
   RETC(0)  /* in this case 0 is an error */
}

/**
 * Gets the window handle of the dialog control that has the focus.  The call to
 * GetFocus() needs to run in the window thread of the dialog to ensure that the
 * correct handle is obtained.
 *
 * @param hDlg    Handle to the dialog of interest.
 * @param retstr  Rexx string to return the result in.
 */
static void getCurrentFocus( HWND hDlg, PRXSTRING retstr )
{
   ULONG hW = (ULONG)SendMessage(hDlg, WM_USER_GETFOCUS, 0,0);
   ltoa(hW, retstr->strptr, 10);
   retstr->strlength = strlen(retstr->strptr);
}

ULONG APIENTRY Wnd_Desktop(
  PUCHAR funcname,
  ULONG argc,
  RXSTRING argv[],
  PUCHAR qname,
  PRXSTRING retstr )

{
   ULONG hW;

   CHECKARGL(1);

   if (!strcmp(argv[0].strptr,"TXT"))      /* get the window text/title */
   {
       CHECKARG(2);
       hW = atol(argv[1].strptr);
       if (hW)
       {
           retstr->strlength = GetWindowText((HWND)hW, retstr->strptr, 255);
           return 0;
       }
       retstr->strlength = 0;
       return 0;
   }
   else
   if (!strcmp(argv[0].strptr,"ENABLE"))    /* enable/disable the window */
   {
       ULONG st;

       CHECKARG(3);
       hW = atol(argv[1].strptr);

       if (atoi(argv[2].strptr))
       {
          if (IsWindowEnabled((HWND) hW))
              st = 0;
          else st = EnableWindow((HWND) hW, TRUE);
       }
       else
       {
          if (!IsWindowEnabled((HWND) hW))
             st = 0;
          else st = EnableWindow((HWND) hW, FALSE);
       }
       RETVAL(st)
   }
   else
   if (!strcmp(argv[0].strptr,"SETTXT"))     /* set the window text/title */
   {
       CHECKARG(3);

       RETC(!SetWindowText((HWND)atol(argv[1].strptr), argv[2].strptr))
   }
   else
   if (!strcmp(argv[0].strptr,"SETFOC"))
   {
       CHAR qualifier = 'C';
       LRESULT result;
       HWND hDlg, hNextFocus;
       CHECKARGL(3);

       hDlg = (HWND)atol(argv[1].strptr);
       if ( ! IsWindow(hDlg) )
          RETVAL(-1)

       /* Get the handle of the window control that had the focus before setting
        * the new focus.  This will be returned to the caller in retstr.
        */
       getCurrentFocus(hDlg, retstr);

       if ( argc > 3 )
          qualifier = argv[3].strptr[0];

       switch ( qualifier )
       {
          case 'F' :  /* set window to Foreground like Wnd_Desktop("TOP"..) but returns hwnd of previous control */
             hNextFocus = (HWND)atol(argv[2].strptr);
             if ( ! IsWindow(hNextFocus) )
                RETVAL(-1)
             result = SetForegroundWindow(hNextFocus);
             break;

          case 'N' :  /* set focus on Next tab stop control */
             result = SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)0, (LPARAM)FALSE);
             break;

          case 'P' :  /* set focus on Previous tab stop control */
             result = SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)1, (LPARAM)FALSE);
             break;

          case 'C' :  /* set focus on specified Control */
          default  :
             hNextFocus = (HWND)atol(argv[2].strptr);
             if ( ! IsWindow(hNextFocus) )
                RETVAL(-1)
             result = SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)hNextFocus, TRUE);
             break;
       }
       if ( ! result )
          RETVAL(-1)  /* change retstr to indicate failure */
       else
          return 0;   /* retstr has handle of control that previously had the focus */
   }
   else
   if (!strcmp(argv[0].strptr,"GETFOC"))
   {
       CHECKARG(2);

       /* return the handle of the window control that has the focus */
       getCurrentFocus((HWND)atol(argv[1].strptr), retstr);
       return 0;
   }
   else
   if (!strcmp(argv[0].strptr,"TOP"))       /* put window to the foreground */
   {
       CHECKARG(2);
       hW = (ULONG) GetForegroundWindow();
       if (hW == (ULONG)atol(argv[1].strptr)) RETVAL(hW)
       if ( SetForegroundWindow((HWND)atol(argv[1].strptr)) )
           RETVAL(hW)  /* return the handle of the window that had the focus before */
       else
           RETVAL(0)   /* indicate failure */
   }
   else
   if (!strcmp(argv[0].strptr,"FG"))       /* get foreground window */
   {
       RETVAL((ULONG) GetForegroundWindow())
   }
   else
   if (!strcmp(argv[0].strptr,"RECT"))     /* get the window pos and size */  /* same as WindoRect("Get",hw) but in plain */
   {
       RECT r;
       CHECKARG(2);
       retstr->strlength = 0;

       hW = atol(argv[1].strptr);
       if (hW)
       {
          if (!GetWindowRect((HWND)hW, &r)) return 0;
          sprintf(retstr->strptr, "%d %d %d %d", r.left, r.top, r.right, r.bottom);
          retstr->strlength = strlen(retstr->strptr);
       }
       return 0;
   }
   else
   if (!strcmp(argv[0].strptr,"ID"))     /* get the window id */
   {
       CHECKARG(2);
       hW = atol(argv[1].strptr);
       if (hW)
       {
          RETVAL((ULONG)GetWindowLong((HWND)hW, GWL_ID))
       }
       RETC(0);
   }
   else
   if (!strcmp(argv[0].strptr,"CAP"))     /* get/set/release the mouse capture */
   {   /* capture must be handled by window thread, therefore sendmessage is used */
       CHECKARG(3);
       if (argv[2].strptr[0] == 'G')
           RETVAL((ULONG)SendMessage((HWND)atol(argv[1].strptr), WM_USER_GETSETCAPTURE, 0,0))
       else
       if (argv[2].strptr[0] == 'R')
           RETC(!SendMessage((HWND)atol(argv[1].strptr), WM_USER_GETSETCAPTURE, 2,0))
       else {
           hW = atol(argv[2].strptr);
           if (hW)
           {
              RETVAL(SendMessage((HWND)atol(argv[1].strptr), WM_USER_GETSETCAPTURE, 1,hW))
           }
           RETC(0)
       }
       RETERR
   }
   else
   if (!strcmp(argv[0].strptr,"CURSOR"))     /* get/set the mouse cursor */
   {
       if (argc == 1)
       {
           POINT pt;
           GetCursorPos(&pt);
           retstr->strlength = sprintf(retstr->strptr, "%ld %ld", pt.x, pt.y);
           return 0;
       }
       else
       if (argc == 3)
       {
           RETC(!SetCursorPos(atol(argv[1].strptr), atol(argv[2].strptr)))
       }
       else
       if (argc == 4)
       {
           hW = atol(argv[1].strptr);
           if (argv[2].strptr[0] == 'S')
           {
               LONG res;
               HCURSOR oC, hC;
               res = atoi(argv[3].strptr);
               hC = LoadCursor(NULL, MAKEINTRESOURCE(res));
               oC = (HCURSOR)SetClassLong((HWND)hW, GCL_HCURSOR, (LONG)hC);
               SetCursor(hC);
               RETVAL((ULONG)oC)
           }
           else if (argv[2].strptr[0] == 'R')
           {
               HCURSOR hC = (HCURSOR)atol(argv[3].strptr);
               if (hC) {
                   SetClassLong((HWND)hW, GCL_HCURSOR, (LONG)hC);
                   RETVAL((ULONG)SetCursor(hC))
               }
               else
               {
                   SetClassLong((HWND)hW, GCL_HCURSOR, (LONG)LoadCursor(NULL, IDC_ARROW));
                   RETVAL((ULONG)SetCursor(LoadCursor(NULL, IDC_ARROW)))
               }
           }
       }
       RETERR
   }
   else
   if (!strcmp(argv[0].strptr,"KSTAT"))     /* key state and mouse buttons*/
   {    /* keystate must be handled by window thread, therefore sendmessage is used */
       CHECKARG(3);
       hW = atol(argv[1].strptr);
       if (hW)
       {
           LONG k = atoi(argv[2].strptr);
           if (GetSystemMetrics(SM_SWAPBUTTON))
           {
               if (k == 1) k = 2;
               else if (k==2) k = 1;
           }
           RETVAL((ULONG)SendMessage((HWND)hW, WM_USER_GETKEYSTATE, k,0))
       }
       RETC(0)
   }
   else
   if (!strcmp(argv[0].strptr,"COORD"))     /* screen to client and  vice versa */
   {
       CHECKARG(5);
       hW = atol(argv[1].strptr);
       retstr->strptr[0] = '\0';
       retstr->strlength = 0;
       if (hW)
       {
           POINT pt;
           BOOL ret;
           pt.x = atol(argv[2].strptr);
           pt.y = atol(argv[3].strptr);
           if (!stricmp(argv[4].strptr, "S2C"))
               ret = ScreenToClient((HWND)hW, &pt);
           else
               ret = ClientToScreen((HWND)hW, &pt);
           if (ret)
           {
               retstr->strlength = sprintf(retstr->strptr, "%ld %ld", pt.x, pt.y);
               return 0;
           }
       }
       return 0;
   }
   RETERR
}




ULONG APIENTRY WndShow_Pos(
  PUCHAR funcname,
  ULONG argc,
  RXSTRING argv[],
  PUCHAR qname,
  PRXSTRING retstr )

{
   RECT r;
   ULONG st;
   INT k;
   HWND w;
   DEF_ADM;

   CHECKARGL(3);

   if (argv[0].strptr[0]=='S')          /* show, update, redraw a window */
   {
       w = (HWND)atol(argv[1].strptr);
       if (argc == 4)
       {
           dlgAdm = (DIALOGADMIN*)atol(argv[3].strptr);
           dlgAdm->AktChild = w;
       }

       if (strstr(argv[2].strptr, "REDRAW"))
       {
           if (RedrawWindow(w, NULL, NULL, RDW_ERASE | RDW_ALLCHILDREN))
               RETC(0) else RETC(1)
       }
       else
       if (strstr(argv[2].strptr, "UPDATE"))
       {
          GetClientRect(w, &r);
          InvalidateRect(w, &r, TRUE);
          RETC(0)
       }
       else
       if (strstr(argv[2].strptr, "FAST"))
       {
           st = GetWindowLong(w, GWL_STYLE);
           if (st)
           {
               if (strstr(argv[2].strptr, "HIDE"))
                   st ^= WS_VISIBLE;
               else
                   st |= WS_VISIBLE;
               SetWindowLong(w, GWL_STYLE, st);
               RETC(0)
           }
           RETC(1)
       }
       else
       {
           /* SW_SHOWDEFAULT has no meaning here, there is no STARTUPINFO struct
            * from CreateProcess that relates to ooDialog dialogs or dialog
            * controls.  For historical reasons, DEFAULT and NORMAL mean the
            * same thing.
            */
           if (strstr(argv[2].strptr, "HIDE")) k = SW_HIDE; else
           if (strstr(argv[2].strptr, "NORMAL") || strstr(argv[2].strptr, "DEFAULT")) k = SW_SHOWNORMAL; else
           if (strstr(argv[2].strptr, "MIN")) k = SW_SHOWMINIMIZED; else
           if (strstr(argv[2].strptr, "MAX")) k = SW_SHOWMAXIMIZED; else
           if (strstr(argv[2].strptr, "RESTORE")) k = SW_RESTORE; else
           if (strstr(argv[2].strptr, "INACTIVE")) k = SW_SHOWNA; else
           k = SW_SHOW;

           if (ShowWindow(w, k))
              RETC(0)
           else
              RETC(1)
       }
   }
   else
   if (argv[0].strptr[0]=='P')        /* move or resize a window */
   {
       LONG ibuffer[5], opts;
       register int i;

       CHECKARGL(6);

       for (i=0;i<5;i++)
           ibuffer[i] = atoi(argv[i+1].strptr);

       opts = 0;
       if (argc > 6)
       {
          if (strstr(argv[6].strptr, "NOMOVE")) opts |= SWP_NOMOVE;
          if (strstr(argv[6].strptr, "NOSIZE")) opts |= SWP_NOSIZE;
          if (strstr(argv[6].strptr, "HIDEWINDOW")) opts |= SWP_HIDEWINDOW;
          if (strstr(argv[6].strptr, "SHOWWINDOW")) opts |= SWP_SHOWWINDOW;
          if (strstr(argv[6].strptr, "NOREDRAW")) opts |= SWP_NOREDRAW;
       }

       RETC(!SetWindowPos(
        (HWND)ibuffer[0],    // window handle
        0,  // for z-order (ignored)
        ibuffer[1],    // x
        ibuffer[2],    // y
        ibuffer[3],    // width
        ibuffer[4],    // height
        opts | SWP_NOZORDER
       ))
   }
   else
   if (argv[0].strptr[0] == 'B')   /* scroll position */
   {
       CHECKARGL(3);
       if (argc == 3)
       {
           w = (HWND)atol(argv[1].strptr);
           RETVAL(GetScrollPos(w, atoi(argv[2].strptr)))  /* SB_HORZ = 0, SB_VERT = 1 */
       }
       else
       if (argc == 5)
       {
           w = (HWND)atol(argv[1].strptr);
           RETVAL(SetScrollPos(w, atoi(argv[2].strptr), atoi(argv[3].strptr), IsYes(argv[4].strptr)))  /* SB_HORZ = 0, SB_VERT = 1 */
       }
       RETERR
   }
   else
   if (argv[0].strptr[0] == 'M')   /* scroll window contents */
   {
       CHECKARG(4);
       w = (HWND)atol(argv[1].strptr);
       RETC(!ScrollWindow(w, atol(argv[2].strptr), atol(argv[3].strptr), NULL, NULL))  /* x, y */
   }
   RETERR
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* Copyright (c) 1995, 2004 IBM Corporation. All rights reserved.             */
/* Copyright (c) 2005-2009 Rexx Language Association. All rights reserved.    */
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
#include "ooDialog.hpp"     // Must be first, includes windows.h and oorexxapi.h

#include <shlwapi.h>
#include <stdio.h>
#include <dlgs.h>
#include <malloc.h>
#ifdef __CTL3D
#include <ctl3d.h>
#endif
#include "oodCommon.hpp"
#include "APICommon.hpp"
#include "oodText.hpp"

HANDLE TimerEvent = NULL;
ULONG TimerCount = 0;
ULONG_PTR Timer = 0;

int getWeight(CSTRING opts)
{
    int weight = FW_NORMAL;

    if (StrStrI(opts, "THIN")) weight = FW_THIN; else
    if (StrStrI(opts, "EXTRALIGHT")) weight = FW_EXTRALIGHT; else
    if (StrStrI(opts, "LIGHT")) weight = FW_LIGHT; else
    if (StrStrI(opts, "MEDIUM")) weight = FW_MEDIUM; else
    if (StrStrI(opts, "SEMIBOLD")) weight = FW_SEMIBOLD; else
    if (StrStrI(opts, "EXTRABOLD")) weight = FW_EXTRABOLD; else
    if (StrStrI(opts, "BOLD")) weight = FW_BOLD; else
    if (StrStrI(opts, "HEAVY")) weight = FW_HEAVY;
    return weight;
}



VOID CALLBACK ScrollTimerProc(
    HWND  hwnd,    // handle of window for timer messages
    UINT  uMsg,    // WM_TIMER message
    UINT  idEvent,    // timer identifier
    DWORD  dwTime     // current system time
   )
{
    SetEvent(TimerEvent);
}



size_t RexxEntry ScrollText(const char *funcname, size_t argc, CONSTRXSTRING *argv, const char *qname, RXSTRING *retstr)
{
    INT disply;
    const char *opts;
    INT size;
    const char *text;
    INT col;

    const char * tp;
    HWND w;
    HDC hDC;
    HFONT hFont, oldF;
    HPEN hpen, oP;
    HBRUSH oB, hbr;
    RECT r, rs, rclip;
    SIZE s, sone;
    INT i, rc, sl, step, j, disp, weight;
    UINT sleep;
    DEF_ADM;

    CHECKARG(10);

    GET_ADM;

    if (!dlgAdm) RETERR

    text = argv[2].strptr;
    size = atoi(argv[4].strptr);
    opts = argv[5].strptr;
    disply = atoi(argv[6].strptr);
    col = atoi(argv[9].strptr);

    w = GET_HWND(argv[1]);
    step = atoi(argv[7].strptr);
    tp = text;

    if (NULL != (hDC = GetWindowDC(w)))
    {
        GetWindowRect(w, &r);

        weight = getWeight(opts);

        hFont = CreateFont(size, size, 0, 0, weight, strstr(opts, "ITALIC") != NULL, strstr(opts, "UNDERLINE") != NULL,
                           strstr(opts, "STRIKEOUT") != NULL, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, FF_DONTCARE, argv[3].strptr);

        oldF = (HFONT)SelectObject(hDC, hFont);

        hpen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));
        hbr = GetSysColorBrush(COLOR_BTNFACE);

        SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
        oP = (HPEN)SelectObject(hDC, hpen);
        oB = (HBRUSH)SelectObject(hDC, hbr);

        if (col > 0) SetTextColor(hDC, PALETTEINDEX(col));
        sl = (int)strlen(text);
        rc = GetTextExtentPoint32(hDC, text, sl, &s);

        r.right = r.right - r.left;
        r.bottom = r.bottom - r.top + disply;
        r.top = disply;
        r.left = 0;

        j = 0;
        disp = 0;
        rc = GetTextExtentPoint32(hDC, tp, 1, &sone);
        rclip= r;
        rs.top = r.top;
        rs.bottom = r.top+s.cy+2;
        rs.right = r.right;
        rs.left = r.right;

        sleep = atoi(argv[8].strptr);
        if (sleep)
        {
            if (!TimerEvent)
            {
                TimerEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
                Timer = SetTimer(NULL,GetCurrentThreadId(), sleep, (TIMERPROC)ScrollTimerProc);
                TimerCount++;
            }
            else TimerCount++;
        }
        else Timer = 0;

        ScrollingButton = w;
        for (i=step; i<=r.right+s.cx; i+=step)
        {
            if (i>=s.cx+step)
            {
                Rectangle(hDC, r.right-i+s.cx, r.top, r.right -i+s.cx+step+step, r.top + s.cy + 2);
            }

            if (j<sl)
            {
                if (RedrawScrollingButton == w)
                {
                    rc = TextOut(hDC, r.right - i, r.top, text, sl);
                }
                else
                {
                    rc = TextOut(hDC, r.right - i+disp, r.top, tp, 1);
                }
            }
            if ((j<sl) && (!rc)) break;

            RedrawScrollingButton = NULL;

            if (i-disp>sone.cx)
            {
                tp++;
                j++;
                disp += sone.cx;
                rc = GetTextExtentPoint32(hDC, tp, 1, &sone);
            }

            rs.left -= step;
            rc = 0;

            if (dlgAdm->StopScroll == (WPARAM) w)
            {
                dlgAdm->StopScroll = 0;
                break;
            }

            if (!ScrollDC(hDC, -step, 0, &rs, &rclip, NULL, NULL)) break;

            if (Timer)
            {
                WaitForSingleObject(TimerEvent, (DWORD)((double)sleep*1.5));
                ResetEvent(TimerEvent);
            }

        }

        if (Timer)
        {
            if (TimerCount == 1)
            {
                KillTimer(NULL, Timer);
                if (TimerEvent) CloseHandle(TimerEvent);
                TimerEvent = NULL;
                TimerCount = 0;
                Timer = 0;
            }
            else TimerCount--;
        }

        ScrollingButton = NULL;

        if (!dlgAdm || (!IsWindow(dlgAdm->TheDlg)))
        {
            RETC(1);
        }
        Rectangle(hDC, r.left, r.top, r.right, r.bottom);
        SelectObject(hDC, oldF);
        SelectObject(hDC, oP);
        SelectObject(hDC, oB);

        // Don't delete hbr, its a system cached brush
        DeleteObject(hpen);
        DeleteObject(hFont);
        ReleaseDC(w, hDC);
        RETC(0);
    }
    RETC(1);
}


HFONT createFontFromName(CSTRING name, uint32_t size)
{
    HDC hdc = GetDC(NULL);
    HFONT font = createFontFromName(GetDeviceCaps(hdc, LOGPIXELSY), name, size);
    ReleaseDC(NULL, hdc);
    return font;
}

HFONT createFontFromName(int logicalPixelsY, CSTRING name, uint32_t size)
{
    LOGFONT lf={0};

    strcpy(lf.lfFaceName, name);
    lf.lfHeight = -MulDiv(size, logicalPixelsY, 72);
    return CreateFontIndirect(&lf);
}


/**
 * Correctly converts from a device coordinate (pixel) to a dialog unit
 * coordinate, for any dialog.
 *
 * MapDialogRect() correctly converts from dialog units to pixels for any
 * dialog.  But, there is no conversion the other way, from pixels to dialog
 * units.
 *
 * MSDN gives these formulas to convert from pixel to dialog unit:
 *
 * templateunitX = MulDiv(pixelX, 4, baseUnitX);
 * templateunitY = MulDiv(pixelY, 8, baseUnitY);
 *
 * Now, you just need to get the correct dialog base unit.
 *
 * GetDialogBaseUnits() always assumes the font is the system font.  If the
 * dialog uses any other font, the base units returned will be incorrect.
 *
 * MSDN, again, has two methods for calculating the correct base units for any
 * font.  This way is the simplest, but it requires the window handle to the
 * dialog.
 *
 * Rect rect{0, 0, 4, 8};
 * MapDialogRect(hDlg, &rc);
 * int baseUnitY = rc.bottom;
 * int baseUnitX = rc.right;
 *
 * @param hwnd   Window handle of the dialog.  If this is not a dialog window
 *               handle, this method will fail.
 *
 * @param point  Pointer to an array of POINT structs.  Not that a SIZE struct
 *               and a POINT struct are binary equivalents.  They both have two
 *               fields, each of which is a long.  Only the field names differ,
 *               cx and cy for a SIZE and x and y for a POINT.  Therefore you
 *               can cast a SIZE pointer to a POINT pointer.
 *
 * @param count  The number of point structs in the array.
 *
 * @return true on success, false otherwise.
 *
 * Dialog class: #32770
 */
bool screenToDlgUnit(HWND hwnd, POINT *point, size_t count)
{
    RECT r = {0, 0, 4, 8};

    if ( MapDialogRect(hwnd, &r) )
    {
        for ( size_t i = 0; i < count; i++ )
        {
            pixel2du(point + i, r.right, r.bottom);
        }
        return true;
    }
    return false;
}

/**
 * Given a device context with the correct font already selected into it,
 * correctly converts from a device coordinate (pixel) to a dialog unit
 * coordinate.  The correct font means, the font actually used by the dialog.
 *
 * See screenToDlgUnit(HWND, POINT *) for a discussion of this
 * conversion.
 *
 * @param hdc    Handle to a device context with the dialog's font selected into
 *               it.
 *
 * @param point  Pointer to an array of POINT structs.  Not that a SIZE struct
 *               and a POINT struct are binary equivalents.  They both have two
 *               fields, each of which is a long.  Only the field names differ,
 *               cx and cy for a SIZE and x and y for a POINT.  Therefore you
 *               can cast a SIZE pointer to a POINT pointer.
 *
 * @param count  The number of point structs in the array.
 *
 * @return true on success, false otherwise.
 *
 */
void screenToDlgUnit(HDC hdc, POINT *point, size_t count)
{
    TEXTMETRIC tm;
    SIZE size;
    GetTextMetrics(hdc, &tm);
    int baseUnitY = tm.tmHeight;

    GetTextExtentPoint32(hdc, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &size);
    int baseUnitX = (size.cx / 26 + 1) / 2;

    for ( size_t i = 0; i < count; i++ )
    {
        pixel2du(point + i, baseUnitX, baseUnitY);
    }
}


/**
 * Calculates the dialog base units using the window handle of the dialog.
 *
 * @param hDlg       The window handle of the dialog whose base units are to be
 *                   calculated.
 * @param baseUnitX  The X base unit is returned here.
 * @param baseUnitY  The Y base unit is returned here.
 */
void calcDlgBaseUnits(HWND hDlg, int *baseUnitX, int *baseUnitY)
{
    RECT r = {0, 0, 4, 8};

    MapDialogRect(hDlg, &r);
    *baseUnitX = r.right;
    *baseUnitY = r.bottom;
}

/**
 * Calculates the dialog base units for a Rexx dialog object.  The underlying
 * Windows dialog does not need to have been created.  The font name and size
 * are gotten from the Rexx dialog object.
 *
 * The base units are calculated using the font of the dialog.  If the
 * underlying Windows dialog is then created later using a different font, the
 * base units will be incorrect.
 *
 * Typically, the correct way to get the right base units would be something
 * like this:
 *
 * dlg = .MyDialog~new(...)
 * dlg~setDlgFont("Tahoma", 14)
 *
 * @param c          Method context we are operating in.
 * @param fontName   The font name in use for the Rexx dialog object.
 * @param fontSize   The font size in use for the Rexx dialog object.
 * @param baseUnitX  The X base unit is returned here.
 * @param baseUnitY  The Y base unit is returned here.
 *
 * @return True on success, false on failure.  On failure an exception has been
 *         raised.
 *
 * @note  It is presumed that the font name and size come from a Rexx dialog
 *        object.
 *
 * @remarks  Once the dialog has been created use:
 *
 *           calcDlgBaseUnits(HWND, baseUnitX, baseUnitY)
 */
bool calcDlgBaseUnits(RexxMethodContext *c, CSTRING fontName, uint32_t fontSize, int *baseUnitX, int *baseUnitY)
{
    HDC hdc = NULL;
    HFONT font = NULL;
    bool result = false;

    hdc = GetDC(NULL);
    if ( hdc == NULL )
    {
        systemServiceExceptionCode(c->threadContext, API_FAILED_MSG, "GetDC");
        goto done_out;
    }

    font = createFontFromName(hdc, fontName, fontSize);
    if ( font == NULL )
    {
        systemServiceExceptionCode(c->threadContext, API_FAILED_MSG, "CreateFontIndirect");
        goto done_out;
    }

    HFONT hOldFont = (HFONT)SelectObject(hdc, font);

    TEXTMETRIC tm;
    SIZE size;
    GetTextMetrics(hdc, &tm);
    GetTextExtentPoint32(hdc, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 52, &size);

    *baseUnitX = (size.cx / 26 + 1) / 2;
    *baseUnitY = tm.tmHeight;

    result = true;

    SelectObject(hdc, hOldFont);

done_out:
    if ( hdc != NULL )
    {
        ReleaseDC(NULL, hdc);
    }
    if ( font != NULL )
    {
        DeleteObject(font);
    }

    return result;
}

/**
 * Given a Rexx dialog object and an array of points in pixels, maps the points
 * to their dialog unit equivalent.
 *
 * @param  c      Method context we are operating in.
 * @param  dlg    The Rexx dialog object.
 * @param  p      Pointer to the array of points
 * @param  count  Count of points in the array.
 *
 * @return  True on success, false on failure.
 *
 * @assumes  The caller has ensured that dlg is indeed a Rexx dialog object.
 */
bool mapPixelToDu(RexxMethodContext *c, RexxObjectPtr dlg, PPOINT p, size_t count)
{
    pCPlainBaseDialog pcpbd = dlgToCSelf(c, dlg);

    if ( pcpbd->hDlg != NULL )
    {
        return screenToDlgUnit(pcpbd->hDlg, p, count);
    }

    int buX, buY;
    if ( ! calcDlgBaseUnits(c, pcpbd->fontName, pcpbd->fontSize, &buX, &buY) )
    {
        return false;
    }

    for ( size_t i = 0; i < count; i++ )
    {
        pixel2du(p + i, buX, buY);
    }
    return true;
}

/**
 * Given a Rexx dialog object and a rectangle in dialog units, maps the
 * rectangle to pixels
 *
 * @param  c      Method context we are operating in.
 * @param  dlg    The Rexx dialog object.
 * @param  r      Pointer to the rectangle.
 *
 * @return  True on success, false on failure.
 *
 * @assumes  The caller has ensured that dlg is indeed a Rexx dialog object.
 */
bool mapDuToPixel(RexxMethodContext *c, RexxObjectPtr dlg, PRECT r)
{
    oodResetSysErrCode(c->threadContext);

    pCPlainBaseDialog pcpbd = dlgToCSelf(c, dlg);

    if ( pcpbd->hDlg != NULL )
    {
        if ( MapDialogRect(pcpbd->hDlg, r) == 0 )
        {
            oodSetSysErrCode(c->threadContext);
            return false;
        }
        return true;
    }

    int buX, buY;
    if ( ! calcDlgBaseUnits(c, pcpbd->fontName, pcpbd->fontSize, &buX, &buY) )
    {
        return false;
    }

    r->left   = MulDiv(r->left,   buX, 4);
    r->right  = MulDiv(r->right,  buX, 4);
    r->top    = MulDiv(r->top,    buY, 8);
    r->bottom = MulDiv(r->bottom, buY, 8);
    return true;
}

/**
 * Uses GetTextExtentPoint32() to get the size needed for a string using the
 * specified font and device context.
 *
 * @param font   The font being used for the string.
 * @param hdc    The device context to use.
 * @param text   The string.
 * @param size   Pointer to a SIZE struct used to return the size.
 *
 * @return True if  GetTextExtentPoint32() succeeds, otherwise false.
 *
 * @note   GetTextExtentPoint32() sets last error and SelectObject() does not.
 *         Therefore if this function fails, GetLastError() will return the
 *         correct error code for the failed GetTextExtentPoint32().
 */
bool getTextExtent(HFONT font, HDC hdc, CSTRING text, SIZE *size)
{
    bool success = true;
    HFONT hOldFont = (HFONT)SelectObject(hdc, font);

    if ( GetTextExtentPoint32(hdc, text, (int)strlen(text), size) == 0 )
    {
        success = false;
    }
    SelectObject(hdc, hOldFont);
    return success;
}

bool textSizeIndirect(RexxMethodContext *context, CSTRING text, CSTRING fontName, uint32_t fontSize,
                      SIZE *size, HWND hwnd)
{
    bool success = true;

    // If hwnd is null, GetDC() returns a device context for the whole screen,
    // and that suites our purpose here.
    HDC hdc = GetDC(hwnd);
    if ( hdc == NULL )
    {
        systemServiceExceptionCode(context->threadContext, API_FAILED_MSG, "GetDC");
        return false;
    }

    HFONT font = createFontFromName(hdc, fontName, fontSize);
    if ( font == NULL )
    {
        systemServiceExceptionCode(context->threadContext, API_FAILED_MSG, "CreateFontIndirect");
        ReleaseDC(hwnd, hdc);
        return false;
    }

    if ( ! getTextExtent(font, hdc, text, size) )
    {
        systemServiceExceptionCode(context->threadContext, API_FAILED_MSG, "GetTextExtentPoint32");
        success = false;
    }

    DeleteObject(font);
    ReleaseDC(hwnd, hdc);

    return success;
}

bool textSizeFromWindow(RexxMethodContext *context, CSTRING text, SIZE *size, HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    if ( hdc == NULL )
    {
        systemServiceExceptionCode(context->threadContext, API_FAILED_MSG, "GetDC");
        return false;
    }

    // Dialogs and controls need to have been issued a WM_SETFONT or else they
    // return null here.  If null, they are using the stock system font.
    HFONT font = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    if ( font == NULL )
    {
        font = (HFONT)GetStockObject(SYSTEM_FONT);
    }

    bool success = true;
    if ( ! getTextExtent(font, hdc, text, size) )
    {
        systemServiceExceptionCode(context->threadContext, API_FAILED_MSG, "GetTextExtentPoint32");
        success = false;
    }

    ReleaseDC(hwnd, hdc);
    return success;
}


/**
 * Retrieves the size needed, in dialog units, to display a given text string in
 * a dialog.
 *
 * This function first retrieves the size needed for the text in pixels, then
 * accurately converts the pixel size to the dialog unit size for the specified
 * dialog.
 *
 * @param context
 * @param text
 * @param fontName
 * @param fontSize
 * @param hwndFontSrc
 * @param dlgObj
 *
 * @return RexxObjectPtr
 */
bool getTextSize(RexxMethodContext *context, CSTRING text, CSTRING fontName, uint32_t fontSize,
                 HWND hwndFontSrc, RexxObjectPtr dlgObj, PSIZE textSize)
{
    pCPlainBaseDialog pcpbd = dlgToCSelf(context, dlgObj);

    // hwndDlg can be null if this is happening before the real dialog is created.
    HWND hwndDlg = pcpbd->hDlg;

    // See if we have a real window handle to use for the call to GetDC().  If
    // both hwndFontSrc and hwndDlg are null, that's okay, we can use null.
    HWND hwndForDC = (hwndFontSrc != NULL ? hwndFontSrc : hwndDlg);

    // If either the font name or the font source window handle were specified,
    // we calculate the text size in pixels now.  The normal case is that the
    // font is coming from the dialog object.
    if ( fontName != NULL )
    {
        if ( ! textSizeIndirect(context, text, fontName, fontSize, textSize, hwndForDC) )
        {
            goto error_out;
        }
    }
    else if ( hwndFontSrc != NULL )
    {
        if ( ! textSizeFromWindow(context, text, textSize, hwndFontSrc) )
        {
            goto error_out;
        }
    }

    // Even if we have already caclulated the text size above, we always have to
    // get the dialog font and select it into a HDC to correctly convert the
    // pixel size to the dialog unit size.
    HDC hdc = GetDC(hwndForDC);
    if ( hdc == NULL )
    {
        systemServiceExceptionCode(context->threadContext, API_FAILED_MSG, "GetDC");
        goto error_out;
    }

    HFONT dlgFont = NULL;
    bool createdFont = false;

    if ( hwndDlg == NULL )
    {
        dlgFont = createFontFromName(hdc, pcpbd->fontName, pcpbd->fontSize);
        if ( dlgFont != NULL )
        {
            createdFont = true;
        }
    }
    else
    {
        dlgFont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);
    }

    // If dlgFont is null, then, (almost for sure,) the dialog will be using the
    // default system font.  We use that font for the rest of the calculations.
    // This may be inacurrate, but we have to use some font.
    //
    // If the user has called getTextSizeDlg() method before the create()
    // method, and then defines a custom font in create(), then this will be
    // inaccurate for sure.  Need to explain in the docs how to correctly use
    // this functionality.
    if ( dlgFont == NULL )
    {
        dlgFont = (HFONT)GetStockObject(SYSTEM_FONT);
    }

    HFONT hOldFont = (HFONT)SelectObject(hdc, dlgFont);

    // Check if the pixel text size has been determined above.  The normal case
    // will be that it has not.  The normal case is that the size is determined
    // here using the DC with the dialog font selected into it.
    if ( textSize->cx == 0 )
    {
        GetTextExtentPoint32(hdc, text, (int)strlen(text), textSize);
    }

    // Now, convert the pixel size to dialog unit size, and clean up.
    screenToDlgUnit(hdc, (POINT *)textSize, 1);

    SelectObject(hdc, hOldFont);
    ReleaseDC(hwndForDC, hdc);

    if ( createdFont )
    {
        DeleteObject(dlgFont);
    }

    return true;

error_out:
    return false;
}



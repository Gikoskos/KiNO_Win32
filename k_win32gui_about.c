/* k_win32gui_about.c
 * Code for the About dialog goes here
 * 
 * Copyright (C) George Koskeridis 2016
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
  ****************/

#include "k_win32gui.h"


/* Static function declarations */
static INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);



VOID InitAboutDialog(HWND hwnd)
{
    if (!DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT_DLG), hwnd, AboutDialogProcedure, (LPARAM)0)) {
        MSGBOX_ERR(hwnd, L"DialogBoxParam");
    }
}

INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT DlgFont = NULL;
    static HWND libxmllink = NULL, libcurllink = NULL, compilertext = NULL, authorlink = NULL;

    switch (msg) {
    case WM_INITDIALOG:
    {
        HICON AboutDlgIcon = LoadIcon(NULL, IDI_INFORMATION);
        if (AboutDlgIcon) {
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)AboutDlgIcon);
        }

        wchar_t tmp[512];

        DlgFont = CreateFont(12, 0, 0, 0, 400,
                             FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                             CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                             VARIABLE_PITCH, TEXT("Ms Shell Dlg"));
        if (!DlgFont) {
            MSGBOX_ERR(NULL, L"CreateFont");
            EndDialog(hwnd, (INT_PTR)FALSE);
        }

        libxmllink = CreateWindowEx(0, WC_LINK, L"<a href=\"http://xmlsoft.org/\">libxml2</a>",
                                    WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_LEFT,
                                    25, 175, 120, 14, hwnd, NULL, GetModuleHandle(NULL), NULL);
        if (!libxmllink) {
            MSGBOX_ERR(NULL, L"CreateWindowEx");
            EndDialog(hwnd, (INT_PTR)FALSE);
        }

        LoadStr(tmp, IDSTRING_34);
        libcurllink = CreateWindowEx(0, WC_LINK, tmp, WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_LEFT,
                                     25, 200, 190, 14, hwnd, NULL, GetModuleHandle(NULL), NULL);
        if (!libcurllink) {
            MSGBOX_ERR(NULL, L"CreateWindowEx");
            EndDialog(hwnd, (INT_PTR)FALSE);
        }

        LoadStr(tmp, IDSTRING_35);
        compilertext = CreateWindowEx(0, WC_STATIC, tmp, WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_LEFT,
                                      25, 225, 190, 14, hwnd, NULL, GetModuleHandle(NULL), NULL);
        if (!compilertext) {
            MSGBOX_ERR(NULL, L"CreateWindowEx");
            EndDialog(hwnd, (INT_PTR)FALSE);
        }

        LoadStr(tmp, IDSTRING_8);
        authorlink = CreateWindowEx(0, WC_LINK, tmp, WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_CENTER,
                                    315, 215, 128, 14, hwnd, NULL, GetModuleHandle(NULL), NULL);
        if (!authorlink) {
            MSGBOX_ERR(NULL, L"CreateWindowEx");
            EndDialog(hwnd, (INT_PTR)FALSE);
        }

        SendMessage(libxmllink, WM_SETFONT, (WPARAM)DlgFont, (LPARAM)TRUE);
        SendMessage(libcurllink, WM_SETFONT, (WPARAM)DlgFont, (LPARAM)TRUE);
        SendMessage(compilertext, WM_SETFONT, (WPARAM)DlgFont, (LPARAM)TRUE);
        SendMessage(authorlink, WM_SETFONT, (WPARAM)DlgFont, (LPARAM)TRUE);

        SetAboutDialogLanguage(hwnd);
        CenterChild(hwnd);
        return (INT_PTR)TRUE;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORDLG:
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDOK:
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            return (INT_PTR)TRUE;
        }
        return (INT_PTR)TRUE;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) {
        case NM_RETURN:
        case NM_CLICK:
        {
            HWND clicked = ((LPNMHDR)lParam)->hwndFrom;
            LITEM link = ((PNMLINK)lParam)->item;
            
            if (clicked == libxmllink || clicked == libcurllink ||
                clicked == compilertext || clicked == authorlink) {
                ShellExecute(NULL, L"open", link.szUrl, NULL, NULL, SW_SHOW);
            }
            break;
        }
        default:
            break;
        }
        return (INT_PTR)TRUE;
    case WM_CLOSE:
        DestroyWindow(libxmllink);
        DestroyWindow(libcurllink);
        DestroyWindow(compilertext);
        DestroyWindow(authorlink);
        DeleteObject(DlgFont);
        EndDialog(hwnd, (INT_PTR)TRUE);
        return (INT_PTR)TRUE;
    default:
        break;
    }

    return (INT_PTR)FALSE;
}

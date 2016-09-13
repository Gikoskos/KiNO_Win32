/* k_win32gui_common.c
 * Functions to be used by the KiNO_Win32 GUI
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


int GetFieldTextW(const HWND hwnd, wchar_t *str)
{
    int bufsiz = GetWindowTextLengthW(hwnd) + 1;

    if (bufsiz > 1 && bufsiz < (int)UINT32_MAX) {
        str = k_malloc(bufsiz * sizeof(wchar_t));

        if (!GetWindowTextW(hwnd, str, bufsiz)) {
            MSGBOX_ERR(hwnd, L"GetWindowTextW");
        }
    } else {
        if (str)
            k_free(str);
        str = NULL;
    }

    return bufsiz - 1;
}

int GetFieldTextA(const HWND hwnd, char *str)
{
    int bufsiz = GetWindowTextLengthA(hwnd) + 1;

    if (bufsiz > 1 && bufsiz < (int)UINT32_MAX) {
        str = k_malloc(bufsiz);

        if (!GetWindowTextA(hwnd, str, bufsiz)) {
            MSGBOX_ERR(hwnd, L"GetWindowTextA");
        }
    } else {
        if (str)
            k_free(str);
        str = NULL;
    }

    return bufsiz - 1;//don't include the '\0' character on the return value
}

VOID GetFieldUInt(const HWND hDlg, const int nIDDlgItem, uint32_t *to_store)
{
    BOOL *res = k_malloc(sizeof(BOOL));

    *to_store = (uint32_t)GetDlgItemInt(hDlg, nIDDlgItem, res, FALSE);
    if (res) {
        if (*res == FALSE) {
            MSGBOX_ERR(NULL, L"GetDlgItemInt");
        }
        k_free(res);
    }
}

VOID CenterChild(const HWND hwnd)
{
    HWND hwndOwner = GetParent(hwnd);
    RECT rc, rcDlg, rcOwner;
    INT final_x, final_y;

    if (!hwndOwner) {
        hwndOwner = GetAncestor(hwnd, GA_PARENT);
        if (!hwndOwner)
            hwndOwner = GetDesktopWindow();
    }

    GetWindowRect(hwndOwner, &rcOwner);
    GetWindowRect(hwnd, &rcDlg);
    CopyRect(&rc, &rcOwner);
    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
    OffsetRect(&rc, -rc.left, -rc.top);
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

    final_x = rcOwner.left + (rc.right / 2);
    final_y = rcOwner.top + (rc.bottom / 2);
    if (final_x < 0) final_x = 0;
    if (final_y < 0) final_y = 0;
    SetWindowPos(hwnd, HWND_TOP, final_x, final_y, 0, 0, SWP_NOSIZE);
}

BOOL ResizeDlgControl(const HWND hDlg, const HWND hCtrl, int d_x, int d_y, int d_width, int d_height)
{
    int final_width, final_height;
    POINT upperleft;
    RECT tmp_rect;

    if (!GetWindowRect(hCtrl, &tmp_rect)) { //get coordinates of hCtrl relative to the monitor
        MSGBOX_ERR(hDlg, L"GetWindowRect");
        return FALSE;
    }

    upperleft.x = tmp_rect.left;
    upperleft.y = tmp_rect.top;

    if (!ScreenToClient(hDlg, &upperleft)) { //transform the upper left coordinates to be relative to the window
        MSGBOX_ERR(hDlg, L"ScreenToClient");
        return FALSE;
    }

    final_width = tmp_rect.right - tmp_rect.left;
    final_height = tmp_rect.bottom - tmp_rect.top;


    if (!MoveWindow(hCtrl, upperleft.x + d_x, upperleft.y + d_y,
                    final_width + d_width, final_height + d_height,
                    TRUE)) {
        MSGBOX_ERR(hDlg, L"MoveWindow");
        return FALSE;
    }

    return TRUE;
}

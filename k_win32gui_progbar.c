/*k_win32gui_progbar.c
 * Code for the download Progress Bar dialog goes here
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



static uint32_t min_lottery, max_lottery;
static lottery_data **lotteries;

extern HANDLE ProgressBarEv; //from k_win32gui_main.c

static UINT CALLBACK DownloadsControllerThread(LPVOID dat);
static INT_PTR CALLBACK DownloadProgressBarDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


VOID InitDownloadProgressBarDialog(HWND hwnd, uint32_t min_lott, uint32_t max_lott, lottery_data **lottdat)
{
    min_lottery = min_lott;
    max_lottery = max_lott;
    lotteries = lottdat;

    if (!DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DOWNLOADING_DLG), hwnd, DownloadProgressBarDialogProcedure, (LPARAM)hwnd)) {
        MSGBOX_ERR(hwnd, L"DialogBoxParam");
    }
}

UINT CALLBACK DownloadsControllerThread(LPVOID dat)
{
    HWND hProgressBarDlg = (HWND)dat;
    HWND progress_bar_handle;

    if (!hProgressBarDlg) {
        MsgBoxErrLocal(NULL, IDSTRING_45, IDSTRING_25);
        _endthreadex(1);
    }

    progress_bar_handle = GetDlgItem(hProgressBarDlg, IDC_PROGRESS_BAR);
    if (!progress_bar_handle) {
        MSGBOX_ERR(hProgressBarDlg, L"GetDlgItem");
        goto FAIL;
    }

    if (
        !download_KINO_lotteries(min_lottery, max_lottery, lotteries, progress_bar_handle, NumOfThreads(),
                                 (DeleteFiles() ? DELETE_FILES : 0) |
                                 (ParallelDownloads() ? PARALLEL_DOWNLOADS : 0))
       ) {
        MsgBoxErrLocal(NULL, IDSTRING_46, IDSTRING_25);
        goto FAIL;                        
    }

    SendNotifyMessage(hProgressBarDlg, WM_CLOSE, (WPARAM)0, 0);
    _endthreadex(0);
    return 0;

FAIL:
    if (*lotteries) {
        k_free(*lotteries);
        *lotteries = NULL;
    }
    SendNotifyMessage(hProgressBarDlg, WM_CLOSE, (WPARAM)1, 0);
    _endthreadex(1);
    return 1;
}

INT_PTR CALLBACK DownloadProgressBarDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE hDldThrd;

    switch (msg) {
    case WM_INITDIALOG:
    {
        wchar_t dlgTitle[255];

        LoadStr(dlgTitle, IDSTRING_9);
        SetWindowText(hwnd, dlgTitle);

        CenterChild(hwnd);

        SendMessage(GetDlgItem(hwnd, IDC_PROGRESS_BAR), PBM_SETRANGE, 0, MAKELPARAM(0, (max_lottery - min_lottery) + 1));
        SendMessage(GetDlgItem(hwnd, IDC_PROGRESS_BAR), PBM_SETSTEP, (WPARAM) 1, 0);

        ResetEvent(ProgressBarEv);
        hDldThrd = (HANDLE)_beginthreadex(NULL, 0, DownloadsControllerThread, (LPVOID)hwnd, 0, NULL);
        if (!hDldThrd) {
            MSGBOX_ERR(NULL, L"_beginthreadex");
            EndDialog(hwnd, FALSE);
            return (INT_PTR)FALSE;
        }
        return (INT_PTR)TRUE;
    }
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }
        return DefWindowProc(hwnd, WM_SYSKEYDOWN, wParam, lParam);
    case WM_CLOSE:
        SetEvent(ProgressBarEv);

        if (hDldThrd) {
            WaitForSingleObject(hDldThrd, INFINITE);
            CloseHandle(hDldThrd);
            hDldThrd = NULL;
        }
        EndDialog(hwnd, TRUE);
        return (INT_PTR)TRUE;
    default:
        break;
    }

    return (INT_PTR)FALSE;
}

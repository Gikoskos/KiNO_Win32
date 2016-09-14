/* k_win32gui_settings.c
 * Code for the Settings dialog goes here
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
#include "resource.h"

#define MAX_ALLOWED_RUNNING_THREADS 150

#define MAX_THREADS_W_STR L"150"



static BOOL parallel_d = TRUE, delete_file = FALSE;
static uint32_t max_threads = 30;
static uint8_t prev_lottery_tickets[8][10];

/* Static function declarations */
static INT_PTR CALLBACK SettingsDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL InitializeTabs(HWND tabControl);
static BOOL OnNotify(HWND hwnd, HWND hTabControl, HWND hListViewControl, LPARAM lParam, WPARAM wParam);
static BOOL InitializeTicketNumLV(HWND listViewCtl);
static VOID GetTicketFromCurrentTab(HWND hTabControl, HWND hListViewControl);
static VOID UpdateCurrentListViewTab(HWND hTabControl, HWND hListViewControl);
static VOID OnRandomLotteryGroups(HWND hwnd, HWND hTabControl, HWND hListViewControl);


BOOL DeleteFiles(VOID)
{
    return delete_file;
}

BOOL ParallelDownloads(VOID)
{
    return parallel_d;
}

uint32_t NumOfThreads(VOID)
{
    return max_threads;
}

VOID GetTicketFromCurrentTab(HWND hTabControl, HWND hListViewControl)
{
    int curr_tab = TabCtrl_GetCurSel(hTabControl);
    int sel_item = ListView_GetNextItem(hListViewControl, -1, LVNI_SELECTED);

    for (int i = 0; i < 10; i++) {
        lottery_tickets[curr_tab][i] = (uint8_t)sel_item + 1;
        if (sel_item != -1) {
            sel_item = ListView_GetNextItem(hListViewControl, sel_item, LVNI_SELECTED);
        }
    }
}

VOID UpdateCurrentListViewTab(HWND hTabControl, HWND hListViewControl)
{
    int curr_tab = TabCtrl_GetCurSel(hTabControl);

    ListView_SetItemState(hListViewControl, -1, 0, LVIS_SELECTED);
    for (int i = 0; i < 10; i++) {
        if (!lottery_tickets[curr_tab][i]) break;

        ListView_SetItemState(hListViewControl, (UINT)lottery_tickets[curr_tab][i] - 1,
                              LVIS_SELECTED, 0x000f);
    }
}

BOOL OnNotify(HWND hwnd, HWND hTabControl, HWND hListViewControl, LPARAM lParam, WPARAM wParam)
{
    LPNMHDR notif_data = (LPNMHDR)lParam;

    switch (notif_data->code) {
    case TCN_SELCHANGING:
    {
        GetTicketFromCurrentTab(hTabControl, hListViewControl);
        return FALSE;
    }
    case TCN_SELCHANGE:
    {
        UpdateCurrentListViewTab(hTabControl, hListViewControl);
        SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)hListViewControl, TRUE);
        break;
    }
    default:
        break;
    }
    return TRUE;
}

VOID OnRandomLotteryGroups(HWND hwnd, HWND hTabControl, HWND hListViewControl)
{
    if (!randomize_lottery_ticket_groups()) {
        MSGBOX_ERRNO(hwnd, L"rand_s");
        reset_lottery_ticket_groups();
    }
    UpdateCurrentListViewTab(hTabControl, hListViewControl);
    SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)hListViewControl, TRUE);
}

BOOL InitializeTabs(HWND tabControl)
{
    wchar_t team_str[25];
    TCITEMW lott_teams = {.mask = TCIF_TEXT, .pszText = team_str};

    LoadStr(team_str, IDSTRING_24);
    for (wchar_t i = L'1'; i <= L'8'; i++) {
        team_str[0] = i;
        if (TabCtrl_InsertItem(tabControl, i - L'1', &lott_teams) == -1) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL InitializeTicketNumLV(HWND listViewCtl)
{
    wchar_t num_str[3];
    HRESULT hRes;
    LVITEM ticket_num = {.mask = LVIF_TEXT | LVIF_STATE, .iSubItem = 0, .pszText = num_str, .state = 0};

    for (int i = 0; i < 80; i++) {
        hRes = StringCchPrintfW(num_str, 3, L"%d", i + 1);
        if (FAILED(hRes)) return FALSE;

        ticket_num.iItem = i;

        if (ListView_InsertItem(listViewCtl, &ticket_num) == -1) return FALSE;
    }

    for (int i = 0; i < 10; i++) {
        if (!lottery_tickets[0][i]) break;

        ListView_SetItemState(listViewCtl, (UINT)lottery_tickets[0][i] - 1,
                              LVIS_SELECTED, 0x000f);
    }

    return TRUE;
}

VOID InitSettingsDialog(HWND hwnd)
{
    if (!DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS_DLG), hwnd, SettingsDialogProcedure, (LPARAM)0)) {
        MSGBOX_ERR(hwnd, L"DialogBoxParam");
    }
}

INT_PTR CALLBACK SettingsDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL blCustLottVisible = FALSE;
    static HWND hCustLottButton, hTicketTabCtl, hTicketNumCtl;

    switch (msg) {
    case WM_INITDIALOG:
    {
        copy_lottery_ticket_groups(lottery_tickets, prev_lottery_tickets);
        EnableWindow(GetDlgItem(hwnd, IDC_SAVEPROCESSED_CHECK), FALSE);

        if (!CheckDlgButton(hwnd, IDC_SAVEDOWNLOADED_CHECK, (!delete_file)?BST_CHECKED:BST_UNCHECKED)) {
            MSGBOX_ERR(NULL, L"CheckDlgButton");
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        if (!CheckDlgButton(hwnd, IDC_MULTITHREADED_CHECK, (parallel_d)?BST_CHECKED:BST_UNCHECKED)) {
            MSGBOX_ERR(NULL, L"CheckDlgButton");
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        if (!SetDlgItemInt(hwnd, IDC_MAXTHREADS_EDIT, max_threads, FALSE)) {
            MSGBOX_ERR(NULL, L"SetDlgItemInt");
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        hCustLottButton = GetDlgItem(hwnd, IDB_CUSTOM_LOTTERY_GROUPS);
        if (!hCustLottButton) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        hTicketTabCtl = GetDlgItem(hwnd, IDT_LOTTERYGROUP_TABS);
        if (!hTicketTabCtl) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        hTicketNumCtl = GetDlgItem(hwnd, IDL_TICKETNUMS_LV);
        if (!hTicketNumCtl) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        RECT tmp_rect;
        if (!GetWindowRect(hwnd, &tmp_rect)) {
            MSGBOX_ERR(NULL, L"GetWindowRect");
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        if (!InitializeTabs(hTicketTabCtl)) {
            MsgBoxErrLocal(NULL, IDSTRING_36, IDSTRING_25);
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        if (!InitializeTicketNumLV(hTicketNumCtl)) {
            MsgBoxErrLocal(NULL, IDSTRING_37, IDSTRING_25);
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }

        BOOL btnState = (IsDlgButtonChecked(hwnd, IDC_MULTITHREADED_CHECK) == BST_CHECKED);

        EnableWindow(GetDlgItem(hwnd, IDC_MAXTHREADS_EDIT), btnState);
        EnableWindow(GetDlgItem(hwnd, IDT_MAXTHREADS_TEXT), btnState);

        if (!blCustLottVisible) {
            if (!MoveWindow(hwnd, 0, 0, 226, tmp_rect.bottom - tmp_rect.top, TRUE)) {
                MSGBOX_ERR(NULL, L"MoveWindow");
                SendMessage(hwnd, WM_CLOSE, 0, 0);
            }
        } else {
            SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)hTicketNumCtl, TRUE);
        }

        SetSettingsDialogLanguage(hwnd);
        CenterChild(hwnd);

        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        switch(HIWORD(wParam)) {
        case BN_CLICKED:
            if (LOWORD(wParam) == IDC_MULTITHREADED_CHECK) {
                BOOL btnState = (IsDlgButtonChecked(hwnd, IDC_MULTITHREADED_CHECK) == BST_CHECKED);

                EnableWindow(GetDlgItem(hwnd, IDC_MAXTHREADS_EDIT), btnState);
                EnableWindow(GetDlgItem(hwnd, IDT_MAXTHREADS_TEXT), btnState);
            }
            break;
        default:
            break;
        }
        switch(LOWORD(wParam)) {
        case IDCANCEL:
            copy_lottery_ticket_groups(prev_lottery_tickets, lottery_tickets);
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDOK:
            GetTicketFromCurrentTab(hTicketTabCtl, hTicketNumCtl);
            parallel_d = IsDlgButtonChecked(hwnd, IDC_MULTITHREADED_CHECK);
            delete_file = !IsDlgButtonChecked(hwnd, IDC_SAVEDOWNLOADED_CHECK);
            GetFieldUInt(hwnd, IDC_MAXTHREADS_EDIT, &max_threads);
            if (parallel_d && (!max_threads || max_threads > 150)) {
                max_threads = 30;
                SetDlgItemInt(hwnd, IDC_MAXTHREADS_EDIT, max_threads, FALSE);
                MsgBoxErrLocal(hwnd, IDSTRING_47, IDSTRING_48);
            }
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case IDB_RESET_LOTTERY_GROUPS:
            reset_lottery_ticket_groups();
            UpdateCurrentListViewTab(hTicketTabCtl, hTicketNumCtl);
            SendMessage(hwnd, WM_NEXTDLGCTL, (WPARAM)hTicketNumCtl, TRUE);
            break;
        case IDB_RANDOM_LOTTERY_GROUPS:
            OnRandomLotteryGroups(hwnd, hTicketTabCtl, hTicketNumCtl);
            break;
        case IDB_CUSTOM_LOTTERY_GROUPS:
        {
            int i, width, height;
            RECT DlgRect;

            if (!GetWindowRect(hwnd, &DlgRect)) {
                MSGBOX_ERR(NULL, L"GetWindowRect");
                SendMessage(hwnd, WM_CLOSE, 0, 0);
            }

            width = DlgRect.right - DlgRect.left;
            height = DlgRect.bottom - DlgRect.top;

            for (i = 1; i <= 588; i+=7) {
                HDWP winnum = BeginDeferWindowPos(1);
                if (!winnum) {
                    MSGBOX_ERR(NULL, L"BeginDeferWindowPos");
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                }

                Sleep(1);
                if (!blCustLottVisible) {
                    DeferWindowPos(winnum, hwnd, HWND_TOP, 0, 0, width + i, height,
                                   SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
                } else {
                    DeferWindowPos(winnum, hwnd, HWND_TOP, 0, 0, width - i, height,
                                   SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
                }
                EndDeferWindowPos(winnum);
            }
            blCustLottVisible = !blCustLottVisible;
            CenterChild(hwnd);
        }
        default:
            break;
        }
        return (INT_PTR)TRUE;
    case WM_NOTIFY:
        return (INT_PTR)OnNotify(hwnd, hTicketTabCtl, hTicketNumCtl, lParam, wParam);
    case WM_CLOSE:
        EndDialog(hwnd, (INT_PTR)TRUE);
        return (INT_PTR)TRUE;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

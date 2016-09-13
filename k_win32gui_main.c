/* k_win32gui_main.c
 * KiNO_Win32 entry point, main window GUI and functions
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
#include <time.h>
#include <dwmapi.h>

#define MAX_OUTPUT_BUFF 6860000
#define PRINT_BUFFSZ 4096
#define TEMP_BUFFSZ 255


static uint32_t min_lottery, max_lottery;

static HICON kino_win32icon = NULL;

static lottery_data *lotteries;

static FILE *err_log_file;


HANDLE ProgressBarEv = NULL;
uint8_t lottery_tickets[8][10];


/* Declarations */
static void cleanup(void);
static void initialize(void);
static BOOL ParseLastLotteriesField(HWND hwnd);
static BOOL ParseLotteryFields(HWND hwnd);
static VOID PrintDefaultTicketDataToPrintBox(const HWND hwnd, const HWND printbox, lottery_data *to_print, size_t data_len);
static VOID AppendTextToLotteryPrintBox(const HWND printbox, LPCWSTR newText);
static INT_PTR CALLBACK MainDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


void cleanup(void)
{
    UnloadLanguageLibraries();

    if (err_log_file) fclose(err_log_file);
    if (kino_win32icon) DestroyIcon(kino_win32icon);
    if (ProgressBarEv) CloseHandle(ProgressBarEv);

    curl_global_cleanup();

    xmlCleanupThreads();
    xmlCleanupParser();
    xmlUnlockLibrary();

    k_rmdir(XML_FOLDER);
}

void initialize(void)
{
    if (!LoadLanguageLibraries()) {
        MessageBoxW(NULL, L"Some language packs failed to load. "
                          L"Reinstall the program to fix this error.",
                    L"Initialization failure!", MB_OK | MB_ICONERROR);
        exit(EXIT_FAILURE);
    }

#ifdef __DBG
    if (!(err_log_file = freopen("K32error_logger.txt", "w+", stderr))) {
        MsgBoxErrLocal(NULL, IDSTRING_32, IDSTRING_25);
    }
#endif
    
    reset_lottery_ticket_groups();

    INITCOMMONCONTROLSEX ComCtlClasses = {
        sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS | ICC_TAB_CLASSES | ICC_LINK_CLASS
    };

    InitCommonControlsEx(&ComCtlClasses);

    curl_global_init(CURL_GLOBAL_ALL);

    xmlInitParser();
    xmlInitThreads();

    _set_errno(0);
    if (k_mkdir(XML_FOLDER) == -1 && errno != EEXIST) {
        MSGBOX_ERRNO(NULL, L"k_mkdir");
    }
}

BOOL ParseLastLotteriesField(HWND hwnd)
{
    uint32_t field_dat;

    GetFieldUInt(hwnd, IDC_LOTTERIESFROMEND_EDIT, &field_dat);

    if (!field_dat || (field_dat > 9999)) {
        MsgBoxErrLocal(hwnd, IDSTRING_27, IDSTRING_26);
        return FALSE;
    }

    uint32_t last_lottery = 0;
    for (int i = 0; i < 5 && !last_lottery; i++)
        last_lottery = get_last_lottery_number();

    if (!last_lottery) {
        MsgBoxErrLocal(hwnd, IDSTRING_28, IDSTRING_25);
        return FALSE;
    }

    min_lottery = last_lottery + 1 - field_dat;
    max_lottery = last_lottery;

    return TRUE;
}

BOOL ParseLotteryFields(HWND hwnd)
{
    BOOL field_is_not_empty[2] = {TRUE, TRUE};

    GetFieldUInt(hwnd, IDC_FIRSTLOTTERY_EDIT, &min_lottery);
    GetFieldUInt(hwnd, IDC_LASTLOTTERY_EDIT, &max_lottery);

    field_is_not_empty[0] = (max_lottery) ? TRUE : FALSE;
    field_is_not_empty[1] = (min_lottery) ? TRUE : FALSE;

    if (!field_is_not_empty[0] && !field_is_not_empty[1]) {
        MsgBoxErrLocal(hwnd, IDSTRING_29, IDSTRING_26);
        return FALSE;
    } else if (field_is_not_empty[0] && field_is_not_empty[1]) {
        if (min_lottery > max_lottery) {
            max_lottery = 0;
            min_lottery = 0;

            MsgBoxErrLocal(hwnd, IDSTRING_30, IDSTRING_26);
            return FALSE;
        }

        if ((max_lottery - min_lottery) >= 10000) {
            max_lottery = 0;
            min_lottery = 0;

            MsgBoxErrLocal(hwnd, IDSTRING_38, IDSTRING_26);
            return FALSE;
        }
    } else {
        if (!max_lottery) max_lottery = min_lottery;
        else min_lottery = max_lottery;
    }

    uint32_t last_lottery = 0;
    for (int i = 0; i < 5 && !last_lottery; i++)
        last_lottery = get_last_lottery_number();

    if (last_lottery) {
        uint32_t max = (max_lottery >= min_lottery) ? max_lottery : min_lottery;
        if (max > last_lottery) {
            wchar_t tmp_buff[255], tmp_buff1[50], tmp_buff2[100];

            max_lottery = 0;
            min_lottery = 0;
            LoadStr(tmp_buff1, IDSTRING_39);
            LoadStr(tmp_buff2, IDSTRING_40);
            StringCchPrintfW(tmp_buff, 255, L"%s%u%s%u", tmp_buff1, max, tmp_buff2, last_lottery);

            MsgBoxErrLocalCapt(hwnd, tmp_buff, IDSTRING_26);
            return FALSE;
        }
    } else {
        MsgBoxErrLocal(hwnd, IDSTRING_28, IDSTRING_25);
        return FALSE;
    }

    return TRUE;
}

void PrintCustomTicketDataToPrintBox(const HWND hwnd, const HWND printbox, lottery_data *to_print, size_t data_len)
{
    HRESULT hRes;
    size_t i;
    wchar_t tmp_buff[TEMP_BUFFSZ], print_buff[PRINT_BUFFSZ];

    print_buff[0] = 0;
    tmp_buff[0] = 0;

    for (i = 0; i <= data_len; i++, to_print++) {
        if (!(*to_print).valid_data) continue;

        hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L"Κλήρωση %u σε αύξουσα σειρά:", (*to_print).lottery_num);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"1 StringCchPrintfW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"1 StringCchCatW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        for (int i = 0; i < 20; i++) {
            if ((*to_print).results[i] == (*to_print).magic_number)
                hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L" (%02u)", (*to_print).results[i]);
            else
                hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L" %02u", (*to_print).results[i]);

            if (FAILED(hRes)) {
#ifdef __DBG
                MsgBoxErrLocalCapt(hwnd, L"2 StringCchPrintfW()", IDSTRING_25);
#else
                MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
                break;
            }
            
            hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
            if (FAILED(hRes)) {
#ifdef __DBG
                MsgBoxErrLocalCapt(hwnd, L"2 StringCchCatW()", IDSTRING_25);
#else
                MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
                break;
            }
        }

        hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L"\r\n\t--- Δελτία ---\r\n");
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"3 StringCchPrintfW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"3 StringCchCatW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        for (size_t j = 0; j < 8; j++) {
            hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L"%uο δελτίο (%02u,%02u,%02u,%02u,%02u,%02u,%02u,%02u,%02u,%02u)"
                                    L"\t-> Σύνολο αριθμών που κερδίζουν: %02u"
                                    L" με τιμή: %01.1f €%s\r\n",
                                    j + 1, lottery_tickets[j][0], lottery_tickets[j][1], lottery_tickets[j][2],
                                    lottery_tickets[j][3], lottery_tickets[j][4], lottery_tickets[j][5], lottery_tickets[j][6],
                                    lottery_tickets[j][7], lottery_tickets[j][8], lottery_tickets[j][9],
                                    (*to_print).winning_tens_total[j], (*to_print).winning_tens_payoff[j],
                                    ((*to_print).magic_number_group[j] == true) ? L" (Το KINO bonus είναι σε αυτήν την όμαδα)" : L"");
            if (FAILED(hRes)) {
#ifdef __DBG
                MsgBoxErrLocalCapt(hwnd, L"4 StringCchPrintfW()", IDSTRING_25);
#else
                MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
                break;
            }

            hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
            if (FAILED(hRes)) {
#ifdef __DBG
                MsgBoxErrLocalCapt(hwnd, L"4 StringCchCatW()", IDSTRING_25);
#else
                MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
                break;
            }
        }
        hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L"Άθροισμα τιμών: %02.1f €\r\n\r\n", (*to_print).total_payoff);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"5 StringCchPrintfW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"5 StringCchCatW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        if (!(i%2)) {
            AppendTextToLotteryPrintBox(printbox, print_buff);
            print_buff[0] = 0;
        }
        tmp_buff[0] = 0;
    }

    if ((i - 1)%2) {
        AppendTextToLotteryPrintBox(printbox, print_buff);
    }
}

void PrintDefaultTicketDataToPrintBox(const HWND hwnd, const HWND printbox, lottery_data *to_print, size_t data_len)
{
    HRESULT hRes;
    size_t i;
    wchar_t tmp_buff[TEMP_BUFFSZ], print_buff[PRINT_BUFFSZ];

    print_buff[0] = 0;
    tmp_buff[0] = 0;

    for (i = 0; i <= data_len; i++, to_print++) {
        if (!(*to_print).valid_data) continue;

        hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L"Κλήρωση %u σε αύξουσα σειρά:", (*to_print).lottery_num);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"1 StringCchPrintfW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"1 StringCchCatW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        for (int i = 0; i < 20; i++) {
            if ((*to_print).results[i] == (*to_print).magic_number)
                hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L" (%02u)", (*to_print).results[i]);
            else
                hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L" %02u", (*to_print).results[i]);

            if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"2 StringCchPrintfW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
                break;
            }
            
            hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
            if (FAILED(hRes)) {
#ifdef __DBG
                MsgBoxErrLocalCapt(hwnd, L"2 StringCchCatW()", IDSTRING_25);
#else
                MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
                break;
            }
        }

        hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L"\r\n\t--- Δελτία ανά δέκα αριθμούς (1 - 10, 11 - 20...) ---\r\n");
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"3 StringCchPrintfW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"3 StringCchCatW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        for (size_t j = 0; j < 8; j++) {
            hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L"%uο δελτίο -> Σύνολο αριθμών που κερδίζουν: %02u"
                                    L" με τιμή: %01.1f €%s\r\n",
                                    j + 1, (*to_print).winning_tens_total[j], (*to_print).winning_tens_payoff[j],
                                    (j == (*to_print).magic_number_group_idx) ? L" (Το KINO bonus είναι σε αυτήν την όμαδα)" : L"");
            if (FAILED(hRes)) {
#ifdef __DBG
                MsgBoxErrLocalCapt(hwnd, L"4 StringCchPrintfW()", IDSTRING_25);
#else
                MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
                break;
            }

            hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
            if (FAILED(hRes)) {
#ifdef __DBG
                MsgBoxErrLocalCapt(hwnd, L"4 StringCchCatW()", IDSTRING_25);
#else
                MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
                break;
            }
        }
        hRes = StringCchPrintfW(tmp_buff, TEMP_BUFFSZ, L"Άθροισμα τιμών: %02.1f €\r\n\r\n", (*to_print).total_payoff);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"5 StringCchPrintfW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        hRes = StringCchCatW(print_buff, PRINT_BUFFSZ, tmp_buff);
        if (FAILED(hRes)) {
#ifdef __DBG
            MsgBoxErrLocalCapt(hwnd, L"5 StringCchCatW()", IDSTRING_25);
#else
            MsgBoxErrLocal(hwnd, IDSTRING_41, IDSTRING_25);
#endif
            break;
        }

        if (!(i%2)) {
            AppendTextToLotteryPrintBox(printbox, print_buff);
            print_buff[0] = 0;
        }
        tmp_buff[0] = 0;
    }
    if ((i - 1)%2) {
        AppendTextToLotteryPrintBox(printbox, print_buff);
    }
}

VOID AppendTextToLotteryPrintBox(const HWND printbox, LPCWSTR newText)
{
    // move the caret to the end of the text
    int outLength = GetWindowTextLength(printbox);
    SendMessage(printbox, EM_SETSEL, outLength, outLength);

    // insert the text at the new caret position
    SendMessage(printbox, EM_REPLACESEL, TRUE, (LPARAM)newText);
}

INT_PTR CALLBACK MainDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static FILETIME curr_date_ft;
    static BOOL dialog_has_opened = FALSE;
    //The previous width and height of the main dialog.
    //They change values after every resize. Initial values are the initial width, height of the main dialog.
    static WORD prev_dlg_width = 721, prev_dlg_height = 551;
    static int groupbox_width = 0;
    static HWND output_box, download_button, from_last_lott_field,
                first_lottery_field, last_lottery_field,
                first_lottery_grpbox, last_lottery_grpbox,
                stattext_lott, stattext_prev;

    switch (msg) {
    case WM_INITDIALOG:
    {
        SYSTEMTIME tmp_sys_date;
        BOOL composit_enabled;
        if (DwmIsCompositionEnabled(&composit_enabled) != S_OK) {
            MsgBoxErrLocal(NULL, IDSTRING_42, IDSTRING_25);
            exit(EXIT_FAILURE);
        }

        if (!composit_enabled) {
            SetLastError(0);
            if (!SetWindowLongPtr(hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW) && GetLastError()) {
                MSGBOX_ERR(NULL, L"SetWindowLongPtr");
                exit(EXIT_FAILURE);
            }
        }

        SetMainWindowLanguage(hwnd);

        GetSystemTime(&tmp_sys_date);
        if (!SystemTimeToFileTime(&tmp_sys_date, &curr_date_ft)) {
            MSGBOX_ERR(NULL, L"SystemTimeToFileTime");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        output_box = GetDlgItem(hwnd, IDC_LOTTERY_DATA_PRINT);
        if (!output_box) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }
        SendMessage(output_box, EM_SETLIMITTEXT, (WPARAM)MAX_OUTPUT_BUFF, 0);

        download_button = GetDlgItem(hwnd, IDB_DOWNLOAD_AND_PROCESS_LOTTERIES_BUTTON);
        if (!download_button) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        //get the window handle of the last lottery field
        last_lottery_field = GetDlgItem(hwnd, IDC_LASTLOTTERY_EDIT);
        if (!last_lottery_field) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        //get the window handle of the first lottery field
        first_lottery_field = GetDlgItem(hwnd, IDC_FIRSTLOTTERY_EDIT);
        if (!first_lottery_field) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        //get the window handle of the last lottery groupbox to use in the dynamic resize
        last_lottery_grpbox = GetDlgItem(hwnd, ID_LASTLOTTERY_GRPBOX);
        if (!last_lottery_grpbox) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        //get the window handle of the first lottery groupbox to use in the dynamic resize
        first_lottery_grpbox = GetDlgItem(hwnd, ID_FIRSTLOTTERY_GRPBOX);
        if (!first_lottery_grpbox) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        from_last_lott_field = GetDlgItem(hwnd, IDC_LOTTERIESFROMEND_EDIT);
        if (!from_last_lott_field) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        stattext_prev = GetDlgItem(hwnd, ID_STATTEXT_PREV);
        if (!stattext_prev) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        stattext_lott = GetDlgItem(hwnd, ID_STATTEXT_LOTT);
        if (!stattext_prev) {
            MSGBOX_ERR(NULL, L"GetDlgItem");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        RECT tmp;
        if (!GetWindowRect(last_lottery_grpbox, &tmp)) { //get coordinates of last lottery groupbox relative to the monitor
            MSGBOX_ERR(NULL, L"GetWindowRect");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }
        groupbox_width = tmp.right - tmp.left;

        //set the application icon to KiNO_Win32.ico
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)kino_win32icon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)kino_win32icon);

        dialog_has_opened = TRUE;
        return (INT_PTR)TRUE;
    }
    case WM_SIZE:
    {
        if (!dialog_has_opened)
            return DefWindowProc(hwnd, msg, wParam, lParam);

        if ((wParam != SIZE_RESTORED) && (wParam != SIZE_MAXIMIZED))
            return (INT_PTR)TRUE;

        int diff_w = (int)LOWORD(lParam) - prev_dlg_width;
        int diff_h = (int)HIWORD(lParam) - prev_dlg_height;
        RECT outputbox_rect, maindialog_rect;

        if (!GetWindowRect(output_box, &outputbox_rect)) {
            MSGBOX_ERR(hwnd, L"GetWindowRect");
            return FALSE;
        }

        if (!GetWindowRect(hwnd, &maindialog_rect)) {
            MSGBOX_ERR(hwnd, L"GetWindowRect");
            return FALSE;
        }

        int offs_width = (maindialog_rect.right - outputbox_rect.right < 0) ?
                         (-(outputbox_rect.right - maindialog_rect.right) - 25) : diff_w;
        int offs_height = (maindialog_rect.bottom - outputbox_rect.bottom < 0) ?
                         (-(outputbox_rect.bottom - maindialog_rect.bottom) - 18) : diff_h;

        if (!ResizeDlgControl(hwnd, output_box, 0, 0, offs_width, offs_height))
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);

        //I don't need to resize the button, I'm just moving it left and right according to how the
        //user resizes the window
        if (!ResizeDlgControl(hwnd, download_button, diff_w, 0, 0, 0))
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);

        if (!ResizeDlgControl(hwnd, from_last_lott_field, diff_w, 0, 0, 0))
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);

        if (!ResizeDlgControl(hwnd, stattext_lott, diff_w, 0, 0, 0))
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);

        if (!ResizeDlgControl(hwnd, stattext_prev, diff_w, 0, 0, 0))
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);

        if (prev_dlg_width < 700) {
            if (!ResizeDlgControl(hwnd, last_lottery_grpbox, 0, 0, diff_w, 0))
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);

            if (!ResizeDlgControl(hwnd, last_lottery_field, 0, 0, diff_w, 0))
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);

            if (!ResizeDlgControl(hwnd, first_lottery_grpbox, 0, 0, diff_w, 0))
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);

            if (!ResizeDlgControl(hwnd, first_lottery_field, 0, 0, diff_w, 0))
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        RECT tmp;
        if (!GetWindowRect(last_lottery_grpbox, &tmp)) { //get coordinates of last lottery groupbox relative to the monitor
            MSGBOX_ERR(NULL, L"GetWindowRect");
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        if (groupbox_width && groupbox_width < tmp.right - tmp.left) {
            diff_w = groupbox_width - tmp.right + tmp.left;

            if (!ResizeDlgControl(hwnd, last_lottery_grpbox, 0, 0, diff_w, 0))
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);

            if (!ResizeDlgControl(hwnd, last_lottery_field, 0, 0, diff_w, 0))
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);

            if (!ResizeDlgControl(hwnd, first_lottery_grpbox, 0, 0, diff_w, 0))
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);

            if (!ResizeDlgControl(hwnd, first_lottery_field, 0, 0, diff_w, 0))
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);
        }

        prev_dlg_width = LOWORD(lParam);
        prev_dlg_height = HIWORD(lParam);

        return (INT_PTR)TRUE;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO *mmi = (MINMAXINFO*)lParam;

        mmi->ptMinTrackSize.x = 650;
        mmi->ptMinTrackSize.y = 320;
        return (INT_PTR)TRUE;
    }
    case WM_NOTIFY:
    {
        LPNMHDR control_dat = (LPNMHDR)lParam;

        if ((control_dat->idFrom == IDC_CALENDAR) && (control_dat->code == NM_RELEASEDCAPTURE)) {
            SYSTEMTIME tmptime[2];
            FILETIME tmp_ft;
            char date_buff[255];
            uint32_t tmp_lott_nums[2];

            if (!MonthCal_GetSelRange(control_dat->hwndFrom, &tmptime)) {
                MsgBoxErrLocal(hwnd, IDSTRING_31, IDSTRING_25);
            }

            if (!SystemTimeToFileTime(&tmptime[1], &tmp_ft)) {
                MSGBOX_ERR(NULL, L"SystemTimeToFileTime");
                SendMessage(hwnd, WM_CLOSE, wParam, lParam);
            }

            if (CompareFileTime(&tmp_ft, &curr_date_ft) > 0) {
                if (!SetDlgItemInt(hwnd, IDC_FIRSTLOTTERY_EDIT, 0, FALSE)) {
                    MSGBOX_ERR(NULL, L"SetDlgItemInt");
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                }

                if (!SetDlgItemInt(hwnd, IDC_LASTLOTTERY_EDIT, 0, FALSE)) {
                    MSGBOX_ERR(NULL, L"SetDlgItemInt");
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                }
                return (INT_PTR)TRUE;
            }

            if ((tmptime[0].wDay == tmptime[1].wDay) &&
                (tmptime[0].wMonth == tmptime[1].wMonth) &&
                (tmptime[0].wYear == tmptime[1].wYear)) {

                snprintf(date_buff, 255, "%02u-%02u-%04u", tmptime[0].wDay, tmptime[0].wMonth, tmptime[0].wYear);
                if (!get_KINO_lotterynums_by_date(date_buff, tmp_lott_nums)) {
                    MsgBoxErrLocal(hwnd, IDSTRING_33, IDSTRING_25);
                    tmp_lott_nums[0] = tmp_lott_nums[1] = 0;
                }

                if (!SetDlgItemInt(hwnd, IDC_FIRSTLOTTERY_EDIT, tmp_lott_nums[0], FALSE)) {
                    MSGBOX_ERR(NULL, L"SetDlgItemInt");
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                }

                if (!SetDlgItemInt(hwnd, IDC_LASTLOTTERY_EDIT, tmp_lott_nums[1], FALSE)) {
                    MSGBOX_ERR(NULL, L"SetDlgItemInt");
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                }
            } else {
                snprintf(date_buff, 255, "%02u-%02u-%04u", tmptime[0].wDay, tmptime[0].wMonth, tmptime[0].wYear);
                if (!get_KINO_lotterynums_by_date(date_buff, tmp_lott_nums)) {
                    MsgBoxErrLocal(hwnd, IDSTRING_33, IDSTRING_25);
                    break;
                }
                
                if (!SetDlgItemInt(hwnd, IDC_FIRSTLOTTERY_EDIT, tmp_lott_nums[0], FALSE)) {
                    MSGBOX_ERR(NULL, L"SetDlgItemInt");
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                }

                snprintf(date_buff, 255, "%02u-%02u-%04u", tmptime[1].wDay, tmptime[1].wMonth, tmptime[1].wYear);
                if (!get_KINO_lotterynums_by_date(date_buff, tmp_lott_nums)) {
                    MsgBoxErrLocal(hwnd, IDSTRING_33, IDSTRING_25);
                    break;
                }

                if (!SetDlgItemInt(hwnd, IDC_LASTLOTTERY_EDIT, tmp_lott_nums[1], FALSE)) {
                    MSGBOX_ERR(NULL, L"SetDlgItemInt");
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                }
            }
        }
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_GR_LANG:
        case IDM_EN_LANG:
            ChangeLanguageDLL(LOWORD(wParam) - 8000);
            SetMainWindowLanguage(hwnd);
            return (INT_PTR)TRUE; 
        //menu buttons
        case IDM_QUIT:
            SendMessage(hwnd, WM_CLOSE, wParam, lParam);
            return (INT_PTR)TRUE; 
        case IDM_SETTINGS:
            InitSettingsDialog(hwnd);
            return (INT_PTR)TRUE;
        case IDM_ABOUT:
            InitAboutDialog(hwnd);
            return (INT_PTR)TRUE;

        //button controls
        case IDB_DOWNLOAD_AND_PROCESS_LOTTERIES_BUTTON:
            if (ParseLotteryFields(hwnd)) {
                if (!SetDlgItemTextW(hwnd, IDC_LOTTERY_DATA_PRINT, 0)) {
                    MSGBOX_ERR(hwnd, L"SetDlgItemTextW");
                    SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                }

                InitDownloadProgressBarDialog(hwnd, min_lottery, max_lottery, &lotteries);

                if (lotteries) {
                    if (default_lottery_ticket_groups()) {
                        PrintDefaultTicketDataToPrintBox(hwnd, output_box, lotteries, max_lottery - min_lottery);
                    } else {
                        PrintCustomTicketDataToPrintBox(hwnd, output_box, lotteries, max_lottery - min_lottery);
                    }
                    k_free(lotteries);
                    lotteries = NULL;
                }
            }
            return (INT_PTR)TRUE;
        case IDOK:
            if (GetFocus() == from_last_lott_field) {
                if (ParseLastLotteriesField(hwnd)) {
                    if (!SetDlgItemInt(hwnd, IDC_LASTLOTTERY_EDIT, max_lottery, FALSE)) {
                        MSGBOX_ERR(NULL, L"SetDlgItemInt");
                        SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                    }

                    if (!SetDlgItemInt(hwnd, IDC_FIRSTLOTTERY_EDIT, min_lottery, FALSE)) {
                        MSGBOX_ERR(NULL, L"SetDlgItemInt");
                        SendMessage(hwnd, WM_CLOSE, wParam, lParam);
                    }
                }
            }
            return (INT_PTR)TRUE;
        default:
            return (INT_PTR)TRUE;
        }
    case WM_DESTROY:
        PostQuitMessage(EXIT_SUCCESS);
        return (INT_PTR)TRUE;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return (INT_PTR)TRUE;
    default:
        break;
    }

    return (INT_PTR)FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    _onexit((_onexit_t)cleanup);

    initialize();

    MSG msg;
    BOOL bRet;

    kino_win32icon = LoadImage(GetModuleHandle(NULL),
                               MAKEINTRESOURCE(IDI_KINOWIN32_ICON),
                               IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                               GetSystemMetrics(SM_CYSMICON), 0);
    if (!kino_win32icon) {
        MSGBOX_ERR(NULL, L"LoadImage");
        exit(EXIT_FAILURE);
    }

    if (!InternetGetConnectedState(NULL, 0)) {
        MsgBoxErrLocal(NULL, IDSTRING_43, IDSTRING_44);
    }

    ProgressBarEv = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!ProgressBarEv) {
        MSGBOX_ERR(NULL, L"CreateEvent");
        exit(EXIT_FAILURE);
    }

    HWND main_dlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN_DLG), NULL, MainDialogProcedure);
    if (!main_dlg) {
        MSGBOX_ERR(NULL, L"CreateDialog");
        exit(EXIT_FAILURE);
    }

    while ((bRet = GetMessage(&msg, NULL, 0, 0)) != FALSE) {
        if (bRet == -1) {
            MSGBOX_ERR(NULL, L"GetMessage");
            exit(EXIT_FAILURE);
        }

        if (!IsDialogMessage(main_dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    exit(msg.wParam);
}

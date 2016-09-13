/* k_win32gui_locale.c
 * Functions related with the localization of KiNO_Win32
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

static struct dll {
    PWSTR filename;
    HMODULE handle;
    HMENU menu;
    WORD id;
} KiNO_Win32DLL[] = {
    {L"lang\\GreekLang.dll", NULL, MAKELANGID(LANG_GREEK, SUBLANG_GREEK_GREECE)},
    /* other DLLs can be put here in the same order they 
     * are enumerated by the enum locale_idx type */
    {L"lang\\EnglishLang.dll", NULL, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)}
};


static enum locale_idx currDLL;
static VOID SetTextDlgItemArrayW(HWND hwnd, UINT *strIDs, UINT *winIDs, size_t arr_len);


VOID ChangeLanguageDLL(enum locale_idx new_langDLL)
{
    if (KiNO_Win32DLL[new_langDLL].handle) {
        currDLL = new_langDLL;
    }
}

BOOL LoadLanguageLibraries(VOID)
{
    switch (PRIMARYLANGID(GetUserDefaultUILanguage())) {
        case LANG_GREEK:
            currDLL = GREEK_DLL;
            break;
        case LANG_ENGLISH:
        default:
            currDLL = ENGLISH_DLL;
            break;
    }

    for (enum locale_idx i = GREEK_DLL; i <= ENGLISH_DLL; i++) {

        KiNO_Win32DLL[i].handle = LoadLibraryExW(KiNO_Win32DLL[i].filename, NULL, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);

        if (!KiNO_Win32DLL[i].handle) {
            if (i == currDLL) {
                if (currDLL == ENGLISH_DLL)
                    return FALSE;
                else
                    //if the language DLL of our system wasn't found we default to English
                    currDLL = ENGLISH_DLL;
            }
            continue;
        }

        KiNO_Win32DLL[i].menu = LoadMenu(KiNO_Win32DLL[i].handle, MAKEINTRESOURCE(IDR_MAIN_MENU));

        if (!KiNO_Win32DLL[i].menu) {
            FreeLibrary(KiNO_Win32DLL[i].handle);
            KiNO_Win32DLL[i].handle = NULL;

            if (i == currDLL) {
                if (currDLL == ENGLISH_DLL)
                    return FALSE;
                else
                    currDLL = ENGLISH_DLL;
            }
        }
    }

    return TRUE;
}

VOID SetAboutDialogLanguage(HWND hDlg)
{
    UINT stringIDs[] = {
        IDSTRING_6, IDSTRING_7
    }, windowIDs[] = {
        IDT_LIBRARIES_TEXT,
        IDT_DESCRIPTION_TEXT
    };

    SetTextDlgItemArrayW(hDlg, stringIDs, windowIDs, sizeof(windowIDs)/sizeof(windowIDs[0]));

    wchar_t buff[50];
    LoadStr(buff, IDSTRING_5);
    SetWindowTextW(hDlg, buff);
}

VOID SetSettingsDialogLanguage(HWND hDlg)
{
    UINT stringIDs[] = {
        IDSTRING_15, IDSTRING_16, IDSTRING_17, IDSTRING_18, IDSTRING_19,
        IDSTRING_20, IDSTRING_21, IDSTRING_22, IDSTRING_23
    }, windowIDs[] = {
        IDC_SAVEDOWNLOADED_CHECK,
        IDC_SAVEPROCESSED_CHECK,
        IDC_MULTITHREADED_CHECK,
        IDCANCEL,
        IDB_CUSTOM_LOTTERY_GROUPS,
        IDT_DOWNLOADTHREADS_TEXT,
        IDT_MAXTHREADS_TEXT,
        IDB_RESET_LOTTERY_GROUPS,
        IDB_RANDOM_LOTTERY_GROUPS
    };

    SetTextDlgItemArrayW(hDlg, stringIDs, windowIDs, sizeof(windowIDs)/sizeof(windowIDs[0]));

    wchar_t buff[50];
    LoadStr(buff, IDSTRING_1);
    SetWindowTextW(hDlg, buff);
}

VOID SetMainWindowLanguage(HWND hwnd)
{
    UINT stringIDs[] = {
        IDSTRING_10, IDSTRING_11, IDSTRING_12, IDSTRING_13, IDSTRING_14
    }, windowIDs[] = {
        IDB_DOWNLOAD_AND_PROCESS_LOTTERIES_BUTTON,
        ID_FIRSTLOTTERY_GRPBOX,
        ID_LASTLOTTERY_GRPBOX,
        ID_STATTEXT_PREV,
        ID_STATTEXT_LOTT
    };

    SetTextDlgItemArrayW(hwnd, stringIDs, windowIDs, sizeof(windowIDs)/sizeof(windowIDs[0]));

    SetMenu(hwnd, KiNO_Win32DLL[currDLL].menu);
}

VOID SetTextDlgItemArrayW(HWND hwnd, UINT *strIDs, UINT *winIDs, size_t arr_len)
{
    wchar_t buff[255];
    int buff_len = sizeof(buff)/sizeof(buff[0]);

    for (size_t i = 0; i < arr_len; i++) {
        LoadStringW(KiNO_Win32DLL[currDLL].handle, strIDs[i], buff, buff_len);
        if (!SetDlgItemTextW(hwnd, winIDs[i], buff)) printf("failed for %d\n", i);
        buff[0] = 0;
    }
}

VOID UnloadLanguageLibraries(VOID)
{
    for (enum locale_idx i = GREEK_DLL; i <= ENGLISH_DLL; i++)
        if (KiNO_Win32DLL[i].handle)
            FreeLibrary(KiNO_Win32DLL[i].handle);
}

int MessageBoxLocalized(HWND hwnd, UINT text_id, UINT caption_id, UINT type)
{
    wchar_t text[512], caption[128];

    LoadStringW(KiNO_Win32DLL[currDLL].handle, text_id, text, sizeof(text)/sizeof(text[0]));
    LoadStringW(KiNO_Win32DLL[currDLL].handle, caption_id, caption, sizeof(caption)/sizeof(caption[0]));
    return MessageBoxExW(hwnd, text, caption, type, KiNO_Win32DLL[currDLL].id);
}

int MessageBoxLocalizedCaption(HWND hwnd, wchar_t *text, UINT caption_id, UINT type)
{
    wchar_t caption[128];

    LoadStringW(KiNO_Win32DLL[currDLL].handle, caption_id, caption, sizeof(caption)/sizeof(caption[0]));
    return MessageBoxExW(hwnd, text, caption, type, KiNO_Win32DLL[currDLL].id);
}

int WrapperLoadStringW(UINT id, PWSTR str, int max)
{
    return LoadStringW(KiNO_Win32DLL[currDLL].handle, id, str, max);
}

VOID __MsgBoxDefaultFuncError(HWND hwnd, PCWSTR failed_func, PCSTR caller_func, const ULONG line, const int flag)
{ 
    PVOID pDisplayBuf;
    INT err;
    int caller_func_len;
    wchar_t *caller_func_wide;
    wchar_t msgbox_title_loc[100];


    if (flag == __GETLASTERR && (err = GetLastError())) {
        PVOID formatted_buff;

        caller_func_len = lstrlenA(caller_func) + 1;
        caller_func_wide = (LPVOID)LocalAlloc(LMEM_ZEROINIT, caller_func_len * sizeof(wchar_t));

        MultiByteToWideChar(CP_UTF8, 0, caller_func, caller_func_len, caller_func_wide, caller_func_len);

        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL, err, KiNO_Win32DLL[currDLL].id, (LPWSTR)&formatted_buff, 0, NULL);

        pDisplayBuf = LocalAlloc(LMEM_ZEROINIT, (
                                                 lstrlenW(formatted_buff) +
                                                 100 +
                                                 lstrlenW(failed_func) +
                                                 lstrlenW(caller_func_wide)
                                                ) * sizeof(wchar_t));

        StringCchPrintfW((PWSTR)pDisplayBuf, LocalSize(pDisplayBuf) / sizeof(wchar_t),
                         L"%s():%lu => %s() (GetLastError = %lu) -> %s",
                         caller_func_wide, line, failed_func, err, formatted_buff);

        err = LoadStr(msgbox_title_loc, IDSTRING_25);
        MessageBoxExW(hwnd, (PCWSTR)pDisplayBuf, (err) ? msgbox_title_loc : L"Something happened!", MB_OK | MB_ICONERROR,
                      KiNO_Win32DLL[currDLL].id);
#ifdef __DBG
        fwprintf(stderr, L"== Error at %s():%s():%lu\r\n-> %s ==\n", caller_func_wide, failed_func, line, formatted_buff);
#endif
        LocalFree(caller_func_wide);
        LocalFree(formatted_buff);
        LocalFree(pDisplayBuf);
    } else if (flag == __GETERRNO && (err = errno)) {
        char tmperrno_buff[255];
        wchar_t errno_buff[255];
        int tmperrno_buff_len = lstrlenA(tmperrno_buff) + 1;

        caller_func_len = lstrlenA(caller_func) + 1;
        caller_func_wide = (PVOID)LocalAlloc(LMEM_ZEROINIT, caller_func_len * sizeof(wchar_t));

        MultiByteToWideChar(CP_UTF8, 0, caller_func, caller_func_len, caller_func_wide, caller_func_len);

        strerror_s(tmperrno_buff, 255, err);

        MultiByteToWideChar(CP_UTF8, 0, tmperrno_buff, tmperrno_buff_len, errno_buff, tmperrno_buff_len);

        pDisplayBuf = LocalAlloc(LMEM_ZEROINIT, (
                                                 lstrlenW(errno_buff) +
                                                 100 +
                                                 lstrlenW(failed_func) +
                                                 lstrlenW(caller_func_wide)
                                                ) * sizeof(wchar_t));

        StringCchPrintfW((PWSTR)pDisplayBuf, LocalSize(pDisplayBuf) / sizeof(wchar_t),
                         L"%s():%lu => %s() (errno = %lu)\r\n-> %s",
                         caller_func_wide, line, failed_func, err, tmperrno_buff);

        err = LoadStr(msgbox_title_loc, IDSTRING_25);
        MessageBoxExW(hwnd, (PCWSTR)pDisplayBuf, (err) ? msgbox_title_loc : L"Something happened!", MB_OK | MB_ICONERROR,
                      KiNO_Win32DLL[currDLL].id);
#ifdef __DBG
        fwprintf(stderr, L"== Error at %s():%s():%lu -> %s ==\n", caller_func_wide, failed_func, line, tmperrno_buff);
#endif
        LocalFree(caller_func_wide);
        LocalFree(pDisplayBuf);
    }
}

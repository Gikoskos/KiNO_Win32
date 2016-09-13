/* k_win32gui.h
 * Main header file for k_win32gui_main.c, k_win32gui_common.c,
 * k_win32gui_about.c, k_win32gui_settings.c, k_win32gui_locale.c,
 * k_win32gui_progbar.c source files
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

#ifndef __WIN32GUI__HEADER__
#define __WIN32GUI__HEADER__

/* Headers */
#ifndef _UNICODE
# define UNICODE
#endif

#ifndef UNICODE
# define UNICODE
#endif


#include <tchar.h> //TCHAR functions

#include "k_hdr.h"
#include "locale_resources/locale_ids.h"

#include <winnt.h> //language macros
#include <winerror.h> //error messages
#include <commctrl.h> //for the progress bar
#include <wininet.h> //for the InternetGetConnectedState() function
#include <windowsx.h>
#include <wincrypt.h>
#include "resource.h"



//flags for the __MsgBoxDefaultFuncError function
#define __GETLASTERR 0x00
#define __GETERRNO 0x01




#define LoadStr(x, y) WrapperLoadStringW(y, x, sizeof(x)/sizeof(x[0]));

#define MSGBOX_ERR(x, y) __MsgBoxDefaultFuncError(x, y, __func__, __LINE__, __GETLASTERR)
#define MSGBOX_ERRNO(x, y) __MsgBoxDefaultFuncError(x, y, __func__, __LINE__, __GETERRNO)
#define MSGBOX_ERR_GEN(x, y, z) MessageBoxW(x, y, z, MB_OK | MB_ICONERROR)


#define MsgBoxErrLocal(x, y, z) MessageBoxLocalized(x, y, z, MB_OK | MB_ICONERROR)
#define MsgBoxErrLocalCapt(x, y, z) MessageBoxLocalizedCaption(x, y, z, MB_OK | MB_ICONERROR)

/* enumeration of the locales, to be used as the index on the DLL and ID arrays
   last locale always has to be English and first locale always has to be Greek */
enum locale_idx {
    GREEK_DLL = 0,
    /* other locales can be put here */
    ENGLISH_DLL
};

/* Declarations per file */

//k_win32gui_common.c
int GetFieldTextW(const HWND hwnd, wchar_t *str);
int GetFieldTextA(const HWND hwnd, char *str);
VOID GetFieldUInt(const HWND hDlg, const int nIDDlgItem, uint32_t *to_store);
VOID CenterChild(const HWND hwnd);
BOOL ResizeDlgControl(const HWND hDlg, const HWND hCtrl, int d_x, int d_y, int d_width, int d_height);

//k_win32gui_about.c
VOID InitAboutDialog(HWND hwnd);

//k_win32gui_progbar.h
VOID InitDownloadProgressBarDialog(HWND hwnd, uint32_t min_lott, uint32_t max_lott, lottery_data **lottdat);

//k_win32gui_settings.c
VOID InitSettingsDialog(HWND hwnd);
BOOL ParallelDownloads(VOID);
BOOL DeleteFiles(VOID);
uint32_t NumOfThreads(VOID);

//k_win32gui_locale.c
VOID ChangeLanguageDLL(enum locale_idx new_langDLL);
VOID SetMainWindowLanguage(HWND hwnd);
VOID SetAboutDialogLanguage(HWND hDlg);
VOID SetSettingsDialogLanguage(HWND hDlg);
BOOL LoadLanguageLibraries(VOID);
VOID UnloadLanguageLibraries(VOID);
int WrapperLoadStringW(UINT id, PWSTR str, int max);
int MessageBoxLocalizedCaption(HWND hwnd, wchar_t *text, UINT caption_id, UINT type);
int MessageBoxLocalized(HWND hwnd, UINT text_id, UINT caption_id, UINT type);
VOID __MsgBoxDefaultFuncError(HWND hwnd, PCWSTR failed_func, PCSTR caller_func, const ULONG line, const int flag);


#endif

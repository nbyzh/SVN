﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010-2012, 2014-2015, 2020-2022 - TortoiseSVN
// Copyright (C) 2011-2016, 2023 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseUDiff.h"
#include "MainWindow.h"
#include "CmdLineParser.h"
#include "TaskbarUUID.h"
#include "LangDll.h"
#include "Monitor.h"
#include "../Utils/CrashReport.h"
#include "ResString.h"
#include "registry.h"
#include "SmartHandle.h"

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#pragma warning(push)
#pragma warning(disable : 4458) // declaration of 'xxx' hides class member
#include <gdiplus.h>
#pragma warning(pop)

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HINSTANCE hResource; // the resource dll

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE /*hPrevInstance*/,
                      LPWSTR lpCmdLine,
                      int /*nCmdShow*/)
{
    SetDllDirectory(L"");
    setTaskIDPerUuid();
    MSG    msg;
    HACCEL hAccelTable;

    CCrashReportTSVN crasher(L"TortoiseUDiff " TEXT(APP_X64_STRING));
    CCrashReport::Instance().AddUserInfoToReport(L"CommandLine", GetCommandLine());
    CRegStdDWORD loc    = CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", 1033);
    long         langId = loc;
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR                    gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    CAutoLibrary hSciLexerDll = ::LoadLibrary(L"SciLexer.DLL");
    if (hSciLexerDll == nullptr)
        return FALSE;

    CLangDll langDll;
    hResource = langDll.Init(L"TortoiseUDiff", langId);
    if (!hResource)
        hResource = hInstance;

    CCmdLineParser parser(lpCmdLine);

    if (parser.HasKey(L"?") || parser.HasKey(L"help"))
    {
        ResString rHelp(hResource, IDS_COMMANDLINEHELP);
        MessageBox(nullptr, rHelp, L"TortoiseUDiff", MB_ICONINFORMATION);
        return 0;
    }

    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_STANDARD_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&used);

    CMainWindow mainWindow(hResource);
    auto        monHash = GetMonitorSetupHash();
    mainWindow.SetRegistryPath(L"Software\\TortoiseSVN\\UDiffViewerWindowPos_" + monHash);
    if (parser.HasVal(L"title"))
        mainWindow.SetTitle(parser.GetVal(L"title"));
    else if (parser.HasVal(L"patchfile"))
        mainWindow.SetTitle(parser.GetVal(L"patchfile"));
    else if (lpCmdLine[0])
    {
        // remove double quotes
        std::wstring path = lpCmdLine;
        path.erase(std::remove(path.begin(), path.end(), L'"'), path.end());
        mainWindow.SetTitle(path.c_str());
    }
    else
    {
        ResString rPipeTitle(hResource, IDS_PIPETITLE);
        mainWindow.SetTitle(rPipeTitle);
    }

    if (!mainWindow.RegisterAndCreateWindow())
        return 0;

    bool bLoadedSuccessfully = false;
    if ((lpCmdLine[0] == L'\0') || (parser.HasKey(L"p")))
    {
        // input from console pipe
        // set console to raw mode
        DWORD oldMode;
        GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &oldMode);
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), oldMode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));

        bLoadedSuccessfully = mainWindow.LoadFile(GetStdHandle(STD_INPUT_HANDLE), parser.HasKey(L"p"));
    }
    else if (parser.HasVal(L"patchfile"))
        bLoadedSuccessfully = mainWindow.LoadFile(parser.GetVal(L"patchfile"));
    else if (lpCmdLine[0] && !parser.HasKey(L"p"))
    {
        // remove double quotes
        std::wstring path = lpCmdLine;
        path.erase(std::remove(path.begin(), path.end(), '"'), path.end());
        bLoadedSuccessfully = mainWindow.LoadFile(path.c_str());
    }

    if (!bLoadedSuccessfully)
        return 0;

    ::ShowWindow(mainWindow.GetHWNDEdit(), SW_SHOW);
    ::SetFocus(mainWindow.GetHWNDEdit());

    hAccelTable = LoadAccelerators(hResource, MAKEINTRESOURCE(IDC_TORTOISEUDIFF));

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(mainWindow, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);

    return static_cast<int>(msg.wParam);
}

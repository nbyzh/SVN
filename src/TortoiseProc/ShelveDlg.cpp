﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2017-2018, 2020-2021, 2024 - TortoiseSVN
// Copyright (C) 2019 - TortoiseGit

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
#include "TortoiseProc.h"
#include "ShelveDlg.h"
#include "SVN.h"
#include "AppUtils.h"

#define REFRESHTIMER 100

IMPLEMENT_DYNAMIC(CShelve, CResizableStandAloneDialog)
CShelve::CShelve(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CShelve::IDD, pParent)
    , m_bThreadRunning(FALSE)
    , m_bCancelled(false)
    , m_revert(false)
{
}

CShelve::~CShelve()
{
}

void CShelve::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PATCHLIST, m_patchList);
    DDX_Control(pDX, IDC_SELECTALL, m_selectAll);
    DDX_Control(pDX, IDC_SHELVES, m_cShelvesCombo);
    DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
}

BEGIN_MESSAGE_MAP(CShelve, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_SELECTALL, OnBnClickedSelectall)
    ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_NEEDSREFRESH, OnSVNStatusListCtrlNeedsRefresh)
    ON_REGISTERED_MESSAGE(CSVNStatusListCtrl::SVNSLNM_ADDFILE, OnFileDropped)
    ON_EN_CHANGE(IDC_EDITCONFIG, OnShelveNameChanged)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_CHECKPOINT, &CShelve::OnBnClickedCheckpoint)
    ON_CBN_SELCHANGE(IDC_SHELVES, &CShelve::OnCbnSelchangeShelves)
    ON_CBN_EDITCHANGE(IDC_SHELVES, &CShelve::OnCbnEditchangeShelves)
END_MESSAGE_MAP()

BOOL CShelve::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    UpdateData(FALSE);

    m_patchList.Init(0, L"ShelveDlg", SVNSLC_POPALL ^ (SVNSLC_POPIGNORE | SVNSLC_POPCOMMIT | SVNSLC_POPCREATEPATCH | SVNSLC_POPRESTORE));
    m_patchList.SetConfirmButton(static_cast<CButton*>(GetDlgItem(IDOK)));
    m_patchList.SetSelectButton(&m_selectAll);
    m_patchList.SetCancelBool(&m_bCancelled);
    m_patchList.EnableFileDrop();
    m_patchList.SetRevertMode(true);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    m_projectProperties.ReadPropsPathList(m_pathList);

    m_cLogMessage.Init(m_projectProperties);
    m_cLogMessage.SetFont(CAppUtils::GetLogFontName(), CAppUtils::GetLogFontSize());
    std::map<int, UINT> icons;
    icons[AUTOCOMPLETE_SPELLING]    = IDI_SPELL;
    icons[AUTOCOMPLETE_FILENAME]    = IDI_FILE;
    icons[AUTOCOMPLETE_PROGRAMCODE] = IDI_CODE;
    icons[AUTOCOMPLETE_SNIPPET]     = IDI_SNIPPET;
    m_cLogMessage.SetIcon(icons);

    std::vector<CString> names;
    m_svn.ShelvesList(names, m_pathList.GetCommonRoot());
    for (const auto& name : names)
    {
        m_cShelvesCombo.AddString(name);
    }
    if (!m_sShelveName.IsEmpty())
        m_cShelvesCombo.SelectString(0, m_sShelveName);

    AdjustControlSize(IDC_SELECTALL);

    AddAnchor(IDC_PATCHLIST, TOP_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_SELECTALL, MIDDLE_LEFT);
    AddAnchor(IDC_SHELVESLABEL, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_SHELVES, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_INFOLABEL, MIDDLE_LEFT, MIDDLE_RIGHT);
    AddAnchor(IDC_LOGMSGLABEL, MIDDLE_LEFT);
    AddAnchor(IDC_LOGMESSAGE, MIDDLE_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_LEFT);
    AddAnchor(IDC_CHECKPOINT, BOTTOM_RIGHT);
    AddAnchor(IDCANCEL, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);

    m_tooltips.AddTool(IDOK, IDS_SHELF_BTN_TT);
    m_tooltips.AddTool(IDC_CHECKPOINT, IDS_CHECKPOINT_BTN_TT);

    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"ShelveDlg");

    // first start a thread to obtain the file list with the status without
    // blocking the dialog
    InterlockedExchange(&m_bThreadRunning, TRUE);
    if (AfxBeginThread(PatchThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }

    return TRUE;
}

UINT CShelve::PatchThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashthread;
    return static_cast<CShelve*>(pVoid)->PatchThread();
}

DWORD CShelve::ShowMask()
{
    return SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS | SVNSLC_SHOWDIRECTFILES;
}

void CShelve::LockOrUnlockBtns()
{
    auto sel = m_cShelvesCombo.GetCurSel();
    if (((m_cShelvesCombo.GetWindowTextLength() > 0) || (m_cShelvesCombo.GetLBTextLen(sel) > 0)) && m_patchList.GetSelected() > 0)
    {
        DialogEnableWindow(IDOK, true);
        DialogEnableWindow(IDC_CHECKPOINT, true);
    }
    else
    {
        DialogEnableWindow(IDOK, false);
        DialogEnableWindow(IDC_CHECKPOINT, false);
    }
}

UINT CShelve::PatchThread()
{
    // get the status of all selected file/folders recursively
    // and show the ones which can be included in a patch (i.e. the versioned and not-normal ones)
    DialogEnableWindow(IDOK, false);
    m_bCancelled = false;

    if (!m_patchList.GetStatus(m_pathList))
    {
        m_patchList.SetEmptyString(m_patchList.GetLastErrorMessage());
    }

    m_patchList.Show(
        ShowMask(), CTSVNPathList(), SVNSLC_SHOWDIRECTFILES | SVNSLC_SHOWVERSIONEDBUTNORMALANDEXTERNALSFROMDIFFERENTREPOS, true, true);

    LockOrUnlockBtns();
    InterlockedExchange(&m_bThreadRunning, FALSE);
    return 0;
}

BOOL CShelve::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
            case VK_RETURN:
                if (OnEnterPressed())
                    return TRUE;
                break;
            case VK_F5:
            {
                if (!InterlockedExchange(&m_bThreadRunning, TRUE))
                {
                    if (AfxBeginThread(PatchThreadEntry, this) == nullptr)
                    {
                        InterlockedExchange(&m_bThreadRunning, FALSE);
                        OnCantStartThread();
                    }
                }
            }
            break;
        }
    }

    return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CShelve::OnBnClickedSelectall()
{
    UINT state = (m_selectAll.GetState() & 0x0003);
    if (state == BST_INDETERMINATE)
    {
        // It is not at all useful to manually place the checkbox into the indeterminate state...
        // We will force this on to the unchecked state
        state = BST_UNCHECKED;
        m_selectAll.SetCheck(state);
    }
    theApp.DoWaitCursor(1);
    m_patchList.SelectAll(state == BST_CHECKED);
    LockOrUnlockBtns();
    theApp.DoWaitCursor(-1);
}

void CShelve::OnBnClickedHelp()
{
    OnHelp();
}

void CShelve::OnCancel()
{
    m_bCancelled = true;
    if (m_bThreadRunning)
        return;

    CResizableStandAloneDialog::OnCancel();
}

void CShelve::OnOK()
{
    // Shelve, not checkpoint
    if (m_bThreadRunning)
        return;

    if (!FillData())
        return;
    m_revert = true;

    CResizableStandAloneDialog::OnOK();
}
void CShelve::OnBnClickedCheckpoint()
{
    // checkpoint, not shelve
    if (m_bThreadRunning)
        return;

    if (!FillData())
        return;
    m_revert = false;

    CResizableStandAloneDialog::OnOK();
}

bool CShelve::FillData()
{
    CString val;
    auto    sel = m_cShelvesCombo.GetCurSel();
    if (sel != CB_ERR)
    {
        m_cShelvesCombo.GetLBText(sel, val);
    }
    if (val.IsEmpty())
        m_cShelvesCombo.GetWindowText(val);
    m_sShelveName = val;

    if (m_sShelveName.IsEmpty())
    {
        m_tooltips.ShowBalloon(&m_cShelvesCombo, IDS_ERR_SHELFNAME_EMPTY, IDS_ERR_ERROR, TTI_ERROR);
        return false;
    }

    m_sLogMsg = m_cLogMessage.GetText();

    //save only the files the user has selected into the path list
    m_patchList.WriteCheckedNamesToPathList(m_pathList);
    return true;
}

LRESULT CShelve::OnSVNStatusListCtrlNeedsRefresh(WPARAM, LPARAM)
{
    if (InterlockedExchange(&m_bThreadRunning, TRUE))
        return 0;
    if (AfxBeginThread(PatchThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }
    return 0;
}

void CShelve::OnShelveNameChanged()
{
    if (m_bThreadRunning)
    {
        return;
    }
    LockOrUnlockBtns();
}

LRESULT CShelve::OnFileDropped(WPARAM, LPARAM lParam)
{
    BringWindowToTop();
    SetForegroundWindow();
    SetActiveWindow();
    // if multiple files/folders are dropped
    // this handler is called for every single item
    // separately.
    // To avoid creating multiple refresh threads and
    // causing crashes, we only add the items to the
    // list control and start a timer.
    // When the timer expires, we start the refresh thread,
    // but only if it isn't already running - otherwise we
    // restart the timer.
    CTSVNPath path;
    path.SetFromWin(reinterpret_cast<LPCWSTR>(lParam));

    if (!m_patchList.HasPath(path))
    {
        if (m_pathList.AreAllPathsFiles())
        {
            m_pathList.AddPath(path);
            m_pathList.RemoveDuplicates();
        }
        else
        {
            // if the path list contains folders, we have to check whether
            // our just (maybe) added path is a child of one of those. If it is
            // a child of a folder already in the list, we must not add it. Otherwise
            // that path could show up twice in the list.
            bool bHasParentInList = false;
            for (int i = 0; i < m_pathList.GetCount(); ++i)
            {
                if (m_pathList[i].IsAncestorOf(path))
                {
                    bHasParentInList = true;
                    break;
                }
            }
            if (!bHasParentInList)
            {
                m_pathList.AddPath(path);
                m_pathList.RemoveDuplicates();
            }
        }
    }

    // Always start the timer, since the status of an existing item might have changed
    SetTimer(REFRESHTIMER, 200, nullptr);
    CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Item %s dropped, timer started\n", path.GetWinPath());
    return 0;
}

void CShelve::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
        case REFRESHTIMER:
            if (m_bThreadRunning)
            {
                SetTimer(REFRESHTIMER, 200, nullptr);
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Wait some more before refreshing\n");
            }
            else
            {
                KillTimer(REFRESHTIMER);
                CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Refreshing after items dropped\n");
                OnSVNStatusListCtrlNeedsRefresh(0, 0);
            }
            break;
    }
    __super::OnTimer(nIDEvent);
}

void CShelve::OnCbnSelchangeShelves()
{
    auto sel = m_cShelvesCombo.GetCurSel();
    if (sel != CB_ERR)
    {
        CString name;
        m_cShelvesCombo.GetLBText(sel, name);
        auto info = m_svn.GetShelfInfo(name, m_pathList.GetCommonRoot());
        m_cLogMessage.SetText(info.logMessage);
        CString sInfo;
        sInfo.Format(IDS_SHELF_INFO_LABEL, static_cast<LPCWSTR>(info.name), static_cast<int>(info.versions.size()));
        SetDlgItemText(IDC_INFOLABEL, sInfo);
    }
    else
        SetDlgItemText(IDC_INFOLABEL, L"");
    LockOrUnlockBtns();
}

void CShelve::OnCbnEditchangeShelves()
{
    SetDlgItemText(IDC_INFOLABEL, L"");
    LockOrUnlockBtns();
}

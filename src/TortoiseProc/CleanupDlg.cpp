﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011, 2013-2015, 2021, 2024 - TortoiseSVN

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
#include "CleanupDlg.h"
#include "AppUtils.h"
#include "../SVN/TSVNPath.h"

// CCleanupDlg dialog

IMPLEMENT_DYNAMIC(CCleanupDlg, CStateStandAloneDialog)

CCleanupDlg::CCleanupDlg(CWnd* pParent /*=NULL*/)
    : CStateStandAloneDialog(CCleanupDlg::IDD, pParent)
    , m_bCleanup(FALSE)
    , m_bRevert(FALSE)
    , m_bDelUnversioned(FALSE)
    , m_bDelIgnored(FALSE)
    , m_bRefreshShell(FALSE)
    , m_bExternals(FALSE)
    , m_bBreakLocks(FALSE)
    , m_bFixTimestamps(FALSE)
    , m_bVacuum(FALSE)
    , m_regRefreshShell(L"Software\\TortoiseSVN\\CleanupRefreshShell", FALSE)
    , m_regExternals(L"Software\\TortoiseSVN\\CleanupExternals", TRUE)
    , m_regFixTimestamps(L"Software\\TortoiseSVN\\CleanupFixTimeStamps", FALSE)
    , m_regVacuum(L"Software\\TortoiseSVN\\CleanupVacuum", FALSE)
    , m_regBreakLocks(L"Software\\TortoiseSVN\\CleanupBreakLocks", FALSE)
{
}

CCleanupDlg::~CCleanupDlg()
{
}

void CCleanupDlg::DoDataExchange(CDataExchange* pDX)
{
    CStateStandAloneDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_CLEANUP, m_bCleanup);
    DDX_Check(pDX, IDC_REVERT, m_bRevert);
    DDX_Check(pDX, IDC_DELETEUNVERSIONED, m_bDelUnversioned);
    DDX_Check(pDX, IDC_DELETEIGNORED, m_bDelIgnored);
    DDX_Check(pDX, IDC_REFRESHSHELL, m_bRefreshShell);
    DDX_Check(pDX, IDC_EXTERNALS, m_bExternals);
    DDX_Check(pDX, IDC_BREAKLOCKS, m_bBreakLocks);
    DDX_Check(pDX, IDC_FIXTIMESTAMPS, m_bFixTimestamps);
    DDX_Check(pDX, IDC_VACUUM, m_bVacuum);
}

BEGIN_MESSAGE_MAP(CCleanupDlg, CStateStandAloneDialog)
    ON_BN_CLICKED(IDHELP, &CCleanupDlg::OnBnClickedHelp)
    ON_BN_CLICKED(IDC_CLEANUP, &CCleanupDlg::OnBnClicked)
    ON_BN_CLICKED(IDC_REVERT, &CCleanupDlg::OnBnClicked)
    ON_BN_CLICKED(IDC_DELETEUNVERSIONED, &CCleanupDlg::OnBnClicked)
    ON_BN_CLICKED(IDC_DELETEIGNORED, &CCleanupDlg::OnBnClicked)
    ON_BN_CLICKED(IDC_REFRESHSHELL, &CCleanupDlg::OnBnClicked)
    ON_BN_CLICKED(IDC_EXTERNALS, &CCleanupDlg::OnBnClicked)
    ON_BN_CLICKED(IDC_BREAKLOCKS, &CCleanupDlg::OnBnClicked)
    ON_BN_CLICKED(IDC_FIXTIMESTAMPS, &CCleanupDlg::OnBnClicked)
    ON_BN_CLICKED(IDC_VACUUM, &CCleanupDlg::OnBnClicked)
END_MESSAGE_MAP()

// CCleanupDlg message handlers

BOOL CCleanupDlg::OnInitDialog()
{
    CStateStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    if (!m_path.IsEmpty())
    {
        CString sWindowTitle;
        GetWindowText(sWindowTitle);
        CAppUtils::SetWindowTitle(m_hWnd, CTSVNPath(m_path).GetUIPathString(), sWindowTitle);
    }
    ExtendFrameIntoClientArea(0);
    m_aeroControls.SubclassControl(this, IDC_CLEANUP);
    m_aeroControls.SubclassControl(this, IDC_REVERT);
    m_aeroControls.SubclassControl(this, IDC_DELETEUNVERSIONED);
    m_aeroControls.SubclassControl(this, IDC_DELETEIGNORED);
    m_aeroControls.SubclassControl(this, IDC_REFRESHSHELL);
    m_aeroControls.SubclassControl(this, IDC_EXTERNALS);
    m_aeroControls.SubclassControl(this, IDC_BREAKLOCKS);
    m_aeroControls.SubclassControl(this, IDC_FIXTIMESTAMPS);
    m_aeroControls.SubclassControl(this, IDC_VACUUM);
    m_aeroControls.SubclassOkCancelHelp(this);

    AdjustControlSize(IDC_CLEANUP);
    AdjustControlSize(IDC_REVERT);
    AdjustControlSize(IDC_DELETEUNVERSIONED);
    AdjustControlSize(IDC_DELETEIGNORED);
    AdjustControlSize(IDC_REFRESHSHELL);
    AdjustControlSize(IDC_EXTERNALS);
    AdjustControlSize(IDC_BREAKLOCKS);
    AdjustControlSize(IDC_FIXTIMESTAMPS);
    AdjustControlSize(IDC_VACUUM);

    m_bCleanup       = true;
    m_bBreakLocks    = m_regBreakLocks;
    m_bExternals     = m_regExternals;
    m_bRefreshShell  = m_regRefreshShell;
    m_bFixTimestamps = m_regFixTimestamps;
    m_bVacuum        = m_regVacuum;
    UpdateData(FALSE);
    EnableOKButton();

    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"CleanupDlg");

    return TRUE;
}

void CCleanupDlg::EnableOKButton()
{
    UpdateData();
    DialogEnableWindow(IDOK, m_bCleanup || m_bRevert || m_bDelUnversioned || m_bRefreshShell || m_bDelIgnored);
    m_regExternals     = m_bExternals;
    m_regRefreshShell  = m_bRefreshShell;
    m_regFixTimestamps = m_bFixTimestamps;
    m_regVacuum        = m_bVacuum;
    m_regBreakLocks    = m_bBreakLocks;
}

void CCleanupDlg::OnBnClickedHelp()
{
    OnHelp();
}

void CCleanupDlg::OnBnClicked()
{
    EnableOKButton();
    DialogEnableWindow(IDC_BREAKLOCKS, m_bCleanup);
    DialogEnableWindow(IDC_FIXTIMESTAMPS, m_bCleanup);
    DialogEnableWindow(IDC_VACUUM, m_bCleanup);
}

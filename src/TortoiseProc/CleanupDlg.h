﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2011-2012, 2014-2015, 2021, 2024 - TortoiseSVN

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
#pragma once
#include "StandAloneDlg.h"

// CCleanupDlg dialog

class CCleanupDlg : public CStateStandAloneDialog
{
    DECLARE_DYNAMIC(CCleanupDlg)

public:
    CCleanupDlg(CWnd* pParent = nullptr); // standard constructor
    ~CCleanupDlg() override;

    // Dialog Data
    enum
    {
        IDD = IDD_CLEANUP
    };

public:
    BOOL m_bCleanup;
    BOOL m_bRevert;
    BOOL m_bDelUnversioned;
    BOOL m_bDelIgnored;
    BOOL m_bRefreshShell;
    BOOL m_bExternals;
    BOOL m_bBreakLocks;
    BOOL m_bFixTimestamps;
    BOOL m_bVacuum;

    CString m_path;

protected:
    void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support
    BOOL OnInitDialog() override;

    afx_msg void OnBnClickedHelp();
    afx_msg void OnBnClicked();

    void EnableOKButton();

    DECLARE_MESSAGE_MAP()

private:
    CRegDWORD m_regRefreshShell;
    CRegDWORD m_regExternals;
    CRegDWORD m_regFixTimestamps;
    CRegDWORD m_regVacuum;
    CRegDWORD m_regBreakLocks;
};

﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2020-2021 - TortoiseSVN

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

#include "ResizableDialog.h"
#include "registry.h"
#include "AeroControls.h"
#include "TaskbarUUID.h"
#include "Tooltip.h"
#include "CommonDialogFunctions.h"
#include "CommonAppUtils.h"
#include "LoadIconEx.h"
#include "EditWordBreak.h"
#include "Theme.h"
#include "DarkModeHelper.h"
#include "DPIAware.h"
#include "resource.h"
#include <Dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")

#define DIALOG_BLOCKHORIZONTAL 1
#define DIALOG_BLOCKVERTICAL   2

std::wstring GetMonitorSetupHash();

/**
 * \ingroup TortoiseProc
 *
 * A template which can be used as the base-class of dialogs which form the main dialog
 * of a 'dialog-style application'
 * Just provides the boiler-plate code for dealing with application icons
 *
 * \remark Replace all references to CDialog or CResizableDialog in your dialog class with
 * either CResizableStandAloneDialog, CStandAloneDialog or CStateStandAloneDialog, as appropriate
 */
template <typename BaseType>
class CStandAloneDialogTmpl : public BaseType
    , protected CommonDialogFunctions<BaseType>
{
#ifndef _DLL
    DECLARE_DYNAMIC(CStandAloneDialogTmpl)
#endif
protected:
    CStandAloneDialogTmpl(UINT nIDTemplate, CWnd* pParentWnd = nullptr)
        : BaseType(nIDTemplate, pParentWnd)
        , CommonDialogFunctions<BaseType>(this)
    {
        m_hIcon                  = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
        m_margins.cxLeftWidth    = 0;
        m_margins.cyTopHeight    = 0;
        m_margins.cxRightWidth   = 0;
        m_margins.cyBottomHeight = 0;
        m_bkgndIconWidth         = 0;
        m_bkgndIconHeight        = 0;
        m_hBkgndIcon             = nullptr;
        m_nResizeBlock           = 0;
        m_height                 = 0;
        m_width                  = 0;
        m_themeCallbackId        = 0;
        m_dpi                    = 0;

        SetBackgroundIcon(IDI_AEROBACKGROUND, 256, 256);
    }
    ~CStandAloneDialogTmpl() override
    {
        CTheme::Instance().RemoveRegisteredCallback(m_themeCallbackId);
        if (m_hBkgndIcon)
            DestroyIcon(m_hBkgndIcon);
    }
    virtual BOOL OnInitDialog()
    {
        m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
            [this]() {
                SetTheme(CTheme::Instance().IsDarkTheme());
            });

        BaseType::OnInitDialog();

        // Set the icon for this dialog.  The framework does this automatically
        //  when the application's main window is not a dialog
        BaseType::SetIcon(m_hIcon, TRUE);  // Set big icon
        BaseType::SetIcon(m_hIcon, FALSE); // Set small icon

        RECT rect;
        BaseType::GetWindowRect(&rect);
        m_height = rect.bottom - rect.top;
        m_width  = rect.right - rect.left;
        BaseType::EnableToolTips();
        m_tooltips.Create(this);
        SetTheme(CTheme::Instance().IsDarkTheme());

        auto customBreak = static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\UseCustomWordBreak", 2));
        if (customBreak)
            SetUrlWordBreakProcToChildWindows(BaseType::GetSafeHwnd(), customBreak == 2);

        m_dpi = CDPIAware::Instance().GetDPI(BaseType::GetSafeHwnd());

        return FALSE;
    }

    virtual BOOL PreTranslateMessage(MSG* pMsg)
    {
        m_tooltips.RelayEvent(pMsg, this);
        if (pMsg->message == WM_KEYDOWN)
        {
            int nVirtKey = static_cast<int>(pMsg->wParam);

            if (nVirtKey == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
            {
                wchar_t buffer[129];
                ::GetClassName(pMsg->hwnd, buffer, 128);

                if (_wcsnicmp(buffer, L"EDIT", 128) == 0)
                {
                    ::PostMessage(pMsg->hwnd, EM_SETSEL, 0, -1);
                    return TRUE;
                }
            }
            if (nVirtKey == 'D' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000))
            {
                CTheme::Instance().SetDarkTheme(!CTheme::Instance().IsDarkTheme());
            }
        }
        return BaseType::PreTranslateMessage(pMsg);
    }
    virtual ULONG GetGestureStatus(CPoint /*ptTouch*/) override
    {
        return 0;
    }

    afx_msg void OnPaint()
    {
        if (BaseType::IsIconic())
        {
            CPaintDC dc(this); // device context for painting

            BaseType::SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

            // Center icon in client rectangle
            int   cxIcon = GetSystemMetrics(SM_CXICON);
            int   cyIcon = GetSystemMetrics(SM_CYICON);
            CRect rect;
            BaseType::GetClientRect(&rect);
            int x = (rect.Width() - cxIcon + 1) / 2;
            int y = (rect.Height() - cyIcon + 1) / 2;

            // Draw the icon
            dc.DrawIcon(x, y, m_hIcon);
        }
        else
        {
            BaseType::OnPaint();
        }
    }

    BOOL OnEraseBkgnd(CDC* pDC)
    {
        BOOL baseRet = BaseType::OnEraseBkgnd(pDC);
        if (m_aeroControls.AeroDialogsEnabled())
        {
            // draw the frame margins in black
            CRect rc;
            BaseType::GetClientRect(&rc);
            if (m_margins.cxLeftWidth < 0)
            {
                pDC->FillSolidRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, RGB(0, 0, 0));
                if (m_hBkgndIcon)
                {
                    // center the icon
                    double scale = 1.0;
                    scale        = min(scale, static_cast<double>(rc.Width()) / static_cast<double>(m_bkgndIconWidth));
                    scale        = min(scale, static_cast<double>(rc.Height()) / static_cast<double>(m_bkgndIconHeight));
                    if (scale > 1.0)
                        scale = 1.0;
                    DrawIconEx(pDC->GetSafeHdc(),
                               static_cast<int>((rc.Width() - (scale * m_bkgndIconWidth)) / 1.0),
                               0,
                               m_hBkgndIcon,
                               static_cast<int>(scale * m_bkgndIconWidth), static_cast<int>(scale * m_bkgndIconHeight),
                               0, nullptr, DI_NORMAL);
                }
            }
            else
            {
                pDC->FillSolidRect(rc.left, rc.top, m_margins.cxLeftWidth, rc.bottom - rc.top, RGB(0, 0, 0));
                pDC->FillSolidRect(rc.left, rc.top, rc.right - rc.left, m_margins.cyTopHeight, RGB(0, 0, 0));
                pDC->FillSolidRect(rc.right - m_margins.cxRightWidth, rc.top, m_margins.cxRightWidth, rc.bottom - rc.top, RGB(0, 0, 0));
                pDC->FillSolidRect(rc.left, rc.bottom - m_margins.cyBottomHeight, rc.right - rc.left, m_margins.cyBottomHeight, RGB(0, 0, 0));
            }
        }
        return baseRet;
    }

    LRESULT OnNcHitTest(CPoint pt)
    {
        if (m_aeroControls.AeroDialogsEnabled())
        {
            CRect rc;
            BaseType::GetClientRect(&rc);
            BaseType::ClientToScreen(&rc);

            if (m_margins.cxLeftWidth < 0)
            {
                return rc.PtInRect(pt) ? HTCAPTION : BaseType::OnNcHitTest(pt);
            }
            else
            {
                CRect m = rc;
                m.DeflateRect(m_margins.cxLeftWidth, m_margins.cyTopHeight, m_margins.cxRightWidth, m_margins.cyBottomHeight);
                return (rc.PtInRect(pt) && !m.PtInRect(pt)) ? HTCAPTION : BaseType::OnNcHitTest(pt);
            }
        }
        return BaseType::OnNcHitTest(pt);
    }

    void ExtendFrameIntoClientArea(UINT bottomControl)
    {
        ExtendFrameIntoClientArea(0, 0, 0, bottomControl);
    }

    /**
     *
     */
    void ExtendFrameIntoClientArea(UINT leftControl, UINT topControl, UINT rightControl, UINT botomControl)
    {
        if (!m_aeroControls.AeroDialogsEnabled())
            return;
        RECT rc, rc2;
        BaseType::GetWindowRect(&rc);
        BaseType::GetClientRect(&rc2);
        BaseType::ClientToScreen(&rc2);

        RECT rcControl;
        if (leftControl)
        {
            HWND hw = BaseType::GetDlgItem(leftControl)->GetSafeHwnd();
            if (hw == nullptr)
                return;
            ::GetWindowRect(hw, &rcControl);
            m_margins.cxLeftWidth = rcControl.left - rc.left;
            m_margins.cxLeftWidth -= (rc2.left - rc.left);
        }
        else
            m_margins.cxLeftWidth = 0;

        if (topControl)
        {
            HWND hw = BaseType::GetDlgItem(topControl)->GetSafeHwnd();
            if (hw == nullptr)
                return;
            ::GetWindowRect(hw, &rcControl);
            m_margins.cyTopHeight = rcControl.top - rc.top;
            m_margins.cyTopHeight -= (rc2.top - rc.top);
        }
        else
            m_margins.cyTopHeight = 0;

        if (rightControl)
        {
            HWND hw = BaseType::GetDlgItem(rightControl)->GetSafeHwnd();
            if (hw == nullptr)
                return;
            ::GetWindowRect(hw, &rcControl);
            m_margins.cxRightWidth = rc.right - rcControl.right;
            m_margins.cxRightWidth -= (rc.right - rc2.right);
        }
        else
            m_margins.cxRightWidth = 0;

        if (botomControl)
        {
            HWND hw = BaseType::GetDlgItem(botomControl)->GetSafeHwnd();
            if (hw == nullptr)
                return;
            ::GetWindowRect(hw, &rcControl);
            m_margins.cyBottomHeight = rc.bottom - rcControl.bottom;
            m_margins.cyBottomHeight -= (rc.bottom - rc2.bottom);
        }
        else
            m_margins.cyBottomHeight = 0;

        if ((m_margins.cxLeftWidth == 0) &&
            (m_margins.cyTopHeight == 0) &&
            (m_margins.cxRightWidth == 0) &&
            (m_margins.cyBottomHeight == 0))
        {
            m_margins.cxLeftWidth    = -1;
            m_margins.cyTopHeight    = -1;
            m_margins.cxRightWidth   = -1;
            m_margins.cyBottomHeight = -1;
        }
        DwmExtendFrameIntoClientArea(BaseType::m_hWnd, &m_margins);
    }

    /**
     * Wrapper around the CWnd::EnableWindow() method, but
     * makes sure that a control that has the focus is not disabled
     * before the focus is passed on to the next control.
     */
    BOOL DialogEnableWindow(UINT nID, BOOL bEnable)
    {
        CWnd* pwndDlgItem = BaseType::GetDlgItem(nID);
        if (pwndDlgItem == nullptr)
            return FALSE;
        if (bEnable)
            return pwndDlgItem->EnableWindow(bEnable);
        if (BaseType::GetFocus() == pwndDlgItem)
        {
            BaseType::SendMessage(WM_NEXTDLGCTL, 0, FALSE);
        }
        return pwndDlgItem->EnableWindow(bEnable);
    }

    /**
     * Refreshes the cursor by forcing a WM_SETCURSOR message.
     */
    void RefreshCursor()
    {
        POINT pt;
        GetCursorPos(&pt);
        SetCursorPos(pt.x, pt.y);
    }

    bool IsCursorOverWindowBorder()
    {
        RECT wRc{}, cRc{};
        BaseType::GetWindowRect(&wRc);
        BaseType::GetClientRect(&cRc);
        BaseType::ClientToScreen(&cRc);
        DWORD pos = GetMessagePos();
        POINT pt;
        pt.x = GET_X_LPARAM(pos);
        pt.y = GET_Y_LPARAM(pos);
        return (PtInRect(&wRc, pt) && !PtInRect(&cRc, pt));
    }
    void SetBackgroundIcon(HICON hIcon, int width, int height)
    {
        m_hBkgndIcon      = hIcon;
        m_bkgndIconWidth  = width;
        m_bkgndIconHeight = height;
    }
    void SetBackgroundIcon(UINT idi, int width, int height)
    {
        auto hIcon = LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(idi), width, height);
        SetBackgroundIcon(hIcon, width, height);
    }
    void BlockResize(int block)
    {
        m_nResizeBlock = block;
    }

    void EnableSaveRestore(LPCWSTR pszSection, bool bRectOnly = FALSE)
    {
        // call the base method with the bHorzResize and bVertResize parameters
        // figured out from the resize block flags.
        std::wstring monitorSetupSection = pszSection + GetMonitorSetupHash();
        BaseType::EnableSaveRestore(monitorSetupSection.c_str(), bRectOnly, (m_nResizeBlock & DIALOG_BLOCKHORIZONTAL) == 0, (m_nResizeBlock & DIALOG_BLOCKVERTICAL) == 0);
    }

    void SetTheme(bool bDark);

protected:
    MARGINS         m_margins;
    AeroControlBase m_aeroControls;
    CToolTips       m_tooltips;
    int             m_nResizeBlock;
    long            m_width;
    long            m_height;
    int             m_themeCallbackId;
    int             m_dpi;
    DECLARE_MESSAGE_MAP()
private:
    HCURSOR OnQueryDragIcon()
    {
        return static_cast<HCURSOR>(m_hIcon);
    }

    virtual void HtmlHelp(DWORD_PTR dwData, UINT nCmd = 0x000F)
    {
        UNREFERENCED_PARAMETER(nCmd);

        if (!CCommonAppUtils::StartHtmlHelp(dwData))
        {
            AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
        }
    }

protected:
    void OnCompositionChanged()
    {
        if (m_aeroControls.AeroDialogsEnabled())
        {
            DwmExtendFrameIntoClientArea(BaseType::m_hWnd, &m_margins);
        }
        BaseType::OnCompositionChanged();
    }

    afx_msg LRESULT OnTaskbarButtonCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
    {
        setUuidOverlayIcon(BaseType::m_hWnd);
        return 0;
    }

    afx_msg void OnSysColorChange()
    {
        BaseType::OnSysColorChange();
        CTheme::Instance().OnSysColorChanged();
        SetTheme(CTheme::Instance().IsDarkTheme());
    }

    LRESULT OnDPIChanged(WPARAM, LPARAM lParam);

    HICON m_hIcon;
    HICON m_hBkgndIcon;
    int   m_bkgndIconWidth;
    int   m_bkgndIconHeight;
};

class CStateDialog : public CDialog
    , public CResizableWndState
{
    DECLARE_DYNAMIC(CStateDialog)
public:
    CStateDialog()
        : CDialog()
        , m_bEnableSaveRestore(false)
        , m_bRectOnly(false)
    {
    }
    CStateDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr)
        : CDialog(nIDTemplate, pParentWnd)
        , m_bEnableSaveRestore(false)
        , m_bRectOnly(false)
    {
    }
    CStateDialog(LPCWSTR lpszTemplateName, CWnd* pParentWnd = nullptr)
        : CDialog(lpszTemplateName, pParentWnd)
        , m_bEnableSaveRestore(false)
        , m_bRectOnly(false)
    {
    }

    ~CStateDialog() override{};

private:
    // flags
    bool m_bEnableSaveRestore;
    bool m_bRectOnly;

    // internal status
    CString m_sSection; // section name (identifies a parent window)

protected:
    // overloaded method, but since this dialog class is for non-resizable dialogs,
    // the bHorzResize and bVertResize params are ignored and passed as false
    // to the base method.
    void  EnableSaveRestore(LPCWSTR pszSection, bool bRectOnly = FALSE, BOOL bHorzResize = TRUE, BOOL bVertResize = TRUE);
    ULONG GetGestureStatus(CPoint /*ptTouch*/) override
    {
        return 0;
    }

    CWnd* GetResizableWnd() const override
    {
        // make the layout know its parent window
        return CWnd::FromHandle(m_hWnd);
    };

    afx_msg void OnDestroy()
    {
        if (m_bEnableSaveRestore)
            SaveWindowRect(m_sSection, m_bRectOnly);
        CDialog::OnDestroy();
    };

    DECLARE_MESSAGE_MAP()
};

class CResizableStandAloneDialog : public CStandAloneDialogTmpl<CResizableDialog>
{
public:
    CResizableStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr);

private:
    DECLARE_DYNAMIC(CResizableStandAloneDialog)

protected:
    afx_msg void    OnSizing(UINT fwSide, LPRECT pRect);
    afx_msg void    OnMoving(UINT fwSide, LPRECT pRect);
    afx_msg void    OnNcMButtonUp(UINT nHitTest, CPoint point);
    afx_msg void    OnNcRButtonUp(UINT nHitTest, CPoint point);
    afx_msg LRESULT OnNcHitTest(CPoint point);
    void            OnCantStartThread() const;
    bool            OnEnterPressed();

    DECLARE_MESSAGE_MAP()

private:
    bool  m_bVertical;
    bool  m_bHorizontal;
    CRect m_rcOrgWindowRect;
    int   m_stickySize;

public:
};

class CStandAloneDialog : public CStandAloneDialogTmpl<CDialog>
{
public:
    CStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr);

protected:
    DECLARE_MESSAGE_MAP()
private:
    DECLARE_DYNAMIC(CStandAloneDialog)
};

class CStateStandAloneDialog : public CStandAloneDialogTmpl<CStateDialog>
{
public:
    CStateStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr);

protected:
    DECLARE_MESSAGE_MAP()
private:
    DECLARE_DYNAMIC(CStateStandAloneDialog)
};

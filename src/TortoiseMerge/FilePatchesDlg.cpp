﻿// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006, 2008, 2010-2017, 2020-2021, 2024 - TortoiseSVN

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
#include "FilePatchesDlg.h"
#include "SVNPatch.h"
#include "PathUtils.h"
#include "ProgressDlg.h"
#include "SysImageList.h"
#include "Theme.h"

IMPLEMENT_DYNAMIC(CFilePatchesDlg, CResizableStandAloneDialog)
CFilePatchesDlg::CFilePatchesDlg(CWnd *pParent /*=nullptr*/)
    : CResizableStandAloneDialog(CFilePatchesDlg::IDD, pParent)
    , m_pPatch(nullptr)
    , m_pCallBack(nullptr)
    , m_bMinimized(FALSE)
    , m_nWindowHeight(-1)
    , m_pMainFrame(nullptr)
    , m_shownIndex(-1)
    , m_boldFont(nullptr)
{
}

CFilePatchesDlg::~CFilePatchesDlg()
{
    if (m_boldFont)
        DeleteObject(m_boldFont);
}

void CFilePatchesDlg::DoDataExchange(CDataExchange *pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_FILELIST, m_cFileList);
}

BOOL CFilePatchesDlg::SetFileStatusAsPatched(CString sPath)
{
    for (int i = 0; i < m_arFileStates.GetCount(); i++)
    {
        if (sPath.CompareNoCase(GetFullPath(i)) == 0)
        {
            m_arFileStates.SetAt(i, static_cast<DWORD>(FPDLG_FILESTATE_PATCHED));
            SetStateText(i, FPDLG_FILESTATE_PATCHED);
            Invalidate();
            return TRUE;
        }
    }
    return FALSE;
}

CString CFilePatchesDlg::GetFullPath(int nIndex) const
{
    CString temp = m_pPatch->GetFilePath(nIndex);
    temp.Replace('/', '\\');
    //temp = temp.Mid(temp.Find('\\')+1);
    if (PathIsRelative(temp))
        temp = m_sPath + temp;
    return temp;
}

BOOL CFilePatchesDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();

    ExtendFrameIntoClientArea(IDC_FILELIST);
    m_aeroControls.SubclassControl(this, IDC_PATCHSELECTEDBUTTON);
    m_aeroControls.SubclassControl(this, IDC_PATCHALLBUTTON);

    // hide the grip since it would overlap with the "path all" button
    HideGrip();

    HFONT   hFont = reinterpret_cast<HFONT>(m_cFileList.SendMessage(WM_GETFONT));
    LOGFONT lf    = {0};
    GetObject(hFont, sizeof(LOGFONT), &lf);
    lf.lfWeight = FW_BOLD;
    m_boldFont  = CreateFontIndirect(&lf);

    AddAnchor(IDC_FILELIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_PATCHSELECTEDBUTTON, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_PATCHALLBUTTON, BOTTOM_LEFT, BOTTOM_RIGHT);

    return TRUE;
}

BOOL CFilePatchesDlg::Init(SVNPatch *pPatch, CPatchFilesDlgCallBack *pCallBack, CString sPath, CWnd *pParent)
{
    if (!pCallBack || !pPatch)
    {
        m_cFileList.DeleteAllItems();
        return FALSE;
    }
    m_arFileStates.RemoveAll();
    m_pPatch    = pPatch;
    m_pCallBack = pCallBack;
    m_sPath     = sPath;
    if (m_sPath.IsEmpty())
    {
        CString title(MAKEINTRESOURCE(IDS_DIFF_TITLE));
        SetWindowText(title);
    }
    else
    {
        CRect rect;
        GetClientRect(&rect);
        SetTitleWithPath(rect.Width());
        if (m_sPath.Right(1).Compare(L"\\") == 0)
            m_sPath = m_sPath.Left(m_sPath.GetLength() - 1);

        m_sPath = m_sPath + L"\\";
    }

    m_cFileList.SetExtendedStyle(LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_cFileList.DeleteAllItems();
    int c = m_cFileList.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_cFileList.DeleteColumn(c--);
    m_cFileList.InsertColumn(0, CString(MAKEINTRESOURCE(IDS_PATH)));
    m_cFileList.InsertColumn(1, CString(MAKEINTRESOURCE(IDS_STATE)));

    m_cFileList.SetRedraw(false);

    for (int i = 0; i < m_pPatch->GetNumberOfFiles(); i++)
    {
        CString sFile = CPathUtils::GetFileNameFromPath(m_pPatch->GetFilePath(i));
        if (m_pPatch->GetPropMods(i))
        {
            sFile += L" ";
            if (m_pPatch->GetContentMods(i))
                sFile += CString(MAKEINTRESOURCE(IDS_PROPANDCONTENTMODS));
            else
                sFile += CString(MAKEINTRESOURCE(IDS_PROPMODS));
        }
        int state;
        if (m_sPath.IsEmpty())
            state = 0;
        else
        {
            state = m_pPatch->GetFailedHunks(i);
        }
        m_arFileStates.Add(state);
        CString sFileName = GetFullPath(i);
        sFileName         = CPathUtils::GetFileNameFromPath(sFileName);
        m_cFileList.InsertItem(i, sFile, SYS_IMAGE_LIST().GetPathIconIndex(sFileName));
        SetStateText(i, state);
    }
    int minCol = 0;
    int maxCol = m_cFileList.GetHeaderCtrl()->GetItemCount() - 1;
    int col    = 0;
    for (col = minCol; col <= maxCol; col++)
    {
        m_cFileList.SetColumnWidth(col, LVSCW_AUTOSIZE_USEHEADER);
    }

    m_cFileList.SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);
    m_cFileList.SetRedraw(true);

    RECT parentRect;
    pParent->GetWindowRect(&parentRect);
    RECT windowRect;
    GetWindowRect(&windowRect);

    int width        = windowRect.right - windowRect.left;
    int height       = windowRect.bottom - windowRect.top;
    windowRect.right = parentRect.left;
    windowRect.left  = windowRect.right - width;

    windowRect.top    = parentRect.top;
    windowRect.bottom = windowRect.top + height;

    auto hMonitor = MonitorFromRect(&windowRect, MONITOR_DEFAULTTONULL);
    if (hMonitor)
        SetWindowPos(nullptr, windowRect.left, windowRect.top, width, height, SWP_NOACTIVATE | SWP_NOZORDER);

    m_nWindowHeight = windowRect.bottom - windowRect.top;
    m_pMainFrame    = pParent;
    return TRUE;
}

BEGIN_MESSAGE_MAP(CFilePatchesDlg, CResizableStandAloneDialog)
    ON_WM_SIZE()
    ON_NOTIFY(LVN_GETINFOTIP, IDC_FILELIST, OnLvnGetInfoTipFilelist)
    ON_NOTIFY(NM_DBLCLK, IDC_FILELIST, OnNMDblclkFilelist)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_FILELIST, OnNMCustomdrawFilelist)
    ON_NOTIFY(NM_RCLICK, IDC_FILELIST, OnNMRclickFilelist)
    ON_WM_NCLBUTTONDBLCLK()
    ON_WM_MOVING()
    ON_BN_CLICKED(IDC_PATCHSELECTEDBUTTON, &CFilePatchesDlg::OnBnClickedPatchselectedbutton)
    ON_BN_CLICKED(IDC_PATCHALLBUTTON, &CFilePatchesDlg::OnBnClickedPatchallbutton)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_FILELIST, &CFilePatchesDlg::OnLvnItemchangedFilelist)
END_MESSAGE_MAP()

void CFilePatchesDlg::OnSize(UINT nType, int cx, int cy)
{
    CResizableStandAloneDialog::OnSize(nType, cx, cy);
    if (this->IsWindowVisible())
    {
        m_cFileList.SetColumnWidth(0, LVSCW_AUTOSIZE);
    }
    SetTitleWithPath(cx);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CFilePatchesDlg::OnLvnGetInfoTipFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);

    if (m_arFileStates.GetCount() > pGetInfoTip->iItem)
    {
        CString temp;
        if (m_arFileStates.GetAt(pGetInfoTip->iItem) == 0)
            temp = GetFullPath(pGetInfoTip->iItem);
        else
            temp.Format(IDS_PATCH_ITEMTT, static_cast<LPCWSTR>(GetFullPath(pGetInfoTip->iItem)), m_arFileStates.GetAt(pGetInfoTip->iItem));
        wcsncpy_s(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, temp, pGetInfoTip->cchTextMax - 1LL);
    }
    else
        pGetInfoTip->pszText[0] = 0;

    *pResult = 0;
}

void CFilePatchesDlg::OnNMDblclkFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    *pResult           = 0;
    if ((pNMLV->iItem < 0) || (pNMLV->iItem >= m_arFileStates.GetCount()))
        return;
    if (!m_pCallBack)
        return;
    if (m_sPath.IsEmpty())
    {
        m_pCallBack->DiffFiles(GetFullPath(pNMLV->iItem), L"",
                               L"", L"");
        m_shownIndex = pNMLV->iItem;
        m_cFileList.Invalidate();
    }
    else
    {
        if (m_arFileStates.GetAt(pNMLV->iItem) != FPDLG_FILESTATE_PATCHED)
        {
            m_pCallBack->PatchFile(m_pPatch->GetFilePath(pNMLV->iItem), m_pPatch->GetContentMods(pNMLV->iItem), m_pPatch->GetPropMods(pNMLV->iItem), L"");
            m_shownIndex = pNMLV->iItem;
            m_cFileList.Invalidate();
        }
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CFilePatchesDlg::OnNMCustomdrawFilelist(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVCUSTOMDRAW *pLVCD = reinterpret_cast<NMLVCUSTOMDRAW *>(pNMHDR);

    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    // First thing - check the draw stage. If it's the control's prepaint
    // stage, then tell Windows we want messages for every item.

    if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        *pResult = CDRF_NOTIFYITEMDRAW;
    }
    else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
    {
        // This is the prepaint stage for an item. Here's where we set the
        // item's text color. Our return value will tell Windows to draw the
        // item itself, but it will use the new color we set here.

        COLORREF crText = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);

        if (m_arFileStates.GetCount() > static_cast<INT_PTR>(pLVCD->nmcd.dwItemSpec))
        {
            if (m_arFileStates.GetAt(pLVCD->nmcd.dwItemSpec) == FPDLG_FILESTATE_ERROR)
            {
                crText = CTheme::Instance().GetThemeColor(RGB(200, 0, 0), true);
            }
            if (m_arFileStates.GetAt(pLVCD->nmcd.dwItemSpec) > 0)
            {
                crText = CTheme::Instance().GetThemeColor(RGB(100, 0, 0), true);
            }
            if (m_arFileStates.GetAt(pLVCD->nmcd.dwItemSpec) == FPDLG_FILESTATE_PATCHED)
            {
                crText = CTheme::Instance().GetThemeColor(::GetSysColor(COLOR_GRAYTEXT));
            }
            // Store the color back in the NMLVCUSTOMDRAW struct.
            pLVCD->clrText = crText;
            if (m_shownIndex == static_cast<int>(pLVCD->nmcd.dwItemSpec))
            {
                SelectObject(pLVCD->nmcd.hdc, m_boldFont);
                // We changed the font, so we're returning CDRF_NEWFONT. This
                // tells the control to recalculate the extent of the text.
                *pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
            }
        }
    }
}

void CFilePatchesDlg::OnNMRclickFilelist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    *pResult = 0;
    if (m_sPath.IsEmpty())
        return;
    CString temp;
    CMenu   popup;
    POINT   point;
    DWORD   ptW = GetMessagePos();
    point.x     = GET_X_LPARAM(ptW);
    point.y     = GET_Y_LPARAM(ptW);
    if (!popup.CreatePopupMenu())
        return;

    UINT nFlags = MF_STRING | (m_cFileList.GetSelectedCount() == 1 ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
    temp.LoadString(IDS_PATCH_PREVIEW);
    popup.AppendMenu(nFlags, ID_PATCHPREVIEW, temp);
    popup.SetDefaultItem(ID_PATCHPREVIEW, FALSE);

    temp.LoadString(IDS_PATCH_ALL);
    popup.AppendMenu(MF_STRING | MF_ENABLED, ID_PATCHALL, temp);

    nFlags = MF_STRING | (m_cFileList.GetSelectedCount() > 0 ? MF_ENABLED : MF_DISABLED | MF_GRAYED);
    temp.LoadString(IDS_PATCH_SELECTED);
    popup.AppendMenu(nFlags, ID_PATCHSELECTED, temp);

    // if the context menu is invoked through the keyboard, we have to use
    // a calculated position on where to anchor the menu on
    if ((point.x == -1) && (point.y == -1))
    {
        CRect rect;
        GetWindowRect(&rect);
        point = rect.CenterPoint();
    }

    int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, nullptr);
    switch (cmd)
    {
        case ID_PATCHPREVIEW:
            if (m_pCallBack)
            {
                int nIndex = m_cFileList.GetSelectionMark();
                if (m_arFileStates.GetAt(nIndex) != FPDLG_FILESTATE_PATCHED)
                {
                    m_pCallBack->PatchFile(m_pPatch->GetFilePath(nIndex), m_pPatch->GetContentMods(nIndex), m_pPatch->GetPropMods(nIndex), L"");
                    m_shownIndex = nIndex;
                    m_cFileList.Invalidate();
                }
            }
            break;
        case ID_PATCHALL:
            PatchAll();
            break;
        case ID_PATCHSELECTED:
            PatchSelected();
            break;
        default:
            break;
    }
}

void CFilePatchesDlg::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
    if (!m_bMinimized)
    {
        RECT windowRect{};
        RECT clientRect{};
        GetWindowRect(&windowRect);
        GetClientRect(&clientRect);
        m_nWindowHeight = windowRect.bottom - windowRect.top;
        MoveWindow(windowRect.left, windowRect.top,
                   windowRect.right - windowRect.left,
                   m_nWindowHeight - (clientRect.bottom - clientRect.top));
    }
    else
    {
        RECT windowRect{};
        GetWindowRect(&windowRect);
        MoveWindow(windowRect.left, windowRect.top, windowRect.right - windowRect.left, m_nWindowHeight);
    }
    m_bMinimized = !m_bMinimized;
    CResizableStandAloneDialog::OnNcLButtonDblClk(nHitTest, point);
}

void CFilePatchesDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
    RECT parentRect{};
    m_pMainFrame->GetWindowRect(&parentRect);
    const int stickySize = 5;
    if (abs(parentRect.left - pRect->right) < stickySize)
    {
        int width    = pRect->right - pRect->left;
        pRect->right = parentRect.left;
        pRect->left  = pRect->right - width;
    }
    CResizableStandAloneDialog::OnMoving(fwSide, pRect);
}

void CFilePatchesDlg::OnOK()
{
    return;
}

void CFilePatchesDlg::SetTitleWithPath(int width)
{
    CString title;
    title.LoadString(IDS_PATCH_TITLE);
    title += L"  " + m_sPath;
    title    = title.Left(MAX_PATH - 1);
    CDC *pDC = GetDC();
    if (pDC)
    {
        PathCompactPath(pDC->GetSafeHdc(), title.GetBuffer(), width);
        ReleaseDC(pDC);
    }
    title.ReleaseBuffer();
    SetWindowText(title);
}

void CFilePatchesDlg::OnBnClickedPatchselectedbutton()
{
    PatchSelected();
}

void CFilePatchesDlg::OnBnClickedPatchallbutton()
{
    PatchAll();
}

void CFilePatchesDlg::PatchAll()
{
    if (m_pCallBack)
    {
        CProgressDlg progDlg;
        progDlg.SetTitle(IDR_MAINFRAME);
        progDlg.SetShowProgressBar(true);
        progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PATCH_ALL)));
        progDlg.ShowModeless(m_hWnd);
        for (int i = 0; i < m_arFileStates.GetCount() && !progDlg.HasUserCancelled(); i++)
        {
            if (m_arFileStates.GetAt(i) != FPDLG_FILESTATE_PATCHED)
            {
                progDlg.SetLine(2, GetFullPath(i), true);
                m_pCallBack->PatchFile(m_pPatch->GetFilePath(i), m_pPatch->GetContentMods(i), m_pPatch->GetPropMods(i), L"", TRUE);
                m_shownIndex = i;
                m_cFileList.Invalidate();
            }
            progDlg.SetProgress64(i, m_arFileStates.GetCount());
        }
        progDlg.Stop();
    }
}

void CFilePatchesDlg::PatchSelected()
{
    if (m_pCallBack)
    {
        CProgressDlg progDlg;
        progDlg.SetTitle(IDR_MAINFRAME);
        progDlg.SetShowProgressBar(true);
        progDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_PATCH_SELECTED)));
        progDlg.ShowModeless(m_hWnd);
        // The list cannot be sorted by user, so the order of the
        // items in the list is identical to the order in the array
        // m_arFileStates.
        int      selCount = m_cFileList.GetSelectedCount();
        int      count    = 1;
        POSITION pos      = m_cFileList.GetFirstSelectedItemPosition();
        int      index;
        while (((index = m_cFileList.GetNextSelectedItem(pos)) >= 0) && (!progDlg.HasUserCancelled()))
        {
            if (m_arFileStates.GetAt(index) != FPDLG_FILESTATE_PATCHED)
            {
                progDlg.SetLine(2, GetFullPath(index), true);
                m_pCallBack->PatchFile(m_pPatch->GetFilePath(index), m_pPatch->GetContentMods(index), m_pPatch->GetPropMods(index), L"", TRUE);
                m_shownIndex = index;
                m_cFileList.Invalidate();
            }
            progDlg.SetProgress64(count++, selCount);
        }
        progDlg.Stop();
    }
}

void CFilePatchesDlg::OnLvnItemchangedFilelist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
    DialogEnableWindow(IDC_PATCHSELECTEDBUTTON, m_cFileList.GetSelectedCount() > 0);

    *pResult = 0;
}

void CFilePatchesDlg::SetStateText(int i, int state)
{
    CString sState;
    switch (state)
    {
        case FPDLG_FILESTATE_PATCHED:
            sState.LoadString(IDS_STATE_PATCHED);
            break;
        case FPDLG_FILESTATE_ERROR:
            // not used
            break;
        case 0:
            // all is ok, not yet patched but no failed hunks
            break;
        default:
            // there are failed hunks in the patch
            sState.Format(IDS_STATE_ERROR, state);
            break;
    }
    m_cFileList.SetItemText(i, 1, sState);
}

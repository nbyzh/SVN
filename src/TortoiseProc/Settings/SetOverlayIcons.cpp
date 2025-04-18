﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010-2014, 2018, 2020-2021, 2024 - TortoiseSVN

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
#include "DirFileEnum.h"
#include "SetOverlayIcons.h"
#include "SVNStatus.h"
#include "PathUtils.h"
#include "SmartHandle.h"
#include "LoadIconEx.h"

IMPLEMENT_DYNAMIC(CSetOverlayIcons, ISettingsPropPage)
CSetOverlayIcons::CSetOverlayIcons()
    : ISettingsPropPage(CSetOverlayIcons::IDD)
    , m_selIndex(0)
{
    m_regNormal      = CRegString(L"Software\\TortoiseOverlays\\NormalIcon");
    m_regModified    = CRegString(L"Software\\TortoiseOverlays\\ModifiedIcon");
    m_regConflicted  = CRegString(L"Software\\TortoiseOverlays\\ConflictIcon");
    m_regReadOnly    = CRegString(L"Software\\TortoiseOverlays\\ReadOnlyIcon");
    m_regDeleted     = CRegString(L"Software\\TortoiseOverlays\\DeletedIcon");
    m_regLocked      = CRegString(L"Software\\TortoiseOverlays\\LockedIcon");
    m_regAdded       = CRegString(L"Software\\TortoiseOverlays\\AddedIcon");
    m_regIgnored     = CRegString(L"Software\\TortoiseOverlays\\IgnoredIcon");
    m_regUnversioned = CRegString(L"Software\\TortoiseOverlays\\UnversionedIcon");
}

CSetOverlayIcons::~CSetOverlayIcons()
{
}

void CSetOverlayIcons::DoDataExchange(CDataExchange* pDX)
{
    ISettingsPropPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_ICONSETCOMBO, m_cIconSet);
    DDX_Control(pDX, IDC_ICONLIST, m_cIconList);
}

BEGIN_MESSAGE_MAP(CSetOverlayIcons, ISettingsPropPage)
    ON_BN_CLICKED(IDC_LISTRADIO, OnBnClickedListradio)
    ON_BN_CLICKED(IDC_SYMBOLRADIO, OnBnClickedSymbolradio)
    ON_CBN_SELCHANGE(IDC_ICONSETCOMBO, OnCbnSelchangeIconsetcombo)
END_MESSAGE_MAP()

BOOL CSetOverlayIcons::OnInitDialog()
{
    ISettingsPropPage::OnInitDialog();

    m_cIconList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES);
    // get the path to our icon sets
    wchar_t buf[MAX_PATH] = {0};
    SHGetSpecialFolderPath(m_hWnd, buf, CSIDL_PROGRAM_FILES_COMMON, true);
    m_sIconPath = buf;
    m_sIconPath += L"\\TortoiseOverlays\\Icons";
    // list all the icon sets
    CDirFileEnum fileFinder(m_sIconPath);
    bool         isDir = false;
    CString      item;
    while (fileFinder.NextFile(item, &isDir))
    {
        if (!isDir)
            continue;
        m_cIconSet.AddString(CPathUtils::GetFileNameFromPath(item));
    }
    CheckRadioButton(IDC_LISTRADIO, IDC_SYMBOLRADIO, IDC_LISTRADIO);
    CString sModifiedIcon = m_regModified;
    if (sModifiedIcon.IsEmpty())
    {
        // no custom icon set, use the default
        sModifiedIcon = m_sIconPath + L"\\XPStyle\\ModifiedIcon.ico";
    }
    if (sModifiedIcon.Left(m_sIconPath.GetLength()).CompareNoCase(m_sIconPath) != 0)
    {
        // an icon set outside our own installation? We don't support that,
        // so fall back to the default!
        sModifiedIcon = m_sIconPath + L"\\XPStyle\\ModifiedIcon.ico";
    }
    // the name of the icon set is the folder of the icon location
    m_sOriginalIconSet = sModifiedIcon.Mid(m_sIconPath.GetLength() + 1);
    m_sOriginalIconSet = m_sOriginalIconSet.Left(m_sOriginalIconSet.ReverseFind('\\'));
    // now we have the name of the icon set. Set the combobox to show
    // that as selected
    CString ComboItem;
    for (int i = 0; i < m_cIconSet.GetCount(); ++i)
    {
        m_cIconSet.GetLBText(i, ComboItem);
        if (ComboItem.CompareNoCase(m_sOriginalIconSet) == 0)
            m_cIconSet.SetCurSel(i);
    }
    WORD    langID                               = static_cast<WORD>(static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", GetUserDefaultLangID())));
    wchar_t statusText[MAX_STATUS_STRING_LENGTH] = {0};
    SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_normal, statusText, _countof(statusText), langID);
    m_sNormal = statusText;
    SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_modified, statusText, _countof(statusText), langID);
    m_sModified = statusText;
    SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_conflicted, statusText, _countof(statusText), langID);
    m_sConflicted = statusText;
    SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_deleted, statusText, _countof(statusText), langID);
    m_sDeleted = statusText;
    SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_added, statusText, _countof(statusText), langID);
    m_sAdded = statusText;
    SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_ignored, statusText, _countof(statusText), langID);
    m_sIgnored = statusText;
    SVNStatus::GetStatusString(AfxGetResourceHandle(), svn_wc_status_unversioned, statusText, _countof(statusText), langID);
    m_sUnversioned = statusText;

    m_sReadOnly.LoadString(IDS_SETTINGS_READONLYNAME);
    m_sLocked.LoadString(IDS_SETTINGS_LOCKEDNAME);

    ShowIconSet(true);

    return TRUE;
}

void CSetOverlayIcons::ShowIconSet(bool bSmallIcons)
{
    m_cIconList.SetRedraw(FALSE);
    m_cIconList.DeleteAllItems();
    m_imageList.DeleteImageList();
    m_imageListBig.DeleteImageList();
    int smallIconWidth   = GetSystemMetrics(SM_CXSMICON);
    int smallIconHeight  = GetSystemMetrics(SM_CYSMICON);
    int normalIconWidth  = GetSystemMetrics(SM_CXICON);
    int normalIconHeight = GetSystemMetrics(SM_CYICON);
    m_imageList.Create(smallIconWidth, smallIconHeight, ILC_COLOR32 | ILC_MASK | ILC_HIGHQUALITYSCALE, 20, 10);
    m_imageListBig.Create(normalIconWidth, normalIconHeight, ILC_COLOR32 | ILC_MASK | ILC_HIGHQUALITYSCALE, 20, 10);
    m_cIconList.SetImageList(&m_imageList, LVSIL_SMALL);
    m_cIconList.SetImageList(&m_imageListBig, LVSIL_NORMAL);

    // find all the icons of the selected icon set
    CString sIconSet;
    int     index = m_cIconSet.GetCurSel();
    if (index == CB_ERR)
    {
        // nothing selected. Shouldn't happen!
        return;
    }
    m_cIconSet.GetLBText(index, sIconSet);
    CString sIconSetPath = m_sIconPath + L"\\" + sIconSet;

    CImageList* pImageList     = bSmallIcons ? &m_imageList : &m_imageListBig;
    int         iconWidth      = (bSmallIcons ? smallIconWidth : normalIconWidth);
    int         iconHeight     = (bSmallIcons ? smallIconHeight : normalIconHeight);
    CAutoIcon   hNormalOverlay = LoadIconEx(nullptr, sIconSetPath + L"\\NormalIcon.ico", iconWidth, iconHeight);
    index                      = pImageList->Add(hNormalOverlay);
    VERIFY(pImageList->SetOverlayImage(index, 1));
    CAutoIcon hModifiedOverlay = LoadIconEx(nullptr, sIconSetPath + L"\\ModifiedIcon.ico", iconWidth, iconHeight);
    index                      = pImageList->Add(hModifiedOverlay);
    VERIFY(pImageList->SetOverlayImage(index, 2));
    CAutoIcon hConflictedOverlay = LoadIconEx(nullptr, sIconSetPath + L"\\ConflictIcon.ico", iconWidth, iconHeight);
    index                        = pImageList->Add(hConflictedOverlay);
    VERIFY(pImageList->SetOverlayImage(index, 3));
    CAutoIcon hReadOnlyOverlay = LoadIconEx(nullptr, sIconSetPath + L"\\ReadOnlyIcon.ico", iconWidth, iconHeight);
    index                      = pImageList->Add(hReadOnlyOverlay);
    VERIFY(pImageList->SetOverlayImage(index, 4));
    CAutoIcon hDeletedOverlay = LoadIconEx(nullptr, sIconSetPath + L"\\DeletedIcon.ico", iconWidth, iconHeight);
    index                     = pImageList->Add(hDeletedOverlay);
    VERIFY(pImageList->SetOverlayImage(index, 5));
    CAutoIcon hLockedOverlay = LoadIconEx(nullptr, sIconSetPath + L"\\LockedIcon.ico", iconWidth, iconHeight);
    index                    = pImageList->Add(hLockedOverlay);
    VERIFY(pImageList->SetOverlayImage(index, 6));
    CAutoIcon hAddedOverlay = LoadIconEx(nullptr, sIconSetPath + L"\\AddedIcon.ico", iconWidth, iconHeight);
    index                   = pImageList->Add(hAddedOverlay);
    VERIFY(pImageList->SetOverlayImage(index, 7));
    CAutoIcon hIgnoredOverlay = LoadIconEx(nullptr, sIconSetPath + L"\\IgnoredIcon.ico", iconWidth, iconHeight);
    index                     = pImageList->Add(hIgnoredOverlay);
    VERIFY(pImageList->SetOverlayImage(index, 8));
    CAutoIcon hUnversionedOverlay = LoadIconEx(nullptr, sIconSetPath + L"\\UnversionedIcon.ico", iconWidth, iconHeight);
    index                         = pImageList->Add(hUnversionedOverlay);
    VERIFY(pImageList->SetOverlayImage(index, 9));

    // create an image list with different file icons
    CAutoIcon hIcon       = GetFileIcon(L"Doesn't matter", bSmallIcons, FILE_ATTRIBUTE_DIRECTORY);
    int       folderindex = pImageList->Add(hIcon); //folder

    //folders
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sNormal, folderindex);
    VERIFY(m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(1), TVIS_OVERLAYMASK));
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sModified, folderindex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(2), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sConflicted, folderindex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(3), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sReadOnly, folderindex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(4), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sDeleted, folderindex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(5), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sLocked, folderindex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(6), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sAdded, folderindex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(7), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sIgnored, folderindex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(8), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sUnversioned, folderindex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(9), TVIS_OVERLAYMASK);

    AddFileTypeGroup(L".cpp", bSmallIcons);
    AddFileTypeGroup(L".h", bSmallIcons);
    AddFileTypeGroup(L".txt", bSmallIcons);
    AddFileTypeGroup(L".java", bSmallIcons);
    AddFileTypeGroup(L".doc", bSmallIcons);
    AddFileTypeGroup(L".pl", bSmallIcons);
    AddFileTypeGroup(L".php", bSmallIcons);
    AddFileTypeGroup(L".asp", bSmallIcons);
    AddFileTypeGroup(L".cs", bSmallIcons);
    AddFileTypeGroup(L".vb", bSmallIcons);
    AddFileTypeGroup(L".xml", bSmallIcons);
    AddFileTypeGroup(L".pas", bSmallIcons);
    AddFileTypeGroup(L".dpr", bSmallIcons);
    AddFileTypeGroup(L".dfm", bSmallIcons);
    AddFileTypeGroup(L".res", bSmallIcons);
    AddFileTypeGroup(L".asmx", bSmallIcons);
    AddFileTypeGroup(L".aspx", bSmallIcons);
    AddFileTypeGroup(L".resx", bSmallIcons);
    AddFileTypeGroup(L".vbp", bSmallIcons);
    AddFileTypeGroup(L".frm", bSmallIcons);
    AddFileTypeGroup(L".frx", bSmallIcons);
    AddFileTypeGroup(L".bas", bSmallIcons);
    AddFileTypeGroup(L".config", bSmallIcons);
    AddFileTypeGroup(L".css", bSmallIcons);
    AddFileTypeGroup(L".acsx", bSmallIcons);
    AddFileTypeGroup(L".jpg", bSmallIcons);
    AddFileTypeGroup(L".png", bSmallIcons);
    m_cIconList.SetRedraw(TRUE);
}
void CSetOverlayIcons::AddFileTypeGroup(CString sFileType, bool bSmallIcons)
{
    CAutoIcon hIcon = GetFileIcon(sFileType, bSmallIcons, FILE_ATTRIBUTE_NORMAL);

    int imageIndex = 0;
    if (bSmallIcons)
        imageIndex = m_imageList.Add(hIcon);
    else
        imageIndex = m_imageListBig.Add(hIcon);

    int index = 0;
    index     = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sNormal + sFileType, imageIndex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(1), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sModified + sFileType, imageIndex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(2), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sConflicted + sFileType, imageIndex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(3), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sReadOnly + sFileType, imageIndex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(4), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sDeleted + sFileType, imageIndex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(5), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sLocked + sFileType, imageIndex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(6), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sAdded + sFileType, imageIndex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(7), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sIgnored + sFileType, imageIndex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(8), TVIS_OVERLAYMASK);
    index = m_cIconList.InsertItem(m_cIconList.GetItemCount(), m_sUnversioned + sFileType, imageIndex);
    m_cIconList.SetItemState(index, INDEXTOOVERLAYMASK(9), TVIS_OVERLAYMASK);
}

void CSetOverlayIcons::OnBnClickedListradio()
{
    m_cIconList.ModifyStyle(LVS_TYPEMASK, LVS_LIST);
    ShowIconSet(true);
}

void CSetOverlayIcons::OnBnClickedSymbolradio()
{
    m_cIconList.ModifyStyle(LVS_TYPEMASK, LVS_ICON);
    ShowIconSet(false);
}

void CSetOverlayIcons::OnCbnSelchangeIconsetcombo()
{
    bool bSmallIcons = (GetCheckedRadioButton(IDC_LISTRADIO, IDC_SYMBOLRADIO) == IDC_LISTRADIO);
    ShowIconSet(bSmallIcons);
    m_selIndex = m_cIconSet.GetCurSel();
    if (m_selIndex != CB_ERR)
    {
        m_cIconSet.GetLBText(m_selIndex, m_sIconSet);
    }
    if (m_sIconSet.CompareNoCase(m_sOriginalIconSet) != 0)
        SetModified();
}

BOOL CSetOverlayIcons::OnApply()
{
    UpdateData();

    if ((!m_sIconSet.IsEmpty()) && (m_sIconSet.CompareNoCase(m_sOriginalIconSet) != 0))
    {
        // the selected icon set has changed.
        Store(m_sIconPath + L"\\" + m_sIconSet + L"\\NormalIcon.ico", m_regNormal);
        Store(m_sIconPath + L"\\" + m_sIconSet + L"\\ModifiedIcon.ico", m_regModified);
        Store(m_sIconPath + L"\\" + m_sIconSet + L"\\ConflictIcon.ico", m_regConflicted);
        Store(m_sIconPath + L"\\" + m_sIconSet + L"\\ReadOnlyIcon.ico", m_regReadOnly);
        Store(m_sIconPath + L"\\" + m_sIconSet + L"\\DeletedIcon.ico", m_regDeleted);
        Store(m_sIconPath + L"\\" + m_sIconSet + L"\\LockedIcon.ico", m_regLocked);
        Store(m_sIconPath + L"\\" + m_sIconSet + L"\\AddedIcon.ico", m_regAdded);
        Store(m_sIconPath + L"\\" + m_sIconSet + L"\\IgnoredIcon.ico", m_regIgnored);
        Store(m_sIconPath + L"\\" + m_sIconSet + L"\\UnversionedIcon.ico", m_regUnversioned);

        m_restart          = Restart_System;
        m_sOriginalIconSet = m_sIconSet;
    }
    SetModified(FALSE);
    return ISettingsPropPage::OnApply();
}

HICON CSetOverlayIcons::GetFileIcon(LPCWSTR fileType, bool bSmallIcons, DWORD attributes)
{
    SHFILEINFO sfi = {nullptr};

    UINT flags = SHGFI_ICON | SHGFI_USEFILEATTRIBUTES;
    if (bSmallIcons)
        flags |= SHGFI_SMALLICON;
    else
        flags |= SHGFI_LARGEICON;
    SHGetFileInfo(
        fileType,
        attributes,
        &sfi, sizeof sfi,
        flags);
    return sfi.hIcon;
}
﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2020-2021, 2024 - TortoiseSVN

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
#include "SVNProperties.h"
#include "TortoiseProc.h"
#include "EditPropertiesDlg.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "ProgressDlg.h"
#include "InputLogDlg.h"
#include "JobScheduler.h"
#include "AsyncCall.h"
#include "IconMenu.h"

#include "EditPropertyValueDlg.h"
#include "EditPropExecutable.h"
#include "EditPropNeedsLock.h"
#include "EditPropMimeType.h"
#include "EditPropBugtraq.h"
#include "EditPropEOL.h"
#include "EditPropKeywords.h"
#include "EditPropExternals.h"
#include "EditPropTSVNSizes.h"
#include "EditPropTSVNLang.h"
#include "EditPropsLocalHooks.h"
#include "EditPropUserBool.h"
#include "EditPropUserState.h"
#include "EditPropUserSingleLine.h"
#include "EditPropUserMultiLine.h"
#include "EditPropMergeLogTemplate.h"

#define ID_CMD_PROP_SAVEVALUE 1
#define ID_CMD_PROP_REMOVE    2
#define ID_CMD_PROP_EDIT      3

#define IDCUSTOM1 23
#define IDCUSTOM2 24

IMPLEMENT_DYNAMIC(CEditPropertiesDlg, CResizableStandAloneDialog)

CEditPropertiesDlg::CEditPropertiesDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CEditPropertiesDlg::IDD, pParent)
    , m_bRecursive(FALSE)
    , m_bChanged(false)
    , m_bRevProps(false)
    , m_bUrlIsFolder(false)
    , m_bThreadRunning(false)
    , m_pProjectProperties(nullptr)
    , m_bCancelled(false)
    , m_nSortedColumn(-1)
    , m_bAscending(false)
{
}

CEditPropertiesDlg::~CEditPropertiesDlg()
{
}

void CEditPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDITPROPLIST, m_propList);
    DDX_Control(pDX, IDC_PROPPATH, m_propPath);
    DDX_Control(pDX, IDC_ADDPROPS, m_btnNew);
    DDX_Control(pDX, IDC_EDITPROPS, m_btnEdit);
}

BEGIN_MESSAGE_MAP(CEditPropertiesDlg, CResizableStandAloneDialog)
    ON_REGISTERED_MESSAGE(WM_AFTERTHREAD, OnAfterThread)
    ON_BN_CLICKED(IDHELP, &CEditPropertiesDlg::OnBnClickedHelp)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnNMCustomdrawEditproplist)
    ON_BN_CLICKED(IDC_REMOVEPROPS, &CEditPropertiesDlg::OnBnClickedRemoveProps)
    ON_BN_CLICKED(IDC_EDITPROPS, &CEditPropertiesDlg::OnBnClickedEditprops)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnLvnItemchangedEditproplist)
    ON_NOTIFY(NM_DBLCLK, IDC_EDITPROPLIST, &CEditPropertiesDlg::OnNMDblclkEditproplist)
    ON_BN_CLICKED(IDC_SAVEPROP, &CEditPropertiesDlg::OnBnClickedSaveprop)
    ON_BN_CLICKED(IDC_ADDPROPS, &CEditPropertiesDlg::OnBnClickedAddprops)
    ON_BN_CLICKED(IDC_EXPORT, &CEditPropertiesDlg::OnBnClickedExport)
    ON_BN_CLICKED(IDC_IMPORT, &CEditPropertiesDlg::OnBnClickedImport)
    ON_WM_SETCURSOR()
    ON_WM_CONTEXTMENU()
    ON_NOTIFY(HDN_ITEMCLICK, 0, &CEditPropertiesDlg::OnHdnItemclickEditproplist)
END_MESSAGE_MAP()

void CEditPropertiesDlg::OnBnClickedHelp()
{
    OnHelp();
}

BOOL CEditPropertiesDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    ExtendFrameIntoClientArea(IDC_GROUP, IDC_GROUP, IDC_GROUP, IDC_GROUP);
    m_aeroControls.SubclassControl(this, IDOK);
    m_aeroControls.SubclassControl(this, IDHELP);

    // fill in the path edit control
    if (m_pathList.GetCount() == 1)
    {
        if (m_pathList[0].IsUrl())
        {
            SetDlgItemText(IDC_PROPPATH, m_pathList[0].GetSVNPathString());
            if (!m_revision.IsValid())
                m_revision = SVNRev::REV_HEAD;
        }
        else
        {
            SetDlgItemText(IDC_PROPPATH, m_pathList[0].GetWinPathString());
            if (!m_revision.IsValid())
                m_revision = SVNRev::REV_WC;
        }
    }
    else
    {
        CString sTemp;
        sTemp.Format(IDS_EDITPROPS_NUMPATHS, m_pathList.GetCount());
        SetDlgItemText(IDC_PROPPATH, sTemp);
        if (!m_revision.IsValid())
        {
            m_revision = SVNRev::REV_WC;
            if ((m_pathList.GetCount() > 0) && (m_pathList[0].IsUrl()))
                m_revision = SVNRev::REV_HEAD;
        }
    }

    // initialize the property list control
    m_propList.DeleteAllItems();
    DWORD exStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
    m_propList.SetExtendedStyle(exStyle);
    int c = m_propList.GetHeaderCtrl()->GetItemCount() - 1;
    while (c >= 0)
        m_propList.DeleteColumn(c--);
    CString temp;
    temp.LoadString(IDS_PROPPROPERTY);
    m_propList.InsertColumn(0, temp);
    temp.LoadString(IDS_PROPVALUE);
    m_propList.InsertColumn(1, temp);
    temp.LoadString(IDS_EDITPROPS_INHERITED);
    m_propList.InsertColumn(3, temp);
    m_propList.SetRedraw(false);

    if (m_bRevProps)
    {
        GetDlgItem(IDC_IMPORT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_EXPORT)->ShowWindow(SW_HIDE);
    }

    if (m_pProjectProperties)
    {
        int     curPos   = 0;
        CString resToken = m_pProjectProperties->m_sFpPath.Tokenize(L"\n", curPos);
        while (!resToken.IsEmpty())
        {
            UserProp up(true);
            if (up.Parse(resToken))
                m_userProperties.push_back(up);
            resToken = m_pProjectProperties->m_sFpPath.Tokenize(L"\n", curPos);
        }

        curPos   = 0;
        resToken = m_pProjectProperties->m_sDpPath.Tokenize(L"\n", curPos);
        while (!resToken.IsEmpty())
        {
            UserProp up(false);
            if (up.Parse(resToken))
                m_userProperties.push_back(up);
            resToken = m_pProjectProperties->m_sDpPath.Tokenize(L"\n", curPos);
        }
    }

    m_newMenu.LoadMenu(IDR_PROPNEWMENU);
    m_btnNew.m_hMenu       = m_newMenu.GetSubMenu(0)->GetSafeHmenu();
    m_btnNew.m_bOSMenu     = TRUE;
    m_btnNew.m_bRightArrow = TRUE;

    // add the user property names to the menu
    int  menuID  = 50000;
    bool bFolder = true;
    bool bFile   = true;
    if (m_pathList.GetCount() == 1)
    {
        if (PathIsDirectory(m_pathList[0].GetWinPath()))
        {
            bFolder = true;
            bFile   = false;
        }
        else
        {
            bFolder = false;
            bFile   = true;
        }
        if (m_pathList[0].IsUrl())
        {
            if (m_bUrlIsFolder)
            {
                bFolder = true;
                bFile   = false;
            }
            else
            {
                bFolder = false;
                bFile   = true;
            }
        }
    }
    for (auto it = m_userProperties.begin(); it != m_userProperties.end(); ++it)
    {
        if ((it->propType != UserPropTypeUnknown) && (!it->propName.IsEmpty()))
        {
            if ((bFile && it->file) || (bFolder && !it->file))
            {
                if (InsertMenu(m_btnNew.m_hMenu, static_cast<UINT>(-1), MF_BYPOSITION, menuID, it->propName))
                    it->SetMenuID(menuID++);
            }
        }
    }

    m_editMenu.LoadMenu(IDR_PROPEDITMENU);
    m_btnEdit.m_hMenu         = m_editMenu.GetSubMenu(0)->GetSafeHmenu();
    m_btnEdit.m_bOSMenu       = TRUE;
    m_btnEdit.m_bRightArrow   = TRUE;
    m_btnEdit.m_bDefaultClick = TRUE;

    m_tooltips.AddTool(IDC_IMPORT, IDS_PROP_TT_IMPORT);
    m_tooltips.AddTool(IDC_EXPORT, IDS_PROP_TT_EXPORT);
    m_tooltips.AddTool(IDC_SAVEPROP, IDS_PROP_TT_SAVE);
    m_tooltips.AddTool(IDC_REMOVEPROPS, IDS_PROP_TT_REMOVE);
    m_tooltips.AddTool(IDC_EDITPROPS, IDS_PROP_TT_EDIT);
    m_tooltips.AddTool(IDC_ADDPROPS, IDS_PROP_TT_ADD);

    CString sWindowTitle;
    GetWindowText(sWindowTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_pathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

    AddAnchor(IDC_GROUP, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_PROPPATH, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_EDITPROPLIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_IMPORT, BOTTOM_RIGHT);
    AddAnchor(IDC_EXPORT, BOTTOM_RIGHT);
    AddAnchor(IDC_SAVEPROP, BOTTOM_RIGHT);
    AddAnchor(IDC_REMOVEPROPS, BOTTOM_RIGHT);
    AddAnchor(IDC_EDITPROPS, BOTTOM_RIGHT);
    AddAnchor(IDC_ADDPROPS, BOTTOM_RIGHT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    AddAnchor(IDHELP, BOTTOM_RIGHT);
    if (GetExplorerHWND())
        CenterWindow(CWnd::FromHandle(GetExplorerHWND()));
    EnableSaveRestore(L"EditPropertiesDlg");

    InterlockedExchange(&m_bThreadRunning, TRUE);
    if (AfxBeginThread(PropsThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }
    GetDlgItem(IDC_EDITPROPLIST)->SetFocus();

    return FALSE;
}

void CEditPropertiesDlg::Refresh()
{
    if (InterlockedExchange(&m_bThreadRunning, TRUE))
        return;
    if (AfxBeginThread(PropsThreadEntry, this) == nullptr)
    {
        InterlockedExchange(&m_bThreadRunning, FALSE);
        OnCantStartThread();
    }
}

UINT CEditPropertiesDlg::PropsThreadEntry(LPVOID pVoid)
{
    CCrashReportThread crashThread;
    return static_cast<CEditPropertiesDlg*>(pVoid)->PropsThread();
}

void CEditPropertiesDlg::ReadProperties(int first, int last)
{
    SVNProperties props(m_revision, m_bRevProps, false);
    props.m_bCancelled = &m_bCancelled;
    for (int i = first; i < last; ++i)
    {
        props.SetFilePath(m_pathList[i]);
        for (int p = 0; p < props.GetCount(); ++p)
        {
            std::string propStr   = props.GetItemName(p);
            std::string propValue = props.GetItemValue(p);

            async::CCriticalSectionLock lock(m_mutex);

            auto it = m_properties.find(propStr);
            if (it != m_properties.end())
            {
                it->second.count++;
                if (it->second.value.compare(propValue) != 0)
                    it->second.allTheSameValue = false;
            }
            else
            {
                it                 = m_properties.emplace_hint(it, propStr, PropValue());
                std::wstring value = CUnicodeUtils::StdGetUnicode(propValue);
                it->second.value   = propValue;
                CString sTemp      = value.c_str();
                sTemp.Replace('\n', ' ');
                sTemp.Remove('\r');
                it->second.valueWithoutNewlines = std::wstring(sTemp);
                it->second.count                = 1;
                it->second.allTheSameValue      = true;
                if (SVNProperties::IsBinary(propValue))
                    it->second.isBinary = true;
            }
        }
    }
    if (m_pathList.GetCount() == 1)
    {
        // if there's only one path, read the inherited properties as well
        SVNProperties propsInh(m_pathList[0], m_revision, m_bRevProps, true);
        auto          inheritedProps = propsInh.GetInheritedProperties();
        for (auto itup : inheritedProps)
        {
            auto propMap = std::get<1>(itup);
            for (auto inp : propMap)
            {
                std::string propStr   = inp.first;
                std::string propValue = inp.second;

                async::CCriticalSectionLock lock(m_mutex);

                auto         it    = m_properties.emplace(propStr, PropValue());
                std::wstring value = CUnicodeUtils::StdGetUnicode(propValue);
                it->second.value   = propValue;
                CString stemp      = value.c_str();
                stemp.Replace('\n', ' ');
                stemp.Remove('\r');
                it->second.valueWithoutNewlines = std::wstring(stemp);
                it->second.count                = 1;
                it->second.allTheSameValue      = true;
                it->second.isInherited          = true;
                if (SVNProperties::IsBinary(propValue))
                    it->second.isBinary = true;
                it->second.inheritedFrom = CUnicodeUtils::StdGetUnicode(std::get<0>(itup));
            }
        }
    }
}

UINT CEditPropertiesDlg::PropsThread()
{
    enum
    {
        CHUNK_SIZE = 100
    };

    m_bCancelled = false;
    // get all properties in multiple threads
    async::CJobScheduler jobs(0, async::CJobScheduler::GetHWThreadCount());
    {
        async::CCriticalSectionLock lock(m_mutex);
        m_properties.clear();
    }
    for (int i = 0; i < m_pathList.GetCount(); i += CHUNK_SIZE)
        new async::CAsyncCall(this, &CEditPropertiesDlg::ReadProperties, i, min(m_pathList.GetCount(), i + CHUNK_SIZE), &jobs);

    jobs.WaitForEmptyQueue();

    // fill the property list control with the gathered information
    async::CCriticalSectionLock lock(m_mutex);
    FillListControl();
    if (!PathIsDirectory(m_pathList[0].GetWinPath()) && !m_bUrlIsFolder)
    {
        // properties for one or more files:
        // remove the menu entries which are only useful for folders
        RemoveMenu(m_btnNew.m_hMenu, ID_NEW_EXTERNALS, MF_BYCOMMAND);
        RemoveMenu(m_btnNew.m_hMenu, ID_NEW_LOGSIZES, MF_BYCOMMAND);
        RemoveMenu(m_btnNew.m_hMenu, ID_NEW_BUGTRAQ, MF_BYCOMMAND);
        RemoveMenu(m_btnNew.m_hMenu, ID_NEW_LANGUAGES, MF_BYCOMMAND);
        RemoveMenu(m_btnNew.m_hMenu, ID_NEW_LOCALHOOKS, MF_BYCOMMAND);
    }
    PostMessage(WM_AFTERTHREAD);
    return 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CEditPropertiesDlg::OnNMCustomdrawEditproplist(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
    // Take the default processing unless we set this to something else below.
    *pResult = CDRF_DODEFAULT;

    if (m_bThreadRunning)
        return;

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

        // Tell Windows to paint the control itself.
        *pResult = CDRF_DODEFAULT;

        if (static_cast<int>(pLVCD->nmcd.dwItemSpec) > m_propList.GetItemCount())
            return;

        COLORREF   crText = CTheme::Instance().IsDarkTheme() ? CTheme::darkTextColor : GetSysColor(COLOR_WINDOWTEXT);
        PropValue* pProp  = reinterpret_cast<PropValue*>(m_propList.GetItemData(static_cast<int>(pLVCD->nmcd.dwItemSpec)));

        if (pProp->isBinary)
        {
            crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
        }
        else if (pProp->count != m_pathList.GetCount())
        {
            // if the property values are the same for all paths they're set
            // but they're not set for *all* paths, then we show the entry grayed out
            crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
        }
        else if (pProp->allTheSameValue)
        {
            if (pProp->isInherited)
                crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
        }
        else
        {
            // if the property values aren't the same for all paths
            crText = CTheme::Instance().GetThemeColor(GetSysColor(COLOR_GRAYTEXT));
        }

        // Store the color back in the NMLVCUSTOMDRAW struct.
        pLVCD->clrText = crText;
    }
}

void CEditPropertiesDlg::OnLvnItemchangedEditproplist(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    *pResult = 0;
    // disable the "remove" button if nothing
    // is selected, enable it otherwise
    int selCount = m_propList.GetSelectedCount();
    DialogEnableWindow(IDC_EXPORT, selCount >= 1);
    DialogEnableWindow(IDC_REMOVEPROPS, selCount >= 1);
    DialogEnableWindow(IDC_SAVEPROP, selCount == 1);
    DialogEnableWindow(IDC_EDITPROPS, selCount == 1);

    *pResult = 0;
}

void CEditPropertiesDlg::OnBnClickedRemoveProps()
{
    RemoveProps();
}

void CEditPropertiesDlg::OnNMDblclkEditproplist(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    if (m_propList.GetSelectedCount() == 1)
        EditProps((GetKeyState(VK_SHIFT) & 0x8000) == 0);

    *pResult = 0;
}

void CEditPropertiesDlg::OnBnClickedEditprops()
{
    switch (m_btnEdit.m_nMenuResult)
    {
        case ID_EDIT_ADVANCED:
            EditProps(false);
            break;
        default:
            EditProps(true);
            break;
    }
}

void CEditPropertiesDlg::OnBnClickedAddprops()
{
    switch (m_btnNew.m_nMenuResult)
    {
        case ID_NEW_EXECUTABLE:
            EditProps(true, SVN_PROP_EXECUTABLE, true);
            break;
        case ID_NEW_NEEDSLOCK:
            EditProps(true, SVN_PROP_NEEDS_LOCK, true);
            break;
        case ID_NEW_MIMETYPE:
            EditProps(true, SVN_PROP_MIME_TYPE, true);
            break;
        case ID_NEW_BUGTRAQ:
            EditProps(true, "bugtraq:", true);
            break;
        case ID_NEW_EOL:
            EditProps(true, SVN_PROP_EOL_STYLE, true);
            break;
        case ID_NEW_KEYWORDS:
            EditProps(true, SVN_PROP_KEYWORDS, true);
            break;
        case ID_NEW_EXTERNALS:
            EditProps(true, SVN_PROP_EXTERNALS, true);
            break;
        case ID_NEW_LOGSIZES:
            EditProps(true, "tsvn:log...", true);
            break;
        case ID_NEW_LANGUAGES:
            EditProps(true, "tsvn:projectlanguage", true);
            break;
        case ID_NEW_LOCALHOOKS:
            EditProps(true, PROJECTPROPNAME_STARTCOMMITHOOK, true);
            break;
        case ID_NEW_ADVANCED:
            EditProps(false, "", true);
            break;
        case ID_NEW_MERGELOGTEMPLATES:
            EditProps(true, PROJECTPROPNAME_MERGELOGTEMPLATEMSG, true);
            break;
        default:
            // maybe a user property?
            {
                bool bFound = false;
                for (auto it = m_userProperties.cbegin(); it != m_userProperties.cend(); ++it)
                {
                    if (it->GetMenuID() == m_btnNew.m_nMenuResult)
                    {
                        bFound = true;
                        EditProps(true, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(it->propName)), true);
                    }
                }
                if (!bFound)
                    EditProps(false, "", true);
            }
            break;
    }
}

EditPropBase* CEditPropertiesDlg::GetPropDialog(bool bDefault, const std::string& sName)
{
    if (!bDefault)
    {
        return new CEditPropertyValueDlg(this);
    }

    EditPropBase* dlg = nullptr;
    if (sName.compare(SVN_PROP_EXECUTABLE) == 0)
        dlg = new CEditPropExecutable(this);
    else if (sName.compare(SVN_PROP_NEEDS_LOCK) == 0)
        dlg = new CEditPropNeedsLock(this);
    else if (sName.compare(SVN_PROP_MIME_TYPE) == 0)
        dlg = new CEditPropMimeType(this);
    else if (sName.substr(0, 8).compare("bugtraq:") == 0)
        dlg = new CEditPropBugtraq(this);
    else if (sName.compare(SVN_PROP_EOL_STYLE) == 0)
        dlg = new CEditPropEOL(this);
    else if (sName.compare(SVN_PROP_KEYWORDS) == 0)
        dlg = new CEditPropKeywords(this);
    else if (sName.compare(SVN_PROP_EXTERNALS) == 0)
        dlg = new CEditPropExternals(this);
    else if ((sName.compare(PROJECTPROPNAME_LOGMINSIZE) == 0) ||
             (sName.compare(PROJECTPROPNAME_LOCKMSGMINSIZE) == 0) ||
             (sName.compare(PROJECTPROPNAME_LOGWIDTHLINE) == 0) ||
             (sName.compare("tsvn:log...") == 0))
        dlg = new CEditPropTSVNSizes(this);
    else if ((sName.compare(PROJECTPROPNAME_LOGFILELISTLANG) == 0) ||
             (sName.compare(PROJECTPROPNAME_PROJECTLANGUAGE) == 0) ||
             (sName.compare("tsvn:projectlanguage") == 0))
        dlg = new CEditPropTSVNLang(this);
    else if ((sName.compare(PROJECTPROPNAME_STARTCOMMITHOOK) == 0) ||
             (sName.compare(PROJECTPROPNAME_PRECOMMITHOOK) == 0) ||
             (sName.compare(PROJECTPROPNAME_CHECKCOMMITHOOK) == 0) ||
             (sName.compare(PROJECTPROPNAME_MANUALPRECOMMITHOOK) == 0) ||
             (sName.compare(PROJECTPROPNAME_POSTCOMMITHOOK) == 0) ||
             (sName.compare(PROJECTPROPNAME_STARTUPDATEHOOK) == 0) ||
             (sName.compare(PROJECTPROPNAME_PREUPDATEHOOK) == 0) ||
             (sName.compare(PROJECTPROPNAME_POSTUPDATEHOOK) == 0) ||
             (sName.compare(PROJECTPROPNAME_PRELOCKHOOK) == 0) ||
             (sName.compare(PROJECTPROPNAME_POSTLOCKHOOK) == 0))
        dlg = new CEditPropsLocalHooks(this);
    else if ((sName.substr(0, 21).compare("tsvn:mergelogtemplate") == 0))
        dlg = new CEditPropMergeLogTemplate(this);
    else
    {
        // before using the default dialog find out if this
        // is maybe a user property with one of the user property dialogs
        if (!m_userProperties.empty())
        {
            for (auto it = m_userProperties.cbegin(); it != m_userProperties.cend(); ++it)
            {
                if (sName.compare(CUnicodeUtils::GetUTF8(it->propName)) == 0)
                {
                    // user property found, but what kind?
                    switch (it->propType)
                    {
                        case UserPropTypeBool:
                        {
                            dlg = new EditPropUserBool(this, &(*it));
                        }
                        break;
                        case UserPropTypeState:
                        {
                            dlg = new EditPropUserState(this, &(*it));
                        }
                        break;
                        case UserPropTypeSingleLine:
                        {
                            dlg = new EditPropUserSingleLine(this, &(*it));
                        }
                        break;
                        case UserPropTypeMultiLine:
                        {
                            dlg = new EditPropUserMultiLine(this, &(*it));
                        }
                        break;
                        case UserPropTypeUnknown:
                            assert(false);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        if (dlg == nullptr)
            dlg = new CEditPropertyValueDlg(this);
    }

    return dlg;
}

void CEditPropertiesDlg::FillListControl()
{
    int index = 0;
    m_propList.SetRedraw(FALSE);
    m_propList.DeleteAllItems();
    for (auto it = m_properties.begin(); it != m_properties.end(); ++it)
    {
        m_propList.InsertItem(index, CUnicodeUtils::StdGetUnicode(it->first).c_str());
        m_propList.SetItemText(index, 2, it->second.inheritedFrom.c_str());
        m_propList.SetItemData(index, reinterpret_cast<LPARAM>(&it->second));

        if (it->second.isBinary)
        {
            m_propList.SetItemText(index, 1, CString(MAKEINTRESOURCE(IDS_EDITPROPS_BINVALUE)));
        }
        else if (it->second.count != m_pathList.GetCount())
        {
            // if the property values are the same for all paths they're set
            // but they're not set for *all* paths, then we show the entry grayed out
            m_propList.SetItemText(index, 1, it->second.valueWithoutNewlines.c_str());
        }
        else if (it->second.allTheSameValue)
        {
            // if the property values are the same for all paths,
            // we show the value
            m_propList.SetItemText(index, 1, it->second.valueWithoutNewlines.c_str());
        }
        else
        {
            // if the property values aren't the same for all paths
            // then we show "values are different" instead of the value
            CString sTemp(MAKEINTRESOURCE(IDS_EDITPROPS_DIFFERENTPROPVALUES));
            m_propList.SetItemText(index, 1, sTemp);
        }
        if (index == 0)
        {
            // select the first entry
            m_propList.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
            m_propList.SetSelectionMark(index);
        }
        index++;
    }

    int maxCol = m_propList.GetHeaderCtrl()->GetItemCount() - 1;
    for (int col = 0; col <= maxCol; col++)
    {
        m_propList.SetColumnWidth(col, LVSCW_AUTOSIZE);
    }
    // resize the middle column so that all columns fit into the client area
    RECT rc;
    m_propList.GetClientRect(&rc);
    const int borderSize = 20;
    m_propList.SetColumnWidth(1, rc.right - m_propList.GetColumnWidth(0) - m_propList.GetColumnWidth(2) - borderSize);

    InterlockedExchange(&m_bThreadRunning, FALSE);
    m_propList.SetRedraw(TRUE);
}

void CEditPropertiesDlg::EditProps(bool bDefault, const std::string& propName /* = "" */, bool bAdd /* = false*/)
{
    m_tooltips.Pop(); // hide the tooltips
    int selIndex = m_propList.GetSelectionMark();

    EditPropBase*               dlg   = nullptr;
    std::string                 sName = propName;
    async::CCriticalSectionLock lock(m_mutex);

    if ((!bAdd) && (selIndex >= 0) && (m_propList.GetSelectedCount()))
    {
        sName = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_propList.GetItemText(selIndex, 0)));
        dlg   = GetPropDialog(bDefault, sName);
        dlg->SetProperties(m_properties);
        PropValue* prop = reinterpret_cast<PropValue*>(m_propList.GetItemData(selIndex));

        dlg->SetPropertyName(sName);
        if (prop->allTheSameValue && !prop->isInherited)
            dlg->SetPropertyValue(prop->value);
        dlg->SetPathList(m_pathList);
        dlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_EDITPROPS_EDITTITLE)));
    }
    else
    {
        dlg = GetPropDialog(bDefault, propName);
        dlg->SetProperties(m_properties);
        if (propName.size())
            dlg->SetPropertyName(sName);
        dlg->SetPathList(m_pathList);
        auto foundIt = m_properties.find(sName);
        if (foundIt != m_properties.end())
        {
            // the property already exists: switch to "edit" instead of "add"
            PropValue* prop = nullptr;
            if (selIndex >= 0 && propName.empty())
                prop = reinterpret_cast<PropValue*>(m_propList.GetItemData(selIndex));
            else
                prop = &foundIt->second;
            dlg->SetPropertyName(sName);
            if (prop->allTheSameValue && !prop->isInherited)
                dlg->SetPropertyValue(prop->value);
        }
        dlg->SetDialogTitle(CString(MAKEINTRESOURCE(IDS_EDITPROPS_ADDTITLE)));
    }

    if (m_pathList.GetCount() > 1)
        dlg->SetMultiple();
    else if (m_pathList.GetCount() == 1)
    {
        if (m_pathList[0].IsUrl())
        {
            if (m_bUrlIsFolder)
            {
                dlg->SetFolder();
            }
        }
        else
        {
            if (PathIsDirectory(m_pathList[0].GetWinPath()))
            {
                dlg->SetFolder();
            }
        }
    }

    dlg->RevProps(m_bRevProps);
    if (dlg->DoModal() == IDOK)
    {
        if (dlg->IsChanged() || dlg->GetRecursive())
        {
            sName                = dlg->GetPropertyName();
            TProperties dlgProps = dlg->GetProperties();
            if (!sName.empty() || (dlg->HasMultipleProperties() && !dlgProps.empty()))
            {
                CString sMsg;
                bool    bDoIt = true;
                if (!m_bRevProps && (m_pathList.GetCount()) && (m_pathList[0].IsUrl()))
                {
                    CInputLogDlg input(this);
                    input.SetUUID(m_sUuid);
                    input.SetProjectProperties(m_pProjectProperties, PROJECTPROPNAME_LOGTEMPLATEPROPSET);
                    CString sHint;
                    sHint.FormatMessage(IDS_INPUT_EDITPROP, sName.c_str(), static_cast<LPCWSTR>(m_pathList[0].GetSVNPathString()));
                    input.SetActionText(sHint);
                    if (input.DoModal() == IDOK)
                    {
                        sMsg = input.GetLogMessage();
                    }
                    else
                        bDoIt = false;
                }
                if (bDoIt)
                {
                    CProgressDlg prog;
                    CString      sTemp;
                    sTemp.LoadString(IDS_SETPROPTITLE);
                    prog.SetTitle(sTemp);
                    sTemp.LoadString(IDS_PROPWAITCANCEL);
                    prog.SetCancelMsg(sTemp);
                    prog.SetTime(TRUE);
                    prog.SetShowProgressBar(TRUE);
                    prog.ShowModeless(m_hWnd);
                    for (int i = 0; i < m_pathList.GetCount(); ++i)
                    {
                        prog.SetLine(1, m_pathList[i].GetWinPath(), true);
                        SVNProperties props(m_pathList[i], m_revision, m_bRevProps, false);
                        props.SetProgressDlg(&prog);
                        if (dlg->HasMultipleProperties())
                        {
                            for (auto propsit = dlgProps.begin(); propsit != dlgProps.end(); ++propsit)
                            {
                                if (dlg->IsFolderOnlyProperty())
                                    props.AddFolderPropName(propsit->first);

                                prog.SetLine(1, CUnicodeUtils::StdGetUnicode(propsit->first).c_str());
                                BOOL ret = FALSE;
                                if (propsit->second.remove)
                                {
                                    if (dlg->GetRecursive() || props.HasProperty(propsit->first))
                                        ret = props.Remove(propsit->first, dlg->GetRecursive() ? svn_depth_infinity : svn_depth_empty, sMsg);
                                    else
                                        ret = TRUE;
                                }
                                else
                                {
                                    if (dlg->GetRecursive() || !props.HasProperty(propsit->first) || props.GetItemValue(props.IndexOf(propsit->first)).compare(propsit->second.value))
                                    {
                                        ret = props.Add(propsit->first, SVNProperties::IsBinary(propsit->second.value) ? propsit->second.value : propsit->second.value.c_str(),
                                                        false, dlg->GetRecursive() ? svn_depth_infinity : svn_depth_empty, sMsg);
                                    }
                                    else
                                        ret = TRUE;
                                }
                                if (!ret)
                                {
                                    prog.Stop();
                                    props.ShowErrorDialog(m_hWnd, props.GetPath().GetDirectory());
                                    if (props.GetSVNError()->apr_err == SVN_ERR_CANCELLED)
                                        break;
                                    prog.ShowModeless(m_hWnd);
                                }
                                else
                                {
                                    m_bChanged = true;
                                    // bump the revision number since we just did a commit
                                    if (!m_bRevProps && m_revision.IsNumber())
                                    {
                                        if (props.GetCommitRev() == SVN_INVALID_REVNUM)
                                            m_revision = static_cast<LONG>(m_revision) + 1;
                                        else
                                            m_revision = props.GetCommitRev();
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (dlg->IsFolderOnlyProperty())
                                props.AddFolderPropName(sName);
                            bool bRemove = false;
                            if ((sName.substr(0, 4).compare("svn:") == 0) ||
                                (sName.substr(0, 5).compare("tsvn:") == 0) ||
                                (sName.substr(0, 10).compare("webviewer:") == 0))
                            {
                                if (dlg->GetPropertyValue().empty())
                                    bRemove = true;
                            }
                            BOOL ret = FALSE;
                            if (bRemove)
                            {
                                if (dlg->GetRecursive() || props.HasProperty(sName))
                                    ret = props.Remove(sName, dlg->GetRecursive() ? svn_depth_infinity : svn_depth_empty, sMsg);
                                else
                                    ret = TRUE;
                            }
                            else
                            {
                                if (dlg->GetRecursive() || !props.HasProperty(sName) || props.GetItemValue(props.IndexOf(sName)).compare(dlg->IsBinary() ? dlg->GetPropertyValue() : dlg->GetPropertyValue().c_str()))
                                {
                                    ret = props.Add(sName, dlg->IsBinary() ? dlg->GetPropertyValue() : dlg->GetPropertyValue().c_str(),
                                                    false, dlg->GetRecursive() ? svn_depth_infinity : svn_depth_empty, sMsg);
                                }
                                else
                                    ret = TRUE;
                            }
                            if (!ret)
                            {
                                prog.Stop();
                                props.ShowErrorDialog(m_hWnd, props.GetPath().GetDirectory());
                                prog.ShowModeless(m_hWnd);
                            }
                            else
                            {
                                m_bChanged = true;
                                // bump the revision number since we just did a commit
                                if (!m_bRevProps && m_revision.IsNumber())
                                {
                                    if (props.GetCommitRev() == SVN_INVALID_REVNUM)
                                        m_revision = static_cast<LONG>(m_revision) + 1;
                                    else
                                    {
                                        if (props.GetCommitRev() != 0)
                                            m_revision = props.GetCommitRev();
                                    }
                                }
                            }
                        }
                    }
                    prog.Stop();
                    Refresh();
                }
            }
        }
    }

    delete dlg;
}

void CEditPropertiesDlg::RemoveProps()
{
    CString  sLogMsg;
    POSITION pos        = m_propList.GetFirstSelectedItemPosition();
    UINT     selCount   = m_propList.GetSelectedCount();
    UINT     defaultRet = 0;
    while (pos)
    {
        int selIndex = m_propList.GetNextSelectedItem(pos);

        bool         bRecurse = false;
        std::string  sName    = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(m_propList.GetItemText(selIndex, 0)));
        std::wstring sUName   = CUnicodeUtils::StdGetUnicode(sName);
        if (m_pathList[0].IsUrl())
        {
            CInputLogDlg input(this);
            input.SetUUID(m_sUuid);
            input.SetProjectProperties(m_pProjectProperties, PROJECTPROPNAME_LOGTEMPLATEPROPSET);
            CString sHint;
            sHint.FormatMessage(IDS_INPUT_REMOVEPROP, sName.c_str(), static_cast<LPCWSTR>(m_pathList[0].GetSVNPathString()));
            input.SetActionText(sHint);
            if (input.DoModal() != IDOK)
                return;
            sLogMsg    = input.GetLogMessage();
            defaultRet = IDCUSTOM2;
        }
        UINT ret = defaultRet;
        if ((ret == 0) && ((m_pathList.GetCount() > 1) || ((m_pathList.GetCount() == 1) && (PathIsDirectory(m_pathList[0].GetWinPath())))))
        {
            CString sQuestion;
            sQuestion.Format(IDS_EDITPROPS_RECURSIVEREMOVEQUESTION, sUName.c_str());

            CTaskDialog taskDlg(sQuestion,
                                CString(MAKEINTRESOURCE(IDS_EDITPROPS_RECURSIVEREMOVE_TASK2)),
                                L"TortoiseSVN",
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            taskDlg.AddCommandControl(IDCUSTOM1, CString(MAKEINTRESOURCE(IDS_EDITPROPS_RECURSIVEREMOVE_TASK3)));
            taskDlg.AddCommandControl(IDCUSTOM2, CString(MAKEINTRESOURCE(IDS_EDITPROPS_RECURSIVEREMOVE_TASK4)));
            taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskDlg.SetDefaultCommandControl(IDCUSTOM1);
            if ((m_pathList.GetCount() > 1) || (selCount > 1))
                taskDlg.SetVerificationCheckboxText(CString(MAKEINTRESOURCE(IDS_EDITPROPS_RECURSIVEREMOVE_TASK6)));
            ret = static_cast<UINT>(taskDlg.DoModal(GetSafeHwnd()));
            if ((m_pathList.GetCount() > 1) || (selCount > 1))
            {
                if (taskDlg.GetVerificationCheckboxState())
                    defaultRet = ret;
            }
        }
        else if (ret == 0)
            ret = IDCUSTOM1;

        if (ret == IDCUSTOM1)
            bRecurse = true;
        else if (ret == IDCUSTOM2)
            bRecurse = false;
        else
            break;

        CProgressDlg prog;
        CString      sTemp;
        sTemp.LoadString(IDS_SETPROPTITLE);
        prog.SetTitle(sTemp);
        sTemp.LoadString(IDS_PROPWAITCANCEL);
        prog.SetCancelMsg(sTemp);
        prog.SetTime(TRUE);
        prog.SetShowProgressBar(TRUE);
        prog.ShowModeless(m_hWnd);
        for (int i = 0; i < m_pathList.GetCount(); ++i)
        {
            prog.SetLine(1, m_pathList[i].GetWinPath(), true);
            SVNProperties props(m_pathList[i], m_revision, m_bRevProps, false);
            props.SetProgressDlg(&prog);
            if (!props.Remove(sName, bRecurse ? svn_depth_infinity : svn_depth_empty, sLogMsg))
            {
                props.ShowErrorDialog(m_hWnd, props.GetPath().GetDirectory());
            }
            else
            {
                m_bChanged = true;
                if (m_revision.IsNumber())
                    m_revision = static_cast<LONG>(m_revision) + 1;
            }
        }
        prog.Stop();
    }
    DialogEnableWindow(IDC_REMOVEPROPS, FALSE);
    DialogEnableWindow(IDC_SAVEPROP, FALSE);
    Refresh();
}

void CEditPropertiesDlg::OnOK()
{
    if (m_bThreadRunning)
        return;
    CStandAloneDialogTmpl<CResizableDialog>::OnOK();
}

void CEditPropertiesDlg::OnCancel()
{
    if (m_bThreadRunning)
    {
        m_bCancelled = true;
        return;
    }
    CStandAloneDialogTmpl<CResizableDialog>::OnCancel();
}

BOOL CEditPropertiesDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        switch (pMsg->wParam)
        {
            case VK_F5:
            {
                Refresh();
            }
            break;
            case VK_RETURN:
                if (OnEnterPressed())
                    return TRUE;
                break;
            case VK_DELETE:
            {
                if (m_bThreadRunning)
                    return __super::PreTranslateMessage(pMsg);
                PostMessage(WM_COMMAND, MAKEWPARAM(IDC_REMOVEPROPS, BN_CLICKED));
            }
            break;
            default:
                break;
        }
    }

    return __super::PreTranslateMessage(pMsg);
}

void CEditPropertiesDlg::OnBnClickedSaveprop()
{
    m_tooltips.Pop(); // hide the tooltips
    int selIndex = m_propList.GetSelectionMark();

    if ((selIndex >= 0) && (m_propList.GetSelectedCount()))
    {
        async::CCriticalSectionLock lock(m_mutex);
        PropValue*                  prop = reinterpret_cast<PropValue*>(m_propList.GetItemData(selIndex));
        if (prop->allTheSameValue)
        {
            CString savePath;
            if (!CAppUtils::FileOpenSave(savePath, nullptr, IDS_REPOBROWSE_SAVEAS, 0, false, CString(), m_hWnd))
                return;

            FILE*   stream = nullptr;
            errno_t err    = 0;
            if ((err = _tfopen_s(&stream, savePath, L"wbS")) == 0)
            {
                fwrite(prop->value.c_str(), sizeof(char), prop->value.size(), stream);
                fclose(stream);
            }
            else
            {
                wchar_t strErr[4096] = {0};
                _wcserror_s(strErr, 4096, err);
                ::MessageBox(m_hWnd, strErr, L"TortoiseSVN", MB_ICONERROR);
            }
        }
    }
}

void CEditPropertiesDlg::OnBnClickedExport()
{
    m_tooltips.Pop(); // hide the tooltips
    if (m_propList.GetSelectedCount() == 0)
        return;

    CString savePath;
    if (!CAppUtils::FileOpenSave(savePath, nullptr, IDS_REPOBROWSE_SAVEAS, IDS_PROPSFILEFILTER, false, CString(), m_hWnd))
        return;

    if (CPathUtils::GetFileExtFromPath(savePath).Compare(L".svnprops"))
    {
        // append the default ".svnprops" extension since the user did not specify it himself
        savePath += L".svnprops";
    }
    // we save the list of selected properties not in a text file but in our own
    // binary format. That's because properties can be binary themselves, a text
    // or xml file just won't do it properly.
    CString sName;
    FILE*   stream;
    errno_t err = 0;

    if ((err = _tfopen_s(&stream, savePath, L"wbS")) == 0)
    {
        POSITION pos = m_propList.GetFirstSelectedItemPosition();
        int      len = m_propList.GetSelectedCount();
        fwrite(&len, sizeof(int), 1, stream); // number of properties
        while (pos)
        {
            int index = m_propList.GetNextSelectedItem(pos);
            sName     = m_propList.GetItemText(index, 0);
            async::CCriticalSectionLock lock(m_mutex);
            PropValue*                  prop = reinterpret_cast<PropValue*>(m_propList.GetItemData(index));
            len                              = sName.GetLength() * sizeof(wchar_t);
            fwrite(&len, sizeof(int), 1, stream);                      // length of property name in bytes
            fwrite(sName, sizeof(wchar_t), sName.GetLength(), stream); // property name
            len = static_cast<int>(prop->value.size());
            fwrite(&len, sizeof(int), 1, stream);                                  // length of property value in bytes
            fwrite(prop->value.c_str(), sizeof(char), prop->value.size(), stream); // property value
        }
        fclose(stream);
    }
    else
    {
        wchar_t strErr[4096] = {0};
        _wcserror_s(strErr, 4096, err);
        ::MessageBox(m_hWnd, strErr, L"TortoiseSVN", MB_ICONERROR);
    }
}

void CEditPropertiesDlg::OnBnClickedImport()
{
    m_tooltips.Pop(); // hide the tooltips
    CString openPath;
    if (!CAppUtils::FileOpenSave(openPath, nullptr, IDS_REPOBROWSE_OPEN, IDS_PROPSFILEFILTER, true, CString(), m_hWnd))
    {
        return;
    }
    // first check the size of the file
    FILE* stream = nullptr;
    _tfopen_s(&stream, openPath, L"rbS");
    int nProperties = 0;
    if (fread(&nProperties, sizeof(int), 1, stream) == 1)
    {
        bool bFailed = false;
        if ((nProperties < 0) || (nProperties > 4096))
        {
            TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_EDITPROPS_ERRIMPORTFORMAT), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
            bFailed = true;
        }
        CProgressDlg prog;
        CString      sTemp;
        sTemp.LoadString(IDS_SETPROPTITLE);
        prog.SetTitle(sTemp);
        sTemp.LoadString(IDS_PROPWAITCANCEL);
        prog.SetCancelMsg(sTemp);
        prog.SetTime(TRUE);
        prog.SetShowProgressBar(TRUE);
        prog.ShowModeless(m_hWnd);
        while ((nProperties-- > 0) && (!bFailed))
        {
            int nNameBytes = 0;
            if (fread(&nNameBytes, sizeof(int), 1, stream) == 1)
            {
                if ((nNameBytes < 0) || (nNameBytes > 4096))
                {
                    prog.Stop();
                    TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_EDITPROPS_ERRIMPORTFORMAT), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
                    bFailed = true;
                    continue;
                }
                auto pNameBuf = std::make_unique<wchar_t[]>(nNameBytes / sizeof(wchar_t));
                if (fread(pNameBuf.get(), 1, nNameBytes, stream) == static_cast<size_t>(nNameBytes))
                {
                    std::string  sName       = CUnicodeUtils::StdGetUTF8(std::wstring(pNameBuf.get(), nNameBytes / sizeof(wchar_t)));
                    std::wstring sUName      = CUnicodeUtils::StdGetUnicode(sName);
                    int          nValueBytes = 0;
                    if (fread(&nValueBytes, sizeof(int), 1, stream) == 1)
                    {
                        auto pValueBuf = std::make_unique<BYTE[]>(nValueBytes);
                        if (fread(pValueBuf.get(), sizeof(char), nValueBytes, stream) == static_cast<size_t>(nValueBytes))
                        {
                            std::string propertyvalue;
                            propertyvalue.assign(reinterpret_cast<const char*>(pValueBuf.get()), nValueBytes);
                            CString sMsg;
                            if (m_pathList[0].IsUrl())
                            {
                                CInputLogDlg input(this);
                                input.SetUUID(m_sUuid);
                                input.SetProjectProperties(m_pProjectProperties, PROJECTPROPNAME_LOGTEMPLATEPROPSET);
                                CString sHint;
                                sHint.FormatMessage(IDS_INPUT_SETPROP, sUName.c_str(), static_cast<LPCWSTR>(m_pathList[0].GetSVNPathString()));
                                input.SetActionText(sHint);
                                if (input.DoModal() == IDOK)
                                {
                                    sMsg = input.GetLogMessage();
                                }
                                else
                                    bFailed = true;
                            }

                            for (int i = 0; i < m_pathList.GetCount() && !bFailed; ++i)
                            {
                                prog.SetLine(1, m_pathList[i].GetWinPath(), true);
                                SVNProperties props(m_pathList[i], m_revision, m_bRevProps, false);
                                if (!props.Add(sName, propertyvalue, false, svn_depth_empty, sMsg))
                                {
                                    prog.Stop();
                                    props.ShowErrorDialog(m_hWnd, props.GetPath().GetDirectory());
                                    bFailed = true;
                                }
                                else
                                {
                                    if (m_revision.IsNumber())
                                        m_revision = static_cast<LONG>(m_revision) + 1;
                                }
                            }
                        }
                        else
                        {
                            prog.Stop();
                            TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_EDITPROPS_ERRIMPORTFORMAT), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
                            bFailed = true;
                        }
                    }
                    else
                    {
                        prog.Stop();
                        TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_EDITPROPS_ERRIMPORTFORMAT), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
                        bFailed = true;
                    }
                }
                else
                {
                    prog.Stop();
                    TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_EDITPROPS_ERRIMPORTFORMAT), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
                    bFailed = true;
                }
            }
            else
            {
                prog.Stop();
                TaskDialog(GetSafeHwnd(), AfxGetResourceHandle(), MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(IDS_ERR_ERROROCCURED), MAKEINTRESOURCE(IDS_EDITPROPS_ERRIMPORTFORMAT), TDCBF_OK_BUTTON, TD_ERROR_ICON, nullptr);
                bFailed = true;
            }
        }
        prog.Stop();
    }
    if (stream)
        fclose(stream);

    Refresh();
}

BOOL CEditPropertiesDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (m_bThreadRunning)
    {
        // only show the wait cursor over the main controls
        if (!IsCursorOverWindowBorder())
        {
            HCURSOR hCur = LoadCursor(nullptr, IDC_WAIT);
            SetCursor(hCur);
            return TRUE;
        }
    }
    HCURSOR hCur = LoadCursor(nullptr, IDC_ARROW);
    SetCursor(hCur);

    return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CEditPropertiesDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if ((pWnd == &m_propList) && (m_propList.GetSelectedCount() == 1))
    {
        int selIndex = m_propList.GetSelectionMark();
        if (selIndex < 0)
            return; // nothing selected, nothing to do with a context menu
        if ((point.x == -1) && (point.y == -1))
        {
            CRect rect;
            m_propList.GetItemRect(selIndex, &rect, LVIR_LABEL);
            m_propList.ClientToScreen(&rect);
            point = rect.CenterPoint();
        }
        CIconMenu popup;
        if (popup.CreatePopupMenu())
        {
            popup.AppendMenuIcon(ID_CMD_PROP_SAVEVALUE, IDS_PROP_SAVEVALUE);
            popup.AppendMenuIcon(ID_CMD_PROP_REMOVE, IDS_PROP_REMOVE);
            popup.AppendMenuIcon(ID_CMD_PROP_EDIT, IDS_PROP_EDIT);
            int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, nullptr);
            switch (cmd)
            {
                case ID_CMD_PROP_SAVEVALUE:
                    OnBnClickedSaveprop();
                    break;
                case ID_CMD_PROP_REMOVE:
                    OnBnClickedRemoveProps();
                    break;
                case ID_CMD_PROP_EDIT:
                    OnBnClickedEditprops();
                    break;
            }
        }
    }
}

LRESULT CEditPropertiesDlg::OnAfterThread(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (m_propname.size())
        EditProps(true, CUnicodeUtils::StdGetUTF8(m_propname), true);
    return 0;
}

void CEditPropertiesDlg::OnHdnItemclickEditproplist(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
    if (m_nSortedColumn == phdr->iItem)
        m_bAscending = !m_bAscending;
    else
        m_bAscending = TRUE;
    m_nSortedColumn = phdr->iItem;

    m_propList.SortItemsEx(&CEditPropertiesDlg::SortCompare, reinterpret_cast<DWORD_PTR>(this));

    CHeaderCtrl* pHeader    = m_propList.GetHeaderCtrl();
    HDITEM       headerItem = {0};
    headerItem.mask         = HDI_FORMAT;
    const int itemCount     = pHeader->GetItemCount();
    for (int i = 0; i < itemCount; ++i)
    {
        pHeader->GetItem(i, &headerItem);
        headerItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
        pHeader->SetItem(i, &headerItem);
    }
    pHeader->GetItem(m_nSortedColumn, &headerItem);
    headerItem.fmt |= (m_bAscending ? HDF_SORTUP : HDF_SORTDOWN);
    pHeader->SetItem(m_nSortedColumn, &headerItem);

    *pResult = 0;
}

int CALLBACK CEditPropertiesDlg::SortCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CEditPropertiesDlg* pDlg = reinterpret_cast<CEditPropertiesDlg*>(lParamSort);

    auto       sName1 = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(pDlg->m_propList.GetItemText(static_cast<int>(lParam1), 0)));
    PropValue* prop1  = reinterpret_cast<PropValue*>(pDlg->m_propList.GetItemData(static_cast<int>(lParam1)));
    auto       sName2 = CUnicodeUtils::StdGetUTF8(static_cast<LPCWSTR>(pDlg->m_propList.GetItemText(static_cast<int>(lParam2), 0)));
    PropValue* prop2  = reinterpret_cast<PropValue*>(pDlg->m_propList.GetItemData(static_cast<int>(lParam2)));

    int result = 0;
    switch (pDlg->m_nSortedColumn)
    {
        case 0: // property name column
            result = sName1.compare(sName2);
            break;
        case 1: // property value column
            result = prop1->valueWithoutNewlines.compare(prop2->valueWithoutNewlines);
            break;
        case 2: // property inherited column
            result = prop1->inheritedFrom.compare(prop2->inheritedFrom);
            break;
        default:
            break;
    }

    // Sort by name if everything else is equal
    if (result == 0)
    {
        result = sName1.compare(sName2);
    }

    if (!pDlg->m_bAscending)
        result = -result;
    return result < 0;
}

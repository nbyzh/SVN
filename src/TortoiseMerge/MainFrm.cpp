﻿// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2004-2024 - TortoiseSVN

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
#include "TortoiseMerge.h"
#include "resource.h"
#include "OpenDlg.h"
#include "ProgressDlg.h"
#include "Settings.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "MainFrm.h"
#include "LeftView.h"
#include "RightView.h"
#include "BottomView.h"
#include "DiffColors.h"
#include "TaskbarUUID.h"
#include "SVNHelpers.h"
#include "SVNConfig.h"
#include "RegexFiltersDlg.h"
#include "Theme.h"
#include "StringUtils.h"
#include "Windows10Colors.h"
#include "DarkModeHelper.h"
#include "ThemeMFCVisualManager.h"

#ifdef _DEBUG
#    define new DEBUG_NEW
#endif

// CMainFrame
const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");
#define IDT_RELOADCHECKTIMER 123

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    // Global help commands
    ON_WM_HELPINFO()
    ON_COMMAND(ID_HELP_FINDER, CFrameWndEx::OnHelpFinder)
    ON_COMMAND(ID_HELP, CFrameWndEx::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CFrameWndEx::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CFrameWndEx::OnHelpFinder)
    ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
    ON_COMMAND(ID_VIEW_WHITESPACES, OnViewWhitespaces)
    ON_WM_SIZE()
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
    ON_COMMAND(ID_VIEW_ONEWAYDIFF, OnViewOnewaydiff)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ONEWAYDIFF, OnUpdateViewOnewaydiff)
    ON_UPDATE_COMMAND_UI(ID_VIEW_WHITESPACES, OnUpdateViewWhitespaces)
    ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
    ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
    ON_WM_CLOSE()
    ON_WM_ACTIVATE()
    ON_COMMAND(ID_FILE_RELOAD, OnFileReload)
    ON_COMMAND(ID_VIEW_LINEDOWN, OnViewLinedown)
    ON_COMMAND(ID_VIEW_LINEUP, OnViewLineup)
    ON_COMMAND(ID_VIEW_MOVEDBLOCKS, OnViewMovedBlocks)
    ON_UPDATE_COMMAND_UI(ID_VIEW_MOVEDBLOCKS, OnUpdateViewMovedBlocks)
    ON_UPDATE_COMMAND_UI(ID_EDIT_MARKASRESOLVED, OnUpdateMergeMarkasresolved)
    ON_COMMAND(ID_EDIT_MARKASRESOLVED, OnMergeMarkasresolved)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_NEXTCONFLICT, OnUpdateMergeNextconflict)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_PREVIOUSCONFLICT, OnUpdateMergePreviousconflict)
    ON_WM_MOVE()
    ON_WM_MOVING()
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND(ID_VIEW_SWITCHLEFT, OnViewSwitchleft)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SWITCHLEFT, OnUpdateViewSwitchleft)
    ON_COMMAND(ID_VIEW_LINELEFT, &CMainFrame::OnViewLineleft)
    ON_COMMAND(ID_VIEW_LINERIGHT, &CMainFrame::OnViewLineright)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWFILELIST, &CMainFrame::OnUpdateViewShowfilelist)
    ON_COMMAND(ID_VIEW_SHOWFILELIST, &CMainFrame::OnViewShowfilelist)
    ON_COMMAND(ID_EDIT_USETHEIRBLOCK, &CMainFrame::OnEditUseTheirs)
    ON_COMMAND(ID_EDIT_USEMYBLOCK, &CMainFrame::OnEditUseMine)
    ON_COMMAND(ID_EDIT_USETHEIRTHENMYBLOCK, &CMainFrame::OnEditUseTheirsThenMine)
    ON_COMMAND(ID_EDIT_USEMINETHENTHEIRBLOCK, &CMainFrame::OnEditUseMineThenTheirs)
    ON_COMMAND(ID_EDIT_UNDO, &CMainFrame::OnEditUndo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CMainFrame::OnUpdateEditUndo)
    ON_COMMAND(ID_EDIT_REDO, &CMainFrame::OnEditRedo)
    ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CMainFrame::OnUpdateEditRedo)
    ON_COMMAND(ID_EDIT_ENABLE, &CMainFrame::OnEditEnable)
    ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLE, &CMainFrame::OnUpdateEditEnable)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USEMINETHENTHEIRBLOCK, &CMainFrame::OnUpdateEditUseminethentheirblock)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USEMYBLOCK, &CMainFrame::OnUpdateEditUsemyblock)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USETHEIRBLOCK, &CMainFrame::OnUpdateEditUsetheirblock)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USETHEIRTHENMYBLOCK, &CMainFrame::OnUpdateEditUsetheirthenmyblock)
    ON_COMMAND(ID_VIEW_INLINEDIFFWORD, &CMainFrame::OnViewInlinediffword)
    ON_UPDATE_COMMAND_UI(ID_VIEW_INLINEDIFFWORD, &CMainFrame::OnUpdateViewInlinediffword)
    ON_COMMAND(ID_VIEW_INLINEDIFF, &CMainFrame::OnViewInlinediff)
    ON_UPDATE_COMMAND_UI(ID_VIEW_INLINEDIFF, &CMainFrame::OnUpdateViewInlinediff)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CREATEUNIFIEDDIFFFILE, &CMainFrame::OnUpdateEditCreateunifieddifffile)
    ON_COMMAND(ID_EDIT_CREATEUNIFIEDDIFFFILE, &CMainFrame::OnEditCreateunifieddifffile)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LINEDIFFBAR, &CMainFrame::OnUpdateViewLinediffbar)
    ON_COMMAND(ID_VIEW_LINEDIFFBAR, &CMainFrame::OnViewLinediffbar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_BARS, &CMainFrame::OnUpdateViewBars)
    ON_UPDATE_COMMAND_UI(ID_VIEW_LOCATORBAR, &CMainFrame::OnUpdateViewLocatorbar)
    ON_COMMAND(ID_VIEW_LOCATORBAR, &CMainFrame::OnViewLocatorbar)
    ON_COMMAND(ID_EDIT_USELEFTBLOCK, &CMainFrame::OnEditUseleftblock)
    ON_UPDATE_COMMAND_UI(ID_USEBLOCKS, &CMainFrame::OnUpdateUseBlock)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USELEFTBLOCK, &CMainFrame::OnUpdateEditUseleftblock)
    ON_COMMAND(ID_EDIT_USELEFTFILE, &CMainFrame::OnEditUseleftfile)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USELEFTFILE, &CMainFrame::OnUpdateEditUseleftfile)
    ON_COMMAND(ID_EDIT_USEBLOCKFROMLEFTBEFORERIGHT, &CMainFrame::OnEditUseblockfromleftbeforeright)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USEBLOCKFROMLEFTBEFORERIGHT, &CMainFrame::OnUpdateEditUseblockfromleftbeforeright)
    ON_COMMAND(ID_EDIT_USEBLOCKFROMRIGHTBEFORELEFT, &CMainFrame::OnEditUseblockfromrightbeforeleft)
    ON_UPDATE_COMMAND_UI(ID_EDIT_USEBLOCKFROMRIGHTBEFORELEFT, &CMainFrame::OnUpdateEditUseblockfromrightbeforeleft)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_NEXTDIFFERENCE, &CMainFrame::OnUpdateNavigateNextdifference)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_PREVIOUSDIFFERENCE, &CMainFrame::OnUpdateNavigatePreviousdifference)
    ON_COMMAND(ID_VIEW_COLLAPSED, &CMainFrame::OnViewCollapsed)
    ON_UPDATE_COMMAND_UI(ID_VIEW_COLLAPSED, &CMainFrame::OnUpdateViewCollapsed)
    ON_COMMAND(ID_VIEW_COMPAREWHITESPACES, &CMainFrame::OnViewComparewhitespaces)
    ON_UPDATE_COMMAND_UI(ID_VIEW_COMPAREWHITESPACES, &CMainFrame::OnUpdateViewComparewhitespaces)
    ON_COMMAND(ID_VIEW_IGNOREWHITESPACECHANGES, &CMainFrame::OnViewIgnorewhitespacechanges)
    ON_UPDATE_COMMAND_UI(ID_VIEW_IGNOREWHITESPACECHANGES, &CMainFrame::OnUpdateViewIgnorewhitespacechanges)
    ON_COMMAND(ID_VIEW_IGNOREALLWHITESPACECHANGES, &CMainFrame::OnViewIgnoreallwhitespacechanges)
    ON_UPDATE_COMMAND_UI(ID_VIEW_IGNOREALLWHITESPACECHANGES, &CMainFrame::OnUpdateViewIgnoreallwhitespacechanges)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_NEXTINLINEDIFF, &CMainFrame::OnUpdateNavigateNextinlinediff)
    ON_UPDATE_COMMAND_UI(ID_NAVIGATE_PREVINLINEDIFF, &CMainFrame::OnUpdateNavigatePrevinlinediff)
    ON_COMMAND(ID_VIEW_WRAPLONGLINES, &CMainFrame::OnViewWraplonglines)
    ON_UPDATE_COMMAND_UI(ID_VIEW_WRAPLONGLINES, &CMainFrame::OnUpdateViewWraplonglines)
    ON_REGISTERED_MESSAGE(TaskBarButtonCreated, CMainFrame::OnTaskbarButtonCreated)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CMainFrame::OnUpdateEditPaste)
    ON_COMMAND(ID_INDICATOR_LEFTVIEW, &CMainFrame::OnIndicatorLeftview)
    ON_COMMAND(ID_INDICATOR_RIGHTVIEW, &CMainFrame::OnIndicatorRightview)
    ON_COMMAND(ID_INDICATOR_BOTTOMVIEW, &CMainFrame::OnIndicatorBottomview)
    ON_WM_TIMER()
    ON_COMMAND(ID_VIEW_IGNORECOMMENTS, &CMainFrame::OnViewIgnorecomments)
    ON_UPDATE_COMMAND_UI(ID_VIEW_IGNORECOMMENTS, &CMainFrame::OnUpdateViewIgnorecomments)
    ON_COMMAND(ID_VIEW_IGNOREEOL, &CMainFrame::OnViewIgnoreEOL)
    ON_UPDATE_COMMAND_UI(ID_VIEW_IGNOREEOL, &CMainFrame::OnUpdateViewIgnoreEOL)
    ON_COMMAND_RANGE(ID_REGEXFILTER, ID_REGEXFILTER + 400, &CMainFrame::OnRegexfilter)
    ON_UPDATE_COMMAND_UI_RANGE(ID_REGEXFILTER, ID_REGEXFILTER + 400, &CMainFrame::OnUpdateViewRegexFilter)
    ON_COMMAND(ID_REGEX_NO_FILTER, &CMainFrame::OnRegexNoFilter)
    ON_UPDATE_COMMAND_UI(ID_REGEX_NO_FILTER, &CMainFrame::OnUpdateRegexNoFilter)
    ON_COMMAND(ID_INDICATOR_LEFTVIEWCOMBOENCODING, &CMainFrame::OnDummyEnabled)
    ON_COMMAND(ID_INDICATOR_RIGHTVIEWCOMBOENCODING, &CMainFrame::OnDummyEnabled)
    ON_COMMAND(ID_INDICATOR_BOTTOMVIEWCOMBOENCODING, &CMainFrame::OnDummyEnabled)
    ON_COMMAND(ID_INDICATOR_LEFTVIEWCOMBOEOL, &CMainFrame::OnDummyEnabled)
    ON_COMMAND(ID_INDICATOR_RIGHTVIEWCOMBOEOL, &CMainFrame::OnDummyEnabled)
    ON_COMMAND(ID_INDICATOR_BOTTOMVIEWCOMBOEOL, &CMainFrame::OnDummyEnabled)
    ON_COMMAND(ID_INDICATOR_LEFTVIEWCOMBOTABMODE, &CMainFrame::OnDummyEnabled)
    ON_COMMAND(ID_INDICATOR_RIGHTVIEWCOMBOTABMODE, &CMainFrame::OnDummyEnabled)
    ON_COMMAND(ID_INDICATOR_BOTTOMVIEWCOMBOTABMODE, &CMainFrame::OnDummyEnabled)
    ON_UPDATE_COMMAND_UI(ID_EDIT_THREEWAY_ACTIONS, &CMainFrame::OnUpdateThreeWayActions)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_COLUMN, &CMainFrame::OnUpdateColumnStatusBar)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_MARKEDWORDS, &CMainFrame::OnUpdateMarkedWords)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FINDNEXTSTART, &CMainFrame::OnUpdateEnableIfSelection)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FINDPREVSTART, &CMainFrame::OnUpdateEnableIfSelection)
    ON_COMMAND_RANGE(ID_INDICATOR_LEFTENCODINGSTART, ID_INDICATOR_LEFTENCODINGSTART + 19, &CMainFrame::OnEncodingLeft)
    ON_COMMAND_RANGE(ID_INDICATOR_RIGHTENCODINGSTART, ID_INDICATOR_RIGHTENCODINGSTART + 19, &CMainFrame::OnEncodingRight)
    ON_COMMAND_RANGE(ID_INDICATOR_BOTTOMENCODINGSTART, ID_INDICATOR_BOTTOMENCODINGSTART + 19, &CMainFrame::OnEncodingBottom)
    ON_COMMAND_RANGE(ID_INDICATOR_LEFTEOLSTART, ID_INDICATOR_LEFTEOLSTART + 19, &CMainFrame::OnEOLLeft)
    ON_COMMAND_RANGE(ID_INDICATOR_RIGHTEOLSTART, ID_INDICATOR_RIGHTEOLSTART + 19, &CMainFrame::OnEOLRight)
    ON_COMMAND_RANGE(ID_INDICATOR_BOTTOMEOLSTART, ID_INDICATOR_BOTTOMEOLSTART + 19, &CMainFrame::OnEOLBottom)
    ON_COMMAND_RANGE(ID_INDICATOR_LEFTTABMODESTART, ID_INDICATOR_LEFTTABMODESTART + 19, &CMainFrame::OnTabModeLeft)
    ON_COMMAND_RANGE(ID_INDICATOR_RIGHTTABMODESTART, ID_INDICATOR_RIGHTTABMODESTART + 19, &CMainFrame::OnTabModeRight)
    ON_COMMAND_RANGE(ID_INDICATOR_BOTTOMTABMODESTART, ID_INDICATOR_BOTTOMTABMODESTART + 19, &CMainFrame::OnTabModeBottom)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_LEFTENCODINGSTART, ID_INDICATOR_LEFTENCODINGSTART + 19, &CMainFrame::OnUpdateEncodingLeft)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_RIGHTENCODINGSTART, ID_INDICATOR_RIGHTENCODINGSTART + 19, &CMainFrame::OnUpdateEncodingRight)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_BOTTOMENCODINGSTART, ID_INDICATOR_BOTTOMENCODINGSTART + 19, &CMainFrame::OnUpdateEncodingBottom)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_LEFTEOLSTART, ID_INDICATOR_LEFTEOLSTART + 19, &CMainFrame::OnUpdateEOLLeft)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_RIGHTEOLSTART, ID_INDICATOR_RIGHTEOLSTART + 19, &CMainFrame::OnUpdateEOLRight)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_BOTTOMEOLSTART, ID_INDICATOR_BOTTOMEOLSTART + 19, &CMainFrame::OnUpdateEOLBottom)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_LEFTTABMODESTART, ID_INDICATOR_LEFTTABMODESTART + 19, &CMainFrame::OnUpdateTabModeLeft)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_RIGHTTABMODESTART, ID_INDICATOR_RIGHTTABMODESTART + 19, &CMainFrame::OnUpdateTabModeRight)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_BOTTOMTABMODESTART, ID_INDICATOR_BOTTOMTABMODESTART + 19, &CMainFrame::OnUpdateTabModeBottom)
    ON_WM_SETTINGCHANGE()
    ON_WM_SYSCOLORCHANGE()
    ON_MESSAGE(WM_DPICHANGED, OnDPIChanged)
END_MESSAGE_MAP()

static UINT indicators[] =
    {
        ID_SEPARATOR, // status line indicator
        ID_INDICATOR_LEFTVIEW,
        ID_INDICATOR_RIGHTVIEW,
        ID_INDICATOR_BOTTOMVIEW,
        ID_INDICATOR_CAPS,
        ID_INDICATOR_NUM,
        ID_INDICATOR_SCRL};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
    : m_bInitSplitter(FALSE)
    , m_bCheckReload(false)
    , m_bHasConflicts(false)
    , m_bInlineWordDiff(true)
    , m_regWrapLines(L"Software\\TortoiseMerge\\WrapLines", 0)
    , m_regViewModedBlocks(L"Software\\TortoiseMerge\\ViewMovedBlocks", TRUE)
    , m_regOneWay(L"Software\\TortoiseMerge\\OnePane")
    , m_regCollapsed(L"Software\\TortoiseMerge\\Collapsed", 0)
    , m_regInlineDiff(L"Software\\TortoiseMerge\\DisplayBinDiff", TRUE)
    , m_regUseRibbons(L"Software\\TortoiseMerge\\UseRibbons", TRUE)
    , m_regUseTaskDialog(L"Software\\TortoiseMerge\\UseTaskDialog", TRUE)
    , m_regIgnoreComments(L"Software\\TortoiseMerge\\IgnoreComments", FALSE)
    , m_regIgnoreEOL(L"Software\\TortoiseMerge\\IgnoreEOL", TRUE)
    , m_regLineDiff(L"Software\\TortoiseMerge\\LineDiff", TRUE)
    , m_regLocatorBar(L"Software\\TortoiseMerge\\LocatorBar", TRUE)
    , m_regStatusBar(L"Software\\TortoiseMerge\\StatusBar", TRUE)
    , m_regexIndex(-1)
    , m_pwndLeftView(nullptr)
    , m_pwndRightView(nullptr)
    , m_pwndBottomView(nullptr)
    , m_bReversedPatch(FALSE)
    , m_bReadOnly(false)
    , m_bMarkedAsResolvedWasDone(false)
    , m_bBlame(false)
    , m_nMoveMovesToIgnore(0)
    , m_bSaveRequired(false)
    , m_bSaveRequiredOnConflicts(false)
    , m_bAskToMarkAsResolved(true)
    , resolveMsgWnd(nullptr)
    , resolveMsgWParam(0)
    , resolveMsgLParam(0)
    , m_themeCallbackId(0)
{
    m_bOneWay          = (0 != static_cast<DWORD>(m_regOneWay));
    m_bCollapsed       = !!static_cast<DWORD>(m_regCollapsed);
    m_bViewMovedBlocks = !!static_cast<DWORD>(m_regViewModedBlocks);
    m_bWrapLines       = !!static_cast<DWORD>(m_regWrapLines);
    m_bInlineDiff      = !!m_regInlineDiff;
    m_bUseRibbons      = !!m_regUseRibbons;
    m_bLineDiff        = m_regLineDiff;
    m_bLocatorBar      = m_regLocatorBar;
}

CMainFrame::~CMainFrame()
{
}

// ReSharper disable once CppMemberFunctionMayBeConst
LRESULT CMainFrame::OnTaskbarButtonCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    setUuidOverlayIcon(m_hWnd);
    return 0;
}

int CMainFrame::InitRibbon()
{
    if (!m_bUseRibbons)
        return 0;

    if (CTheme::Instance().IsDarkTheme())
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CThemeMFCVisualManager));
    else
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    if (HRESULT hr = m_pRibbonFramework.CoCreateInstance(__uuidof(UIRibbonFramework)); FAILED(hr))
    {
        TRACE(L"Failed to create ribbon framework (%08x)\n", hr);
        return -1; // fail to create
    }

    m_pRibbonApp.reset(new CNativeRibbonApp(this, m_pRibbonFramework));
    m_pRibbonApp->SetSettingsFileName(CPathUtils::GetAppDataDirectory() + L"TortoiseMerge-RibbonSettings");

    if (HRESULT hr = m_pRibbonFramework->Initialize(m_hWnd, m_pRibbonApp.get()); FAILED(hr))
    {
        TRACE(L"Failed to initialize ribbon framework (%08x)\n", hr);
        return -1; // fail to create
    }

    if (HRESULT hr = m_pRibbonFramework->LoadUI(AfxGetResourceHandle(), L"TORTOISEMERGERIBBON_RIBBON"); FAILED(hr))
    {
        TRACE(L"Failed to load ribbon UI (%08x)\n", hr);
        return -1; // fail to create
    }

    m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
        [this]() {
            if (CTheme::Instance().IsDarkTheme())
                CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CThemeMFCVisualManager));
            else
                CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
            SetTheme(CTheme::Instance().IsDarkTheme());
        });
    SetTheme(CTheme::Instance().IsDarkTheme());

    BuildRegexSubitems();
    if (!m_wndRibbonStatusBar.Create(this))
    {
        TRACE0("Failed to create ribbon status bar\n");
        return -1; // fail to create
    }

    // column info
    CString sColumn;
    sColumn.Format(IDS_INDICATOR_COLUMN, 0);
    auto columnPane = new CMFCRibbonStatusBarPane(ID_INDICATOR_COLUMN, sColumn, FALSE);
    m_wndRibbonStatusBar.AddElement(columnPane, L"");
    sColumn.Format(IDS_INDICATOR_COLUMN, 999999);
    columnPane->SetAlmostLargeText(sColumn);
    // marked word counter
    auto columnPaneMw = new CMFCRibbonStatusBarPane(ID_INDICATOR_MARKEDWORDS, L"", FALSE);
    m_wndRibbonStatusBar.AddElement(columnPaneMw, L"");
    columnPaneMw->SetAlmostLargeText(L"Marked words: l: XXXX | r: XXXX | b: XXXX");

    CString sTooltip(MAKEINTRESOURCE(IDS_ENCODING_COMBO_TOOLTIP));
    auto    apBtnGroupLeft = std::make_unique<CMFCRibbonButtonsGroup>();
    apBtnGroupLeft->SetID(ID_INDICATOR_LEFTVIEW);
    apBtnGroupLeft->AddButton(new CMFCRibbonStatusBarPane(ID_SEPARATOR, CString(MAKEINTRESOURCE(IDS_STATUSBAR_LEFTVIEW)), TRUE));
    CMFCRibbonButton *pButton = new CMFCRibbonButton(ID_INDICATOR_LEFTVIEWCOMBOENCODING, L"");
    pButton->SetToolTipText(sTooltip);
    FillEncodingButton(pButton, ID_INDICATOR_LEFTENCODINGSTART);
    apBtnGroupLeft->AddButton(pButton);
    pButton = new CMFCRibbonButton(ID_INDICATOR_LEFTVIEWCOMBOEOL, L"");
    FillEOLButton(pButton, ID_INDICATOR_LEFTEOLSTART);
    apBtnGroupLeft->AddButton(pButton);
    pButton = new CMFCRibbonButton(ID_INDICATOR_LEFTVIEWCOMBOTABMODE, L"");
    FillTabModeButton(pButton, ID_INDICATOR_LEFTTABMODESTART);
    apBtnGroupLeft->AddButton(pButton);
    apBtnGroupLeft->AddButton(new CMFCRibbonStatusBarPane(ID_INDICATOR_LEFTVIEW, L"", TRUE));
    m_wndRibbonStatusBar.AddExtendedElement(apBtnGroupLeft.release(), L"");

    auto apBtnGroupRight = std::make_unique<CMFCRibbonButtonsGroup>();
    apBtnGroupRight->SetID(ID_INDICATOR_RIGHTVIEW);
    apBtnGroupRight->AddButton(new CMFCRibbonStatusBarPane(ID_SEPARATOR, CString(MAKEINTRESOURCE(IDS_STATUSBAR_RIGHTVIEW)), TRUE));
    pButton = new CMFCRibbonButton(ID_INDICATOR_RIGHTVIEWCOMBOENCODING, L"");
    pButton->SetToolTipText(sTooltip);
    FillEncodingButton(pButton, ID_INDICATOR_RIGHTENCODINGSTART);
    apBtnGroupRight->AddButton(pButton);
    pButton = new CMFCRibbonButton(ID_INDICATOR_RIGHTVIEWCOMBOEOL, L"");
    FillEOLButton(pButton, ID_INDICATOR_RIGHTEOLSTART);
    apBtnGroupRight->AddButton(pButton);
    pButton = new CMFCRibbonButton(ID_INDICATOR_RIGHTVIEWCOMBOTABMODE, L"");
    FillTabModeButton(pButton, ID_INDICATOR_RIGHTTABMODESTART);
    apBtnGroupRight->AddButton(pButton);
    apBtnGroupRight->AddButton(new CMFCRibbonStatusBarPane(ID_INDICATOR_RIGHTVIEW, L"", TRUE));
    m_wndRibbonStatusBar.AddExtendedElement(apBtnGroupRight.release(), L"");

    ShowPane(&m_wndRibbonStatusBar, m_regStatusBar, FALSE, FALSE);

    return 0;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_bUseRibbons)
    {
        if (!m_wndMenuBar.Create(this))
        {
            TRACE0("Failed to create menubar\n");
            return -1; // fail to create
        }
        m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

        // prevent the menu bar from taking the focus on activation
        CMFCPopupMenu::SetForceMenuFocus(FALSE);
        if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY) || !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
        {
            TRACE0("Failed to create toolbar\n");
            return -1; // fail to create
        }
        m_wndToolBar.SetWindowText(L"Main");
        if (!m_wndStatusBar.Create(this) ||
            !m_wndStatusBar.SetIndicators(indicators,
                                          _countof(indicators)))
        {
            TRACE0("Failed to create status bar\n");
            return -1; // fail to create
        }
        m_wndStatusBar.EnablePaneDoubleClick();

        m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback(
            [this]() {
                if (CTheme::Instance().IsDarkTheme())
                    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CThemeMFCVisualManager));
                else
                    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
                SetTheme(CTheme::Instance().IsDarkTheme());
            });
        SetTheme(CTheme::Instance().IsDarkTheme());
        ShowPane(&m_wndStatusBar, m_regStatusBar, FALSE, FALSE);
        if (CTheme::Instance().IsDarkTheme())
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CThemeMFCVisualManager));
        else
            CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
    }

    if (!m_wndLocatorBar.Create(this, IDD_DIFFLOCATOR,
                                CBRS_ALIGN_LEFT | CBRS_SIZE_FIXED, ID_VIEW_LOCATORBAR))
    {
        TRACE0("Failed to create dialogbar\n");
        return -1; // fail to create
    }
    if (!m_wndLineDiffBar.Create(this, IDD_LINEDIFF,
                                 CBRS_ALIGN_BOTTOM | CBRS_SIZE_FIXED, ID_VIEW_LINEDIFFBAR))
    {
        TRACE0("Failed to create dialogbar\n");
        return -1; // fail to create
    }
    m_wndLocatorBar.m_pMainFrm  = this;
    m_wndLineDiffBar.m_pMainFrm = this;

    EnableDocking(CBRS_ALIGN_ANY);
    if (!m_bUseRibbons)
    {
        m_wndMenuBar.EnableDocking(CBRS_ALIGN_TOP);
        m_wndToolBar.EnableDocking(CBRS_ALIGN_TOP);
        DockPane(&m_wndMenuBar);
        DockPane(&m_wndToolBar);
    }
    DockPane(&m_wndLocatorBar);
    DockPane(&m_wndLineDiffBar);
    ShowPane(&m_wndLocatorBar, m_bLocatorBar, false, true);
    ShowPane(&m_wndLineDiffBar, m_bLineDiff, false, true);

    m_wndLocatorBar.EnableGripper(FALSE);
    m_wndLineDiffBar.EnableGripper(FALSE);

    return 0;
}

void CMainFrame::OnDestroy()
{
    CTheme::Instance().RemoveRegisteredCallback(m_themeCallbackId);
    if (m_pRibbonFramework)
    {
        m_pRibbonFramework->Destroy();
    }

    CFrameWndEx::OnDestroy();
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT &cs)
{
    // extra styles needed to avoid refresh problems with the ribbon
    cs.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    if (!CFrameWndEx::PreCreateWindow(cs))
        return FALSE;
    if (CTheme::Instance().IsDarkTheme())
    {
        WNDCLASSEX wndClass{};
        wndClass.cbSize = sizeof(WNDCLASSEX);
        GetClassInfoEx(AfxGetInstanceHandle(), cs.lpszClass, &wndClass);
        UnregisterClass(cs.lpszClass, AfxGetInstanceHandle());
        wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOWTEXT + 1);
        RegisterClassEx(&wndClass);
    }
    return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext &dc) const
{
    CFrameWndEx::Dump(dc);
}

#endif //_DEBUG

// CMainFrame message handlers

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext *pContext)
{
    CRect cr;
    GetClientRect(&cr);

    // split into three panes:
    //        -------------
    //        |     |     |
    //        |     |     |
    //        |------------
    //        |           |
    //        |           |
    //        |------------

    // create a splitter with 2 rows, 1 column
    if (!m_wndSplitter.CreateStatic(this, 2, 1))
    {
        TRACE0("Failed to CreateStaticSplitter\n");
        return FALSE;
    }

    // add the second splitter pane - which is a nested splitter with 2 columns
    if (!m_wndSplitter2.CreateStatic(
            &m_wndSplitter,                    // our parent window is the first splitter
            1, 2,                              // the new splitter is 1 row, 2 columns
            WS_CHILD | WS_VISIBLE | WS_BORDER, // style, WS_BORDER is needed
            m_wndSplitter.IdFromRowCol(0, 0)
            // new splitter is in the first row, 1st column of first splitter
            ))
    {
        TRACE0("Failed to create nested splitter\n");
        return FALSE;
    }
    // add the first splitter pane - the default view in row 0
    if (!m_wndSplitter.CreateView(1, 0,
                                  RUNTIME_CLASS(CBottomView), CSize(cr.Width(), cr.Height()), pContext))
    {
        TRACE0("Failed to create first pane\n");
        return FALSE;
    }
    m_pwndBottomView                    = static_cast<CBottomView *>(m_wndSplitter.GetPane(1, 0));
    m_pwndBottomView->m_pwndLocator     = &m_wndLocatorBar;
    m_pwndBottomView->m_pwndLineDiffBar = &m_wndLineDiffBar;
    if (m_bUseRibbons)
        m_pwndBottomView->m_pwndRibbonStatusBar = &m_wndRibbonStatusBar;
    else
        m_pwndBottomView->m_pwndStatusBar = &m_wndStatusBar;
    m_pwndBottomView->m_pMainFrame = this;

    // now create the two views inside the nested splitter

    if (!m_wndSplitter2.CreateView(0, 0,
                                   RUNTIME_CLASS(CLeftView), CSize(cr.Width() / 2, cr.Height() / 2), pContext))
    {
        TRACE0("Failed to create second pane\n");
        return FALSE;
    }
    m_pwndLeftView                    = static_cast<CLeftView *>(m_wndSplitter2.GetPane(0, 0));
    m_pwndLeftView->m_pwndLocator     = &m_wndLocatorBar;
    m_pwndLeftView->m_pwndLineDiffBar = &m_wndLineDiffBar;
    if (m_bUseRibbons)
        m_pwndLeftView->m_pwndRibbonStatusBar = &m_wndRibbonStatusBar;
    else
        m_pwndLeftView->m_pwndStatusBar = &m_wndStatusBar;
    m_pwndLeftView->m_pMainFrame = this;

    if (!m_wndSplitter2.CreateView(0, 1,
                                   RUNTIME_CLASS(CRightView), CSize(cr.Width() / 2, cr.Height() / 2), pContext))
    {
        TRACE0("Failed to create third pane\n");
        return FALSE;
    }
    m_pwndRightView                    = static_cast<CRightView *>(m_wndSplitter2.GetPane(0, 1));
    m_pwndRightView->m_pwndLocator     = &m_wndLocatorBar;
    m_pwndRightView->m_pwndLineDiffBar = &m_wndLineDiffBar;
    if (m_bUseRibbons)
        m_pwndRightView->m_pwndRibbonStatusBar = &m_wndRibbonStatusBar;
    else
        m_pwndRightView->m_pwndStatusBar = &m_wndStatusBar;
    m_pwndRightView->m_pMainFrame = this;
    m_bInitSplitter               = TRUE;

    m_dlgFilePatches.Create(IDD_FILEPATCHES, this);
    UpdateLayout();
    return TRUE;
}

// Callback function
BOOL CMainFrame::PatchFile(CString sFilePath, bool /*bContentMods*/, bool bPropMods, CString sVersion, BOOL bAutoPatch)
{
    //"dry run" was successful, so save the patched file somewhere...
    CString sTempFile = CTempFiles::Instance().GetTempFilePathString();
    CString sRejectedFile;
    if (m_patch.GetPatchResult(sFilePath, sTempFile, sRejectedFile) < 0)
    {
        MessageBox(m_patch.GetErrorMessage(), nullptr, MB_ICONERROR);
        return FALSE;
    }
    sFilePath = m_patch.GetTargetPath() + L"\\" + sFilePath;
    sFilePath.Replace('/', '\\');
    if (m_bReversedPatch)
    {
        m_data.m_baseFile.SetFileName(sTempFile);
        CString temp;
        temp.Format(L"%s %s", static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(sFilePath)), static_cast<LPCWSTR>(m_data.m_sPatchPatched));
        m_data.m_baseFile.SetDescriptiveName(temp);
        m_data.m_yourFile.SetFileName(sFilePath);
        temp.Format(L"%s %s", static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(sFilePath)), static_cast<LPCWSTR>(m_data.m_sPatchOriginal));
        m_data.m_yourFile.SetDescriptiveName(temp);
        m_data.m_theirFile.SetOutOfUse();
        m_data.m_mergedFile.SetOutOfUse();
    }
    else
    {
        if ((!PathFileExists(sFilePath)) || (PathIsDirectory(sFilePath)))
        {
            m_data.m_baseFile.SetFileName(CTempFiles::Instance().GetTempFilePathString());
            m_data.m_baseFile.CreateEmptyFile();
        }
        else
        {
            m_data.m_baseFile.SetFileName(sFilePath);
        }
        CString sDescription;
        sDescription.Format(L"%s %s", static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(sFilePath)), static_cast<LPCWSTR>(m_data.m_sPatchOriginal));
        m_data.m_baseFile.SetDescriptiveName(sDescription);
        m_data.m_yourFile.SetFileName(sTempFile);
        CString temp;
        temp.Format(L"%s %s", static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(sFilePath)), static_cast<LPCWSTR>(m_data.m_sPatchPatched));
        m_data.m_yourFile.SetDescriptiveName(temp);
        m_data.m_theirFile.SetOutOfUse();
        m_data.m_mergedFile.SetFileName(sFilePath);
        m_data.m_bPatchRequired = bPropMods;
    }
    TRACE(L"comparing %s\nwith the patched result %s\n", static_cast<LPCWSTR>(sFilePath), static_cast<LPCWSTR>(sTempFile));

    LoadViews();
    if (!sRejectedFile.IsEmpty())
    {
        // start TortoiseUDiff with the rejected hunks
        CString sTitle;
        sTitle.Format(IDS_TITLE_REJECTEDHUNKS, static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(sFilePath)));
        CAppUtils::StartUnifiedDiffViewer(sRejectedFile, sTitle);
    }
    if (bAutoPatch)
    {
        PatchSave();
    }
    return TRUE;
}

// Callback function
BOOL CMainFrame::DiffFiles(CString sURL1, CString sRev1, CString sURL2, CString sRev2)
{
    CString tempfile1 = CTempFiles::Instance().GetTempFilePathString();
    CString tempfile2 = CTempFiles::Instance().GetTempFilePathString();

    ASSERT(tempfile1.Compare(tempfile2));

    CString      sTemp;
    CProgressDlg progDlg;
    sTemp.Format(IDS_GETVERSIONOFFILE, static_cast<LPCWSTR>(sRev1));
    progDlg.SetLine(1, sTemp, true);
    progDlg.SetLine(2, sURL1, true);
    sTemp.LoadString(IDS_GETVERSIONOFFILETITLE);
    progDlg.SetTitle(sTemp);
    progDlg.SetShowProgressBar(true);
    progDlg.SetTime(FALSE);
    progDlg.SetProgress(1, 100);
    progDlg.ShowModeless(this);
    if (!CAppUtils::GetVersionedFile(sURL1, sRev1, tempfile1, &progDlg, m_hWnd))
    {
        progDlg.Stop();
        CString sErrMsg;
        sErrMsg.FormatMessage(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, static_cast<LPCWSTR>(sRev1), static_cast<LPCWSTR>(sURL1));
        MessageBox(sErrMsg, nullptr, MB_ICONERROR);
        return FALSE;
    }
    sTemp.Format(IDS_GETVERSIONOFFILE, static_cast<LPCWSTR>(sRev2));
    progDlg.SetLine(1, sTemp, true);
    progDlg.SetLine(2, sURL2, true);
    progDlg.SetProgress(50, 100);
    if (!CAppUtils::GetVersionedFile(sURL2, sRev2, tempfile2, &progDlg, m_hWnd))
    {
        progDlg.Stop();
        CString sErrMsg;
        sErrMsg.FormatMessage(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, static_cast<LPCWSTR>(sRev2), static_cast<LPCWSTR>(sURL2));
        MessageBox(sErrMsg, nullptr, MB_ICONERROR);
        return FALSE;
    }
    progDlg.SetProgress(100, 100);
    progDlg.Stop();
    CString temp;
    temp.Format(L"%s Revision %s", static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(sURL1)), static_cast<LPCWSTR>(sRev1));
    m_data.m_baseFile.SetFileName(tempfile1);
    m_data.m_baseFile.SetDescriptiveName(temp);
    temp.Format(L"%s Revision %s", static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(sURL2)), static_cast<LPCWSTR>(sRev2));
    m_data.m_yourFile.SetFileName(tempfile2);
    m_data.m_yourFile.SetDescriptiveName(temp);

    LoadViews();

    return TRUE;
}

void CMainFrame::OnFileOpen()
{
    if (CheckForSave(CHFSR_OPEN) == IDCANCEL)
        return;
    return OnFileOpen(false);
}

void CMainFrame::OnFileOpen(bool fillyours)
{
    COpenDlg dlg;
    if (fillyours)
        dlg.m_sBaseFile = m_data.m_yourFile.GetFilename();
    if (dlg.DoModal() != IDOK)
    {
        return;
    }
    m_bMarkedAsResolvedWasDone = false;
    m_dlgFilePatches.ShowWindow(SW_HIDE);
    m_dlgFilePatches.Init(nullptr, nullptr, CString(), nullptr);
    TRACE(L"got the files:\n   %s\n   %s\n   %s\n   %s\n   %s\n", static_cast<LPCWSTR>(dlg.m_sBaseFile), static_cast<LPCWSTR>(dlg.m_sTheirFile), static_cast<LPCWSTR>(dlg.m_sYourFile),
          static_cast<LPCWSTR>(dlg.m_sUnifiedDiffFile), static_cast<LPCWSTR>(dlg.m_sPatchDirectory));
    m_data.m_baseFile.SetFileName(dlg.m_sBaseFile);
    m_data.m_theirFile.SetFileName(dlg.m_sTheirFile);
    m_data.m_yourFile.SetFileName(dlg.m_sYourFile);
    m_data.m_sDiffFile  = dlg.m_sUnifiedDiffFile;
    m_data.m_sPatchPath = dlg.m_sPatchDirectory;
    m_data.m_mergedFile.SetOutOfUse();
    CCrashReport::Instance().AddFile2(dlg.m_sBaseFile, nullptr, L"Basefile", CR_AF_MAKE_FILE_COPY);
    CCrashReport::Instance().AddFile2(dlg.m_sTheirFile, nullptr, L"Theirfile", CR_AF_MAKE_FILE_COPY);
    CCrashReport::Instance().AddFile2(dlg.m_sYourFile, nullptr, L"Yourfile", CR_AF_MAKE_FILE_COPY);
    CCrashReport::Instance().AddFile2(dlg.m_sUnifiedDiffFile, nullptr, L"Difffile", CR_AF_MAKE_FILE_COPY);

    if (!m_data.IsBaseFileInUse() && m_data.IsTheirFileInUse() && m_data.IsYourFileInUse())
    {
        // a diff between two files means "Yours" against "Base", not "Theirs" against "Yours"
        m_data.m_baseFile.TransferDetailsFrom(m_data.m_theirFile);
    }
    if (m_data.IsBaseFileInUse() && m_data.IsTheirFileInUse() && !m_data.IsYourFileInUse())
    {
        // a diff between two files means "Yours" against "Base", not "Theirs" against "Base"
        m_data.m_yourFile.TransferDetailsFrom(m_data.m_theirFile);
    }
    m_bSaveRequired = false;

    LoadViews();
}

void CMainFrame::ClearViewNamesAndPaths() const
{
    m_pwndLeftView->m_sWindowName.Empty();
    m_pwndLeftView->m_sFullFilePath.Empty();
    m_pwndLeftView->m_sReflectedName.Empty();
    m_pwndRightView->m_sWindowName.Empty();
    m_pwndRightView->m_sFullFilePath.Empty();
    m_pwndRightView->m_sReflectedName.Empty();
    m_pwndBottomView->m_sWindowName.Empty();
    m_pwndBottomView->m_sFullFilePath.Empty();
    m_pwndBottomView->m_sReflectedName.Empty();
}

bool CMainFrame::LoadViews(int line)
{
    LoadIgnoreCommentData();
    m_data.SetBlame(m_bBlame);
    m_data.SetMovedBlocks(m_bViewMovedBlocks);
    m_bHasConflicts           = false;
    CBaseView *pwndActiveView = m_pwndLeftView;
    int        nOldLine       = m_pwndRightView ? m_pwndRightView->m_nTopLine : -1;
    int        nOldLineNumber =
        m_pwndRightView && m_pwndRightView->m_pViewData ? m_pwndRightView->m_pViewData->GetLineNumber(m_pwndRightView->m_nTopLine) : -1;
    POINT ptOldCaretPos = {-1, -1};
    if (m_pwndRightView && m_pwndRightView->IsTarget())
        ptOldCaretPos = m_pwndRightView->GetCaretPosition();
    if (m_pwndBottomView && m_pwndBottomView->IsTarget())
        ptOldCaretPos = m_pwndBottomView->GetCaretPosition();
    CString sExt = CPathUtils::GetFileExtFromPath(m_data.m_baseFile.GetFilename()).MakeLower();
    sExt.TrimLeft(L".");
    auto sC = m_ignoreCommentsMap.find(sExt);
    if (sC == m_ignoreCommentsMap.end())
    {
        sExt = CPathUtils::GetFileExtFromPath(m_data.m_yourFile.GetFilename()).MakeLower();
        sC   = m_ignoreCommentsMap.find(sExt);
        if (sC == m_ignoreCommentsMap.end())
        {
            sExt = CPathUtils::GetFileExtFromPath(m_data.m_theirFile.GetFilename()).MakeLower();
            sC   = m_ignoreCommentsMap.find(sExt);
        }
    }
    if (sC != m_ignoreCommentsMap.end())
        m_data.SetCommentTokens(std::get<0>(sC->second), std::get<1>(sC->second), std::get<2>(sC->second));
    else
        m_data.SetCommentTokens(L"", L"", L"");
    if (!m_data.Load())
    {
        m_pwndLeftView->BuildAllScreen2ViewVector();
        m_pwndLeftView->DocumentUpdated();
        m_pwndRightView->DocumentUpdated();
        m_pwndBottomView->DocumentUpdated();
        m_wndLocatorBar.DocumentUpdated();
        m_wndLineDiffBar.DocumentUpdated();
        ::MessageBox(m_hWnd, m_data.GetError(), L"TortoiseMerge", MB_ICONERROR);
        m_data.m_mergedFile.SetOutOfUse();
        m_bSaveRequired = false;
        return false;
    }
    SetWindowTitle();
    m_pwndLeftView->BuildAllScreen2ViewVector();
    m_pwndLeftView->DocumentUpdated();
    m_pwndRightView->DocumentUpdated();
    m_pwndBottomView->DocumentUpdated();
    m_wndLocatorBar.DocumentUpdated();
    m_wndLineDiffBar.DocumentUpdated();

    m_pwndLeftView->SetWritable(false);
    m_pwndLeftView->SetWritableIsChangable(false);
    m_pwndLeftView->SetTarget(false);
    m_pwndRightView->SetWritable(false);
    m_pwndRightView->SetWritableIsChangable(false);
    m_pwndRightView->SetTarget(false);
    m_pwndBottomView->SetWritable(false);
    m_pwndBottomView->SetWritableIsChangable(false);
    m_pwndBottomView->SetTarget(false);

    if (!m_data.IsBaseFileInUse())
    {
        CProgressDlg progDlg;
        if (m_data.IsYourFileInUse() && m_data.IsTheirFileInUse())
        {
            m_data.m_baseFile.TransferDetailsFrom(m_data.m_theirFile);
        }
        else if ((!m_data.m_sDiffFile.IsEmpty()) && (m_patch.Init(m_data.m_sDiffFile, m_data.m_sPatchPath, &progDlg) < 0))
        {
            progDlg.Stop();
            ClearViewNamesAndPaths();
            MessageBox(m_patch.GetErrorMessage(), nullptr, MB_ICONERROR);
            m_bSaveRequired = false;
            return false;
        }
        if (m_patch.GetNumberOfFiles() > 0)
        {
            CString betterpatchpath = m_patch.CheckPatchPath(m_data.m_sPatchPath);
            if (betterpatchpath.CompareNoCase(m_data.m_sPatchPath) != 0)
            {
                progDlg.Stop();
                CString msg;
                msg.FormatMessage(IDS_WARNBETTERPATCHPATHFOUND, static_cast<LPCWSTR>(m_data.m_sPatchPath), static_cast<LPCWSTR>(betterpatchpath));
                CTaskDialog taskdlg(msg,
                                    CString(MAKEINTRESOURCE(IDS_WARNBETTERPATCHPATHFOUND_TASK2)),
                                    L"TortoiseMerge",
                                    0,
                                    TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                CString     task3;
                WCHAR       t3[MAX_PATH] = {0};
                CString     cp           = betterpatchpath.Left(MAX_PATH - 1);
                PathCompactPathEx(t3, cp, 50, 0);
                task3.Format(IDS_WARNBETTERPATCHPATHFOUND_TASK3, t3);
                taskdlg.AddCommandControl(100, task3);
                CString task4;
                WCHAR   t4[MAX_PATH] = {0};
                cp                   = m_data.m_sPatchPath.Left(MAX_PATH - 1);
                PathCompactPathEx(t4, cp, 50, 0);
                task4.Format(IDS_WARNBETTERPATCHPATHFOUND_TASK4, t4);
                taskdlg.AddCommandControl(200, task4);
                taskdlg.SetDefaultCommandControl(100);
                taskdlg.SetMainIcon(TD_INFORMATION_ICON);
                taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                if (taskdlg.DoModal(m_hWnd) == 100)
                {
                    m_data.m_sPatchPath = betterpatchpath;
                    m_patch.Init(m_data.m_sDiffFile, m_data.m_sPatchPath, &progDlg);
                }
            }
            m_dlgFilePatches.Init(&m_patch, this, m_data.m_sPatchPath, this);
            m_dlgFilePatches.ShowWindow(SW_SHOW);
            ClearViewNamesAndPaths();
            if (!m_wndSplitter.IsRowHidden(1))
                m_wndSplitter.HideRow(1);
            m_pwndLeftView->SetHidden(FALSE);
            m_pwndRightView->SetHidden(FALSE);
            m_pwndBottomView->SetHidden(TRUE);
            progDlg.Stop();
        }
    }
    if (m_data.IsBaseFileInUse() && !m_data.IsYourFileInUse() && m_data.IsTheirFileInUse())
    {
        m_data.m_yourFile.TransferDetailsFrom(m_data.m_theirFile);
    }
    if (m_data.IsBaseFileInUse() && m_data.IsYourFileInUse() && !m_data.IsTheirFileInUse())
    {
        //diff between YOUR and BASE
        if (m_bOneWay)
        {
            pwndActiveView = m_pwndLeftView;
            if (!m_wndSplitter2.IsColumnHidden(1))
                m_wndSplitter2.HideColumn(1);

            m_pwndLeftView->m_pViewData = &m_data.m_yourBaseBoth;
            m_pwndLeftView->SetTextType(m_data.m_arYourFile.GetUnicodeType());
            m_pwndLeftView->SetLineEndingStyle(m_data.m_arYourFile.GetLineEndings());
            m_pwndLeftView->m_sWindowName    = m_data.m_baseFile.GetWindowName() + L" - " + m_data.m_yourFile.GetWindowName();
            m_pwndLeftView->m_sFullFilePath  = m_data.m_baseFile.GetFilename() + L" - " + m_data.m_yourFile.GetFilename();
            m_pwndLeftView->m_sReflectedName = m_data.m_yourFile.GetReflectedName();
            m_pwndLeftView->m_pWorkingFile   = &m_data.m_yourFile;
            m_pwndLeftView->SetTarget();
            m_pwndLeftView->SetWritableIsChangable(true);

            m_pwndRightView->m_pViewData     = nullptr;
            m_pwndRightView->m_pWorkingFile  = nullptr;
            m_pwndBottomView->m_pViewData    = nullptr;
            m_pwndBottomView->m_pWorkingFile = nullptr;

            if (!m_wndSplitter.IsRowHidden(1))
                m_wndSplitter.HideRow(1);
            m_pwndLeftView->SetHidden(FALSE);
            m_pwndRightView->SetHidden(TRUE);
            m_pwndBottomView->SetHidden(TRUE);
            ::SetWindowPos(m_pwndLeftView->m_hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        }
        else
        {
            pwndActiveView = m_pwndRightView;
            if (m_wndSplitter2.IsColumnHidden(1))
                m_wndSplitter2.ShowColumn();

            m_pwndLeftView->m_pViewData = &m_data.m_yourBaseLeft;
            m_pwndLeftView->SetTextType(m_data.m_arBaseFile.GetUnicodeType());
            m_pwndLeftView->SetLineEndingStyle(m_data.m_arBaseFile.GetLineEndings());
            m_pwndLeftView->m_sWindowName        = m_data.m_baseFile.GetWindowName();
            m_pwndLeftView->m_sFullFilePath      = m_data.m_baseFile.GetFilename();
            m_pwndLeftView->m_sConvertedFilePath = m_data.m_baseFile.GetConvertedFileName();
            m_pwndLeftView->m_sReflectedName     = m_data.m_baseFile.GetReflectedName();
            m_pwndLeftView->m_pWorkingFile       = &m_data.m_baseFile;
            m_pwndLeftView->SetWritableIsChangable(true);

            m_pwndRightView->m_pViewData = &m_data.m_yourBaseRight;
            m_pwndRightView->SetTextType(m_data.m_arYourFile.GetUnicodeType());
            m_pwndRightView->SetLineEndingStyle(m_data.m_arYourFile.GetLineEndings());
            m_pwndRightView->m_sWindowName        = m_data.m_yourFile.GetWindowName();
            m_pwndRightView->m_sFullFilePath      = m_data.m_yourFile.GetFilename();
            m_pwndRightView->m_sConvertedFilePath = m_data.m_yourFile.GetConvertedFileName();
            m_pwndRightView->m_sReflectedName     = m_data.m_yourFile.GetReflectedName();
            m_pwndRightView->m_pWorkingFile       = &m_data.m_yourFile;
            m_pwndRightView->SetWritable();
            m_pwndRightView->SetTarget();

            m_pwndBottomView->m_pViewData    = nullptr;
            m_pwndBottomView->m_pWorkingFile = nullptr;

            if (!m_wndSplitter.IsRowHidden(1))
                m_wndSplitter.HideRow(1);
            m_pwndLeftView->SetHidden(FALSE);
            m_pwndRightView->SetHidden(FALSE);
            m_pwndBottomView->SetHidden(TRUE);
        }
        bool hasMods, hasConflicts, hasWhitespaceMods, hasFilteredMods;
        pwndActiveView->CheckModifications(hasMods, hasConflicts, hasWhitespaceMods, hasFilteredMods);
        if (!hasMods && !hasConflicts)
        {
            // files appear identical, show a dialog informing the user that there are or might
            // be other differences
            bool hasEncodingDiff = m_data.m_arBaseFile.GetUnicodeType() != m_data.m_arYourFile.GetUnicodeType();
            bool hasEOLDiff      = m_data.m_arBaseFile.GetLineEndings() != m_data.m_arYourFile.GetLineEndings();
            if (hasWhitespaceMods || hasEncodingDiff || hasEOLDiff)
            {
                // text is identical, but the files do not match
                CString sWarning(MAKEINTRESOURCE(IDS_TEXTIDENTICAL_MAIN));
                CString sWhitespace(MAKEINTRESOURCE(IDS_TEXTIDENTICAL_WHITESPACE));
                CString sEncoding(MAKEINTRESOURCE(IDS_TEXTIDENTICAL_ENCODING));
                CString sEOL(MAKEINTRESOURCE(IDS_TEXTIDENTICAL_EOL));
                if (hasWhitespaceMods)
                {
                    sWarning += L"\r\n";
                    sWarning += sWhitespace;
                }
                if (hasEncodingDiff)
                {
                    sWarning += L"\r\n";
                    sWarning += sEncoding;
                    sWarning += L" (";
                    sWarning += m_data.m_arBaseFile.GetEncodingName(m_data.m_arBaseFile.GetUnicodeType());
                    sWarning += L", ";
                    sWarning += m_data.m_arYourFile.GetEncodingName(m_data.m_arYourFile.GetUnicodeType());
                    sWarning += L")";
                }
                if (hasEOLDiff)
                {
                    sWarning += L"\r\n";
                    sWarning += sEOL;
                }
                AfxMessageBox(sWarning, MB_ICONINFORMATION);
            }
        }
    }
    else if (m_data.IsBaseFileInUse() && m_data.IsYourFileInUse() && m_data.IsTheirFileInUse())
    {
        //diff between THEIR, YOUR and BASE
        m_pwndBottomView->SetWritable();
        m_pwndBottomView->SetTarget();
        pwndActiveView              = m_pwndBottomView;

        m_pwndLeftView->m_pViewData = &m_data.m_theirBaseBoth;
        m_pwndLeftView->SetTextType(m_data.m_arTheirFile.GetUnicodeType());
        m_pwndLeftView->SetLineEndingStyle(m_data.m_arTheirFile.GetLineEndings());
        m_pwndLeftView->m_sWindowName.LoadString(IDS_VIEWTITLE_THEIRS);
        m_pwndLeftView->m_sWindowName += L" - " + m_data.m_theirFile.GetWindowName();
        m_pwndLeftView->m_sFullFilePath      = m_data.m_theirFile.GetFilename();
        m_pwndLeftView->m_sConvertedFilePath = m_data.m_theirFile.GetConvertedFileName();
        m_pwndLeftView->m_sReflectedName     = m_data.m_theirFile.GetReflectedName();
        m_pwndLeftView->m_pWorkingFile       = &m_data.m_theirFile;

        m_pwndRightView->m_pViewData         = &m_data.m_yourBaseBoth;
        m_pwndRightView->SetTextType(m_data.m_arYourFile.GetUnicodeType());
        m_pwndRightView->SetLineEndingStyle(m_data.m_arYourFile.GetLineEndings());
        m_pwndRightView->m_sWindowName.LoadString(IDS_VIEWTITLE_MINE);
        m_pwndRightView->m_sWindowName += L" - " + m_data.m_yourFile.GetWindowName();
        m_pwndRightView->m_sFullFilePath      = m_data.m_yourFile.GetFilename();
        m_pwndRightView->m_sConvertedFilePath = m_data.m_yourFile.GetConvertedFileName();
        m_pwndRightView->m_sReflectedName     = m_data.m_yourFile.GetReflectedName();
        m_pwndRightView->m_pWorkingFile       = &m_data.m_yourFile;

        m_pwndBottomView->m_pViewData         = &m_data.m_diff3;
        m_pwndBottomView->SetTextType(m_data.m_arTheirFile.GetUnicodeType());
        m_pwndBottomView->SetLineEndingStyle(m_data.m_arTheirFile.GetLineEndings());
        m_pwndBottomView->m_sWindowName.LoadString(IDS_VIEWTITLE_MERGED);
        m_pwndBottomView->m_sWindowName += L" - " + m_data.m_mergedFile.GetWindowName();
        m_pwndBottomView->m_sFullFilePath      = m_data.m_mergedFile.GetFilename();
        m_pwndBottomView->m_sConvertedFilePath = m_data.m_mergedFile.GetConvertedFileName();
        m_pwndBottomView->m_sReflectedName     = m_data.m_mergedFile.GetReflectedName();
        m_pwndBottomView->m_pWorkingFile       = &m_data.m_mergedFile;

        if (m_wndSplitter2.IsColumnHidden(1))
            m_wndSplitter2.ShowColumn();
        if (m_wndSplitter.IsRowHidden(1))
            m_wndSplitter.ShowRow();
        m_pwndLeftView->SetHidden(FALSE);
        m_pwndRightView->SetHidden(FALSE);
        m_pwndBottomView->SetHidden(FALSE);
        // in three pane view, hide the line diff bar
        m_wndLineDiffBar.ShowPane(false, false, true);
        m_wndLineDiffBar.DocumentUpdated();
    }
    if (!m_data.m_mergedFile.InUse())
    {
        m_data.m_mergedFile.SetFileName(m_data.m_yourFile.GetFilename());
    }
    m_pwndLeftView->BuildAllScreen2ViewVector();
    m_pwndLeftView->DocumentUpdated();
    m_pwndRightView->DocumentUpdated();
    m_pwndBottomView->DocumentUpdated();
    m_wndLocatorBar.DocumentUpdated();
    m_wndLineDiffBar.DocumentUpdated();
    UpdateLayout();
    SetActiveView(pwndActiveView);

    if ((line >= -1) && m_pwndRightView->m_pViewData)
    {
        int n = line == -1 ? min(nOldLineNumber, nOldLine) : line;
        if (n >= 0)
        {
            n = m_pwndRightView->m_pViewData->FindLineNumber(n);
            if (m_bCollapsed && line >= 0)
            {
                // adjust the goto-line position if we're collapsed
                int step = m_pwndRightView->m_nTopLine > n ? -1 : 1;
                int skip = 0;
                for (int i = m_pwndRightView->m_nTopLine; i != n; i += step)
                {
                    if (m_pwndRightView->m_pViewData->GetHideState(i) == HIDESTATE_HIDDEN)
                        ++skip;
                }
                if (m_pwndRightView->m_pViewData->GetHideState(n) == HIDESTATE_HIDDEN)
                {
                    OnViewTextFoldUnfold();
                }
                else
                    n = n + (skip * step * -1);
            }
        }
        if (n < 0)
            n = nOldLine;

        m_pwndRightView->ScrollAllToLine(n);
        POINT p;
        p.x = 0;
        p.y = n;
        if ((ptOldCaretPos.x >= 0) || (ptOldCaretPos.y >= 0))
            p = ptOldCaretPos;
        m_pwndLeftView->SetCaretPosition(p);
        m_pwndRightView->SetCaretPosition(p);
        m_pwndBottomView->SetCaretPosition(p);
        m_pwndBottomView->ScrollToChar(0);
        m_pwndLeftView->ScrollToChar(0);
        m_pwndRightView->ScrollToChar(0);
    }
    else
    {
        CRegDWORD regFirstDiff     = CRegDWORD(L"Software\\TortoiseMerge\\FirstDiffOnLoad", TRUE);
        CRegDWORD regFirstConflict = CRegDWORD(L"Software\\TortoiseMerge\\FirstConflictOnLoad", TRUE);
        bool      bGoFirstDiff     = (0 != static_cast<DWORD>(regFirstDiff));
        bool      bGoFirstConflict = (0 != static_cast<DWORD>(regFirstConflict));
        if (bGoFirstConflict && (CheckResolved() >= 0))
        {
            pwndActiveView->GoToFirstConflict();
            // Ignore the first few Mouse Move messages, so that the line diff stays on
            // the first diff line until the user actually moves the mouse
            m_nMoveMovesToIgnore = MOVESTOIGNORE;
        }
        else if (bGoFirstDiff)
        {
            pwndActiveView->GoToFirstDifference();
            // Ignore the first few Mouse Move messages, so that the line diff stays on
            // the first diff line until the user actually moves the mouse
            m_nMoveMovesToIgnore = MOVESTOIGNORE;
        }
        else
        {
            // Avoid incorrect rendering of active pane.
            m_pwndBottomView->ScrollToChar(0);
            m_pwndLeftView->ScrollToChar(0);
            m_pwndRightView->ScrollToChar(0);
        }
    }
    CheckResolved();
    if (m_bHasConflicts && !m_bSaveRequiredOnConflicts)
        m_bSaveRequired = false;
    CUndo::GetInstance().Clear();
    return true;
}

void CMainFrame::HtmlHelp(DWORD_PTR dwData, UINT nCmd)
{
    UNREFERENCED_PARAMETER(nCmd);

    if (!CCommonAppUtils::StartHtmlHelp(dwData))
    {
        AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
    }
}

BOOL CMainFrame::OnHelpInfo(HELPINFO * /*pHelpInfo*/)
{
    return FALSE;
}

void CMainFrame::UpdateLayout()
{
    if (m_bInitSplitter)
    {
        m_wndSplitter.CenterSplitter();
        m_wndSplitter2.CenterSplitter();
    }
}

void CMainFrame::RecalcLayout(BOOL bNotify)
{
    GetClientRect(&m_dockManager.m_rectInPlace);
    if (m_pRibbonApp)
    {
        m_dockManager.m_rectInPlace.top += m_pRibbonApp->GetRibbonHeight();
    }
    CFrameWndEx::RecalcLayout(bNotify);
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWndEx::OnSize(nType, cx, cy);
    if (m_bInitSplitter && nType != SIZE_MINIMIZED)
    {
        if (m_wndSplitter.GetSafeHwnd())
        {
            if (m_wndSplitter.HasOldRowSize() && (m_wndSplitter.GetOldRowCount() == 2))
            {
                int oldTotal = m_wndSplitter.GetOldRowSize(0) + m_wndSplitter.GetOldRowSize(1);
                if (oldTotal)
                {
                    int cxCur0, cxCur1, cxMin0, cxMin1;
                    m_wndSplitter.GetRowInfo(0, cxCur0, cxMin0);
                    m_wndSplitter.GetRowInfo(1, cxCur1, cxMin1);
                    cxCur0 = m_wndSplitter.GetOldRowSize(0) * (cxCur0 + cxCur1) / oldTotal;
                    cxCur1 = m_wndSplitter.GetOldRowSize(1) * (cxCur0 + cxCur1) / oldTotal;
                    m_wndSplitter.SetRowInfo(0, cxCur0, 0);
                    m_wndSplitter.SetRowInfo(1, cxCur1, 0);
                    m_wndSplitter.RecalcLayout();
                }
            }

            if (m_wndSplitter2.HasOldColSize() && (m_wndSplitter2.GetOldColCount() == 2))
            {
                int oldTotal = m_wndSplitter2.GetOldColSize(0) + m_wndSplitter2.GetOldColSize(1);
                if (oldTotal)
                {
                    int cyCur0, cyCur1, cyMin0, cyMin1;
                    m_wndSplitter2.GetColumnInfo(0, cyCur0, cyMin0);
                    m_wndSplitter2.GetColumnInfo(1, cyCur1, cyMin1);
                    cyCur0 = m_wndSplitter2.GetOldColSize(0) * (cyCur0 + cyCur1) / oldTotal;
                    cyCur1 = m_wndSplitter2.GetOldColSize(1) * (cyCur0 + cyCur1) / oldTotal;
                    m_wndSplitter2.SetColumnInfo(0, cyCur0, 0);
                    m_wndSplitter2.SetColumnInfo(1, cyCur1, 0);
                    m_wndSplitter2.RecalcLayout();
                }
            }
        }
    }
    if ((nType == SIZE_RESTORED) && m_bCheckReload)
    {
        m_bCheckReload = false;
        CheckForReload();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnViewWhitespaces()
{
    CRegDWORD regViewWhitespaces = CRegDWORD(L"Software\\TortoiseMerge\\ViewWhitespaces", 1);
    BOOL      bViewWhitespaces   = regViewWhitespaces;
    if (m_pwndLeftView)
        bViewWhitespaces = m_pwndLeftView->m_bViewWhitespace;

    bViewWhitespaces   = !bViewWhitespaces;
    regViewWhitespaces = bViewWhitespaces;
    if (m_pwndLeftView)
    {
        m_pwndLeftView->m_bViewWhitespace = bViewWhitespaces;
        m_pwndLeftView->Invalidate();
    }
    if (m_pwndRightView)
    {
        m_pwndRightView->m_bViewWhitespace = bViewWhitespaces;
        m_pwndRightView->Invalidate();
    }
    if (m_pwndBottomView)
    {
        m_pwndBottomView->m_bViewWhitespace = bViewWhitespaces;
        m_pwndBottomView->Invalidate();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewWhitespaces(CCmdUI *pCmdUI)
{
    if (m_pwndLeftView)
        pCmdUI->SetCheck(m_pwndLeftView->m_bViewWhitespace);
}

void CMainFrame::OnViewCollapsed()
{
    m_regCollapsed = !static_cast<DWORD>(m_regCollapsed);
    m_bCollapsed   = !!static_cast<DWORD>(m_regCollapsed);

    OnViewTextFoldUnfold();
    m_wndLocatorBar.Invalidate();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewCollapsed(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bCollapsed);
}

void CMainFrame::OnViewWraplonglines()
{
    m_bWrapLines   = !static_cast<DWORD>(m_regWrapLines);
    m_regWrapLines = m_bWrapLines;

    if (m_pwndLeftView)
        m_pwndLeftView->WrapChanged();
    if (m_pwndRightView)
        m_pwndRightView->WrapChanged();
    if (m_pwndBottomView)
        m_pwndBottomView->WrapChanged();
    OnViewTextFoldUnfold();
    m_wndLocatorBar.DocumentUpdated();
}

void CMainFrame::OnViewTextFoldUnfold()
{
    OnViewTextFoldUnfold(m_pwndLeftView);
    OnViewTextFoldUnfold(m_pwndRightView);
    OnViewTextFoldUnfold(m_pwndBottomView);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CMainFrame::OnViewTextFoldUnfold(CBaseView *view)
{
    if (view == nullptr)
        return;
    view->BuildAllScreen2ViewVector();
    view->UpdateCaret();
    view->Invalidate();
    view->EnsureCaretVisible();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewWraplonglines(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bWrapLines);
}

void CMainFrame::OnViewOnewaydiff()
{
    if (CheckForSave(CHFSR_RELOAD) == IDCANCEL)
        return;
    m_bOneWay   = !m_bOneWay;
    m_regOneWay = m_bOneWay;
    ShowDiffBar(!m_bOneWay);
    LoadViews(-1);
}

void CMainFrame::ShowDiffBar(bool bShow)
{
    if (bShow)
    {
        // restore the line diff bar
        m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
        m_wndLineDiffBar.DocumentUpdated();
        m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
        m_wndLocatorBar.DocumentUpdated();
    }
    else
    {
        // in one way view, hide the line diff bar
        m_wndLineDiffBar.ShowPane(false, false, true);
        m_wndLineDiffBar.DocumentUpdated();
    }
}

void CMainFrame::DiffLeftToBase() const
{
    DiffTwo(m_data.m_baseFile, m_data.m_theirFile);
}

void CMainFrame::DiffRightToBase() const
{
    DiffTwo(m_data.m_baseFile, m_data.m_yourFile);
}

void CMainFrame::DiffTwo(const CWorkingFile &file1, const CWorkingFile &file2)
{
    wchar_t ownPath[MAX_PATH] = {0};
    GetModuleFileName(nullptr, ownPath, MAX_PATH);
    CString sCmd;
    sCmd.Format(L"\"%s\"", ownPath);
    sCmd += L" /base:\"";
    sCmd += file1.GetFilename();
    sCmd += L"\" /mine:\"";
    sCmd += file2.GetFilename();
    sCmd += L'"';
    if (!file1.GetWindowName().IsEmpty())
        sCmd += L" /basename:\"" + file1.GetWindowName() + L"\"";
    if (!file2.GetWindowName().IsEmpty())
        sCmd += L" /yourname:\"" + file2.GetWindowName() + L"\"";

    CAppUtils::LaunchApplication(sCmd, 0, false);
}

// Implementation helper only,
// use CTheme::Instance::SetDarkTheme to actually set the theme.
#ifndef UI_PKEY_DarkModeRibbon
// ReSharper disable CppInconsistentNaming
DEFINE_UIPROPERTYKEY(UI_PKEY_DarkModeRibbon, VT_BOOL, 2004);
DEFINE_UIPROPERTYKEY(UI_PKEY_ApplicationButtonColor, VT_UI4, 2003);
// ReSharper restore CppInconsistentNaming
#endif

void CMainFrame::SetTheme(bool bDark) const
{
    if (m_bUseRibbons)
        SetAccentColor();

    if (bDark)
    {
        CComPtr<IPropertyStore> spPropertyStore;
        if (m_bUseRibbons && SUCCEEDED(m_pRibbonFramework->QueryInterface(&spPropertyStore)))
        {
            DarkModeHelper::Instance().AllowDarkModeForApp(TRUE);
            DarkModeHelper::Instance().AllowDarkModeForWindow(GetSafeHwnd(), TRUE);
            PROPVARIANT propvarDarkMode;
            InitPropVariantFromBoolean(1, &propvarDarkMode);
            spPropertyStore->SetValue(UI_PKEY_DarkModeRibbon, propvarDarkMode);
            spPropertyStore->Commit();
        }
        SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetStockObject(BLACK_BRUSH)));
        BOOL                                        darkFlag = TRUE;
        DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data     = {DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag)};
        DarkModeHelper::Instance().SetWindowCompositionAttribute(*this, &data);
        DarkModeHelper::Instance().FlushMenuThemes();
        DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
    }
    else
    {
        DarkModeHelper::Instance().AllowDarkModeForApp(FALSE);
        DarkModeHelper::Instance().AllowDarkModeForWindow(GetSafeHwnd(), FALSE);
        CComPtr<IPropertyStore> spPropertyStore;
        if (m_bUseRibbons && SUCCEEDED(m_pRibbonFramework->QueryInterface(&spPropertyStore)))
        {
            PROPVARIANT propvarDarkMode;
            InitPropVariantFromBoolean(false, &propvarDarkMode);
            spPropertyStore->SetValue(UI_PKEY_DarkModeRibbon, propvarDarkMode);
            spPropertyStore->Commit();
        }
        SetClassLongPtr(GetSafeHwnd(), GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
        BOOL                                        darkFlag = FALSE;
        DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data     = {DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag)};
        DarkModeHelper::Instance().SetWindowCompositionAttribute(*this, &data);
        DarkModeHelper::Instance().FlushMenuThemes();
        DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
    }
    ::RedrawWindow(GetSafeHwnd(), nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void CMainFrame::SetAccentColor() const
{
    if (!m_bUseRibbons)
        return;

    // set the accent color for the main button
    if (m_pRibbonFramework)
    {
        Win10Colors::AccentColor accentColor;
        if (SUCCEEDED(Win10Colors::GetAccentColor(accentColor)))
        {
            BYTE h, s, b;
            CTheme::RGBToHSB(accentColor.accent, h, s, b);
            UI_HSBCOLOR             aColor = UI_HSB(h, s, b);

            CComPtr<IPropertyStore> spPropertyStore;
            HRESULT                 hr = m_pRibbonFramework->QueryInterface(&spPropertyStore);
            if (SUCCEEDED(hr))
            {
                PROPVARIANT propvarAccentColor;
                InitPropVariantFromUInt32(aColor, &propvarAccentColor);
                spPropertyStore->SetValue(UI_PKEY_ApplicationButtonColor, propvarAccentColor);
                spPropertyStore->Commit();
            }
        }
    }
}

int CMainFrame::CheckResolved()
{
    //only in three way diffs can be conflicts!
    m_bHasConflicts = true;
    if (IsViewGood(m_pwndBottomView))
    {
        CViewData *viewData = m_pwndBottomView->m_pViewData;
        if (viewData)
        {
            for (int i = 0; i < viewData->GetCount(); i++)
            {
                const DiffStates state = viewData->GetState(i);
                if ((DIFFSTATE_CONFLICTED == state) || (DIFFSTATE_CONFLICTED_IGNORED == state))
                    return i;
            }
        }
    }
    m_bHasConflicts = false;
    return -1;
}

int CMainFrame::SaveFile(const CString &sFilePath)
{
    CBaseView *     pView       = nullptr;
    CViewData *     pViewData   = nullptr;
    CFileTextLines *pOriginFile = &m_data.m_arBaseFile;
    if (IsViewGood(m_pwndBottomView))
    {
        pView     = m_pwndBottomView;
        pViewData = m_pwndBottomView->m_pViewData;
    }
    else if (IsViewGood(m_pwndRightView))
    {
        pView     = m_pwndRightView;
        pViewData = m_pwndRightView->m_pViewData;
        if (m_data.IsYourFileInUse())
            pOriginFile = &m_data.m_arYourFile;
        else if (m_data.IsTheirFileInUse())
            pOriginFile = &m_data.m_arTheirFile;
    }
    else
    {
        // nothing to save!
        return 1;
    }
    Invalidate();
    if ((pViewData) && (pOriginFile))
    {
        CFileTextLines file;
        pOriginFile->CopySettings(&file);
        CFileTextLines::SaveParams saveParams;
        saveParams.lineEndings = pView->GetLineEndings();
        saveParams.unicodeType = pView->GetTextType();
        file.SetSaveParams(saveParams);
        for (int i = 0; i < pViewData->GetCount(); i++)
        {
            //only copy non-removed lines
            DiffStates state = pViewData->GetState(i);
            switch (state)
            {
                case DIFFSTATE_CONFLICTED:
                case DIFFSTATE_CONFLICTED_IGNORED:
                {
                    int first = i;
                    int last  = i;
                    do
                    {
                        last++;
                    } while ((last < pViewData->GetCount()) && ((pViewData->GetState(last) == DIFFSTATE_CONFLICTED) || (pViewData->GetState(last) == DIFFSTATE_CONFLICTED_IGNORED)));
                    file.Add(L"<<<<<<< .mine", EOL_NOENDING);
                    for (int j = first; j < last; j++)
                    {
                        file.Add(m_pwndRightView->m_pViewData->GetLine(j), m_pwndRightView->m_pViewData->GetLineEnding(j));
                    }
                    file.Add(L"=======", EOL_NOENDING);
                    for (int j = first; j < last; j++)
                    {
                        file.Add(m_pwndLeftView->m_pViewData->GetLine(j), m_pwndLeftView->m_pViewData->GetLineEnding(j));
                    }
                    file.Add(L">>>>>>> .theirs", EOL_NOENDING);
                    i = last - 1;
                }
                break;
                case DIFFSTATE_EMPTY:
                case DIFFSTATE_CONFLICTEMPTY:
                case DIFFSTATE_IDENTICALREMOVED:
                case DIFFSTATE_REMOVED:
                case DIFFSTATE_THEIRSREMOVED:
                case DIFFSTATE_YOURSREMOVED:
                case DIFFSTATE_CONFLICTRESOLVEDEMPTY:
                    // do not save removed lines
                    break;
                default:
                    file.Add(pViewData->GetLine(i), pViewData->GetLineEnding(i));
                    break;
            }
        }
        if (!file.Save(sFilePath, false, false))
        {
            ::MessageBox(m_hWnd, file.GetErrorString(), L"TortoiseMerge", MB_ICONERROR);
            return -1;
        }
        if (sFilePath == m_data.m_baseFile.GetFilename())
        {
            m_data.m_baseFile.StoreFileAttributes();
        }
        if (sFilePath == m_data.m_theirFile.GetFilename())
        {
            m_data.m_theirFile.StoreFileAttributes();
        }
        if (sFilePath == m_data.m_yourFile.GetFilename())
        {
            m_data.m_yourFile.StoreFileAttributes();
        }
        /*if (sFilePath == m_Data.m_mergedFile.GetFilename())
        {
            m_Data.m_mergedFile.StoreFileAttributes();
        }//*/
        m_dlgFilePatches.SetFileStatusAsPatched(sFilePath);
        if (IsViewGood(m_pwndBottomView))
            m_pwndBottomView->SetModified(FALSE);
        else if (IsViewGood(m_pwndRightView))
            m_pwndRightView->SetModified(FALSE);
        CUndo::GetInstance().MarkAsOriginalState(
            false,
            IsViewGood(m_pwndRightView) && (pViewData == m_pwndRightView->m_pViewData),
            IsViewGood(m_pwndBottomView) && (pViewData == m_pwndBottomView->m_pViewData));
        if (file.GetCount() == 1 && file.GetAt(0).IsEmpty() && file.GetLineEnding(0) == EOL_NOENDING)
            return 0;
        return file.GetCount();
    }
    return 1;
}

void CMainFrame::OnFileSave()
{
    // when multiple files are set as writable we have to ask what file to save
    int nEditableViewCount =
        (CBaseView::IsViewGood(m_pwndLeftView) && m_pwndLeftView->IsWritable() ? 1 : 0) + (CBaseView::IsViewGood(m_pwndRightView) && m_pwndRightView->IsWritable() ? 1 : 0) + (CBaseView::IsViewGood(m_pwndBottomView) && m_pwndBottomView->IsWritable() ? 1 : 0);
    bool bLeftIsModified   = CBaseView::IsViewGood(m_pwndLeftView) && m_pwndLeftView->IsModified();
    bool bRightIsModified  = CBaseView::IsViewGood(m_pwndRightView) && m_pwndRightView->IsModified();
    bool bBottomIsModified = CBaseView::IsViewGood(m_pwndBottomView) && m_pwndBottomView->IsModified();
    int  nModifiedViewCount =
        (bLeftIsModified ? 1 : 0) + (bRightIsModified ? 1 : 0) + (bBottomIsModified ? 1 : 0);
    if (nEditableViewCount > 1)
    {
        if (nModifiedViewCount == 1)
        {
            if (bLeftIsModified)
                m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
            else
                FileSave();
        }
        else if (nModifiedViewCount > 0)
        {
            // both views
            UINT        ret = IDNO;
            CTaskDialog taskDlg(CString(MAKEINTRESOURCE(IDS_SAVE_MORE)),
                                CString(MAKEINTRESOURCE(IDS_SAVE)),
                                CString(MAKEINTRESOURCE(IDS_APPNAME)),
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            CString     sTaskTemp;
            if (m_pwndLeftView->m_pWorkingFile->InUse() && !m_pwndLeftView->m_pWorkingFile->IsReadonly())
                sTaskTemp.Format(IDS_ASKFORSAVE_SAVELEFT, static_cast<LPCWSTR>(m_pwndLeftView->m_pWorkingFile->GetFilename()));
            else
                sTaskTemp = CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVELEFTAS));
            taskDlg.AddCommandControl(201, sTaskTemp, bLeftIsModified); // left
            if (bLeftIsModified)
            {
                taskDlg.SetDefaultCommandControl(201);
            }
            if (m_pwndRightView->m_pWorkingFile->InUse() && !m_pwndRightView->m_pWorkingFile->IsReadonly())
                sTaskTemp.Format(IDS_ASKFORSAVE_SAVERIGHT, static_cast<LPCWSTR>(m_pwndRightView->m_pWorkingFile->GetFilename()));
            else
                sTaskTemp = CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVERIGHTAS));
            taskDlg.AddCommandControl(202, sTaskTemp, bRightIsModified); // right
            if (bRightIsModified)
            {
                taskDlg.SetDefaultCommandControl(202);
            }
            taskDlg.AddCommandControl(203, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVEALL2)), nModifiedViewCount > 1); // both
            if (nModifiedViewCount > 1)
            {
                taskDlg.SetDefaultCommandControl(203);
            }
            taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskDlg.SetMainIcon(TD_WARNING_ICON);
            ret = static_cast<UINT>(taskDlg.DoModal(m_hWnd));
            switch (ret)
            {
                case 201: // left
                    m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
                    break;
                case 203: // both
                    m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
                    [[fallthrough]];
                case 202: // right
                    m_pwndRightView->SaveFile();
                    break;
            }
        }
    }
    else
    {
        // only target view was modified
        FileSave();
    }
}

void CMainFrame::PatchSave()
{
    bool bDoesNotExist = !PathFileExists(m_data.m_mergedFile.GetFilename());
    if (m_data.m_bPatchRequired)
    {
        m_patch.PatchPath(m_data.m_mergedFile.GetFilename());
    }
    if (!PathIsDirectory(m_data.m_mergedFile.GetFilename()))
    {
        int saveRet = SaveFile(m_data.m_mergedFile.GetFilename());
        if (saveRet == 0)
        {
            // file was saved with 0 lines, remove it.
            m_patch.RemoveFile(m_data.m_mergedFile.GetFilename());
            // just in case
            DeleteFile(m_data.m_mergedFile.GetFilename());
        }
        m_data.m_mergedFile.StoreFileAttributes();
        if (m_data.m_mergedFile.GetFilename() == m_data.m_yourFile.GetFilename())
            m_data.m_yourFile.StoreFileAttributes();
        if ((bDoesNotExist) && static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseMerge\\AutoAdd", TRUE)))
        {
            // call TortoiseProc to add the new file to version control
            CString cmd = L"/command:add /noui /path:\"";
            cmd += m_data.m_mergedFile.GetFilename() + L"\"";
            CAppUtils::RunTortoiseProc(cmd);
        }
    }
}

svn_error_t *CMainFrame::getallstatus(void *baton, const char * /*path*/, const svn_client_status_t *status, apr_pool_t * /*pool*/)
{
    svn_wc_status_kind *s = static_cast<svn_wc_status_kind *>(baton);
    *s                    = status->node_status;
    return nullptr;
}

bool CMainFrame::FileSave(bool bCheckResolved /*=true*/)
{
    if (HasMarkedBlocks())
    {
        CString     sTitle(MAKEINTRESOURCE(IDS_WARNMARKEDBLOCKS));
        CString     sSubTitle(MAKEINTRESOURCE(IDS_ASKFORSAVE_MARKEDBLOCKS));
        CString     sAppName(MAKEINTRESOURCE(IDS_APPNAME));
        CTaskDialog taskDlg(sTitle,
                            sSubTitle,
                            sAppName,
                            0,
                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskDlg.AddCommandControl(10, CString(MAKEINTRESOURCE(IDS_MARKEDBLOCKSSAVEINCLUDE)));
        taskDlg.AddCommandControl(11, CString(MAKEINTRESOURCE(IDS_MARKEDBLOCKSSAVEEXCLUDE)));
        taskDlg.AddCommandControl(12, CString(MAKEINTRESOURCE(IDS_MARKEDBLCOKSSAVEIGNORE)));
        taskDlg.AddCommandControl(IDCANCEL, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_CANCEL_OPEN)));
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetDefaultCommandControl(10);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        UINT ret = static_cast<UINT>(taskDlg.DoModal(m_hWnd));
        if (ret == 10)
            m_pwndRightView->LeaveOnlyMarkedBlocks();
        else if (ret == 11)
            m_pwndRightView->UseViewFileOfMarked();
        else if (ret == 12)
            m_pwndRightView->UseViewFileExceptEdited();
        else
            return false;
    }

    if (!m_data.m_mergedFile.InUse())
        return FileSaveAs(bCheckResolved);
    // check if the file has the readonly attribute set
    bool  bDoesNotExist = false;
    DWORD fAttribs      = GetFileAttributes(m_data.m_mergedFile.GetFilename());
    if ((fAttribs != INVALID_FILE_ATTRIBUTES) && (fAttribs & FILE_ATTRIBUTE_READONLY))
        return FileSaveAs(bCheckResolved);
    if (fAttribs == INVALID_FILE_ATTRIBUTES)
    {
        bDoesNotExist = (GetLastError() == ERROR_FILE_NOT_FOUND);
    }
    if (bCheckResolved && HasConflictsWontKeep())
        return false;

    if (static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseMerge\\Backup")) != 0)
    {
        MoveFileEx(m_data.m_mergedFile.GetFilename(), m_data.m_mergedFile.GetFilename() + L".bak", MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }
    if (m_data.m_bPatchRequired)
    {
        m_patch.PatchPath(m_data.m_mergedFile.GetFilename());
    }
    int saveret = SaveFile(m_data.m_mergedFile.GetFilename());
    if (saveret == 0)
    {
        // file was saved with 0 lines!
        // ask the user if the file should be deleted
        CString msg;
        msg.Format(IDS_DELETEWHENEMPTY, static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(m_data.m_mergedFile.GetFilename())));
        CTaskDialog taskDlg(msg,
                            CString(MAKEINTRESOURCE(IDS_DELETEWHENEMPTY_TASK2)),
                            CString(MAKEINTRESOURCE(IDS_APPNAME)),
                            0,
                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_DELETEWHENEMPTY_TASK3)));
        taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_DELETEWHENEMPTY_TASK4)));
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetDefaultCommandControl(100);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        bool bDelete = (taskDlg.DoModal(m_hWnd) == 100);
        if (bDelete)
        {
            m_patch.RemoveFile(m_data.m_mergedFile.GetFilename());
            DeleteFile(m_data.m_mergedFile.GetFilename());
        }
    }
    else if (saveret < 0)
    {
        // error while saving the file
        return false;
    }

    // if we're in conflict resolve mode (three pane view), check if there are no more conflicts
    // and if there aren't, ask to mark the file as resolved
    if (IsViewGood(m_pwndBottomView) && !m_bHasConflicts && bCheckResolved)
    {
        CTSVNPath svnPath = CTSVNPath(m_data.m_mergedFile.GetFilename());
        if (m_bAskToMarkAsResolved && SVNHelper::IsVersioned(svnPath, true))
        {
            SVNPool            pool;
            svn_opt_revision_t rev;
            rev.kind                      = svn_opt_revision_unspecified;
            svn_wc_status_kind statusKind = svn_wc_status_none;
            svn_client_ctx_t * ctx        = nullptr;
            svn_error_clear(svn_client_create_context2(&ctx, SVNConfig::Instance().GetConfig(pool), pool));
            svn_error_t *err = svn_client_status6(nullptr, ctx, svnPath.GetSVNApiPath(pool), &rev,
                                                  svn_depth_empty,
                                                  true,
                                                  false,
                                                  true,
                                                  true,
                                                  true,
                                                  false,
                                                  nullptr,
                                                  getallstatus,
                                                  &statusKind,
                                                  pool);
            if ((!err) && (statusKind == svn_wc_status_conflicted))
            {
                CString msg;
                msg.Format(IDS_MARKASRESOLVED, static_cast<LPCWSTR>(CPathUtils::GetFileNameFromPath(m_data.m_mergedFile.GetFilename())));
                CTaskDialog taskDlg(msg,
                                    CString(MAKEINTRESOURCE(IDS_MARKASRESOLVED_TASK2)),
                                    CString(MAKEINTRESOURCE(IDS_APPNAME)),
                                    0,
                                    TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
                taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_MARKASRESOLVED_TASK3)));
                taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_MARKASRESOLVED_TASK4)));
                taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
                taskDlg.SetDefaultCommandControl(100);
                taskDlg.SetMainIcon(TD_WARNING_ICON);
                bool bResolve = (taskDlg.DoModal(m_hWnd) == 100);
                if (bResolve)
                {
                    MarkAsResolved();
                }
            }
            svn_error_clear(err);
        }
    }

    m_bSaveRequired = false;
    m_data.m_mergedFile.StoreFileAttributes();

    if ((bDoesNotExist) && static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseMerge\\AutoAdd", TRUE)))
    {
        // call TortoiseProc to add the new file to version control
        CString cmd = L"/command:add /noui /path:\"";
        cmd += m_data.m_mergedFile.GetFilename() + L"\"";
        if (!CAppUtils::RunTortoiseProc(cmd))
            return false;
    }
    return true;
}

void CMainFrame::OnFileSaveAs()
{
    // ask what file to save as
    bool        bHaveConflict = (CheckResolved() >= 0);
    CTaskDialog taskdlg(
        CString(MAKEINTRESOURCE(bHaveConflict ? IDS_ASKFORSAVEAS_MORECONFLICT : IDS_ASKFORSAVEAS_MORE)),
        CString(MAKEINTRESOURCE(IDS_ASKFORSAVEAS)),
        CString(MAKEINTRESOURCE(IDS_APPNAME)),
        0,
        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
    // default can be last view (target) as was in 1.7 or actual (where is cursor) as is in most text editor
    if (IsViewGood(m_pwndLeftView))
    {
        taskdlg.AddCommandControl(201, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVELEFTAS))); // left
        taskdlg.SetDefaultCommandControl(201);
    }
    if (IsViewGood(m_pwndRightView))
    {
        taskdlg.AddCommandControl(202, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVERIGHTAS))); // right
        taskdlg.SetDefaultCommandControl(202);
    }
    if (IsViewGood(m_pwndBottomView))
    {
        taskdlg.AddCommandControl(203, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVEBOTTOMAS))); // bottom
        taskdlg.SetDefaultCommandControl(203);
    }
    if (bHaveConflict)
    {
        taskdlg.AddCommandControl(204, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_NEEDRESOLVE))); // resolve
        taskdlg.SetDefaultCommandControl(204);
    }
    taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskdlg.SetMainIcon(bHaveConflict ? TD_WARNING_ICON : TD_INFORMATION_ICON);
    int     nCommand = static_cast<int>(taskdlg.DoModal(m_hWnd));
    CString sFileName;
    switch (nCommand)
    {
        case 201: // left
            if (TryGetFileName(sFileName))
            {
                // in 2, 3 view display we want to keep removed lines
                m_pwndLeftView->SaveFileTo(sFileName, IsViewGood(m_pwndRightView) ? SAVE_REMOVEDLINES : 0);
            }
            break;
        case 202: // right
            if (TryGetFileName(sFileName))
            {
                m_pwndRightView->SaveFileTo(sFileName);
            }
            break;
        case 203: // bottom
            FileSaveAs();
            break;
        case 204: // continue resolving
            if (IsViewGood(m_pwndBottomView))
            {
                m_pwndBottomView->GoToLine(CheckResolved());
            }
            break;
    }
}

bool CMainFrame::FileSaveAs(bool bCheckResolved /*=true*/)
{
    if (bCheckResolved && HasConflictsWontKeep())
        return false;

    CString fileName;
    if (!TryGetFileName(fileName))
        return false;

    SaveFile(fileName);
    return true;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateFileSave(CCmdUI *pCmdUI)
{
    BOOL bEnable = FALSE;
    if (m_data.m_mergedFile.InUse())
    {
        if (IsViewGood(m_pwndBottomView) && (m_pwndBottomView->m_pViewData))
            bEnable = TRUE;
        else if ((IsViewGood(m_pwndRightView) && (m_pwndRightView->m_pViewData)) &&
                 (m_pwndRightView->IsModified() || (m_data.m_yourFile.GetWindowName().Right(9).Compare(L": patched") == 0)))
            bEnable = TRUE;
        else if (IsViewGood(m_pwndLeftView) && (m_pwndLeftView->m_pViewData) && (m_pwndLeftView->IsModified()))
            bEnable = TRUE;
    }
    pCmdUI->Enable(bEnable);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateFileSaveAs(CCmdUI *pCmdUI)
{
    // any file is open we can save it as
    BOOL bEnable = FALSE;
    if (IsViewGood(m_pwndBottomView) && (m_pwndBottomView->m_pViewData))
        bEnable = TRUE;
    else if (IsViewGood(m_pwndRightView) && (m_pwndRightView->m_pViewData))
        bEnable = TRUE;
    else if (IsViewGood(m_pwndLeftView) && (m_pwndLeftView->m_pViewData))
        bEnable = TRUE;
    pCmdUI->Enable(bEnable);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewOnewaydiff(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(!m_bOneWay);
    BOOL bEnable = TRUE;
    if (IsViewGood(m_pwndBottomView))
    {
        bEnable = FALSE;
    }
    pCmdUI->Enable(bEnable);
}

void CMainFrame::OnViewOptions()
{
    CString sTemp;
    sTemp.LoadString(IDS_SETTINGSTITLE);
    // dialog does not work well with different dpi settings, disable awareness
    auto      oldDpiAwareness = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    CSettings dlg(sTemp);
    dlg.DoModal();
    m_regIgnoreEOL.read();
    SetThreadDpiAwarenessContext(oldDpiAwareness);
    CTheme::Instance().SetDarkTheme(dlg.IsDarkMode());
    if (dlg.IsReloadNeeded())
    {
        if (CheckForSave(CHFSR_OPTIONS) == IDCANCEL)
            return;
        CDiffColors::GetInstance().LoadRegistry();
        LoadViews();
        return;
    }
    CDiffColors::GetInstance().LoadRegistry();
    if (m_pwndBottomView)
        m_pwndBottomView->DocumentUpdated();
    if (m_pwndLeftView)
        m_pwndLeftView->DocumentUpdated();
    if (m_pwndRightView)
        m_pwndRightView->DocumentUpdated();
}

void CMainFrame::OnClose()
{
    if (!IsWindowEnabled())
        return; // just in case someone sends a WM_CLOSE to the main window while another window (dialog,...) is open
    if (CheckForSave(CHFSR_CLOSE) != IDCANCEL)
    {
        WINDOWPLACEMENT wp;

        // before it is destroyed, save the position of the window
        wp.length = sizeof wp;

        if (GetWindowPlacement(&wp))
        {
            if (IsIconic())
                // never restore to Iconic state
                wp.showCmd = SW_SHOW;

            if ((wp.flags & WPF_RESTORETOMAXIMIZED) != 0)
                // if maximized and maybe iconic restore maximized state
                wp.showCmd = SW_SHOWMAXIMIZED;

            // and write it
            WriteWindowPlacement(&wp);
        }
        WriteViewBarPreferences();
        __super::OnClose();
    }
}

void CMainFrame::OnActivate(UINT nValue, CWnd * /*pwnd*/, BOOL /*bActivated?*/)
{
    if (nValue != 0) // activated
    {
        if (IsIconic())
        {
            m_bCheckReload = TRUE;
        }
        else
        {
            // use a timer to give other messages time to get processed
            // first, like e.g. the WM_CLOSE message in case the user
            // clicked the close button and that brought the window
            // to the front - in that case checking for reload wouldn't
            // do any good.
            SetTimer(IDT_RELOADCHECKTIMER, 300, nullptr);
        }
    }
}

void CMainFrame::OnViewLinedown()
{
    OnViewLineUpDown(1);
}

void CMainFrame::OnViewLineup()
{
    OnViewLineUpDown(-1);
}

void CMainFrame::OnViewLineUpDown(int direction)
{
    if (m_pwndLeftView)
        m_pwndLeftView->ScrollToLine(m_pwndLeftView->m_nTopLine + direction);
    if (m_pwndRightView)
        m_pwndRightView->ScrollToLine(m_pwndRightView->m_nTopLine + direction);
    if (m_pwndBottomView)
        m_pwndBottomView->ScrollToLine(m_pwndBottomView->m_nTopLine + direction);
    m_wndLocatorBar.Invalidate();
    m_nMoveMovesToIgnore = MOVESTOIGNORE;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnViewLineleft()
{
    OnViewLineLeftRight(-1);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnViewLineright()
{
    OnViewLineLeftRight(1);
}

void CMainFrame::OnViewLineLeftRight(int direction) const
{
    if (m_pwndLeftView)
        m_pwndLeftView->ScrollSide(direction);
    if (m_pwndRightView)
        m_pwndRightView->ScrollSide(direction);
    if (m_pwndBottomView)
        m_pwndBottomView->ScrollSide(direction);
}

void CMainFrame::OnEditUseTheirs()
{
    if (m_pwndBottomView)
        m_pwndBottomView->UseTheirTextBlock();
}
// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEditUsetheirblock(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_pwndBottomView && m_pwndBottomView->HasSelection());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditUseMine()
{
    if (m_pwndBottomView)
        m_pwndBottomView->UseMyTextBlock();
}
void CMainFrame::OnUpdateEditUsemyblock(CCmdUI *pCmdUI)
{
    OnUpdateEditUsetheirblock(pCmdUI);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditUseTheirsThenMine()
{
    if (m_pwndBottomView)
        m_pwndBottomView->UseTheirAndYourBlock();
}

void CMainFrame::OnUpdateEditUsetheirthenmyblock(CCmdUI *pCmdUI)
{
    OnUpdateEditUsetheirblock(pCmdUI);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditUseMineThenTheirs()
{
    if (m_pwndBottomView)
        m_pwndBottomView->UseYourAndTheirBlock();
}

void CMainFrame::OnUpdateEditUseminethentheirblock(CCmdUI *pCmdUI)
{
    OnUpdateEditUsetheirblock(pCmdUI);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditUseleftblock()
{
    if (m_pwndBottomView->IsWindowVisible())
        m_pwndBottomView->UseRightBlock();
    else
        m_pwndRightView->UseLeftBlock();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEditUseleftblock(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(IsViewGood(m_pwndRightView) && m_pwndRightView->IsTarget() && m_pwndRightView->HasSelection());
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CMainFrame::OnUpdateUseBlock(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(TRUE);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditUseleftfile()
{
    if (m_pwndBottomView->IsWindowVisible())
        m_pwndBottomView->UseRightFile();
    else
        m_pwndRightView->UseLeftFile();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEditUseleftfile(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(IsViewGood(m_pwndRightView) && m_pwndRightView->IsTarget());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditUseblockfromleftbeforeright()
{
    if (m_pwndRightView)
        m_pwndRightView->UseBothLeftFirst();
}

void CMainFrame::OnUpdateEditUseblockfromleftbeforeright(CCmdUI *pCmdUI)
{
    OnUpdateEditUseleftblock(pCmdUI);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditUseblockfromrightbeforeleft()
{
    if (m_pwndRightView)
        m_pwndRightView->UseBothRightFirst();
}

void CMainFrame::OnUpdateEditUseblockfromrightbeforeleft(CCmdUI *pCmdUI)
{
    OnUpdateEditUseleftblock(pCmdUI);
}

void CMainFrame::OnFileReload()
{
    if (CheckForSave(CHFSR_RELOAD) == IDCANCEL)
        return;
    CDiffColors::GetInstance().LoadRegistry();
    LoadViews(-1);
}

void CMainFrame::ActivateFrame(int nCmdShow)
{
    // nCmdShow is the normal show mode this frame should be in
    // translate default nCmdShow (-1)
    if (nCmdShow == -1)
    {
        if (!IsWindowVisible())
            nCmdShow = SW_SHOWNORMAL;
        else if (IsIconic())
            nCmdShow = SW_RESTORE;
    }

    // bring to top before showing
    BringToTop(nCmdShow);

    if (nCmdShow != -1)
    {
        // show the window as specified
        WINDOWPLACEMENT wp;

        if (!ReadWindowPlacement(&wp))
        {
            ShowWindow(nCmdShow);
        }
        else
        {
            if (nCmdShow != SW_SHOWNORMAL)
                wp.showCmd = nCmdShow;

            SetWindowPlacement(&wp);
        }

        // and finally, bring to top after showing
        BringToTop(nCmdShow);
    }
    CTheme::Instance().SetDarkTheme(CTheme::Instance().IsDarkTheme());
}

BOOL CMainFrame::ReadWindowPlacement(WINDOWPLACEMENT *pwp)
{
    auto       monHash    = GetMonitorSetupHash();
    CRegString placement  = CRegString(CString(L"Software\\TortoiseMerge\\WindowPos_") + monHash.c_str());
    CString    sPlacement = placement;
    if (sPlacement.IsEmpty())
        return FALSE;
    int nRead = _stscanf_s(sPlacement, L"%u,%u,%d,%d,%d,%d,%d,%d,%d,%d",
                           &pwp->flags, &pwp->showCmd,
                           &pwp->ptMinPosition.x, &pwp->ptMinPosition.y,
                           &pwp->ptMaxPosition.x, &pwp->ptMaxPosition.y,
                           &pwp->rcNormalPosition.left, &pwp->rcNormalPosition.top,
                           &pwp->rcNormalPosition.right, &pwp->rcNormalPosition.bottom);
    if (nRead != 10)
        return FALSE;
    pwp->length = sizeof(WINDOWPLACEMENT);

    return TRUE;
}

void CMainFrame::WriteWindowPlacement(WINDOWPLACEMENT *pwp) const
{
    auto       monHash = GetMonitorSetupHash();
    CRegString placement(CString(L"Software\\TortoiseMerge\\WindowPos_") + monHash.c_str());
    wchar_t    szBuffer[_countof("-32767") * 8 + sizeof("65535") * 2];

    swprintf_s(szBuffer, L"%u,%u,%d,%d,%d,%d,%d,%d,%d,%d",
               pwp->flags, pwp->showCmd,
               pwp->ptMinPosition.x, pwp->ptMinPosition.y,
               pwp->ptMaxPosition.x, pwp->ptMaxPosition.y,
               pwp->rcNormalPosition.left, pwp->rcNormalPosition.top,
               pwp->rcNormalPosition.right, pwp->rcNormalPosition.bottom);
    placement = szBuffer;
}

void CMainFrame::WriteViewBarPreferences()
{
    m_regLineDiff = m_bLineDiff;
    m_regLocatorBar = m_bLocatorBar;

    if (m_bUseRibbons)
        m_regStatusBar = (m_wndRibbonStatusBar.GetStyle() & WS_VISIBLE) != 0;
    else
        m_regStatusBar = (m_wndStatusBar.GetStyle() & WS_VISIBLE) != 0;
}


// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateMergeMarkasresolved(CCmdUI *pCmdUI)
{
    if (!pCmdUI)
        return;
    BOOL bEnable = FALSE;
    if ((!m_bReadOnly) && (m_data.m_mergedFile.InUse()))
    {
        if (IsViewGood(m_pwndBottomView) && (m_pwndBottomView->m_pViewData))
        {
            bEnable = !m_bMarkedAsResolvedWasDone;
        }
    }
    pCmdUI->Enable(bEnable);
}

void CMainFrame::OnMergeMarkasresolved()
{
    if (HasConflictsWontKeep())
        return;

    // now check if the file has already been saved and if not, save it.
    if (m_data.m_mergedFile.InUse())
    {
        if (IsViewGood(m_pwndBottomView) && (m_pwndBottomView->m_pViewData))
        {
            if (!FileSave(false))
                return;
            m_bSaveRequired = false;
        }
    }
    MarkAsResolved();
}

BOOL CMainFrame::MarkAsResolved()
{
    if (m_bReadOnly)
        return FALSE;
    if (!IsViewGood(m_pwndBottomView))
        return FALSE;
    if (m_bMarkedAsResolvedWasDone)
        return FALSE;

    CString cmd = L"/command:resolve /path:\"";
    cmd += m_data.m_mergedFile.GetFilename();
    cmd += L"\" /closeonend:1 /noquestion /skipcheck /silent";
    if (resolveMsgWnd)
        cmd.AppendFormat(L" /resolvemsghwnd:%I64d /resolvemsgwparam:%I64d /resolvemsglparam:%I64d", reinterpret_cast<long long>(resolveMsgWnd), static_cast<long long>(resolveMsgWParam), static_cast<long long>(resolveMsgLParam));
    if (!CAppUtils::RunTortoiseProc(cmd))
        return FALSE;
    m_bSaveRequired = false;
    m_bMarkedAsResolvedWasDone = true;
    return TRUE;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateMergeNextconflict(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if (HasNextConflict(m_pwndBottomView))
        bShow = TRUE;
    else if (HasNextConflict(m_pwndRightView))
        bShow = TRUE;
    else if (HasNextConflict(m_pwndLeftView))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

bool CMainFrame::HasNextConflict(CBaseView *view)
{
    if (view == nullptr)
        return false;
    if (!view->IsTarget())
        return false;
    return view->HasNextConflict();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateMergePreviousconflict(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if (HasPrevConflict(m_pwndBottomView))
        bShow = TRUE;
    else if (HasPrevConflict(m_pwndRightView))
        bShow = TRUE;
    else if (HasPrevConflict(m_pwndLeftView))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

bool CMainFrame::HasPrevConflict(CBaseView *view)
{
    if (view == nullptr)
        return false;
    if (!view->IsTarget())
        return false;
    return view->HasPrevConflict();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateNavigateNextdifference(CCmdUI *pCmdUI)
{
    CBaseView *baseView = GetActiveBaseView();
    BOOL       bShow    = FALSE;
    if (baseView != nullptr)
        bShow = baseView->HasNextDiff();
    pCmdUI->Enable(bShow);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateNavigatePreviousdifference(CCmdUI *pCmdUI)
{
    CBaseView *baseView = GetActiveBaseView();
    BOOL       bShow    = FALSE;
    if (baseView != nullptr)
        bShow = baseView->HasPrevDiff();
    pCmdUI->Enable(bShow);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateNavigateNextinlinediff(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if (HasNextInlineDiff(m_pwndBottomView))
        bShow = TRUE;
    else if (HasNextInlineDiff(m_pwndRightView))
        bShow = TRUE;
    else if (HasNextInlineDiff(m_pwndLeftView))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

bool CMainFrame::HasNextInlineDiff(CBaseView *view)
{
    if (view == nullptr)
        return false;
    if (!view->IsTarget())
        return false;
    return view->HasNextInlineDiff();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateNavigatePrevinlinediff(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if (HasPrevInlineDiff(m_pwndBottomView))
        bShow = TRUE;
    else if (HasPrevInlineDiff(m_pwndRightView))
        bShow = TRUE;
    else if (HasPrevInlineDiff(m_pwndLeftView))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

bool CMainFrame::HasPrevInlineDiff(CBaseView *view)
{
    if (view == nullptr)
        return false;
    if (!view->IsTarget())
        return false;
    return view->HasPrevInlineDiff();
}

void CMainFrame::OnMoving(UINT fwSide, LPRECT pRect)
{
    // if the pathfilelist dialog is attached to the mainframe,
    // move it along with the mainframe
    if (::IsWindow(m_dlgFilePatches.m_hWnd))
    {
        RECT patchRect;
        m_dlgFilePatches.GetWindowRect(&patchRect);
        if (::IsWindow(m_hWnd))
        {
            RECT thisRect;
            GetWindowRect(&thisRect);
            if (patchRect.right == thisRect.left)
            {
                m_dlgFilePatches.SetWindowPos(nullptr, patchRect.left - (thisRect.left - pRect->left), patchRect.top - (thisRect.top - pRect->top),
                                              0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
            }
        }
    }
    __super::OnMoving(fwSide, pRect);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
    BOOL bShow = FALSE;
    if ((m_pwndBottomView) && (m_pwndBottomView->HasSelection()))
        bShow = TRUE;
    else if ((m_pwndRightView) && (m_pwndRightView->HasSelection()))
        bShow = TRUE;
    else if ((m_pwndLeftView) && (m_pwndLeftView->HasSelection()))
        bShow = TRUE;
    pCmdUI->Enable(bShow);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
    BOOL bWritable = FALSE;
    if ((m_pwndBottomView) && (m_pwndBottomView->IsWritable()))
        bWritable = TRUE;
    else if ((m_pwndRightView) && (m_pwndRightView->IsWritable()))
        bWritable = TRUE;
    else if ((m_pwndLeftView) && (m_pwndLeftView->IsWritable()))
        bWritable = TRUE;
    pCmdUI->Enable(bWritable && ::IsClipboardFormatAvailable(CF_TEXT));
}

void CMainFrame::OnViewSwitchleft()
{
    if (CheckForSave(CHFSR_SWITCH) != IDCANCEL)
    {
        CWorkingFile file = m_data.m_baseFile;
        m_data.m_baseFile = m_data.m_yourFile;
        m_data.m_yourFile = file;
        if (m_data.m_mergedFile.GetFilename().CompareNoCase(m_data.m_yourFile.GetFilename()) == 0)
        {
            m_data.m_mergedFile = m_data.m_baseFile;
        }
        else if (m_data.m_mergedFile.GetFilename().CompareNoCase(m_data.m_baseFile.GetFilename()) == 0)
        {
            m_data.m_mergedFile = m_data.m_yourFile;
        }
        LoadViews();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewSwitchleft(CCmdUI *pCmdUI)
{
    BOOL bEnable = !IsViewGood(m_pwndBottomView);
    pCmdUI->Enable(bEnable);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewShowfilelist(CCmdUI *pCmdUI)
{
    BOOL bEnable = m_dlgFilePatches.HasFiles();
    pCmdUI->Enable(bEnable);
    pCmdUI->SetCheck(m_dlgFilePatches.IsWindowVisible());
}

void CMainFrame::OnViewShowfilelist()
{
    m_dlgFilePatches.ShowWindow(m_dlgFilePatches.IsWindowVisible() ? SW_HIDE : SW_SHOW);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditUndo()
{
    if (CUndo::GetInstance().CanUndo())
    {
        CUndo::GetInstance().Undo(m_pwndLeftView, m_pwndRightView, m_pwndBottomView);
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CMainFrame::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(CUndo::GetInstance().CanUndo());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditRedo()
{
    if (CUndo::GetInstance().CanRedo())
    {
        CUndo::GetInstance().Redo(m_pwndLeftView, m_pwndRightView, m_pwndBottomView);
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CMainFrame::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(CUndo::GetInstance().CanRedo());
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditEnable()
{
    CBaseView *pView = GetActiveBaseView();
    if (pView && pView->IsReadonlyChangable())
    {
        bool isReadOnly = pView->IsReadonly();
        pView->SetReadonly(!isReadOnly);
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEditEnable(CCmdUI *pCmdUI)
{
    CBaseView *pView = GetActiveBaseView();
    if (pView)
    {
        pCmdUI->Enable(pView->IsReadonlyChangable() || !pView->IsReadonly());
        pCmdUI->SetCheck(!pView->IsReadonly());
    }
    else
    {
        pCmdUI->Enable(FALSE);
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnIndicatorLeftview()
{
    if (m_bUseRibbons)
        return;
    if (IsViewGood(m_pwndLeftView))
    {
        m_pwndLeftView->AskUserForNewLineEndingsAndTextType(IDS_STATUSBAR_LEFTVIEW);
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnIndicatorRightview()
{
    if (m_bUseRibbons)
        return;
    if (IsViewGood(m_pwndRightView))
    {
        m_pwndRightView->AskUserForNewLineEndingsAndTextType(IDS_STATUSBAR_RIGHTVIEW);
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnIndicatorBottomview()
{
    if (m_bUseRibbons)
        return;
    if (IsViewGood(m_pwndBottomView))
    {
        m_pwndBottomView->AskUserForNewLineEndingsAndTextType(IDS_STATUSBAR_BOTTOMVIEW);
    }
}

int CMainFrame::CheckForReload()
{
    static bool bLock = false; //we don't want to check when activated after MessageBox we just created ... this is simple, but we don't need multithread lock
    if (bLock)
    {
        return IDNO;
    }
    bLock = true;
    bool bSourceChanged =
        m_data.m_baseFile.HasSourceFileChanged() || m_data.m_yourFile.HasSourceFileChanged() || m_data.m_theirFile.HasSourceFileChanged()
        /*|| m_Data.m_mergedFile.HasSourceFileChanged()*/;
    if (!bSourceChanged)
    {
        bLock = false;
        return IDNO;
    }

    CString     msg = HasUnsavedEdits() ? CString(MAKEINTRESOURCE(IDS_WARNMODIFIEDOUTSIDELOOSECHANGES)) : CString(MAKEINTRESOURCE(IDS_WARNMODIFIEDOUTSIDE));
    CTaskDialog taskDlg(msg,
                        CString(MAKEINTRESOURCE(IDS_WARNMODIFIEDOUTSIDE_TASK2)),
                        L"TortoiseMerge",
                        0,
                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
    CString     sTask3;
    if (HasUnsavedEdits())
        sTask3.LoadString(IDS_WARNMODIFIEDOUTSIDE_TASK3);
    else
        sTask3.LoadString(IDS_WARNMODIFIEDOUTSIDE_TASK4);
    taskDlg.AddCommandControl(IDYES, sTask3);
    taskDlg.AddCommandControl(IDNO, CString(MAKEINTRESOURCE(IDS_WARNMODIFIEDOUTSIDE_TASK5)));
    taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskDlg.SetDefaultCommandControl(IDYES);
    taskDlg.SetMainIcon(TD_WARNING_ICON);
    UINT ret = static_cast<UINT>(taskDlg.DoModal(m_hWnd));

    if (ret == IDYES)
    {
        CDiffColors::GetInstance().LoadRegistry();
        LoadViews(-1);
    }
    else
    {
        if (IsViewGood(m_pwndBottomView)) // three pane view
        {
            /*if (m_Data.m_sourceFile.HasSourceFileChanged())
                m_pwndBottomView->SetModified();
            if (m_Data.m_mergedFile.HasSourceFileChanged())
                m_pwndBottomView->SetModified();//*/
            if (m_data.m_yourFile.HasSourceFileChanged())
                m_pwndRightView->SetModified();
            if (m_data.m_theirFile.HasSourceFileChanged())
                m_pwndLeftView->SetModified();
        }
        else if (IsViewGood(m_pwndRightView)) // two pane view
        {
            if (m_data.m_baseFile.HasSourceFileChanged())
                m_pwndLeftView->SetModified();
            if (m_data.m_yourFile.HasSourceFileChanged())
                m_pwndRightView->SetModified();
        }
        else
        {
            if (m_data.m_yourFile.HasSourceFileChanged())
                m_pwndLeftView->SetModified();
        }

        // no reload just store updated file time
        m_data.m_baseFile.StoreFileAttributes();
        m_data.m_theirFile.StoreFileAttributes();
        m_data.m_yourFile.StoreFileAttributes();
        //m_Data.m_mergedFile.StoreFileAttributes();
    }
    bLock = false;
    return ret;
}

int CMainFrame::CheckForSave(ECheckForSaveReason eReason)
{
    int idTitle        = IDS_WARNMODIFIEDLOOSECHANGES;
    int idNoSave       = IDS_ASKFORSAVE_TASK7;
    int idCancelAction = IDS_ASKFORSAVE_CANCEL_OPEN;
    switch (eReason)
    {
        case CHFSR_CLOSE:
            //idTitle = IDS_WARNMODIFIEDLOOSECHANGES;
            idNoSave       = IDS_ASKFORSAVE_TASK4;
            idCancelAction = IDS_ASKFORSAVE_TASK5;
            break;
        case CHFSR_SWITCH:
            //idTitle = IDS_WARNMODIFIEDLOOSECHANGES;
            //idNoSave = IDS_ASKFORSAVE_TASK7;
            idCancelAction = IDS_ASKFORSAVE_TASK8;
            break;
        case CHFSR_RELOAD:
            //idTitle = IDS_WARNMODIFIEDLOOSECHANGES;
            //idNoSave = IDS_ASKFORSAVE_TASK7;
            idCancelAction = IDS_ASKFORSAVE_CANCEL_OPEN;
            break;
        case CHFSR_OPTIONS:
            idTitle        = IDS_WARNMODIFIEDLOOSECHANGESOPTIONS;
            //idNoSave = IDS_ASKFORSAVE_TASK7;
            idCancelAction = IDS_ASKFORSAVE_CANCEL_OPTIONS;
            break;
        case CHFSR_OPEN:
            //idTitle = IDS_WARNMODIFIEDLOOSECHANGES;
            idNoSave       = IDS_ASKFORSAVE_NOSAVE_OPEN;
            idCancelAction = IDS_ASKFORSAVE_CANCEL_OPEN;
            break;
    }

    CString sTitle(MAKEINTRESOURCE(idTitle));
    CString sSubTitle(MAKEINTRESOURCE(IDS_ASKFORSAVE_TASK2));
    CString sNoSave(MAKEINTRESOURCE(idNoSave));
    CString sCancelAction(MAKEINTRESOURCE(idCancelAction));
    CString sAppName(MAKEINTRESOURCE(IDS_APPNAME));

    // TODO simplify logic, reduce code duplication
    if (CBaseView::IsViewGood(m_pwndBottomView))
    {
        // three-way diff - by design only bottom can be changed
        // use 1.7 way to do that
    }
    else if (CBaseView::IsViewGood(m_pwndRightView))
    {
        // two-way diff -
        // in 1.7 version only right was saved, now left and/or right can be save, so we need to indicate what we are asking to save
        if (HasUnsavedEdits(m_pwndLeftView))
        {
            // both views
            CTaskDialog taskDlg(sTitle,
                                sSubTitle,
                                sAppName,
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            CString     sTaskTemp;
            if (m_pwndLeftView->m_pWorkingFile->InUse() && !m_pwndLeftView->m_pWorkingFile->IsReadonly())
                sTaskTemp.Format(IDS_ASKFORSAVE_SAVELEFT, static_cast<LPCWSTR>(m_pwndLeftView->m_pWorkingFile->GetFilename()));
            else
                sTaskTemp = CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVELEFTAS));
            taskDlg.AddCommandControl(201, sTaskTemp); // left
            taskDlg.SetDefaultCommandControl(201);
            if (HasUnsavedEdits(m_pwndRightView))
            {
                if (m_pwndRightView->m_pWorkingFile->InUse() && !m_pwndRightView->m_pWorkingFile->IsReadonly())
                    sTaskTemp.Format(IDS_ASKFORSAVE_SAVERIGHT, static_cast<LPCWSTR>(m_pwndRightView->m_pWorkingFile->GetFilename()));
                else
                    sTaskTemp = CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVERIGHTAS));
                taskDlg.AddCommandControl(202, sTaskTemp);                                         // right
                taskDlg.AddCommandControl(203, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVEALL2))); // both
                taskDlg.SetDefaultCommandControl(203);
            }
            taskDlg.AddCommandControl(IDNO, sNoSave);           // none
            taskDlg.AddCommandControl(IDCANCEL, sCancelAction); // cancel
            taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskDlg.SetMainIcon(TD_WARNING_ICON);
            UINT ret = static_cast<UINT>(taskDlg.DoModal(m_hWnd));
            switch (ret)
            {
                case 201: // left
                    m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
                    break;
                case 203: // both
                    m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
                    [[fallthrough]];
                case 202: // right
                    m_pwndRightView->SaveFile();
                    break;
            }
            return ret;
        }
        else
        {
            // only secondary (left) view
        }
        // only right view - 1.7 implementation is used
    }
    else if (CBaseView::IsViewGood(m_pwndLeftView))
    {
        // only one view - only one to save
        // 1.7 FileSave don't support this mode
        if (HasUnsavedEdits(m_pwndLeftView))
        {
            CTaskDialog taskDlg(sTitle,
                                sSubTitle,
                                sAppName,
                                0,
                                TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
            CString     sTask3;
            if (m_data.m_mergedFile.InUse())
                sTask3.Format(IDS_ASKFORSAVE_TASK3, static_cast<LPCWSTR>(m_data.m_mergedFile.GetFilename()));
            else
                sTask3.LoadString(IDS_ASKFORSAVE_TASK6);
            taskDlg.AddCommandControl(IDYES, sTask3);
            taskDlg.AddCommandControl(IDNO, sNoSave);
            taskDlg.AddCommandControl(IDCANCEL, sCancelAction);
            taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
            taskDlg.SetDefaultCommandControl(IDYES);
            taskDlg.SetMainIcon(TD_WARNING_ICON);
            UINT ret = static_cast<UINT>(taskDlg.DoModal(m_hWnd));

            if (ret == IDYES)
            {
                if (m_pwndLeftView->SaveFile() < 0)
                    return IDCANCEL;
            }
        }
        return IDNO;
    }
    else
    {
        return IDNO; // nothing to save
    }

    // 1.7 implementation
    UINT ret = IDNO;
    if (HasUnsavedEdits())
    {
        CTaskDialog taskDlg(sTitle,
                            sSubTitle,
                            sAppName,
                            0,
                            TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
        CString     sTask3;
        if (m_data.m_mergedFile.InUse())
            sTask3.Format(IDS_ASKFORSAVE_TASK3, static_cast<LPCWSTR>(m_data.m_mergedFile.GetFilename()));
        else
            sTask3.LoadString(IDS_ASKFORSAVE_TASK6);
        taskDlg.AddCommandControl(IDYES, sTask3);
        taskDlg.AddCommandControl(IDNO, sNoSave);
        taskDlg.AddCommandControl(IDCANCEL, sCancelAction);
        taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
        taskDlg.SetDefaultCommandControl(IDYES);
        taskDlg.SetMainIcon(TD_WARNING_ICON);
        ret = static_cast<UINT>(taskDlg.DoModal(m_hWnd));

        if (ret == IDYES)
        {
            if (!FileSave())
                ret = IDCANCEL;
        }
    }
    return ret;
}

bool CMainFrame::HasUnsavedEdits() const
{
    return HasUnsavedEdits(m_pwndBottomView) || HasUnsavedEdits(m_pwndRightView) || m_bSaveRequired;
}

bool CMainFrame::HasUnsavedEdits(const CBaseView *view)
{
    if (!CBaseView::IsViewGood(view))
        return false;
    return view->IsModified();
}

bool CMainFrame::HasMarkedBlocks() const
{
    return CBaseView::IsViewGood(m_pwndRightView) && m_pwndRightView->HasMarkedBlocks();
}

bool CMainFrame::IsViewGood(const CBaseView *view)
{
    return CBaseView::IsViewGood(view);
}

void CMainFrame::OnViewInlinediffword()
{
    m_bInlineWordDiff = !m_bInlineWordDiff;
    if (m_pwndLeftView)
    {
        m_pwndLeftView->SetInlineWordDiff(m_bInlineWordDiff);
        m_pwndLeftView->BuildAllScreen2ViewVector();
        m_pwndLeftView->DocumentUpdated();
    }
    if (m_pwndRightView)
    {
        m_pwndRightView->SetInlineWordDiff(m_bInlineWordDiff);
        m_pwndRightView->BuildAllScreen2ViewVector();
        m_pwndRightView->DocumentUpdated();
    }
    if (m_pwndBottomView)
    {
        m_pwndBottomView->SetInlineWordDiff(m_bInlineWordDiff);
        m_pwndBottomView->BuildAllScreen2ViewVector();
        m_pwndBottomView->DocumentUpdated();
    }
    m_wndLineDiffBar.DocumentUpdated();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewInlinediffword(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_bInlineDiff && IsViewGood(m_pwndLeftView) && IsViewGood(m_pwndRightView));
    pCmdUI->SetCheck(m_bInlineWordDiff);
}

void CMainFrame::OnViewInlinediff()
{
    m_bInlineDiff = !m_bInlineDiff;
    if (m_pwndLeftView)
    {
        m_pwndLeftView->SetInlineDiff(m_bInlineDiff);
        m_pwndLeftView->BuildAllScreen2ViewVector();
        m_pwndLeftView->DocumentUpdated();
    }
    if (m_pwndRightView)
    {
        m_pwndRightView->SetInlineDiff(m_bInlineDiff);
        m_pwndRightView->BuildAllScreen2ViewVector();
        m_pwndRightView->DocumentUpdated();
    }
    if (m_pwndBottomView)
    {
        m_pwndBottomView->SetInlineDiff(m_bInlineDiff);
        m_pwndBottomView->BuildAllScreen2ViewVector();
        m_pwndBottomView->DocumentUpdated();
    }
    m_wndLineDiffBar.DocumentUpdated();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewInlinediff(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(IsViewGood(m_pwndLeftView) && IsViewGood(m_pwndRightView));
    pCmdUI->SetCheck(m_bInlineDiff);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEditCreateunifieddifffile(CCmdUI *pCmdUI)
{
    // "create unified diff file" is only available if two files
    // are diffed, not three.
    bool bEnabled = true;
    if (!IsViewGood(m_pwndLeftView))
        bEnabled = false;
    else if (!IsViewGood(m_pwndRightView))
        bEnabled = false;
    else if (IsViewGood(m_pwndBottomView)) //no negation here
        bEnabled = false;
    pCmdUI->Enable(bEnabled);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEditCreateunifieddifffile()
{
    CString origFile, modifiedFile;
    // the original file is the one on the left
    if (m_pwndLeftView)
        origFile = m_pwndLeftView->m_sFullFilePath;
    if (m_pwndRightView)
        modifiedFile = m_pwndRightView->m_sFullFilePath;
    if (origFile.IsEmpty() || modifiedFile.IsEmpty())
        return;

    CString              outputFile;
    bool                 ignoreEOL = true;
    // Create a new common save file dialog
    CComPtr<IFileDialog> pfd       = nullptr;

    auto                 hr        = pfd.CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER);
    if (SUCCEEDED(hr))
    {
        // Set the dialog options
        DWORD dwOptions;
        if (SUCCEEDED(hr = pfd->GetOptions(&dwOptions)))
        {
            hr = pfd->SetOptions(dwOptions | FOS_OVERWRITEPROMPT | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        }

        CComPtr<IFileDialogCustomize> pfdCustomize;
        hr = pfd.QueryInterface(&pfdCustomize);
        if (SUCCEEDED(hr))
        {
            pfdCustomize->AddCheckButton(101, CString(MAKEINTRESOURCE(IDS_DIFF_IGNORE_EOL)), TRUE);
        }

        // Show the save/open file dialog
        if (SUCCEEDED(hr) && SUCCEEDED(hr = pfd->Show(GetSafeHwnd())))
        {
            CComPtr<IFileDialogCustomize> pfdCustomizeRet;
            hr = pfd.QueryInterface(&pfdCustomizeRet);
            if (SUCCEEDED(hr))
            {
                BOOL bChecked = TRUE;
                pfdCustomizeRet->GetCheckButtonState(101, &bChecked);
                ignoreEOL = (bChecked != 0);
            }

            // Get the selection from the user
            CComPtr<IShellItem> psiResult = nullptr;
            hr                            = pfd->GetResult(&psiResult);
            if (SUCCEEDED(hr))
            {
                PWSTR pszPath = nullptr;
                hr            = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                if (SUCCEEDED(hr))
                {
                    outputFile = CString(pszPath);
                }
            }
        }
        else
            return; // user cancelled
    }

    CRegStdDWORD regContextLines(L"Software\\TortoiseMerge\\ContextLines", 3);
    CAppUtils::CreateUnifiedDiff(origFile, modifiedFile, outputFile, regContextLines, ignoreEOL, true);
    CAppUtils::StartUnifiedDiffViewer(outputFile, CPathUtils::GetFileNameFromPath(outputFile));
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewLinediffbar(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bLineDiff);
    pCmdUI->Enable();
}

void CMainFrame::OnViewLinediffbar()
{
    m_bLineDiff = !m_bLineDiff;
    m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
    m_wndLineDiffBar.DocumentUpdated();
    m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
    m_wndLocatorBar.DocumentUpdated();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewLocatorbar(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bLocatorBar);
    pCmdUI->Enable();
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CMainFrame::OnUpdateViewBars(CCmdUI *pCmdUI)
{
    pCmdUI->Enable();
}

void CMainFrame::OnViewLocatorbar()
{
    m_bLocatorBar = !m_bLocatorBar;
    m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
    m_wndLocatorBar.DocumentUpdated();
    m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
    m_wndLineDiffBar.DocumentUpdated();
}

void CMainFrame::OnViewComparewhitespaces()
{
    if (CheckForSave(CHFSR_OPTIONS) == IDCANCEL)
        return;
    CRegDWORD regIgnoreWS(L"Software\\TortoiseMerge\\IgnoreWS");
    regIgnoreWS = static_cast<int>(IgnoreWS::None);
    LoadViews(-1);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CMainFrame::OnUpdateViewComparewhitespaces(CCmdUI *pCmdUI)
{
    CRegDWORD regIgnoreWS(L"Software\\TortoiseMerge\\IgnoreWS");
    IgnoreWS  ignoreWs = static_cast<IgnoreWS>(static_cast<DWORD>(regIgnoreWS));
    pCmdUI->SetCheck(ignoreWs == IgnoreWS::None);
}

void CMainFrame::OnViewIgnorewhitespacechanges()
{
    if (CheckForSave(CHFSR_OPTIONS) == IDCANCEL)
        return;
    CRegDWORD regIgnoreWS(L"Software\\TortoiseMerge\\IgnoreWS");
    regIgnoreWS = static_cast<int>(IgnoreWS::WhiteSpaces);
    LoadViews(-1);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CMainFrame::OnUpdateViewIgnorewhitespacechanges(CCmdUI *pCmdUI)
{
    CRegDWORD regIgnoreWS(L"Software\\TortoiseMerge\\IgnoreWS");
    IgnoreWS  ignoreWs = static_cast<IgnoreWS>(static_cast<DWORD>(regIgnoreWS));
    pCmdUI->SetCheck(ignoreWs == IgnoreWS::WhiteSpaces);
}

void CMainFrame::OnViewIgnoreallwhitespacechanges()
{
    if (CheckForSave(CHFSR_OPTIONS) == IDCANCEL)
        return;
    CRegDWORD regIgnoreWS(L"Software\\TortoiseMerge\\IgnoreWS");
    regIgnoreWS = static_cast<int>(IgnoreWS::AllWhiteSpaces);
    LoadViews(-1);
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CMainFrame::OnUpdateViewIgnoreallwhitespacechanges(CCmdUI *pCmdUI)
{
    CRegDWORD regIgnoreWS(L"Software\\TortoiseMerge\\IgnoreWS");
    IgnoreWS  ignoreWs = static_cast<IgnoreWS>(static_cast<DWORD>(regIgnoreWS));
    pCmdUI->SetCheck(ignoreWs == IgnoreWS::AllWhiteSpaces);
}

void CMainFrame::OnViewMovedBlocks()
{
    m_bViewMovedBlocks   = !static_cast<DWORD>(m_regViewModedBlocks);
    m_regViewModedBlocks = m_bViewMovedBlocks;
    LoadViews(-1);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewMovedBlocks(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bViewMovedBlocks);
    BOOL bEnable = TRUE;
    if (IsViewGood(m_pwndBottomView))
    {
        bEnable = FALSE;
    }
    pCmdUI->Enable(bEnable);
}

bool CMainFrame::HasConflictsWontKeep()
{
    const int nConflictLine = CheckResolved();
    if (nConflictLine < 0)
        return false;
    if (!m_pwndBottomView)
        return false;

    CString sTemp;
    sTemp.Format(IDS_ERR_MAINFRAME_FILEHASCONFLICTS, m_pwndBottomView->m_pViewData->GetLineNumber(nConflictLine) + 1);
    CTaskDialog taskDlg(sTemp,
                        CString(MAKEINTRESOURCE(IDS_ERR_MAINFRAME_FILEHASCONFLICTS_TASK2)),
                        L"TortoiseMerge",
                        0,
                        TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SIZE_TO_CONTENT);
    taskDlg.AddCommandControl(100, CString(MAKEINTRESOURCE(IDS_ERR_MAINFRAME_FILEHASCONFLICTS_TASK3)));
    taskDlg.AddCommandControl(200, CString(MAKEINTRESOURCE(IDS_ERR_MAINFRAME_FILEHASCONFLICTS_TASK4)));
    taskDlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
    taskDlg.SetDefaultCommandControl(200);
    taskDlg.SetMainIcon(TD_ERROR_ICON);
    bool bSave = (taskDlg.DoModal(m_hWnd) == 100);

    if (bSave)
        return false;

    m_pwndBottomView->GoToLine(nConflictLine);
    return true;
}

bool CMainFrame::TryGetFileName(CString &result) const
{
    return CCommonAppUtils::FileOpenSave(result, nullptr, IDS_SAVEASTITLE, IDS_COMMONFILEFILTER, false, CString(), m_hWnd);
}

CBaseView *CMainFrame::GetActiveBaseView() const
{
    CView *    activeView = GetActiveView();
    CBaseView *activeBase = dynamic_cast<CBaseView *>(activeView);
    return activeBase;
}

void CMainFrame::SetWindowTitle()
{
    // try to find a suitable window title
    CString sYour = m_data.m_yourFile.GetDescriptiveName();
    if (sYour.Find(L" - ") >= 0)
        sYour = sYour.Left(sYour.Find(L" - "));
    if (sYour.Find(L" : ") >= 0)
        sYour = sYour.Left(sYour.Find(L" : "));
    CString sTheir = m_data.m_theirFile.GetDescriptiveName();
    if (sTheir.IsEmpty())
        sTheir = m_data.m_baseFile.GetDescriptiveName();
    if (sTheir.Find(L" - ") >= 0)
        sTheir = sTheir.Left(sTheir.Find(L" - "));
    if (sTheir.Find(L" : ") >= 0)
        sTheir = sTheir.Left(sTheir.Find(L" : "));

    if (!sYour.IsEmpty() && !sTheir.IsEmpty())
    {
        if (sYour.CompareNoCase(sTheir) == 0)
            SetWindowText(sYour + L" - TortoiseMerge");
        else if ((sYour.GetLength() < 10) &&
                 (sTheir.GetLength() < 10))
            SetWindowText(sYour + L" - " + sTheir + L" - TortoiseMerge");
        else
        {
            // we have two very long descriptive texts here, which
            // means we have to find a way to use them as a window
            // title in a shorter way.
            // for simplicity, we just use the one from "yourfile"
            SetWindowText(sYour + L" - TortoiseMerge");
        }
    }
    else if (!sYour.IsEmpty())
        SetWindowText(sYour + L" - TortoiseMerge");
    else if (!sTheir.IsEmpty())
        SetWindowText(sTheir + L" - TortoiseMerge");
    else
        SetWindowText(L"TortoiseMerge");
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    switch (nIDEvent)
    {
        case IDT_RELOADCHECKTIMER:
            KillTimer(nIDEvent);
            CheckForReload();
            break;
    }

    __super::OnTimer(nIDEvent);
}

void CMainFrame::LoadIgnoreCommentData()
{
    static bool bLoaded = false;
    if (bLoaded)
        return;
    CString sPath = CPathUtils::GetAppDataDirectory() + L"ignorecomments.txt";
    if (!PathFileExists(sPath))
    {
        // ignore comments file does not exist (yet), so create a default one
        HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(IDR_IGNORECOMMENTSTXT), L"config");
        if (hRes)
        {
            HGLOBAL hResourceLoaded = LoadResource(nullptr, hRes);
            if (hResourceLoaded)
            {
                char *lpResLock = static_cast<char *>(LockResource(hResourceLoaded));
                DWORD dwSizeRes = SizeofResource(nullptr, hRes);
                if (lpResLock)
                {
                    HANDLE hFile = CreateFile(sPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        DWORD dwWritten = 0;
                        WriteFile(hFile, lpResLock, dwSizeRes, &dwWritten, nullptr);
                        CloseHandle(hFile);
                    }
                }
            }
        }
    }

    try
    {
        CStdioFile file;
        if (file.Open(sPath, CFile::modeRead))
        {
            CString sLine;
            while (file.ReadString(sLine))
            {
                int eqpos = sLine.Find('=');
                if (eqpos >= 0)
                {
                    CString sExts        = sLine.Left(eqpos);
                    CString sComments    = sLine.Mid(eqpos + 1);

                    int     pos          = sComments.Find(',');
                    CString sLineStart   = sComments.Left(pos);
                    pos                  = sComments.Find(',', pos);
                    int     pos2         = sComments.Find(',', pos + 1);
                    CString sBlockStart  = sComments.Mid(pos + 1, pos2 - pos - 1);
                    CString sBlockEnd    = sComments.Mid(pos2 + 1);

                    auto    commentTuple = std::make_tuple(sLineStart, sBlockStart, sBlockEnd);

                    pos                  = 0;
                    CString temp;
                    for (;;)
                    {
                        temp = sExts.Tokenize(L",", pos);
                        if (temp.IsEmpty())
                        {
                            break;
                        }
                        ASSERT(m_ignoreCommentsMap.find(temp) == m_ignoreCommentsMap.end());
                        m_ignoreCommentsMap[temp] = commentTuple;
                    }
                }
            }
        }
    }
    catch (CFileException *e)
    {
        e->Delete();
    }
    bLoaded = true;
}

void CMainFrame::OnViewIgnorecomments()
{
    if (CheckForSave(CHFSR_OPTIONS) == IDCANCEL)
        return;
    m_regIgnoreComments = !static_cast<DWORD>(m_regIgnoreComments);
    LoadViews(-1);
}

void CMainFrame::OnUpdateViewIgnorecomments(CCmdUI *pCmdUI)
{
    // only enable if we have comments defined for this file extension
    CString sExt = CPathUtils::GetFileExtFromPath(m_data.m_baseFile.GetFilename()).MakeLower();
    sExt.TrimLeft(L".");
    auto sC = m_ignoreCommentsMap.find(sExt);
    if (sC == m_ignoreCommentsMap.end())
    {
        sExt = CPathUtils::GetFileExtFromPath(m_data.m_yourFile.GetFilename()).MakeLower();
        sExt.TrimLeft(L".");
        sC = m_ignoreCommentsMap.find(sExt);
        if (sC == m_ignoreCommentsMap.end())
        {
            sExt = CPathUtils::GetFileExtFromPath(m_data.m_theirFile.GetFilename()).MakeLower();
            sExt.TrimLeft(L".");
            sC = m_ignoreCommentsMap.find(sExt);
        }
    }
    pCmdUI->Enable(sC != m_ignoreCommentsMap.end());

    pCmdUI->SetCheck(static_cast<DWORD>(m_regIgnoreComments) != 0);
}

void CMainFrame::OnViewIgnoreEOL()
{
    if (CheckForSave(ECheckForSaveReason::CHFSR_OPTIONS) == IDCANCEL)
        return;
    bool      bIgnoreEOL = static_cast<DWORD>(m_regIgnoreEOL) != 0;
    bIgnoreEOL           = !bIgnoreEOL;
    m_regIgnoreEOL         = bIgnoreEOL;
    LoadViews();
}

void CMainFrame::OnUpdateViewIgnoreEOL(CCmdUI *pCmdUI)
{
    bool      bIgnoreEOL = static_cast<DWORD>(m_regIgnoreEOL) != 0;
    pCmdUI->SetCheck(bIgnoreEOL);
}

void CMainFrame::OnRegexfilter(UINT cmd)
{
    if ((cmd == ID_REGEXFILTER) || (cmd == (ID_REGEXFILTER + 1)))
    {
        CRegexFiltersDlg dlg(this);
        dlg.SetIniFile(&m_regexIni);
        if (dlg.DoModal() == IDOK)
        {
            FILE *pFile = nullptr;
            _tfopen_s(&pFile, CPathUtils::GetAppDataDirectory() + L"regexfilters.ini", L"wb");
            m_regexIni.SaveFile(pFile);
            fclose(pFile);
        }
        BuildRegexSubitems();
    }
    else
    {
        if (cmd == static_cast<UINT>(m_regexIndex) && !m_bUseRibbons)
        {
            if (CheckForSave(CHFSR_OPTIONS) == IDCANCEL)
                return;
            m_data.SetRegexTokens(std::wregex(), L"");
            m_regexIndex = -1;
            LoadViews(-1);
        }
        else if (cmd != static_cast<UINT>(m_regexIndex))
        {
            CSimpleIni::TNamesDepend sections;
            m_regexIni.GetAllSections(sections);
            int index    = ID_REGEXFILTER + 2;
            m_regexIndex = -1;
            for (const auto &section : sections)
            {
                if (cmd == static_cast<UINT>(index))
                {
                    if (CheckForSave(CHFSR_OPTIONS) == IDCANCEL)
                        break;
                    try
                    {
                        std::wregex rx(m_regexIni.GetValue(section, L"regex", L""));
                        m_data.SetRegexTokens(rx, m_regexIni.GetValue(section, L"replace", L""));
                    }
                    catch (std::exception &ex)
                    {
                        MessageBox(CString(MAKEINTRESOURCE(IDS_ERR_REGEX_INVALID)) + L"\r\n" + CString(ex.what()));
                    }
                    m_regexIndex = index;
                    try
                    {
                        LoadViews(-1);
                    }
                    catch (const std::regex_error &ex)
                    {
                        CString sErr;
                        sErr.Format(IDS_ERR_REGEX_INVALIDRETRY, ex.what());
                        MessageBox(sErr);
                        m_data.SetRegexTokens(std::wregex(), L"");
                        m_regexIndex = -1;
                        LoadViews(-1);
                    }
                    break;
                }
                ++index;
            }
        }
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateViewRegexFilter(CCmdUI *pCmdUI)
{
    pCmdUI->Enable();
    pCmdUI->SetCheck(pCmdUI->m_nID == static_cast<UINT>(m_regexIndex));
}

void CMainFrame::BuildRegexSubitems(CMFCPopupMenu *pMenuPopup)
{
    CString sIniPath = CPathUtils::GetAppDataDirectory() + L"regexfilters.ini";
    if (!PathFileExists(sIniPath))
    {
        // ini file does not exist (yet), so create a default one
        HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(IDR_REGEXFILTERINI), L"config");
        if (hRes)
        {
            HGLOBAL hResourceLoaded = LoadResource(nullptr, hRes);
            if (hResourceLoaded)
            {
                char *lpResLock = static_cast<char *>(LockResource(hResourceLoaded));
                DWORD dwSizeRes = SizeofResource(nullptr, hRes);
                if (lpResLock)
                {
                    HANDLE hFile = CreateFile(sIniPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        DWORD dwWritten = 0;
                        WriteFile(hFile, lpResLock, dwSizeRes, &dwWritten, nullptr);
                        CloseHandle(hFile);
                    }
                }
            }
        }
    }

    m_regexIni.LoadFile(sIniPath);
    CSimpleIni::TNamesDepend sections;
    m_regexIni.GetAllSections(sections);

    if (m_bUseRibbons)
    {
        std::list<CNativeRibbonDynamicItemInfo> items;
        int                                     cmdIndex = 2;
        items.push_back(CNativeRibbonDynamicItemInfo(ID_REGEX_NO_FILTER, CString(MAKEINTRESOURCE(ID_REGEX_NO_FILTER)), IDB_REGEX_FILTER));
        for (const auto &section : sections)
        {
            items.push_back(CNativeRibbonDynamicItemInfo(ID_REGEXFILTER + cmdIndex, section, IDB_REGEX_FILTER));
            cmdIndex++;
        }

        m_pRibbonApp->SetItems(ID_REGEXFILTER, items);
    }
    else if (pMenuPopup)
    {
        int iIndex = -1;
        if (!CMFCToolBar::IsCustomizeMode() &&
            (iIndex = pMenuPopup->GetMenuBar()->CommandToIndex(ID_REGEXFILTER)) >= 0)
        {
            if (!sections.empty())
                pMenuPopup->InsertSeparator(iIndex + 1); // insert the separator at the end
            int cmdIndex = 2;
            for (const auto &section : sections)
            {
                pMenuPopup->InsertItem(CMFCToolBarMenuButton(ID_REGEXFILTER + cmdIndex, nullptr, -1, static_cast<LPCWSTR>(section)), iIndex + cmdIndex);
                cmdIndex++;
            }
        }
    }
}

void CMainFrame::FillEncodingButton(CMFCRibbonButton *pButton, int start)
{
    pButton->SetDefaultCommand(FALSE);
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::ASCII, L"ASCII"));
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::BINARY, L"BINARY"));
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF16_LE, L"UTF-16LE"));
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF16_LEBOM, L"UTF-16LE BOM"));
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF16_BE, L"UTF-16BE"));
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF16_BEBOM, L"UTF-16BE BOM"));
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF32_LE, L"UTF-32LE"));
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF32_BE, L"UTF-32BE"));
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF8, L"UTF-8"));
    pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF8BOM, L"UTF-8 BOM"));
}

void CMainFrame::FillEOLButton(CMFCRibbonButton *pButton, int start)
{
    pButton->SetDefaultCommand(FALSE);
    pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_LF, L"LF"));
    pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_CRLF, L"CRLF"));
    pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_LFCR, L"LRCR"));
    pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_CR, L"CR"));
    pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_VT, L"VT"));
    pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_FF, L"FF"));
    pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_NEL, L"NEL"));
    pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_LS, L"LS"));
    pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_PS, L"PS"));
}

void CMainFrame::FillTabModeButton(CMFCRibbonButton *pButton, int start)
{
    pButton->SetDefaultCommand(FALSE);
    pButton->AddSubItem(new CMFCRibbonButton(start + TABMODE_NONE, L"Tab"));
    pButton->AddSubItem(new CMFCRibbonButton(start + TABMODE_USESPACES, L"Space"));
    pButton->AddSubItem(new CMFCRibbonSeparator(TRUE));
    pButton->AddSubItem(new CMFCRibbonButton(start + TABMODE_SMARTINDENT, L"Smart tab char"));
    pButton->AddSubItem(new CMFCRibbonSeparator(TRUE));
    pButton->AddSubItem(new CMFCRibbonButton(start + TABSIZEBUTTON1, L"1"));
    pButton->AddSubItem(new CMFCRibbonButton(start + TABSIZEBUTTON2, L"2"));
    pButton->AddSubItem(new CMFCRibbonButton(start + TABSIZEBUTTON4, L"4"));
    pButton->AddSubItem(new CMFCRibbonButton(start + TABSIZEBUTTON8, L"8"));
    pButton->AddSubItem(new CMFCRibbonSeparator(TRUE));
    pButton->AddSubItem(new CMFCRibbonButton(start + ENABLEEDITORCONFIG, L"EditorConfig"));
}

bool CMainFrame::AdjustUnicodeTypeForLoad(CFileTextLines::UnicodeType &type)
{
    switch (type)
    {
        case CFileTextLines::UnicodeType::AUTOTYPE:
        case CFileTextLines::UnicodeType::BINARY:
            return false;

        case CFileTextLines::UnicodeType::ASCII:
        case CFileTextLines::UnicodeType::UTF16_LE:
        case CFileTextLines::UnicodeType::UTF16_BE:
        case CFileTextLines::UnicodeType::UTF32_LE:
        case CFileTextLines::UnicodeType::UTF32_BE:
        case CFileTextLines::UnicodeType::UTF8:
            return true;

        case CFileTextLines::UnicodeType::UTF16_LEBOM:
            type = CFileTextLines::UnicodeType::UTF16_LE;
            return true;

        case CFileTextLines::UnicodeType::UTF16_BEBOM:
            type = CFileTextLines::UnicodeType::UTF16_BE;
            return true;

        case CFileTextLines::UnicodeType::UTF8BOM:
            type = CFileTextLines::UnicodeType::UTF8;
            return true;
    }
    return false;
}

void CMainFrame::OnEncodingLeft(UINT cmd)
{
    if (m_pwndLeftView)
    {
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            // reload with selected encoding
            auto saveParams        = m_data.m_arBaseFile.GetSaveParams();
            saveParams.unicodeType = static_cast<CFileTextLines::UnicodeType>(cmd - ID_INDICATOR_LEFTENCODINGSTART);
            if (AdjustUnicodeTypeForLoad(saveParams.unicodeType))
            {
                m_data.m_arBaseFile.SetSaveParams(saveParams);
                m_data.m_arBaseFile.KeepEncoding();
                LoadViews();
            }
        }
        else
        {
            m_pwndLeftView->SetTextType(static_cast<CFileTextLines::UnicodeType>(cmd - ID_INDICATOR_LEFTENCODINGSTART));
            m_pwndLeftView->RefreshViews();
        }
    }
}

void CMainFrame::OnEncodingRight(UINT cmd)
{
    if (m_pwndRightView)
    {
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            // reload with selected encoding
            auto saveParams        = m_data.m_arYourFile.GetSaveParams();
            saveParams.unicodeType = static_cast<CFileTextLines::UnicodeType>(cmd - ID_INDICATOR_RIGHTENCODINGSTART);
            if (AdjustUnicodeTypeForLoad(saveParams.unicodeType))
            {
                m_data.m_arYourFile.SetSaveParams(saveParams);
                m_data.m_arYourFile.KeepEncoding();
                LoadViews();
            }
        }
        else
        {
            m_pwndRightView->SetTextType(static_cast<CFileTextLines::UnicodeType>(cmd - ID_INDICATOR_RIGHTENCODINGSTART));
            m_pwndRightView->RefreshViews();
        }
    }
}

void CMainFrame::OnEncodingBottom(UINT cmd)
{
    if (m_pwndBottomView)
    {
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
            // reload with selected encoding
            auto saveParams        = m_data.m_arTheirFile.GetSaveParams();
            saveParams.unicodeType = static_cast<CFileTextLines::UnicodeType>(cmd - ID_INDICATOR_BOTTOMENCODINGSTART);
            if (AdjustUnicodeTypeForLoad(saveParams.unicodeType))
            {
                m_data.m_arTheirFile.SetSaveParams(saveParams);
                m_data.m_arTheirFile.KeepEncoding();
                LoadViews();
            }
        }
        else
        {
            m_pwndBottomView->SetTextType(static_cast<CFileTextLines::UnicodeType>(cmd - ID_INDICATOR_BOTTOMENCODINGSTART));
            m_pwndBottomView->RefreshViews();
        }
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEOLLeft(UINT cmd)
{
    if (m_pwndLeftView)
    {
        m_pwndLeftView->ReplaceLineEndings(static_cast<EOL>(cmd - ID_INDICATOR_LEFTEOLSTART));
        m_pwndLeftView->RefreshViews();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEOLRight(UINT cmd)
{
    if (m_pwndRightView)
    {
        m_pwndRightView->ReplaceLineEndings(static_cast<EOL>(cmd - ID_INDICATOR_RIGHTEOLSTART));
        m_pwndRightView->RefreshViews();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnEOLBottom(UINT cmd)
{
    if (m_pwndBottomView)
    {
        m_pwndBottomView->ReplaceLineEndings(static_cast<EOL>(cmd - ID_INDICATOR_BOTTOMEOLSTART));
        m_pwndBottomView->RefreshViews();
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnTabModeLeft(UINT cmd)
{
    OnTabMode(m_pwndLeftView, static_cast<int>(cmd) - ID_INDICATOR_LEFTTABMODESTART);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnTabModeRight(UINT cmd)
{
    OnTabMode(m_pwndRightView, static_cast<int>(cmd) - ID_INDICATOR_RIGHTTABMODESTART);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnTabModeBottom(UINT cmd)
{
    OnTabMode(m_pwndBottomView, static_cast<int>(cmd) - ID_INDICATOR_BOTTOMTABMODESTART);
}

void CMainFrame::OnTabMode(CBaseView *view, int cmd)
{
    if (!view)
        return;
    int nTabMode = view->GetTabMode();
    if (cmd == TABMODE_NONE || cmd == TABMODE_USESPACES)
        view->SetTabMode((nTabMode & (~TABMODE_USESPACES)) | (cmd & TABMODE_USESPACES));
    else if (cmd == TABMODE_SMARTINDENT) // Toggle
        view->SetTabMode((nTabMode & (~TABMODE_SMARTINDENT)) | ((nTabMode & TABMODE_SMARTINDENT) ? 0 : TABMODE_SMARTINDENT));
    else if (cmd == TABSIZEBUTTON1)
        view->SetTabSize(1);
    else if (cmd == TABSIZEBUTTON2)
        view->SetTabSize(2);
    else if (cmd == TABSIZEBUTTON4)
        view->SetTabSize(4);
    else if (cmd == TABSIZEBUTTON8)
        view->SetTabSize(8);
    else if (cmd == ENABLEEDITORCONFIG)
        view->SetEditorConfigEnabled(!view->GetEditorConfigEnabled());
    view->RefreshViews();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEncodingLeft(CCmdUI *pCmdUI)
{
    if (m_pwndLeftView)
    {
        pCmdUI->SetCheck(static_cast<CFileTextLines::UnicodeType>(pCmdUI->m_nID - ID_INDICATOR_LEFTENCODINGSTART) == m_pwndLeftView->GetTextType());
        pCmdUI->Enable(m_pwndLeftView->IsWritable() || (GetKeyState(VK_CONTROL) & 0x8000));
    }
    else
        pCmdUI->Enable(FALSE);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEncodingRight(CCmdUI *pCmdUI)
{
    if (m_pwndRightView)
    {
        pCmdUI->SetCheck(static_cast<CFileTextLines::UnicodeType>(pCmdUI->m_nID - ID_INDICATOR_RIGHTENCODINGSTART) == m_pwndRightView->GetTextType());
        pCmdUI->Enable(m_pwndRightView->IsWritable() || (GetKeyState(VK_CONTROL) & 0x8000));
    }
    else
        pCmdUI->Enable(FALSE);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEncodingBottom(CCmdUI *pCmdUI)
{
    if (m_pwndBottomView)
    {
        pCmdUI->SetCheck(static_cast<CFileTextLines::UnicodeType>(pCmdUI->m_nID - ID_INDICATOR_BOTTOMENCODINGSTART) == m_pwndBottomView->GetTextType());
        pCmdUI->Enable(m_pwndBottomView->IsWritable() || (GetKeyState(VK_CONTROL) & 0x8000));
    }
    else
        pCmdUI->Enable(FALSE);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEOLLeft(CCmdUI *pCmdUI)
{
    if (m_pwndLeftView)
    {
        pCmdUI->SetCheck(static_cast<EOL>(pCmdUI->m_nID - ID_INDICATOR_LEFTEOLSTART) == m_pwndLeftView->GetLineEndings());
        pCmdUI->Enable(m_pwndLeftView->IsWritable());
    }
    else
        pCmdUI->Enable(FALSE);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEOLRight(CCmdUI *pCmdUI)
{
    if (m_pwndRightView)
    {
        pCmdUI->SetCheck(static_cast<EOL>(pCmdUI->m_nID - ID_INDICATOR_RIGHTEOLSTART) == m_pwndRightView->GetLineEndings());
        pCmdUI->Enable(m_pwndRightView->IsWritable());
    }
    else
        pCmdUI->Enable(FALSE);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEOLBottom(CCmdUI *pCmdUI)
{
    if (m_pwndBottomView)
    {
        pCmdUI->SetCheck(static_cast<EOL>(pCmdUI->m_nID - ID_INDICATOR_BOTTOMEOLSTART) == m_pwndBottomView->GetLineEndings());
        pCmdUI->Enable(m_pwndBottomView->IsWritable());
    }
    else
        pCmdUI->Enable(FALSE);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateTabModeLeft(CCmdUI *pCmdUI)
{
    OnUpdateTabMode(m_pwndLeftView, pCmdUI, ID_INDICATOR_LEFTTABMODESTART);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateTabModeRight(CCmdUI *pCmdUI)
{
    OnUpdateTabMode(m_pwndRightView, pCmdUI, ID_INDICATOR_RIGHTTABMODESTART);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateTabModeBottom(CCmdUI *pCmdUI)
{
    OnUpdateTabMode(m_pwndBottomView, pCmdUI, ID_INDICATOR_BOTTOMTABMODESTART);
}

void CMainFrame::OnUpdateTabMode(CBaseView *view, CCmdUI *pCmdUI, int startid)
{
    if (view)
    {
        int cmd = static_cast<int>(pCmdUI->m_nID) - startid;
        if (cmd == TABMODE_NONE)
            pCmdUI->SetCheck((view->GetTabMode() & TABMODE_USESPACES) == TABMODE_NONE);
        else if (cmd == TABMODE_USESPACES)
            pCmdUI->SetCheck(view->GetTabMode() & TABMODE_USESPACES);
        else if (cmd == TABMODE_SMARTINDENT)
            pCmdUI->SetCheck(view->GetTabMode() & TABMODE_SMARTINDENT);
        else if (cmd == TABSIZEBUTTON1)
            pCmdUI->SetCheck(view->GetTabSize() == 1);
        else if (cmd == TABSIZEBUTTON2)
            pCmdUI->SetCheck(view->GetTabSize() == 2);
        else if (cmd == TABSIZEBUTTON4)
            pCmdUI->SetCheck(view->GetTabSize() == 4);
        else if (cmd == TABSIZEBUTTON8)
            pCmdUI->SetCheck(view->GetTabSize() == 8);
        else if (cmd == ENABLEEDITORCONFIG)
            pCmdUI->SetCheck(view->GetEditorConfigEnabled());
        pCmdUI->Enable(view->IsWritable());
        if (cmd == ENABLEEDITORCONFIG)
            pCmdUI->Enable(view->IsWritable() && view->GetEditorConfigLoaded());
    }
    else
        pCmdUI->Enable(FALSE);
}

BOOL CMainFrame::OnShowPopupMenu(CMFCPopupMenu *pMenuPopup)
{
    __super::OnShowPopupMenu(pMenuPopup);

    if (!pMenuPopup)
        return TRUE;

    int iIndex = -1;
    if (!CMFCToolBar::IsCustomizeMode() &&
        (iIndex = pMenuPopup->GetMenuBar()->CommandToIndex(ID_REGEXFILTER)) >= 0)
    {
        BuildRegexSubitems(pMenuPopup);
    }

    return TRUE;
}

LRESULT CMainFrame::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam)
{
    __super::OnIdleUpdateCmdUI(wParam, lParam);

    if (m_pRibbonApp)
    {
        BOOL bDisableIfNoHandler = static_cast<BOOL>(wParam);
        m_pRibbonApp->UpdateCmdUI(bDisableIfNoHandler);
    }
    return 0;
}

LRESULT CMainFrame::OnDPIChanged(WPARAM, LPARAM lParam)
{
    CDPIAware::Instance().Invalidate();
    if (m_pwndLeftView)
        m_pwndLeftView->DPIChanged();
    if (m_pwndRightView)
        m_pwndRightView->DPIChanged();
    if (m_pwndBottomView)
        m_pwndBottomView->DPIChanged();
    const RECT *rect = reinterpret_cast<RECT *>(lParam);
    SetWindowPos(nullptr, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
    ::RedrawWindow(GetSafeHwnd(), nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
    return 1; // let MFC handle this message as well
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void CMainFrame::OnUpdateThreeWayActions(CCmdUI *pCmdUI)
{
    pCmdUI->Enable();
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateColumnStatusBar(CCmdUI *pCmdUI)
{
    int  column        = 0;
    auto pWndWithFocus = GetFocus();
    if (pWndWithFocus)
    {
        if (pWndWithFocus == m_pwndBottomView)
            column = m_pwndBottomView->GetCaretViewPosition().x;
        if (pWndWithFocus == m_pwndLeftView)
            column = m_pwndLeftView->GetCaretViewPosition().x;
        if (pWndWithFocus == m_pwndRightView)
            column = m_pwndRightView->GetCaretViewPosition().x;
    }
    CString sColumn;
    sColumn.Format(IDS_INDICATOR_COLUMN, column + 1);
    pCmdUI->SetText(sColumn);
    pCmdUI->Enable(true);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateMarkedWords(CCmdUI *pCmdUI)
{
    CString sText;
    CString sTmp;
    if (IsViewGood(m_pwndLeftView) && m_pwndLeftView->GetMarkedWordCount())
    {
        sTmp.Format(L"L: %d", m_pwndLeftView->GetMarkedWordCount());
        if (!sText.IsEmpty())
            sText += L" | ";
        sText += sTmp;
    }
    if (IsViewGood(m_pwndRightView) && m_pwndRightView->GetMarkedWordCount())
    {
        sTmp.Format(L"R: %d", m_pwndRightView->GetMarkedWordCount());
        if (!sText.IsEmpty())
            sText += L" | ";
        sText += sTmp;
    }
    if (IsViewGood(m_pwndBottomView) && m_pwndBottomView->GetMarkedWordCount())
    {
        sTmp.Format(L"L: %d", m_pwndBottomView->GetMarkedWordCount());
        if (!sText.IsEmpty())
            sText += L" | ";
        sText += sTmp;
    }
    if (!sText.IsEmpty())
    {
        CString sStatusBarText;
        sStatusBarText.Format(IDS_INDICATOR_MARKEDWORDCOUNT, static_cast<LPCWSTR>(sText));
        pCmdUI->SetText(sStatusBarText);
        pCmdUI->Enable(true);
    }
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateEnableIfSelection(CCmdUI *pCmdUI)
{
    bool bEnabled      = false;
    auto pWndWithFocus = GetFocus();
    if (pWndWithFocus)
    {
        if (pWndWithFocus == m_pwndBottomView)
            bEnabled = !m_pwndBottomView->GetSelectedText().IsEmpty();
        if (pWndWithFocus == m_pwndLeftView)
            bEnabled = !m_pwndLeftView->GetSelectedText().IsEmpty();
        if (pWndWithFocus == m_pwndRightView)
            bEnabled = !m_pwndRightView->GetSelectedText().IsEmpty();
    }
    pCmdUI->Enable(bEnabled);
}

void CMainFrame::OnRegexNoFilter()
{
    if (CheckForSave(CHFSR_OPTIONS) == IDCANCEL)
        return;
    m_data.SetRegexTokens(std::wregex(), L"");
    m_regexIndex = -1;
    LoadViews(-1);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void CMainFrame::OnUpdateRegexNoFilter(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_regexIndex < 0);
}

void CMainFrame::OnSettingChange(UINT uFlags, LPCWSTR lpszSection)
{
    __super::OnSettingChange(uFlags, lpszSection);

    SetAccentColor();
}

void CMainFrame::OnSysColorChange()
{
    __super::OnSysColorChange();

    CTheme::Instance().OnSysColorChanged();
    CTheme::Instance().SetDarkTheme(CTheme::Instance().IsDarkTheme(), true);
    SetAccentColor();
}

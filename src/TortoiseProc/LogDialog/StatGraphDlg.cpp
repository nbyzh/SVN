﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2021-2022, 2024 - TortoiseSVN

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
#include "StatGraphDlg.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "PathUtils.h"
#include "registry.h"
#include "FormatMessageWrapper.h"

#include <cmath>
#include <locale>
#include <list>
#include <utility>
#include <strsafe.h>
#pragma warning(push)
#pragma warning(disable : 4458) // declaration of 'xxx' hides class member
#include <gdiplus.h>
#pragma warning(pop)

using namespace Gdiplus;

// BinaryPredicate for comparing authors based on their commit count
template <class DataType>
class MoreCommitsThan
{
public:
    MoreCommitsThan(std::map<std::wstring, DataType>& authorCommits)
        : m_authorCommits(authorCommits)
    {
    }

    bool operator()(const std::wstring& lhs, const std::wstring& rhs) const
    {
        return (m_authorCommits)[lhs] > (m_authorCommits)[rhs];
    }

private:
    std::map<std::wstring, DataType>& m_authorCommits;
};

IMPLEMENT_DYNAMIC(CStatGraphDlg, CResizableStandAloneDialog)
CStatGraphDlg::CStatGraphDlg(CWnd* pParent /*=NULL*/)
    : CResizableStandAloneDialog(CStatGraphDlg::IDD, pParent)
    , m_bAuthorsCaseSensitive(TRUE)
    , m_bSortByCommitCount(TRUE)
    , m_graphType(MyGraph::Bar)
    , m_bStacked(FALSE)
    , m_langOrder(0)
    , m_nDays(-1)
    , m_nWeeks(-1)
    , m_minDate(0)
    , m_maxDate(0)
    , m_nTotalCommits(0)
    , m_nTotalFileChanges(0)
    , m_firstInterval(0)
    , m_lastInterval(0)
{
    m_parDates       = nullptr;
    m_parFileChanges = nullptr;
    m_parAuthors     = nullptr;
    m_pToolTip       = nullptr;
}

CStatGraphDlg::~CStatGraphDlg()
{
    ClearGraph();
    delete m_pToolTip;
}

void CStatGraphDlg::OnOK()
{
    StoreCurrentGraphType();
    __super::OnOK();
}

void CStatGraphDlg::OnCancel()
{
    StoreCurrentGraphType();
    __super::OnCancel();
}

void CStatGraphDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableStandAloneDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_GRAPH, m_graph);
    DDX_Control(pDX, IDC_GRAPHCOMBO, m_cGraphType);
    DDX_Control(pDX, IDC_SKIPPER, m_skipper);
    DDX_Check(pDX, IDC_AUTHORSCASESENSITIVE, m_bAuthorsCaseSensitive);
    DDX_Check(pDX, IDC_SORTBYCOMMITCOUNT, m_bSortByCommitCount);
    DDX_Control(pDX, IDC_GRAPHBARBUTTON, m_btnGraphBar);
    DDX_Control(pDX, IDC_GRAPHBARSTACKEDBUTTON, m_btnGraphBarStacked);
    DDX_Control(pDX, IDC_GRAPHLINEBUTTON, m_btnGraphLine);
    DDX_Control(pDX, IDC_GRAPHLINESTACKEDBUTTON, m_btnGraphLineStacked);
    DDX_Control(pDX, IDC_GRAPHPIEBUTTON, m_btnGraphPie);
}

BEGIN_MESSAGE_MAP(CStatGraphDlg, CResizableStandAloneDialog)
    ON_CBN_SELCHANGE(IDC_GRAPHCOMBO, OnCbnSelchangeGraphcombo)
    ON_WM_HSCROLL()
    ON_NOTIFY(TTN_NEEDTEXT, NULL, OnNeedText)
    ON_BN_CLICKED(IDC_AUTHORSCASESENSITIVE, &CStatGraphDlg::AuthorsCaseSensitiveChanged)
    ON_BN_CLICKED(IDC_SORTBYCOMMITCOUNT, &CStatGraphDlg::SortModeChanged)
    ON_BN_CLICKED(IDC_GRAPHBARBUTTON, &CStatGraphDlg::OnBnClickedGraphbarbutton)
    ON_BN_CLICKED(IDC_GRAPHBARSTACKEDBUTTON, &CStatGraphDlg::OnBnClickedGraphbarstackedbutton)
    ON_BN_CLICKED(IDC_GRAPHLINEBUTTON, &CStatGraphDlg::OnBnClickedGraphlinebutton)
    ON_BN_CLICKED(IDC_GRAPHLINESTACKEDBUTTON, &CStatGraphDlg::OnBnClickedGraphlinestackedbutton)
    ON_BN_CLICKED(IDC_GRAPHPIEBUTTON, &CStatGraphDlg::OnBnClickedGraphpiebutton)
    ON_COMMAND(ID_FILE_SAVESTATGRAPHAS, &CStatGraphDlg::OnFileSavestatgraphas)
END_MESSAGE_MAP()

void CStatGraphDlg::LoadStatQueries(__in UINT curStr, Metrics loadMetric, bool setDef /* = false */)
{
    CString temp;
    temp.LoadString(curStr);
    int sel = m_cGraphType.AddString(temp);
    m_cGraphType.SetItemData(sel, loadMetric);

    if (setDef)
        m_cGraphType.SetCurSel(sel);
}

void CStatGraphDlg::SetSkipper(bool reloadSkiper)
{
    // We need to limit the number of authors due to GUI resource limitation.
    // However, since author #251 will properly have < 1000th of the commits,
    // the resolution limit of the screen will already not allow for displaying
    // it in a reasonable way

    int maxAuthorsCount = std::max(1, std::min(static_cast<int>(m_authorNames.size()), 250));
    m_skipper.SetRange(1, maxAuthorsCount);
    m_skipper.SetPageSize(5);

    if (reloadSkiper)
        m_skipper.SetPos(maxAuthorsCount);
}

BOOL CStatGraphDlg::OnInitDialog()
{
    CResizableStandAloneDialog::OnInitDialog();
    CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

    m_pToolTip = new CToolTipCtrl;
    if (m_pToolTip->Create(this))
    {
        m_pToolTip->AddTool(&m_btnGraphPie, IDS_STATGRAPH_PIEBUTTON_TT);
        m_pToolTip->AddTool(&m_btnGraphLineStacked, IDS_STATGRAPH_LINESTACKEDBUTTON_TT);
        m_pToolTip->AddTool(&m_btnGraphLine, IDS_STATGRAPH_LINEBUTTON_TT);
        m_pToolTip->AddTool(&m_btnGraphBarStacked, IDS_STATGRAPH_BARSTACKEDBUTTON_TT);
        m_pToolTip->AddTool(&m_btnGraphBar, IDS_STATGRAPH_BARBUTTON_TT);

        m_pToolTip->Activate(TRUE);
    }

    m_bAuthorsCaseSensitive = static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\StatAuthorsCaseSensitive"));
    m_bSortByCommitCount    = static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseSVN\\StatSortByCommitCount"));
    UpdateData(FALSE);

    //Load statistical queries
    LoadStatQueries(IDS_STATGRAPH_STATS, AllStat, true);
    LoadStatQueries(IDS_STATGRAPH_COMMITSBYDATE, CommitsByDate);
    LoadStatQueries(IDS_STATGRAPH_COMMITSBYAUTHOR, CommitsByAuthor);
    LoadStatQueries(IDS_STATGRAPH_PERCENTAGE_OF_AUTHORSHIP, PercentageOfAuthorship);

    // set the dialog title to "Statistics - path/to/whatever/we/show/the/statistics/for"
    CString sTitle;
    GetWindowText(sTitle);
    CAppUtils::SetWindowTitle(m_hWnd, m_path.GetUIPathString(), sTitle);

    int iconWidth  = GetSystemMetrics(SM_CXSMICON);
    int iconHeight = GetSystemMetrics(SM_CYSMICON);
    m_btnGraphBar.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHBAR, iconWidth, iconHeight));
    m_btnGraphBar.SizeToContent();
    m_btnGraphBar.Invalidate();
    m_btnGraphBarStacked.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHBARSTACKED, iconWidth, iconHeight));
    m_btnGraphBarStacked.SizeToContent();
    m_btnGraphBarStacked.Invalidate();
    m_btnGraphLine.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHLINE, iconWidth, iconHeight));
    m_btnGraphLine.SizeToContent();
    m_btnGraphLine.Invalidate();
    m_btnGraphLineStacked.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHLINESTACKED, iconWidth, iconHeight));
    m_btnGraphLineStacked.SizeToContent();
    m_btnGraphLineStacked.Invalidate();
    m_btnGraphPie.SetImage(CCommonAppUtils::LoadIconEx(IDI_GRAPHPIE, iconWidth, iconHeight));
    m_btnGraphPie.SizeToContent();
    m_btnGraphPie.Invalidate();

    AddAnchor(IDC_GRAPHTYPELABEL, TOP_LEFT);
    AddAnchor(IDC_GRAPH, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_GRAPHCOMBO, TOP_LEFT, TOP_RIGHT);

    AddAnchor(IDC_NUMWEEK, TOP_LEFT);
    AddAnchor(IDC_NUMWEEKVALUE, TOP_RIGHT);
    AddAnchor(IDC_NUMAUTHOR, TOP_LEFT);
    AddAnchor(IDC_NUMAUTHORVALUE, TOP_RIGHT);
    AddAnchor(IDC_NUMCOMMITS, TOP_LEFT);
    AddAnchor(IDC_NUMCOMMITSVALUE, TOP_RIGHT);
    AddAnchor(IDC_NUMFILECHANGES, TOP_LEFT);
    AddAnchor(IDC_NUMFILECHANGESVALUE, TOP_RIGHT);

    AddAnchor(IDC_AVG, TOP_RIGHT);
    AddAnchor(IDC_MIN, TOP_RIGHT);
    AddAnchor(IDC_MAX, TOP_RIGHT);
    AddAnchor(IDC_COMMITSEACHWEEK, TOP_LEFT);
    AddAnchor(IDC_MOSTACTIVEAUTHOR, TOP_LEFT);
    AddAnchor(IDC_LEASTACTIVEAUTHOR, TOP_LEFT);
    AddAnchor(IDC_MOSTACTIVEAUTHORNAME, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_LEASTACTIVEAUTHORNAME, TOP_LEFT, TOP_RIGHT);
    AddAnchor(IDC_FILECHANGESEACHWEEK, TOP_LEFT);
    AddAnchor(IDC_COMMITSEACHWEEKAVG, TOP_RIGHT);
    AddAnchor(IDC_COMMITSEACHWEEKMIN, TOP_RIGHT);
    AddAnchor(IDC_COMMITSEACHWEEKMAX, TOP_RIGHT);
    AddAnchor(IDC_MOSTACTIVEAUTHORAVG, TOP_RIGHT);
    AddAnchor(IDC_MOSTACTIVEAUTHORMIN, TOP_RIGHT);
    AddAnchor(IDC_MOSTACTIVEAUTHORMAX, TOP_RIGHT);
    AddAnchor(IDC_LEASTACTIVEAUTHORAVG, TOP_RIGHT);
    AddAnchor(IDC_LEASTACTIVEAUTHORMIN, TOP_RIGHT);
    AddAnchor(IDC_LEASTACTIVEAUTHORMAX, TOP_RIGHT);
    AddAnchor(IDC_FILECHANGESEACHWEEKAVG, TOP_RIGHT);
    AddAnchor(IDC_FILECHANGESEACHWEEKMIN, TOP_RIGHT);
    AddAnchor(IDC_FILECHANGESEACHWEEKMAX, TOP_RIGHT);

    AddAnchor(IDC_GRAPHBARBUTTON, BOTTOM_RIGHT);
    AddAnchor(IDC_GRAPHBARSTACKEDBUTTON, BOTTOM_RIGHT);
    AddAnchor(IDC_GRAPHLINEBUTTON, BOTTOM_RIGHT);
    AddAnchor(IDC_GRAPHLINESTACKEDBUTTON, BOTTOM_RIGHT);
    AddAnchor(IDC_GRAPHPIEBUTTON, BOTTOM_RIGHT);

    AddAnchor(IDC_AUTHORSCASESENSITIVE, BOTTOM_LEFT);
    AddAnchor(IDC_SORTBYCOMMITCOUNT, BOTTOM_LEFT);
    AddAnchor(IDC_SKIPPER, BOTTOM_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_SKIPPERLABEL, BOTTOM_LEFT);
    AddAnchor(IDOK, BOTTOM_RIGHT);
    EnableSaveRestore(L"StatGraphDlg");

    // gather statistics data, only needs to be updated when the checkbox with
    // the case sensitivity of author names is changed
    GatherData();

    // set the min/max values on the skipper
    SetSkipper(true);

    // we use a stats page encoding here, 0 stands for the statistics dialog
    CRegDWORD lastStatsPage = CRegDWORD(L"Software\\TortoiseSVN\\LastViewedStatsPage", 0);

    // open last viewed statistics page as first page
    int graphType = lastStatsPage / 10;
    graphType     = max(1, min(3, graphType));
    m_cGraphType.SetCurSel(graphType - 1);

    OnCbnSelchangeGraphcombo();

    int statsPage = lastStatsPage % 10;
    switch (statsPage)
    {
        case 1:
            m_graphType = MyGraph::Bar;
            m_bStacked  = true;
            break;
        case 2:
            m_graphType = MyGraph::Bar;
            m_bStacked  = false;
            break;
        case 3:
            m_graphType = MyGraph::Line;
            m_bStacked  = true;
            break;
        case 4:
            m_graphType = MyGraph::Line;
            m_bStacked  = false;
            break;
        case 5:
            m_graphType = MyGraph::PieChart;
            break;

        default:
            return TRUE;
    }

    LCID locale           = MAKELCID(static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))), SORT_DEFAULT);
    bool bUseSystemLocale = !!static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseSVN\\UseSystemLocaleForDates", TRUE));
    locale                = bUseSystemLocale ? MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT) : locale;

    wchar_t l[2] = {0};
    GetLocaleInfo(locale, LOCALE_IDATE, l, sizeof(wchar_t));

    m_langOrder = (l[0] > 0) ? l[0] - '0' : -1;

    RedrawGraph();

    return TRUE;
}

void CStatGraphDlg::ShowLabels(BOOL bShow)
{
    if ((m_parAuthors == nullptr) || (m_parDates == nullptr) || (m_parFileChanges == nullptr))
        return;

    int nCmdShow = bShow ? SW_SHOW : SW_HIDE;

    GetDlgItem(IDC_GRAPH)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
    GetDlgItem(IDC_NUMWEEK)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_NUMWEEKVALUE)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_NUMAUTHOR)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_NUMAUTHORVALUE)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_NUMCOMMITS)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_NUMCOMMITSVALUE)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_NUMFILECHANGES)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_NUMFILECHANGESVALUE)->ShowWindow(nCmdShow);

    GetDlgItem(IDC_AVG)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_MIN)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_MAX)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_COMMITSEACHWEEK)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_MOSTACTIVEAUTHOR)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_LEASTACTIVEAUTHOR)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_MOSTACTIVEAUTHORNAME)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_LEASTACTIVEAUTHORNAME)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_FILECHANGESEACHWEEK)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_COMMITSEACHWEEKAVG)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_COMMITSEACHWEEKMIN)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_COMMITSEACHWEEKMAX)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_MOSTACTIVEAUTHORAVG)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_MOSTACTIVEAUTHORMIN)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_MOSTACTIVEAUTHORMAX)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_LEASTACTIVEAUTHORAVG)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_LEASTACTIVEAUTHORMIN)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_LEASTACTIVEAUTHORMAX)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_FILECHANGESEACHWEEKAVG)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_FILECHANGESEACHWEEKMIN)->ShowWindow(nCmdShow);
    GetDlgItem(IDC_FILECHANGESEACHWEEKMAX)->ShowWindow(nCmdShow);

    GetDlgItem(IDC_SORTBYCOMMITCOUNT)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
    GetDlgItem(IDC_SKIPPER)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
    GetDlgItem(IDC_SKIPPERLABEL)->ShowWindow(bShow ? SW_HIDE : SW_SHOW);
    m_btnGraphBar.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
    m_btnGraphBarStacked.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
    m_btnGraphLine.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
    m_btnGraphLineStacked.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
    m_btnGraphPie.ShowWindow(bShow ? SW_HIDE : SW_SHOW);
}

void CStatGraphDlg::UpdateWeekCount()
{
    // Sanity check
    if ((!m_parDates) || (m_parDates->IsEmpty()))
        return;

    // Already updated? No need to do it again.
    if (m_nWeeks >= 0)
        return;

    // Determine first and last date in dates array
    __time64_t minDate = static_cast<__time64_t>(m_parDates->GetAt(0));
    __time64_t maxDate = minDate;
    INT_PTR    count   = m_parDates->GetCount();
    for (INT_PTR i = 0; i < count; ++i)
    {
        __time64_t d = static_cast<__time64_t>(m_parDates->GetAt(i));
        if (d < minDate)
            minDate = d;
        else if (d > maxDate)
            maxDate = d;
    }

    // Store start date of the interval in the member variable m_minDate
    m_minDate = minDate;
    m_maxDate = maxDate;

    // How many weeks does the time period cover?

    // Get time difference between start and end date
    double secs = _difftime64(maxDate, m_minDate);

    m_nWeeks = static_cast<int>(ceil(secs / static_cast<double>(SECONDS_IN_WEEK)));
    m_nDays  = static_cast<int>(ceil(secs / static_cast<double>(SECONDS_IN_DAY)));
}

int CStatGraphDlg::GetCalendarWeek(const CTime& time)
{
    // Note:
    // the calculation of the calendar week is wrong if DST is in effect
    // and the date to calculate the week for is in DST and within the range
    // of the DST offset (e.g. one hour).
    // For example, if DST starts on Sunday march 30 and the date to get the week for
    // is Monday, march 31, 0:30:00, then the returned week is one week less than
    // the real week.
    // TODO: ?
    // write a function
    // getDSTOffset(const CTime& time)
    // which returns the DST offset for a given time/date. Then we can use this offset
    // to correct our GetDays() calculation to get the correct week again
    // This of course won't work for 'history' dates, because Windows doesn't have
    // that information (only Vista has such a function: GetTimeZoneInformationForYear() )
    int iWeekOfYear = 0;

    int     iYear            = time.GetYear();
    int     iFirstDayOfWeek  = 0;
    int     iFirstWeekOfYear = 0;
    wchar_t loc[2]           = {0};
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTDAYOFWEEK, loc, _countof(loc));
    iFirstDayOfWeek = static_cast<int>(loc[0] - '0');
    GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IFIRSTWEEKOFYEAR, loc, _countof(loc));
    iFirstWeekOfYear = static_cast<int>(loc[0] - '0');
    CTime dDateFirstJanuary(iYear, 1, 1, 0, 0, 0);
    int   iDayOfWeek = (dDateFirstJanuary.GetDayOfWeek() + 5 + iFirstDayOfWeek) % 7;

    // Select mode
    // 0 Week containing 1/1 is the first week of that year.
    // 1 First full week following 1/1 is the first week of that year.
    // 2 First week containing at least four days is the first week of that year.
    switch (iFirstWeekOfYear)
    {
        case 0:
        {
            // Week containing 1/1 is the first week of that year.

            // check if this week reaches into the next year
            dDateFirstJanuary = CTime(iYear + 1, 1, 1, 0, 0, 0);

            // Get start of week
            try
            {
                iDayOfWeek = (time.GetDayOfWeek() + 5 + iFirstDayOfWeek) % 7;
            }
            catch (CAtlException)
            {
            }
            CTime dStartOfWeek = time - CTimeSpan(iDayOfWeek, 0, 0, 0);

            // If this week spans over to 1/1 this is week 1
            if (dStartOfWeek + CTimeSpan(6, 0, 0, 0) >= dDateFirstJanuary)
            {
                // we are in the last week of the year that spans over 1/1
                iWeekOfYear = 1;
            }
            else
            {
                // Get week day of 1/1
                dDateFirstJanuary = CTime(iYear, 1, 1, 0, 0, 0);
                iDayOfWeek        = (dDateFirstJanuary.GetDayOfWeek() + 5 + iFirstDayOfWeek) % 7;
                // Just count from 1/1
                iWeekOfYear = static_cast<int>(((time - dDateFirstJanuary).GetDays() + iDayOfWeek) / 7) + 1;
            }
        }
        break;
        case 1:
        {
            // First full week following 1/1 is the first week of that year.

            // If the 1.1 is the start of the week everything is ok
            // else we need the next week is the correct result
            iWeekOfYear =
                static_cast<int>(((time - dDateFirstJanuary).GetDays() + iDayOfWeek) / 7) +
                (iDayOfWeek == 0 ? 1 : 0);

            // If we are in week 0 we are in the first not full week
            // calculate from the last year
            if (iWeekOfYear == 0)
            {
                // Special case: we are in the week of 1.1 but 1.1. is not on the
                // start of week. Calculate based on the last year
                dDateFirstJanuary = CTime(iYear - 1, 1, 1, 0, 0, 0);
                iDayOfWeek =
                    (dDateFirstJanuary.GetDayOfWeek() + 5 + iFirstDayOfWeek) % 7;
                // and we correct this in the same way we done this before but
                // the result is now 52 or 53 and not 0
                iWeekOfYear =
                    static_cast<int>(((time - dDateFirstJanuary).GetDays() + iDayOfWeek) / 7) +
                    (iDayOfWeek <= 3 ? 1 : 0);
            }
        }
        break;
        case 2:
        {
            // First week containing at least four days is the first week of that year.

            // Each year can start with any day of the week. But our
            // weeks always start with Monday. So we add the day of week
            // before calculation of the final week of year.
            // Rule: is the 1.1 a Mo,Tu,We,Th than the week starts on the 1.1 with
            // week==1, else a week later, so we add one for all those days if
            // day is less <=3 Mo,Tu,We,Th. Otherwise 1.1 is in the last week of the
            // previous year
            iWeekOfYear =
                static_cast<int>(((time - dDateFirstJanuary).GetDays() + iDayOfWeek) / 7) +
                (iDayOfWeek <= 3 ? 1 : 0);

            // special cases
            if (iWeekOfYear == 0)
            {
                // special case week 0. We got a day before the 1.1, 2.1 or 3.1, were the
                // 1.1. is not a Mo, Tu, We, Th. So the week 1 does not start with the 1.1.
                // So we calculate the week according to the 1.1 of the year before

                dDateFirstJanuary = CTime(iYear - 1, 1, 1, 0, 0, 0);
                iDayOfWeek =
                    (dDateFirstJanuary.GetDayOfWeek() + 5 + iFirstDayOfWeek) % 7;
                // and we correct this in the same way we done this before but the result
                // is now 52 or 53 and not 0
                iWeekOfYear =
                    static_cast<int>(((time - dDateFirstJanuary).GetDays() + iDayOfWeek) / 7) +
                    (iDayOfWeek <= 3 ? 1 : 0);
            }
            else if (iWeekOfYear == 53)
            {
                // special case week 53. Either we got the correct week 53 or we just got the
                // week 1 of the next year. So is the 1.1.(year+1) also a Mo, Tu, We, Th than
                // we already have the week 1, otherwise week 53 is correct

                dDateFirstJanuary = CTime(iYear + 1, 1, 1, 0, 0, 0);
                iDayOfWeek =
                    (dDateFirstJanuary.GetDayOfWeek() + 5 + iFirstDayOfWeek) % 7;
                // 1.1. in week 1 or week 53?
                iWeekOfYear = iDayOfWeek <= 3 ? 1 : 53;
            }
        }
        break;
        default:
            ASSERT(FALSE);
            break;
    }
    // return result
    return iWeekOfYear;
}

void CStatGraphDlg::GatherData()
{
    // Sanity check
    if ((m_parAuthors == nullptr) || (m_parDates == nullptr) || (m_parFileChanges == nullptr))
        return;
    m_nTotalCommits     = m_parAuthors->GetCount();
    m_nTotalFileChanges = 0;

    // Update m_nWeeks and m_minDate
    UpdateWeekCount();

    // Now create a mapping that holds the information per week.
    m_commitsPerUnitAndAuthor.clear();
    m_filechangesPerUnitAndAuthor.clear();
    m_commitsPerAuthor.clear();
    m_unitNames.clear();
    m_percentageOfAuthorship.clear();

    int        interval              = 0;
    __time64_t d                     = static_cast<__time64_t>(m_parDates->GetAt(0));
    int        nLastUnit             = GetUnit(d);
    double     allContributionAuthor = 0;

    // Now loop over all weeks and gather the info
    for (LONG i = 0; i < m_nTotalCommits; ++i)
    {
        // Find the interval number
        __time64_t commitDate = static_cast<__time64_t>(m_parDates->GetAt(i));
        int        u          = GetUnit(commitDate);
        if (nLastUnit != u)
            interval++;
        nLastUnit = u;
        // Find the authors name
        CString sAuth = m_parAuthors->GetAt(i);
        if (!m_bAuthorsCaseSensitive)
            sAuth = sAuth.MakeLower();
        std::wstring author = std::wstring(sAuth);
        // Increase total commit count for this author
        m_commitsPerAuthor[author]++;
        // Increase the commit count for this author in this week
        m_commitsPerUnitAndAuthor[interval][author]++;
        CTime t               = m_parDates->GetAt(i);
        m_unitNames[interval] = GetUnitLabel(nLastUnit, t);
        // Increase the file change count for this author in this week
        int fileChanges = m_parFileChanges->GetAt(i);
        m_filechangesPerUnitAndAuthor[interval][author] += fileChanges;
        m_nTotalFileChanges += fileChanges;

        //calculate Contribution Author
        double contributionAuthor = CoeffContribution(static_cast<int>(m_nTotalCommits) - i - 1) * fileChanges;
        allContributionAuthor += contributionAuthor;
        m_percentageOfAuthorship[author] += contributionAuthor;
    }

    // Find first and last interval number.
    if (!m_commitsPerUnitAndAuthor.empty())
    {
        auto intervalIt = m_commitsPerUnitAndAuthor.begin();
        m_firstInterval = intervalIt->first;
        intervalIt      = m_commitsPerUnitAndAuthor.end();
        --intervalIt;
        m_lastInterval = intervalIt->first;
        // Sanity check - if m_lastInterval is too large it could freeze TSVN and take up all memory!!!
        assert(m_lastInterval >= 0 && m_lastInterval < 10000);
    }
    else
    {
        m_firstInterval = 0;
        m_lastInterval  = -1;
    }

    // Get a list of authors names
    LoadListOfAuthors(m_commitsPerAuthor);

    // Calculate percent of Contribution Authors
    for (std::list<std::wstring>::iterator it = m_authorNames.begin(); it != m_authorNames.end(); ++it)
    {
        m_percentageOfAuthorship[*it] = (m_percentageOfAuthorship[*it] * 100) / allContributionAuthor;
    }

    // All done, now the statistics pages can retrieve the data and
    // extract the information to be shown.
}

void CStatGraphDlg::FilterSkippedAuthors(std::list<std::wstring>& includedAuthors,
                                         std::list<std::wstring>& skippedAuthors)
{
    includedAuthors.clear();
    skippedAuthors.clear();

    unsigned int includedAuthorsCount = m_skipper.GetPos();
    // if we only leave out one author, still include him with his name
    if (includedAuthorsCount + 1ULL == m_authorNames.size())
        ++includedAuthorsCount;

    // add the included authors first
    std::list<std::wstring>::iterator authorIt = m_authorNames.begin();
    while (includedAuthorsCount > 0 && authorIt != m_authorNames.end())
    {
        // Add him/her to the included list
        includedAuthors.push_back(*authorIt);
        ++authorIt;
        --includedAuthorsCount;
    }

    // If we haven't reached the end yet, copy all remaining authors into the
    // skipped author list.
    std::copy(authorIt, m_authorNames.end(), std::back_inserter(skippedAuthors));

    // Sort authors alphabetically if user wants that.
    if (!m_bSortByCommitCount)
        includedAuthors.sort();
}

bool CStatGraphDlg::PreViewStat(bool fShowLabels)
{
    if ((m_parAuthors == nullptr) || (m_parDates == nullptr) || (m_parFileChanges == nullptr))
        return false;
    ShowLabels(fShowLabels);

    //If view graphic
    if (!fShowLabels)
        ClearGraph();

    // This function relies on a previous call of GatherData().
    // This can be detected by checking the week count.
    // If the week count is equal to -1, it hasn't been called before.
    if (m_nWeeks == -1)
        GatherData();
    // If week count is still -1, something bad has happened, probably invalid data!
    if (m_nWeeks == -1)
        return false;

    return true;
}

MyGraphSeries* CStatGraphDlg::PreViewGraph(__in UINT graphTitle, __in UINT yAxisLabel, __in UINT xAxisLabel /*= NULL*/)
{
    if (!PreViewStat(false))
        return nullptr;

    // We need at least one author
    if (m_authorNames.empty())
        return nullptr;

    // Add a single series to the chart
    MyGraphSeries* graphData = new MyGraphSeries();
    m_graph.AddSeries(*graphData);
    m_graphDataArray.Add(graphData);

    // Set up the graph.
    CString temp;
    UpdateData();
    m_graph.SetGraphType(m_graphType, m_bStacked);
    temp.LoadString(yAxisLabel);
    m_graph.SetYAxisLabel(temp);
    temp.LoadString(xAxisLabel);
    m_graph.SetXAxisLabel(temp);
    temp.LoadString(graphTitle);
    m_graph.SetGraphTitle(temp);

    return graphData;
}

void CStatGraphDlg::ShowPercentageOfAuthorship()
{
    // Set up the graph.
    MyGraphSeries* graphData = PreViewGraph(IDS_STATGRAPH_PERCENTAGE_OF_AUTHORSHIP,
                                            IDS_STATGRAPH_PERCENTAGE_OF_AUTHORSHIPY,
                                            IDS_STATGRAPH_COMMITSBYAUTHORX);
    if (graphData == nullptr)
        return;

    // Find out which authors are to be shown and which are to be skipped.
    std::list<std::wstring> authors;
    std::list<std::wstring> others;

    FilterSkippedAuthors(authors, others);

    // Loop over all authors in the authors list and
    // add them to the graph.

    if (!authors.empty())
    {
        for (auto it = authors.begin(); it != authors.end(); ++it)
        {
            int group = m_graph.AppendGroup(it->c_str());
            graphData->SetData(group, RollPercentageOfAuthorship(m_percentageOfAuthorship[*it]));
        }
    }

    //     If we have other authors, count them and their commits.
    if (!others.empty())
        DrawOthers(others, graphData, m_percentageOfAuthorship);

    // Paint the graph now that we're through.
    m_graph.Invalidate();
}

void CStatGraphDlg::ShowCommitsByAuthor()
{
    // Set up the graph.
    MyGraphSeries* graphData = PreViewGraph(IDS_STATGRAPH_COMMITSBYAUTHOR,
                                            IDS_STATGRAPH_COMMITSBYAUTHORY,
                                            IDS_STATGRAPH_COMMITSBYAUTHORX);
    if (graphData == nullptr)
        return;

    // Find out which authors are to be shown and which are to be skipped.
    std::list<std::wstring> authors;
    std::list<std::wstring> others;
    FilterSkippedAuthors(authors, others);

    // Loop over all authors in the authors list and
    // add them to the graph.

    if (!authors.empty())
    {
        for (auto it = authors.begin(); it != authors.end(); ++it)
        {
            int group = m_graph.AppendGroup(it->c_str());
            graphData->SetData(group, m_commitsPerAuthor[*it]);
        }
    }

    //     If we have other authors, count them and their commits.
    if (!others.empty())
        DrawOthers(others, graphData, m_commitsPerAuthor);

    // Paint the graph now that we're through.
    m_graph.Invalidate();
}

void CStatGraphDlg::ShowCommitsByDate()
{
    if (!PreViewStat(false))
        return;

    // We need at least one author
    if (m_authorNames.empty())
        return;

    // Set up the graph.
    CString temp;
    UpdateData();
    m_graph.SetGraphType(m_graphType, m_bStacked);
    temp.LoadString(IDS_STATGRAPH_COMMITSBYDATEY);
    m_graph.SetYAxisLabel(temp);
    temp.LoadString(IDS_STATGRAPH_COMMITSBYDATE);
    m_graph.SetGraphTitle(temp);

    m_graph.SetXAxisLabel(GetUnitString());

    // Find out which authors are to be shown and which are to be skipped.
    std::list<std::wstring> authors;
    std::list<std::wstring> others;
    FilterSkippedAuthors(authors, others);

    // Add a graph series for each author.
    std::map<std::wstring, LONG> authorGraphMap;
    for (std::list<std::wstring>::iterator it = authors.begin(); it != authors.end(); ++it)
        authorGraphMap[*it] = m_graph.AppendGroup(it->c_str());
    // If we have skipped authors, add a graph series for all those.
    CString      sOthers(MAKEINTRESOURCE(IDS_STATGRAPH_OTHERGROUP));
    std::wstring othersName;
    if (!others.empty())
    {
        temp.Format(L" (%Iu)", others.size());
        sOthers += temp;
        othersName                 = static_cast<LPCWSTR>(sOthers);
        authorGraphMap[othersName] = m_graph.AppendGroup(sOthers);
    }

    // Mapping to collect commit counts in each interval
    std::map<std::wstring, LONG> commitCount;

    // Loop over all intervals/weeks and collect filtered data.
    // Sum up data in each interval until the time unit changes.
    for (int i = m_lastInterval; i >= m_firstInterval; --i)
    {
        // Collect data for authors listed by name.
        if (!authors.empty())
        {
            for (auto it = authors.begin(); it != authors.end(); ++it)
            {
                // Do we have some data for the current author in the current interval?
                auto dataIt = m_commitsPerUnitAndAuthor[i].find(*it);
                if (dataIt == m_commitsPerUnitAndAuthor[i].end())
                    continue;
                commitCount[*it] += dataIt->second;
            }
        }
        // Collect data for all skipped authors.
        if (!others.empty())
        {
            for (auto it = others.begin(); it != others.end(); ++it)
            {
                // Do we have some data for the author in the current interval?
                auto dataIt = m_commitsPerUnitAndAuthor[i].find(*it);
                if (dataIt == m_commitsPerUnitAndAuthor[i].end())
                    continue;
                commitCount[othersName] += dataIt->second;
            }
        }

        // Create a new data series for this unit/interval.
        MyGraphSeries* graphData = new MyGraphSeries();
        // Loop over all created graphs and set the corresponding data.
        if (!authorGraphMap.empty())
        {
            for (auto it = authorGraphMap.begin(); it != authorGraphMap.end(); ++it)
            {
                graphData->SetData(it->second, commitCount[it->first]);
            }
        }
        graphData->SetLabel(m_unitNames[i].c_str());
        m_graph.AddSeries(*graphData);
        m_graphDataArray.Add(graphData);

        // Reset commit count mapping.
        commitCount.clear();
    }

    // Paint the graph now that we're through.
    m_graph.Invalidate();
}

void CStatGraphDlg::ShowStats()
{
    if (!PreViewStat(true))
        return;

    // Now we can use the gathered data to update the stats dialog.
    size_t nAuthors = m_authorNames.size();

    // Find most and least active author names.
    std::wstring mostActiveAuthor;
    std::wstring leastActiveAuthor;
    if (nAuthors > 0)
    {
        mostActiveAuthor  = m_authorNames.front();
        leastActiveAuthor = m_authorNames.back();
    }

    // Obtain the statistics for the table.
    long nCommitsMin     = -1;
    long nCommitsMax     = -1;
    long nFileChangesMin = -1;
    long nFileChangesMax = -1;

    long nMostActiveMaxCommits  = -1;
    long nMostActiveMinCommits  = -1;
    long nLeastActiveMaxCommits = -1;
    long nLeastActiveMinCommits = -1;

    // Loop over all intervals and find min and max values for commit count and file changes.
    // Also store the stats for the most and least active authors.
    for (int i = m_firstInterval; i <= m_lastInterval; ++i)
    {
        // Loop over all commits in this interval and count the number of commits by all authors.
        int  commitCount = 0;
        auto commitEndit = m_commitsPerUnitAndAuthor[i].end();
        for (auto commitIt = m_commitsPerUnitAndAuthor[i].begin();
             commitIt != commitEndit; ++commitIt)
        {
            commitCount += commitIt->second;
        }
        if (nCommitsMin == -1 || commitCount < nCommitsMin)
            nCommitsMin = commitCount;
        if (nCommitsMax == -1 || commitCount > nCommitsMax)
            nCommitsMax = commitCount;

        // Loop over all commits in this interval and count the number of file changes by all authors.
        int  fileChangeCount = 0;
        auto filechangeEndit = m_filechangesPerUnitAndAuthor[i].end();
        for (auto filechangeIt = m_filechangesPerUnitAndAuthor[i].begin();
             filechangeIt != filechangeEndit; ++filechangeIt)
        {
            fileChangeCount += filechangeIt->second;
        }
        if (nFileChangesMin == -1 || fileChangeCount < nFileChangesMin)
            nFileChangesMin = fileChangeCount;
        if (nFileChangesMax == -1 || fileChangeCount > nFileChangesMax)
            nFileChangesMax = fileChangeCount;

        // also get min/max data for most and least active authors
        if (nAuthors > 0)
        {
            // check if author is present in this interval
            auto authorIt = m_commitsPerUnitAndAuthor[i].find(mostActiveAuthor);
            long authorCommits;
            if (authorIt == m_commitsPerUnitAndAuthor[i].end())
                authorCommits = 0;
            else
                authorCommits = authorIt->second;
            if (nMostActiveMaxCommits == -1 || authorCommits > nMostActiveMaxCommits)
                nMostActiveMaxCommits = authorCommits;
            if (nMostActiveMinCommits == -1 || authorCommits < nMostActiveMinCommits)
                nMostActiveMinCommits = authorCommits;

            authorIt = m_commitsPerUnitAndAuthor[i].find(leastActiveAuthor);
            if (authorIt == m_commitsPerUnitAndAuthor[i].end())
                authorCommits = 0;
            else
                authorCommits = authorIt->second;
            if (nLeastActiveMaxCommits == -1 || authorCommits > nLeastActiveMaxCommits)
                nLeastActiveMaxCommits = authorCommits;
            if (nLeastActiveMinCommits == -1 || authorCommits < nLeastActiveMinCommits)
                nLeastActiveMinCommits = authorCommits;
        }
    }
    if (nMostActiveMaxCommits == -1)
        nMostActiveMaxCommits = 0;
    if (nMostActiveMinCommits == -1)
        nMostActiveMinCommits = 0;
    if (nLeastActiveMaxCommits == -1)
        nLeastActiveMaxCommits = 0;
    if (nLeastActiveMinCommits == -1)
        nLeastActiveMinCommits = 0;

    int nWeeks = m_lastInterval - m_firstInterval;
    if (nWeeks == 0)
        nWeeks = 1;
    // Adjust the labels with the unit type (week, month, ...)
    CString labelText;
    labelText.Format(IDS_STATGRAPH_NUMBEROFUNIT, static_cast<LPCWSTR>(GetUnitsString()));
    SetDlgItemText(IDC_NUMWEEK, labelText);
    labelText.Format(IDS_STATGRAPH_COMMITSBYUNIT, static_cast<LPCWSTR>(GetUnitString()));
    SetDlgItemText(IDC_COMMITSEACHWEEK, labelText);
    labelText.Format(IDS_STATGRAPH_FILECHANGESBYUNIT, static_cast<LPCWSTR>(GetUnitString()));
    SetDlgItemText(IDC_FILECHANGESEACHWEEK, labelText);
    // We have now all data we want and we can fill in the labels...
    CString number;
    number.Format(L"%d", nWeeks);
    SetDlgItemText(IDC_NUMWEEKVALUE, number);
    number.Format(L"%Iu", nAuthors);
    SetDlgItemText(IDC_NUMAUTHORVALUE, number);
    number.Format(L"%Id", m_nTotalCommits);
    SetDlgItemText(IDC_NUMCOMMITSVALUE, number);
    number.Format(L"%ld", m_nTotalFileChanges);
    SetDlgItemText(IDC_NUMFILECHANGESVALUE, number);

    number.Format(L"%Id", m_parAuthors->GetCount() / nWeeks);
    SetDlgItemText(IDC_COMMITSEACHWEEKAVG, number);
    number.Format(L"%ld", nCommitsMax);
    SetDlgItemText(IDC_COMMITSEACHWEEKMAX, number);
    number.Format(L"%ld", nCommitsMin);
    SetDlgItemText(IDC_COMMITSEACHWEEKMIN, number);

    number.Format(L"%ld", m_nTotalFileChanges / nWeeks);
    SetDlgItemText(IDC_FILECHANGESEACHWEEKAVG, number);
    number.Format(L"%ld", nFileChangesMax);
    SetDlgItemText(IDC_FILECHANGESEACHWEEKMAX, number);
    number.Format(L"%ld", nFileChangesMin);
    SetDlgItemText(IDC_FILECHANGESEACHWEEKMIN, number);

    if (nAuthors == 0)
    {
        SetDlgItemText(IDC_MOSTACTIVEAUTHORNAME, L"");
        SetDlgItemText(IDC_MOSTACTIVEAUTHORAVG, L"0");
        SetDlgItemText(IDC_MOSTACTIVEAUTHORMAX, L"0");
        SetDlgItemText(IDC_MOSTACTIVEAUTHORMIN, L"0");
        SetDlgItemText(IDC_LEASTACTIVEAUTHORNAME, L"");
        SetDlgItemText(IDC_LEASTACTIVEAUTHORAVG, L"0");
        SetDlgItemText(IDC_LEASTACTIVEAUTHORMAX, L"0");
        SetDlgItemText(IDC_LEASTACTIVEAUTHORMIN, L"0");
    }
    else
    {
        SetDlgItemText(IDC_MOSTACTIVEAUTHORNAME, mostActiveAuthor.c_str());
        number.Format(L"%ld", m_commitsPerAuthor[mostActiveAuthor] / nWeeks);
        SetDlgItemText(IDC_MOSTACTIVEAUTHORAVG, number);
        number.Format(L"%ld", nMostActiveMaxCommits);
        SetDlgItemText(IDC_MOSTACTIVEAUTHORMAX, number);
        number.Format(L"%ld", nMostActiveMinCommits);
        SetDlgItemText(IDC_MOSTACTIVEAUTHORMIN, number);

        SetDlgItemText(IDC_LEASTACTIVEAUTHORNAME, leastActiveAuthor.c_str());
        number.Format(L"%ld", m_commitsPerAuthor[leastActiveAuthor] / nWeeks);
        SetDlgItemText(IDC_LEASTACTIVEAUTHORAVG, number);
        number.Format(L"%ld", nLeastActiveMaxCommits);
        SetDlgItemText(IDC_LEASTACTIVEAUTHORMAX, number);
        number.Format(L"%ld", nLeastActiveMinCommits);
        SetDlgItemText(IDC_LEASTACTIVEAUTHORMIN, number);
    }
}

int CStatGraphDlg::RollPercentageOfAuthorship(double it)
{
    return static_cast<int>(it) + (it - static_cast<int>(it) >= 0.5);
}

void CStatGraphDlg::OnCbnSelchangeGraphcombo()
{
    UpdateData();

    Metrics useMetric = static_cast<Metrics>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel()));
    switch (useMetric)
    {
        case AllStat:
        case CommitsByDate:
            // by date
            m_btnGraphLine.EnableWindow(TRUE);
            m_btnGraphLineStacked.EnableWindow(TRUE);
            m_btnGraphPie.EnableWindow(TRUE);
            m_graphType = MyGraph::Line;
            m_bStacked  = false;
            break;
        case PercentageOfAuthorship:
        case CommitsByAuthor:
            // by author
            m_btnGraphLine.EnableWindow(FALSE);
            m_btnGraphLineStacked.EnableWindow(FALSE);
            m_btnGraphPie.EnableWindow(TRUE);
            m_graphType = MyGraph::Bar;
            m_bStacked  = false;
            break;
        case TextStatStart:
            break;
        case TextStatEnd:
            break;
        case GraphicStatStart:
            break;
        case GraphicStatEnd:
            break;
        default:
            break;
    }
    RedrawGraph();
}

int CStatGraphDlg::GetUnitCount() const
{
    if (m_nDays < 8)
        return m_nDays;
    if (m_nWeeks < 15)
        return m_nWeeks;
    if (m_nWeeks < 80)
        return (m_nWeeks / 4) + 1;
    if (m_nWeeks < 320)
        return (m_nWeeks / 13) + 1; // quarters
    return (m_nWeeks / 52) + 1;
}

int CStatGraphDlg::GetUnit(const CTime& time) const
{
    if (m_nDays < 8)
        return time.GetMonth() * 100 + time.GetDay(); // month*100+day as the unit
    if (m_nWeeks < 15)
        return GetCalendarWeek(time);
    if (m_nWeeks < 80)
        return time.GetMonth();
    if (m_nWeeks < 320)
        return ((time.GetMonth() - 1) / 3) + 1; // quarters
    return time.GetYear();
}

CStatGraphDlg::UnitType CStatGraphDlg::GetUnitType() const
{
    if (m_nDays < 8)
        return Days;
    if (m_nWeeks < 15)
        return Weeks;
    if (m_nWeeks < 80)
        return Months;
    if (m_nWeeks < 320)
        return Quarters;
    return Years;
}

CString CStatGraphDlg::GetUnitString() const
{
    if (m_nDays < 8)
        return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXDAY));
    if (m_nWeeks < 15)
        return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXWEEK));
    if (m_nWeeks < 80)
        return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXMONTH));
    if (m_nWeeks < 320)
        return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXQUARTER));
    return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXYEAR));
}

CString CStatGraphDlg::GetUnitsString() const
{
    if (m_nDays < 8)
        return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXDAYS));
    if (m_nWeeks < 15)
        return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXWEEKS));
    if (m_nWeeks < 80)
        return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXMONTHS));
    if (m_nWeeks < 320)
        return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXQUARTERS));
    return CString(MAKEINTRESOURCE(IDS_STATGRAPH_COMMITSBYDATEXYEARS));
}

CString CStatGraphDlg::GetUnitLabel(int unit, CTime& lasttime) const
{
    CString temp;
    switch (GetUnitType())
    {
        case Days:
        {
            // month*100+day as the unit
            int day   = unit % 100;
            int month = unit / 100;
            switch (m_langOrder)
            {
                case 0: // month day year
                    temp.Format(L"%d/%d/%.2d", month, day, lasttime.GetYear() % 100);
                    break;
                case 1: // day month year
                default:
                    temp.Format(L"%d/%d/%.2d", day, month, lasttime.GetYear() % 100);
                    break;
                case 2: // year month day
                    temp.Format(L"%.2d/%d/%d", lasttime.GetYear() % 100, month, day);
                    break;
            }
        }
        break;
        case Weeks:
        {
            int year = lasttime.GetYear();
            if ((unit == 1) && (lasttime.GetMonth() == 12))
                year += 1;

            switch (m_langOrder)
            {
                case 0: // month day year
                case 1: // day month year
                default:
                    temp.Format(L"%d/%.2d", unit, year % 100);
                    break;
                case 2: // year month day
                    temp.Format(L"%.2d/%d", year % 100, unit);
                    break;
            }
        }
        break;
        case Months:
            switch (m_langOrder)
            {
                case 0: // month day year
                case 1: // day month year
                default:
                    temp.Format(L"%d/%.2d", unit, lasttime.GetYear() % 100);
                    break;
                case 2: // year month day
                    temp.Format(L"%.2d/%d", lasttime.GetYear() % 100, unit);
                    break;
            }
            break;
        case Quarters:
            switch (m_langOrder)
            {
                case 0: // month day year
                case 1: // day month year
                default:
                    temp.Format(L"%d/%.2d", unit, lasttime.GetYear() % 100);
                    break;
                case 2: // year month day
                    temp.Format(L"%.2d/%d", lasttime.GetYear() % 100, unit);
                    break;
            }
            break;
        case Years:
            temp.Format(L"%d", unit);
            break;
    }
    return temp;
}

void CStatGraphDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (nSBCode == TB_THUMBTRACK)
        return CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

    ShowSelectStat(static_cast<Metrics>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel())));
    CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CStatGraphDlg::OnNeedText(NMHDR* pnmh, LRESULT* /*pResult*/)
{
    TOOLTIPTEXT* pttt = reinterpret_cast<NMTTDISPINFOW*>(pnmh);
    if (pttt->hdr.idFrom == reinterpret_cast<UINT_PTR>(m_skipper.GetSafeHwnd()))
    {
        size_t includedAuthorsCount = m_skipper.GetPos();
        // if we only leave out one author, still include him with his name
        if (includedAuthorsCount + 1 == m_authorNames.size())
            ++includedAuthorsCount;

        // find the minimum number of commits that the shown authors have
        int minCommits                             = 0;
        includedAuthorsCount                       = min(includedAuthorsCount, m_authorNames.size());
        std::list<std::wstring>::iterator authorIt = m_authorNames.begin();
        advance(authorIt, includedAuthorsCount);
        if (authorIt != m_authorNames.begin())
            minCommits = m_commitsPerAuthor[*(--authorIt)];

        CString string;
        int     percentage = static_cast<int>(minCommits * 100.0 / (m_nTotalCommits ? m_nTotalCommits : 1));
        string.FormatMessage(IDS_STATGRAPH_AUTHORSLIDER_TT, m_skipper.GetPos(), minCommits, percentage);
        StringCchCopy(pttt->szText, _countof(pttt->szText), static_cast<LPCWSTR>(string));
    }
}

void CStatGraphDlg::AuthorsCaseSensitiveChanged()
{
    UpdateData();  // update checkbox state
    GatherData();  // first regenerate the statistics data
    RedrawGraph(); // then update the current statistics page
}

void CStatGraphDlg::SortModeChanged()
{
    UpdateData();  // update checkbox state
    RedrawGraph(); // then update the current statistics page
}

void CStatGraphDlg::ClearGraph()
{
    m_graph.Clear();
    for (int j = 0; j < m_graphDataArray.GetCount(); ++j)
        delete static_cast<MyGraphSeries*>(m_graphDataArray.GetAt(j));
    m_graphDataArray.RemoveAll();
}

void CStatGraphDlg::RedrawGraph()
{
    EnableDisableMenu();
    m_btnGraphBar.SetState(BST_UNCHECKED);
    m_btnGraphBarStacked.SetState(BST_UNCHECKED);
    m_btnGraphLine.SetState(BST_UNCHECKED);
    m_btnGraphLineStacked.SetState(BST_UNCHECKED);
    m_btnGraphPie.SetState(BST_UNCHECKED);

    if ((m_graphType == MyGraph::Bar) && (m_bStacked))
    {
        m_btnGraphBarStacked.SetState(BST_CHECKED);
    }
    if ((m_graphType == MyGraph::Bar) && (!m_bStacked))
    {
        m_btnGraphBar.SetState(BST_CHECKED);
    }
    if ((m_graphType == MyGraph::Line) && (m_bStacked))
    {
        m_btnGraphLineStacked.SetState(BST_CHECKED);
    }
    if ((m_graphType == MyGraph::Line) && (!m_bStacked))
    {
        m_btnGraphLine.SetState(BST_CHECKED);
    }
    if (m_graphType == MyGraph::PieChart)
    {
        m_btnGraphPie.SetState(BST_CHECKED);
    }

    UpdateData();
    ShowSelectStat(static_cast<Metrics>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel())), true);
}
void CStatGraphDlg::OnBnClickedGraphbarbutton()
{
    m_graphType = MyGraph::Bar;
    m_bStacked  = false;
    RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphbarstackedbutton()
{
    m_graphType = MyGraph::Bar;
    m_bStacked  = true;
    RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphlinebutton()
{
    m_graphType = MyGraph::Line;
    m_bStacked  = false;
    RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphlinestackedbutton()
{
    m_graphType = MyGraph::Line;
    m_bStacked  = true;
    RedrawGraph();
}

void CStatGraphDlg::OnBnClickedGraphpiebutton()
{
    m_graphType = MyGraph::PieChart;
    m_bStacked  = false;
    RedrawGraph();
}

BOOL CStatGraphDlg::PreTranslateMessage(MSG* pMsg)
{
    if (nullptr != m_pToolTip)
        m_pToolTip->RelayEvent(pMsg);

    return CStandAloneDialogTmpl<CResizableDialog>::PreTranslateMessage(pMsg);
}

void CStatGraphDlg::EnableDisableMenu() const
{
    UINT nEnable = MF_BYCOMMAND;

    Metrics selectMetric = static_cast<Metrics>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel()));

    nEnable |= (selectMetric > TextStatStart && selectMetric < TextStatEnd)
                   ? (MF_DISABLED | MF_GRAYED)
                   : MF_ENABLED;

    GetMenu()->EnableMenuItem(ID_FILE_SAVESTATGRAPHAS, nEnable);
}

void CStatGraphDlg::OnFileSavestatgraphas()
{
    CString tempFile;
    int     filterIndex = 0;
    if (CAppUtils::FileOpenSave(tempFile, &filterIndex, IDS_REVGRAPH_SAVEPIC, IDS_PICTUREFILEFILTER, false, CString(), m_hWnd))
    {
        // if the user doesn't specify a file extension, default to
        // wmf and add that extension to the filename. But only if the
        // user chose the 'pictures' filter. The filename isn't changed
        // if the 'All files' filter was chosen.
        CString extension;
        int     dotPos   = tempFile.ReverseFind('.');
        int     slashPos = tempFile.ReverseFind('\\');
        if (dotPos > slashPos)
            extension = tempFile.Mid(dotPos);
        if ((filterIndex == 1) && (extension.IsEmpty()))
        {
            extension = L".wmf";
            tempFile += extension;
        }
        SaveGraph(tempFile);
    }
}

void CStatGraphDlg::SaveGraph(CString sFilename)
{
    CString extension = CPathUtils::GetFileExtFromPath(sFilename);
    if (extension.CompareNoCase(L".wmf") == 0)
    {
        // save the graph as an enhanced meta file
        CMyMetaFileDC wmfDC;
        wmfDC.CreateEnhanced(nullptr, sFilename, nullptr, L"TortoiseSVN\0Statistics\0\0");
        wmfDC.SetAttribDC(GetDC()->GetSafeHdc());
        RedrawGraph();
        m_graph.DrawGraph(wmfDC);
        HENHMETAFILE hEmf = wmfDC.CloseEnhanced();
        DeleteEnhMetaFile(hEmf);
    }
    else
    {
        // save the graph as a pixel picture instead of a vector picture
        // create dc to paint on
        try
        {
            CWindowDC ddc(this);
            CDC       dc;
            if (!dc.CreateCompatibleDC(&ddc))
            {
                ShowErrorMessage();
                return;
            }
            CRect rect;
            GetDlgItem(IDC_GRAPH)->GetClientRect(&rect);
            HBITMAP hBm = ::CreateCompatibleBitmap(ddc.m_hDC, rect.Width(), rect.Height());
            if (hBm == nullptr)
            {
                ShowErrorMessage();
                return;
            }
            HBITMAP oldBm = static_cast<HBITMAP>(dc.SelectObject(hBm));
            // paint the whole graph
            RedrawGraph();
            m_graph.DrawGraph(dc);
            // now use GDI+ to save the picture
            CLSID               encoderClsid;
            GdiplusStartupInput gdiplusStartupInput;
            ULONG_PTR           gdiplusToken;
            CString             sErrormessage;
            if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) == Ok)
            {
                {
                    Bitmap bitmap(hBm, nullptr);
                    if (bitmap.GetLastStatus() == Ok)
                    {
                        // Get the CLSID of the encoder.
                        int ret = 0;
                        if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".png") == 0)
                            ret = GetEncoderClsid(L"image/png", &encoderClsid);
                        else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".jpg") == 0)
                            ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
                        else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".jpeg") == 0)
                            ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
                        else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".bmp") == 0)
                            ret = GetEncoderClsid(L"image/bmp", &encoderClsid);
                        else if (CPathUtils::GetFileExtFromPath(sFilename).CompareNoCase(L".gif") == 0)
                            ret = GetEncoderClsid(L"image/gif", &encoderClsid);
                        else
                        {
                            sFilename += L".jpg";
                            ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
                        }
                        if (ret >= 0)
                        {
                            CStringW tfile = CStringW(sFilename);
                            bitmap.Save(tfile, &encoderClsid, nullptr);
                        }
                        else
                        {
                            sErrormessage.Format(IDS_REVGRAPH_ERR_NOENCODER, static_cast<LPCWSTR>(CPathUtils::GetFileExtFromPath(sFilename)));
                        }
                    }
                    else
                    {
                        sErrormessage.LoadString(IDS_REVGRAPH_ERR_NOBITMAP);
                    }
                }
                GdiplusShutdown(gdiplusToken);
            }
            else
            {
                sErrormessage.LoadString(IDS_REVGRAPH_ERR_GDIINIT);
            }
            dc.SelectObject(oldBm);
            dc.DeleteDC();
            if (!sErrormessage.IsEmpty())
            {
                ::MessageBox(m_hWnd, sErrormessage, L"TortoiseSVN", MB_ICONERROR);
            }
        }
        catch (CException* pE)
        {
            wchar_t szErrorMsg[2048] = {0};
            pE->GetErrorMessage(szErrorMsg, 2048);
            pE->Delete();
            ::MessageBox(m_hWnd, szErrorMsg, L"TortoiseSVN", MB_ICONERROR);
        }
    }
}

int CStatGraphDlg::GetEncoderClsid(const WCHAR* format, CLSID* pClsid) const
{
    UINT num  = 0; // number of image encoders
    UINT size = 0; // size of the image encoder array in bytes

    if (GetImageEncodersSize(&num, &size) != Ok)
        return -1;
    if (size == 0)
        return -1; // Failure

    auto            pMem            = std::make_unique<BYTE[]>(size);
    ImageCodecInfo* pImageCodecInfo = reinterpret_cast<ImageCodecInfo*>(pMem.get());
    if (pImageCodecInfo == nullptr)
        return -1; // Failure

    if (GetImageEncoders(num, size, pImageCodecInfo) == Ok)
    {
        for (UINT j = 0; j < num; ++j)
        {
            if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
            {
                *pClsid = pImageCodecInfo[j].Clsid;
                return j; // Success
            }
        }
    }
    return -1; // Failure
}

void CStatGraphDlg::StoreCurrentGraphType()
{
    UpdateData();
    DWORD graphtype = static_cast<DWORD>(m_cGraphType.GetItemData(m_cGraphType.GetCurSel()));
    // encode the current chart type
    DWORD statspage = graphtype * 10;
    if ((m_graphType == MyGraph::Bar) && (m_bStacked))
    {
        statspage += 1;
    }
    if ((m_graphType == MyGraph::Bar) && (!m_bStacked))
    {
        statspage += 2;
    }
    if ((m_graphType == MyGraph::Line) && (m_bStacked))
    {
        statspage += 3;
    }
    if ((m_graphType == MyGraph::Line) && (!m_bStacked))
    {
        statspage += 4;
    }
    if (m_graphType == MyGraph::PieChart)
    {
        statspage += 5;
    }

    // store current chart type in registry
    CRegDWORD lastStatsPage(L"Software\\TortoiseSVN\\LastViewedStatsPage", 0);
    lastStatsPage = statspage;

    CRegDWORD regAuthors(L"Software\\TortoiseSVN\\StatAuthorsCaseSensitive");
    regAuthors = m_bAuthorsCaseSensitive;

    CRegDWORD regSort(L"Software\\TortoiseSVN\\StatSortByCommitCount");
    regSort = m_bSortByCommitCount;
}

void CStatGraphDlg::ShowErrorMessage()
{
    CFormatMessageWrapper errorDetails;
    if (errorDetails)
        MessageBox(errorDetails, L"Error", MB_OK | MB_ICONINFORMATION);
}

void CStatGraphDlg::ShowSelectStat(Metrics selectedMetric, bool reloadSkiper /* = false */)
{
    switch (selectedMetric)
    {
        case AllStat:
            LoadListOfAuthors(m_commitsPerAuthor, reloadSkiper);
            ShowStats();
            break;
        case CommitsByDate:
            LoadListOfAuthors(m_commitsPerAuthor, reloadSkiper);
            ShowCommitsByDate();
            break;
        case CommitsByAuthor:
            LoadListOfAuthors(m_commitsPerAuthor, reloadSkiper);
            ShowCommitsByAuthor();
            break;
        case PercentageOfAuthorship:
            LoadListOfAuthors(m_percentageOfAuthorship, reloadSkiper, true);
            ShowPercentageOfAuthorship();
            break;
        default:
            ShowErrorMessage();
    }
}

double CStatGraphDlg::CoeffContribution(int distFromEnd)
{
    return distFromEnd ? 1.0 / COEFF_AUTHOR_SHIP * distFromEnd : 1;
}

template <class Map>
void CStatGraphDlg::DrawOthers(const std::list<std::wstring>& others, MyGraphSeries* graphData, Map& map)
{
    int nCommits = 0;
    for (auto it = others.begin(); it != others.end(); ++it)
    {
        nCommits += RollPercentageOfAuthorship(map[*it]);
    }

    CString temp;
    temp.Format(L" (%Iu)", others.size());

    CString sOthers(MAKEINTRESOURCE(IDS_STATGRAPH_OTHERGROUP));
    sOthers += temp;
    int group = m_graph.AppendGroup(sOthers);
    graphData->SetData(group, static_cast<int>(nCommits));
}

template <class Map>
void CStatGraphDlg::LoadListOfAuthors(Map& map, bool reloadSkiper /*= false*/, bool compare /*= false*/)
{
    m_authorNames.clear();
    if (map.size())
    {
        for (auto it = map.begin(); it != map.end(); ++it)
        {
            if ((compare && RollPercentageOfAuthorship(map[it->first]) != 0) || !compare)
                m_authorNames.push_back(it->first);
        }
    }
    // Sort the list of authors based on commit count
    m_authorNames.sort(MoreCommitsThan<typename Map::mapped_type>(map));

    // Set Skipper
    SetSkipper(reloadSkiper);
}

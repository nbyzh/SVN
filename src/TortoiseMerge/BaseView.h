﻿// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2003-2015, 2017-2022, 2024 - TortoiseSVN

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
#include "DiffData.h"
#include "SVNLineDiff.h"
#include "ScrollTool.h"
#include "Undo.h"
#include "LocatorBar.h"
#include "LineColors.h"
#include "TripleClick.h"
#include "IconMenu.h"
#include "FindDlg.h"

struct InlineDiffPos
{
    apr_off_t start;
    apr_off_t end;
};

/**
 * \ingroup TortoiseMerge
 *
 * View class providing the basic functionality for
 * showing diffs. Has three parent classes which inherit
 * from this base class: CLeftView, CRightView and CBottomView.
 */
class CBaseView : public CView
    , public CTripleClick
{
    DECLARE_DYNCREATE(CBaseView)
    friend class CLineDiffBar;

public:
    using UnicodeType = CFileTextLines::UnicodeType;
    enum ECharGroup
    { // ordered by priority low-to-hi
        CHG_UNKNOWN,
        CHG_CONTROL,       // x00-x08, x0a-x1f
        CHG_WHITESPACE,    // space, tab
        CHG_WORDSEPARATOR, // 0x21-2f, x3a-x40, x5b-x60, x7b-x7f .,:;!?(){}[]/\<> ...
        CHG_WORDLETTER,    // alpha num _ (others)
    };

    CBaseView();
    ~CBaseView() override;

public: // methods
    /**
     * Indicates that the underlying document has been updated. Reloads all
     * data and redraws the view.
     */
    virtual void DocumentUpdated();
    /**
     * Returns the number of lines visible on the view.
     */
    int GetScreenLines();
    /**
     * Scrolls the view to the given line.
     * \param nNewTopLine The new top line to scroll the view to
     * \param bTrackScrollBar If TRUE, then the scrollbars are affected too.
     */
    void        ScrollToLine(int nNewTopLine, BOOL bTrackScrollBar = TRUE);
    static void ScrollAllToLine(int nNewTopLine, BOOL bTrackScrollBar = TRUE);
    void        ScrollSide(int delta);
    void        ScrollAllSide(int delta);
    void        ScrollVertical(short delta);
    static void RecalcAllVertScrollBars(BOOL bPositionOnly = FALSE);
    static void RecalcAllHorzScrollBars(BOOL bPositionOnly = FALSE);
    void        GoToLine(int nNewLine, BOOL bAll = TRUE);
    void        ScrollToChar(int nNewOffsetChar, BOOL bTrackScrollBar = TRUE);
    static void ScrollAllToChar(int nNewOffsetChar, BOOL bTrackScrollBar = TRUE);
    void        SetReadonly(bool bReadonly = true)
    {
        m_bReadonly = bReadonly;
        if (m_pFindDialog)
            m_pFindDialog->SetReadonly(m_bReadonly);
    }
    void SetWritable(bool bWritable = true)
    {
        m_bReadonly = !bWritable;
        if (m_pFindDialog)
            m_pFindDialog->SetReadonly(m_bReadonly);
    }
    void SetWritableIsChangable(bool bWritableIsChangable = true) { m_bReadonlyIsChangable = bWritableIsChangable; }
    void SetTarget(bool bTarget = true) { m_bTarget = bTarget; }
    bool IsReadonly() const { return m_bReadonly; }
    bool IsWritable() const { return !m_bReadonly && m_pViewData; }
    bool IsReadonlyChangable() const { return m_bReadonlyIsChangable && !IsModified(); }
    bool IsTarget() const { return m_bTarget; }
    void SetCaretAndGoalPosition(const POINT& pt)
    {
        UpdateCaretPosition(pt);
        UpdateGoalPos();
    }
    void SetCaretAndGoalViewPosition(const POINT& pt)
    {
        UpdateCaretViewPosition(pt);
        UpdateGoalPos();
    }
    void  SetCaretPosition(const POINT& pt) { SetCaretViewPosition(ConvertScreenPosToView(pt)); }
    POINT GetCaretPosition() { return ConvertViewPosToScreen(GetCaretViewPosition()); }
    void  SetCaretViewPosition(const POINT& pt) { m_ptCaretViewPos = pt; }
    POINT GetCaretViewPosition() const { return m_ptCaretViewPos; }
    void  UpdateCaretPosition(const POINT& pt)
    {
        SetCaretPosition(pt);
        UpdateCaret();
    }
    void UpdateCaretViewPosition(const POINT& pt)
    {
        SetCaretViewPosition(pt);
        UpdateCaret();
        EnsureCaretVisible();
    }
    void SetCaretToViewStart()
    {
        SetCaretToFirstViewLine();
        SetCaretToViewLineStart();
    }
    void SetCaretToFirstViewLine() { m_ptCaretViewPos.y = 0; }
    void SetCaretToViewLineStart() { m_ptCaretViewPos.x = 0; }
    void SetCaretToLineStart() { SetCaretAndGoalPosition(SetupPoint(0, GetCaretPosition().y)); }
    void EnsureCaretVisible();
    void UpdateCaret();

    static bool  ArePointsSame(const POINT& pt1, const POINT& pt2) { return (pt1.x == pt2.x) && (pt1.y == pt2.y); };
    static POINT SetupPoint(int x, int y)
    {
        POINT ptRet = {x, y};
        return ptRet;
    };
    POINT ConvertScreenPosToView(const POINT& pt);
    POINT ConvertViewPosToScreen(const POINT& pt);

    static void RefreshViews();
    static void BuildAllScreen2ViewVector();                                    ///< schedule full screen2view rebuild
    static void BuildAllScreen2ViewVector(int viewLine);                        ///< schedule rebuild screen2view for single line
    static void BuildAllScreen2ViewVector(int firstViewLine, int lastViewLine); ///< schedule rebuild screen2view for line range (first and last inclusive)
    void        UpdateViewLineNumbers();
    int         CleanEmptyLines() const; ///< remove line empty in all views
    int         GetLineCount() const;
    static int  GetViewLineForScreen(int screenLine) { return m_screen2View.GetViewLineForScreen(screenLine); }
    static int  FindScreenLineForViewLine(int viewLine);
    // TODO: find better consistent names for Multiline(line with sublines) and Subline, Count.. or Get..Count ?
    int         CountMultiLines(int nViewLine);
    static int  GetSubLineOffset(int index);
    LineColors& GetLineColors(int nViewLine);
    static void UpdateLocator()
    {
        if (m_pwndLocator)
            m_pwndLocator->DocumentUpdated();
    }
    void WrapChanged();

    void HighlightViewLines(int start, int end = -1);
    BOOL IsHidden() const { return m_bIsHidden; }
    void SetHidden(BOOL bHidden) { m_bIsHidden = bHidden; }
    bool IsModified() const { return m_bModified; }
    // cppcheck-suppress bitwiseOnBoolean
    void SetModified(bool bModified = true)
    {
        m_bModified = bModified;
        m_pState->modifies |= bModified;
        Invalidate();
    }
    bool HasMarkedBlocks() const { return m_pViewData->HasMarkedBlocks(); }
    void ClearStepModifiedMark() const { m_pState->modifies = false; }
    void SetInlineWordDiff(bool bWord) { m_bInlineWordDiff = bWord; }
    void SetInlineDiff(bool bDiff) { m_bShowInlineDiff = bDiff; }
    void SetMarkedWord(const CString& word)
    {
        m_sMarkedWord = word;
        BuildMarkedWordArray();
    }
    LPCWSTR GetMarkedWord() const { return static_cast<LPCWSTR>(m_sMarkedWord); }
    int     GetMarkedWordCount() const { return m_markedWordCount; }
    LPCWSTR GetFindString() const { return static_cast<LPCWSTR>(m_sFindText); }

    // Selection methods; all public methods dealing with selection go here
    static void ClearSelection();
    BOOL        GetViewSelection(int& start, int& end) const;
    BOOL        HasSelection() const { return (!((m_nSelViewBlockEnd < 0) || (m_nSelViewBlockStart < 0) || (m_nSelViewBlockStart > m_nSelViewBlockEnd))); }
    BOOL        HasTextSelection() const { return ((m_ptSelectionViewPosStart.x != m_ptSelectionViewPosEnd.x) || (m_ptSelectionViewPosStart.y != m_ptSelectionViewPosEnd.y)); }
    BOOL        HasTextLineSelection() const { return m_ptSelectionViewPosStart.y != m_ptSelectionViewPosEnd.y; }
    static void SetupAllViewSelection(int start, int end);
    static void SetupAllSelection(int start, int end);
    void        SetupSelection(int start, int end);
    static void SetupViewSelection(CBaseView* view, int start, int end);
    void        SetupViewSelection(int start, int end);
    CString     GetSelectedText() const;
    void        CheckModifications(bool& hasMods, bool& hasConflicts, bool& hasWhitespaceMods, bool& hasFilteredMods) const;

    // state classifying methods; note: state may belong to more classes
    static bool       IsStateConflicted(DiffStates state);
    static bool       IsStateEmpty(DiffStates state);
    static bool       IsStateRemoved(DiffStates state);
    static DiffStates ResolveState(DiffStates state);

    bool IsLineEmpty(int nLineIndex) const;
    bool IsViewLineEmpty(int nViewLine) const;
    bool IsLineRemoved(int nLineIndex) const;
    bool IsViewLineRemoved(int nViewLine) const;
    bool IsBlockWhitespaceOnly(int nLineIndex, bool& bIdentical, int& blockstart, int& blockend);
    bool HasNextConflict();
    bool HasPrevConflict();
    bool HasNextDiff();
    bool HasPrevDiff();
    bool GetNextInlineDiff(int& nPos);
    bool GetPrevInlineDiff(int& nPos);
    bool HasNextInlineDiff();
    bool HasPrevInlineDiff();

    static const ViewData& GetEmptyLineData();
    void                   InsertViewEmptyLines(int nFirstView, int nCount) const;

    virtual void UseBothLeftFirst() { return UseBothBlocks(m_pwndLeft, m_pwndRight); }
    virtual void UseBothRightFirst() { return UseBothBlocks(m_pwndRight, m_pwndLeft); }
    void         UseTheirAndYourBlock() { return UseBothLeftFirst(); }  ///< ! for backward compatibility
    void         UseYourAndTheirBlock() { return UseBothRightFirst(); } ///< ! for backward compatibility

    virtual void UseLeftBlock() { return UseViewBlock(m_pwndLeft); }
    virtual void UseLeftFile() { return UseViewFile(m_pwndLeft); }
    virtual void UseRightBlock() { return UseViewBlock(m_pwndRight); }
    virtual void UseRightFile() { return UseViewFile(m_pwndRight); }
    virtual void LeaveOnlyMarkedBlocks() { return LeaveOnlyMarkedBlocks(m_pwndLeft); }
    virtual void UseViewFileOfMarked() { UseViewFileOfMarked(m_pwndLeft); }
    virtual void UseViewFileExceptEdited() { UseViewFileExceptEdited(m_pwndLeft); }

    // ViewData methods
    void InsertViewData(int index, const CString& sLine, DiffStates state, int linenumber, EOL ending, HideState hide, int movedline) const;
    void InsertViewData(int index, const ViewData& data) const;
    void RemoveViewData(int index) const;

    const ViewData& GetViewData(int index) const { return m_pViewData->GetData(index); }
    const CString&  GetViewLine(int index) const { return m_pViewData->GetLine(index); }
    DiffStates      GetViewState(int index) const { return m_pViewData->GetState(index); }
    HideState       GetViewHideState(int index) const { return m_pViewData->GetHideState(index); }
    int             GetViewLineNumber(int index) const { return m_pViewData->GetLineNumber(index); }
    int             GetViewMovedIndex(int index) const { return m_pViewData->GetMovedIndex(index); }
    int             FindViewLineNumber(int number) const { return m_pViewData->FindLineNumber(number); }
    EOL             GetViewLineEnding(int index) const { return m_pViewData->GetLineEnding(index); }
    bool            GetViewMarked(int index) const { return m_pViewData->GetMarked(index); }

    int GetViewCount() const { return m_pViewData ? m_pViewData->GetCount() : -1; }

    void SetViewData(int index, const ViewData& data) const;
    void SetViewState(int index, DiffStates state) const;
    void SetViewLine(int index, const CString& sLine) const;
    void SetViewLineNumber(int index, int linenumber) const;
    void SetViewLineEnding(int index, EOL ending) const;
    void SetViewMarked(int index, bool marked) const;

    static bool       IsViewGood(const CBaseView* view) { return (view != nullptr) && view->IsWindowVisible(); }
    static CBaseView* GetFirstGoodView();

    int  GetLargestSpaceStreak(const CString& line) const;
    int  GetIndentCharsForLine(int x, int y);
    void AddIndentationForSelectedBlock();
    void RemoveIndentationForSelectedBlock();
    void ConvertTabToSpaces();
    void Tabularize();
    void RemoveTrailWhiteChars();

    struct TWhitecharsProperties
    {
        bool hasMixedEols;
        bool hasTrailWhiteChars;
        bool hasSpacesToConvert;
        bool hasTabsToConvert;
    };

    TWhitecharsProperties GetWhitecharsProperties() const;

public: // variables
    CViewData* m_pViewData;
    CViewData* m_pOtherViewData;
    CBaseView* m_pOtherView;

    CString m_sWindowName;        ///< The name of the view which is shown as a window title to the user
    CString m_sFullFilePath;      ///< The full path of the file shown
    CString m_sConvertedFilePath; ///< the path to the converted file that's shown in the view
    CString m_sReflectedName;     ///< The reflected name of file

    BOOL             m_bViewWhitespace;        ///< If TRUE, then SPACE and TAB are shown as special characters
    BOOL             m_bShowInlineDiff;        ///< If TRUE, diffs in lines are marked colored
    bool             m_bShowSelection;         ///< If true, selection bars are shown and selected text darkened
    bool             m_bWhitespaceInlineDiffs; ///< if true, inline diffs are shown for identical lines only differing in whitespace
    int              m_nTopLine;               ///< The topmost text line in the view
    std::vector<int> m_arMarkedWordLines;      ///< which lines contain a marked word
    std::vector<int> m_arFindStringLines;      ///< which lines contain a found string

    static CLocatorBar*         m_pwndLocator;         ///< Pointer to the locator bar on the left
    static CLineDiffBar*        m_pwndLineDiffBar;     ///< Pointer to the line diff bar at the bottom
    static CMFCStatusBar*       m_pwndStatusBar;       ///< Pointer to the status bar
    static CMFCRibbonStatusBar* m_pwndRibbonStatusBar; ///< Pointer to the status bar
    static CMainFrame*          m_pMainFrame;          ///< Pointer to the mainframe

    int  m_nTabMode;
    bool m_bEditorConfigEnabled;
    BOOL m_bEditorConfigLoaded;

    void GoToFirstDifference();
    void GoToFirstConflict();
    void AddEmptyViewLine(int nLineIndex);
#define SAVE_REMOVEDLINES 1
    int SaveFile(int flags = 0);
    int SaveFileTo(CString fileName, int flags = 0);

    EOL         GetLineEndings() const; ///< Get Line endings on view from lineendings or "mixed"
    EOL         GetLineEndings(bool mixelEols) const;
    void        ReplaceLineEndings(EOL); ///< Set AUTO lineending and replaces all EOLs
    void        SetLineEndingStyle(EOL); ///< Set AUTO lineending
    UnicodeType GetTextType() const { return m_textType; }
    void        SetTextType(UnicodeType);                 ///< Changes TextType
    void        AskUserForNewLineEndingsAndTextType(int); ///< Open gui
    int         GetTabMode() const { return m_nTabMode; }
    void        SetTabMode(int nTabMode) { m_nTabMode = nTabMode; }
    int         GetTabSize() const { return m_nTabSize; }
    void        SetTabSize(int nTabSize) { m_nTabSize = nTabSize; }
    bool        GetEditorConfigEnabled() const { return m_bEditorConfigEnabled; }
    void        SetEditorConfigEnabled(bool bEditorConfigEnabled);
    BOOL        GetEditorConfigLoaded() const { return m_bEditorConfigLoaded; }

    void          DPIChanged();
    CWorkingFile* m_pWorkingFile; ///< pointer to source/destination file parametrers

protected: // methods
    enum
    {
        MOVERIGHT = 0,
        MOVELEFT  = 1,
    };

    BOOL            PreCreateWindow(CREATESTRUCT& cs) override;
    void            OnDraw(CDC* pDC) override;
    INT_PTR         OnToolHitTest(CPoint point, TOOLINFO* pTi) const override;
    BOOL            PreTranslateMessage(MSG* pMsg) override;
    virtual ULONG   GetGestureStatus(CPoint ptTouch) override;
    BOOL            OnToolTipNotify(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void    OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void    OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg BOOL    OnEraseBkgnd(CDC* pDC);
    afx_msg int     OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void    OnDestroy();
    afx_msg void    OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL    OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void    OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg BOOL    OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void    OnKillFocus(CWnd* pNewWnd);
    afx_msg void    OnSetFocus(CWnd* pOldWnd);
    afx_msg void    OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void    OnMergeNextdifference();
    afx_msg void    OnMergePreviousdifference();
    afx_msg void    OnMergePreviousconflict();
    afx_msg void    OnMergeNextconflict();
    afx_msg void    OnNavigateNextinlinediff();
    afx_msg void    OnNavigatePrevinlinediff();
    afx_msg void    OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void    OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void    OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void    OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void    OnLButtonTrippleClick(UINT nFlags, CPoint point) override;
    afx_msg void    OnEditCopy();
    afx_msg void    OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void    OnTimer(UINT_PTR nIDEvent);
    afx_msg void    OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void    OnCaretDown();
    afx_msg void    OnCaretLeft();
    afx_msg void    OnCaretRight();
    afx_msg void    OnCaretUp();
    afx_msg void    OnCaretWordleft();
    afx_msg void    OnCaretWordright();
    afx_msg void    OnEditCut();
    afx_msg void    OnEditPaste();
    afx_msg void    OnEditSelectall();
    afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
    afx_msg void    OnEditFind();
    afx_msg void    OnEditFindnext();
    afx_msg void    OnEditFindprev();
    afx_msg void    OnEditFindnextStart();
    afx_msg void    OnEditFindprevStart();
    afx_msg void    OnEditGotoline();
    afx_msg void    OnContextMenu(CPoint point, DiffStates state);

    DECLARE_MESSAGE_MAP()

    void DrawHeader(CDC* pdc, const CRect& rect);
    void DrawMargin(CDC* pdc, const CRect& rect, int nLineIndex);
    void DrawSingleLine(CDC* pDC, const CRect& rc, int nLineIndex);
    /**
     * Draws the horizontal lines around current diff block or selection block.
     */
    void DrawBlockLine(CDC* pDC, const CRect& rc, int nLineIndex) const;
    /**
     * Draws the line ending 'char'.
     */
    void    DrawLineEnding(CDC* pDC, const CRect& rc, int nLineIndex, const CPoint& origin);
    void    ExpandChars(const CString& sLine, int nOffset, int nCount, CString& line);
    CString ExpandChars(const CString& sLine, int nOffset = 0);
    int     CountExpandedChars(const CString& sLine, int nLength);

    void RecalcVertScrollBar(BOOL bPositionOnly = FALSE);
    void RecalcHorzScrollBar(BOOL bPositionOnly = FALSE);

    void OnDoMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    void OnDoMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
    void OnDoHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar, CBaseView* master);
    void OnDoVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar, CBaseView* master);

    void ShowDiffLines(int nLine);

    void DeleteFonts();

    void        CalcLineCharDim();
    int         GetLineHeight();
    int         GetCharWidth();
    int         GetMaxLineLength();
    int         GetLineLength(int index);
    int         GetLineLengthWithTabsConverted(int index);
    int         GetViewLineLength(int index) const;
    int         GetScreenChars();
    int         GetAllMinScreenChars() const;
    int         GetAllMaxLineLength() const;
    int         GetAllLineCount() const;
    int         GetAllMinScreenLines() const;
    CString     GetViewLineChars(int index) const;
    CString     GetLineChars(int index);
    int         GetLineNumber(int index) const;
    CFont*      GetFont(BOOL bItalic = FALSE, BOOL bBold = FALSE);
    int         GetLineFromPoint(CPoint point);
    int         GetMarginWidth();
    COLORREF    InlineDiffColor(int nLineIndex) const;
    COLORREF    InlineViewLineDiffColor(int nLineIndex) const;
    bool        GetInlineDiffPositions(int lineIndex, std::vector<InlineDiffPos>& positions);
    void        CheckOtherView();
    static void GetWhitespaceBlock(CViewData* viewData, int nLineIndex, int& nStartBlock, int& nEndBlock);
    CString     GetWhitespaceString(CViewData* viewData, int nStartBlock, int nEndBlock) const;
    bool        IsViewLineHidden(int nViewLine) const;
    static bool IsViewLineHidden(CViewData* pViewData, int nViewLine);

    /**
     * Updates the status bar pane. Call this if the document changed.
     */
    void UpdateStatusBar();

    static bool IsLeftViewGood() { return IsViewGood(m_pwndLeft); }
    static bool IsRightViewGood() { return IsViewGood(m_pwndRight); }
    static bool IsBottomViewGood() { return IsViewGood(m_pwndBottom); }

    int   CalculateActualOffset(const POINT& point);
    int   CalculateCharIndex(int nLineIndex, int nActualOffset, bool allowEOL);
    int   CalcColFromPoint(int xpos, int lineIndex);
    POINT TextToClient(const POINT& point);
    void  DrawTextLine(CDC* pDC, const CRect& rc, int nLineIndex, POINT& coords);
    void  ClearCurrentSelection();
    void  AdjustSelection(bool bMoveLeft);
    bool  SelectNextBlock(int nDirection, bool bConflict, bool bSkipEndOfCurrentBlock = true, bool dryrun = false);

    enum SearchDirection
    {
        SearchNext     = 0,
        SearchPrevious = 1
    };
    bool StringFound(const CString& str, SearchDirection srchDir, int& start, int& end) const;
    bool Search(SearchDirection srchDir, bool useStart, bool flashIfNotFound, bool stopEof);
    void BuildFindStringArray();

    void RemoveSelectedText();
    void PasteText();
    void InsertText(const CString& sText);

    void AddUndoViewLine(int nViewLine, bool bAddEmptyLine = false);

    bool MoveCaretLeft();
    bool MoveCaretRight();
    void MoveCaretWordLeft();
    void MoveCaretWordRight();
    void OnCaretMove(bool bMoveLeft);
    void OnCaretMove(bool bMoveLeft, bool isShiftPressed);
    void UpdateGoalPos();

    ECharGroup GetCharGroup(const CString& str, int index) const { return index >= 0 && index < str.GetLength() ? GetCharGroup(str[index]) : CHG_UNKNOWN; }
    ECharGroup GetCharGroup(const wchar_t zChar) const;
    bool       IsWordSeparator(const wchar_t ch) const;
    bool       IsCaretAtWordBoundary() const;
    void       UpdateViewsCaretPosition();
    void       BuildMarkedWordArray();

    virtual void UseBothBlocks(CBaseView* /*pwndFirst*/, CBaseView* /*pwndLast*/){};
    virtual void UseViewBlock(CBaseView* /*pwndView*/) {}
    void         UseViewBlock(
                CBaseView* pwndView, int nFirstViewLine, int nLastViewLine, std::function<bool(int)> fnSkip = [](int) -> bool { return false; });
    virtual void UseViewFile(CBaseView* /*pwndView*/) {}
    virtual void MarkBlock(bool /*marked*/) {}
    void         MarkBlock(bool marked, int nFirstViewLine, int nLastViewLine);
    void         LeaveOnlyMarkedBlocks(CBaseView* pwndView);
    void         UseViewFileOfMarked(CBaseView* pwndView);
    void         UseViewFileExceptEdited(CBaseView* pwndView);

    virtual void AddContextItems(CIconMenu& popup, DiffStates state);
    void         AddCutCopyAndPaste(CIconMenu& popup) const;
    void         CompensateForKeyboard(CPoint& point) const;
    void         ReleaseBitmap();
    static bool  LinesInOneChange(int direction, DiffStates firstLineState, DiffStates currentLineState);
    static void  FilterWhitespaces(CString& first, CString& second);
    static void  FilterWhitespaces(CString& line);
    int          GetButtonEventLineIndex(const POINT& point);

    static void ResetUndoStep();
    void        SaveUndoStep() const;

    void SetTheme(bool bDark);

protected: // variables
    COLORREF m_inlineRemovedBk;
    COLORREF m_inlineAddedBk;
    COLORREF m_modifiedBk;
    COLORREF m_inlineRemovedDarkBk;
    COLORREF m_inlineAddedDarkBk;
    COLORREF m_modifiedDarkBk;
    COLORREF m_whiteSpaceFg;
    UINT     m_nStatusBarID; ///< The ID of the status bar pane used by this view. Must be set by the parent class.

    SVNLineDiff m_svnlinediff;
    DWORD       m_nInlineDiffMaxLineLength;
    BOOL        m_bOtherDiffChecked;
    bool        m_bModified;
    BOOL        m_bFocused;
    BOOL        m_bViewLinenumbers;
    BOOL        m_bIsHidden;
    BOOL        m_bIconLFs;
    int         m_nLineHeight;
    int         m_nCharWidth;
    int         m_nMaxLineLength;
    int         m_nScreenLines;
    int         m_nScreenChars;
    int         m_nLastScreenChars;
    int         m_nOffsetChar;
    int         m_nTabSize;
    int         m_nDigits;
    bool        m_bInlineWordDiff;

    // Block selection attributes
    int m_nSelViewBlockStart;
    int m_nSelViewBlockEnd;

    int     m_nMouseLine;
    int     m_nLDownLine;
    bool    m_mouseInMargin;
    HCURSOR m_margincursor;

    // caret
    bool  m_bReadonly;
    bool  m_bReadonlyIsChangable;
    bool  m_bTarget; ///< view intended as result
    POINT m_ptCaretViewPos;
    int   m_nCaretGoalPos;

    // Text selection attributes
    POINT m_ptSelectionViewPosStart;
    POINT m_ptSelectionViewPosEnd;
    POINT m_ptSelectionViewPosOrigin;

    static const UINT FIND_DIALOG_MESSAGE;
    CFindDlg*         m_pFindDialog;
    CString           m_sFindText;
    BOOL              m_bMatchCase;
    bool              m_bLimitToDiff;
    bool              m_bWholeWord;

    HICON m_hAddedIcon;
    HICON m_hRemovedIcon;
    HICON m_hConflictedIcon;
    HICON m_hConflictedIgnoredIcon;
    HICON m_hWhitespaceBlockIcon;
    HICON m_hEqualIcon;
    HICON m_hEditedIcon;

    HICON m_hLineEndingCR;
    HICON m_hLineEndingCRLF;
    HICON m_hLineEndingLF;

    HICON m_hMovedIcon;
    HICON m_hMarkedIcon;

    LOGFONT          m_lfBaseFont;
    static const int m_fontsCount = 4;
    CFont*           m_apFonts[m_fontsCount];
    CString          m_sConflictedText;
    CString          m_sNoLineNr;
    CString          m_sMarkedWord;
    CString          m_sPreviousMarkedWord;
    int              m_markedWordCount;

    CBitmap*    m_pCacheBitmap;
    CDC*        m_pDC;
    CScrollTool m_scrollTool;
    CString     m_sWordSeparators;
    CString     m_eols[EOL_COUNT];

    UnicodeType m_textType;    ///< the text encoding this view uses
    EOL         m_lineEndings; ///< the line endings the view uses
    bool        m_bInsertMode;
    bool        m_bDark;
    int         m_themeCallbackId;
    char        m_szTip[MAX_PATH * 2 + 1];
    wchar_t     m_wszTip[MAX_PATH * 2 + 1];
    // These three pointers lead to the three parent
    // classes CLeftView, CRightView and CBottomView
    // and are used for the communication between
    // the views (e.g. synchronized scrolling, ...)
    // To find out which parent class this object
    // is made of just compare e.g. (m_pwndLeft==this).
    static CBaseView* m_pwndLeft;   ///< Pointer to the left view. Must be set by the CLeftView parent class.
    static CBaseView* m_pwndRight;  ///< Pointer to the right view. Must be set by the CRightView parent class.
    static CBaseView* m_pwndBottom; ///< Pointer to the bottom view. Must be set by the CBottomView parent class.

    struct TScreenLineInfo
    {
        int nViewLine;
        int nViewSubLine;
    };
    class TScreenedViewLine
    {
    public:
        TScreenedViewLine()
        {
            Clear();
        }

        void Clear()
        {
            bSublinesSet             = false;
            eIcon                    = ICN_UNKNOWN;
            bLineColorsSet           = false;
            bLineColorsSetWhiteSpace = false;
        }

        bool                 bSublinesSet;
        std::vector<CString> m_subLines;

        enum EIcon
        {
            ICN_UNKNOWN,
            ICN_NONE,
            ICN_EDIT,
            ICN_SAME,
            ICN_WHITESPACEDIFF,
            ICN_ADD,
            ICN_REMOVED,
            ICN_MOVED,
            ICN_MARKED,
            ICN_CONFLICT,
            ICN_CONFLICTIGNORED,
        } eIcon;

        bool       bLineColorsSetWhiteSpace;
        LineColors lineColorsWhiteSpace;
        bool       bLineColorsSet;
        LineColors lineColors;
    };
    std::vector<TScreenedViewLine> m_screenedViewLine; ///< cached data for screening

    static AllViewState m_allState;
    ViewState*          m_pState;

    enum PopupCommands
    {
        POPUPCOMMAND_DISMISSED = 0,
        // 2-pane view commands
        POPUPCOMMAND_USELEFTBLOCK,
        POPUPCOMMAND_USELEFTFILE,
        POPUPCOMMAND_USEBOTHLEFTFIRST,
        POPUPCOMMAND_USEBOTHRIGHTFIRST,
        POPUPCOMMAND_MARKBLOCK,
        POPUPCOMMAND_UNMARKBLOCK,
        POPUPCOMMAND_LEAVEONLYMARKEDBLOCKS,
        // multiple writable views
        POPUPCOMMAND_PREPENDFROMRIGHT,
        POPUPCOMMAND_REPLACEBYRIGHT,
        POPUPCOMMAND_APPENDFROMRIGHT,
        POPUPCOMMAND_USERIGHTFILE,
        // 3-pane view commands
        POPUPCOMMAND_USEYOURANDTHEIRBLOCK,
        POPUPCOMMAND_USETHEIRANDYOURBLOCK,
        POPUPCOMMAND_USEYOURBLOCK,
        POPUPCOMMAND_USEYOURFILE,
        POPUPCOMMAND_USETHEIRBLOCK,
        POPUPCOMMAND_USETHEIRFILE,
        // others
        POPUPCOMMAND_TABTOSPACES,
        POPUPCOMMAND_SPACESTOTABS,
        POPUPCOMMAND_REMOVETRAILWHITES,

        POPUPCOMMAND_LAST,
    };

    class Screen2View
    {
    public:
        Screen2View()
            : m_pViewData(nullptr)
        {
            m_bFull = false;
        }

        int  GetViewLineForScreen(int screenLine);
        int  GetSubLineOffset(int screenLine);
        int  FindScreenLineForViewLine(int viewLine);
        void ScheduleFullRebuild(CViewData* viewData);
        void ScheduleRangeRebuild(CViewData* viewData, int firstViewLine, int lastViewLine);
        int  size();

    private:
        struct TRebuildRange
        {
            int firstViewLine;
            int lastViewLine;
        };

        bool        FixScreenedCacheSize(CBaseView* view) const;
        void        RebuildIfNecessary();
        bool        ResetScreenedViewLineCache(CBaseView* view) const;
        static bool ResetScreenedViewLineCache(CBaseView* view, const TRebuildRange& range);

        CViewData*                   m_pViewData;
        bool                         m_bFull;
        std::vector<TScreenLineInfo> m_screen2View;
        std::vector<TRebuildRange>   m_rebuildRanges;
    };

    static Screen2View         m_screen2View;
    CFileTextLines::SaveParams m_saveParams; ///< encoding and new line style for saving
};

;
;
;

Include "HJConst.jsh"
Include "HJGlobal.jsh"
Include "Common.jsm"

Const
	URLOfRepositoryControlID = 1029,
	URLOfRepositoryComboWrapperClassName = "ComboBoxEx32",
	RevisionLogListViewControlID = 1003

Script ScriptFileName ()
ScriptAndAppNames (GetActiveConfiguration ())
EndScript

Void Function AutoStartEvent ()
let nSaySelectAfter = IniReadInteger (SECTION_OPTIONS, hKey_SaySelectedFirst, 0, file_default_jcf)
let nSaySelectAfter = IniReadInteger (SECTION_OPTIONS, hKey_SaySelectedFirst, nSaySelectAfter, GetActiveConfiguration () + cScPeriod + jcfFileExt)
let nSaySelectAfter = Not nSaySelectAfter
AutoStartEvent ()
EndFunction

String Function FindHotKey (String ByRef sPrompt)
var
	Handle hPrompt,
	Int iChildID,
	Int iType,
	Int iControlID,
	String sHotKey

let hPrompt = GetFocus ()
let iType = GetWindowSubTypeCode (hPrompt)
let iControlID = GetControlID (hPrompt)
If iType == WT_EDITCOMBO
&& iControlID == URLOfRepositoryControlID then
	let hPrompt = GetParent (hPrompt)
	let iType = GetWindowSubTypeCode (hPrompt)
	let iControlID = GetControlID (hPrompt)
	If iType == WT_COMBOBOX
	&& iControlID == URLOfRepositoryControlID then
		let hPrompt = GetParent (hPrompt)
		let iControlID = GetControlID (hPrompt)
		If GetWindowClass (hPrompt) == URLOfRepositoryComboWrapperClassName
		&& iControlID == URLOfRepositoryControlID then
			let sHotKey = GetHotKey(GetPriorWindow (hPrompt))
			If sHotKey then
				let sPrompt = GetWindowName (GetPriorWindow (hPrompt))
				return (sHotKey)
			EndIf
		EndIf
	EndIf
EndIf
let sHotKey = FindHotKey(sPrompt)
If StringIsBlank (sHotKey) then
	let sHotKey = GetFocusObject (iChildID).accKeyboardShortcut
EndIf
Return (sHotKey)
EndFunction

Void Function SayObjectActiveItem (Int iPosition)
var
	Handle hFocus,
	Object oListView,
	String sHelp,
	Int iChildID,
	Int iControlID,
	Int iWindowType

let hFocus = GetFocus ()
let iWindowType = GetWindowSubtypeCode (hFocus, TRUE)
let iControlID = GetControlID (hFocus)
If iControlID == RevisionLogListViewControlID
&& iWindowType == WT_LISTVIEW then
	let oListView = GetFocusObject (iChildID)
	let sHelp = oListView.accHelp (iChildID)
	If (! nSaySelectAfter) then
		Say (sHelp, OT_ITEM_STATE)
	EndIf
EndIf
SayObjectActiveItem (iPosition)
If iControlID == RevisionLogListViewControlID
&& iWindowType == WT_LISTVIEW
&& nSaySelectAfter then
	Say (sHelp, OT_ITEM_STATE)
EndIf
EndFunction

Int Function HandleCustomWindows (Handle hWnd)
var
	Int iWindowType,
	Int iObjectType,
	Int iType,
	Int iControlID

let iWindowType = GetWindowSubtypeCode (hWnd, TRUE)
let iObjectType = GetObjectSubTypeCode (TRUE)
let iType = iWindowType
If Not iType then
	let iType = iObjectType
EndIf
let iControlID = GetControlID (hWnd)
If iControlID == RevisionLogListViewControlID
&& iWindowType == WT_LISTVIEW then
	IndicateControlType (iWindowType, cScSpace)
	SayObjectActiveItem (TRUE)
	Return (TRUE)
EndIf
If (! IsWindowVisible (GetPriorWindow (hWnd)))
&& GetWindowSubtypeCode (GetPriorWindow (hWnd), TRUE) == WT_STATIC then
	Say (GetGroupBoxName (), OT_CONTROL_GROUP_NAME)
	IndicateControlType (iType, cScSpace)
	If iType == WT_LISTVIEW then
		SayLine (2)
	EndIf
	Return (TRUE)
EndIf
If iWindowType == WT_STATIC
&& iObjectType == WT_LINK then
	Say (GetGroupBoxName (), OT_CONTROL_GROUP_NAME)
	IndicateControlType (iObjectType, GetObjectName (TRUE))
	Return (TRUE)
EndIf
Return (HandleCustomWindows (hWnd))
EndFunction

Script Debug ()
SayString ("Debug.")
SayString (GetObjectHelp ())
SayString (GetObjectDescription (TRUE))
var Object o, Int i
let o = GetFocusObject (i)
SayInteger (i)
SayString (o.accHelp (i))
EndScript



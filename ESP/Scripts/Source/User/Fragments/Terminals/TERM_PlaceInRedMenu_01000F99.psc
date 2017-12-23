;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
Scriptname Fragments:Terminals:TERM_PlaceInRedMenu_01000F99 Extends Terminal Hidden Const

;BEGIN FRAGMENT Fragment_Terminal_01
Function Fragment_Terminal_01(ObjectReference akTerminalRef)
;BEGIN CODE
float val = PIRS_bPlaceInRed.GetValue()

if val == 1
   PIRS_bPlaceInRed.SetValue(0)
elseif val == 0
   PIRS_bPlaceInRed.SetValue(1)
endif
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_02
Function Fragment_Terminal_02(ObjectReference akTerminalRef)
;BEGIN CODE
float val = PIRS_bDisableObjectSnap.GetValue()

if val == 1
   PIRS_bDisableObjectSnap.SetValue(0)
elseif val == 0
   PIRS_bDisableObjectSnap.SetValue(1)
endif
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_03
Function Fragment_Terminal_03(ObjectReference akTerminalRef)
;BEGIN CODE
float val = PIRS_bDisableGroundSnap.GetValue()

if val == 1
   PIRS_bDisableGroundSnap.SetValue(0)
elseif val == 0
   PIRS_bDisableGroundSnap.SetValue(1)
endif
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_04
Function Fragment_Terminal_04(ObjectReference akTerminalRef)
;BEGIN CODE
float val = PIRS_bDisableObjectHighlighting.GetValue()

if val == 1
   PIRS_bDisableObjectHighlighting.SetValue(0)
elseif val == 0
   PIRS_bDisableObjectHighlighting.SetValue(1)
endif
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_07
Function Fragment_Terminal_07(ObjectReference akTerminalRef)
;BEGIN CODE
float val = PIRS_bEnableAchievementsModded.GetValue()

if val == 1
   PIRS_bEnableAchievementsModded.SetValue(0)
elseif val == 0
   PIRS_bEnableAchievementsModded.SetValue(1)
endif
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_08
Function Fragment_Terminal_08(ObjectReference akTerminalRef)
;BEGIN CODE
PIRS_bPlaceInRed.SetValue(1)
PIRS_bDisableObjectSnap.SetValue(0)
PIRS_bDisableGroundSnap.SetValue(0)
PIRS_bDisableObjectHighlighting.SetValue(0)
PIRS_fObjectZoomSpeed.SetValue(10)
PIRS_fObjectRotationSpeed.SetValue(5)
PIRS_bEnableAchievementsModded.SetValue(1)
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

GlobalVariable Property PIRS_bPlaceInRed Auto Const

GlobalVariable Property PIRS_bDisableObjectSnap Auto Const

GlobalVariable Property PIRS_bDisableGroundSnap Auto Const

GlobalVariable Property PIRS_bDisableObjectHighlighting Auto Const

GlobalVariable Property PIRS_bEnableAchievementsModded Auto Const

GlobalVariable Property PIRS_fObjectZoomSpeed Auto Const

GlobalVariable Property PIRS_fObjectRotationSpeed Auto Const

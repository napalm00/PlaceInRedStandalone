;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
Scriptname Fragments:Terminals:TERM_PlaceInRedObjectZoomSpe_01000FA1 Extends Terminal Hidden Const

;BEGIN FRAGMENT Fragment_Terminal_01
Function Fragment_Terminal_01(ObjectReference akTerminalRef)
;BEGIN CODE
float val = PIRS_fObjectZoomSpeed.GetValue()
PIRS_fObjectZoomSpeed.SetValue(val + 1)
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_02
Function Fragment_Terminal_02(ObjectReference akTerminalRef)
;BEGIN CODE
float val = PIRS_fObjectZoomSpeed.GetValue()

if val == 1
   PIRS_fObjectZoomSpeed.SetValue(0)
else
   PIRS_fObjectZoomSpeed.SetValue(val - 1)
endif

akTerminalRef.addtextreplacementData("fObjectZoomSpeed", PIRS_fObjectZoomSpeed)
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_03
Function Fragment_Terminal_03(ObjectReference akTerminalRef)
;BEGIN CODE
PIRS_fObjectZoomSpeed.SetValue(1)
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_04
Function Fragment_Terminal_04(ObjectReference akTerminalRef)
;BEGIN CODE
PIRS_fObjectZoomSpeed.SetValue(10)
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

GlobalVariable Property PIRS_fObjectZoomSpeed Auto Const

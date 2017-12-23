;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
Scriptname Fragments:Terminals:TERM_PlaceInRedObjectRotatio_01000FA4 Extends Terminal Hidden Const

;BEGIN FRAGMENT Fragment_Terminal_01
Function Fragment_Terminal_01(ObjectReference akTerminalRef)
;BEGIN CODE
float val = PIRS_fObjectRotationSpeed.GetValue()
PIRS_fObjectRotationSpeed.SetValue(val + 1)
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_02
Function Fragment_Terminal_02(ObjectReference akTerminalRef)
;BEGIN CODE
float val = PIRS_fObjectRotationSpeed.GetValue()

if val == 1
   PIRS_fObjectRotationSpeed.SetValue(0)
else
   PIRS_fObjectRotationSpeed.SetValue(val - 1)
endif
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_03
Function Fragment_Terminal_03(ObjectReference akTerminalRef)
;BEGIN CODE
PIRS_fObjectRotationSpeed.SetValue(0.5)
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_04
Function Fragment_Terminal_04(ObjectReference akTerminalRef)
;BEGIN CODE
PIRS_fObjectRotationSpeed.SetValue(5)
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

GlobalVariable Property PIRS_fObjectRotationSpeed Auto Const

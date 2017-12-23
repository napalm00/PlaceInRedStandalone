Scriptname PlaceInRedHolotapeInit extends ObjectReference Const

GlobalVariable Property PIRS_bPlaceInRed Auto Const

GlobalVariable Property PIRS_bDisableObjectSnap Auto Const

GlobalVariable Property PIRS_bDisableGroundSnap Auto Const

GlobalVariable Property PIRS_bDisableObjectHighlighting Auto Const

GlobalVariable Property PIRS_fObjectZoomSpeed Auto Const

GlobalVariable Property PIRS_fObjectRotationSpeed Auto Const

GlobalVariable Property PIRS_bEnableAchievementsModded Auto Const

GlobalVariable Property PIRS_InitFromConfig Auto Const

Event OnHolotapePlay(ObjectReference akTerminalRef)

    if PIRS_InitFromConfig.GetValue() == 2
      PIRS_InitFromConfig.SetValue(0)
    endif
    
    if PIRS_InitFromConfig.GetValue() == 0
        PIRS_InitFromConfig.SetValue(1)
        PIRS_bPlaceInRed.SetValue(1)
        PIRS_bDisableObjectSnap.SetValue(0)
        PIRS_bDisableGroundSnap.SetValue(0)
        PIRS_bDisableObjectHighlighting.SetValue(0)
        PIRS_fObjectZoomSpeed.SetValue(10)
        PIRS_fObjectRotationSpeed.SetValue(5)
        PIRS_bEnableAchievementsModded.SetValue(1)
        PIRS_InitFromConfig.SetValue(0)
    endif

    akTerminalRef.addtextreplacementData("bPlaceInRed", PIRS_bPlaceInRed)
    akTerminalRef.addtextreplacementData("bDisableObjectSnap", PIRS_bDisableObjectSnap )
    akTerminalRef.addtextreplacementData("bDisableGroundSnap", PIRS_bDisableGroundSnap )
    akTerminalRef.addtextreplacementData("bDisableObjectHighlighting", PIRS_bDisableObjectHighlighting )
    akTerminalRef.addtextreplacementData("fObjectZoomSpeed", PIRS_fObjectZoomSpeed )
    akTerminalRef.addtextreplacementData("fObjectRotationSpeed", PIRS_fObjectRotationSpeed )
    akTerminalRef.addtextreplacementData("bEnableAchievementsModded", PIRS_bEnableAchievementsModded )
EndEvent

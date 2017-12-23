Scriptname PlaceInRedQuest extends Quest

Holotape Property PlaceInRedHolotape Auto Const

Event OnInit()
	Self.AddPlaceInRedHolotape()
EndEvent

Function AddPlaceInRedHolotape()
	If (Game.GetPlayer().GetItemCount(PlaceInRedHolotape as Form) == 0)
		Game.GetPlayer().AddItem(PlaceInRedHolotape as Form, 1, True)
	EndIf
EndFunction

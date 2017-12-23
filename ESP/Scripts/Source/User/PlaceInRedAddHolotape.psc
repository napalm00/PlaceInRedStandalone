Scriptname PlaceInRedAddHolotape extends Actor Const
import Game

Event OnLoad()
  Game.GetPlayer().addItem(PlaceInRedStandaloneSettingsHolotape)
endEvent
Holotape Property PlaceInRedStandaloneSettingsHolotape Auto Const

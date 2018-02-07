![Place in Red: Standalone (VR)](https://i.imgur.com/Neqr57p.jpg)

# Place In Red: Standalone (FO4VR Only)
Standalone (no Cheat Engine required) version of [RandyConstan](https://www.nexusmods.com/fallout4/users/29852470)'s excellent [Place in Red](https://www.nexusmods.com/fallout4/mods/28570/) mod.
Released with his permission and collaboration.

## Features
* **No Cheat Engine required!**
* Easy drag & drop installation
* Place workshop objects anywhere (within workshop bounds)
* Disable object/ground snapping
* Disable object highlight when using the workshop
* Set zoom/rotation speed when placing the object in the workshop
* Enable achievements for modded games
* Settings holotape given to the player automatically to configure the various parameters
* Settings saved in .ini file within the `Fallout 4 VR/Data` folder, they can be edited manually or through the holotape

## Installation
1. Download the latest version from the [releases page](https://github.com/napalm00/PlaceInRedStandalone/releases)
2. Unzip the contents in your `Fallout 4 VR` directory, making sure you replace `steam_api64.dll` and that, after unpacking, you also have `steam_api64_org.dll`
3. Load the .esp like any other mod
4. Start the game and ejoy!

## Settings
To change the settings you can:
* Play the "Place in Red Settings" holotape in a Terminal (using the Pip-Boy will NOT work, see [Known Issues](https://github.com/napalm00/PlaceInRedStandalone#known-issues))
* Edit the `Fallout 4 VR/Data/PlaceInRedStandalone.ini` file

The settings and their default values are:

* Place In Red Enabled (place workshop objects anywhere) (default: ON)
* Disable Object Snap (default: OFF)
* Disable Ground Snap (default: OFF)
* Disable Object Highlighting (default: OFF)
* Object Zoom Speed (default: 10.0)
* Object Rotation Speed (default: 5.0)
* Enable Modded Achievements (default: ON)


## Compiling from source
To compile the DLL from source, use Visual Studio 2017. 
Note that if Visual Studio updates their compiler, you will need to recompile MinHook for the version you're using.
To do this, go to the [MinHook releases page](https://github.com/TsudaKageyu/minhook/releases), open the project in VS and build the **static** `.lib`.

To compile the `.esp`, use the [Creation Kit](https://www.creationkit.com/fallout4/index.php?title=Main_Page) from Bethesda. The project and source files are in the `ESP` folder of the repository.

## Known Issues
* The settings holotape can only be played in a Terminal, not the Pip-Boy. This is a known limitation of the engine.

## Changelog
**1.2:**
* Fixed error caused by the beta update ([issue #2](https://github.com/napalm00/PlaceInRedStandalone/issues/2))

**1.1:**
* Fixed object outline bug ([issue #1](https://github.com/napalm00/PlaceInRedStandalone/issues/1))

**1.0:**
* Initial release

## Credits
* [RandyConstan](https://www.nexusmods.com/fallout4/users/29852470): Making the original Place in Red mod, add-holotape-to-inventory script, lots of support with the Creation Kit
* [naPalm](https://github.com/napalm00): Porting Place in Red to a standalone DLL mod, reversing globals, settings holotape
* gir489/NTAuthority/DarthTon/Forza: The various pattern scanning functions in `Pattern.h`
* [Tsuda Kageyu](https://github.com/TsudaKageyu): [MinHook library](https://github.com/TsudaKageyu/minhook)
* [Brodie Thiesfield](https://github.com/brofield): [SimpleIni library](https://github.com/brofield/simpleini)

## Licenses
* Place in Red Standalone: [MIT](https://opensource.org/licenses/MIT)
* MinHook: [BSD 2-Clause](https://opensource.org/licenses/BSD-2-Clause)
* SimpleIni: [MIT](https://opensource.org/licenses/MIT)

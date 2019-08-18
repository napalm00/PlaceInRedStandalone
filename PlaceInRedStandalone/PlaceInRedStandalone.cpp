/*
* The MIT License
* Copyright 2017-2019 naPalm / PapaRadroach
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute,
* sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
* LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
* THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "PlaceInRedStandalone.h"

PlaceInRedStandalone::SetGlobalValue PlaceInRedStandalone::m_pSetGlobalValueOrig = NULL;

int __fastcall SetGlobalValueHook(DWORD64* unk1, unsigned int unk2, DWORD64* globalValuePtr, float newValue)
{
	static bool bIsInitializing = false;
	PlaceInRedStandalone::GlobalData* pGlobalData = (PlaceInRedStandalone::GlobalData*)globalValuePtr;
	std::string sName = std::string(pGlobalData->name);

	// is_const = (*(DWORD64*)(globalValuePtr + 16) >> 6) & 1 ? 1 : 0

#ifdef DEBUG
	Logging::LogMessage("sName = %s\r\n", sName.c_str());
#endif
	if(sName.compare(0, GLOBAL_PREFIX.length(), GLOBAL_PREFIX) == 0) // begins with GLOBAL_PREFIX
	{
		std::string sSettingName = sName.substr(GLOBAL_PREFIX.length());
		std::string sSectionName = "Workshop";

#ifdef DEBUG
		Logging::LogMessage("sSettingName = %s\r\n", sSettingName.c_str());
#endif

		if(sSettingName == "InitFromConfig")
		{
			bIsInitializing = (newValue == 1);
#ifdef DEBUG
			Logging::LogMessage("InitFromConfig bIsInitializing= %i\r\n", bIsInitializing);
#endif

			return PlaceInRedStandalone::m_pSetGlobalValueOrig(unk1, unk2, globalValuePtr, newValue);
		}

		std::shared_ptr<ConfigurationManager> ConfigManager = PlaceInRedStandalone::GetInstance().GetConfigurationManager();
#ifdef DEBUG
		Logging::LogMessage("ConfigManager ok\r\n");
#endif
		if(sSettingName == "bPlaceInRed")
		{
			if(bIsInitializing)
			{
				newValue = ConfigManager->GetBool("bPlaceInRed");
			}
			else
			{
				if(newValue == 1.0)
				{
					PlaceInRedStandalone::GetInstance().EnablePlaceInRed();
				}
				else
				{
					PlaceInRedStandalone::GetInstance().DisablePlaceInRed();
				}
			}
		}
		else if(sSettingName == "bDisableObjectSnap")
		{
			if(bIsInitializing)
			{
				newValue = ConfigManager->GetBool("bDisableObjectSnap");
			}
			else
			{
				if(newValue == 1.0)
				{
					PlaceInRedStandalone::GetInstance().DisableObjectSnap();
#ifdef DEBUG
					Logging::LogMessage("DisableObjectSnap ok\r\n");
#endif
				}
				else
				{
					PlaceInRedStandalone::GetInstance().EnableObjectSnap();
				}
			}
		}
		else if(sSettingName == "bDisableGroundSnap")
		{
			if(bIsInitializing)
			{
				newValue = ConfigManager->GetBool("bDisableGroundSnap");
			}
			else
			{
				if(newValue == 1.0)
				{
					PlaceInRedStandalone::GetInstance().DisableGroundSnap();
				}
				else
				{
					PlaceInRedStandalone::GetInstance().EnableGroundSnap();
				}
			}
		}
		else if(sSettingName == "bDisableObjectHighlighting")
		{
			if(bIsInitializing)
			{
				newValue = ConfigManager->GetBool("bDisableObjectHighlighting");
			}
			else
			{
				if(newValue == 1.0)
				{
					PlaceInRedStandalone::GetInstance().DisableObjectHighlighting();
				}
				else
				{
					PlaceInRedStandalone::GetInstance().EnableObjectHighlighting();
				}
			}
		}
		else if(sSettingName == "fObjectZoomSpeed")
		{
			if(bIsInitializing)
			{
				newValue = ConfigManager->GetFloat("fObjectZoomSpeed");
			}
			else
			{
				PlaceInRedStandalone::GetInstance().SetObjectZoomSpeed(newValue);
			}
		}
		else if(sSettingName == "fObjectRotationSpeed")
		{
			if(bIsInitializing)
			{
				newValue = ConfigManager->GetFloat("fObjectRotationSpeed");
			}
			else
			{
				PlaceInRedStandalone::GetInstance().SetObjectRotateSpeed(newValue);
			}
		}
		else if(sSettingName == "bEnableAchievementsModded")
		{
			sSectionName = "Misc";

			if(bIsInitializing)
			{
				newValue = ConfigManager->GetBool("bEnableAchievementsModded");
			}
			else
			{
				if(newValue == 1.0)
				{
					PlaceInRedStandalone::GetInstance().EnableModdedAchievements();
				}
				else
				{
					PlaceInRedStandalone::GetInstance().DisableModdedAchievements();
				}
			}
		}

		if(!bIsInitializing)
		{
			ConfigManager->SaveSetting(sSectionName.c_str(), sSettingName.c_str(), std::to_string(newValue).c_str());
#ifdef DEBUG
			Logging::LogMessage("SaveSetting ok\r\n");
#endif
		}

#ifdef DEBUG
		Logging::LogMessage("unk1 = %X, sName = %s, sSettingName = %s, value = %f\r\n", unk1, sName.c_str(), sSettingName.c_str(), newValue);
#endif
	}

	return PlaceInRedStandalone::m_pSetGlobalValueOrig(unk1, unk2, globalValuePtr, newValue);
}


bool PlaceInRedStandalone::Init(MODULEINFO mainModuleInfo)
{
	m_pConfigurationManager.reset(new ConfigurationManager(INI_PATH));

	if(MH_Initialize() != MH_OK)
	{
		MessageBoxA(NULL, "MinHook initialization failed", TITLE, MB_ICONERROR);
		return false;
	}

#ifdef DEBUG
	Logging::LogMessage("MH_Initialize OK\r\n");
#endif

	if(!m_pConfigurationManager->Init())
	{
		return false;
	}

#ifdef DEBUG
	Logging::LogMessage("ConfigurationManager OK\r\n");
#endif

	/*m_PointersMap["PTR01_A"] = PointerInfo(NULL, "C6 05 ? ? ? ? 01 84 C0 75 ? B1");
	m_PointersMap["PTR02_A"] = PointerInfo(NULL, "40 88 35 ? ? ? ? C6 05 ? ? ? ? 01 40 88 35 ? ? ? ? 48 85 FF");
	m_PointersMap["PTR02_B"] = PointerInfo(NULL, "B2 01 88 15 ? ? ? ? EB 04 84 D2 74 07 C6 85");
	m_PointersMap["PTR02_C"] = PointerInfo(NULL, "74 0C B0 01 B1 03 88 05 ? ? ? ? EB 0F 32 C0");
	m_PointersMap["PTR02_E"] = PointerInfo(NULL, "0F 95 05 ? ? ? ? E8 ? ? ? ? 40 38 3D");
	m_PointersMap["PTR03_A"] = PointerInfo(NULL, "41 B8 09 00 00 00 84 D2 41 0F 44 C8 88 0D");
	m_PointersMap["PTR03_B"] = PointerInfo(NULL, "88 0D ? ? ? ? F3 0F 10 05");
	m_PointersMap["PTR03_C"] = PointerInfo(NULL, "88 05 ? ? ? ? 48 85 ED 0F 84 ? ? ? ? 48 8B CD E8 ? ? ? ? 84 C0 0F 84 ? ? ? ? 40 38 35");
	m_PointersMap["PTR03_D"] = PointerInfo(NULL, "B8 07 00 00 00 40 84 FF 0F 44 C8 88 0D");
	m_PointersMap["PTR03_E"] = PointerInfo(NULL, "B9 04 00 00 00 0F B6 F8 0F B6 05 ? ? ? ? 40 84 FF 0F 44 C1 88");
	m_PointersMap["PTR03_F"] = PointerInfo(NULL, "C6 05 ? ? ? ? 00 48 C7 45 C7 ? ? ? ? E8 ? ? ? ? 48 8B 5D C7 83 CE FF 48 85 DB 0F 84");
	m_PointersMap["PTR03_H"] = PointerInfo(NULL, "C6 05 ? ? ? ? 03 48 8B 05 ? ? ? ? 80");
	m_PointersMap["PTR03_J"] = PointerInfo(NULL, "C6 05 ? ? ? ? 06 E9 ? ? ? ? 45 84 F6");
	m_PointersMap["PTR03_K"] = PointerInfo(NULL, "C6 05 ? ? ? ? 08 E9 ? ? ? ? 45 32 F6 48 8B 8D");
	m_PointersMap["PTR03_L"] = PointerInfo(NULL, "C6 05 ? ? ? ? 0A E9 ? ? ? ? 45");
	m_PointersMap["PTR03_M"] = PointerInfo(NULL, "C6 05 ? ? ? ? 0B E9 ? ? ? ? 45");
	m_PointersMap["PTR03_N"] = PointerInfo(NULL, "C6 05 ? ? ? ? 0C E9 ? ? ? ? C6");
	m_PointersMap["PTR03_O"] = PointerInfo(NULL, "C6 05 ? ? ? ? 0D");
	m_PointersMap["PTR04_A"] = PointerInfo(NULL, "88 05 ? ? ? ? 88 44 24 70 E8 ? ? ? ? 48 8D 54 24 70");
	m_PointersMap["YELLOW"] = PointerInfo(NULL, "8B 58 14 48 8D 4C 24 30 45 33 C0 8B D3 E8");
	m_PointersMap["RED"] = PointerInfo(NULL, "C6 05 ? ? ? ? 01 F3 0F 11 05 ? ? ? ? 0F 28");
	m_PointersMap["WORKSHOPTIMER"] = PointerInfo(NULL, "48 81 EC 38 01 00 00 48 85 C9 0F 84 E5 00 00 00");
	m_PointersMap["_aob_object_snap"] = PointerInfo(NULL, "F3 0F 10 05 ? ? ? ? 89 A9 ? ? ? ? 66 89 A9");
	m_PointersMap["_aob_object_snapavskip"] = PointerInfo(NULL, "41 0F 2F C0 F3 0F 11 87 ? ? ? ? 77 10 F3 0F 10 05");
	m_PointersMap["_aob_groundsnap"] = PointerInfo(NULL, "86 ? ? ? ? 49 8B 8E ? ? ? ? 4C 8D 4C 24 60 45 33 C0 49 8B D5 E8 ? ? ? ? 84 C0");
	m_PointersMap["_aob_zoom"] = PointerInfo(NULL, "F3 0F 10 05 ? ? ? ? 0F 29 74 24 20 F3 0F 10 35 ? ? ? ? F3 0F 59 C2");
	m_PointersMap["_aob_rotate"] = PointerInfo(NULL, "F3 0F 10 0D ? ? ? ? F3 0F 59 0D ? ? ? ? F3 0F 59 0D ? ? ? ? 84 C9 75 07");
	m_PointersMap["_aob_outlines"] = PointerInfo(NULL, "C6 05 ? ? ? ? 01 88 15 ? ? ? ? 76 13 48 8B 05");
	m_PointersMap["_aob_achievements"] = PointerInfo(NULL, "40 57 48 83 EC 30 40 32 FF 84 D2 74 20 48");
	m_PointersMap["set_global_value"] = PointerInfo(NULL, "48 89 ? ? 08 57 48 83 ? ? 41 8B ? ? 8B FA 48 8B D9 C1 ? ? A8 01");*/

    m_PointersMap["PTR01_A"] = PointerInfo(NULL, "C6 05 ? ? ? ? 01 84 C0 75 ? B1");
    m_PointersMap["PTR02_A"] = PointerInfo(NULL, "40 88 35 ? ? ? ? C6 05 ? ? ? ? 01 40 88 35 ? ? ? ? 48 85 DB");
    m_PointersMap["PTR02_B"] = PointerInfo(NULL, "88 15 ? ? ? ? EB 04 84 D2 74 07 C6 85 ? ? ? ? 00 84 D2 F3");
    m_PointersMap["PTR02_C"] = PointerInfo(NULL, "45 84 F6 74 11 40 84 F6 74 0C B0 01 B1 03 88 05 ? ? ? ? EB 0F 32 C0");
    m_PointersMap["PTR02_D"] = PointerInfo(NULL, "88 05 ? ? ? ? 0F B6 0D ? ? ? ? 0F B6 15 ? ? ? ? 0F B6 C9 41 B8 ? ? ? ? 84 D2");
    //m_PointersMap["PTR02_E"] = PointerInfo(NULL, "EB 05 E8 ? ? ? ? 83 BD ? ? ? ? 00 48 8D 8D ? ? ? ? 0F 95 05 ? ? ? ? E8");
    m_PointersMap["PTR02_E"] = PointerInfo(NULL, "0F 95 05 ? ? ? ? E8 ? ? ? ? 40 38 3D");
    m_PointersMap["PTR03_A"] = PointerInfo(NULL, "88 0D ? ? ? ? 45 84 F6 0F 85 ? ? ? ? 84 D2 0F 84 ? ? ? ? 45 84 ED");
    m_PointersMap["PTR03_B"] = PointerInfo(NULL, "88 0D ? ? ? ? F3 0F 10 05");
    m_PointersMap["PTR03_C"] = PointerInfo(NULL, "88 05 ? ? ? ? 48 85 ED 0F 84 ? ? ? ? 48 8B CD E8 ? ? ? ? 84 C0 0F 84 ? ? ? ? 40 38 35");
    m_PointersMap["PTR03_D"] = PointerInfo(NULL, "88 0D ? ? ? ? 48 8B 0D ? ? ? ? F3 41 0F 10 06 F3 41 0F 10 4E 04 F3 0F 11 44 24 60");
    m_PointersMap["PTR03_E"] = PointerInfo(NULL, "88 05 ? ? ? ? 48 8B B4 24 ? ? ? ? 48 8D 95 ? ? ? ? 48 8D 0D ? ? ? ? 48 C7 85");
    m_PointersMap["PTR03_F"] = PointerInfo(NULL, "C6 05 ? ? ? ? 00 48 C7 45 C7 ? ? ? ? E8 ? ? ? ? 48 8B 5D C7 83 CE FF 48 85 DB 0F 84");
    //m_PointersMap["PTR03_H"] = PointerInfo(NULL, "C6 05 ? ? ? ? 03 B9 ? ? ? ? B8 ? ? ? ? 40 84 FF 0F 45 C8 E8 ? ? ? ? 83 3D ? ? ? ? 00");
    m_PointersMap["PTR03_H"] = PointerInfo(NULL, "C6 05 ? ? ? ? 03 48 8B 05 ? ? ? ? 80");
    m_PointersMap["PTR03_I"] = PointerInfo(NULL, "C6 05 ? ? ? ? 05 E9 ? ? ? ? 48 8B 0D ? ? ? ? 40 32 FF 48 8B 71 58 48 85 F6 0F 95 C0 88 85");
    m_PointersMap["PTR03_J"] = PointerInfo(NULL, "C6 05 ? ? ? ? 06 E9 ? ? ? ? 45 84 F6 0F 84 ? ? ? ? 48 8D 94 24 ? ? ? ? 48 8D 0D");
    m_PointersMap["PTR03_K"] = PointerInfo(NULL, "C6 05 ? ? ? ? 08 E9 ? ? ? ? 45 32 F6 48 8B 8D ? ? ? ? 4C 8D 4C 24 38 45 33 C0 33 D2 48");
    m_PointersMap["PTR03_L"] = PointerInfo(NULL, "C6 05 ? ? ? ? 0A E9 ? ? ? ? 45 84 ED 75 3D 44 38 2D ? ? ? ? 74 0F 40 32 FF");
    m_PointersMap["PTR03_M"] = PointerInfo(NULL, "C6 05 ? ? ? ? 0B E9 ? ? ? ? 45 84 ED 75 20 48 8D 54 24 40 48 8B CB E8 ? ? ? ? 84 C0");
    m_PointersMap["PTR03_N"] = PointerInfo(NULL, "C6 05 ? ? ? ? 0C E9 ? ? ? ? C6 05 ? ? ? ? 01 45 84 ED 74 0D");
    m_PointersMap["PTR03_O"] = PointerInfo(NULL, "C6 05 ? ? ? ? 0D 40 32 FF E9 ? ? ? ? 0F B6 05 ? ? ? ? 0F B6 B5 ? ? ? ? 84 C0"); //
    m_PointersMap["PTR04_A"] = PointerInfo(NULL, "88 05 ? ? ? ? 88 44 24 70 E8 ? ? ? ? 48 8D 54 24 70 48 8B C8 E8 ? ? ? ? 48 85 DB");
    m_PointersMap["RED"] = PointerInfo(NULL, "C6 05 ? ? ? ? 01 F3 0F 11 05 ? ? ? ? 0F 28 74 24 40 44 0F 28 44 24 30"); //Red to green
    m_PointersMap["YELLOW"] = PointerInfo(NULL, "C1 48 85 C0 74 07 E8 ? ? ? ? EB 07 48 8B 81 ? ? ? ? 8B 58 14"); //Allow movement of temporary yellow scrap only objects
    m_PointersMap["_aob_object_snap"] = PointerInfo(NULL, "F3 0F 10 05 ? ? ? ? 89 A9 ? ? ? ? 66 89 A9");
    m_PointersMap["_aob_object_snapavskip"] = PointerInfo(NULL, "41 0F 2F C0 F3 0F 11 87 ? ? ? ? 77 10 F3 0F 10 05");
    m_PointersMap["_aob_groundsnap"] = PointerInfo(NULL, "0F 86 ? ? ? ? 49 8B 8E ? ? ? ? 4C 8D 4C 24 60 45 33 C0 49 8B D5 E8");
    m_PointersMap["_aob_zoom"] = PointerInfo(NULL, "F3 0F 10 05 ? ? ? ? 0F 29 74 24 20 F3 0F 10 35 ? ? ? ? F3 0F 59 C2");
    m_PointersMap["_aob_rotate"] = PointerInfo(NULL, "F3 0F 10 0D ? ? ? ? F3 0F 59 0D ? ? ? ? F3 0F 59 0D ? ? ? ? 84 C9 75 07");
    m_PointersMap["_aob_outlines"] = PointerInfo(NULL, "C6 05 ? ? ? ? 01 88 15 ? ? ? ? 76 13 48 8B 05");

    /*
    Achievements

    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8AE0 - 40 57                 - push rdi
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8AE2 - 48 83 EC 30           - sub rsp,30 { 48 }
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8AE6 - 40 32 FF              - xor dil,dil
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8AE9 - 84 D2                 - test dl,dl
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8AEB - 74 20                 - je Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8B0D
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8AED - 48 8B 05 2C4A9E05     - mov rax,[Fallout4VR.exe+5B043F0] { [1C45F53D980] }
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8AF4 - 48 85 C0              - test rax,rax
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8AF7 - 74 11                 - je Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8B0A
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8AF9 - F6 80 A2120000 40     - test byte ptr [rax+000012A2],40 { 64 }
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8B00 - 74 08                 - je Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8B0A
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8B02 - B0 01                 - mov al,01 { 1 }
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8B04 - 48 83 C4 30           - add rsp,30 { 48 }
    Fallout4VR.Scaleform::Render::Matrix4x4<float>::SetIdentity+F8B08 - 5F                    - pop rdi
    */
    m_PointersMap["_aob_achievements"] = PointerInfo(NULL, "40 57 48 83 EC 30 40 32 FF 84 D2 74 20 48");

    /*
    SetGlobalValue

    48 89 ? ? 08 57 48 83 ? ? 41 8B ? ? 8B FA 48 8B D9 C1 ? ? A8 01

    <Address>Fallout4VR.exe+1454B50</Address>


    Fallout4.flexMakeShapeFlags+119B270 - 48 89 5C 24 08        - mov [rsp+08],rbx
    Fallout4.flexMakeShapeFlags+119B275 - 57                    - push rdi
    Fallout4.flexMakeShapeFlags+119B276 - 48 83 EC 40           - sub rsp,40 { 64 }
    Fallout4.flexMakeShapeFlags+119B27A - 41 8B 40 10           - mov eax,[r8+10]
    Fallout4.flexMakeShapeFlags+119B27E - 8B FA                 - mov edi,edx
    Fallout4.flexMakeShapeFlags+119B280 - 48 8B D9              - mov rbx,rcx
    Fallout4.flexMakeShapeFlags+119B283 - C1 E8 06              - shr eax,06 { 6 }
    Fallout4.flexMakeShapeFlags+119B286 - A8 01                 - test al,01 { 1 }
    Fallout4.flexMakeShapeFlags+119B288 - 75 11                 - jne Fallout4.flexMakeShapeFlags+119B29B
    Fallout4.flexMakeShapeFlags+119B28A - F3 41 0F11 58 30      - movss [r8+30],xmm3
    Fallout4.flexMakeShapeFlags+119B290 - 48 8B 5C 24 50        - mov rbx,[rsp+50]
    Fallout4.flexMakeShapeFlags+119B295 - 48 83 C4 40           - add rsp,40 { 64 }
    Fallout4.flexMakeShapeFlags+119B299 - 5F                    - pop rdi
    Fallout4.flexMakeShapeFlags+119B29A - C3                    - ret
    */
    m_PointersMap["set_global_value"] = PointerInfo(NULL, "48 89 ? ? 08 57 48 83 ? ? 41 8B ? ? 8B FA 48 8B D9 C1 ? ? A8 01");

	if(!MemoryUtils::BruteForceScanFastAllSignatures(m_PointersMap, mainModuleInfo, MAX_TRIES, SIGSCAN_DELAY))
	{
		return false;
	}

#ifdef DEBUG
	Logging::LogMessage("BruteForceScanFastAllSignatures done\r\n");
#endif

	// This delay is needed otherwise the hook won't work. 
	// I have no idea why and I can't debug it properly either because adding
	// a log message adds itself a delay, making everything work perfectly when it 
	// should in fact not work. If anyone has a better solution for this I'll gladly
	// accept any pull requests on the github.
	// - naPalm
	Sleep(2000);

	MH_STATUS status = MH_OK;
	if((status = MH_CreateHook((void*)m_PointersMap["set_global_value"].address, &SetGlobalValueHook,
		reinterpret_cast<LPVOID*>(&m_pSetGlobalValueOrig))) != MH_OK)
	{
		char errorMessage[128];
		sprintf_s(errorMessage, sizeof(errorMessage) / sizeof(errorMessage[0]), "MinHook hook creation failed: SetGlobalValueHook (status = %i)", status);
		MessageBoxA(NULL, errorMessage, TITLE, MB_ICONERROR);
		return false;
	}

#ifdef DEBUG
	Logging::LogMessage("SetGlobalValueHook OK\r\n");
#endif

	if(MH_EnableHook((void*)m_PointersMap["set_global_value"].address) != MH_OK)
	{
		MessageBoxA(NULL, "MinHook hook enabling failed: SetGlobalValueHook", TITLE, MB_ICONERROR);
		return false;
	}

#ifdef DEBUG
	Logging::LogMessage("All hooks enabled\r\n");
#endif

	if(m_pConfigurationManager->GetBool("bPlaceInRed"))
	{
#ifdef DEBUG
        Logging::LogMessage("before EnablePlaceInRed\r\n");
#endif
		EnablePlaceInRed();
	}

	if(m_pConfigurationManager->GetBool("bDisableObjectSnap"))
	{
#ifdef DEBUG
        Logging::LogMessage("before DisableObjectSnap\r\n");
#endif
		DisableObjectSnap();
	}

	if(m_pConfigurationManager->GetBool("bDisableGroundSnap"))
	{
#ifdef DEBUG
        Logging::LogMessage("before DisableGroundSnap\r\n");
#endif
		DisableGroundSnap();
	}

	if(m_pConfigurationManager->GetBool("bDisableObjectHighlighting"))
	{
#ifdef DEBUG
        Logging::LogMessage("before DisableObjectHighlighting\r\n");
#endif
		DisableObjectHighlighting();
	}

#ifdef DEBUG
    Logging::LogMessage("before SetObjectZoomSpeed\r\n");
#endif
	SetObjectZoomSpeed(m_pConfigurationManager->GetFloat("fObjectZoomSpeed"));

#ifdef DEBUG
    Logging::LogMessage("before SetObjectRotateSpeed\r\n");
#endif
	SetObjectRotateSpeed(m_pConfigurationManager->GetFloat("fObjectRotationSpeed"));

	if(m_pConfigurationManager->GetBool("bEnableAchievementsModded"))
	{
#ifdef DEBUG
        Logging::LogMessage("before EnableModdedAchievements\r\n");
#endif
		EnableModdedAchievements();
	}

	return true;
}

std::shared_ptr<ConfigurationManager> PlaceInRedStandalone::GetConfigurationManager()
{
	return m_pConfigurationManager;
}
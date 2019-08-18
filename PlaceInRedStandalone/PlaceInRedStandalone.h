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

#pragma once
#include "stdafx.h"
#include "common.h"
#include <memory>
#include "ConfigurationManager.h"
#include "MemoryUtils.h"

#ifdef DEBUG
#include "Logging.h"
#endif

class PlaceInRedStandalone
{
private:
	std::map<std::string, PointerInfo> m_PointersMap;
	std::shared_ptr<ConfigurationManager> m_pConfigurationManager;

	PlaceInRedStandalone(){}

public: 
	PlaceInRedStandalone(PlaceInRedStandalone const&) = delete;
	void operator=(PlaceInRedStandalone const&) = delete;

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
	typedef int(__fastcall *SetGlobalValue)(DWORD64*, unsigned int, DWORD64*, float);

	typedef struct
	{
		UINT8 pad0[0x20];
		char* name; // 0x20
		UINT8 pad1[0x08];
		float value; // 0x30
	} GlobalData;

	// Original function
	static SetGlobalValue m_pSetGlobalValueOrig;
	bool Init(MODULEINFO mainModuleInfo);

	std::shared_ptr<ConfigurationManager> GetConfigurationManager();

	static PlaceInRedStandalone& GetInstance()
	{
		static PlaceInRedStandalone    instance; // Guaranteed to be destroyed.
							  // Instantiated on first use.
		return instance;
	}

	void EnablePlaceInRed()
	{
		/*MemoryUtils::PatchMemory(m_PointersMap["PTR01_A"].address + 0x06, {0x00});
		MemoryUtils::PatchMemory(m_PointersMap["PTR01_A"].address + 0x0C, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR02_B"].address + 0x01, {0x00});
		MemoryUtils::PatchMemory(m_PointersMap["PTR02_C"].address, {0xEB});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_A"].address + 0x02, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_D"].address + 0x01, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_E"].address + 0x01, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_F"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_H"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_J"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_K"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_L"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_M"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_N"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_O"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["RED"].address + 0x06, {0x00});
		MemoryUtils::PatchMemory(m_PointersMap["YELLOW"].address, {0x90, 0x90, 0x90});
		MemoryUtils::PatchMemory(m_PointersMap["WORKSHOPTIMER"].address + 0x0A, {0xE9, 0xE6, 0x00, 0x00, 0x00, 0x90});*/

        MemoryUtils::PatchMemory(m_PointersMap["PTR01_A"].address + 0x06, {0x00});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_A"].address, {0xEB, 0x05});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_B"].address, {0xEB, 0x04});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_C"].address + 0x0E, {0xEB, 0x04});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_D"].address, {0xEB, 0x04});
        //MemoryUtils::PatchMemory(m_PointersMap["PTR02_E"].address + 0x15, {0xEB, 0x05});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_E"].address, {0xEB, 0x05});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_A"].address, {0xEB, 0x04});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_B"].address, {0xEB, 0x04});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_C"].address, {0xEB, 0x04});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_D"].address, {0xEB, 0x04});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_E"].address, {0xEB, 0x04});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_F"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_H"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_I"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_J"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_K"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_L"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_M"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_N"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_O"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR04_A"].address, {0xEB, 0x04});
        MemoryUtils::PatchMemory(m_PointersMap["RED"].address + 0x06, {0x00});
        MemoryUtils::PatchMemory(m_PointersMap["YELLOW"].address + 0x14, {0x90, 0x90, 0x90});
	}

	void DisablePlaceInRed()
	{
		/*MemoryUtils::PatchMemory(m_PointersMap["PTR01_A"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR01_A"].address + 0x0C, {0x02});
		MemoryUtils::PatchMemory(m_PointersMap["PTR02_B"].address + 0x01, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["PTR02_C"].address, {0x74});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_A"].address + 0x02, {0x09});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_D"].address + 0x01, {0x07});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_E"].address + 0x01, {0x04});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_F"].address + 0x06, {0x00});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_H"].address + 0x06, {0x03});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_J"].address + 0x06, {0x06});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_K"].address + 0x06, {0x08});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_L"].address + 0x06, {0x0A});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_M"].address + 0x06, {0x0B});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_N"].address + 0x06, {0x0C});
		MemoryUtils::PatchMemory(m_PointersMap["PTR03_O"].address + 0x06, {0x0D});
		MemoryUtils::PatchMemory(m_PointersMap["RED"].address + 0x06, {0x01});
		MemoryUtils::PatchMemory(m_PointersMap["YELLOW"].address, {0x8B, 0x58, 0x14});
		MemoryUtils::PatchMemory(m_PointersMap["WORKSHOPTIMER"].address + 0x0A, {0x0F, 0x84, 0xE5, 0x00, 0x00, 0x00});*/

        MemoryUtils::PatchMemory(m_PointersMap["PTR01_A"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_A"].address, {0x40, 0x88});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_B"].address, {0x88, 0x15});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_C"].address + 0x0E, {0x88, 0x05});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_D"].address, {0x88, 0x05});
        //MemoryUtils::PatchMemory(m_PointersMap["PTR02_E"].address + 0x15, {0x0F, 0x95});
        MemoryUtils::PatchMemory(m_PointersMap["PTR02_E"].address, {0x0F, 0x95});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_A"].address, {0x88, 0x0D});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_B"].address, {0x88, 0x0D});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_C"].address, {0x88, 0x05});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_D"].address, {0x88, 0x0D});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_E"].address, {0x88, 0x05});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_F"].address + 0x06, {0x00});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_H"].address + 0x06, {0x03});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_I"].address + 0x06, {0x05});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_J"].address + 0x06, {0x06});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_K"].address + 0x06, {0x08});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_L"].address + 0x06, {0x0A});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_M"].address + 0x06, {0x0B});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_N"].address + 0x06, {0x0C});
        MemoryUtils::PatchMemory(m_PointersMap["PTR03_O"].address + 0x06, {0x0D});
        MemoryUtils::PatchMemory(m_PointersMap["PTR04_A"].address, {0x88, 0x05});
        MemoryUtils::PatchMemory(m_PointersMap["RED"].address + 0x06, {0x01});
        MemoryUtils::PatchMemory(m_PointersMap["YELLOW"].address + 0x14, {0x88, 0x58, 0x14});
	}

	void EnableObjectSnap()
	{
        *(float*)(m_PointersMap["_aob_object_snap"].address + ((*(UINT32*)((UINT8*)(m_PointersMap["_aob_object_snap"].address + 4))) + 8)) = 0.0f;
        MemoryUtils::PatchMemory(m_PointersMap["_aob_object_snapavskip"].address + 0x0C, {0xEB, 0x01});
	}

	void DisableObjectSnap()
	{
        *(float*)(m_PointersMap["_aob_object_snap"].address + ((*(UINT32*)((UINT8*)(m_PointersMap["_aob_object_snap"].address + 4))) + 8)) = 48.0f;
        MemoryUtils::PatchMemory(m_PointersMap["_aob_object_snapavskip"].address + 0x0C, {0x77, 0x10});
	}

	void EnableGroundSnap()
	{
		MemoryUtils::PatchMemory(m_PointersMap["_aob_groundsnap"].address, {0x0F, 0x86});
	}

	void DisableGroundSnap()
	{
		MemoryUtils::PatchMemory(m_PointersMap["_aob_groundsnap"].address, {0x0F, 0x85});
	}

	void EnableObjectHighlighting()
	{
		MemoryUtils::PatchMemory(m_PointersMap["_aob_outlines"].address + 0x06, {0x01});
	}

	void DisableObjectHighlighting()
	{
		MemoryUtils::PatchMemory(m_PointersMap["_aob_outlines"].address + 0x06, {0x00});
	}

	void SetObjectZoomSpeed(float value)
	{
		*(float*)(m_PointersMap["_aob_zoom"].address + ((*(UINT32*)((UINT8*)(m_PointersMap["_aob_zoom"].address + 4))) + 8)) = value;
	}

	void SetObjectRotateSpeed(float value)
	{
		*(float*)(m_PointersMap["_aob_rotate"].address + ((*(UINT32*)((UINT8*)(m_PointersMap["_aob_rotate"].address + 4))) + 8)) = value;
	}

	void EnableModdedAchievements()
	{
		MemoryUtils::PatchMemory(m_PointersMap["_aob_achievements"].address, {0xB0, 0x00, 0xC3, 0x90, 0x90, 0x90});
	}

	void DisableModdedAchievements()
	{
		MemoryUtils::PatchMemory(m_PointersMap["_aob_achievements"].address, {0x40, 0x57, 0x48, 0x83, 0xEC, 0x30});
	}
};
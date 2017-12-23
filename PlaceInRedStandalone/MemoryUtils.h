/*
* The MIT License
* Copyright 2017 naPalm / PapaRadroach
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
#include "Pattern.h"

class MemoryUtils
{
public:
	static void PatchMemory(DWORD64 address, std::vector<UINT8> patchBytes)
	{
		DWORD* pAddress = (DWORD*)address;
		DWORD oldProtect = 0;
		if(VirtualProtect(pAddress, patchBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) // make memory writable
		{
			memcpy((void*)address, patchBytes.data(), patchBytes.size());
			VirtualProtect(pAddress, patchBytes.size(), oldProtect, NULL); // reprotect
		}
	}

	static bool BruteForceScanFastAllSignatures(std::map<std::string, PointerInfo> &pointersMap, MODULEINFO mi, int delay, int maxRetries)
	{
		int iTries = 0;
		bool bAllValid = false;
		while(!bAllValid && iTries < maxRetries)
		{
			for(auto it = pointersMap.begin(); it != pointersMap.end(); it++)
			{
				if(!it->second.address)
				{
					pointersMap[it->first].address = Pattern::ScanFast(mi, it->second.signature);
				}
			}

			bool bCheck = true;
			for(auto it = pointersMap.begin(); it != pointersMap.end(); it++)
			{
				if(!it->second.address)
				{
					bCheck = false;
					break;
				}
			}

			bAllValid = bCheck;

			if(bAllValid)
			{
				break;
			}
			else
			{
				iTries++;
				Sleep(delay);
			}
		}

		for(auto it = pointersMap.begin(); it != pointersMap.end(); it++)
		{
			if(!it->second.address)
			{
				char errorMessage[128];
				sprintf_s(errorMessage, sizeof(errorMessage) / sizeof(errorMessage[0]), "Error: %s is NULL. This probably means the mod is outdated.", it->first.c_str());
				MessageBoxA(NULL, errorMessage, TITLE, MB_ICONERROR);
				return false;
			}
		}

		return true;
	}
};
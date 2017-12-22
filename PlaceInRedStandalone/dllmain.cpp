// By naPalm (http://napalm.me)
// Proxy DLL built using https://github.com/zeroKilo/ProxyDllMaker

#define MAX_TRIES 100 // Maximum number of times to sigscan before giving up
#define SIGSCAN_DELAY 500 // Delay between each (failed) sigscan attempt in milliseconds
#define TITLE "Place in Red - Standalone"

#include "stdafx.h"
#include "Pattern.h"

#pragma comment(lib, "libs/MinHook/libMinHook.x64.lib")
#pragma pack(1)

void LogMessage(const char* format, ...)
{
	char messageBuf[1024] = {0};
	va_list argptr;
	va_start(argptr, format);
	vsnprintf(messageBuf, 1024, format, argptr);
	va_end(argptr);

	HANDLE hFile;
	DWORD dwBytesWritten = 0;
	BOOL bErrorFlag = FALSE;

	hFile = CreateFile(L"./placeinredstandalone.log",                // name of the write
		FILE_APPEND_DATA,          // open for writing
		0,                      // do not share
		NULL,                   // default security
		OPEN_ALWAYS,             // create new file only
		FILE_ATTRIBUTE_NORMAL,  // normal file
		NULL);                  // no attr. template

	if(hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	bErrorFlag = WriteFile(
		hFile,           // open file handle
		messageBuf,      // start of data to write
		strlen(messageBuf),  // number of bytes to write
		&dwBytesWritten, // number of bytes that were written
		NULL);            // no overlapped structure

	CloseHandle(hFile);
}

void patchMemory(DWORD64 address, std::vector<BYTE> patchBytes)
{
	// LogMessage("Address = 0x%X, bytes = %02X, size = %i\r\n", address, patchBytes[0], patchBytes.size());
	// LogMessage("Before patch = %02X \r\n", *((BYTE*)(address)));

	DWORD* pAddress = (DWORD*)address;
	DWORD oldProtect = 0;
	if(VirtualProtect(pAddress, patchBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) // make memory writable
	{
		memcpy((void*)address, patchBytes.data(), patchBytes.size());
		VirtualProtect(pAddress, 6, oldProtect, NULL); // reprotect
	}
}

/*
SetGlobalValue

48 ? ? ? 08  57 48 83 EC 40 41 ? ? 10 8B FA 48 8B D9 C1 ? ? A8 01

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

// Original function
SetGlobalValue pSetGlobalValueOrig = NULL;

typedef struct
{
	BYTE pad0[0x20];
	char* name; // 0x20
	BYTE pad1[0x08];
	float value; // 0x30
} GlobalData;


int __fastcall SetGlobalValueHook(DWORD64* unk1, unsigned int unk2, DWORD64* globalValuePtr, float newValue)
{
	GlobalData* pGlobalData = (GlobalData*)globalValuePtr;
	LogMessage("name = %s, const = %i, old value = %f, new value = %f\r\n", pGlobalData->name, (*(DWORD64*)(globalValuePtr + 16) >> 6) & 1 ? 1 : 0, pGlobalData->value, newValue);

	return pSetGlobalValueOrig(unk1, unk2, globalValuePtr, newValue);
}

DWORD __stdcall WorkerThread(HANDLE mainProcess)
{
	if(MH_Initialize() != MH_OK)
	{
		MessageBoxA(NULL, "MinHook initialization failed", TITLE, MB_ICONERROR);
		return NULL;
	}

	MODULEINFO mainModuleInfo = {0};
	if(!GetModuleInformation(mainProcess, GetModuleHandle(0), &mainModuleInfo, sizeof(mainModuleInfo)))
	{
		MessageBoxA(NULL, "GetModuleInformation failed", TITLE, MB_ICONERROR);
		return NULL;
	}

	DWORD64 dwBaseSigScan = NULL;
	int iTries = 0;
	while(!dwBaseSigScan && iTries < MAX_TRIES)
	{
		dwBaseSigScan = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 01 84 C0 75 ? B1");

		if(dwBaseSigScan)
		{
			break;
		}
		else 
		{
			iTries++;
			Sleep(SIGSCAN_DELAY);
		}
	}

	if(dwBaseSigScan)
	{
		std::map<std::string, DWORD64> pointersMap;

		pointersMap["PTR01_A"] = dwBaseSigScan;
		pointersMap["PTR02_A"] = Pattern::Scan(mainModuleInfo, "40 88 35 ? ? ? ? C6 05 ? ? ? ? 01 40 88 35 ? ? ? ? 48 85 FF");
		pointersMap["PTR02_B"] = Pattern::Scan(mainModuleInfo, "B2 01 88 15 ? ? ? ? EB 04 84 D2 74 07 C6 85");
		pointersMap["PTR02_C"] = Pattern::Scan(mainModuleInfo, "74 0C B0 01 B1 03 88 05 ? ? ? ? EB 0F 32 C0");
		pointersMap["PTR02_E"] = Pattern::Scan(mainModuleInfo, "0F 95 05 ? ? ? ? E8 ? ? ? ? 40 38 3D");
		pointersMap["PTR03_A"] = Pattern::Scan(mainModuleInfo, "41 B8 09 00 00 00 84 D2 41 0F 44 C8 88 0D");
		pointersMap["PTR03_B"] = Pattern::Scan(mainModuleInfo, "88 0D ? ? ? ? F3 0F 10 05");
		pointersMap["PTR03_C"] = Pattern::Scan(mainModuleInfo, "88 05 ? ? ? ? 48 85 ED 0F 84 ? ? ? ? 48 8B CD E8 ? ? ? ? 84 C0 0F 84 ? ? ? ? 40 38 35");
		pointersMap["PTR03_D"] = Pattern::Scan(mainModuleInfo, "B8 07 00 00 00 40 84 FF 0F 44 C8 88 0D");
		pointersMap["PTR03_E"] = Pattern::Scan(mainModuleInfo, "B9 04 00 00 00 0F B6 F8 0F B6 05 ? ? ? ? 40 84 FF 0F 44 C1 88");
		pointersMap["PTR03_F"] = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 00 48 C7 45 C7 ? ? ? ? E8 ? ? ? ? 48 8B 5D C7 83 CE FF 48 85 DB 0F 84");
		pointersMap["PTR03_H"] = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 03 48 8B 05 ? ? ? ? 80");
		pointersMap["PTR03_J"] = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 06 E9 ? ? ? ? 45 84 F6");
		pointersMap["PTR03_K"] = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 08 E9 ? ? ? ? 45 32 F6 48 8B 8D");
		pointersMap["PTR03_L"] = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 0A E9 ? ? ? ? 45");
		pointersMap["PTR03_M"] = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 0B E9 ? ? ? ? 45");
		pointersMap["PTR03_N"] = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 0C E9 ? ? ? ? C6");
		pointersMap["PTR03_O"] = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 0D");
		pointersMap["PTR04_A"] = Pattern::Scan(mainModuleInfo, "88 05 ? ? ? ? 88 44 24 70 E8 ? ? ? ? 48 8D 54 24 70");
		pointersMap["YELLOW"] = Pattern::Scan(mainModuleInfo, "8B 58 14 48 8D 4C 24 30 45 33 C0 8B D3 E8");
		pointersMap["RED"] = Pattern::Scan(mainModuleInfo, "C6 05 ? ? ? ? 01 F3 0F 11 05 ? ? ? ? 0F 28");
		pointersMap["WORKSHOPTIMER"] = Pattern::Scan(mainModuleInfo, "48 81 EC 38 01 00 00 48 85 C9 0F 84 E5 00 00 00");

		for(auto it = pointersMap.begin(); it != pointersMap.end(); it++)
		{
			if(!it->second)
			{
				char errorMessage[128];
				sprintf_s(errorMessage, sizeof(errorMessage) / sizeof(errorMessage[0]), "Error: %s is NULL. This probably means the mod is outdated.", it->first.c_str());
				MessageBoxA(NULL, errorMessage, TITLE, MB_ICONERROR);
				return NULL;
			}
		}
		
		Sleep(1000);

		DWORD64 dwSetGlobalValueAddress = Pattern::Scan(mainModuleInfo, "48 ? ? ? 08  57 48 83 EC 40 41 ? ? 10 8B FA 48 8B D9 C1 ? ? A8 01");
		LogMessage("dwSetGlobalValueAddress = %X\r\n", dwSetGlobalValueAddress);
		MH_STATUS status = MH_OK;
		if((status = MH_CreateHook((void*)dwSetGlobalValueAddress, &SetGlobalValueHook,
			reinterpret_cast<LPVOID*>(&pSetGlobalValueOrig))) != MH_OK)
		{
			char errorMessage[128];
			sprintf_s(errorMessage, sizeof(errorMessage) / sizeof(errorMessage[0]), "MinHook hook creation failed: SetGlobalValueHook (status = %i)", status);
			MessageBoxA(NULL, errorMessage, TITLE, MB_ICONERROR);
			return NULL;
		}

		if(MH_EnableHook((void*)dwSetGlobalValueAddress) != MH_OK)
		{
			MessageBoxA(NULL, "MinHook hook enabling failed: SetGlobalValueHook", TITLE, MB_ICONERROR);
			return NULL;
		}

		patchMemory(pointersMap["PTR01_A"] + 0x06, {0x00});
		patchMemory(pointersMap["PTR01_A"] + 0x0C, {0x01});
		patchMemory(pointersMap["PTR02_B"] + 0x01, {0x00});
		patchMemory(pointersMap["PTR02_C"], {0xEB});
		patchMemory(pointersMap["PTR03_A"] + 0x02, {0x01});
		patchMemory(pointersMap["PTR03_D"] + 0x01, {0x01});
		patchMemory(pointersMap["PTR03_E"] + 0x01, {0x01});
		patchMemory(pointersMap["PTR03_F"] + 0x06, {0x01});
		patchMemory(pointersMap["PTR03_H"] + 0x06, {0x01});
		patchMemory(pointersMap["PTR03_J"] + 0x06, {0x01});
		patchMemory(pointersMap["PTR03_K"] + 0x06, {0x01});
		patchMemory(pointersMap["PTR03_L"] + 0x06, {0x01});
		patchMemory(pointersMap["PTR03_M"] + 0x06, {0x01});
		patchMemory(pointersMap["PTR03_N"] + 0x06, {0x01});
		patchMemory(pointersMap["PTR03_O"] + 0x06, {0x01});
		patchMemory(pointersMap["RED"] + 0x06, {0x00});
		patchMemory(pointersMap["YELLOW"], {0x90, 0x90, 0x90});
		patchMemory(pointersMap["WORKSHOPTIMER"] + 0x0A, {0xE9, 0xE6, 0x00, 0x00, 0x00, 0x90});
	}
	else
	{
		MessageBoxA(NULL, "dwBaseSigScan failed. If the game has updated, this might be out of date.", TITLE, MB_ICONERROR);
		return NULL;
	}

	return NULL;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		CreateThread(NULL, 0, WorkerThread, GetCurrentProcess(), 0, NULL);
	}

	return TRUE;
}


// Proxy DLL stuff, automatically generated
#pragma comment(linker, "/export:SteamAPI_ISteamApps_BGetDLCDataByIndex=steam_api64_org.SteamAPI_ISteamApps_BGetDLCDataByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_DecompressVoice=steam_api64_org.SteamAPI_ISteamUser_DecompressVoice")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetAppOwner=steam_api64_org.SteamAPI_ISteamApps_GetAppOwner")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_SetVerticalScroll=steam_api64_org.SteamAPI_ISteamHTMLSurface_SetVerticalScroll")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_FindOrCreateLeaderboard=steam_api64_org.SteamAPI_ISteamUserStats_FindOrCreateLeaderboard")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_BShutdownIfAllPipesClosed=steam_api64_org.SteamAPI_ISteamClient_BShutdownIfAllPipesClosed")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetGameData=steam_api64_org.SteamAPI_ISteamGameServer_SetGameData")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetEnteredGamepadTextInput=steam_api64_org.SteamAPI_ISteamUtils_GetEnteredGamepadTextInput")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_QueueWillChange=steam_api64_org.SteamAPI_ISteamMusicRemote_QueueWillChange")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetHTTPRequestRequiresVerifiedCertificate=steam_api64_org.SteamAPI_ISteamHTTP_SetHTTPRequestRequiresVerifiedCertificate")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetLanguage=steam_api64_org.SteamAPI_ISteamUGC_SetLanguage")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetGameBadgeLevel=steam_api64_org.SteamAPI_ISteamUser_GetGameBadgeLevel")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetUGCDownloadProgress=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetUGCDownloadProgress")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetDlcDownloadProgress=steam_api64_org.SteamAPI_ISteamApps_GetDlcDownloadProgress")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetIPCCallCount=steam_api64_org.SteamAPI_ISteamUtils_GetIPCCallCount")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamApps=steam_api64_org.SteamAPI_ISteamClient_GetISteamApps")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_UpdateVolume=steam_api64_org.SteamAPI_ISteamMusicRemote_UpdateVolume")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_AddExcludedTag=steam_api64_org.SteamAPI_ISteamUGC_AddExcludedTag")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetAchievementName=steam_api64_org.SteamAPI_ISteamUserStats_GetAchievementName")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_MouseDoubleClick=steam_api64_org.SteamAPI_ISteamHTMLSurface_MouseDoubleClick")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_ServerRules=steam_api64_org.SteamAPI_ISteamMatchmakingServers_ServerRules")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetMapName=steam_api64_org.SteamAPI_ISteamGameServer_SetMapName")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetFileCount=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetFileCount")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetCurrentBetaName=steam_api64_org.SteamAPI_ISteamApps_GetCurrentBetaName")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_GetHTTPDownloadProgressPct=steam_api64_org.SteamAPI_ISteamHTTP_GetHTTPDownloadProgressPct")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendsGroupMembersCount=steam_api64_org.SteamAPI_ISteamFriends_GetFriendsGroupMembersCount")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_GetItemsByID=steam_api64_org.SteamAPI_ISteamInventory_GetItemsByID")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_AddRequestLobbyListNumericalFilter=steam_api64_org.SteamAPI_ISteamMatchmaking_AddRequestLobbyListNumericalFilter")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SendHTTPRequestAndStreamResponse=steam_api64_org.SteamAPI_ISteamHTTP_SendHTTPRequestAndStreamResponse")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetUserDataFolder=steam_api64_org.SteamAPI_ISteamUser_GetUserDataFolder")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetImageRGBA=steam_api64_org.SteamAPI_ISteamUtils_GetImageRGBA")

#pragma comment(linker, "/export:SteamAPI_ISteamScreenshots_TagPublishedFile=steam_api64_org.SteamAPI_ISteamScreenshots_TagPublishedFile")

#pragma comment(linker, "/export:SteamAPI_ISteamMusic_PlayNext=steam_api64_org.SteamAPI_ISteamMusic_PlayNext")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_ReleaseRequest=steam_api64_org.SteamAPI_ISteamMatchmakingServers_ReleaseRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_EnablePlayPrevious=steam_api64_org.SteamAPI_ISteamMusicRemote_EnablePlayPrevious")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_SetSize=steam_api64_org.SteamAPI_ISteamHTMLSurface_SetSize")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetAchievement=steam_api64_org.SteamAPI_ISteamUserStats_GetAchievement")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_LogOnAnonymous=steam_api64_org.SteamAPI_ISteamGameServer_LogOnAnonymous")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamGameServer=steam_api64_org.SteamAPI_ISteamClient_GetISteamGameServer")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_BIsSubscribedApp=steam_api64_org.SteamAPI_ISteamApps_BIsSubscribedApp")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileWriteStreamOpen=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileWriteStreamOpen")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_DestroyListenSocket=steam_api64_org.SteamAPI_ISteamNetworking_DestroyListenSocket")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetVoiceOptimalSampleRate=steam_api64_org.SteamAPI_ISteamUser_GetVoiceOptimalSampleRate")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendsGroupCount=steam_api64_org.SteamAPI_ISteamFriends_GetFriendsGroupCount")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_GetHTTPResponseBodyData=steam_api64_org.SteamAPI_ISteamHTTP_GetHTTPResponseBodyData")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_UpdatePlaybackStatus=steam_api64_org.SteamAPI_ISteamMusicRemote_UpdatePlaybackStatus")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamGenericInterface=steam_api64_org.SteamAPI_ISteamClient_GetISteamGenericInterface")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetAPICallFailureReason=steam_api64_org.SteamAPI_ISteamUtils_GetAPICallFailureReason")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_AddPromoItems=steam_api64_org.SteamAPI_ISteamInventory_AddPromoItems")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_InstallDLC=steam_api64_org.SteamAPI_ISteamApps_InstallDLC")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetFileSize=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetFileSize")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetAchievementDisplayAttribute=steam_api64_org.SteamAPI_ISteamUserStats_GetAchievementDisplayAttribute")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_RefreshServer=steam_api64_org.SteamAPI_ISteamMatchmakingServers_RefreshServer")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_ExecuteJavascript=steam_api64_org.SteamAPI_ISteamHTMLSurface_ExecuteJavascript")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetMaxPlayerCount=steam_api64_org.SteamAPI_ISteamGameServer_SetMaxPlayerCount")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamInventory=steam_api64_org.SteamAPI_ISteamClient_GetISteamInventory")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_RequestGlobalAchievementPercentages=steam_api64_org.SteamAPI_ISteamUserStats_RequestGlobalAchievementPercentages")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UpdatePublishedFileTags=steam_api64_org.SteamAPI_ISteamRemoteStorage_UpdatePublishedFileTags")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetItemUpdateLanguage=steam_api64_org.SteamAPI_ISteamUGC_SetItemUpdateLanguage")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_JSDialogResponse=steam_api64_org.SteamAPI_ISteamHTMLSurface_JSDialogResponse")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_GetGameplayStats=steam_api64_org.SteamAPI_ISteamGameServer_GetGameplayStats")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_WasRestartRequested=steam_api64_org.SteamAPI_ISteamGameServer_WasRestartRequested")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_GetHTTPResponseBodySize=steam_api64_org.SteamAPI_ISteamHTTP_GetHTTPResponseBodySize")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_EnablePlaylists=steam_api64_org.SteamAPI_ISteamMusicRemote_EnablePlaylists")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FilePersisted=steam_api64_org.SteamAPI_ISteamRemoteStorage_FilePersisted")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_AddHeader=steam_api64_org.SteamAPI_ISteamHTMLSurface_AddHeader")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_AddPromoItem=steam_api64_org.SteamAPI_ISteamInventory_AddPromoItem")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_GetServerCount=steam_api64_org.SteamAPI_ISteamMatchmakingServers_GetServerCount")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_IsAPICallCompleted=steam_api64_org.SteamAPI_ISteamUtils_IsAPICallCompleted")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetAchievementIcon=steam_api64_org.SteamAPI_ISteamUserStats_GetAchievementIcon")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamMatchmakingServers=steam_api64_org.SteamAPI_ISteamClient_GetISteamMatchmakingServers")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetQueryUGCNumKeyValueTags=steam_api64_org.SteamAPI_ISteamUGC_GetQueryUGCNumKeyValueTags")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_DestroySocket=steam_api64_org.SteamAPI_ISteamNetworking_DestroySocket")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_TriggerItemDrop=steam_api64_org.SteamAPI_ISteamInventory_TriggerItemDrop")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_CurrentEntryIsAvailable=steam_api64_org.SteamAPI_ISteamMusicRemote_CurrentEntryIsAvailable")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_MouseWheel=steam_api64_org.SteamAPI_ISteamHTMLSurface_MouseWheel")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetReturnLongDescription=steam_api64_org.SteamAPI_ISteamUGC_SetReturnLongDescription")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetSpectatorPort=steam_api64_org.SteamAPI_ISteamGameServer_SetSpectatorPort")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_IsOverlayEnabled=steam_api64_org.SteamAPI_ISteamUtils_IsOverlayEnabled")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamRemoteStorage=steam_api64_org.SteamAPI_ISteamClient_GetISteamRemoteStorage")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetInstalledDepots=steam_api64_org.SteamAPI_ISteamApps_GetInstalledDepots")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_CreateCookieContainer=steam_api64_org.SteamAPI_ISteamHTTP_CreateCookieContainer")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetQuota=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetQuota")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_Remove_SteamAPI_CPostAPIResultInProcess=steam_api64_org.SteamAPI_ISteamClient_Remove_SteamAPI_CPostAPIResultInProcess")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_AttachLeaderboardUGC=steam_api64_org.SteamAPI_ISteamUserStats_AttachLeaderboardUGC")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_SetBackgroundMode=steam_api64_org.SteamAPI_ISteamHTMLSurface_SetBackgroundMode")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetItemTitle=steam_api64_org.SteamAPI_ISteamUGC_SetItemTitle")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UpdatePublishedFileDescription=steam_api64_org.SteamAPI_ISteamRemoteStorage_UpdatePublishedFileDescription")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileWriteStreamWriteChunk=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileWriteStreamWriteChunk")

#pragma comment(linker, "/export:SteamAPI_ISteamMusic_SetVolume=steam_api64_org.SteamAPI_ISteamMusic_SetVolume")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_BIsDlcInstalled=steam_api64_org.SteamAPI_ISteamApps_BIsDlcInstalled")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_DeferHTTPRequest=steam_api64_org.SteamAPI_ISteamHTTP_DeferHTTPRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_LogOff=steam_api64_org.SteamAPI_ISteamGameServer_LogOff")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_SerializeResult=steam_api64_org.SteamAPI_ISteamInventory_SerializeResult")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_StartVoiceRecording=steam_api64_org.SteamAPI_ISteamUser_StartVoiceRecording")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_EnablePlayNext=steam_api64_org.SteamAPI_ISteamMusicRemote_EnablePlayNext")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_GetServerDetails=steam_api64_org.SteamAPI_ISteamMatchmakingServers_GetServerDetails")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetCSERIPPort=steam_api64_org.SteamAPI_ISteamUtils_GetCSERIPPort")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_AllowP2PPacketRelay=steam_api64_org.SteamAPI_ISteamNetworking_AllowP2PPacketRelay")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_SetAchievement=steam_api64_org.SteamAPI_ISteamUserStats_SetAchievement")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_AddRequestLobbyListNearValueFilter=steam_api64_org.SteamAPI_ISteamMatchmaking_AddRequestLobbyListNearValueFilter")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_SetLocalIPBinding=steam_api64_org.SteamAPI_ISteamClient_SetLocalIPBinding")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_StopLoad=steam_api64_org.SteamAPI_ISteamHTMLSurface_StopLoad")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_ActivateGameOverlayToStore=steam_api64_org.SteamAPI_ISteamFriends_ActivateGameOverlayToStore")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_Set_SteamAPI_CPostAPIResultInProcess=steam_api64_org.SteamAPI_ISteamClient_Set_SteamAPI_CPostAPIResultInProcess")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_StartItemUpdate=steam_api64_org.SteamAPI_ISteamUGC_StartItemUpdate")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_PlaylistDidChange=steam_api64_org.SteamAPI_ISteamMusicRemote_PlaylistDidChange")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UpdatePublishedFileTitle=steam_api64_org.SteamAPI_ISteamRemoteStorage_UpdatePublishedFileTitle")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_CancelAuthTicket=steam_api64_org.SteamAPI_ISteamGameServer_CancelAuthTicket")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_ActivateGameOverlayInviteDialog=steam_api64_org.SteamAPI_ISteamFriends_ActivateGameOverlayInviteDialog")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_RequestUserGroupStatus=steam_api64_org.SteamAPI_ISteamGameServer_RequestUserGroupStatus")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_SetLobbyType=steam_api64_org.SteamAPI_ISteamMatchmaking_SetLobbyType")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_CreateItem=steam_api64_org.SteamAPI_ISteamUGC_CreateItem")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_SetCurrentPlaylistEntry=steam_api64_org.SteamAPI_ISteamMusicRemote_SetCurrentPlaylistEntry")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UpdatePublishedFilePreviewFile=steam_api64_org.SteamAPI_ISteamRemoteStorage_UpdatePublishedFilePreviewFile")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamHTMLSurface=steam_api64_org.SteamAPI_ISteamClient_GetISteamHTMLSurface")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_ActivateGameOverlayToWebPage=steam_api64_org.SteamAPI_ISteamFriends_ActivateGameOverlayToWebPage")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_CloseClanChatWindowInSteam=steam_api64_org.SteamAPI_ISteamFriends_CloseClanChatWindowInSteam")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_EnumerateFollowingList=steam_api64_org.SteamAPI_ISteamFriends_EnumerateFollowingList")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetChatMemberByIndex=steam_api64_org.SteamAPI_ISteamFriends_GetChatMemberByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanActivityCounts=steam_api64_org.SteamAPI_ISteamFriends_GetClanActivityCounts")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanByIndex=steam_api64_org.SteamAPI_ISteamFriends_GetClanByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanChatMemberCount=steam_api64_org.SteamAPI_ISteamFriends_GetClanChatMemberCount")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanChatMessage=steam_api64_org.SteamAPI_ISteamFriends_GetClanChatMessage")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanOfficerByIndex=steam_api64_org.SteamAPI_ISteamFriends_GetClanOfficerByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanOfficerCount=steam_api64_org.SteamAPI_ISteamFriends_GetClanOfficerCount")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanOwner=steam_api64_org.SteamAPI_ISteamFriends_GetClanOwner")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetCoplayFriend=steam_api64_org.SteamAPI_ISteamFriends_GetCoplayFriend")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFollowerCount=steam_api64_org.SteamAPI_ISteamFriends_GetFollowerCount")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendByIndex=steam_api64_org.SteamAPI_ISteamFriends_GetFriendByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendCoplayGame=steam_api64_org.SteamAPI_ISteamFriends_GetFriendCoplayGame")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendCoplayTime=steam_api64_org.SteamAPI_ISteamFriends_GetFriendCoplayTime")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyMemberData=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyMemberData")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendCountFromSource=steam_api64_org.SteamAPI_ISteamFriends_GetFriendCountFromSource")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendFromSourceByIndex=steam_api64_org.SteamAPI_ISteamFriends_GetFriendFromSourceByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendMessage=steam_api64_org.SteamAPI_ISteamFriends_GetFriendMessage")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendPersonaName=steam_api64_org.SteamAPI_ISteamFriends_GetFriendPersonaName")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_SetUserAchievement=steam_api64_org.SteamAPI_ISteamGameServerStats_SetUserAchievement")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_StoreUserStats=steam_api64_org.SteamAPI_ISteamGameServerStats_StoreUserStats")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendPersonaNameHistory=steam_api64_org.SteamAPI_ISteamFriends_GetFriendPersonaNameHistory")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_GetP2PSessionState=steam_api64_org.SteamAPI_ISteamNetworking_GetP2PSessionState")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendPersonaState=steam_api64_org.SteamAPI_ISteamFriends_GetFriendPersonaState")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendRichPresence=steam_api64_org.SteamAPI_ISteamFriends_GetFriendRichPresence")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendRichPresenceKeyByIndex=steam_api64_org.SteamAPI_ISteamFriends_GetFriendRichPresenceKeyByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendRichPresenceKeyCount=steam_api64_org.SteamAPI_ISteamFriends_GetFriendRichPresenceKeyCount")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendSteamLevel=steam_api64_org.SteamAPI_ISteamFriends_GetFriendSteamLevel")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetMediumFriendAvatar=steam_api64_org.SteamAPI_ISteamFriends_GetMediumFriendAvatar")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_SetLobbyJoinable=steam_api64_org.SteamAPI_ISteamMatchmaking_SetLobbyJoinable")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetSmallFriendAvatar=steam_api64_org.SteamAPI_ISteamFriends_GetSmallFriendAvatar")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_InviteUserToGame=steam_api64_org.SteamAPI_ISteamFriends_InviteUserToGame")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_IsClanChatAdmin=steam_api64_org.SteamAPI_ISteamFriends_IsClanChatAdmin")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_IsClanChatWindowOpenInSteam=steam_api64_org.SteamAPI_ISteamFriends_IsClanChatWindowOpenInSteam")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_IsFollowing=steam_api64_org.SteamAPI_ISteamFriends_IsFollowing")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_JoinClanChatRoom=steam_api64_org.SteamAPI_ISteamFriends_JoinClanChatRoom")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_LeaveClanChatRoom=steam_api64_org.SteamAPI_ISteamFriends_LeaveClanChatRoom")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_OpenClanChatWindowInSteam=steam_api64_org.SteamAPI_ISteamFriends_OpenClanChatWindowInSteam")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_ReplyToFriendMessage=steam_api64_org.SteamAPI_ISteamFriends_ReplyToFriendMessage")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_RequestClanOfficerList=steam_api64_org.SteamAPI_ISteamFriends_RequestClanOfficerList")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_RequestFriendRichPresence=steam_api64_org.SteamAPI_ISteamFriends_RequestFriendRichPresence")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_SendClanChatMessage=steam_api64_org.SteamAPI_ISteamFriends_SendClanChatMessage")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_SetInGameVoiceSpeaking=steam_api64_org.SteamAPI_ISteamFriends_SetInGameVoiceSpeaking")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_BUpdateUserData=steam_api64_org.SteamAPI_ISteamGameServer_BUpdateUserData")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_SetListenForFriendsMessages=steam_api64_org.SteamAPI_ISteamFriends_SetListenForFriendsMessages")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendGamePlayed=steam_api64_org.SteamAPI_ISteamFriends_GetFriendGamePlayed")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_ClearUserAchievement=steam_api64_org.SteamAPI_ISteamGameServerStats_ClearUserAchievement")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_AcceptP2PSessionWithUser=steam_api64_org.SteamAPI_ISteamNetworking_AcceptP2PSessionWithUser")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_GetUserAchievement=steam_api64_org.SteamAPI_ISteamGameServerStats_GetUserAchievement")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_CheckResultSteamID=steam_api64_org.SteamAPI_ISteamInventory_CheckResultSteamID")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_GetUserStat=steam_api64_org.SteamAPI_ISteamGameServerStats_GetUserStat")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_GetUserStat0=steam_api64_org.SteamAPI_ISteamGameServerStats_GetUserStat0")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_RequestUserStats=steam_api64_org.SteamAPI_ISteamGameServerStats_RequestUserStats")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_CloseP2PSessionWithUser=steam_api64_org.SteamAPI_ISteamNetworking_CloseP2PSessionWithUser")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_SetUserStat0=steam_api64_org.SteamAPI_ISteamGameServerStats_SetUserStat0")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_UpdateUserAvgRateStat=steam_api64_org.SteamAPI_ISteamGameServerStats_UpdateUserAvgRateStat")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_AssociateWithClan=steam_api64_org.SteamAPI_ISteamGameServer_AssociateWithClan")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_BeginAuthSession=steam_api64_org.SteamAPI_ISteamGameServer_BeginAuthSession")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_ActivateGameOverlayToUser=steam_api64_org.SteamAPI_ISteamFriends_ActivateGameOverlayToUser")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_ComputeNewPlayerCompatibility=steam_api64_org.SteamAPI_ISteamGameServer_ComputeNewPlayerCompatibility")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_CreateUnauthenticatedUserConnection=steam_api64_org.SteamAPI_ISteamGameServer_CreateUnauthenticatedUserConnection")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_EndAuthSession=steam_api64_org.SteamAPI_ISteamGameServer_EndAuthSession")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_GetNextOutgoingPacket=steam_api64_org.SteamAPI_ISteamGameServer_GetNextOutgoingPacket")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_GetSteamID=steam_api64_org.SteamAPI_ISteamGameServer_GetSteamID")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_HandleIncomingPacket=steam_api64_org.SteamAPI_ISteamGameServer_HandleIncomingPacket")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_DestructISteamHTMLSurface=steam_api64_org.SteamAPI_ISteamHTMLSurface_DestructISteamHTMLSurface")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetItemMetadata=steam_api64_org.SteamAPI_ISteamUGC_SetItemMetadata")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_GetServerReputation=steam_api64_org.SteamAPI_ISteamGameServer_GetServerReputation")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_FileLoadDialogResponse=steam_api64_org.SteamAPI_ISteamHTMLSurface_FileLoadDialogResponse")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_CommitPublishedFileUpdate=steam_api64_org.SteamAPI_ISteamRemoteStorage_CommitPublishedFileUpdate")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamVideo=steam_api64_org.SteamAPI_ISteamClient_GetISteamVideo")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_Find=steam_api64_org.SteamAPI_ISteamHTMLSurface_Find")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_SetCookie=steam_api64_org.SteamAPI_ISteamHTMLSurface_SetCookie")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetPlayerSteamLevel=steam_api64_org.SteamAPI_ISteamUser_GetPlayerSteamLevel")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetAllowCachedResponse=steam_api64_org.SteamAPI_ISteamUGC_SetAllowCachedResponse")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_DownloadClanActivityCounts=steam_api64_org.SteamAPI_ISteamFriends_DownloadClanActivityCounts")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetRegion=steam_api64_org.SteamAPI_ISteamGameServer_SetRegion")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_SetKeyFocus=steam_api64_org.SteamAPI_ISteamHTMLSurface_SetKeyFocus")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetAppBuildId=steam_api64_org.SteamAPI_ISteamApps_GetAppBuildId")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetSteamUILanguage=steam_api64_org.SteamAPI_ISteamUtils_GetSteamUILanguage")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetHTTPRequestAbsoluteTimeoutMS=steam_api64_org.SteamAPI_ISteamHTTP_SetHTTPRequestAbsoluteTimeoutMS")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_ResetQueueEntries=steam_api64_org.SteamAPI_ISteamMusicRemote_ResetQueueEntries")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamHTTP=steam_api64_org.SteamAPI_ISteamClient_GetISteamHTTP")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_FindLeaderboard=steam_api64_org.SteamAPI_ISteamUserStats_FindLeaderboard")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_SetPageScaleFactor=steam_api64_org.SteamAPI_ISteamHTMLSurface_SetPageScaleFactor")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_GetAuthSessionTicket=steam_api64_org.SteamAPI_ISteamGameServer_GetAuthSessionTicket")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_ActivateGameOverlay=steam_api64_org.SteamAPI_ISteamFriends_ActivateGameOverlay")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_ResetPlaylistEntries=steam_api64_org.SteamAPI_ISteamMusicRemote_ResetPlaylistEntries")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_CreatePublishedFileUpdateRequest=steam_api64_org.SteamAPI_ISteamRemoteStorage_CreatePublishedFileUpdateRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_StopFind=steam_api64_org.SteamAPI_ISteamHTMLSurface_StopFind")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamMusic=steam_api64_org.SteamAPI_ISteamClient_GetISteamMusic")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_AddRequiredKeyValueTag=steam_api64_org.SteamAPI_ISteamUGC_AddRequiredKeyValueTag")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetAvailableVoice=steam_api64_org.SteamAPI_ISteamUser_GetAvailableVoice")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamUtils=steam_api64_org.SteamAPI_ISteamClient_GetISteamUtils")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetAchievementAndUnlockTime=steam_api64_org.SteamAPI_ISteamUserStats_GetAchievementAndUnlockTime")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_GetHTTPResponseHeaderSize=steam_api64_org.SteamAPI_ISteamHTTP_GetHTTPResponseHeaderSize")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetAppID=steam_api64_org.SteamAPI_ISteamUtils_GetAppID")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_AddRequestLobbyListDistanceFilter=steam_api64_org.SteamAPI_ISteamMatchmaking_AddRequestLobbyListDistanceFilter")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileWriteStreamCancel=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileWriteStreamCancel")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_RefreshQuery=steam_api64_org.SteamAPI_ISteamMatchmakingServers_RefreshQuery")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_BIsSubscribedFromFreeWeekend=steam_api64_org.SteamAPI_ISteamApps_BIsSubscribedFromFreeWeekend")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_BSecure=steam_api64_org.SteamAPI_ISteamGameServer_BSecure")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_EnableLooped=steam_api64_org.SteamAPI_ISteamMusicRemote_EnableLooped")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetQueryUGCNumAdditionalPreviews=steam_api64_org.SteamAPI_ISteamUGC_GetQueryUGCNumAdditionalPreviews")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_GoBack=steam_api64_org.SteamAPI_ISteamHTMLSurface_GoBack")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetAPICallResult=steam_api64_org.SteamAPI_ISteamUtils_GetAPICallResult")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_GetHTTPStreamingResponseBodyData=steam_api64_org.SteamAPI_ISteamHTTP_GetHTTPStreamingResponseBodyData")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_GetAllItems=steam_api64_org.SteamAPI_ISteamInventory_GetAllItems")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetImageSize=steam_api64_org.SteamAPI_ISteamUtils_GetImageSize")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetAvailableGameLanguages=steam_api64_org.SteamAPI_ISteamApps_GetAvailableGameLanguages")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_SetSyncPlatforms=steam_api64_org.SteamAPI_ISteamRemoteStorage_SetSyncPlatforms")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_LoadURL=steam_api64_org.SteamAPI_ISteamHTMLSurface_LoadURL")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_AddRequestLobbyListStringFilter=steam_api64_org.SteamAPI_ISteamMatchmaking_AddRequestLobbyListStringFilter")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamUser=steam_api64_org.SteamAPI_ISteamClient_GetISteamUser")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SendHTTPRequest=steam_api64_org.SteamAPI_ISteamHTTP_SendHTTPRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_UpdateAvgRateStat=steam_api64_org.SteamAPI_ISteamUserStats_UpdateAvgRateStat")

#pragma comment(linker, "/export:SteamAPI_ISteamMusic_PlayPrevious=steam_api64_org.SteamAPI_ISteamMusic_PlayPrevious")

#pragma comment(linker, "/export:SteamAPI_ISteamController_SetOverrideMode=steam_api64_org.SteamAPI_ISteamController_SetOverrideMode")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_SetPNGIcon_64x64=steam_api64_org.SteamAPI_ISteamMusicRemote_SetPNGIcon_64x64")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_LogOn=steam_api64_org.SteamAPI_ISteamGameServer_LogOn")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetCookie=steam_api64_org.SteamAPI_ISteamHTTP_SetCookie")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetHTTPRequestRawPostBody=steam_api64_org.SteamAPI_ISteamHTTP_SetHTTPRequestRawPostBody")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_DeserializeResult=steam_api64_org.SteamAPI_ISteamInventory_DeserializeResult")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_CreateListenSocket=steam_api64_org.SteamAPI_ISteamNetworking_CreateListenSocket")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_ExchangeItems=steam_api64_org.SteamAPI_ISteamInventory_ExchangeItems")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_GenerateItems=steam_api64_org.SteamAPI_ISteamInventory_GenerateItems")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_UpdateCurrentEntryCoverArt=steam_api64_org.SteamAPI_ISteamMusicRemote_UpdateCurrentEntryCoverArt")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_KeyChar=steam_api64_org.SteamAPI_ISteamHTMLSurface_KeyChar")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_GetItemDefinitionIDs=steam_api64_org.SteamAPI_ISteamInventory_GetItemDefinitionIDs")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetIPCCallCount=steam_api64_org.SteamAPI_ISteamClient_GetIPCCallCount")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetReturnAdditionalPreviews=steam_api64_org.SteamAPI_ISteamUGC_SetReturnAdditionalPreviews")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_GetSocketConnectionType=steam_api64_org.SteamAPI_ISteamNetworking_GetSocketConnectionType")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetKeyValue=steam_api64_org.SteamAPI_ISteamGameServer_SetKeyValue")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_SetCloudEnabledForApp=steam_api64_org.SteamAPI_ISteamRemoteStorage_SetCloudEnabledForApp")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetHTTPRequestCookieContainer=steam_api64_org.SteamAPI_ISteamHTTP_SetHTTPRequestCookieContainer")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_RequestEncryptedAppTicket=steam_api64_org.SteamAPI_ISteamUser_RequestEncryptedAppTicket")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_GetItemDefinitionProperty=steam_api64_org.SteamAPI_ISteamInventory_GetItemDefinitionProperty")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_TradeItems=steam_api64_org.SteamAPI_ISteamInventory_TradeItems")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_Shutdown=steam_api64_org.SteamAPI_ISteamHTMLSurface_Shutdown")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetHTTPRequestNetworkActivityTimeout=steam_api64_org.SteamAPI_ISteamHTTP_SetHTTPRequestNetworkActivityTimeout")

#pragma comment(linker, "/export:SteamAPI_ISteamScreenshots_TriggerScreenshot=steam_api64_org.SteamAPI_ISteamScreenshots_TriggerScreenshot")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetStat=steam_api64_org.SteamAPI_ISteamUserStats_GetStat")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_ConnectToGlobalUser=steam_api64_org.SteamAPI_ISteamClient_ConnectToGlobalUser")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingPlayersResponse_PlayersRefreshComplete=steam_api64_org.SteamAPI_ISteamMatchmakingPlayersResponse_PlayersRefreshComplete")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetConnectedUniverse=steam_api64_org.SteamAPI_ISteamUtils_GetConnectedUniverse")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_GetResultTimestamp=steam_api64_org.SteamAPI_ISteamInventory_GetResultTimestamp")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_BIsCybercafe=steam_api64_org.SteamAPI_ISteamApps_BIsCybercafe")

#pragma comment(linker, "/export:SteamAPI_ISteamController_RunFrame=steam_api64_org.SteamAPI_ISteamController_RunFrame")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetGameDescription=steam_api64_org.SteamAPI_ISteamGameServer_SetGameDescription")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServerListResponse_RefreshComplete=steam_api64_org.SteamAPI_ISteamMatchmakingServerListResponse_RefreshComplete")

#pragma comment(linker, "/export:SteamAPI_ISteamMusic_GetPlaybackStatus=steam_api64_org.SteamAPI_ISteamMusic_GetPlaybackStatus")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetPersonaState=steam_api64_org.SteamAPI_ISteamFriends_GetPersonaState")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileForget=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileForget")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingRulesResponse_RulesRefreshComplete=steam_api64_org.SteamAPI_ISteamMatchmakingRulesResponse_RulesRefreshComplete")

#pragma comment(linker, "/export:SteamAPI_ISteamAppList_GetAppName=steam_api64_org.SteamAPI_ISteamAppList_GetAppName")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_CreateQueryUGCDetailsRequest=steam_api64_org.SteamAPI_ISteamUGC_CreateQueryUGCDetailsRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_BIsCurrentMusicRemote=steam_api64_org.SteamAPI_ISteamMusicRemote_BIsCurrentMusicRemote")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_AddRequestLobbyListFilterSlotsAvailable=steam_api64_org.SteamAPI_ISteamMatchmaking_AddRequestLobbyListFilterSlotsAvailable")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_BLoggedOn=steam_api64_org.SteamAPI_ISteamGameServer_BLoggedOn")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_CancelQuery=steam_api64_org.SteamAPI_ISteamMatchmakingServers_CancelQuery")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_PrioritizeHTTPRequest=steam_api64_org.SteamAPI_ISteamHTTP_PrioritizeHTTPRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamFriends=steam_api64_org.SteamAPI_ISteamClient_GetISteamFriends")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetCurrentBatteryPower=steam_api64_org.SteamAPI_ISteamUtils_GetCurrentBatteryPower")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_Reload=steam_api64_org.SteamAPI_ISteamHTMLSurface_Reload")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileWriteStreamClose=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileWriteStreamClose")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_StopVoiceRecording=steam_api64_org.SteamAPI_ISteamUser_StopVoiceRecording")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetEarliestPurchaseUnixTime=steam_api64_org.SteamAPI_ISteamApps_GetEarliestPurchaseUnixTime")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_EnableShuffled=steam_api64_org.SteamAPI_ISteamMusicRemote_EnableShuffled")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_ClearAchievement=steam_api64_org.SteamAPI_ISteamUserStats_ClearAchievement")

#pragma comment(linker, "/export:SteamAPI_ISteamMusic_GetVolume=steam_api64_org.SteamAPI_ISteamMusic_GetVolume")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_RequestAppProofOfPurchaseKey=steam_api64_org.SteamAPI_ISteamApps_RequestAppProofOfPurchaseKey")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetServerName=steam_api64_org.SteamAPI_ISteamGameServer_SetServerName")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_MouseDown=steam_api64_org.SteamAPI_ISteamHTMLSurface_MouseDown")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_AddRequiredTag=steam_api64_org.SteamAPI_ISteamUGC_AddRequiredTag")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_RunFrame=steam_api64_org.SteamAPI_ISteamUtils_RunFrame")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamGameServerStats=steam_api64_org.SteamAPI_ISteamClient_GetISteamGameServerStats")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_ReleaseHTTPRequest=steam_api64_org.SteamAPI_ISteamHTTP_ReleaseHTTPRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetNumAchievements=steam_api64_org.SteamAPI_ISteamUserStats_GetNumAchievements")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendsGroupName=steam_api64_org.SteamAPI_ISteamFriends_GetFriendsGroupName")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_IsDataAvailableOnSocket=steam_api64_org.SteamAPI_ISteamNetworking_IsDataAvailableOnSocket")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetSyncPlatforms=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetSyncPlatforms")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_PlayerDetails=steam_api64_org.SteamAPI_ISteamMatchmakingServers_PlayerDetails")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_UpdateLooped=steam_api64_org.SteamAPI_ISteamMusicRemote_UpdateLooped")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_RequestFavoritesServerList=steam_api64_org.SteamAPI_ISteamMatchmakingServers_RequestFavoritesServerList")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_ReadP2PPacket=steam_api64_org.SteamAPI_ISteamNetworking_ReadP2PPacket")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_RequestFriendsServerList=steam_api64_org.SteamAPI_ISteamMatchmakingServers_RequestFriendsServerList")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_RequestHistoryServerList=steam_api64_org.SteamAPI_ISteamMatchmakingServers_RequestHistoryServerList")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_AddFavoriteGame=steam_api64_org.SteamAPI_ISteamMatchmaking_AddFavoriteGame")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_AddRequestLobbyListCompatibleMembersFilter=steam_api64_org.SteamAPI_ISteamMatchmaking_AddRequestLobbyListCompatibleMembersFilter")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetPlayerNickname=steam_api64_org.SteamAPI_ISteamFriends_GetPlayerNickname")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_DeleteLobbyData=steam_api64_org.SteamAPI_ISteamMatchmaking_DeleteLobbyData")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_CreateHTTPRequest=steam_api64_org.SteamAPI_ISteamHTTP_CreateHTTPRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamVideo_GetVideoURL=steam_api64_org.SteamAPI_ISteamVideo_GetVideoURL")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetSecondsSinceAppActive=steam_api64_org.SteamAPI_ISteamUtils_GetSecondsSinceAppActive")

#pragma comment(linker, "/export:SteamAPI_ISteamAppList_GetNumInstalledApps=steam_api64_org.SteamAPI_ISteamAppList_GetNumInstalledApps")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_BIsSubscribed=steam_api64_org.SteamAPI_ISteamApps_BIsSubscribed")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileWrite=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileWrite")

#pragma comment(linker, "/export:SteamAPI_ISteamController_Init=steam_api64_org.SteamAPI_ISteamController_Init")

#pragma comment(linker, "/export:SteamAPI_ISteamMusic_BIsEnabled=steam_api64_org.SteamAPI_ISteamMusic_BIsEnabled")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_RequestCurrentStats=steam_api64_org.SteamAPI_ISteamUserStats_RequestCurrentStats")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetHSteamUser=steam_api64_org.SteamAPI_ISteamUser_GetHSteamUser")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServerListResponse_ServerResponded=steam_api64_org.SteamAPI_ISteamMatchmakingServerListResponse_ServerResponded")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetPersonaName=steam_api64_org.SteamAPI_ISteamFriends_GetPersonaName")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingPingResponse_ServerResponded=steam_api64_org.SteamAPI_ISteamMatchmakingPingResponse_ServerResponded")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetFavoriteGameCount=steam_api64_org.SteamAPI_ISteamMatchmaking_GetFavoriteGameCount")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingPlayersResponse_AddPlayerToList=steam_api64_org.SteamAPI_ISteamMatchmakingPlayersResponse_AddPlayerToList")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingRulesResponse_RulesResponded=steam_api64_org.SteamAPI_ISteamMatchmakingRulesResponse_RulesResponded")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_RegisterSteamMusicRemote=steam_api64_org.SteamAPI_ISteamMusicRemote_RegisterSteamMusicRemote")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_CreateSteamPipe=steam_api64_org.SteamAPI_ISteamClient_CreateSteamPipe")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_GetResultStatus=steam_api64_org.SteamAPI_ISteamInventory_GetResultStatus")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyByIndex=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyChatEntry=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyChatEntry")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyDataByIndex=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyDataByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanTag=steam_api64_org.SteamAPI_ISteamFriends_GetClanTag")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyDataCount=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyDataCount")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyGameServer=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyGameServer")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyMemberByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_UserHasLicenseForApp=steam_api64_org.SteamAPI_ISteamGameServer_UserHasLicenseForApp")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyMemberLimit=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyMemberLimit")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_SetPlayedWith=steam_api64_org.SteamAPI_ISteamFriends_SetPlayedWith")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyOwner=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyOwner")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_RemoveFavoriteGame=steam_api64_org.SteamAPI_ISteamMatchmaking_RemoveFavoriteGame")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_RequestLobbyData=steam_api64_org.SteamAPI_ISteamMatchmaking_RequestLobbyData")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_SendLobbyChatMsg=steam_api64_org.SteamAPI_ISteamMatchmaking_SendLobbyChatMsg")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SendUserDisconnect=steam_api64_org.SteamAPI_ISteamGameServer_SendUserDisconnect")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_IsUserInSource=steam_api64_org.SteamAPI_ISteamFriends_IsUserInSource")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_RequestUserInformation=steam_api64_org.SteamAPI_ISteamFriends_RequestUserInformation")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_SetLinkedLobby=steam_api64_org.SteamAPI_ISteamMatchmaking_SetLinkedLobby")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_SetLobbyData=steam_api64_org.SteamAPI_ISteamMatchmaking_SetLobbyData")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanName=steam_api64_org.SteamAPI_ISteamFriends_GetClanName")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_SetLobbyGameServer=steam_api64_org.SteamAPI_ISteamMatchmaking_SetLobbyGameServer")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_SetLobbyMemberData=steam_api64_org.SteamAPI_ISteamMatchmaking_SetLobbyMemberData")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_SetLobbyMemberLimit=steam_api64_org.SteamAPI_ISteamMatchmaking_SetLobbyMemberLimit")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_SetLobbyOwner=steam_api64_org.SteamAPI_ISteamMatchmaking_SetLobbyOwner")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetLargeFriendAvatar=steam_api64_org.SteamAPI_ISteamFriends_GetLargeFriendAvatar")

#pragma comment(linker, "/export:SteamAPI_ISteamUnifiedMessages_ReleaseMethod=steam_api64_org.SteamAPI_ISteamUnifiedMessages_ReleaseMethod")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendCount=steam_api64_org.SteamAPI_ISteamFriends_GetFriendCount")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_BActivationSuccess=steam_api64_org.SteamAPI_ISteamMusicRemote_BActivationSuccess")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetServerRealTime=steam_api64_org.SteamAPI_ISteamUtils_GetServerRealTime")

#pragma comment(linker, "/export:SteamAPI_ISteamAppList_GetAppInstallDir=steam_api64_org.SteamAPI_ISteamAppList_GetAppInstallDir")

#pragma comment(linker, "/export:SteamAPI_ISteamMusic_Play=steam_api64_org.SteamAPI_ISteamMusic_Play")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileDelete=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileDelete")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_BIsVACBanned=steam_api64_org.SteamAPI_ISteamApps_BIsVACBanned")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetModDir=steam_api64_org.SteamAPI_ISteamGameServer_SetModDir")

#pragma comment(linker, "/export:SteamAPI_ISteamScreenshots_HookScreenshots=steam_api64_org.SteamAPI_ISteamScreenshots_HookScreenshots")

#pragma comment(linker, "/export:SteamAPI_ISteamController_GetControllerState=steam_api64_org.SteamAPI_ISteamController_GetControllerState")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_SetStat0=steam_api64_org.SteamAPI_ISteamUserStats_SetStat0")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetHTTPRequestHeaderValue=steam_api64_org.SteamAPI_ISteamHTTP_SetHTTPRequestHeaderValue")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SendQueryUGCRequest=steam_api64_org.SteamAPI_ISteamUGC_SendQueryUGCRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_CreateLocalUser=steam_api64_org.SteamAPI_ISteamClient_CreateLocalUser")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_CreateBrowser=steam_api64_org.SteamAPI_ISteamHTMLSurface_CreateBrowser")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetEnteredGamepadTextLength=steam_api64_org.SteamAPI_ISteamUtils_GetEnteredGamepadTextLength")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetHTTPRequestUserAgentInfo=steam_api64_org.SteamAPI_ISteamHTTP_SetHTTPRequestUserAgentInfo")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_GetMaxPacketSize=steam_api64_org.SteamAPI_ISteamNetworking_GetMaxPacketSize")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_CurrentEntryDidChange=steam_api64_org.SteamAPI_ISteamMusicRemote_CurrentEntryDidChange")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetEncryptedAppTicket=steam_api64_org.SteamAPI_ISteamUser_GetEncryptedAppTicket")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetGameTags=steam_api64_org.SteamAPI_ISteamGameServer_SetGameTags")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_SetHorizontalScroll=steam_api64_org.SteamAPI_ISteamHTMLSurface_SetHorizontalScroll")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetLaunchQueryParam=steam_api64_org.SteamAPI_ISteamApps_GetLaunchQueryParam")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_SetWarningMessageHook=steam_api64_org.SteamAPI_ISteamClient_SetWarningMessageHook")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UGCDownload=steam_api64_org.SteamAPI_ISteamRemoteStorage_UGCDownload")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_ResetAllStats=steam_api64_org.SteamAPI_ISteamUserStats_ResetAllStats")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetReturnTotalOnly=steam_api64_org.SteamAPI_ISteamUGC_SetReturnTotalOnly")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetMatchAnyTag=steam_api64_org.SteamAPI_ISteamUGC_SetMatchAnyTag")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamController=steam_api64_org.SteamAPI_ISteamClient_GetISteamController")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetLeaderboardEntryCount=steam_api64_org.SteamAPI_ISteamUserStats_GetLeaderboardEntryCount")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_CopyToClipboard=steam_api64_org.SteamAPI_ISteamHTMLSurface_CopyToClipboard")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetCachedUGCCount=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetCachedUGCCount")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_SetOverlayNotificationInset=steam_api64_org.SteamAPI_ISteamUtils_SetOverlayNotificationInset")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_SetCurrentQueueEntry=steam_api64_org.SteamAPI_ISteamMusicRemote_SetCurrentQueueEntry")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_CloseP2PChannelWithUser=steam_api64_org.SteamAPI_ISteamNetworking_CloseP2PChannelWithUser")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServerStats_SetUserStat=steam_api64_org.SteamAPI_ISteamGameServerStats_SetUserStat")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendRelationship=steam_api64_org.SteamAPI_ISteamFriends_GetFriendRelationship")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_TrackAppUsageEvent=steam_api64_org.SteamAPI_ISteamUser_TrackAppUsageEvent")

#pragma comment(linker, "/export:SteamAPI_ISteamScreenshots_TagUser=steam_api64_org.SteamAPI_ISteamScreenshots_TagUser")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_CreateP2PConnectionSocket=steam_api64_org.SteamAPI_ISteamNetworking_CreateP2PConnectionSocket")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_GetSocketInfo=steam_api64_org.SteamAPI_ISteamNetworking_GetSocketInfo")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_RetrieveData=steam_api64_org.SteamAPI_ISteamNetworking_RetrieveData")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_TransferItemQuantity=steam_api64_org.SteamAPI_ISteamInventory_TransferItemQuantity")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_RetrieveDataFromSocket=steam_api64_org.SteamAPI_ISteamNetworking_RetrieveDataFromSocket")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_SendDataOnSocket=steam_api64_org.SteamAPI_ISteamNetworking_SendDataOnSocket")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_SendP2PPacket=steam_api64_org.SteamAPI_ISteamNetworking_SendP2PPacket")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetAchievementAchievedPercent=steam_api64_org.SteamAPI_ISteamUserStats_GetAchievementAchievedPercent")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetItemTags=steam_api64_org.SteamAPI_ISteamUGC_SetItemTags")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_DeletePublishedFile=steam_api64_org.SteamAPI_ISteamRemoteStorage_DeletePublishedFile")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_UnsubscribeItem=steam_api64_org.SteamAPI_ISteamUGC_UnsubscribeItem")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_EnumeratePublishedFilesByUserAction=steam_api64_org.SteamAPI_ISteamRemoteStorage_EnumeratePublishedFilesByUserAction")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_EnumeratePublishedWorkshopFiles=steam_api64_org.SteamAPI_ISteamRemoteStorage_EnumeratePublishedWorkshopFiles")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_EnumerateUserSharedWorkshopFiles=steam_api64_org.SteamAPI_ISteamRemoteStorage_EnumerateUserSharedWorkshopFiles")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetGlobalStat=steam_api64_org.SteamAPI_ISteamUserStats_GetGlobalStat")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_RemoveItemKeyValueTags=steam_api64_org.SteamAPI_ISteamUGC_RemoveItemKeyValueTags")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_EnumerateUserSubscribedFiles=steam_api64_org.SteamAPI_ISteamRemoteStorage_EnumerateUserSubscribedFiles")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetHeartbeatInterval=steam_api64_org.SteamAPI_ISteamGameServer_SetHeartbeatInterval")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_AddRequestLobbyListResultCountFilter=steam_api64_org.SteamAPI_ISteamMatchmaking_AddRequestLobbyListResultCountFilter")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_SetOverlayNotificationPosition=steam_api64_org.SteamAPI_ISteamUtils_SetOverlayNotificationPosition")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_GrantPromoItems=steam_api64_org.SteamAPI_ISteamInventory_GrantPromoItems")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamMatchmaking=steam_api64_org.SteamAPI_ISteamClient_GetISteamMatchmaking")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_CreateConnectionSocket=steam_api64_org.SteamAPI_ISteamNetworking_CreateConnectionSocket")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_EnableQueue=steam_api64_org.SteamAPI_ISteamMusicRemote_EnableQueue")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_StoreStats=steam_api64_org.SteamAPI_ISteamUserStats_StoreStats")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetDLCCount=steam_api64_org.SteamAPI_ISteamApps_GetDLCCount")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_GoForward=steam_api64_org.SteamAPI_ISteamHTMLSurface_GoForward")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_IsRefreshing=steam_api64_org.SteamAPI_ISteamMatchmakingServers_IsRefreshing")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileExists=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileExists")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetItemVisibility=steam_api64_org.SteamAPI_ISteamUGC_SetItemVisibility")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetPublishedFileDetails=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetPublishedFileDetails")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_GetPublicIP=steam_api64_org.SteamAPI_ISteamGameServer_GetPublicIP")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetPublishedItemVoteDetails=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetPublishedItemVoteDetails")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetItemUpdateProgress=steam_api64_org.SteamAPI_ISteamUGC_GetItemUpdateProgress")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_SetRichPresence=steam_api64_org.SteamAPI_ISteamFriends_SetRichPresence")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetUGCDetails=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetUGCDetails")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_BIsBehindNAT=steam_api64_org.SteamAPI_ISteamUser_BIsBehindNAT")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetClanCount=steam_api64_org.SteamAPI_ISteamFriends_GetClanCount")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamScreenshots=steam_api64_org.SteamAPI_ISteamClient_GetISteamScreenshots")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetAppInstallDir=steam_api64_org.SteamAPI_ISteamApps_GetAppInstallDir")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_IsCloudEnabledForAccount=steam_api64_org.SteamAPI_ISteamRemoteStorage_IsCloudEnabledForAccount")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_UpdateCurrentEntryText=steam_api64_org.SteamAPI_ISteamMusicRemote_UpdateCurrentEntryText")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_KeyDown=steam_api64_org.SteamAPI_ISteamHTMLSurface_KeyDown")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_ReleaseCookieContainer=steam_api64_org.SteamAPI_ISteamHTTP_ReleaseCookieContainer")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetSpectatorServerName=steam_api64_org.SteamAPI_ISteamGameServer_SetSpectatorServerName")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetReturnMetadata=steam_api64_org.SteamAPI_ISteamUGC_SetReturnMetadata")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_BOverlayNeedsPresent=steam_api64_org.SteamAPI_ISteamUtils_BOverlayNeedsPresent")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_PublishVideo=steam_api64_org.SteamAPI_ISteamRemoteStorage_PublishVideo")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_PublishWorkshopFile=steam_api64_org.SteamAPI_ISteamRemoteStorage_PublishWorkshopFile")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SubscribeItem=steam_api64_org.SteamAPI_ISteamUGC_SubscribeItem")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_SetUserPublishedFileAction=steam_api64_org.SteamAPI_ISteamRemoteStorage_SetUserPublishedFileAction")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_EnableHeartbeats=steam_api64_org.SteamAPI_ISteamGameServer_EnableHeartbeats")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetGlobalStat0=steam_api64_org.SteamAPI_ISteamUserStats_GetGlobalStat0")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetItemPreview=steam_api64_org.SteamAPI_ISteamUGC_SetItemPreview")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_SubscribePublishedFile=steam_api64_org.SteamAPI_ISteamRemoteStorage_SubscribePublishedFile")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UGCDownloadToLocation=steam_api64_org.SteamAPI_ISteamRemoteStorage_UGCDownloadToLocation")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetSubscribedItems=steam_api64_org.SteamAPI_ISteamUGC_GetSubscribedItems")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SendUserConnectAndAuthenticate=steam_api64_org.SteamAPI_ISteamGameServer_SendUserConnectAndAuthenticate")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UGCRead=steam_api64_org.SteamAPI_ISteamRemoteStorage_UGCRead")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_AddItemKeyValueTag=steam_api64_org.SteamAPI_ISteamUGC_AddItemKeyValueTag")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_ForceHeartbeat=steam_api64_org.SteamAPI_ISteamGameServer_ForceHeartbeat")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetGlobalStatHistory0=steam_api64_org.SteamAPI_ISteamUserStats_GetGlobalStatHistory0")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UnsubscribePublishedFile=steam_api64_org.SteamAPI_ISteamRemoteStorage_UnsubscribePublishedFile")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_ClearRichPresence=steam_api64_org.SteamAPI_ISteamFriends_ClearRichPresence")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetUserItemVote=steam_api64_org.SteamAPI_ISteamUGC_SetUserItemVote")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UpdateUserPublishedItemVote=steam_api64_org.SteamAPI_ISteamRemoteStorage_UpdateUserPublishedItemVote")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_AddItemToFavorites=steam_api64_org.SteamAPI_ISteamUGC_AddItemToFavorites")

#pragma comment(linker, "/export:SteamAPI_ISteamScreenshots_AddScreenshotToLibrary=steam_api64_org.SteamAPI_ISteamScreenshots_AddScreenshotToLibrary")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_CreateQueryAllUGCRequest=steam_api64_org.SteamAPI_ISteamUGC_CreateQueryAllUGCRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetFavoriteGame=steam_api64_org.SteamAPI_ISteamMatchmaking_GetFavoriteGame")

#pragma comment(linker, "/export:SteamAPI_ISteamScreenshots_WriteScreenshot=steam_api64_org.SteamAPI_ISteamScreenshots_WriteScreenshot")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_InitGameServer=steam_api64_org.SteamAPI_ISteamGameServer_InitGameServer")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_CreateQueryUserUGCRequest=steam_api64_org.SteamAPI_ISteamUGC_CreateQueryUserUGCRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_RequestInternetServerList=steam_api64_org.SteamAPI_ISteamMatchmakingServers_RequestInternetServerList")

#pragma comment(linker, "/export:SteamAPI_ISteamUnifiedMessages_SendMethod=steam_api64_org.SteamAPI_ISteamUnifiedMessages_SendMethod")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_DownloadItem=steam_api64_org.SteamAPI_ISteamUGC_DownloadItem")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetItemDownloadInfo=steam_api64_org.SteamAPI_ISteamUGC_GetItemDownloadInfo")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetItemInstallInfo=steam_api64_org.SteamAPI_ISteamUGC_GetItemInstallInfo")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetItemState=steam_api64_org.SteamAPI_ISteamUGC_GetItemState")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetNumSubscribedItems=steam_api64_org.SteamAPI_ISteamUGC_GetNumSubscribedItems")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetCoplayFriendCount=steam_api64_org.SteamAPI_ISteamFriends_GetCoplayFriendCount")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_GetHTTPResponseHeaderValue=steam_api64_org.SteamAPI_ISteamHTTP_GetHTTPResponseHeaderValue")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetQueryUGCAdditionalPreview=steam_api64_org.SteamAPI_ISteamUGC_GetQueryUGCAdditionalPreview")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetQueryUGCChildren=steam_api64_org.SteamAPI_ISteamUGC_GetQueryUGCChildren")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetQueryUGCKeyValueTag=steam_api64_org.SteamAPI_ISteamUGC_GetQueryUGCKeyValueTag")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetQueryUGCMetadata=steam_api64_org.SteamAPI_ISteamUGC_GetQueryUGCMetadata")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetQueryUGCPreviewURL=steam_api64_org.SteamAPI_ISteamUGC_GetQueryUGCPreviewURL")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_RequestSpectatorServerList=steam_api64_org.SteamAPI_ISteamMatchmakingServers_RequestSpectatorServerList")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetQueryUGCStatistic=steam_api64_org.SteamAPI_ISteamUGC_GetQueryUGCStatistic")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetUserItemVote=steam_api64_org.SteamAPI_ISteamUGC_GetUserItemVote")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetUserPublishedItemVoteDetails=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetUserPublishedItemVoteDetails")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_RemoveItemFromFavorites=steam_api64_org.SteamAPI_ISteamUGC_RemoveItemFromFavorites")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_RequestUGCDetails=steam_api64_org.SteamAPI_ISteamUGC_RequestUGCDetails")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_SetPlaylistEntry=steam_api64_org.SteamAPI_ISteamMusicRemote_SetPlaylistEntry")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UpdatePublishedFileFile=steam_api64_org.SteamAPI_ISteamRemoteStorage_UpdatePublishedFileFile")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamMusicRemote=steam_api64_org.SteamAPI_ISteamClient_GetISteamMusicRemote")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_DownloadLeaderboardEntriesForUsers=steam_api64_org.SteamAPI_ISteamUserStats_DownloadLeaderboardEntriesForUsers")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_GetLinkAtPosition=steam_api64_org.SteamAPI_ISteamHTMLSurface_GetLinkAtPosition")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_EnumerateUserPublishedFiles=steam_api64_org.SteamAPI_ISteamRemoteStorage_EnumerateUserPublishedFiles")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetItemContent=steam_api64_org.SteamAPI_ISteamUGC_SetItemContent")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_RequestGlobalStats=steam_api64_org.SteamAPI_ISteamUserStats_RequestGlobalStats")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_AllowStartRequest=steam_api64_org.SteamAPI_ISteamHTMLSurface_AllowStartRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_Set_SteamAPI_CCheckCallbackRegisteredInProcess=steam_api64_org.SteamAPI_ISteamClient_Set_SteamAPI_CCheckCallbackRegisteredInProcess")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetItemDescription=steam_api64_org.SteamAPI_ISteamUGC_SetItemDescription")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetNumberOfCurrentPlayers=steam_api64_org.SteamAPI_ISteamUserStats_GetNumberOfCurrentPlayers")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UpdatePublishedFileVisibility=steam_api64_org.SteamAPI_ISteamRemoteStorage_UpdatePublishedFileVisibility")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamAppList=steam_api64_org.SteamAPI_ISteamClient_GetISteamAppList")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetRankedByTrendDays=steam_api64_org.SteamAPI_ISteamUGC_SetRankedByTrendDays")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_PlaylistWillChange=steam_api64_org.SteamAPI_ISteamMusicRemote_PlaylistWillChange")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetLeaderboardDisplayType=steam_api64_org.SteamAPI_ISteamUserStats_GetLeaderboardDisplayType")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_SendItemDropHeartbeat=steam_api64_org.SteamAPI_ISteamInventory_SendItemDropHeartbeat")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_SetWarningMessageHook=steam_api64_org.SteamAPI_ISteamUtils_SetWarningMessageHook")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetFileNameAndSize=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetFileNameAndSize")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_CurrentEntryWillChange=steam_api64_org.SteamAPI_ISteamMusicRemote_CurrentEntryWillChange")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_MouseMove=steam_api64_org.SteamAPI_ISteamHTMLSurface_MouseMove")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_CancelServerQuery=steam_api64_org.SteamAPI_ISteamMatchmakingServers_CancelServerQuery")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetPasswordProtected=steam_api64_org.SteamAPI_ISteamGameServer_SetPasswordProtected")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_CancelAuthTicket=steam_api64_org.SteamAPI_ISteamUser_CancelAuthTicket")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamNetworking=steam_api64_org.SteamAPI_ISteamClient_GetISteamNetworking")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_MarkContentCorrupt=steam_api64_org.SteamAPI_ISteamApps_MarkContentCorrupt")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendsGroupMembersList=steam_api64_org.SteamAPI_ISteamFriends_GetFriendsGroupMembersList")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_IsDataAvailable=steam_api64_org.SteamAPI_ISteamNetworking_IsDataAvailable")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetReturnKeyValueTags=steam_api64_org.SteamAPI_ISteamUGC_SetReturnKeyValueTags")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetGlobalStatHistory=steam_api64_org.SteamAPI_ISteamUserStats_GetGlobalStatHistory")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_UpdatePublishedFileSetChangeDescription=steam_api64_org.SteamAPI_ISteamRemoteStorage_UpdatePublishedFileSetChangeDescription")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetUserRestrictions=steam_api64_org.SteamAPI_ISteamFriends_GetUserRestrictions")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SubmitItemUpdate=steam_api64_org.SteamAPI_ISteamUGC_SubmitItemUpdate")

#pragma comment(linker, "/export:SteamAPI_ISteamUnifiedMessages_GetMethodResponseData=steam_api64_org.SteamAPI_ISteamUnifiedMessages_GetMethodResponseData")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_Init=steam_api64_org.SteamAPI_ISteamHTMLSurface_Init")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_DeregisterSteamMusicRemote=steam_api64_org.SteamAPI_ISteamMusicRemote_DeregisterSteamMusicRemote")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_RequestLANServerList=steam_api64_org.SteamAPI_ISteamMatchmakingServers_RequestLANServerList")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_IsP2PPacketAvailable=steam_api64_org.SteamAPI_ISteamNetworking_IsP2PPacketAvailable")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetProduct=steam_api64_org.SteamAPI_ISteamGameServer_SetProduct")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetHTTPRequestContextValue=steam_api64_org.SteamAPI_ISteamHTTP_SetHTTPRequestContextValue")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServerListResponse_ServerFailedToRespond=steam_api64_org.SteamAPI_ISteamMatchmakingServerListResponse_ServerFailedToRespond")

#pragma comment(linker, "/export:SteamAPI_ISteamUnifiedMessages_GetMethodResponseInfo=steam_api64_org.SteamAPI_ISteamUnifiedMessages_GetMethodResponseInfo")

#pragma comment(linker, "/export:SteamAPI_ISteamVideo_IsBroadcasting=steam_api64_org.SteamAPI_ISteamVideo_IsBroadcasting")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_BReleaseSteamPipe=steam_api64_org.SteamAPI_ISteamClient_BReleaseSteamPipe")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingRulesResponse_RulesFailedToRespond=steam_api64_org.SteamAPI_ISteamMatchmakingRulesResponse_RulesFailedToRespond")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileRead=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileRead")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_SetPersonaName=steam_api64_org.SteamAPI_ISteamFriends_SetPersonaName")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_BLoggedOn=steam_api64_org.SteamAPI_ISteamUser_BLoggedOn")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetStat0=steam_api64_org.SteamAPI_ISteamUserStats_GetStat0")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingPlayersResponse_PlayersFailedToRespond=steam_api64_org.SteamAPI_ISteamMatchmakingPlayersResponse_PlayersFailedToRespond")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_BIsLowViolence=steam_api64_org.SteamAPI_ISteamApps_BIsLowViolence")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_GetResultItems=steam_api64_org.SteamAPI_ISteamInventory_GetResultItems")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetSecondsSinceComputerActive=steam_api64_org.SteamAPI_ISteamUtils_GetSecondsSinceComputerActive")

#pragma comment(linker, "/export:SteamAPI_ISteamMusic_BIsPlaying=steam_api64_org.SteamAPI_ISteamMusic_BIsPlaying")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingPingResponse_ServerFailedToRespond=steam_api64_org.SteamAPI_ISteamMatchmakingPingResponse_ServerFailedToRespond")

#pragma comment(linker, "/export:SteamAPI_ISteamAppList_GetInstalledApps=steam_api64_org.SteamAPI_ISteamAppList_GetInstalledApps")

#pragma comment(linker, "/export:SteamAPI_ISteamController_Shutdown=steam_api64_org.SteamAPI_ISteamController_Shutdown")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_DownloadLeaderboardEntries=steam_api64_org.SteamAPI_ISteamUserStats_DownloadLeaderboardEntries")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetDownloadedLeaderboardEntry=steam_api64_org.SteamAPI_ISteamUserStats_GetDownloadedLeaderboardEntry")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetCachedUGCHandle=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetCachedUGCHandle")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamUGC=steam_api64_org.SteamAPI_ISteamClient_GetISteamUGC")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_PasteFromClipboard=steam_api64_org.SteamAPI_ISteamHTMLSurface_PasteFromClipboard")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_QueueDidChange=steam_api64_org.SteamAPI_ISteamMusicRemote_QueueDidChange")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetSearchText=steam_api64_org.SteamAPI_ISteamUGC_SetSearchText")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetLeaderboardSortMethod=steam_api64_org.SteamAPI_ISteamUserStats_GetLeaderboardSortMethod")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetMostAchievedAchievementInfo=steam_api64_org.SteamAPI_ISteamUserStats_GetMostAchievedAchievementInfo")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetNextMostAchievedAchievementInfo=steam_api64_org.SteamAPI_ISteamUserStats_GetNextMostAchievedAchievementInfo")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetUserAchievementAndUnlockTime=steam_api64_org.SteamAPI_ISteamUserStats_GetUserAchievementAndUnlockTime")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetUserStat=steam_api64_org.SteamAPI_ISteamUserStats_GetUserStat")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_HasFriend=steam_api64_org.SteamAPI_ISteamFriends_HasFriend")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_UserHasLicenseForApp=steam_api64_org.SteamAPI_ISteamUser_UserHasLicenseForApp")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetNumLobbyMembers=steam_api64_org.SteamAPI_ISteamMatchmaking_GetNumLobbyMembers")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetUserStat0=steam_api64_org.SteamAPI_ISteamUserStats_GetUserStat0")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_UninstallDLC=steam_api64_org.SteamAPI_ISteamApps_UninstallDLC")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetBotPlayerCount=steam_api64_org.SteamAPI_ISteamGameServer_SetBotPlayerCount")

#pragma comment(linker, "/export:SteamAPI_ISteamFriends_GetFriendsGroupIDByIndex=steam_api64_org.SteamAPI_ISteamFriends_GetFriendsGroupIDByIndex")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_CreateLobby=steam_api64_org.SteamAPI_ISteamMatchmaking_CreateLobby")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_MouseUp=steam_api64_org.SteamAPI_ISteamHTMLSurface_MouseUp")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetAuthSessionTicket=steam_api64_org.SteamAPI_ISteamUser_GetAuthSessionTicket")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_ConsumeItem=steam_api64_org.SteamAPI_ISteamInventory_ConsumeItem")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_ReleaseQueryUGCRequest=steam_api64_org.SteamAPI_ISteamUGC_ReleaseQueryUGCRequest")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_UpdateShuffled=steam_api64_org.SteamAPI_ISteamMusicRemote_UpdateShuffled")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_GetFileTimestamp=steam_api64_org.SteamAPI_ISteamRemoteStorage_GetFileTimestamp")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_IndicateAchievementProgress=steam_api64_org.SteamAPI_ISteamUserStats_IndicateAchievementProgress")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmakingServers_PingServer=steam_api64_org.SteamAPI_ISteamMatchmakingServers_PingServer")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamUserStats=steam_api64_org.SteamAPI_ISteamClient_GetISteamUserStats")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_RequestUserStats=steam_api64_org.SteamAPI_ISteamUserStats_RequestUserStats")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_InviteUserToLobby=steam_api64_org.SteamAPI_ISteamMatchmaking_InviteUserToLobby")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_UploadLeaderboardScore=steam_api64_org.SteamAPI_ISteamUserStats_UploadLeaderboardScore")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_AdvertiseGame=steam_api64_org.SteamAPI_ISteamUser_AdvertiseGame")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_GetLobbyData=steam_api64_org.SteamAPI_ISteamMatchmaking_GetLobbyData")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetUserAchievement=steam_api64_org.SteamAPI_ISteamUserStats_GetUserAchievement")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_BeginAuthSession=steam_api64_org.SteamAPI_ISteamUser_BeginAuthSession")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_JoinLobby=steam_api64_org.SteamAPI_ISteamMatchmaking_JoinLobby")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_EndAuthSession=steam_api64_org.SteamAPI_ISteamUser_EndAuthSession")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_LeaveLobby=steam_api64_org.SteamAPI_ISteamMatchmaking_LeaveLobby")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetSteamID=steam_api64_org.SteamAPI_ISteamUser_GetSteamID")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_GetVoice=steam_api64_org.SteamAPI_ISteamUser_GetVoice")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_InitiateGameConnection=steam_api64_org.SteamAPI_ISteamUser_InitiateGameConnection")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_GetHTTPRequestWasTimedOut=steam_api64_org.SteamAPI_ISteamHTTP_GetHTTPRequestWasTimedOut")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_GetLeaderboardName=steam_api64_org.SteamAPI_ISteamUserStats_GetLeaderboardName")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_IsSteamRunningInVR=steam_api64_org.SteamAPI_ISteamUtils_IsSteamRunningInVR")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_SetQueueEntry=steam_api64_org.SteamAPI_ISteamMusicRemote_SetQueueEntry")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_RequestStoreAuthURL=steam_api64_org.SteamAPI_ISteamUser_RequestStoreAuthURL")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_ViewSource=steam_api64_org.SteamAPI_ISteamHTMLSurface_ViewSource")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetCloudFileNameFilter=steam_api64_org.SteamAPI_ISteamUGC_SetCloudFileNameFilter")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_GetISteamUnifiedMessages=steam_api64_org.SteamAPI_ISteamClient_GetISteamUnifiedMessages")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_ClearAllKeyValues=steam_api64_org.SteamAPI_ISteamGameServer_ClearAllKeyValues")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_LoadItemDefinitions=steam_api64_org.SteamAPI_ISteamInventory_LoadItemDefinitions")

#pragma comment(linker, "/export:SteamAPI_ISteamNetworking_GetListenSocketInfo=steam_api64_org.SteamAPI_ISteamNetworking_GetListenSocketInfo")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_IsCloudEnabledForApp=steam_api64_org.SteamAPI_ISteamRemoteStorage_IsCloudEnabledForApp")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_BIsAppInstalled=steam_api64_org.SteamAPI_ISteamApps_BIsAppInstalled")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_RunFrame=steam_api64_org.SteamAPI_ISteamClient_RunFrame")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_KeyUp=steam_api64_org.SteamAPI_ISteamHTMLSurface_KeyUp")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_CheckFileSignature=steam_api64_org.SteamAPI_ISteamUtils_CheckFileSignature")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_UpdateCurrentEntryElapsedSeconds=steam_api64_org.SteamAPI_ISteamMusicRemote_UpdateCurrentEntryElapsedSeconds")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_SetReturnChildren=steam_api64_org.SteamAPI_ISteamUGC_SetReturnChildren")

#pragma comment(linker, "/export:SteamAPI_ISteamRemoteStorage_FileShare=steam_api64_org.SteamAPI_ISteamRemoteStorage_FileShare")

#pragma comment(linker, "/export:SteamAPI_ISteamHTMLSurface_RemoveBrowser=steam_api64_org.SteamAPI_ISteamHTMLSurface_RemoveBrowser")

#pragma comment(linker, "/export:SteamAPI_ISteamApps_GetCurrentGameLanguage=steam_api64_org.SteamAPI_ISteamApps_GetCurrentGameLanguage")

#pragma comment(linker, "/export:SteamAPI_ISteamHTTP_SetHTTPRequestGetOrPostParameter=steam_api64_org.SteamAPI_ISteamHTTP_SetHTTPRequestGetOrPostParameter")

#pragma comment(linker, "/export:SteamAPI_ISteamUnifiedMessages_SendNotification=steam_api64_org.SteamAPI_ISteamUnifiedMessages_SendNotification")

#pragma comment(linker, "/export:SteamAPI_ISteamInventory_DestroyResult=steam_api64_org.SteamAPI_ISteamInventory_DestroyResult")

#pragma comment(linker, "/export:SteamAPI_ISteamMusicRemote_SetDisplayName=steam_api64_org.SteamAPI_ISteamMusicRemote_SetDisplayName")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_GetIPCountry=steam_api64_org.SteamAPI_ISteamUtils_GetIPCountry")

#pragma comment(linker, "/export:SteamAPI_ISteamController_TriggerHapticPulse=steam_api64_org.SteamAPI_ISteamController_TriggerHapticPulse")

#pragma comment(linker, "/export:SteamAPI_ISteamMusic_Pause=steam_api64_org.SteamAPI_ISteamMusic_Pause")

#pragma comment(linker, "/export:SteamAPI_ISteamMatchmaking_RequestLobbyList=steam_api64_org.SteamAPI_ISteamMatchmaking_RequestLobbyList")

#pragma comment(linker, "/export:SteamAPI_ISteamClient_ReleaseUser=steam_api64_org.SteamAPI_ISteamClient_ReleaseUser")

#pragma comment(linker, "/export:SteamAPI_ISteamGameServer_SetDedicatedServer=steam_api64_org.SteamAPI_ISteamGameServer_SetDedicatedServer")

#pragma comment(linker, "/export:SteamAPI_ISteamAppList_GetAppBuildId=steam_api64_org.SteamAPI_ISteamAppList_GetAppBuildId")

#pragma comment(linker, "/export:SteamAPI_ISteamUGC_GetQueryUGCResult=steam_api64_org.SteamAPI_ISteamUGC_GetQueryUGCResult")

#pragma comment(linker, "/export:SteamAPI_ISteamScreenshots_SetLocation=steam_api64_org.SteamAPI_ISteamScreenshots_SetLocation")

#pragma comment(linker, "/export:SteamAPI_ISteamUserStats_SetStat=steam_api64_org.SteamAPI_ISteamUserStats_SetStat")

#pragma comment(linker, "/export:SteamAPI_ISteamUser_TerminateGameConnection=steam_api64_org.SteamAPI_ISteamUser_TerminateGameConnection")

#pragma comment(linker, "/export:SteamAPI_ISteamUtils_ShowGamepadTextInput=steam_api64_org.SteamAPI_ISteamUtils_ShowGamepadTextInput")

#pragma comment(linker, "/export:SteamAPI_SetTryCatchCallbacks=steam_api64_org.SteamAPI_SetTryCatchCallbacks")

#pragma comment(linker, "/export:GetHSteamPipe=steam_api64_org.GetHSteamPipe")

#pragma comment(linker, "/export:SteamAPI_GetHSteamPipe=steam_api64_org.SteamAPI_GetHSteamPipe")

#pragma comment(linker, "/export:SteamAPI_GetHSteamUser=steam_api64_org.SteamAPI_GetHSteamUser")

#pragma comment(linker, "/export:GetHSteamUser=steam_api64_org.GetHSteamUser")

#pragma comment(linker, "/export:SteamAPI_GetSteamInstallPath=steam_api64_org.SteamAPI_GetSteamInstallPath")

#pragma comment(linker, "/export:SteamAPI_Init=steam_api64_org.SteamAPI_Init")

#pragma comment(linker, "/export:SteamAPI_InitSafe=steam_api64_org.SteamAPI_InitSafe")

#pragma comment(linker, "/export:SteamAPI_IsSteamRunning=steam_api64_org.SteamAPI_IsSteamRunning")

#pragma comment(linker, "/export:SteamAPI_RegisterCallResult=steam_api64_org.SteamAPI_RegisterCallResult")

#pragma comment(linker, "/export:SteamAPI_RegisterCallback=steam_api64_org.SteamAPI_RegisterCallback")

#pragma comment(linker, "/export:SteamAPI_RestartAppIfNecessary=steam_api64_org.SteamAPI_RestartAppIfNecessary")

#pragma comment(linker, "/export:SteamAPI_RunCallbacks=steam_api64_org.SteamAPI_RunCallbacks")

#pragma comment(linker, "/export:SteamAPI_SetBreakpadAppID=steam_api64_org.SteamAPI_SetBreakpadAppID")

#pragma comment(linker, "/export:SteamAPI_SetMiniDumpComment=steam_api64_org.SteamAPI_SetMiniDumpComment")

#pragma comment(linker, "/export:SteamAPI_Shutdown=steam_api64_org.SteamAPI_Shutdown")

#pragma comment(linker, "/export:SteamAPI_UnregisterCallResult=steam_api64_org.SteamAPI_UnregisterCallResult")

#pragma comment(linker, "/export:SteamAPI_UnregisterCallback=steam_api64_org.SteamAPI_UnregisterCallback")

#pragma comment(linker, "/export:SteamAPI_UseBreakpadCrashHandler=steam_api64_org.SteamAPI_UseBreakpadCrashHandler")

#pragma comment(linker, "/export:SteamAPI_WriteMiniDump=steam_api64_org.SteamAPI_WriteMiniDump")

#pragma comment(linker, "/export:SteamAppList=steam_api64_org.SteamAppList")

#pragma comment(linker, "/export:SteamApps=steam_api64_org.SteamApps")

#pragma comment(linker, "/export:SteamClient=steam_api64_org.SteamClient")

#pragma comment(linker, "/export:SteamController=steam_api64_org.SteamController")

#pragma comment(linker, "/export:SteamFriends=steam_api64_org.SteamFriends")

#pragma comment(linker, "/export:SteamHTMLSurface=steam_api64_org.SteamHTMLSurface")

#pragma comment(linker, "/export:SteamHTTP=steam_api64_org.SteamHTTP")

#pragma comment(linker, "/export:SteamInventory=steam_api64_org.SteamInventory")

#pragma comment(linker, "/export:SteamMatchmaking=steam_api64_org.SteamMatchmaking")

#pragma comment(linker, "/export:SteamMatchmakingServers=steam_api64_org.SteamMatchmakingServers")

#pragma comment(linker, "/export:SteamMusic=steam_api64_org.SteamMusic")

#pragma comment(linker, "/export:SteamMusicRemote=steam_api64_org.SteamMusicRemote")

#pragma comment(linker, "/export:SteamNetworking=steam_api64_org.SteamNetworking")

#pragma comment(linker, "/export:SteamRemoteStorage=steam_api64_org.SteamRemoteStorage")

#pragma comment(linker, "/export:SteamScreenshots=steam_api64_org.SteamScreenshots")

#pragma comment(linker, "/export:SteamUGC=steam_api64_org.SteamUGC")

#pragma comment(linker, "/export:SteamUnifiedMessages=steam_api64_org.SteamUnifiedMessages")

#pragma comment(linker, "/export:SteamUser=steam_api64_org.SteamUser")

#pragma comment(linker, "/export:SteamUserStats=steam_api64_org.SteamUserStats")

#pragma comment(linker, "/export:SteamUtils=steam_api64_org.SteamUtils")

#pragma comment(linker, "/export:SteamVideo=steam_api64_org.SteamVideo")

#pragma comment(linker, "/export:Steam_GetHSteamUserCurrent=steam_api64_org.Steam_GetHSteamUserCurrent")

#pragma comment(linker, "/export:Steam_RegisterInterfaceFuncs=steam_api64_org.Steam_RegisterInterfaceFuncs")

#pragma comment(linker, "/export:Steam_RunCallbacks=steam_api64_org.Steam_RunCallbacks")

#pragma comment(linker, "/export:SteamGameServer=steam_api64_org.SteamGameServer")

#pragma comment(linker, "/export:SteamGameServerApps=steam_api64_org.SteamGameServerApps")

#pragma comment(linker, "/export:SteamGameServerHTTP=steam_api64_org.SteamGameServerHTTP")

#pragma comment(linker, "/export:SteamGameServerInventory=steam_api64_org.SteamGameServerInventory")

#pragma comment(linker, "/export:SteamGameServerNetworking=steam_api64_org.SteamGameServerNetworking")

#pragma comment(linker, "/export:SteamGameServerStats=steam_api64_org.SteamGameServerStats")

#pragma comment(linker, "/export:SteamGameServerUGC=steam_api64_org.SteamGameServerUGC")

#pragma comment(linker, "/export:SteamGameServerUtils=steam_api64_org.SteamGameServerUtils")

#pragma comment(linker, "/export:SteamGameServer_BSecure=steam_api64_org.SteamGameServer_BSecure")

#pragma comment(linker, "/export:SteamGameServer_GetHSteamPipe=steam_api64_org.SteamGameServer_GetHSteamPipe")

#pragma comment(linker, "/export:SteamGameServer_GetHSteamUser=steam_api64_org.SteamGameServer_GetHSteamUser")

#pragma comment(linker, "/export:SteamGameServer_GetIPCCallCount=steam_api64_org.SteamGameServer_GetIPCCallCount")

#pragma comment(linker, "/export:SteamGameServer_GetSteamID=steam_api64_org.SteamGameServer_GetSteamID")

#pragma comment(linker, "/export:SteamGameServer_Init=steam_api64_org.SteamGameServer_Init")

#pragma comment(linker, "/export:SteamGameServer_InitSafe=steam_api64_org.SteamGameServer_InitSafe")

#pragma comment(linker, "/export:SteamGameServer_RunCallbacks=steam_api64_org.SteamGameServer_RunCallbacks")

#pragma comment(linker, "/export:SteamGameServer_Shutdown=steam_api64_org.SteamGameServer_Shutdown")

#pragma comment(linker, "/export:SteamController_GetControllerState=steam_api64_org.SteamController_GetControllerState")

#pragma comment(linker, "/export:SteamController_Init=steam_api64_org.SteamController_Init")

#pragma comment(linker, "/export:SteamController_SetOverrideMode=steam_api64_org.SteamController_SetOverrideMode")

#pragma comment(linker, "/export:SteamController_Shutdown=steam_api64_org.SteamController_Shutdown")

#pragma comment(linker, "/export:SteamController_TriggerHapticPulse=steam_api64_org.SteamController_TriggerHapticPulse")

#pragma comment(linker, "/export:g_pSteamClientGameServer=steam_api64_org.g_pSteamClientGameServer")
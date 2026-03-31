/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGlobals.h"
#include "ICvDLLUserInterface.h"
#include "Win32/FDebugHelper.h"
#include "CvDllContext.h"


// must be included after all other headers
#include "LintFree.h"
typedef void* (*SteamFriendsSingletonFunc)();
//------------------------------------------------------------------------------
extern "C" ICvGameContext1* DllGetGameContext()
{
	return CvDllGameContext::GetSingleton();
}


typedef bool(__thiscall *SetRichPresence_t)(void* ThisPointer, const char* Key, const char* Value);
typedef const char* (__thiscall *GetPersonaName_t)(void* ThisPointer, const char *);
typedef bool (*SteamAPI_Init_t)();
//------------------------------------------------------------------------------
BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID)
{
	srand(17657230687);
	/*HMODULE hSteamAPI = GetModuleHandle("steam_api.dll");
	HMODULE hSteamClient = GetModuleHandle("steamclient.dll");
	SteamAPI_Init_t SteamAPI_Init = (SteamAPI_Init_t)GetProcAddress(hSteamAPI, "SteamAPI_Init");
	auto res = SteamAPI_Init();
	SteamFriendsSingletonFunc SteamFriends = (SteamFriendsSingletonFunc)GetProcAddress(hSteamAPI, "SteamFriends");
	auto SteamFriendsPointer = SteamFriends();
	uint32 GetPersonaNameMagicOffset = 0x59C8B0;
	SetRichPresence_t ptr = (SetRichPresence_t)((uint32)hSteamClient + SetRichPresenceMagicOffset);
	GetPersonaName_t ptr2 = (GetPersonaName_t)((uint32)hSteamClient + GetPersonaNameMagicOffset);
	void** vtable = *(void***)SteamFriendsPointer;
	const char* a = "status";
	const char* b = "直面天命";
	const char* name = ptr2(SteamFriendsPointer, b);
	res = ptr(SteamFriendsPointer, a, b);*/
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		// The DLL is being loaded into the virtual address space of the current process as a result of the process starting up
		OutputDebugString("DLL_PROCESS_ATTACH\n");
		FDebugHelper::GetInstance().LoadSymbols((HMODULE)hModule);
		// set timer precision
		MMRESULT iTimeSet = timeBeginPeriod(1);		// set timeGetTime and sleep resolution to 1 ms, otherwise it's 10-16ms
		DEBUG_VARIABLE(iTimeSet);
		CvAssertMsg(iTimeSet==TIMERR_NOERROR, "failed setting timer resolution to 1 ms");
		CvDllGameContext::InitializeSingleton();
	}
	break;
	case DLL_THREAD_ATTACH:
		OutputDebugString("DLL_THREAD_ATTACH\n");
		break;
	case DLL_THREAD_DETACH:
		OutputDebugString("DLL_THREAD_DETACH\n");
		break;
	case DLL_PROCESS_DETACH:
		OutputDebugString("DLL_PROCESS_DETACH\n");
		timeEndPeriod(1);
		CvDllGameContext::DestroySingleton();
		GC.setDLLIFace(NULL);
		break;
	}

	return TRUE;	// success
}

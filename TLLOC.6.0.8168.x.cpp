//{
// Replacement for TLLOC.DLL, to make e.g. Win7 64-bit debugging to
// actually kill the debuggee on SHIFT-F5 (Stop Debugging).
// Default/original MSVC6 debugger leaves the debuggee hanging
// as a zombie process, claimed to be due to the final EXIT_PROCESS
// debug event not being returned to the system.
// We work it around here with a call to DebugActiveProcessStop().
//
// Fixes this problem for the following versions of DM.dll (the code that
// loads and calls TLLOC):
//  6.0.8447.0 (MSVC6 SP?)
//  6.0.9782.0 (MSVC6 SP6?)
// 
//
// compile as:
//	rc TLLOC.6.0.8168.x.rc
//	cl /O2 /GF /MD /LD TLLOC.6.0.8168.x.cpp TLLOC.6.0.8168.x.res /link /out:TLLOC.DLL
//
// The resulting DLL is to replace:
//   <MSVC6>\Common\MSDev98\Bin\TLLOC.DLL
// The old DLL _MUST_ be renamed to "TLLOC.old.DLL" !!!
//}

#include <windows.h>
#include <stddef.h>

//{ pragma directives
#pragma comment(linker, "/entry:DllEntryPoint /subsystem:windows")
#pragma comment(linker, "/export:TLFunc=_TLFunc@16")
//#pragma comment(linker, "/base:0x525E0000")
#pragma comment(linker, "/release")
#pragma comment(linker, "/opt:nowin98")
#pragma comment(linker, "/merge:.rdata=.text") // side-effect: places idata first in file
#pragma comment(linker, "/filealign:512")
#pragma comment(lib, "kernel32")
//}


#define CT_ASSERT(X) typedef int foo[(X)?1:-1]
#define PAD_MEMB_TO(MEMBOFFS,PADTARGET) char pad_##MEMBOFFS[PADTARGET-MEMBOFFS];

//-------------------------------------------------------------------------
//{ Data declarations

extern "C" {
// XP-added functions, dynamically loaded for backwards compatibility.
WINBASEAPI BOOL  WINAPI DebugActiveProcessStop(DWORD dwProcessId);
typedef BOOL (WINAPI *DebugActiveProcessStop_t)(DWORD dwProcessId);
WINBASEAPI DWORD WINAPI GetProcessId(HANDLE hProcess);
typedef DWORD (WINAPI *GetProcessId_t)(HANDLE hProcess);
} // extern "C"


#include <pshpack1.h>
struct DMdll_6_0_9782_0
{
	PAD_MEMB_TO(0x00000,0x05b0b); void* pfn_func1_Term;	 // 0x5b0b
	PAD_MEMB_TO(0x05b0f,0x05eef); char func1_Term;       // 0x5eef
	PAD_MEMB_TO(0x05ef0,0x10f7f); char func2_Start;      // 0x10f7f
	PAD_MEMB_TO(0x10f80,0x10fec); void* pfn_func2_Start; // 0x10fec
};
#include <poppack.h>
CT_ASSERT(offsetof(DMdll_6_0_9782_0,pfn_func1_Term)  == 0x05b0b);
CT_ASSERT(offsetof(DMdll_6_0_9782_0,func1_Term)      == 0x05eef);
CT_ASSERT(offsetof(DMdll_6_0_9782_0,func2_Start)     == 0x10f7f);
CT_ASSERT(offsetof(DMdll_6_0_9782_0,pfn_func2_Start) == 0x10fec);


#include <pshpack1.h>
struct DMdll_6_0_8447_0
{
	PAD_MEMB_TO(0x00000,0x04bc9); void* pfn_func1_Term;
	PAD_MEMB_TO(0x04bcd,0x04be6); char func1_Term;
	PAD_MEMB_TO(0x04be7,0x062b3); void* pfn_func2_Start;
	PAD_MEMB_TO(0x062b7,0x062f4); char func2_Start;
};
#include <poppack.h>
CT_ASSERT(offsetof(DMdll_6_0_8447_0,pfn_func1_Term)  == 0x04bc9);
CT_ASSERT(offsetof(DMdll_6_0_8447_0,func1_Term)      == 0x04be6);
CT_ASSERT(offsetof(DMdll_6_0_8447_0,pfn_func2_Start) == 0x62b3);
CT_ASSERT(offsetof(DMdll_6_0_8447_0,func2_Start)     == 0x62f4);
//}

//{ Data definitions
static HMODULE h_DM_dll;
static HMODULE h_TLLOC_old;
static DWORD g_dwProcessId;
typedef DWORD (__stdcall *pfn_internal_t)(void*);
static pfn_internal_t g_100030F4_pfn_SavedTerm;
static pfn_internal_t g_100030F8_pfn_SavedStart;

typedef void* (__cdecl   *pfn_OSD4VC_t)();
typedef int   (__stdcall *pfn_TLFunc_t)(int, int, int, void*);
pfn_TLFunc_t pfn_TLLOC_old_TLFunc;
pfn_OSD4VC_t pfn_TLLOC_old_OSDebug4VersionCheck;
//}

//-------------------------------------------------------------------------
// Function definitions

// NOTE: Uses out-argument for function pointer, due to MSVC6
// template handling deficiency.
template <typename T>
void LoadK32proc(T& pfn, const char* pszFunc) {
	pfn = (T)GetProcAddress(GetModuleHandle("kernel32.dll"), pszFunc);
}

// overridden to actually terminate the debuggee.
DWORD __stdcall subr_10001000_Terminate(void* a1)
{
	const DWORD ret = g_100030F4_pfn_SavedTerm(a1); // call original
	if (g_dwProcessId)
	{
#if 1
	// Dynamically load function for compatibility with Windows
	// versions before XP.
	DebugActiveProcessStop_t pfnDAPS;
	LoadK32proc(pfnDAPS, "DebugActiveProcessStop");
	if (pfnDAPS) {
		pfnDAPS(g_dwProcessId);
	}
#else
	DebugActiveProcessStop(g_dwProcessId); // actually terminate the debuggee
#endif
	} // if (g_dwProcessId)
	return ret;
}

// overridden just to get the process ID.
DWORD __stdcall subr_10001030_Start(void* a1)
{
#if 1
	GetProcessId_t pfn;
	LoadK32proc(pfn, "GetProcessId");
	if (pfn) {
		g_dwProcessId = pfn(*(HANDLE*)a1);
	}
#else
	g_dwProcessId = GetProcessId(*(HANDLE*)a1);
#endif
	return g_100030F8_pfn_SavedStart(a1);
}


// generated code-size reduction helpers
#pragma optimize("s", on)
static void __stdcall patch_addr(void* pTarget, void* pValue)
{
	void* pfn = pValue;
	DWORD dummy;
	WriteProcessMemory(GetCurrentProcess(), pTarget, &pfn, 4, &dummy);
}

template <typename T>
bool __fastcall patch_pfns_if_match(T* pDLL)
{
	if (pDLL->pfn_func2_Start == &pDLL->func2_Start && // sanity check
	    pDLL->pfn_func1_Term  == &pDLL->func1_Term)
	{
		void* pfn;
		DWORD dummy;
		g_100030F8_pfn_SavedStart = (pfn_internal_t)pDLL->pfn_func2_Start;
		if (pDLL->pfn_func2_Start != subr_10001030_Start) {
			patch_addr(&pDLL->pfn_func2_Start, subr_10001030_Start);
		}
		g_100030F4_pfn_SavedTerm = (pfn_internal_t)pDLL->pfn_func1_Term;
		if (pDLL->pfn_func1_Term != subr_10001000_Terminate) {
			patch_addr(&pDLL->pfn_func1_Term, subr_10001000_Terminate);
		}
		return true;
	}
	return false;
}


void __fastcall DMdll_patch(void* pDLL)
{
	patch_pfns_if_match<DMdll_6_0_9782_0>((DMdll_6_0_9782_0*)pDLL);
	patch_pfns_if_match<DMdll_6_0_8447_0>((DMdll_6_0_8447_0*)pDLL);
}
#pragma optimize("", on)


BOOL __stdcall DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH) {
		h_TLLOC_old = LoadLibrary("TLLOC.old.dll");
		if (!h_TLLOC_old) {
			return FALSE;
		}
		pfn_TLLOC_old_TLFunc = (pfn_TLFunc_t)GetProcAddress(h_TLLOC_old, "TLFunc");
		pfn_TLLOC_old_OSDebug4VersionCheck = (pfn_OSD4VC_t)GetProcAddress(h_TLLOC_old, "OSDebug4VersionCheck");
		if (!pfn_TLLOC_old_TLFunc || !pfn_TLLOC_old_OSDebug4VersionCheck) {
			FreeLibrary(h_TLLOC_old); h_TLLOC_old = 0;
			return FALSE;
		}

	}
	if (fdwReason == DLL_PROCESS_DETACH) {
//		pfn_TLLOC_old_TLFunc = 0;
//		pfn_TLLOC_old_OSDebug4VersionCheck = 0;
		FreeLibrary(h_TLLOC_old); h_TLLOC_old = 0;
		FreeLibrary(h_DM_dll); h_DM_dll = 0;
	}
	return TRUE;
}


extern "C"
__declspec(dllexport)
void* __cdecl OSDebug4VersionCheck()
{
	return pfn_TLLOC_old_OSDebug4VersionCheck();
}


//__declspec(dllexport)
extern "C"
int __stdcall TLFunc(int a1, int a2, int a3_int, void* a4_pInOut)
{
	if (a1 == 14)
	{
		if (!h_DM_dll)
		{
			h_DM_dll = LoadLibraryA(*(LPCSTR *)a4_pInOut);
			if (!h_DM_dll) {
				return 4;
			}
		}
		DMdll_patch(h_DM_dll);
	}
	return pfn_TLLOC_old_TLFunc(a1, a2, a3_int, a4_pInOut);
}


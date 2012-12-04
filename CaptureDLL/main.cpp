/*----------------------------------------------------------------------------*\
|Main file for the capture DLL                                                 |
|                                                                              |
|Stewart Hall                                                                  |
|12/2/2012                                                                     |
\*----------------------------------------------------------------------------*/

#include <windows.h>

//Types for forwarded functions
typedef HGLRC (*wglCreateContext_td)(HDC hdc);
typedef BOOL (*wglMakeCurrent_td)(HDC hdc, HGLRC hglrc);
typedef BOOL (*wglCopyContext_td)(HGLRC, HGLRC, UINT);
typedef HGLRC (*wglCreateLayerContext_td)(HDC, int);
typedef BOOL (*wglDeleteContext_td)(HGLRC);
typedef HGLRC (*wglGetCurrentContext_td)(VOID);
typedef HDC (*wglGetCurrentDC_td)(VOID);
typedef PROC (*wglGetProcAddress_td)(LPCSTR);
typedef BOOL (*wglShareLists_td)(HGLRC, HGLRC);
typedef BOOL (*wglUseFontBitmapsA_td)(HDC, DWORD, DWORD, DWORD);
typedef BOOL (*wglUseFontBitmapsW_td)(HDC, DWORD, DWORD, DWORD);

//Globals to hold addresses of actual DLL functions
wglCreateContext_td real_wglCreateContext;
wglMakeCurrent_td real_wglMakeCurrent;
wglCopyContext_td real_wglCopyContext;
wglCreateLayerContext_td real_wglCreateLayerContext;
wglDeleteContext_td real_wglDeleteContext;
wglGetCurrentContext_td real_wglGetCurrentContext;
wglGetCurrentDC_td real_wglGetCurrentDC;
wglGetProcAddress_td real_wglGetProcAddress;
wglShareLists_td real_wglShareLists;
wglUseFontBitmapsA_td real_wglUseFontBitmapsA;
wglUseFontBitmapsW_td real_wglUseFontBitmapsW;

//This is a global handle to point to the REAL opengl32.dll (in system32 folder)
HINSTANCE hRealDll;

//DLL entry point
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	if(ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		//Load the real DLL
		char infoBuf[MAX_PATH];
		GetSystemDirectory(infoBuf, MAX_PATH);
		strcat_s(infoBuf, "\\opengl32.dll");

		//Load the real opengl32.dll, storing the handle in hRealDll.
		hRealDll = LoadLibrary(infoBuf);
		
		//Set real function pointers
		real_wglCreateContext = (wglCreateContext_td)GetProcAddress(hRealDll, "wglCreateContext");
		real_wglMakeCurrent = (wglMakeCurrent_td)GetProcAddress(hRealDll, "wglMakeCurrent");
		real_wglCopyContext = (wglCopyContext_td)GetProcAddress(hRealDll, "wglCopyContext");
		real_wglCreateLayerContext = (wglCreateLayerContext_td)GetProcAddress(hRealDll, "wglCreateLayerContext");
		real_wglDeleteContext = (wglDeleteContext_td)GetProcAddress(hRealDll, "wglDeleteContext");
		real_wglGetCurrentContext = (wglGetCurrentContext_td)GetProcAddress(hRealDll, "wglGetCurrentContext");
		real_wglGetCurrentDC = (wglGetCurrentDC_td)GetProcAddress(hRealDll, "wglGetCurrentDC");
		real_wglGetProcAddress = (wglGetProcAddress_td)GetProcAddress(hRealDll, "wglGetProcAddress");
		real_wglShareLists = (wglShareLists_td)GetProcAddress(hRealDll, "wglShareLists");
		real_wglUseFontBitmapsA = (wglUseFontBitmapsA_td)GetProcAddress(hRealDll, "wglUseFontBitmapsA");
		real_wglUseFontBitmapsW = (wglUseFontBitmapsW_td)GetProcAddress(hRealDll, "wglUseFontBitmapsW");
		
		//And this is just to prove we're now loaded into the process.
		MessageBox(NULL, "Proxy DLL loaded", "Status", MB_OK | MB_ICONEXCLAMATION);
	}
	else if(ul_reason_for_call == DLL_PROCESS_DETACH)
	{
	
	}

    return TRUE;
}

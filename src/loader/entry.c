#include "core/types.h"
#include "core/debug.h"
#include "hook\hook.h"
#include <Windows.h>
#include <d3d9.h>

// The path to reshade.
#define RESHADE_PATH "bin64\\ReShade64.dll"
#define DLLCHAIN_PATH "bin64\\d3d9_chain.dll"

// The path to the meter to dynamically load.
#define METER_PATH "bin64\\bgdm.dll"

// The temporary path to copy the meter to before loading, to allow for DLL replacement.
#define TEMP_PATH "bin64\\tmp.dll"

// D3D9 function typedefs.
typedef IDirect3D9* (WINAPI* Direct3DCreate9_t)(UINT);
typedef HRESULT (WINAPI* Direct3DCreate9Ex_t)(UINT, IDirect3D9Ex**);
typedef HRESULT(STDMETHODCALLTYPE* CreateDevice_t) (IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
typedef HRESULT(STDMETHODCALLTYPE* Present_t) (IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
typedef HRESULT(STDMETHODCALLTYPE* Reset_t) (IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

// Meter functions typedefs.
typedef void(*meter_create_t)(void*, void*, HWND);
typedef void(*meter_method_t)(void);
typedef bool(*meter_update_t)(void);

// Imported functions from D3D9.
static Direct3DCreate9_t _imp_Direct3DCreate9;
static Direct3DCreate9_t _imp_DllChainCreate9;
static CreateDevice_t _imp_CreateDevice;
static Present_t _imp_Present;
static Reset_t _imp_Reset;
static LPVOID g_present = NULL;
static LPVOID g_reset = NULL;

// Imported meter functions
static meter_create_t _imp_meter_create;
static meter_method_t _imp_meter_destroy;
static meter_method_t _imp_meter_reset_init;
static meter_method_t _imp_meter_reset_post;
static meter_update_t _imp_meter_update;

// Imported DLLs.
static HANDLE g_d3d9;
static HANDLE g_dll_chain;
static HANDLE g_meter;

// Meter state.
static bool g_is_loaded;
static bool g_is_loading;
static HANDLE g_load_thread;

// Game client window variables
HWND g_hwnd = NULL;
D3DVIEWPORT9 g_view = { 0 };


// Loads the meter.
static DWORD WINAPI meter_load(void* device)
{
	// Copy the meter to the temporary path.
	BOOL copy_res = CopyFile(TEXT(METER_PATH), TEXT(TEMP_PATH), FALSE);
	TCHAR* path = copy_res ? TEXT(TEMP_PATH) : TEXT(METER_PATH);

	// Try to load the meter.
	g_meter = LoadLibrary(path);
	if (g_meter == 0)
	{
		MessageBox(0, TEXT("Failed to load the meter."), TEXT("Info"), MB_OK);
		return 0;
	}

	DBGPRINT(TEXT("Meter loaded succefully from '%s'"), TEXT(METER_PATH));

	// Import functions from the meter.
	_imp_meter_create = (meter_create_t)GetProcAddress(g_meter, "meter_create");
	_imp_meter_destroy = (meter_method_t)GetProcAddress(g_meter, "meter_destroy");
	_imp_meter_reset_init = (meter_method_t)GetProcAddress(g_meter, "meter_reset_init");
	_imp_meter_reset_post = (meter_method_t)GetProcAddress(g_meter, "meter_reset_post");
	_imp_meter_update = (meter_update_t)GetProcAddress(g_meter, "meter_update");

	// Try initializing the meter.
	if (_imp_meter_create)
	{
		_imp_meter_create(device, (void*)&g_view, g_hwnd);
	}

	g_is_loaded = true;

	return 0;
}

// Triggers a reload of the meter.
static void meter_reload(void* device)
{
	// Check if the meter is already reloading.
	if (g_is_loading == true)
	{
		return;
	}

	// Release the resources from the old meter.
	if (g_meter)
	{
		if (_imp_meter_destroy)
		{
			_imp_meter_destroy();
		}

		FreeLibrary(g_meter);

		_imp_meter_create = 0;
		_imp_meter_destroy = 0;
		_imp_meter_reset_init = 0;
		_imp_meter_reset_post = 0;
		_imp_meter_update = 0;
		g_meter = 0;
	}

	// Otherwise, we reset the loading and loaded state.
	g_is_loading = true;
	g_is_loaded = false;

	// Create the loading thread.
	g_load_thread = CreateThread(0, 0, meter_load, device, 0, 0);

	// If thread creation failed, then we're done loading.
	if (g_load_thread == 0)
	{
		g_is_loading = false;
	}
}

static HRESULT STDMETHODCALLTYPE reset_hook(IDirect3DDevice9* This, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
	if (g_is_loaded && _imp_meter_reset_init)
	{
		_imp_meter_reset_init();
	}

	HRESULT res = _imp_Reset(This, pPresentationParameters);

	if (g_is_loaded && _imp_meter_reset_post)
	{
		_imp_meter_reset_post();
	}
	
	// We reload this from the meter DLL renderer_reset_post()
	//if (FAILED(This->lpVtbl->GetViewport(This, &g_view))) {}

	return res;
}

static HRESULT STDMETHODCALLTYPE present_hook(IDirect3DDevice9* This, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	// Check if the DLL is currently loading or not.
	if (g_is_loading == false)
	{
		static bool key_toggle;

		// If Ctrl + F9 is pressed, try reloading the DLL.
		bool is_shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000);
		bool is_ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
		bool is_f9 = (GetAsyncKeyState(VK_F9) & 0x8000);

		if (!key_toggle && is_ctrl && is_shift && is_f9)
		{
			meter_reload(This);
			key_toggle = true;
		}
		else if (is_f9 == false)
		{
			key_toggle = false;
		}

		// If the DLL is loaded, then run the update.
		if (g_is_loaded && _imp_meter_update && _imp_meter_update())
		{
			meter_reload(This);
		}
	}
	else
	{
		// Check if the meter has finished reloading, but don't block.
		DWORD res = WaitForSingleObject(g_load_thread, 0);
		if (res != WAIT_TIMEOUT)
		{
			g_is_loading = false;
		}
	}

	return _imp_Present(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

static HRESULT STDMETHODCALLTYPE create_device_hook(
	IDirect3D9* This,
	UINT Adapter,
	D3DDEVTYPE DeviceType,
	HWND hFocusWindow,
	DWORD BehaviorFlags,
	D3DPRESENT_PARAMETERS* pPresentationParameters,
	IDirect3DDevice9** ppReturnedDeviceInterface)
{

	// Create the device.
	IDirect3DDevice9* dev;
	HRESULT res = _imp_CreateDevice(This, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, &dev);
	*ppReturnedDeviceInterface = dev;

	if (FAILED(res))
	{
		MessageBox(0, TEXT("Failed to create the IDirect3DDevice9 interface."), TEXT("Info"), MB_OK);
		return res;
	}

	// Hook reset and present to allow for our callbacks.
	if (MH_CreateHook(g_reset ? g_reset : (LPVOID)dev->lpVtbl->Reset, (LPVOID)&reset_hook, (LPVOID*)&_imp_Reset) != MH_OK)
	{
		MessageBox(0, TEXT("Failed to create the Reset hook."), TEXT("Error"), MB_OK);
		return res;
	}

	if (MH_CreateHook(g_present ? g_present : (LPVOID)dev->lpVtbl->Present, (LPVOID)&present_hook, (LPVOID*)&_imp_Present) != MH_OK)
	{
		MessageBox(0, TEXT("Failed to create the Present hook."), TEXT("Error"), MB_OK);
		return res;
	}

	if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
	{
		MessageBox(0, TEXT("Failed to enable Reset and Present hooks."), TEXT("Error"), MB_OK);
		return res;
	}

	
	// get client window variables
	g_hwnd = GetActiveWindow();
	if (FAILED(dev->lpVtbl->GetViewport(dev, &g_view))) {
	}

	// Try loading the meter.
	meter_reload(dev);

	return res;
}

bool d3d_get_VTfncs(LPDIRECT3D9 pD3D, LPVOID *pPresent, LPVOID *pReset)
{
	// create a dummy d3d device
	//LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D) return false;

	WNDCLASSEXA wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_CLASSDC;
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = GetModuleHandleA(NULL);
	wc.lpszClassName = "DXTMP";
	RegisterClassExA(&wc);

	HWND hWnd = CreateWindowA("DXTMP", 0, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, GetDesktopWindow(), 0, wc.hInstance, 0);
	if (!hWnd) return false;
	
	D3DPRESENT_PARAMETERS d3dPar = { 0 };
	d3dPar.Windowed = TRUE;
	d3dPar.SwapEffect = D3DSWAPEFFECT_DISCARD;
	LPDIRECT3DDEVICE9 pDev = NULL;
	_imp_CreateDevice(pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dPar, &pDev);
	if (!pDev) goto cleanup;

	// fetch the location of reset/present in the
	// d3d9 module via vtable of dummy device
	if (pPresent)
		*pPresent = (LPVOID)pDev->lpVtbl->Present;
	if (pReset)
		*pReset = (LPVOID)pDev->lpVtbl->Reset;

	DBGPRINT(TEXT("IDirect3DDevice9::Present=%016I64X  IDirect3DDevice9::Reset=%016I64X"),
		(LPVOID)pDev->lpVtbl->Present, (LPVOID)pDev->lpVtbl->Reset);

cleanup:
	// clean up dummy windows/device
	if (pDev) {
		pDev->lpVtbl->Release(pDev);
	}

	if (hWnd) {
		DestroyWindow(hWnd);
		UnregisterClassA("DXTMP", GetModuleHandleA(NULL));
	}

	return true;
}

static HANDLE LoadD3D9()
{
	if (g_d3d9 == 0)
	{

		TCHAR path[MAX_PATH];
		GetSystemDirectory(path, ARRAYSIZE(path));

		u32 i = 0;
		while (path[i])
		{
			++i;
		}

		TCHAR* dll = TEXT("\\d3d9.dll");

		u32 j = 0;
		while (dll[j])
		{
			path[i++] = dll[j++];
		}

		path[i] = 0;

		g_d3d9 = LoadLibrary(path);
	}

	return g_d3d9;
}

IDirect3D9* WINAPI d3d_create_hook(UINT SDKVersion)
{
	// Import the D3D9 device creation function.
	if (_imp_Direct3DCreate9 == 0)
	{
		g_d3d9 = LoadD3D9();
		if (g_d3d9 == 0)
		{
			MessageBox(0, TEXT("System d3d9.dll is missing or invalid."), TEXT("Error"), MB_OK);
			return 0;
		}

		_imp_Direct3DCreate9 = (Direct3DCreate9_t)GetProcAddress(g_d3d9, "Direct3DCreate9");
		if (_imp_Direct3DCreate9 == 0)
		{
			MessageBox(0, TEXT("Importing Direct3DCreate9 failed."), TEXT("Error"), MB_OK);
			return 0;
		}
	}

	// Load Reshade if available.
	if (g_dll_chain == 0)
	{
		g_dll_chain = LoadLibrary(TEXT(RESHADE_PATH));
		
		// Try with diff name?
		if (g_dll_chain == 0)
			g_dll_chain = LoadLibrary(TEXT(DLLCHAIN_PATH));

		if (g_dll_chain)
		{
			_imp_DllChainCreate9 = (Direct3DCreate9_t)GetProcAddress(g_dll_chain, "Direct3DCreate9");
		}
	}

	// Try creating the D3D9 interface.
	IDirect3D9* d3d = _imp_Direct3DCreate9(SDKVersion);
	if (d3d == 0)
	{
		MessageBox(0, TEXT("IDirect3D9 interface creation failed."), TEXT("Error"), MB_OK);
		return 0;
	}

	// Try hooking the device creation routine.
	MH_STATUS res = MH_CreateHook((LPVOID)d3d->lpVtbl->CreateDevice, (LPVOID)&create_device_hook, (LPVOID*)&_imp_CreateDevice);
	if (res == MH_OK)
	{
		if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK)
		{
			MessageBox(0, TEXT("Enabling the CreateDevice hook failed."), TEXT("Error"), MB_OK);
		}
	}
	else if (res != MH_ERROR_ALREADY_CREATED)
	{
		MessageBox(0, TEXT("CreateDevice hook failed."), TEXT("Error"), MB_OK);
	}

	// Use Reshade if it's loaded.
	if (_imp_DllChainCreate9) {
		// Save the original Present/Reset VTs
		d3d_get_VTfncs(d3d, &g_present, &g_reset);
		d3d->lpVtbl->Release(d3d);
		d3d = _imp_DllChainCreate9(SDKVersion);
	}

	return d3d;
}

BOOL WINAPI DllMain(HANDLE instance, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
		case DLL_PROCESS_ATTACH:
		{
			// Initialize hooking.
			if (MH_Initialize() != MH_OK)
			{
				MessageBox(0, TEXT("hook lib failed to initialize."), TEXT("Error"), MB_OK);
				return FALSE;
			}
		} break;

		case DLL_PROCESS_DETACH:
		{
			// Release our hooks.
			MH_Uninitialize();

			// Free dlls we loaded.
			if (g_meter)
			{
				FreeLibrary(g_meter);
			}

			if (g_dll_chain)
			{
				FreeLibrary(g_dll_chain);
			}

			if (g_d3d9)
			{
				FreeLibrary(g_d3d9);
			}
		} break;
	}

	return TRUE;
}



HRESULT WINAPI Direct3DCreate9Ex(
	_In_  UINT         SDKVersion,
	_Out_ IDirect3D9Ex **ppD3D
)
{
	DBGPRINT(TEXT("SDKVersion:%d"), SDKVersion);

	static Direct3DCreate9Ex_t _imp_Direct3DCreate9Ex = NULL;
	HANDLE hD3D = LoadD3D9();

	if (hD3D && !_imp_Direct3DCreate9Ex) {
		_imp_Direct3DCreate9Ex = (Direct3DCreate9Ex_t)GetProcAddress(hD3D, "Direct3DCreate9Ex");
	}

	if (_imp_Direct3DCreate9Ex)
		return _imp_Direct3DCreate9Ex(SDKVersion, ppD3D);

	return D3DERR_NOTAVAILABLE;
}


int WINAPI D3DPERF_BeginEvent(D3DCOLOR col, LPCWSTR wszName)
{
	DBGPRINT(TEXT("color:%d, wszName:%s"), col, wszName);

	typedef int (WINAPI* BeginEvent_t)(D3DCOLOR, LPCWSTR);
	static BeginEvent_t _imp_BeginEvent = NULL;
	HANDLE hD3D = LoadD3D9();

	if (hD3D && !_imp_BeginEvent)
		_imp_BeginEvent = (BeginEvent_t)GetProcAddress(hD3D, "D3DPERF_BeginEvent");

	if (_imp_BeginEvent)
		return _imp_BeginEvent(col, wszName);

	return D3DERR_NOTAVAILABLE;
}

int WINAPI D3DPERF_EndEvent(void)
{
	DBGPRINT(TEXT("EndEvent"));

	typedef int (WINAPI* EndEvent_t)(void);
	static EndEvent_t _imp_EndEvent = NULL;
	HANDLE hD3D = LoadD3D9();

	if (hD3D && !_imp_EndEvent)
		_imp_EndEvent = (EndEvent_t)GetProcAddress(hD3D, "D3DPERF_EndEvent");

	if (_imp_EndEvent)
		return _imp_EndEvent();

	return D3DERR_NOTAVAILABLE;
}

void WINAPI D3DPERF_SetMarker(D3DCOLOR col, LPCWSTR wszName)
{
	DBGPRINT(TEXT("color:%d, wszName:%s"), col, wszName);

	typedef VOID(WINAPI* Direct3DSet_t)(D3DCOLOR, LPCWSTR);
	static Direct3DSet_t _imp_SetMarker = NULL;
	HANDLE hD3D = LoadD3D9();

	if (hD3D && !_imp_SetMarker)
		_imp_SetMarker = (Direct3DSet_t)GetProcAddress(hD3D, "D3DPERF_SetMarker");

	if (_imp_SetMarker)
		_imp_SetMarker(col, wszName);
}

void WINAPI D3DPERF_SetRegion(D3DCOLOR col, LPCWSTR wszName)
{
	DBGPRINT(TEXT("color:%d, wszName:%s"), col, wszName);

	typedef VOID(WINAPI* Direct3DSet_t)(D3DCOLOR, LPCWSTR);
	static Direct3DSet_t _imp_SetRegion = NULL;
	HANDLE hD3D = LoadD3D9();

	if (hD3D && !_imp_SetRegion)
		_imp_SetRegion = (Direct3DSet_t)GetProcAddress(hD3D, "D3DPERF_SetRegion");

	if (_imp_SetRegion)
		_imp_SetRegion(col, wszName);
}

BOOL WINAPI D3DPERF_QueryRepeatFrame(void)
{
	DBGPRINT(TEXT("QueryRepeatFrame"));

	typedef BOOL (WINAPI* QueryRepeatFrame_t)(void);
	static QueryRepeatFrame_t _imp_QueryRepeatFrame = NULL;
	HANDLE hD3D = LoadD3D9();

	if (hD3D && !_imp_QueryRepeatFrame)
		_imp_QueryRepeatFrame = (QueryRepeatFrame_t)GetProcAddress(hD3D, "D3DPERF_QueryRepeatFrame");

	if (_imp_QueryRepeatFrame)
		return _imp_QueryRepeatFrame();

	return FALSE;
}

void WINAPI D3DPERF_SetOptions(DWORD dwOptions)
{
	DBGPRINT(TEXT("SetOptions:%d"), dwOptions);

	typedef void (WINAPI* SetOptions_t)(DWORD);
	static SetOptions_t _imp_SetOptions = NULL;
	HANDLE hD3D = LoadD3D9();

	if (hD3D && !_imp_SetOptions)
		_imp_SetOptions = (SetOptions_t)GetProcAddress(hD3D, "D3DPERF_SetOptions");

	if (_imp_SetOptions)
		_imp_SetOptions(dwOptions);
}

DWORD WINAPI D3DPERF_GetStatus(void)
{
	DBGPRINT(TEXT("GetStatus"));

	typedef DWORD (WINAPI* GetStatus_t)(void);
	static GetStatus_t _imp_GetStatus = NULL;
	HANDLE hD3D = LoadD3D9();

	if (hD3D && !_imp_GetStatus)
		_imp_GetStatus = (GetStatus_t)GetProcAddress(hD3D, "D3DPERF_GetStatus");

	if (_imp_GetStatus)
		return _imp_GetStatus();

	return 0;
}

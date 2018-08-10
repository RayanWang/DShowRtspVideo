#include "stdafx.h"

// Initialize the GUIDs
#include <InitGuid.h>

#include "CudaDecodeFilter.h"
#include "DeltaCudaSettingsProp.h"
#include "otheruuids.h"

// --- COM factory table and registration code --------------

// Workaround: graphedit crashes when a filter exposes more than 115 input MediaTypes!
const AMOVIESETUP_PIN sudpPinsCUDAVideoDec[] = 
{
	{L"Input", FALSE, FALSE, FALSE, FALSE, &CLSID_NULL, NULL, CDeltaCudaH264Decoder::sudPinTypesInCount > 115 ? 115 : CDeltaCudaH264Decoder::sudPinTypesInCount,  CDeltaCudaH264Decoder::sudPinTypesIn},
	{L"Output", FALSE, TRUE, FALSE, FALSE, &CLSID_NULL, NULL, CDeltaCudaH264Decoder::sudPinTypesOutCount, CDeltaCudaH264Decoder::sudPinTypesOut}
};

const AMOVIESETUP_FILTER sudFilterReg =
{
	&__uuidof(CDeltaCudaH264Decoder),   // filter clsid
	L"Delta Cuda H264 Decoder",			// filter name
	MERIT_PREFERRED + 3,				// merit
	countof(sudpPinsCUDAVideoDec), 
	sudpPinsCUDAVideoDec
};

// --- COM factory table and registration code --------------

// DirectShow base class COM factory requires this table, 
// declaring all the COM objects in this DLL
CFactoryTemplate g_Templates[] = 
{
	// one entry for each CoCreate-able object
	{
		sudFilterReg.strName,
		sudFilterReg.clsID,
		CDeltaCudaH264Decoder::CreateInstance,
		NULL,
		&sudFilterReg
	},
	// This entry is for the property page.
	{
		L"Delta Cuda Properties",
		&CLSID_DeltaCudaSettingsProp,
		CDeltaCudaSettingsProp::CreateInstance,
		NULL, NULL
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// self-registration entrypoint
STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(true);
}

STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(false);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
	return DllEntryPoint(reinterpret_cast<HINSTANCE>(hDllHandle), dwReason, lpReserved);
}

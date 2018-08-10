// IpCamGraph.cpp : DLL 匯出的實作。


#include "stdafx.h"
#include "resource.h"
#include "IpCamGraph_i.h"
#include "dllmain.h"


using namespace ATL;

// 用來判斷 DLL 是否可被 OLE 卸載。
STDAPI DllCanUnloadNow(void)
{
			AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (AfxDllCanUnloadNow()==S_OK && _AtlModule.GetLockCount()==0) ? S_OK : S_FALSE;
	}

// 傳回 Class Factory 以建立所要求型別的物件。
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv)
{
		return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

// DllRegisterServer - 將項目加入到系統登錄。
STDAPI DllRegisterServer(void)
{
	// 登錄物件、Typelib 和所有 Typelib 中的介面
	HRESULT hr = _AtlModule.DllRegisterServer();
		return hr;
}

// DllUnregisterServer - 從系統登錄移除項目。
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer();
		return hr;
}

// DllInstall - 針對個別使用者及個別電腦，在系統登錄中加入/移除項目。
STDAPI DllInstall(BOOL bInstall, _In_opt_  LPCWSTR pszCmdLine)
{
	HRESULT hr = E_FAIL;
	static const wchar_t szUserSwitch[] = L"user";

	if (pszCmdLine != NULL)
	{
		if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0)
		{
			ATL::AtlSetPerUserRegistration(true);
		}
	}

	if (bInstall)
	{	
		hr = DllRegisterServer();
		if (FAILED(hr))
		{
			DllUnregisterServer();
		}
	}
	else
	{
		hr = DllUnregisterServer();
	}

	return hr;
}



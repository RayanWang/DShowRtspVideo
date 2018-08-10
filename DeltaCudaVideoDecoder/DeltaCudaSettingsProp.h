#pragma once

#include "DeltaCudaSettings.h"

// {E39EBB8D-602C-49C0-8221-BC825A5DD978}
DEFINE_GUID(CLSID_DeltaCudaSettingsProp, 
0xe39ebb8d, 0x602c, 0x49c0, 0x82, 0x21, 0xbc, 0x82, 0x5a, 0x5d, 0xd9, 0x78);

class CDeltaCudaSettingsProp : public CBasePropertyPage
{
public:
	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);

	~CDeltaCudaSettingsProp();

	HRESULT OnActivate();
	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnDisconnect();
	HRESULT OnApplyChanges();
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	CDeltaCudaSettingsProp(IUnknown *pUnk);

	HRESULT LoadData();

	void SetDirty()
	{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
	}

	DWORD GetDeintMode();

	TOOLINFO addHint(int id, const LPWSTR text);
	HWND createHintWindow(HWND parent, int timePop = 1700, int timeInit = 70, int timeReshow = 7);

private:
	HWND m_hHint;

	IDeltaCudaSettings *m_pSettings;

	DWORD m_dwDeinterlace;
	BOOL m_bFrameDoubling;
	DWORD m_dwFieldOrder;
	bool m_bFormats[cudaVideoCodec_NumCodecs];
	BOOL m_bStreamAR;
	BOOL m_bDXVA;
	DWORD m_dwOutputFormat;
};

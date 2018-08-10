#include "stdafx.h"
#include "DeltaCudaSettingsProp.h"
#include "GlobalAPI.h"

#include "resource.h"
#include "version.h"

// static constructor
CUnknown* WINAPI CDeltaCudaSettingsProp::CreateInstance(LPUNKNOWN pUnk, HRESULT* phr)
{
	CDeltaCudaSettingsProp *propPage = new CDeltaCudaSettingsProp(pUnk);
	if (!propPage) 
	{
		*phr = E_OUTOFMEMORY;
	}

	return propPage;
}

CDeltaCudaSettingsProp::CDeltaCudaSettingsProp(IUnknown *pUnk)
  : CBasePropertyPage(NAME("DeltaCudaSettingsProp")
  , pUnk
  , IDD_PROPPAGE_DELTACUDA_SETTINGS
  , IDS_SETTINGS)
  , m_hHint(0)
  , m_pSettings(NULL)
  , m_dwDeinterlace(0)
  , m_bDXVA(0)
  , m_bFrameDoubling(0)
  , m_dwFieldOrder(0)
{
}


CDeltaCudaSettingsProp::~CDeltaCudaSettingsProp()
{
}

HWND CDeltaCudaSettingsProp::createHintWindow(HWND parent, int timePop, int timeInit, int timeReshow)
{
	HWND hhint = CreateWindowEx(WS_EX_TOPMOST, 
								TOOLTIPS_CLASS, 
								NULL,
								WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								parent, NULL, NULL, NULL);
	SetWindowPos(hhint, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	SendMessage(hhint, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELONG(timePop, 0));
	SendMessage(hhint, TTM_SETDELAYTIME, TTDT_INITIAL, MAKELONG(timeInit, 0));
	SendMessage(hhint, TTM_SETDELAYTIME, TTDT_RESHOW, MAKELONG(timeReshow, 0));
	SendMessage(hhint, TTM_SETMAXTIPWIDTH, 0, 470);

	return hhint;
}

TOOLINFO CDeltaCudaSettingsProp::addHint(int id, const LPWSTR text)
{
	if (!m_hHint) 
		m_hHint = createHintWindow(m_Dlg, 15000);

	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS|TTF_IDISHWND;
	ti.hwnd = m_Dlg;
	ti.uId = (LPARAM)GetDlgItem(m_Dlg, id);
	ti.lpszText = text;
	SendMessage(m_hHint, TTM_ADDTOOL, 0, (LPARAM)&ti);

	return ti;
}

HRESULT CDeltaCudaSettingsProp::OnConnect(IUnknown *pUnk)
{
	if (pUnk == NULL) 
	{
		return E_POINTER;
	}
	ASSERT(m_pSettings == NULL);

	return pUnk->QueryInterface(&m_pSettings);
}

HRESULT CDeltaCudaSettingsProp::OnDisconnect()
{
	SafeRelease(&m_pSettings);

	return S_OK;
}


HRESULT CDeltaCudaSettingsProp::OnApplyChanges()
{
	ASSERT(m_pSettings != NULL);
	HRESULT hr = S_OK;

	bool bFormats[cudaVideoCodec_NumCodecs];
	for (int i = 0; i < cudaVideoCodec_NumCodecs; ++i) 
	{
		bFormats[i] = true;
	}

	bFormats[cudaVideoCodec_H264] = SendDlgItemMessage(m_Dlg, IDC_FORMAT_H264, BM_GETCHECK, 0, 0) ? true : false;
	bFormats[cudaVideoCodec_VC1] = SendDlgItemMessage(m_Dlg, IDC_FORMAT_VC1, BM_GETCHECK, 0, 0) ? true : false;
	bFormats[cudaVideoCodec_MPEG2] = SendDlgItemMessage(m_Dlg, IDC_FORMAT_MPEG2, BM_GETCHECK, 0, 0) ? true : false;
	bFormats[cudaVideoCodec_MPEG4] = SendDlgItemMessage(m_Dlg, IDC_FORMAT_MPEG4, BM_GETCHECK, 0, 0) ? true : false;
	m_pSettings->SetFormatConfiguration(bFormats);

	BOOL bStreamAR = (BOOL)SendDlgItemMessage(m_Dlg, IDC_STREAMAR, BM_GETCHECK, 0, 0);
	m_pSettings->SetStreamAR(bStreamAR);

	BOOL bDXVA = (BOOL)SendDlgItemMessage(m_Dlg, IDC_DXVA, BM_GETCHECK, 0, 0);
	m_pSettings->SetDXVA(bDXVA);
  
	m_pSettings->SetDeinterlaceMode(GetDeintMode());

	BOOL bFrameDoubling = (BOOL)SendDlgItemMessage(m_Dlg, IDC_DEINT_FPS_FULL, BM_GETCHECK, 0, 0);
	m_pSettings->SetFrameDoubling(bFrameDoubling);

	/*DWORD dwVal = (DWORD)SendDlgItemMessage(m_Dlg, IDC_DEINT_FIELDORDER, CB_GETCURSEL, 0, 0);
	m_pSettings->SetFieldOrder(dwVal);*/

	/*dwVal = (DWORD)SendDlgItemMessage(m_Dlg, IDC_OUTPUT_FORMAT, CB_GETCURSEL, 0, 0);
	m_pSettings->SetOutputFormat(dwVal);*/

	LoadData();

	return hr;
} 

HRESULT CDeltaCudaSettingsProp::OnActivate()
{
	HRESULT hr = S_OK;
	//INITCOMMONCONTROLSEX icc;
	//icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	//icc.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
	//if (InitCommonControlsEx(&icc) == FALSE) 
	//{
	//	return E_FAIL;
	//}
	ASSERT(m_pSettings != NULL);

	hr = LoadData();
	if (SUCCEEDED(hr)) 
	{
		SendDlgItemMessage(m_Dlg, IDC_DEINT_WEAVE, BM_SETCHECK, 0, 0);
		SendDlgItemMessage(m_Dlg, IDC_DEINT_BOB, BM_SETCHECK, 0, 0);
		SendDlgItemMessage(m_Dlg, IDC_DEINT_ADAPTIVE, BM_SETCHECK, 0, 0);
		switch(m_dwDeinterlace) 
		{
		case 0:
			SendDlgItemMessage(m_Dlg, IDC_DEINT_WEAVE, BM_SETCHECK, 1, 0);
			break;
		case 1:
			SendDlgItemMessage(m_Dlg, IDC_DEINT_BOB, BM_SETCHECK, 1, 0);
			break;
		case 2:
			SendDlgItemMessage(m_Dlg, IDC_DEINT_ADAPTIVE, BM_SETCHECK, 1, 0);
			break;
		}

		SendDlgItemMessage(m_Dlg, IDC_DEINT_FPS_HALF, BM_SETCHECK, !m_bFrameDoubling, 0);
		SendDlgItemMessage(m_Dlg, IDC_DEINT_FPS_FULL, BM_SETCHECK, m_bFrameDoubling, 0);
		addHint(IDC_DEINT_FPS_HALF, L"Deinterlace in \"Film\" Mode.\nFor every pair of interlaced fields, one frame will be created, resulting in 25/30 fps.");
		addHint(IDC_DEINT_FPS_FULL, L"Deinterlace in \"Video\" Mode. (Recommended)\nFor every interlaced field, one frame will be created, resulting in 50/60 fps.");

		//WCHAR stringBuffer[100];

		// Init the fieldorder Combo Box
		/*SendDlgItemMessage(m_Dlg, IDC_DEINT_FIELDORDER, CB_RESETCONTENT, 0, 0);
		WideStringFromResource(stringBuffer, IDS_FIELDORDER_AUTO);
		SendDlgItemMessage(m_Dlg, IDC_DEINT_FIELDORDER, CB_ADDSTRING, 0, (LPARAM)stringBuffer);
		WideStringFromResource(stringBuffer, IDS_FIELDORDER_TOP);
		SendDlgItemMessage(m_Dlg, IDC_DEINT_FIELDORDER, CB_ADDSTRING, 0, (LPARAM)stringBuffer);
		WideStringFromResource(stringBuffer, IDS_FIELDORDER_BOTTOM);
		SendDlgItemMessage(m_Dlg, IDC_DEINT_FIELDORDER, CB_ADDSTRING, 0, (LPARAM)stringBuffer);

		SendDlgItemMessage(m_Dlg, IDC_DEINT_FIELDORDER, CB_SETCURSEL, m_dwFieldOrder, 0);*/

		// Init the output combo box
		/*SendDlgItemMessage(m_Dlg, IDC_OUTPUT_FORMAT, CB_RESETCONTENT, 0, 0);
		WideStringFromResource(stringBuffer, IDS_OUTPUT_AUTO);
		SendDlgItemMessage(m_Dlg, IDC_OUTPUT_FORMAT, CB_ADDSTRING, 0, (LPARAM)stringBuffer);
		WideStringFromResource(stringBuffer, IDS_OUTPUT_NV12);
		SendDlgItemMessage(m_Dlg, IDC_OUTPUT_FORMAT, CB_ADDSTRING, 0, (LPARAM)stringBuffer);
		WideStringFromResource(stringBuffer, IDS_OUTPUT_YV12);
		SendDlgItemMessage(m_Dlg, IDC_OUTPUT_FORMAT, CB_ADDSTRING, 0, (LPARAM)stringBuffer);

		SendDlgItemMessage(m_Dlg, IDC_OUTPUT_FORMAT, CB_SETCURSEL, m_dwOutputFormat, 0);*/

		SendDlgItemMessage(m_Dlg, IDC_FORMAT_H264, BM_SETCHECK, m_bFormats[cudaVideoCodec_H264], 0);
		SendDlgItemMessage(m_Dlg, IDC_FORMAT_VC1, BM_SETCHECK, m_bFormats[cudaVideoCodec_VC1], 0);
		SendDlgItemMessage(m_Dlg, IDC_FORMAT_MPEG2, BM_SETCHECK, m_bFormats[cudaVideoCodec_MPEG2], 0);
		SendDlgItemMessage(m_Dlg, IDC_FORMAT_MPEG4, BM_SETCHECK, m_bFormats[cudaVideoCodec_MPEG4], 0);

		SendDlgItemMessage(m_Dlg, IDC_STREAMAR, BM_SETCHECK, m_bStreamAR, 0);
		SendDlgItemMessage(m_Dlg, IDC_DXVA, BM_SETCHECK, m_bDXVA, 0);

		addHint(IDC_DXVA, L"Instruct the decoder to access the hardware through the DXVA API.\nThis will cost performance, it is however required for the best deinterlacing quality.");
	}

	return hr;
}

HRESULT CDeltaCudaSettingsProp::LoadData()
{
	HRESULT hr = S_OK;

	m_dwDeinterlace = m_pSettings->GetDeinterlaceMode();
	m_bFrameDoubling = m_pSettings->GetFrameDoubling();
	m_dwFieldOrder = m_pSettings->GetFieldOrder();
	m_pSettings->GetFormatConfiguration(m_bFormats);
	m_bStreamAR = m_pSettings->GetStreamAR();
	m_bDXVA = m_pSettings->GetDXVA();
	m_dwOutputFormat = m_pSettings->GetOutputFormat();

	return hr;
}

DWORD CDeltaCudaSettingsProp::GetDeintMode()
{
	DWORD dwDeintMode = 0;

	if (SendDlgItemMessage(m_Dlg, IDC_DEINT_WEAVE, BM_GETCHECK, 0, 0)) 
	{
		dwDeintMode = 0;
	} 
	else if (SendDlgItemMessage(m_Dlg, IDC_DEINT_BOB, BM_GETCHECK, 0, 0)) 
	{
		dwDeintMode = 1;
	}
	else if (SendDlgItemMessage(m_Dlg, IDC_DEINT_ADAPTIVE, BM_GETCHECK, 0, 0)) 
	{
		dwDeintMode = 2;
	}

	return dwDeintMode;
}

INT_PTR CDeltaCudaSettingsProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if ((LOWORD(wParam) == IDC_DEINT_WEAVE || LOWORD(wParam) == IDC_DEINT_BOB || LOWORD(wParam) == IDC_DEINT_ADAPTIVE) && HIWORD(wParam) == BN_CLICKED) 
		{
			if (GetDeintMode() != m_dwDeinterlace) 
			{
				SetDirty();
			}
		} 
		else if ((LOWORD(wParam) == IDC_DEINT_FPS_FULL || LOWORD(wParam) == IDC_DEINT_FPS_HALF) && HIWORD(wParam) == BN_CLICKED) 
		{
			BOOL bFlag = (BOOL)SendDlgItemMessage(m_Dlg, IDC_DEINT_FPS_FULL, BM_GETCHECK, 0, 0);
			if (bFlag != m_bFrameDoubling) 
			{
				SetDirty();
			}
		} 
		/*else if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_DEINT_FIELDORDER) 
		{
			DWORD dwVal = (DWORD)SendDlgItemMessage(m_Dlg, LOWORD(wParam), CB_GETCURSEL, 0, 0);
			if (dwVal != m_dwFieldOrder) 
			{
				SetDirty();
			}
		} */
		/*else if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_OUTPUT_FORMAT) 
		{
			DWORD dwVal = (DWORD)SendDlgItemMessage(m_Dlg, LOWORD(wParam), CB_GETCURSEL, 0, 0);
			if (dwVal != m_dwOutputFormat) 
			{
				SetDirty();
			}
		} */
		else if (LOWORD(wParam) == IDC_FORMAT_H264 && HIWORD(wParam) == BN_CLICKED) 
		{
			bool bFlag = SendDlgItemMessage(m_Dlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? true : false;
			if (bFlag != m_bFormats[cudaVideoCodec_H264]) 
			{
				SetDirty();
			}
		}  
		else if (LOWORD(wParam) == IDC_FORMAT_VC1 && HIWORD(wParam) == BN_CLICKED) 
		{
			bool bFlag = SendDlgItemMessage(m_Dlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? true : false;
			if (bFlag != m_bFormats[cudaVideoCodec_VC1]) 
			{
				SetDirty();
			}
		}  
		else if (LOWORD(wParam) == IDC_FORMAT_MPEG2 && HIWORD(wParam) == BN_CLICKED) 
		{
			bool bFlag = SendDlgItemMessage(m_Dlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? true : false;
			if (bFlag != m_bFormats[cudaVideoCodec_MPEG2]) 
			{
				SetDirty();
			}
		}  
		else if (LOWORD(wParam) == IDC_FORMAT_MPEG4 && HIWORD(wParam) == BN_CLICKED) 
		{
			bool bFlag = SendDlgItemMessage(m_Dlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? true : false;
			if (bFlag != m_bFormats[cudaVideoCodec_MPEG4]) 
			{
				SetDirty();
			}
		} 
		else if (LOWORD(wParam) == IDC_STREAMAR && HIWORD(wParam) == BN_CLICKED) 
		{
			BOOL bFlag = (BOOL)SendDlgItemMessage(m_Dlg, LOWORD(wParam), BM_GETCHECK, 0, 0);
			if (bFlag != m_bStreamAR) 
			{
				SetDirty();
			}
		} 
		else if (LOWORD(wParam) == IDC_DXVA && HIWORD(wParam) == BN_CLICKED) 
		{
			BOOL bFlag = (BOOL)SendDlgItemMessage(m_Dlg, LOWORD(wParam), BM_GETCHECK, 0, 0);
			if (bFlag != m_bDXVA) 
			{
				SetDirty();
			}
		}
		break;
	}

	// Let the parent class handle the message.
	return __super::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

#include "RtspStreamProp.h"
#include "resource.h"
#include "iRtspStream.h"


CRtspStreamProp::CRtspStreamProp(IUnknown *pUnk)
: CBasePropertyPage(NAME("RtspStreamProp"), pUnk, IDD_PROPPAGE, IDS_PROPPAGE_TITLE)
{
	wcscpy_s(m_wszWidth, L"1280");
	wcscpy_s(m_wszHeight, L"720");
	::ZeroMemory(&m_wszNewUrl, sizeof(m_wszNewUrl));
}


CRtspStreamProp::~CRtspStreamProp(void)
{
}


HRESULT CRtspStreamProp::OnConnect(IUnknown *pUnk)
{
    if (pUnk == NULL)
    {
        return E_POINTER;
    }

    ASSERT(m_spRtspStreaming == NULL);

    return pUnk->QueryInterface(IID_IRtspStreaming, reinterpret_cast<void**>(&m_spRtspStreaming));
}


HRESULT CRtspStreamProp::OnActivate(void)
{
    ASSERT(m_spRtspStreaming != NULL);

	HRESULT hr = m_spRtspStreaming->GetStreamURL(m_wszUrl);

	if (SUCCEEDED(hr))
		SetDlgItemText(m_Dlg, IDC_EDIT_URL, (LPCTSTR)m_wszUrl);

	SetDlgItemText(m_Dlg, IDC_EDIT_WIDTH, m_wszWidth);
	SetDlgItemText(m_Dlg, IDC_EDIT_HEIGHT, m_wszHeight);

	return hr;
}

BOOL CRtspStreamProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
		{
			SetDirty();
		}
        break;
	case WM_INITDIALOG:
		break;
    } // Switch.
    
    // Let the parent class handle the message.
    return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CRtspStreamProp::OnApplyChanges(void)
{
	GetDlgItemText(m_Dlg, IDC_EDIT_URL, (LPTSTR)m_wszNewUrl, 1024);
	GetDlgItemText(m_Dlg, IDC_EDIT_WIDTH, (LPTSTR)m_wszWidth, MAX_PATH);
	GetDlgItemText(m_Dlg, IDC_EDIT_HEIGHT, (LPTSTR)m_wszHeight, MAX_PATH);

    return S_OK;
}

HRESULT CRtspStreamProp::OnDisconnect(void)
{
    if (m_spRtspStreaming)
    {
		wcscpy_s(m_wszUrl, m_wszNewUrl);

		m_spRtspStreaming->SetStreamURL(m_wszUrl);

		int nWidth = _wtoi(m_wszWidth);
		int nHeight = _wtoi(m_wszHeight);
		m_spRtspStreaming->SetResolution(nWidth, nHeight);

		m_spRtspStreaming.Release();
    }

    return S_OK;
}

CUnknown * WINAPI CRtspStreamProp::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
{
    CRtspStreamProp *pNewObject = new CRtspStreamProp(pUnk);
    if (pNewObject == NULL) 
    {
        *pHr = E_OUTOFMEMORY;
    }

    return pNewObject;
}

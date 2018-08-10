#pragma once

#include "streams.h"
#include <atlbase.h>

interface IRtspStreaming;

class CRtspStreamProp : public CBasePropertyPage
{
public:
	CRtspStreamProp(IUnknown *pUnk);
	virtual ~CRtspStreamProp(void);

	// CBasePropertyPage
	virtual HRESULT OnConnect(IUnknown *pUnknown);
	virtual HRESULT OnActivate();
	virtual INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	virtual HRESULT OnApplyChanges();
	virtual HRESULT OnDisconnect();

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

private:
    void SetDirty()
    {
        m_bDirty = TRUE;
        if (m_pPageSite)
        {
            m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }
    }

private:
    CComPtr<IRtspStreaming>	m_spRtspStreaming;
	WCHAR					m_wszUrl[1024];
	WCHAR					m_wszNewUrl[1024];
	WCHAR					m_wszWidth[MAX_PATH];
	WCHAR					m_wszHeight[MAX_PATH];
};


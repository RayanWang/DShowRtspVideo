// GraphAPI.cpp : CGraphAPI 的實作

#include "stdafx.h"
#include "GraphAPI.h"
#include "iFilterAPI.h"
#include "iVideoProcesser.h"

#include "DatasteadRTSPSourceFilter.h"

// { 36a5f770-fe4c-11ce-a8ed-00aa002feab5 }
DEFINE_GUID(CLSID_Dump,
			0x36a5f770, 0xfe4c, 0x11ce, 0xa8, 0xed, 0x00, 0xaa, 0x00, 0x2f, 0xea, 0xb5);

// { F8388A40-D5BB-11D0-BE5A-0080C706568E }
DEFINE_GUID(CLSID_Tee,
			0xF8388A40, 0xD5BB, 0x11D0, 0xBE, 0x5A, 0x00, 0x80, 0xC7, 0x06, 0x56, 0x8E);

// {6886D765-03DB-4E20-B76F-777D095F4E6B}
DEFINE_GUID(CLSID_StreamScope,
			0x6886D765, 0x03DB, 0x4E20, 0xB7, 0x6F, 0x77, 0x7D, 0x09, 0x5F, 0x4E, 0x6B);


// CGraphAPI
#define SAFE_RELEASE(p) if(p) {p->Release(); p = NULL;}
#define WM_GRAPHNOTIFY  (WM_USER+20)

STDMETHODIMP CGraphAPI::Initialize(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	if (!m_spGraphBuilder)
	{
		if (SUCCEEDED(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&m_spGraphBuilder)))
		{
			AddToObjectTable();

			return QueryInterfaces();
		}
		m_spGraphBuilder = NULL;
	}

	return S_OK;
}


STDMETHODIMP CGraphAPI::CreateGraph(BOOL bIsRender, BSTR bstrRtspUrl, int nColorSpace)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼

	CheckPointer(m_spGraphBuilder, E_POINTER);

	HRESULT hr = S_OK;

	if(FALSE == m_bEnableGPU)
	{
		hr = AddFilterByCLSID(m_spGraphBuilder, CLSID_DatasteadRtspRtmpSource, L"RTSP Source Filter", &m_spRtspSourceFilter);
		if(FAILED(hr))
			return false;

		SetRtspSourceFilterConfig(bstrRtspUrl);		// Standard Camera
	}
	else
	{
		hr = AddFilterByCLSID(m_spGraphBuilder, CLSID_DeltaRtspSourceFilter, L"Delta Rtsp Source Filter", &m_spRtspSourceFilter);
		if(FAILED(hr))
			return hr;

		IFileSourceFilter* pFileSourceFilter = NULL;
		hr = m_spRtspSourceFilter->QueryInterface(IID_IFileSourceFilter, (void**)&pFileSourceFilter);

		ASSERT(pFileSourceFilter != NULL);

		hr = pFileSourceFilter->Load(bstrRtspUrl, NULL);

		if (pFileSourceFilter)
			pFileSourceFilter->Release();
	}

	IPin *pPinOut= NULL;
	hr = GetUnconnectedPin(m_spRtspSourceFilter, PINDIR_OUTPUT, &pPinOut);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinOut);
		return hr;
	}

	IPin *pPinIn = NULL;

	IBaseFilter *pStreamScope = NULL;
#ifdef _DEBUG_RTSP_STREAM
	IBaseFilter *pTee = NULL;
	hr = AddFilterByCLSID(m_spGraphBuilder, CLSID_Tee, L"InfTee Filter", &pTee);

	hr = GetUnconnectedPin(pTee, PINDIR_INPUT, &pPinIn);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinIn);
		return hr;
	}
	hr = ConnectFilters(pPinOut, pPinIn);
	if(FAILED(hr))
		return hr;
	SAFE_RELEASE(pPinOut);
	SAFE_RELEASE(pPinIn);

	hr = GetUnconnectedPin(pTee, PINDIR_OUTPUT, &pPinOut);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinOut);
		return hr;
	}

	static int i = 0;

	IBaseFilter *pDump = NULL;
	hr = AddFilterByCLSID(m_spGraphBuilder, CLSID_Dump, L"Dump", &pDump);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinOut);
		return false;
	}
	IFileSinkFilter* pFileSinkFilter = NULL;
	hr = pDump->QueryInterface(IID_IFileSinkFilter, (void**)&pFileSinkFilter);

	ASSERT(pFileSinkFilter != NULL);

	CString strFileName;
	strFileName.Format(_T("D:\\test_%d.h264"), i);
	hr = pFileSinkFilter->SetFileName(strFileName, NULL);
	i++;
	pFileSinkFilter->Release();

	hr = GetUnconnectedPin(pDump, PINDIR_INPUT, &pPinIn);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinIn);
		return hr;
	}
	hr = ConnectFilters(pPinOut, pPinIn);
	if(FAILED(hr))
		return hr;
	SAFE_RELEASE(pPinOut);
	SAFE_RELEASE(pPinIn);
	pDump->Release();

	hr = GetUnconnectedPin(pTee, PINDIR_OUTPUT, &pPinOut);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinOut);
		return hr;
	}
	/*hr = AddFilterByCLSID(m_spGraphBuilder, CLSID_StreamScope, L"Stream Scope", &pStreamScope);

	hr = GetUnconnectedPin(pStreamScope, PINDIR_INPUT, &pPinIn);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinIn);
		return false;
	}
	hr = ConnectFilters(pPinOut, pPinIn);
	if(FAILED(hr))
		return hr;
	SAFE_RELEASE(pPinOut);
	SAFE_RELEASE(pPinIn);

	hr = GetUnconnectedPin(pStreamScope, PINDIR_OUTPUT, &pPinOut);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinOut);
		return false;
	}
	pStreamScope->Release();*/
#endif

	if(m_bEnableGPU)
	{
		hr = AddFilterByCLSID(m_spGraphBuilder, CLSID_DeltaCudaH264Decoder, L"Delta Cuda H264 Decoder", &m_spDeltaCudaDecoder);
		if(FAILED(hr))
			return hr;

		hr = GetUnconnectedPin(m_spDeltaCudaDecoder, PINDIR_INPUT, &pPinIn);
		if(FAILED(hr))
		{
			SAFE_RELEASE(pPinIn);
			return hr;
		}

		hr = ConnectFilters(pPinOut, pPinIn);
		if(FAILED(hr))
			return hr;
		SAFE_RELEASE(pPinOut);
		SAFE_RELEASE(pPinIn);

		hr = GetUnconnectedPin(m_spDeltaCudaDecoder, PINDIR_OUTPUT, &pPinOut);
		if(FAILED(hr))
		{
			SAFE_RELEASE(pPinOut);
			return hr;
		}

#ifdef _DEBUG_CUDA_STREAM
		hr = AddFilterByCLSID(m_spGraphBuilder, CLSID_StreamScope, L"Stream Scope", &pStreamScope);

		hr = GetUnconnectedPin(pStreamScope, PINDIR_INPUT, &pPinIn);
		if(FAILED(hr))
		{
			SAFE_RELEASE(pPinIn);
			return false;
		}
		hr = ConnectFilters(pPinOut, pPinIn);
		if(FAILED(hr))
			return hr;
		SAFE_RELEASE(pPinOut);
		SAFE_RELEASE(pPinIn);

		hr = GetUnconnectedPin(pStreamScope, PINDIR_OUTPUT, &pPinOut);
		if(FAILED(hr))
		{
			SAFE_RELEASE(pPinOut);
			return false;
		}
		pStreamScope->Release();
#endif
	}

#ifdef _GPU_NV12TOARGB
	hr = AddFilterByCLSID(m_spGraphBuilder, CLSID_DeltaVideoProcesser, L"Delta Video Processer", &m_spDeltaVideoProcesser);
	if(FAILED(hr))
		return hr;

	SetColorSpace((tool_ColorSpace)nColorSpace);

	hr = GetUnconnectedPin(m_spDeltaVideoProcesser, PINDIR_INPUT, &pPinIn);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinIn);
		return hr;
	}

	hr = ConnectFilters(pPinOut, pPinIn);
	if(FAILED(hr))
		return hr;
	SAFE_RELEASE(pPinOut);
	SAFE_RELEASE(pPinIn);

	hr = GetUnconnectedPin(m_spDeltaVideoProcesser, PINDIR_OUTPUT, &pPinOut);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinOut);
		return hr;
	}

#ifdef _DEBUG_VP_STREAM
	hr = AddFilterByCLSID(m_spGraphBuilder, CLSID_StreamScope, L"Stream Scope", &pStreamScope);

	hr = GetUnconnectedPin(pStreamScope, PINDIR_INPUT, &pPinIn);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinIn);
		return false;
	}
	hr = ConnectFilters(pPinOut, pPinIn);
	if(FAILED(hr))
		return hr;
	SAFE_RELEASE(pPinOut);
	SAFE_RELEASE(pPinIn);

	hr = GetUnconnectedPin(pStreamScope, PINDIR_OUTPUT, &pPinOut);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinOut);
		return false;
	}
	pStreamScope->Release();
#endif

#endif

	hr = m_spGraphBuilder->Render(pPinOut);
	if(FAILED(hr))
	{
		SAFE_RELEASE(pPinOut);
		return hr;
	}
	SAFE_RELEASE(pPinOut);

	return hr;
}


STDMETHODIMP CGraphAPI::SetDisplayWindow(LONG_PTR lWindow)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	if (m_spVideoWindow)
	{
		HWND inWindow = (HWND)lWindow;
		// Hide the video window first
		m_spVideoWindow->put_Visible(OAFALSE);
		m_spVideoWindow->put_Owner((OAHWND)inWindow);

		RECT windowRect;
		::GetClientRect(inWindow, &windowRect);
		m_spVideoWindow->put_Left(0);
		m_spVideoWindow->put_Top(0);
		m_spVideoWindow->put_Width(windowRect.right - windowRect.left);
		m_spVideoWindow->put_Height(windowRect.bottom - windowRect.top);
		m_spVideoWindow->put_WindowStyle(WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS);

		m_spVideoWindow->put_MessageDrain((OAHWND) inWindow);
		// Restore the video window
		if (inWindow != NULL)
		{
			m_spVideoWindow->put_Visible(OATRUE);
		}
		else
		{
			m_spVideoWindow->put_Visible(OAFALSE);
		}
		return S_OK;
	}

	return E_FAIL;
}


STDMETHODIMP CGraphAPI::SetNotifyWindow(LONG_PTR lWindow)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	if (m_spEvent)
	{
		HWND inWindow = (HWND)lWindow;
		return m_spEvent->SetNotifyWindow((OAHWND)inWindow, WM_GRAPHNOTIFY, 0);
	}

	return E_FAIL;
}


STDMETHODIMP CGraphAPI::HandleEvent(UINT_PTR inWParam, LONG_PTR inLParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	if (m_spEvent)
	{
		LONG eventCode = 0, eventParam1 = 0, eventParam2 = 0;
		while (SUCCEEDED(m_spEvent->GetEvent(&eventCode, &eventParam1, &eventParam2, 0)))
		{
			m_spEvent->FreeEventParams(eventCode, eventParam1, eventParam2);
			switch (eventCode)
			{
			case EC_COMPLETE:
				break;

			case EC_USERABORT:
			case EC_ERRORABORT:
				break;

			default:
				break;
			}
		}
	}

	return S_OK;
}


STDMETHODIMP CGraphAPI::Run(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	if (m_spGraphBuilder && m_spMediaControl)
	{
		if (!IsRunning())
		{
			return m_spMediaControl->Run();
		}
		else
		{
			return S_OK;
		}
	}

	return E_FAIL;
}


STDMETHODIMP CGraphAPI::Stop(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	if (m_spGraphBuilder && m_spMediaControl)
	{
		if (!IsStopped())
		{	
			return m_spMediaControl->Stop();
		}
		else
		{
			return S_OK;
		}
	}

	return E_FAIL;
}


STDMETHODIMP CGraphAPI::Pause(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	if (m_spGraphBuilder && m_spMediaControl)
	{
		if (!IsPaused())
		{	
			return m_spMediaControl->Pause();
		}
		else
		{
			return S_OK;
		}
	}

	return E_FAIL;
}


STDMETHODIMP CGraphAPI::InitCalibrate(BOOL bIsStandardCamera)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	CheckPointer(m_spDeltaVideoProcesser, E_POINTER);

	CComPtr<ICameraCalibration> spCalibration;
	HRESULT hr = m_spDeltaVideoProcesser->QueryInterface(IID_ICameraCalibration, (LPVOID *)&spCalibration);
	hr = spCalibration->Initialize(bIsStandardCamera);

	//spCalibration->SetCallback(ColorCorrectionMatrixCallback, this);

	spCalibration.Release();

	return hr;
}


STDMETHODIMP CGraphAPI::EnableCalibrate(BOOL bEnable)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	CheckPointer(m_spDeltaVideoProcesser, false);

	CComPtr<ICameraCalibration> spCalibration;
	HRESULT hr = m_spDeltaVideoProcesser->QueryInterface(IID_ICameraCalibration, (LPVOID *)&spCalibration);
	hr = spCalibration->EnableCalibrate(bEnable);
	spCalibration.Release();

	return hr;
}


STDMETHODIMP CGraphAPI::EnableDemodulate(BOOL bEnable)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	CheckPointer(m_spDeltaVideoProcesser, false);

	CComPtr<ICameraCalibration> spCalibration;
	HRESULT hr = m_spDeltaVideoProcesser->QueryInterface(IID_ICameraCalibration, (LPVOID *)&spCalibration);
	hr = spCalibration->EnableDemodulate(bEnable);
	spCalibration.Release();

	return hr;
}


STDMETHODIMP CGraphAPI::UnInitialize(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	DestroyGraph();

	m_spMediaControl.Release();
	m_spEvent.Release();
	m_spBasicVideo.Release();

	if (m_spVideoWindow)
	{
		m_spVideoWindow->put_Visible(OAFALSE);
		m_spVideoWindow->put_MessageDrain((OAHWND)NULL);
		m_spVideoWindow->put_Owner(OAHWND(0));
		m_spVideoWindow.Release();
	}

	m_spRtspSourceFilter.Release();
	m_spDeltaCudaDecoder.Release();
	m_spDeltaVideoProcesser.Release();

	RemoveFromObjectTable();
	m_spGraphBuilder.Release();

	return S_OK;
}


STDMETHODIMP CGraphAPI::DestroyGraph(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	Stop();

	// Remove all filters in graph
	HRESULT hr = E_FAIL;
    if(m_spGraphBuilder)
	{
		CComPtr<IEnumFilters> pEnumFilters;
		if (m_spGraphBuilder->EnumFilters(&pEnumFilters)==S_OK)
		{
			pEnumFilters->Reset();
			DWORD dwFilterCount = 0;		
			// get filters one by one
			//CComPtr<IBaseFilter> pBaseFilter;
			IBaseFilter *pBaseFilter = NULL;
			while (pEnumFilters->Next(1, &pBaseFilter, NULL)==S_OK && 
                pBaseFilter)
			{
				FILTER_INFO FilterInfo;
                if( SUCCEEDED(pBaseFilter->QueryFilterInfo(&FilterInfo)) )
                {
                    //Print remove filter info
                }
                hr = m_spGraphBuilder->RemoveFilter(pBaseFilter);
				ULONG ulCount = 0;
                ulCount = pBaseFilter->Release();
                pEnumFilters->Reset(); //Reset when filter been remove
			}
			pEnumFilters.Release();
		}
	}

	return S_OK;
}


STDMETHODIMP CGraphAPI::EnableGPUCompute(BOOL bEnable)
{
	m_bEnableGPU = bEnable;

	return NO_ERROR;
}


STDMETHODIMP CGraphAPI::SnapShot(BSTR bstrImgPath)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: 在這裡加入您的實作程式碼
	CheckPointer(m_spDeltaVideoProcesser, false);

	CComPtr<ICameraCalibration> spCalibration;
	HRESULT hr = m_spDeltaVideoProcesser->QueryInterface(IID_ICameraCalibration, (LPVOID *)&spCalibration);
	hr = spCalibration->SnapShot(bstrImgPath);
	spCalibration.Release();

	return hr;
}


void CGraphAPI::AddToObjectTable(void)
{
	IMoniker * pMoniker = 0;
    IRunningObjectTable * objectTable = 0;
    if (SUCCEEDED(GetRunningObjectTable(0, &objectTable))) 
	{
		WCHAR wsz[256];
		wsprintfW(wsz, L"FilterGraph %08p pid %08x", (DWORD_PTR)m_spGraphBuilder.p, GetCurrentProcessId());
		HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
		if (SUCCEEDED(hr)) 
		{
			hr = objectTable->Register(0, m_spGraphBuilder, pMoniker, &m_dwObjectTableEntry);
			pMoniker->Release();
		}
		objectTable->Release();
	}
}


void CGraphAPI::RemoveFromObjectTable(void)
{
	IRunningObjectTable * objectTable = 0;
    if (SUCCEEDED(GetRunningObjectTable(0, &objectTable))) 
	{
        objectTable->Revoke(m_dwObjectTableEntry);
        objectTable->Release();
		m_dwObjectTableEntry = 0;
    }
}


HRESULT CGraphAPI::QueryInterfaces(void)
{
	if (m_spGraphBuilder)
	{
		HRESULT hr = NOERROR;
		hr |= m_spGraphBuilder->QueryInterface(IID_IMediaControl, (void **)&m_spMediaControl);
		hr |= m_spGraphBuilder->QueryInterface(IID_IMediaEventEx, (void **)&m_spEvent);
		hr |= m_spGraphBuilder->QueryInterface(IID_IBasicVideo, (void **)&m_spBasicVideo);
		hr |= m_spGraphBuilder->QueryInterface(IID_IVideoWindow, (void **)&m_spVideoWindow);
		return hr;
	}

	return S_OK;
}


HRESULT CGraphAPI::AddFilterByCLSID(IGraphBuilder *pGraph,  // Pointer to the Filter Graph Manager.
									const GUID& clsid,      // CLSID of the filter to create.
									LPCWSTR wszName,        // A name for the filter.
									IBaseFilter **ppF)      // Receives a pointer to the filter.
{
    if (!pGraph || ! ppF) return E_POINTER;
    *ppF = 0;
    IBaseFilter *pF = 0;
    HRESULT hr = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER,
        IID_IBaseFilter, reinterpret_cast<void**>(&pF));
    if (SUCCEEDED(hr))
    {
        hr = pGraph->AddFilter(pF, wszName);
        if (SUCCEEDED(hr))
            *ppF = pF;
        else
            pF->Release();
    }
    return hr;
}


HRESULT CGraphAPI::GetUnconnectedPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    PIN_DIRECTION PinDir,   // Direction of the pin to find.
    IPin **ppPin)           // Receives a pointer to the pin.
{
    *ppPin = 0;
    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }
    while (pEnum->Next(1, &pPin, NULL) == S_OK)
    {
        PIN_DIRECTION ThisPinDir;
        pPin->QueryDirection(&ThisPinDir);
        if (ThisPinDir == PinDir)
        {
            IPin *pTmp = 0;
            hr = pPin->ConnectedTo(&pTmp);
            if (SUCCEEDED(hr))  // Already connected, not the pin we want.
            {
                pTmp->Release();
            }
            else  // Unconnected, this is the pin we want.
            {
                pEnum->Release();
                *ppPin = pPin;
                return S_OK;
            }
        }
        pPin->Release();
    }
    pEnum->Release();
    // Did not find a matching pin.
    return E_FAIL;
}


// Connect filter from the upstream output pin to the downstream input pin
HRESULT CGraphAPI::ConnectFilters(IPin * inOutputPin, IPin * inInputPin, 
							  const AM_MEDIA_TYPE * inMediaType)
{
	if (m_spGraphBuilder && inOutputPin && inInputPin)
	{
		HRESULT hr = m_spGraphBuilder->ConnectDirect(inOutputPin, inInputPin, inMediaType);
		return hr;
	}

	return E_FAIL;
}


HRESULT CGraphAPI::SetColorSpace(tool_ColorSpace color)
{
	CheckPointer(m_spDeltaVideoProcesser, E_POINTER);

	CComPtr<ICameraCalibration> spCalibration;
	HRESULT hr = m_spDeltaVideoProcesser->QueryInterface(IID_ICameraCalibration, (LPVOID *)&spCalibration);
	switch(color)
	{
	case ColorRGB:
		hr = spCalibration->SetColorSpace(Space_RGB);
		break;
	case ColorRGB_XYZ:
		hr = spCalibration->SetColorSpace(Space_RGB_XYZ);
		break;
	case ColorXYZ:
		hr = spCalibration->SetColorSpace(Space_XYZ);
		break;
	case ColorLab:
		hr = spCalibration->SetColorSpace(Space_Lab);
		break;
	case ColorHSL:
		hr = spCalibration->SetColorSpace(Space_HSL);
		break;
	}
	spCalibration.Release();

	return SUCCEEDED(hr);
}


BOOL CGraphAPI::IsRunning(void)
{
	if (m_spGraphBuilder && m_spMediaControl)
	{
		OAFilterState state = State_Stopped;
		if (SUCCEEDED(m_spMediaControl->GetState(10, &state)))
		{
			return state == State_Running;
		}
	}
	return false;
}


BOOL CGraphAPI::IsStopped(void)
{
	if (m_spGraphBuilder && m_spMediaControl)
	{
		OAFilterState state = State_Stopped;
		if (SUCCEEDED(m_spMediaControl->GetState(10, &state)))
		{
			return state == State_Stopped;
		}
	}
	return false;
}


BOOL CGraphAPI::IsPaused(void)
{
	if (m_spGraphBuilder && m_spMediaControl)
	{
		OAFilterState state = State_Stopped;
		if (SUCCEEDED(m_spMediaControl->GetState(10, &state)))
		{
			return state == State_Paused;
		}
	}
	return false;
}


BOOL CGraphAPI::SetRtspSourceFilterConfig(LPCTSTR lpszRtspUrl)
{
	CheckPointer(m_spRtspSourceFilter, FALSE);

	HRESULT hr = E_FAIL;
	CComPtr<IDatasteadRTSPSourceConfig>	spRtspConfig;
	hr = m_spRtspSourceFilter->QueryInterface(IID_IDatasteadRTSPSourceConfig, (LPVOID *)&spRtspConfig);
	if (FAILED(hr))
		return FALSE;

	hr = spRtspConfig->Action(RTSP_Action_OpenURL, lpszRtspUrl);
	if (FAILED(hr))
		return FALSE;

	return TRUE;
}
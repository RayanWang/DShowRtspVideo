// GraphAPI.h : CGraphAPI 的宣告

#pragma once
#include "resource.h"       // 主要符號



#include "IpCamGraph_i.h"

#include <streams.h>



#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Windows CE 平台上未正確支援單一執行緒 COM 物件，例如 Windows Mobile 平台沒有包含完整的 DCOM 支援。請定義 _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA 來強制 ATL 支援建立單一執行緒 COM 物件的實作，以及允許使用其單一執行緒 COM 物件實作。您的 rgs 檔中的執行緒模型已設定為 'Free'，因為這是非 DCOM Windows CE 平台中唯一支援的執行緒模型。"
#endif

using namespace ATL;

typedef enum tool_ColorSpace
{
	ColorRGB		= 0,
	ColorRGB_XYZ	= 1,
	ColorXYZ		= 2,
	ColorLab		= 3,
	ColorHSL		= 4,
} tool_ColorSpace;

// CGraphAPI

class ATL_NO_VTABLE CGraphAPI :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CGraphAPI, &CLSID_GraphAPI>,
	public IGraphAPI
{
public:
	CGraphAPI()
	{
		m_dwObjectTableEntry = 0;
		m_bEnableGPU = FALSE;
	}
	~CGraphAPI()
	{
		UnInitialize();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_GRAPHAPI)


BEGIN_COM_MAP(CGraphAPI)
	COM_INTERFACE_ENTRY(IGraphAPI)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:
	STDMETHOD(Initialize)(void);
	STDMETHOD(CreateGraph)(BOOL bIsRender, BSTR bstrRtspUrl, int nColorSpace);
	STDMETHOD(SetDisplayWindow)(LONG_PTR lWindow);
	STDMETHOD(SetNotifyWindow)(LONG_PTR lWindow);
	STDMETHOD(HandleEvent)(UINT_PTR inWParam, LONG_PTR inLParam);
	STDMETHOD(Run)(void);
	STDMETHOD(Stop)(void);
	STDMETHOD(Pause)(void);
	STDMETHOD(InitCalibrate)(BOOL bIsStandardCamera);
	STDMETHOD(EnableCalibrate)(BOOL bEnable);
	STDMETHOD(EnableDemodulate)(BOOL bEnable);
	STDMETHOD(UnInitialize)(void);
	STDMETHOD(DestroyGraph)(void);
	STDMETHOD(EnableGPUCompute)(BOOL bEnable);
	STDMETHOD(SnapShot)(BSTR bstrImgPath);

protected:
	void AddToObjectTable(void) ;
	void RemoveFromObjectTable(void);
	HRESULT QueryInterfaces(void);

	HRESULT AddFilterByCLSID(IGraphBuilder *pGraph,  // Pointer to the Filter Graph Manager.
							 const GUID& clsid,      // CLSID of the filter to create.
							 LPCWSTR wszName,        // A name for the filter.
							 IBaseFilter **ppF);     // Receives a pointer to the filter.

	HRESULT GetUnconnectedPin(IBaseFilter *pFilter,   // Pointer to the filter.
							  PIN_DIRECTION PinDir,   // Direction of the pin to find.
							  IPin **ppPin);          // Receives a pointer to the pin.

	HRESULT ConnectFilters(IPin * inOutputPin, IPin * inInputPin, const AM_MEDIA_TYPE * inMediaType = 0);

	HRESULT SetColorSpace(tool_ColorSpace color);

	BOOL IsRunning(void);  // Filter graph status
	BOOL IsStopped(void);
	BOOL IsPaused(void);

	BOOL SetRtspSourceFilterConfig(LPCTSTR lpszRtspUrl);

private:
	CComPtr<IGraphBuilder>		m_spGraphBuilder;  
	CComPtr<IMediaControl> 		m_spMediaControl;
	CComPtr<IMediaEventEx>		m_spEvent;
	CComPtr<IBasicVideo>		m_spBasicVideo;
	CComPtr<IVideoWindow> 		m_spVideoWindow;

	CComPtr<IBaseFilter>		m_spRtspSourceFilter;
	CComPtr<IBaseFilter>		m_spDeltaVideoProcesser;
	CComPtr<IBaseFilter>		m_spDeltaCudaDecoder;

	DWORD						m_dwObjectTableEntry;

	BOOL						m_bEnableGPU;
};

OBJECT_ENTRY_AUTO(__uuidof(GraphAPI), CGraphAPI)

#pragma once

class CCalibration;

class CDeltaVideoProcesser : public CTransformFilter,
							 public ICameraCalibration
{
public:
	CDeltaVideoProcesser(LPUNKNOWN pUnk, HRESULT *phr);

	virtual ~CDeltaVideoProcesser(void);

	DECLARE_IUNKNOWN;

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	HRESULT CheckInputType(const CMediaType *mtIn);

	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);

	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp);

	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);

	// Override this so we can grab the video format
    HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);

#pragma region ICameraCalibration
	// These implement the custom ICameraCalibration interface
	STDMETHODIMP Initialize(BOOL bIsStandardCamera);
	STDMETHODIMP EnableCalibrate(BOOL bEnable){ m_bEnableCalibrate = bEnable; return NO_ERROR; }
	STDMETHODIMP EnableDemodulate(BOOL bEnable){ m_bEnableDemodulate = bEnable; return NO_ERROR; }
	STDMETHODIMP SetColorSpace(ColorSpace color);
	STDMETHODIMP GetCCMatrix(double	(&CCMatrix)[3][3]);
	STDMETHODIMP SetCallback(CCMCallback fpCCMCallback, LPVOID pUserParam);
	STDMETHODIMP SetChartBoundary(BoundaryParam oChartBoundary);
	STDMETHODIMP SetRegressionAlg(BSTR bstrAlg);
	STDMETHODIMP SnapShot(BSTR bstrImgPath);
#pragma endregion

#pragma region CBaseFilter
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Stop(void);
	STDMETHODIMP Pause(void);
#pragma endregion

private:
	HRESULT ProcessFrameUYVY(BYTE *pbInput, BYTE *pbOutput, long *pcbByte);   

    HRESULT ProcessFrameYUY2(BYTE *pbInput, BYTE *pbOutput, long *pcbByte);  

	HRESULT ProcessFrameRGB(BYTE *pData);

	BOOL CanPerformRGB(const CMediaType *pMediaType) const;

	HRESULT Copy(IMediaSample *pSource, IMediaSample *pDest) const;

	DWORD	StandardProc(void);
	static	DWORD	WINAPI StandardThreadProc(LPVOID data)
	{
		CDeltaVideoProcesser *pDeltaVp = (CDeltaVideoProcesser*)data;
		return pDeltaVp->StandardProc();
	}

	DWORD	CalibratedProc(void);
	static	DWORD	WINAPI CalibratedThreadProc(LPVOID data)
	{
		CDeltaVideoProcesser *pDeltaVp = (CDeltaVideoProcesser*)data;
		return pDeltaVp->CalibratedProc();
	}

	void DisplayROI(int nImgWidth, int nLeftTop, int nRightTop, int nLeftBottom, int nRightBottom, RGBTRIPLE *prgb);

private:
    VIDEOINFOHEADER m_VihIn;   // Holds the current video format (input)
    VIDEOINFOHEADER m_VihOut;  // Holds the current video format (output)

#pragma region HeapObj
	// Need to be deleted
	CCalibration	*m_pCalibrate;
	RGBTRIPLE		*m_pPatchRGB;
	RGBTRIPLE		*m_pOriginalRGB;
#pragma endregion

#pragma region BooleanFlags
	BOOL			m_bEnableCalibrate;
	BOOL			m_bEnableDemodulate;
	BOOL			m_bCaptureFrame;
	BOOL			m_bColorChartResult;
#pragma endregion

	// The number of color charts getting from m_pCalibrate
	int				m_nColorChartNum;

#pragma region ThreadParams
	HANDLE			m_hThread;
	DWORD			m_dwThreadID;
	HANDLE			m_hStandardEvent;
	HANDLE			m_hCalibratedEvent;
	HANDLE			m_hPatchEvent;
#pragma endregion

	// Callback function
	CCMCallback		m_fpCCMCallback;
	LPVOID			m_pUserData;

	// For Region Of Interest
	vector<Rect>    m_vOutputBounding;
	Rect			m_rcRoiRect;

	BoundaryParam	m_oChartBoundary;

	string			m_strRegressionAlg;

	BOOL			m_bSnapShot;
	string			m_strImgPath;
};


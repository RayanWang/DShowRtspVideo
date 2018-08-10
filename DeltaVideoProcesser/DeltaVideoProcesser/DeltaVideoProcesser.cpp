#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <streams.h> 
#include <initguid.h> 
#include <olectl.h>
#include <aviriff.h>	// defines 'FCC' macro

#include "iVideoProcesser.h"
#include "Calibration.h"
#include "DeltaVideoProcesser.h"

#include "ParamDef.h"
#include <atlconv.h>

// Forward declares
void GetVideoInfoParameters(
    const VIDEOINFOHEADER *pvih, // Pointer to the format header.
    BYTE  * const pbData,        // Pointer to the first address in the buffer.
    DWORD *pdwWidth,         // Returns the width in pixels.
    DWORD *pdwHeight,        // Returns the height in pixels.
    LONG  *plStrideInBytes,  // Add this to a row to get the new row down
    BYTE **ppbTop,           // Returns pointer to the first byte in the top row of pixels.
    bool bYuv
    );
bool IsValidUYVY(const CMediaType *pmt);
bool IsValidYUY2(const CMediaType *pmt);

//Filter & pins info
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_Video,       // Major type
	&MEDIASUBTYPE_NULL      // Sub type
};


const AMOVIESETUP_PIN psudPins[] =
{
	{
			L"Input",           // String pin name
			FALSE,              // Is it rendered
			FALSE,              // Is it an output
			FALSE,              // Allowed none
			FALSE,              // Allowed many
			&CLSID_NULL,        // Connects to filter
			L"Output",          // Connects to pin
			1,                  // Number of types
			&sudPinTypes		// The pin details
	},     
	{ 
			L"Output",          // String pin name
			FALSE,              // Is it rendered
			TRUE,               // Is it an output
			FALSE,              // Allowed none
			FALSE,              // Allowed many
			&CLSID_NULL,        // Connects to filter
			L"Input",           // Connects to pin
			1,                  // Number of types
			&sudPinTypes        // The pin details
	}
};


const AMOVIESETUP_FILTER sudVideoProcess =
{
	&CLSID_DeltaVideoProcesser,			// Filter CLSID
	VIDEO_PROCESS_FILTER_NAME,			// Filter name
	MERIT_DO_NOT_USE,					// Its merit
	2,									// Number of pins
	psudPins							// Pin details
};


CFactoryTemplate g_Templates[1] = 
{
	{ VIDEO_PROCESS_FILTER_NAME
	, &CLSID_DeltaVideoProcesser
	, CDeltaVideoProcesser::CreateInstance
	, NULL
	, &sudVideoProcess }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


CDeltaVideoProcesser::CDeltaVideoProcesser(LPUNKNOWN punk, HRESULT *phr)
:	CTransformFilter(VIDEO_PROCESS_FILTER_NAME, punk, CLSID_DeltaVideoProcesser)
,	m_pCalibrate(new CCalibration)
,	m_pPatchRGB(NULL)
,	m_pOriginalRGB(NULL)
,	m_bEnableCalibrate(FALSE)
,	m_bEnableDemodulate(FALSE)
,	m_bCaptureFrame(FALSE)
,	m_bColorChartResult(FALSE)
,	m_nColorChartNum(0)
,	m_hThread(NULL)
,	m_dwThreadID(0)
,	m_fpCCMCallback(NULL)
,	m_pUserData(NULL)
,	m_bSnapShot(FALSE)
{
	ASSERT(phr);

	m_hStandardEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
	m_hCalibratedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hPatchEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_vOutputBounding.clear();
	ZeroMemory(&m_rcRoiRect, sizeof(Rect));

	ZeroMemory(&m_oChartBoundary, sizeof(BoundaryParam));

	m_strRegressionAlg = "LeastSquare";
}


CDeltaVideoProcesser::~CDeltaVideoProcesser(void)
{
	if(WAIT_OBJECT_0 != WaitForSingleObject(m_hStandardEvent, 1))
	{
		WORD wVersionRequested; 
		WSADATA wsaData;
  
		wVersionRequested = MAKEWORD(2, 2);
		WSAStartup(wVersionRequested, &wsaData);

		char hostname[256];
		struct hostent *phost;
		gethostname(hostname, 256);
		phost = gethostbyname(hostname);

		// Create address information
		SOCKADDR_IN hostAddr;
		hostAddr.sin_addr = *(in_addr*)phost->h_addr_list[0];
		hostAddr.sin_family = AF_INET;
		hostAddr.sin_port = htons(DEFAULT_PORT);

		// Create connection socket
		SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

		connect(sock, (sockaddr*)&hostAddr, sizeof(sockaddr));

		send(sock, CLOSE_SOCKET, strlen(CLOSE_SOCKET)+1, 0);

		WaitForSingleObject(m_hStandardEvent, INFINITE);

		WSACleanup();
	}

	if(WAIT_OBJECT_0 != WaitForSingleObject(m_hPatchEvent, TIME_OUT))
	{
		DWORD dwExitCode = 0;
		GetExitCodeThread(m_hThread, &dwExitCode);
		TerminateThread(m_hThread, dwExitCode);
	}

	SAFE_DELETE_ARRAY(m_pPatchRGB);
	SAFE_DELETE_ARRAY(m_pOriginalRGB);
	SAFE_DELETE(m_pCalibrate);

	CloseHandle(m_hThread);
	CloseHandle(m_hStandardEvent);
	CloseHandle(m_hCalibratedEvent);
	CloseHandle(m_hPatchEvent);

	m_vOutputBounding.clear();
}


//----------------------------------------------------------------------------
// CDeltaVideoProcesser::CreateInstance
//
// Static method that returns a new instance of our filter.
// Note: The DirectShow class factory object needs this method.
//
// pUnk: Pointer to the controlling IUnknown (usually NULL)
// pHR:  Set this to an error code, if an error occurs
//-----------------------------------------------------------------------------

CUnknown * WINAPI CDeltaVideoProcesser::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHR) 
{
    CDeltaVideoProcesser *pFilter = new CDeltaVideoProcesser(pUnk, pHR);
    if (pFilter == NULL) 
    {
        *pHR = E_OUTOFMEMORY;
    }
    return pFilter;
}


//
// NonDelegatingQueryInterface
//
// Reveals IIPEffect and ISpecifyPropertyPages
//
STDMETHODIMP CDeltaVideoProcesser::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv, E_POINTER);

    if (riid == IID_ICameraCalibration) 
	{
        return GetInterface((ICameraCalibration *) this, ppv);
    }
	else 
	{
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


//----------------------------------------------------------------------------
// CDeltaVideoProcesser::CheckInputType
//  
// checks whether a specified media type is acceptable for input.
// Examine a proposed input type. Returns S_OK if we can accept his input type
// or VFW_E_TYPE_NOT_ACCEPTED otherwise. This filter accepts UYVY types only.
//-----------------------------------------------------------------------------

HRESULT CDeltaVideoProcesser::CheckInputType(const CMediaType *pmt)
{
	if (pmt->majortype	== MEDIATYPE_Video		&&
		(pmt->subtype	== MEDIASUBTYPE_RGB24	/*||
		pmt->subtype	== MEDIASUBTYPE_UYVY	||
		pmt->subtype	== MEDIASUBTYPE_YUY2*/))
	{
		return NOERROR;
	}

	return VFW_E_TYPE_NOT_ACCEPTED; 
}


//----------------------------------------------------------------------------
// CDeltaVideoProcesser::CheckTransform
//
// Compare an input type with an output type, and see if we can convert from 
// one to the other. The input type is known to be OK from ::CheckInputType,
// so this is really a check on the output type.
//-----------------------------------------------------------------------------


HRESULT CDeltaVideoProcesser::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	// Make sure the subtypes match   
    if (mtIn->subtype != mtOut->subtype)   
    {   
        return VFW_E_TYPE_NOT_ACCEPTED;   
    }   
   
    if (!IsValidUYVY(mtOut) && !IsValidYUY2(mtOut) && !CanPerformRGB(mtIn))   
    {   
		return VFW_E_TYPE_NOT_ACCEPTED;
    }
   
    BITMAPINFOHEADER *pBmi = HEADER(mtIn);   
    BITMAPINFOHEADER *pBmi2 = HEADER(mtOut);   
   
    if ((pBmi->biWidth <= pBmi2->biWidth) &&   
        (pBmi->biHeight == abs(pBmi2->biHeight)))   
    {   
       return S_OK;   
    }

    return VFW_E_TYPE_NOT_ACCEPTED;
}


//----------------------------------------------------------------------------
// CDeltaVideoProcesser::DecideBufferSize
//
// Decide the buffer size and other allocator properties, for the downstream
// allocator.
//
// pAlloc: Pointer to the allocator. 
// pProp: Contains the downstream filter's request (or all zeroes)
//-----------------------------------------------------------------------------

HRESULT CDeltaVideoProcesser::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProp)
{
	// Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) 
	{
        return E_UNEXPECTED;
    }

    CheckPointer(pAlloc, E_POINTER);
    CheckPointer(pProp, E_POINTER);
    HRESULT hr = NOERROR;

    pProp->cBuffers = 1;
    pProp->cbBuffer = m_pInput->CurrentMediaType().GetSampleSize();
    ASSERT(pProp->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProp, &Actual);
    if (FAILED(hr))
	{
        return hr;
    }

    ASSERT( Actual.cBuffers == 1 );

    if (pProp->cBuffers > Actual.cBuffers ||
        pProp->cbBuffer > Actual.cbBuffer) 
	{
        return E_FAIL;
    }

    return NOERROR;
}


//----------------------------------------------------------------------------
// CDeltaVideoProcesser::GetMediaType
//
// Return an output type that we like, in order of preference, by index number.
//
// iPosition: index number
// pMediaType: Write the media type into this object.
//-----------------------------------------------------------------------------

HRESULT CDeltaVideoProcesser::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    // This should never happen

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Do we have more items to offer

    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    CheckPointer(pMediaType,E_POINTER);
    *pMediaType = m_pInput->CurrentMediaType();

    return NOERROR;
}


//----------------------------------------------------------------------------
// CDeltaVideoProcesser::Transform
//
// Transform the image.
//
// pSource: Contains the source image.
// pDest:   Write the transformed image here.
//-----------------------------------------------------------------------------


HRESULT CDeltaVideoProcesser::Transform(IMediaSample *pSource, IMediaSample *pDest)
{
    // Note: The filter has already set the sample properties on pOut,   
    // (see CTransformFilter::InitializeOutputSample).   
    // You can override the timestamps if you need - but not in our case.   
   
    // The filter already locked m_csReceive so we're OK.   
   
    // Process the buffers   
    // Do it slightly differently for different video formats   
    HRESULT hr = S_OK;   
   
    ASSERT(m_VihOut.bmiHeader.biCompression == FCC('UYVY') ||   
        m_VihOut.bmiHeader.biCompression == FCC('YUY2') ||
		m_VihOut.bmiHeader.biCompression == BI_RGB);

	// Get the addresses of the actual bufffers.   
	BYTE *pBufferIn, *pBufferOut;   
   
	pSource->GetPointer(&pBufferIn);   
	pDest->GetPointer(&pBufferOut);  
   
	if(m_VihOut.bmiHeader.biCompression == BI_RGB)  
    {   
		hr = Copy(pSource, pDest);
		if (FAILED(hr)) 
		{
			return hr;
		}

		AM_MEDIA_TYPE* pType = &m_pInput->CurrentMediaType();
		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pType->pbFormat;
		ASSERT(pvi);

		// Get the image properties from the BITMAPINFOHEADER
		int cxImage    = pvi->bmiHeader.biWidth;
		int cyImage    = pvi->bmiHeader.biHeight;

		FrameData Frame;
		Frame.nWidth	= cxImage;
		Frame.nHeight	= cyImage;
		long lSourceSize = pSource->GetActualDataLength();

		if(m_bSnapShot)
		{
			m_bSnapShot = FALSE;

			Frame.pFrame	= new BYTE[lSourceSize];
			RGBTRIPLE *prgb = (RGBTRIPLE*) pBufferOut;
			for(int i = lSourceSize - 1; i >= 0; ++prgb)
			{
				Frame.pFrame[i--] = prgb->rgbtRed;
				Frame.pFrame[i--] = prgb->rgbtGreen;
				Frame.pFrame[i--] = prgb->rgbtBlue;
			}

			m_pCalibrate->SaveImageToFile(&Frame, m_strImgPath);
			SAFE_DELETE_ARRAY(Frame.pFrame);
		}

		if(m_bCaptureFrame)
		{
			m_bCaptureFrame = FALSE;

			Frame.pFrame	= new BYTE[lSourceSize];
			RGBTRIPLE *prgb = (RGBTRIPLE*) pBufferOut;
			for(int i = lSourceSize - 1; i >= 0; ++prgb)
			{
				Frame.pFrame[i--] = prgb->rgbtRed;
				Frame.pFrame[i--] = prgb->rgbtGreen;
				Frame.pFrame[i--] = prgb->rgbtBlue;
			}

#ifdef _SNAP_SHOT_
			// Save image data to file
			using namespace std;
			static int nBmpIndex = 0;
			string strFileName;
			stringstream ss(strFileName);
			ss << ++nBmpIndex;
			strFileName = ss.str() + ".jpg";
			string strFilePath(SAVE_IMAGE_PATH);
			strFilePath += strFileName;
			m_pCalibrate->SaveImageToFile(&Frame, strFilePath);
#endif

			m_nColorChartNum = 0;
			m_vOutputBounding.clear();
			ZeroMemory(&m_rcRoiRect, sizeof(Rect));
			m_pCalibrate->GetColorChartNum( &Frame, 
											m_strRegressionAlg,
											&m_nColorChartNum);
			if(m_nColorChartNum)
			{
				SAFE_DELETE_ARRAY(m_pPatchRGB);
				m_pPatchRGB = new RGBTRIPLE[m_nColorChartNum];
				SAFE_DELETE_ARRAY(m_pOriginalRGB);
				m_pOriginalRGB = new RGBTRIPLE[m_nColorChartNum];
				m_pCalibrate->GetColorPatchArry(&m_pPatchRGB, &m_pOriginalRGB);

				m_pCalibrate->GetRoiRect(m_rcRoiRect);
				m_pCalibrate->GetOutputBounding(m_vOutputBounding);
			}
			SetEvent(m_hPatchEvent);

			SAFE_DELETE_ARRAY(Frame.pFrame);
		}

		if(m_bEnableCalibrate || m_bEnableDemodulate)
			ProcessFrameRGB(pBufferOut);

		if(WAIT_OBJECT_0 == WaitForSingleObject(m_hCalibratedEvent, 1))
		{
			ResetEvent(m_hCalibratedEvent);

#ifdef _CALLBACK_FUNCTION_
			if(m_bColorChartResult)
			{
				m_bColorChartResult = FALSE;
				m_fpCCMCallback(CALIBRATED_CALLBACK, m_nColorChartNum, (void*)m_pOriginalRGB, m_pUserData);
				SAFE_DELETE_ARRAY(m_pOriginalRGB);

				double dbPolyCoef[3];
				m_pCalibrate->GetPolynomialCoef(dbPolyCoef, R);
				m_fpCCMCallback(CALIBRATED_POLYNOMIAL, R, (void*)dbPolyCoef, m_pUserData);
				m_pCalibrate->GetPolynomialCoef(dbPolyCoef, G);
				m_fpCCMCallback(CALIBRATED_POLYNOMIAL, G, (void*)dbPolyCoef, m_pUserData);
				m_pCalibrate->GetPolynomialCoef(dbPolyCoef, B);
				m_fpCCMCallback(CALIBRATED_POLYNOMIAL, B, (void*)dbPolyCoef, m_pUserData);
			}
			else
				m_fpCCMCallback(COLORCHART_FAILED, 0, NULL, m_pUserData);
#endif
		}

		if(m_rcRoiRect.width && m_rcRoiRect.height)
		{
			AM_MEDIA_TYPE* pType = &m_pInput->CurrentMediaType();
			VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pType->pbFormat;
			ASSERT(pvi);

			int cxImage    = pvi->bmiHeader.biWidth;
			int cyImage    = pvi->bmiHeader.biHeight;
			RGBTRIPLE *prgb = (RGBTRIPLE*) pBufferOut;

			int nLTPoint = cxImage * (cyImage-(m_rcRoiRect.y-1)) + m_rcRoiRect.x;
			int nRTPoint = cxImage * (cyImage-(m_rcRoiRect.y-1)) + m_rcRoiRect.x + m_rcRoiRect.width;
			int nRBPoint = cxImage * (cyImage-(m_rcRoiRect.y+m_rcRoiRect.height-1)) + m_rcRoiRect.x + m_rcRoiRect.width;
			int nLBPoint = cxImage * (cyImage-(m_rcRoiRect.y+m_rcRoiRect.height-1)) + m_rcRoiRect.x;

			for(int c = -2; c <= 2; ++c)
			{
				for(int j = nLTPoint; j <= nRTPoint; ++j)
				{
					prgb[j+c*cxImage].rgbtRed = 255;
					prgb[j+c*cxImage].rgbtGreen = 0;
					prgb[j+c*cxImage].rgbtBlue = 0;
				}
				for(int k = nLBPoint; k <= nRBPoint; ++k)
				{
					prgb[k+c*cxImage].rgbtRed = 255;
					prgb[k+c*cxImage].rgbtGreen = 0;
					prgb[k+c*cxImage].rgbtBlue = 0;
				}
				for(int m = nLBPoint; m <= nLTPoint; m += cxImage)
				{
					prgb[m+c].rgbtRed = 255;
					prgb[m+c].rgbtGreen = 0;
					prgb[m+c].rgbtBlue = 0;
				}
				for(int n = nRBPoint; n <= nRTPoint; n += cxImage)
				{
					prgb[n+c].rgbtRed = 255;
					prgb[n+c].rgbtGreen = 0;
					prgb[n+c].rgbtBlue = 0;
				}
			}
		}

		if(m_vOutputBounding.size())
		{
			for(int i = 0; i < (int)m_vOutputBounding.size(); ++i)
			{
				AM_MEDIA_TYPE* pType = &m_pInput->CurrentMediaType();
				VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pType->pbFormat;
				ASSERT(pvi);

				int cxImage    = pvi->bmiHeader.biWidth;
				int cyImage    = pvi->bmiHeader.biHeight;
				RGBTRIPLE *prgb = (RGBTRIPLE*) pBufferOut;

				int nPoint = cxImage*(cyImage-(m_vOutputBounding[i].y+m_vOutputBounding[i].height/2-1)) + m_vOutputBounding[i].x + m_vOutputBounding[i].width/2;
				
				for(int j = -3; j <= 3; ++j)
				{
					for(int k = -3; k <= 3; ++k)
					{
						// Horizontal line
						ZeroMemory(&prgb[nPoint+j+k*cxImage], sizeof(RGBTRIPLE));

						// Vertical line
						ZeroMemory(&prgb[nPoint+j*cxImage+k], sizeof(RGBTRIPLE));
					}
				}

				// The outermost bounding
				/*int nLeftTop		= cxImage*(m_vOutputBounding[i].y-1) + m_vOutputBounding[i].x;
				int nRightTop		= nLeftTop + m_vOutputBounding[i].width;
				int nLeftBottom		= cxImage*(m_vOutputBounding[i].y+m_vOutputBounding[i].height-1) + m_vOutputBounding[i].x;
				int nRightBottom	= nLeftBottom + m_vOutputBounding[i].width;*/

				//DisplayROI(cxImage, nLeftTop, nRightTop, nLeftBottom, nRightBottom, prgb);

				// The secondary bounding
				/*nLeftTop		= nLeftTop + cxImage + 1;
				nRightTop		= nLeftTop + m_vOutputBounding[i].width - 2;
				nLeftBottom		= cxImage*(m_vOutputBounding[i].y+1+m_vOutputBounding[i].height-3) + m_vOutputBounding[i].x + 1;
				nRightBottom	= nLeftBottom + m_vOutputBounding[i].width - 2;*/

				//DisplayROI(cxImage, nLeftTop, nRightTop, nLeftBottom, nRightBottom, prgb);
			}
		}
    }
	else
	{
		// Look for format changes from the video renderer.   
		CMediaType *pmt = 0;   
		if (S_OK == pDest->GetMediaType((AM_MEDIA_TYPE**)&pmt) && pmt)   
		{   
			DbgLog((LOG_TRACE, 0, TEXT("CDeltaVideoProcesser: Handling format change from the renderer...")));   
   
			// Notify our own output pin about the new type.   
			m_pOutput->SetMediaType(pmt);   
			DeleteMediaType(pmt);   
		}   
   
		long cbByte = 0;   

		if (m_VihOut.bmiHeader.biCompression == FCC('UYVY'))   
		{   
			hr = ProcessFrameUYVY(pBufferIn, pBufferOut, &cbByte);   
		}    
		else if (m_VihOut.bmiHeader.biCompression == FCC('YUY2'))   
		{   
			hr = ProcessFrameYUY2(pBufferIn, pBufferOut, &cbByte);   
		}   
		else
			return E_UNEXPECTED;

		// Set the size of the destination image.
		ASSERT(pDest->GetSize() >= cbByte);   
       
		pDest->SetActualDataLength(cbByte); 
	}
   
    return hr;
}


//----------------------------------------------------------------------------
// CDeltaVideoProcesser::SetMediaType
//
// The CTransformFilter class calls this method when the media type is 
// set on either pin. This gives us a chance to grab the format block. 
//
// direction: Which pin (input or output) 
// pmt: The media type that is being set.
//
// The method is called when the media type is set on one of the filter's pins.
// Note: If the pins were friend classes of the filter, we could access the
// connection type directly. But this is easier than sub-classing the pins.
//-----------------------------------------------------------------------------

HRESULT CDeltaVideoProcesser::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
    if (direction == PINDIR_INPUT)
    {
        //ASSERT(pmt->formattype == FORMAT_VideoInfo);
        VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmt->pbFormat;

        // WARNING! In general you cannot just copy a VIDEOINFOHEADER
        // struct, because the BITMAPINFOHEADER member may be followed by
        // random amounts of palette entries or color masks. (See VIDEOINFO
        // structure in the DShow SDK docs.) Here it's OK because we just
        // want the information that's in the VIDEOINFOHEADER stuct itself.

        CopyMemory(&m_VihIn, pVih, sizeof(VIDEOINFOHEADER));
    }
    else   // output pin
    {
        ASSERT(direction == PINDIR_OUTPUT);
        //ASSERT(pmt->formattype == FORMAT_VideoInfo);
        VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmt->pbFormat;

        CopyMemory(&m_VihOut, pVih, sizeof(VIDEOINFOHEADER));
    }

    return S_OK;
}


STDMETHODIMP CDeltaVideoProcesser::Initialize(BOOL bIsStandardCamera)
{
	if(bIsStandardCamera)	// Server
	{
		m_hThread = CreateThread(	NULL,					// default security attributes
									0,						// use default stack size
									StandardThreadProc,		// thread function
									this,					// argument to thread function
									CREATE_SUSPENDED,		// use default creation flags
									&m_dwThreadID);			// returns the thread identifier
		ResetEvent(m_hStandardEvent);
	}
	else
	{
		m_hThread = CreateThread(	NULL,					// default security attributes
									0,						// use default stack size
									CalibratedThreadProc,	// thread function
									this,					// argument to thread function
									CREATE_SUSPENDED,		// use default creation flags
									&m_dwThreadID);			// returns the thread identifier
		ResetEvent(m_hCalibratedEvent);
	}

	if(m_hThread)
	{
		ResumeThread(m_hThread);

		return NOERROR;
	}

	return E_FAIL;
}


STDMETHODIMP CDeltaVideoProcesser::SetColorSpace(ColorSpace color)
{
	if(m_pCalibrate) 
	{
		switch(color)
		{
		case Space_RGB:
			return (m_pCalibrate->SetColorSpace(ColorRGB)) ? S_OK : E_FAIL;
		case Space_RGB_XYZ:
			return (m_pCalibrate->SetColorSpace(ColorRGB2XYZ)) ? S_OK : E_FAIL;
		case Space_XYZ:
			return (m_pCalibrate->SetColorSpace(ColorXYZ)) ? S_OK : E_FAIL;
		case Space_Lab:
			return (m_pCalibrate->SetColorSpace(ColorLab)) ? S_OK : E_FAIL;
		case Space_HSL:
			return (m_pCalibrate->SetColorSpace(ColorHSL)) ? S_OK : E_FAIL;
		}
	}

	return E_FAIL;
}


STDMETHODIMP CDeltaVideoProcesser::GetCCMatrix(double (&CCMatrix)[3][3])
{
	CheckPointer(m_pCalibrate, E_POINTER);

	m_pCalibrate->GetCCMatrix(CCMatrix);

	return NOERROR;
}


STDMETHODIMP CDeltaVideoProcesser::SetCallback(CCMCallback fpCCMCallback, LPVOID pUserParam)
{
	m_fpCCMCallback = fpCCMCallback;
	m_pUserData = pUserParam;

	return NOERROR;
}


STDMETHODIMP CDeltaVideoProcesser::SetChartBoundary(BoundaryParam oChartBoundary)
{
	m_oChartBoundary = oChartBoundary;

	return NOERROR;
}


STDMETHODIMP CDeltaVideoProcesser::SetRegressionAlg(BSTR bstrAlg)
{
	USES_CONVERSION;
	m_strRegressionAlg = std::string(W2A(bstrAlg));

	return NOERROR; 
}


STDMETHODIMP CDeltaVideoProcesser::SnapShot(BSTR bstrImgPath)
{
	USES_CONVERSION;
	m_bSnapShot = TRUE;
	m_strImgPath = std::string(W2A(bstrImgPath));

	return NOERROR; 
}


STDMETHODIMP CDeltaVideoProcesser::Run(REFERENCE_TIME tStart)
{
	return CBaseFilter::Run(tStart);
}


STDMETHODIMP CDeltaVideoProcesser::Stop(void)
{
	SetEvent(m_hPatchEvent);

	return CBaseFilter::Stop();
}


STDMETHODIMP CDeltaVideoProcesser::Pause(void)
{
	return CBaseFilter::Pause();
}


//----------------------------------------------------------------------------   
// CDeltaVideoProcesser::ProcessFrameUYVY   
//   
// Private method to process one frame of UYVY data.   
//   
// pbInput:  Pointer to the buffer that holds the image.   
// pbOutput: Write the transformed image here.   
// pcbBytes: Receives the number of bytes written.   
//-----------------------------------------------------------------------------   
   
HRESULT CDeltaVideoProcesser::ProcessFrameUYVY(BYTE *pbInput, BYTE *pbOutput, long *pcbByte)   
{   
    DWORD dwWidth, dwHeight;      // Width and height in pixels (input)   
    DWORD dwWidthOut, dwHeightOut;    // Width and height in pixels (output)   
    LONG  lStrideIn, lStrideOut;  // Stride in bytes   
    BYTE  *pbSource, *pbTarget;   // First byte in first row, for source and target.   
   
    *pcbByte = m_VihOut.bmiHeader.biSizeImage;   
   
    GetVideoInfoParameters(&m_VihIn, pbInput, &dwWidth, &dwHeight, &lStrideIn, &pbSource, true);   
    GetVideoInfoParameters(&m_VihOut, pbOutput, &dwWidthOut, &dwHeightOut, &lStrideOut, &pbTarget, true);   
   
    // Formats should match (except maybe stride)   
    ASSERT(dwWidth == dwWidthOut);   
    ASSERT(dwHeight == dwHeightOut);    
   
    // You could optimize this slightly by storing these values when the   
    // media type is set, instead of re-calculating them for each frame.   
   
    for (DWORD y = 0; y < dwHeight; y++)
    {
        WORD *pwTarget = (WORD*)pbTarget;
        WORD *pwSource = (WORD*)pbSource;

        for (DWORD x = 0; x < dwWidth; x++)
        {
            // Each WORD is a 'UY' or 'VY' block.
            // Set the low byte (chroma) to 0x80 and leave the high byte (luma)

            WORD pixel = pwSource[x] & 0xFF00;
            pixel |= 0x0080;
            pwTarget[x] = pixel;
        }

        // Advance the stride on both buffers.   
   
        pbTarget += lStrideOut;   
        pbSource += lStrideIn;   
    }   

    return S_OK;
}   
   
//----------------------------------------------------------------------------   
// CDeltaVideoProcesser::ProcessFrameYUY2   
//   
// Private method to process one frame of YUY2 data.   
//   
// pbInput:  Pointer to the buffer that holds the image.   
// pbOutput: Write the transformed image here.   
// pcbBytes: Receives the number of bytes written.   
//-----------------------------------------------------------------------------   
   
HRESULT CDeltaVideoProcesser::ProcessFrameYUY2(BYTE *pbInput, BYTE *pbOutput, long *pcbByte)   
{   
    DWORD dwWidth, dwHeight;      // Width and height in pixels   
    DWORD dwWidthOut, dwHeightOut;    // Width and height in pixels (output)   
    LONG  lStrideIn, lStrideOut;  // Stride in bytes   
    BYTE  *pbSource, *pbTarget;   // First byte in first row, for source and target.   
   
    *pcbByte = m_VihOut.bmiHeader.biSizeImage;   
   
    GetVideoInfoParameters(&m_VihIn, pbInput, &dwWidth, &dwHeight, &lStrideIn, &pbSource, true);   
    GetVideoInfoParameters(&m_VihOut, pbOutput, &dwWidthOut, &dwHeightOut, &lStrideOut, &pbTarget, true);   
   
    // Formats should match (except maybe stride)   
    ASSERT(dwWidth == dwWidthOut);   
    ASSERT(dwHeight == dwHeightOut);
   
    // You could optimize this slightly by storing these values when the   
    // media type is set, instead of re-calculating them for each frame.   
   
    for (DWORD y = 0; y < dwHeight; y++)   
    {   
        WORD *pwTarget = (WORD*)pbTarget;   
        WORD *pwSource = (WORD*)pbSource;   
   
        for (DWORD x = 0; x < dwWidth; x++)   
        {   
            // Each WORD is a 'YU' or 'YV' block.    
            // Set the high byte (chroma) to 0x80 and leave the low byte (luma)   
   
            WORD pixel = pwSource[x] & 0x00FF;   
            pixel |= 0x8000;   
            pwTarget[x] = pixel;   
        }   
   
        // Advance the stride on both buffers.   
   
        pbTarget += lStrideOut;   
        pbSource += lStrideIn;   
    }   
   
    return S_OK;
}


//----------------------------------------------------------------------------   
// CDeltaVideoProcesser::ProcessFrameRGB 
//   
// Private method to process one frame of RGB data.   
// 
//-----------------------------------------------------------------------------  

HRESULT CDeltaVideoProcesser::ProcessFrameRGB(BYTE *pData)
{
	CheckPointer(pData, E_POINTER);
	CheckPointer(m_pCalibrate, E_POINTER);

    int iPixel;                 // Used to loop through the image pixels
    RGBTRIPLE *prgb;            // Holds a pointer to the current pixel

    AM_MEDIA_TYPE* pType = &m_pInput->CurrentMediaType();
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pType->pbFormat;
    ASSERT(pvi);

    // Get the image properties from the BITMAPINFOHEADER

    int cxImage    = pvi->bmiHeader.biWidth;
    int cyImage    = pvi->bmiHeader.biHeight;
    int numPixels  = cxImage * cyImage;
                        
	prgb = (RGBTRIPLE*) pData;
    for (iPixel = 0; iPixel < numPixels; iPixel++, prgb++) 
	{
		if(m_bEnableDemodulate)
			m_pCalibrate->Demodulate(prgb);

        // Do camera calibration
		if(m_bEnableCalibrate)
			m_pCalibrate->CalibratedRGB(prgb);
    } 
   
    return S_OK;
}


//
// CanPerformRGB
//
// Check if this is a RGB24 true colour format
//
BOOL CDeltaVideoProcesser::CanPerformRGB(const CMediaType *pMediaType) const
{
    CheckPointer(pMediaType,FALSE);

    if (IsEqualGUID(*pMediaType->Type(), MEDIATYPE_Video)) 
    {
        if (IsEqualGUID(*pMediaType->Subtype(), MEDIASUBTYPE_RGB24)) 
        {
            VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pMediaType->Format();
            return (pvi->bmiHeader.biBitCount == 24);
        }
    }

    return FALSE;

} // CanPerformRGB


//
// Copy
//
// Make destination an identical copy of source
//
HRESULT CDeltaVideoProcesser::Copy(IMediaSample *pSource, IMediaSample *pDest) const
{
    CheckPointer(pSource,E_POINTER);   
    CheckPointer(pDest,E_POINTER);   

    // Copy the sample data

    BYTE *pSourceBuffer, *pDestBuffer;
    long lSourceSize = pSource->GetActualDataLength();

#ifdef DEBUG
    long lDestSize = pDest->GetSize();
    ASSERT(lDestSize >= lSourceSize);
#endif

    pSource->GetPointer(&pSourceBuffer);
    pDest->GetPointer(&pDestBuffer);

    CopyMemory( (PVOID) pDestBuffer, (PVOID) pSourceBuffer, lSourceSize);

    // Copy the sample times

    REFERENCE_TIME TimeStart, TimeEnd;
    if (NOERROR == pSource->GetTime(&TimeStart, &TimeEnd)) 
	{
        pDest->SetTime(&TimeStart, &TimeEnd);
    }

    LONGLONG MediaStart, MediaEnd;
    if (pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) 
	{
        pDest->SetMediaTime(&MediaStart,&MediaEnd);
    }

    // Copy the Sync point property

    HRESULT hr = pSource->IsSyncPoint();
    if (hr == S_OK) 
	{
        pDest->SetSyncPoint(TRUE);
    }
    else if (hr == S_FALSE) 
	{
        pDest->SetSyncPoint(FALSE);
    }
    else 
	{  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the media type

    AM_MEDIA_TYPE *pMediaType;
    pSource->GetMediaType(&pMediaType);
    pDest->SetMediaType(pMediaType);
    DeleteMediaType(pMediaType);

    // Copy the preroll property

    hr = pSource->IsPreroll();
    if (hr == S_OK) 
	{
        pDest->SetPreroll(TRUE);
    }
    else if (hr == S_FALSE) 
	{
        pDest->SetPreroll(FALSE);
    }
    else 
	{  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the discontinuity property

    hr = pSource->IsDiscontinuity();
    if (hr == S_OK) 
	{
		pDest->SetDiscontinuity(TRUE);
    }
    else if (hr == S_FALSE) 
	{
        pDest->SetDiscontinuity(FALSE);
    }
    else 
	{  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the actual data length

    long lDataLength = pSource->GetActualDataLength();
    pDest->SetActualDataLength(lDataLength);

    return NOERROR;

} // Copy


void CDeltaVideoProcesser::DisplayROI(int nImgWidth, int nLeftTop, int nRightTop, int nLeftBottom, int nRightBottom, RGBTRIPLE *prgb)
{
	for(int j = nLeftTop; j <= nRightTop; ++j)
	{
		prgb[j].rgbtBlue	= 0;
		prgb[j].rgbtGreen	= 0;
		prgb[j].rgbtRed		= 255;
	}
	for(int j = nLeftBottom; j <= nRightBottom; ++j)
	{
		prgb[j].rgbtBlue	= 0;
		prgb[j].rgbtGreen	= 0;
		prgb[j].rgbtRed		= 255;
	}
	for(int j = nLeftTop; j <= nLeftBottom; j += nImgWidth)
	{
		prgb[j].rgbtBlue	= 0;
		prgb[j].rgbtGreen	= 0;
		prgb[j].rgbtRed		= 255;
	}
	for(int j = nRightTop; j <= nRightBottom; j += nImgWidth)
	{
		prgb[j].rgbtBlue	= 0;
		prgb[j].rgbtGreen	= 0;
		prgb[j].rgbtRed		= 255;
	}
}


DWORD CDeltaVideoProcesser::StandardProc(void)
{
	// Version check
	WORD wVersionRequested;
    WSADATA wsaData;

    int err;
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0)
	{
		SetEvent(this->m_hStandardEvent);
        return 0;
	}

	if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        WSACleanup();
		SetEvent(this->m_hStandardEvent);
        return 0;
    }

	// Create socket   
    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
  
	// Create IP address and port
    SOCKADDR_IN addrSvr;
    addrSvr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    addrSvr.sin_family = AF_INET;
    addrSvr.sin_port = htons(DEFAULT_PORT);
   
    bind(ListenSocket, (SOCKADDR*)&addrSvr, sizeof(SOCKADDR));   
    listen(ListenSocket, 5);
    sockaddr_in addrClient;
    int len = sizeof(sockaddr);

	int nResult = 0;

	do
	{
		// Acquire a client connection
		SOCKET ClientSocket = accept(ListenSocket, (sockaddr*)&addrClient, &len);

		char rev[DEFAULT_BUFLEN];
		ZeroMemory(rev, sizeof(rev));
		nResult = recv(ClientSocket, rev, DEFAULT_BUFLEN, 0);
		if(nResult > 0)
		{
			if(0 == strcmp(rev, CLOSE_SOCKET))
			{
				closesocket(ClientSocket);
				break;
			}
		}

		nResult = send(ClientSocket, START_COLOR_PATCH, strlen(START_COLOR_PATCH)+1, 0);
		if (nResult == SOCKET_ERROR) 
		{
			closesocket(ClientSocket);
			continue;
		}

		// ------------------------------ //
		// Fill 3xN RGB Matrix to sendbuffer
		if(this->m_pCalibrate)
		{
			ResetEvent(this->m_hPatchEvent);
			this->m_bCaptureFrame = TRUE;
			WaitForSingleObject(this->m_hPatchEvent, INFINITE);

			if(NULL == this->m_pPatchRGB)
			{
				closesocket(ClientSocket);
				continue;
			}

#ifdef _CALLBACK_FUNCTION_
			this->m_fpCCMCallback(STANDARD_CALLBACK, this->m_nColorChartNum, (void*)(this->m_pOriginalRGB), this->m_pUserData);

			double dbPolyCoef[3];
			this->m_pCalibrate->GetPolynomialCoef(dbPolyCoef, R);
			this->m_fpCCMCallback(STANDARD_POLYNOMIAL, R, (void*)dbPolyCoef, this->m_pUserData);
			this->m_pCalibrate->GetPolynomialCoef(dbPolyCoef, G);
			this->m_fpCCMCallback(STANDARD_POLYNOMIAL, G, (void*)dbPolyCoef, this->m_pUserData);
			this->m_pCalibrate->GetPolynomialCoef(dbPolyCoef, B);
			this->m_fpCCMCallback(STANDARD_POLYNOMIAL, B, (void*)dbPolyCoef, this->m_pUserData);
#endif

			BYTE *psendbuffer = new BYTE[this->m_nColorChartNum*3];

			int nBufferIndex = 0;
			for(int i = 0; i < this->m_nColorChartNum; ++i)
			{
				psendbuffer[nBufferIndex++] = this->m_pPatchRGB[i].rgbtRed;
				psendbuffer[nBufferIndex++] = this->m_pPatchRGB[i].rgbtGreen;
				psendbuffer[nBufferIndex++] = this->m_pPatchRGB[i].rgbtBlue;
				if(nBufferIndex >= this->m_nColorChartNum*3)
					break;
			}

			if(this->m_nColorChartNum)
			{
				nResult = send(ClientSocket, (char*)psendbuffer, this->m_nColorChartNum*3, 0);
				if (nResult == SOCKET_ERROR) 
				{
					closesocket(ClientSocket);
					break;
				}
			}

			SAFE_DELETE_ARRAY(psendbuffer);

			// Receive data from client socket
			char recvbuffer[DEFAULT_BUFLEN];
			ZeroMemory(recvbuffer, sizeof(recvbuffer));
			nResult = recv(ClientSocket, recvbuffer, DEFAULT_BUFLEN, 0);
		}
		// ------------------------------ //

		closesocket(ClientSocket);

	} while(true);

	// Close socket
	closesocket(ListenSocket);
    WSACleanup();

	SetEvent(this->m_hStandardEvent);

	return 0;
} // StandardProc


DWORD CDeltaVideoProcesser::CalibratedProc(void)
{
    WORD wVersionRequested; 
    WSADATA wsaData;
    int err;
  
    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0)
    {
        return 0;
    }
  
    if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) 
    {
        WSACleanup();
        return 0;
    }

	char hostname[256];
	struct hostent *phost;
	gethostname(hostname, 256);
	phost = gethostbyname(hostname);

	// Create address information
    SOCKADDR_IN hostAddr;
	hostAddr.sin_addr = *(in_addr*)phost->h_addr_list[0];
    hostAddr.sin_family = AF_INET;
    hostAddr.sin_port = htons(DEFAULT_PORT);

	// Connect to server
	int nResult = 0;

	// Create connection socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

	nResult = connect(sock, (sockaddr*)&hostAddr, sizeof(sockaddr));
	if(nResult)
	{
		closesocket(sock);
		return 0;
	}

	send(sock, CONTINUE, strlen(CONTINUE)+1, 0);

	char rev[DEFAULT_BUFLEN];
	ZeroMemory(rev, sizeof(rev));
	nResult = recv(sock, rev, DEFAULT_BUFLEN, 0);
	if(nResult > 0)
	{
		if(strcmp(rev, START_COLOR_PATCH))
		{
			closesocket(sock);
			return 0;
		}
		else
		{
			ResetEvent(this->m_hPatchEvent);
			this->m_bCaptureFrame = TRUE;
		}
	}
	else
	{
		closesocket(sock);
		return 0;
	}

	// Wait until receive a 3xN RGB matrix and patch number from server
	char revBuf[DEFAULT_BUFLEN];
	ZeroMemory(revBuf, sizeof(revBuf));
	nResult = recv(sock, revBuf, DEFAULT_BUFLEN, 0);
	if(nResult > 0)
	{
		send(sock, FINISH_RECEIVE, strlen(FINISH_RECEIVE), 0);

		int nPatchNum = nResult/3;
		RGBTRIPLE *prgbBuffer = new RGBTRIPLE[nPatchNum];
		int nBufferIndex = 0;
		for(int i = 0; i < nPatchNum; ++i)
		{
			prgbBuffer[i].rgbtRed	= revBuf[nBufferIndex++];
			prgbBuffer[i].rgbtGreen = revBuf[nBufferIndex++];
			prgbBuffer[i].rgbtBlue	= revBuf[nBufferIndex++];
			if(nBufferIndex >= DEFAULT_BUFLEN)
				break;
		}
		if(this->m_pCalibrate)
		{
			WaitForSingleObject(this->m_hPatchEvent, INFINITE);
			if(this->m_nColorChartNum == nPatchNum)
			{
				this->m_bColorChartResult = TRUE;
				this->m_pCalibrate->CalculateCCMatrix(prgbBuffer, this->m_pPatchRGB);
			}
			else
			{
				this->m_bColorChartResult = FALSE;
			}
		}

		SAFE_DELETE_ARRAY(prgbBuffer);
	}
		
	closesocket(sock);

	WSACleanup();

	SetEvent(this->m_hCalibratedEvent);

	return 0;
} // CalibratedProc


//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);


BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved) 
{  
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved); 
}


STDAPI DllRegisterServer() 
{      
	return AMovieDllRegisterServer2( TRUE ); 
}


STDAPI DllUnregisterServer() 
{       
	return AMovieDllRegisterServer2( FALSE );
}


//----------------------------------------------------------------------------
// GetVideoInfoParameters
//
// Helper function to get the important information out of a VIDEOINFOHEADER
//
//-----------------------------------------------------------------------------

void GetVideoInfoParameters(
    const VIDEOINFOHEADER *pvih,	// Pointer to the format header.
    BYTE  * const pbData,			// Pointer to the first address in the buffer.
    DWORD *pdwWidth,				// Returns the width in pixels.
    DWORD *pdwHeight,				// Returns the height in pixels.
    LONG  *plStrideInBytes,			// Add this to a row to get the new row down
    BYTE **ppbTop,					// Returns pointer to the first byte in the top row of pixels.
    bool bYuv
    )
{
    LONG lStride;
   
    //  For 'normal' formats, biWidth is in pixels.    
    //  Expand to bytes and round up to a multiple of 4.   
    if (pvih->bmiHeader.biBitCount != 0 &&   
        0 == (7 & pvih->bmiHeader.biBitCount))    
    {   
        lStride = (pvih->bmiHeader.biWidth * (pvih->bmiHeader.biBitCount / 8) + 3) & ~3;   
    }    
    else   // Otherwise, biWidth is in bytes.   
    {   
        lStride = pvih->bmiHeader.biWidth;   
    }   
   
    //  If rcTarget is empty, use the whole image.   
    if (IsRectEmpty(&pvih->rcTarget))    
    {   
        *pdwWidth = (DWORD)pvih->bmiHeader.biWidth;   
        *pdwHeight = (DWORD)(abs(pvih->bmiHeader.biHeight));   
           
        if (pvih->bmiHeader.biHeight < 0 || bYuv)   // Top-down bitmap.    
        {   
            *plStrideInBytes = lStride; // Stride goes "down"   
            *ppbTop           = pbData; // Top row is first.   
        }    
        else        // Bottom-up bitmap   
        {   
            *plStrideInBytes = -lStride;    // Stride goes "up"   
            *ppbTop = pbData + lStride * (*pdwHeight - 1);  // Bottom row is first.   
        }   
    }    
    else   // rcTarget is NOT empty. Use a sub-rectangle in the image.   
    {   
        *pdwWidth = (DWORD)(pvih->rcTarget.right - pvih->rcTarget.left);   
        *pdwHeight = (DWORD)(pvih->rcTarget.bottom - pvih->rcTarget.top);   
           
        if (pvih->bmiHeader.biHeight < 0 || bYuv)   // Top-down bitmap.   
        {   
            // Same stride as above, but first pixel is modified down   
            // and and over by the target rectangle.   
            *plStrideInBytes = lStride;        
            *ppbTop = pbData +   
                     lStride * pvih->rcTarget.top +   
                     (pvih->bmiHeader.biBitCount * pvih->rcTarget.left) / 8;   
        }    
        else  // Bottom-up bitmap.   
        {   
            *plStrideInBytes = -lStride;   
            *ppbTop = pbData +   
                     lStride * (pvih->bmiHeader.biHeight - pvih->rcTarget.top - 1) +   
                     (pvih->bmiHeader.biBitCount * pvih->rcTarget.left) / 8;   
        }   
    }
}


bool IsValidUYVY(const CMediaType *pmt)
{   
    // Note: The pmt->formattype member indicates what kind of data   
    // structure is contained in pmt->pbFormat. But it's important   
    // to check that pbFormat is non-NULL and the size (cbFormat) is   
    // what we think it is.    
   
    if ((pmt->majortype		== MEDIATYPE_Video)		&&   
        (pmt->subtype		== MEDIASUBTYPE_UYVY)	&&   
        (pmt->formattype	== FORMAT_VideoInfo)	&&   
        (pmt->pbFormat		!= NULL)				&&   
        (pmt->cbFormat		>= sizeof(VIDEOINFOHEADER)))   
    {   
        VIDEOINFOHEADER *pVih	= reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);   
        BITMAPINFOHEADER *pBmi	= &(pVih->bmiHeader);   
   
        // Sanity check   
        if ((pBmi->biBitCount		== 16)			&&   
            (pBmi->biCompression	== FCC('UYVY')) &&   
            (pBmi->biSizeImage		>= DIBSIZE(*pBmi)))   
        {   
            return true;   
        }   
   
        // Note: The DIBSIZE macro calculates the real size of the bitmap,   
        // taking DWORD alignment into account. For YUV formats, this works   
        // only when the bitdepth is an even power of 2, not for all YUV types.   
    }   
   
    return false;
}

   
bool IsValidYUY2(const CMediaType *pmt)   
{   
    // Note: The pmt->formattype member indicates what kind of data   
    // structure is contained in pmt->pbFormat. But it's important   
    // to check that pbFormat is non-NULL and the size (cbFormat) is   
    // what we think it is.    
   
    if ((pmt->majortype		== MEDIATYPE_Video)		&&   
        (pmt->subtype		== MEDIASUBTYPE_YUY2)	&&   
        (pmt->formattype	== FORMAT_VideoInfo)	&&   
        (pmt->pbFormat		!= NULL)				&&   
        (pmt->cbFormat		>= sizeof(VIDEOINFOHEADER)))   
    {   
        VIDEOINFOHEADER *pVih	= reinterpret_cast<VIDEOINFOHEADER*>(pmt->pbFormat);   
        BITMAPINFOHEADER *pBmi	= &(pVih->bmiHeader);   
   
        // Sanity check   
        if ((pBmi->biBitCount		== 16)			&&   
            (pBmi->biCompression	== FCC('YUY2')) &&   
            (pBmi->biSizeImage		>= DIBSIZE(*pBmi)))
        {   
            return true;   
        }
   
        // Note: The DIBSIZE macro calculates the real size of the bitmap,   
        // taking DWORD alignment into account. For YUV formats, this works   
        // only when the bitdepth is an even power of 2, not for all YUV types.   
    }   
   
    return false;
}
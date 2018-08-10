#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <tchar.h>
#include <dshow.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
//#include <atlbase.h>
//#include <string.h>
#include <stdlib.h>
#include <Dvdmedia.h>
#include <Mmreg.h>

#include "live.h"
#include "iRtspStream.h"
#include "RtspStreamFilter.h"
#include "RtspStreamProp.h"

#define STANDARD_CAMERA		L"rtsp://Admin:123456@172.16.8.82:7070/stream1"

#pragma warning(disable:4710)  // 'function': function not inlined (optimzation)

DEFINE_GUID(MEDIASUBTYPE_H264SUB ,
			0x31435641, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

//7634706D-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_mpg4SUB ,
			0x7634706D, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

//33363268-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_h263SUB ,
			0x33363268, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

//32564D57-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_WMVSUB ,
			0x32564D57, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

//000000FF-0000-0010-8000-00AA00389B71
DEFINE_GUID(MEDIASUBTYPE_AAC ,
			0x000000FF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// Setup data

const AMOVIESETUP_MEDIATYPE videoPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_MEDIATYPE audioPinTypes =
{
	&MEDIATYPE_Video,       // Major type
	&MEDIASUBTYPE_NULL      // Minor type
};
const AMOVIESETUP_PIN sudOpPin[] =
{
	{
		L"Output",              // Pin string name
		FALSE,                  // Is it rendered
		TRUE,                   // Is it an output
		FALSE,                  // Can we have none
		FALSE,                  // Can we have many
		&CLSID_NULL,            // Connects to filter
		NULL,                   // Connects to pin
		1,                      // Number of types
		&videoPinTypes 
	},
	{
		L"Output",              // Pin string name
		FALSE,                  // Is it rendered
		TRUE,                   // Is it an output
		FALSE,                  // Can we have none
		FALSE,                  // Can we have many
		&CLSID_NULL,            // Connects to filter
		NULL,                   // Connects to pin
		1,                      // Number of types
		&audioPinTypes 
	}

};  

const AMOVIESETUP_FILTER rtspfilterax =
{
    &CLSID_DeltaRtspSourceFilter,    // Filter CLSID
    L"Delta Rtsp Source Filter",       // String name
    MERIT_DO_NOT_USE,       // Filter merit
    2,                      // Number pins
    sudOpPin               // Pin details
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = 
{
  { 
		L"Delta Rtsp Source Filter",
		&CLSID_DeltaRtspSourceFilter,
		CRTSPStreamFilter::CreateInstance,
		NULL,
		&rtspfilterax 
  },
  { 
        L"Rtsp Stream Props",
        &CLSID_RtspUrlProp,
        CRtspStreamProp::CreateInstance, 
        NULL, 
		NULL
  }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
} // DllUnregisterServer


//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

//
// CreateInstance
//
// The only allowed way to create Bouncing balls!
//
CUnknown * WINAPI CRTSPStreamFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);

    CUnknown *punk = new CRTSPStreamFilter(lpunk, phr);
    if(punk == NULL)
    {
        if(phr)
            *phr = E_OUTOFMEMORY;
    }

    return punk;
} // CreateInstance

CRTSPStreamFilter::~CRTSPStreamFilter()
{
	if(m_pStreamMedia)
	{
		//m_pStreamMedia->rtspClientStopStream();
		delete m_pStreamMedia;
	}
	if (videoOutPin)
	{
		delete videoOutPin;
		videoOutPin = NULL;			 
	}
	if (audioOutPin)
	{
		delete audioOutPin;
		audioOutPin = NULL;
	}
	m_nPins = 0;
} // (Destructor)

//
// Constructor
//
// Initialise a CBallStream object so that we have a pin.
//
CRTSPStreamFilter::CRTSPStreamFilter(LPUNKNOWN lpunk, HRESULT *phr) 
: CBaseFilter(NAME("Delta Rtsp Source Filter"), lpunk, &m_cStateLock, CLSID_DeltaRtspSourceFilter)
, m_nPins(0)
, m_nWidth(1280)
, m_nHeight(720)
{
    ASSERT(phr);
    CAutoLock cAutoLock(&m_cStateLock);	
	
	videoOutPin = NULL;
	audioOutPin = NULL;

	m_bIsSeeking = FALSE;
	m_rtStart = 0;
	m_rtDuration = 0;
	m_pStreamMedia = new CStreamMedia;
	if(m_pStreamMedia == NULL)
	{
		if(phr)
			*phr = E_OUTOFMEMORY;
	}
	m_RtspState = StateStopped;
	ZeroMemory(m_wszFileName, sizeof(m_wszFileName));
} // (Constructor)

STDMETHODIMP CRTSPStreamFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IFileSourceFilter)
	{
		return GetInterface(static_cast<IFileSourceFilter*>(this), ppv);
	}
	else if (riid == IID_IRtspStreaming)
    {
        return GetInterface(static_cast<IRtspStreaming*>(this), ppv);
    }
	else if (riid == IID_ISpecifyPropertyPages)
    {
        return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
    }
	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CRTSPStreamFilter::GetCurFile(LPOLESTR* outFileName, AM_MEDIA_TYPE* outMediaType)
{
	if(*m_wszFileName)
	{
		LPOLESTR lpFileName = SysAllocString((const OLECHAR *)m_wszFileName);
		*outFileName = lpFileName;

		return S_OK;
	}
	else
	{
		wcscpy_s(m_wszFileName, STANDARD_CAMERA);
		LPOLESTR lpFileName = SysAllocString((const OLECHAR *)m_wszFileName);
		*outFileName = lpFileName;

		outMediaType = (AM_MEDIA_TYPE*)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
		if(outMediaType)
			memset(outMediaType, 0, sizeof(AM_MEDIA_TYPE));

		return E_FAIL;
	}
}

STDMETHODIMP CRTSPStreamFilter::Load(LPCOLESTR inFileName, const AM_MEDIA_TYPE* inMediaType)
{
	wcscpy_s(m_wszFileName, inFileName);

	HRESULT hr, *phr = &hr;
	hr = S_OK;
	//if (m_wszFileName[0] != 0 && m_RtspState == StateStopped)
	{	
		char filename[1024] = {0};		
		WideCharToMultiByte (CP_OEMCP, NULL, m_wszFileName, -1, (LPSTR)filename, 1024, NULL, FALSE);
		int ret = m_pStreamMedia->rtspClientOpenStream((const char *)filename, 0);
		if (ret < 0)
		{
			return E_FAIL;
		}
		m_RtspState = StateOpened;
		SetPin();
	}
	
	return S_OK;
}

STDMETHODIMP CRTSPStreamFilter::GetPages(CAUUID *pPages)
{
	if (pPages == NULL) 
		return E_POINTER;

    pPages->cElems = 1;
    pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pPages->pElems[0] = CLSID_RtspUrlProp;

    return S_OK;
}

STDMETHODIMP CRTSPStreamFilter::SetStreamURL(WCHAR* pwszURL)
{
	CheckPointer(pwszURL, E_POINTER);

	CAutoLock lock(&m_csSharedData);

	return Load((LPCTSTR)pwszURL, NULL);
}

STDMETHODIMP CRTSPStreamFilter::GetStreamURL(WCHAR* pwszURL)
{
	CheckPointer(pwszURL, E_POINTER);

	CAutoLock lock(&m_csSharedData);

	wcscpy_s(pwszURL, sizeof(m_wszFileName)/sizeof(wchar_t), m_wszFileName);

	return S_OK;
}

STDMETHODIMP CRTSPStreamFilter::SetResolution(int nWidth, int nHeight)
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;

	return S_OK;
}

STDMETHODIMP CRTSPStreamFilter::SetAccount(WCHAR* pwszAccount)
{
	CheckPointer(pwszAccount, E_POINTER);
	CheckPointer(m_pStreamMedia, E_POINTER);

	m_pStreamMedia->SetAccount(pwszAccount);

	return S_OK;
}

STDMETHODIMP CRTSPStreamFilter::SetPassword(WCHAR* pwszPass)
{
	CheckPointer(pwszPass, E_POINTER);
	CheckPointer(m_pStreamMedia, E_POINTER);

	m_pStreamMedia->SetPassword(pwszPass);

	return S_OK;
}

HRESULT CRTSPStreamFilter::SetPin()
{
	HRESULT hr, *phr = &hr;
	int stream_nb = m_pStreamMedia->m_nStream;
	int cur_stream = m_pStreamMedia->m_nCurValidStream;
	int stream_num = 0;
	hr = S_OK;
	if(stream_nb > 0)
	{		
		if ((cur_stream & (0x1<<CODEC_TYPE_VIDEO)))
		{
			if (videoOutPin)
			{
				delete videoOutPin;
				videoOutPin = NULL;
			}
			videoOutPin = new CRTSPStreamOutputPin(CODEC_TYPE_VIDEO, 34, phr, this, L"Video Pin");

			if(videoOutPin == NULL)
			{
				if(phr)
					*phr = E_OUTOFMEMORY;

				return hr;
			}
			++stream_num;
		}
		if ((cur_stream & (0x1<<CODEC_TYPE_AUDIO)))
		{
			if (audioOutPin)
			{
				delete audioOutPin;
				audioOutPin = NULL;
			}
			audioOutPin = new CRTSPStreamOutputPin(CODEC_TYPE_AUDIO, 20, phr, this, L"Audio Pin");

			if(audioOutPin == NULL)
			{
				if(phr)
					*phr = E_OUTOFMEMORY;

				return hr;
			}
			++stream_num;
		}
		m_nPins = stream_num;

		return hr;
	}

	return S_FALSE;
}

int CRTSPStreamFilter::GetPinCount(void)
{
	CAutoLock lock(&m_cStateLock);

	return m_nPins;
}

CBasePin *CRTSPStreamFilter::GetPin(int n)
{
	CAutoLock lock(&m_cStateLock);

	if (n < 0 || n >= 2)
		return NULL;
	
	if (n == 0)
	{
		if (videoOutPin == NULL)
		{
			return audioOutPin;
		}
		return videoOutPin;
	}
	if (n == 1)
	{
		return audioOutPin;
	}

	return NULL;
}

STDMETHODIMP  CRTSPStreamFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{ 
	CheckPointer(State, E_POINTER);

	*State = m_State;
	if(m_State==State_Paused)
		return VFW_S_CANT_CUE;
	else
		return S_OK;
} 

STDMETHODIMP CRTSPStreamFilter::Run(REFERENCE_TIME tStart)
{
	//HRESULT hr;
	int ret;
	CAutoLock cAutoLock(&m_cStateLock);

	if (m_RtspState == StatePaused)
	{
		if (m_rtStart > 0)
		{
			Seek(m_rtStart);
			m_RtspState = StateRunning;
			m_rtStart = 0;			
			m_bIsSeeking = TRUE;
			err("run seek ing \n");
		}
		else
		{		
			ret = m_pStreamMedia->rtspClientPlayStream();
			if (ret >= 0)
			{
				m_RtspState = StateRunning;
			}
		}
	}

	return CBaseFilter::Run(tStart);
}

STDMETHODIMP CRTSPStreamFilter::Stop(void)
{
	HRESULT hr;
	CAutoLock cAutoLock(&m_cStateLock);	
	m_RtspState = StateStopped;

	err("CRTSPStreamFilter::Stop\n");
	if (ThreadExists()) 
	{
		hr = ThreadStop();

		if (FAILED(hr)) 
		{
			return hr;
		}

		hr = ThreadExit();
		if (FAILED(hr)) 
		{
			return hr;
		}
		err("begin Thread Close \n");
		Close();	// Wait for the thread to exit, then tidy up.
	}
	err("begin rtspClientStopStream \n");
	if (m_pStreamMedia->rtspClientStopStream() < 0)
		;//return E_FAIL;
	err("end rtspClientStopStream \n");
	return CBaseFilter::Stop();
}

STDMETHODIMP CRTSPStreamFilter::Pause(void)
{
	HRESULT hr;
	int ret;
	char filename[1024];
    CAutoLock cAutoLock(&m_cStateLock);	

	do {
		if (m_RtspState == StateStopped)
		{
			WideCharToMultiByte (CP_OEMCP, NULL, m_wszFileName, -1, (LPSTR)filename, 1024, NULL, FALSE);
			ret = m_pStreamMedia->rtspClientOpenStream((const char *)filename, 0);
			if (ret < 0)
			{
				break;//return E_FAIL;
			}
			m_RtspState = StateOpened;
			/*SetPin();*/
			ret = m_pStreamMedia->rtspClientPlayStream();
			if (ret < 0)
			{
				break;//return E_FAIL;
			}
			m_RtspState = StateRunning;
			//return S_OK;
		}
		else if (m_RtspState == StateRunning)
		{
			m_RtspState = StatePaused;	//for test 
			if (m_pStreamMedia->rtspClientPauseStream() < 0)
			{
				err("!!!!!!!!!!!rtspClientPauseStream !!!!!!!!!!!\n");
				break;//return S_FALSE;
			}
			else
			{
				m_RtspState = StatePaused;	
				break;//return S_OK;
			}
		}	
		else if (m_RtspState == StateOpened)
		{
			ret = m_pStreamMedia->rtspClientPlayStream();
			if (ret < 0)
			{
				err("rtspClientPlayStream fail\n");
	/*			return S_FALSE;*/
			}
			m_RtspState = StateRunning;
		}
	} while(0);
	//if (m_State == State_Running)
	//{
	//	Stop();
	//}
	if (m_State == State_Stopped)
	{
		int cPins = GetPinCount();
		for (int c = 0; c < cPins; c++) 
		{
			CBasePin *pPin = GetPin(c);

			// Disconnected pins are not activated - this saves pins
			// worrying about this state themselves

			if (pPin->IsConnected()) 
			{
				hr = pPin->Active();
				if (FAILED(hr)) 
				{
					return hr;
				}
			}			
		}

		ASSERT(!ThreadExists());

		// start the thread
		if (!Create()) 
		{
			return E_FAIL;
		}

		// Tell thread to initialize. If OnThreadCreate Fails, so does this.
		hr = ThreadInit();
		if (FAILED(hr))
			return hr;

		hr = ThreadPause();
		if (FAILED(hr))
			return hr;
	}

	CAutoLock cObjectLock(m_pLock);
	m_State = State_Paused;

	return S_OK;
}

//
// ThreadProc
//
// When this returns the thread exits
// Return codes > 0 indicate an error occured
DWORD CRTSPStreamFilter::ThreadProc(void) 
{

	HRESULT hr;  // the return code from calls
	Command com;

	do {
		com = GetRequest();
		if (com != CMD_INIT) 
		{
			DbgLog((LOG_ERROR, 1, TEXT("Thread expected init command")));
			Reply((DWORD) E_UNEXPECTED);
		}
	} while (com != CMD_INIT);

	DbgLog((LOG_TRACE, 1, TEXT("CSourceStream worker thread initializing")));

	hr = OnThreadCreate(); // perform set up tasks
	if (FAILED(hr)) 
	{
		DbgLog((LOG_ERROR, 1, TEXT("CSourceStream::OnThreadCreate failed. Aborting thread.")));
		OnThreadDestroy();
		Reply(hr);	// send failed return code from OnThreadCreate
		return 1;
	}

	// Initialisation suceeded
	Reply(NOERROR);

	Command cmd;
	do {
		cmd = GetRequest();

		switch (cmd) 
		{
		case CMD_EXIT:
			Reply(NOERROR);
			break;
		case CMD_RUN:
			DbgLog((LOG_ERROR, 1, TEXT("CMD_RUN received before a CMD_PAUSE???")));
			// !!! fall through???

		case CMD_PAUSE:
			Reply(NOERROR);
			DoBufferProcessingLoop();
			break;
		case CMD_STOP:
			Reply(NOERROR);
			break;
		default:
			DbgLog((LOG_ERROR, 1, TEXT("Unknown command %d received!"), cmd));
			Reply((DWORD) E_NOTIMPL);
			break;
		}
	} while (cmd != CMD_EXIT);

	hr = OnThreadDestroy();	// tidy up.
	if (FAILED(hr)) 
	{
		DbgLog((LOG_ERROR, 1, TEXT("CSourceStream::OnThreadDestroy failed. Exiting thread.")));
		return 1;
	}

	DbgLog((LOG_TRACE, 1, TEXT("CSourceStream worker thread exiting")));
	return 0;
}

HRESULT CRTSPStreamFilter::FillBuffer(IMediaSample **pSamp)
{
	FrameInfo* frame = NULL;
	int len;
	LONG timeStamp = 0;	
	CRefTime rtStart, rtStop;
	IMediaSample *pms;

	BYTE *pData;
	long lDataLen;
	HRESULT hr;

	CAutoLock cAutoLockShared(&m_cSharedState);
	if (m_RtspState == State_Stopped)
	{
		return S_FALSE;
	}
	len = m_pStreamMedia->rtspClientReadFrame(frame);
	if (len < 0)
	{	
		//pms->SetActualDataLength(0);		
		if (m_bIsSeeking || m_RtspState ==  State_Paused)
		{
			if(m_bIsSeeking)
				m_bIsSeeking = FALSE;
			err("fillbufer m_RtspState == StatePaused || m_RtspState == State_Stopped\n");
			return E_SEEK;
		}
		m_pStreamMedia->rtspClientCloseStream();
		//if (m_pStreamMedia->rtspClientm_nNoStream())
		//{
		//	err("video rtspClientm_nNoStream\n");
		//	return S_FALSE;
		//}
		return S_FALSE;
	}

	if (frame && frame->pdata)
	{
		if (frame->frameHead.FrameType == CODEC_TYPE_VIDEO)
		{
			pcurOutPin = videoOutPin;
		}
		else if (frame->frameHead.FrameType == CODEC_TYPE_AUDIO)
		{
			pcurOutPin = audioOutPin;
		}
		else
		{
			err("err stream frame type \n");
			return S_FALSE;
		}
		hr = pcurOutPin->GetDeliveryBuffer(pSamp, NULL, NULL, 0);
		if (FAILED(hr)) 
		{
			err("GetDeliveryBuffer fail\n");
			return E_POINTER;
		}
		pms = *pSamp;
		pms->GetPointer(&pData);
		lDataLen = pms->GetSize();
		if (pData)
		{
			if(lDataLen < len)
				len = lDataLen;
			memcpy((char *)pData, frame->pdata, len);
		}	
	}
	else
	{
		err("frame is Null \n");
		return S_FALSE;
	}

#if 1	
	if (frame->frameHead.FrameType == CODEC_TYPE_AUDIO)
	{
		timeStamp = (frame->frameHead.TimeStamp - audioOutPin->m_lLast_timeStamp);
		audioOutPin->m_lLast_timeStamp = frame->frameHead.TimeStamp;
		/*pcurOutPin->m_rtSampleTime -= m_rtStart;*/
		rtStart = audioOutPin->m_rtStartSampleTime = audioOutPin->m_rtSampleTime;
		if (timeStamp < 0 || timeStamp >= 250)
		{
			audioOutPin->m_rtSampleTime += audioOutPin->m_lDefaultDealt;
			timeStamp = audioOutPin->m_lDefaultDealt;
		}
		else
		{
			audioOutPin->m_rtSampleTime += timeStamp;
		}			
	}
	else
	{
		if (audioOutPin && audioOutPin->IsConnected())
		{
			timeStamp = (frame->frameHead.TimeStamp - audioOutPin->m_lLast_timeStamp);
			if (timeStamp < 0 || timeStamp >= 250)
			{
				timeStamp = videoOutPin->m_lDefaultDealt;
			}					
			rtStart = videoOutPin->m_rtSampleTime;
			videoOutPin->m_rtSampleTime = audioOutPin->m_rtSampleTime + timeStamp * 10000;
		}
		else
		{
			timeStamp = (frame->frameHead.TimeStamp - videoOutPin->m_lLast_timeStamp);
			videoOutPin->m_lLast_timeStamp = frame->frameHead.TimeStamp;
			/*pcurOutPin->m_rtSampleTime -= m_rtStart;*/
			m_rtStart = videoOutPin->m_rtSampleTime;
			if (timeStamp < 0 || timeStamp >= 250)
			{
				videoOutPin->m_rtSampleTime += videoOutPin->m_lDefaultDealt;
				timeStamp = videoOutPin->m_lDefaultDealt;
			}
			else
			{
				videoOutPin->m_rtSampleTime += timeStamp;
			}	
		}
	}

#else
	// The current time is the sample's start
	timeStamp = (frame->frameHead.TimeStamp - pcurOutPin->m_lLast_timeStamp);
	pcurOutPin->m_lLast_timeStamp = frame->frameHead.TimeStamp;		
	/*pcurOutPin->m_rtSampleTime -= m_rtStart;*/
	rtStart = pcurOutPin->m_rtSampleTime;
	// Increment to find the finish time


	//err("###video len = %d frame->frameHead.TimeStamp = %d timeStamp = %d m_rtSampleTime = %lld\n", len, frame->frameHead.TimeStamp, timeStamp,pcurOutPin->m_rtSampleTime);

	if (timeStamp < 0 || timeStamp >= 100)
	{
		pcurOutPin->m_rtSampleTime += pcurOutPin->m_lDefaultDealt;
	}
	else
	{
		pcurOutPin->m_rtSampleTime += timeStamp;
	}
#endif

	err("###%s len = %d frame->frameHead.TimeStamp = %d timeStamp = %d m_rtSampleTime = %lld\n", (frame->frameHead.FrameType == CODEC_TYPE_VIDEO) ? "video" : "audio", len, frame->frameHead.TimeStamp, timeStamp,pcurOutPin->m_rtSampleTime/10000);

	if (frame && frame->pdata)
	{		
		free(frame);
	}

	//pms->SetTime((REFERENCE_TIME *) &rtStart,(REFERENCE_TIME *) &(pcurOutPin->m_rtSampleTime));
	rtStop = pcurOutPin->m_rtSampleTime + timeStamp;
	pms->SetTime((REFERENCE_TIME *) &(pcurOutPin->m_rtSampleTime), (REFERENCE_TIME *) &rtStop);
	//pms->SetTime(NULL, NULL);
	pms->SetActualDataLength(len);
	pms->SetSyncPoint(TRUE);

	if (m_bIsSeeking)
	{
		m_bIsSeeking = FALSE;
	}
	
	return NOERROR;
}

HRESULT CRTSPStreamFilter::DoBufferProcessingLoop(void)
{
	Command com;

	OnThreadStartPlay();

	do {
		while (!CheckRequest(&com)) 
		{
			IMediaSample *pSample = NULL;
			HRESULT hr;
			
			// Virtual function user will override.
			hr = FillBuffer(&pSample);
			if (hr == E_POINTER || hr == E_SEEK)
			{
				Sleep(1);
				continue;	// go round again. Perhaps the error will go away
			}

			if (hr == S_OK && pSample != NULL) 
			{
				if (pcurOutPin->m_bDiscontinuity)
				{
					pcurOutPin->m_bDiscontinuity = FALSE;
					pSample->SetDiscontinuity(TRUE);
				}				
				if (pSample != NULL)
				{
					hr = pcurOutPin->Deliver(pSample);
					pSample->Release();
				}

				// downstream filter returns S_FALSE if it wants us to
				// stop or an error if it's reporting an error.
				if(hr != S_OK)
				{
					DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));
					return S_OK;
				}
			} 
			else if (hr == S_FALSE) 
			{
				// derived class wants us to stop pushing data
				if (pSample)
				{
					pSample->Release();
				}
				if (videoOutPin)
				{
					videoOutPin->DeliverEndOfStream();
				}
				if (audioOutPin)
				{
					audioOutPin->DeliverEndOfStream();
				}
				
				return S_OK;
			} 
			else 
			{
				// derived class encountered an error
				if (pSample)
				{
					pSample->Release();
				}
				DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
				if (videoOutPin)
				{
					videoOutPin->DeliverEndOfStream();
				}
				if (audioOutPin)
				{
					audioOutPin->DeliverEndOfStream();
				}
				NotifyEvent(EC_ERRORABORT, hr, 0);
				return hr;
			}
			// all paths release the sample
		}

		// For all commands sent to us there must be a Reply call!
		if (com == CMD_RUN || com == CMD_PAUSE) 
		{
			Reply(NOERROR);
		} 
		else if (com != CMD_STOP) 
		{
			Reply((DWORD) E_UNEXPECTED);
			DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
		}
	} while (com != CMD_STOP);

	return S_FALSE;
}

STDMETHODIMP CRTSPStreamFilter::FindPin(LPCWSTR Id, IPin **ppPin)
{
	CheckPointer(ppPin, E_POINTER);
	ValidateReadWritePtr(ppPin, sizeof(IPin *));
	if (m_nPins == 0)
	{
		return VFW_E_NOT_FOUND;
	}
	// The -1 undoes the +1 in QueryId and ensures that totally invalid
	// strings (for which WstrToInt delivers 0) give a deliver a NULL pin.
	int i = WstrToInt(Id) -1;
	*ppPin = GetPin(i);
	if (*ppPin != NULL)
	{
		(*ppPin)->AddRef();
		return NOERROR;
	} 
	else 
	{
		return VFW_E_NOT_FOUND;
	}
}

HRESULT CRTSPStreamFilter::OnThreadCreate(void) 
{
	if (videoOutPin)
	{
		videoOutPin->m_rtSampleTime = 0;
	}
	if (audioOutPin)
	{
		audioOutPin->m_rtSampleTime = 0;
	}
	return NOERROR;
}

HRESULT CRTSPStreamFilter::OnThreadStartPlay(void)
{
	HRESULT hr = S_OK;

	if (videoOutPin && videoOutPin->IsConnected())
	{
		hr = videoOutPin->DeliverNewSegment(videoOutPin->m_rtStart, videoOutPin->m_rtStop, videoOutPin->m_dRateSeeking);
		videoOutPin->m_bDiscontinuity = TRUE;
	}
	if (audioOutPin && audioOutPin->IsConnected())
	{
		hr = audioOutPin->DeliverNewSegment(audioOutPin->m_rtStart, audioOutPin->m_rtStop, audioOutPin->m_dRateSeeking);
		audioOutPin->m_bDiscontinuity = TRUE;
	}
	
	return hr;
}

int CRTSPStreamFilter::Seek(REFERENCE_TIME rtStart)
{
	long timeStamp = (long)(rtStart/1E7);

	if (m_pStreamMedia->rtspClientSeekStream(timeStamp, 0) < 0)
	{
		err("rtspClientSeekStream fail \n");
		return(-1);
	}
	
	if (ThreadExists()) 
	{
		if (videoOutPin)
		{
			videoOutPin->m_rtSampleTime = 0;
			videoOutPin->m_bDiscontinuity  = TRUE;
			videoOutPin->DeliverBeginFlush();
		}
		if (audioOutPin)
		{
			audioOutPin->m_rtSampleTime = 0;
			audioOutPin->m_bDiscontinuity  = TRUE;
			audioOutPin->DeliverBeginFlush();
		}
		
		ThreadStop();
		if (videoOutPin)
		{
			videoOutPin->DeliverEndFlush();
		}
		if (audioOutPin)
		{
			audioOutPin->DeliverEndFlush();
		}
		
		ThreadRun();
	}

	return(0);
}


//
// Constructor
//
CRTSPStreamOutputPin::CRTSPStreamOutputPin(CodecType codeType, long defaultDealt, HRESULT *phr, CRTSPStreamFilter *pParent,LPCWSTR pPinName)
: CBaseOutputPin(NAME("Delta Rtsp Source Filter"), pParent, pParent->pStateLock(), phr, pPinName)
, CSourceSeeking(NAME("Delta Rtsp Source Filter"), pParent->GetOwner(), phr, CBasePin::m_pLock)
, m_pFilter(pParent)
, m_codecType(codeType)
, m_lDefaultDealt(defaultDealt)
{
	 m_rtSampleTime = 0;
	 m_lLast_timeStamp = 0;
	 // m_rtStart = 0;
	 m_rtStop  = 0;
	 m_bDiscontinuity = TRUE;
	 m_dRateSeeking = 1.0;
	 m_bIsSeek = FALSE;
}

HRESULT CRTSPStreamOutputPin::CheckMediaType(const CMediaType *pMediaType) 
{
	CMediaType mt;
	CAutoLock lock(m_pFilter->pStateLock());
	
	GetMediaType(0, &mt);
	if (pMediaType->majortype == mt.majortype && pMediaType->subtype == mt.subtype)
	{
		return NOERROR;
	}    
	
	return E_FAIL;
}

HRESULT CRTSPStreamOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
	GUID videoSubType;
	int ret;
	GUID audioSubType;
	PBYTE pdata;
	WORD FormatTag = 0;
	WAVEFORMATEX WaveInfo;
	WAVEFORMATEX *pmWaveInfo = &WaveInfo; 
	MPEG1WAVEFORMAT mpga;
	char *ptr;

	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());
	if(iPosition < 0)
	{
		return E_INVALIDARG;
	}

	// Have we run off the end of types?

	if(iPosition >= 1)
	{
		return VFW_S_NO_MORE_ITEMS;
	}
	if (this->m_codecType == CODEC_TYPE_VIDEO)
	{
		ret = ((CRTSPStreamFilter *)m_pFilter)->m_pStreamMedia->rtspClinetGetMediaInfo(CODEC_TYPE_VIDEO, m_videoMediaInfo);
		if(ret < 0)
		{	
			return VFW_S_NO_MORE_ITEMS;
		}
		switch(m_videoMediaInfo.i_format)
		{
		case FOURCC( 'H', '2', '6', '3' ):
			{
				memcpy(&videoSubType, &MEDIASUBTYPE_h263SUB, sizeof(GUID));
			}
			break;
		case FOURCC( 'm', 'p', 'g', 'v' ):
			{
				memcpy(&videoSubType, &MEDIASUBTYPE_MPEG2_VIDEO, sizeof(GUID));
			}
			break;
		case FOURCC( 'H', '2', '6', '1' ):
			break;
		case FOURCC( 'h', '2', '6', '4' ):
			{
				memcpy(&videoSubType, &MEDIASUBTYPE_H264, sizeof(GUID));
			}
			break;
		case FOURCC( 'm', 'p', '4', 'v' ):
			{		
				memcpy(&videoSubType, &MEDIASUBTYPE_mpg4SUB, sizeof(GUID));
			}
			break;
		case FOURCC( 'M', 'J', 'P', 'G' ):
			break;
		default:
			videoSubType =  GUID_NULL;
			break;
		}

		VIDEOINFOHEADER2 *pvid;
		PBYTE pBuffer = pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2) + m_videoMediaInfo.extra_size);
		if(NULL == pBuffer)
			return(E_OUTOFMEMORY);

		pmt->SetFormatType(&FORMAT_VideoInfo2);

		pvid = (VIDEOINFOHEADER2 *)pBuffer;
		ZeroMemory(pvid, sizeof(VIDEOINFOHEADER2));
		pvid->rcSource.left = 0;
		pvid->rcSource.top  = 0;
		pvid->rcSource.right  = m_videoMediaInfo.video.width > 0 ? m_videoMediaInfo.video.width : m_pFilter->GetTargetWidth();
		pvid->rcSource.bottom = m_videoMediaInfo.video.height > 0 ? m_videoMediaInfo.video.height : m_pFilter->GetTargetHeight();
		pvid->rcTarget = pvid->rcSource;
		pvid->dwBitRate = m_videoMediaInfo.video.bitrate > 0 ? m_videoMediaInfo.video.bitrate : 304018;
		pvid->AvgTimePerFrame = 400000;
		pvid->dwPictAspectRatioX = 16;
		pvid->dwPictAspectRatioY = 9;
		pvid->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pvid->bmiHeader.biWidth = pvid->rcSource.right;
		pvid->bmiHeader.biHeight = pvid->rcSource.bottom;
		pvid->bmiHeader.biPlanes = 1;
		pvid->bmiHeader.biBitCount = 24;
		pvid->bmiHeader.biCompression = m_videoMediaInfo.i_format;
		pvid->bmiHeader.biSizeImage = pvid->bmiHeader.biWidth*pvid->bmiHeader.biHeight;
		pBuffer = pBuffer + sizeof(VIDEOINFOHEADER2);
		memcpy(pBuffer, m_videoMediaInfo.extra, m_videoMediaInfo.extra_size);
		//SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
		// SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

		pmt->SetType(&MEDIATYPE_Video);

		pmt->SetTemporalCompression(TRUE);

		// Work out the GUID for the subtype from the header info.

		pmt->SetSubtype(&videoSubType);
		pmt->SetSampleSize(0);
		pmt->lSampleSize = 0;
	}

	if (this->m_codecType == CODEC_TYPE_AUDIO)
	{
		ret = ((CRTSPStreamFilter *)m_pFilter)->m_pStreamMedia->rtspClinetGetMediaInfo(CODEC_TYPE_AUDIO, m_audioMediaInfo);
		if(ret < 0)
		{	
			return VFW_S_NO_MORE_ITEMS;
		}
		memcpy(&audioSubType, &MEDIASUBTYPE_AAC, sizeof(GUID));
		pmWaveInfo->nSamplesPerSec = m_audioMediaInfo.audio.sampleRate;//32000; 
		pmWaveInfo->nChannels = m_audioMediaInfo.audio.channels;//2 
		pmWaveInfo->wBitsPerSample = 16; 
		pmWaveInfo->cbSize	= m_audioMediaInfo.extra_size;
		pmWaveInfo->nBlockAlign = pmWaveInfo->nChannels * pmWaveInfo->wBitsPerSample / 8; //4
		pmWaveInfo->nAvgBytesPerSec = pmWaveInfo->nSamplesPerSec/2;//16000

		switch(m_audioMediaInfo.i_format)
		{
		case FOURCC( 'm', 'p', 'g', 'a' ):
			{
				memcpy(&audioSubType, &MEDIASUBTYPE_MPEG1AudioPayload, sizeof(GUID));
				FormatTag = 0x0050;
				pmWaveInfo->nSamplesPerSec = 0;
				
				//MP3
				pmWaveInfo->nChannels = 2;
				pmWaveInfo->nSamplesPerSec = 44100;
				pmWaveInfo->nAvgBytesPerSec = 16000;
				pmWaveInfo->wBitsPerSample = 0;
				pmWaveInfo->cbSize  = 22;
				mpga.dwHeadBitrate = pmWaveInfo->nAvgBytesPerSec*8;
				if (pmWaveInfo->nSamplesPerSec == 32000 || pmWaveInfo->nSamplesPerSec == 48000)
				{
					pmWaveInfo->nBlockAlign = (WORD)(144*mpga.dwHeadBitrate/m_audioMediaInfo.audio.sampleRate);
				}
				else
				{
					pmWaveInfo->nBlockAlign = 1;
				}

				mpga.fwHeadLayer = ACM_MPEG_LAYER3;				
				if (pmWaveInfo->nChannels >= 2)
				{
					mpga.fwHeadMode = ACM_MPEG_STEREO; 
				}
				else
				{
					mpga.fwHeadMode = ACM_MPEG_SINGLECHANNEL;
				}
				mpga.fwHeadModeExt  = 0x00;
				mpga.wHeadEmphasis  = 1;
				mpga.fwHeadFlags = ACM_MPEG_ID_MPEG1;
				mpga.dwPTSLow  =  0;
				mpga.dwPTSHigh = 0;	
				if (m_audioMediaInfo.extra_size > 0)
				{
					free(m_audioMediaInfo.extra);
				}
				m_audioMediaInfo.extra = (char*)&mpga.fwHeadLayer;
				m_audioMediaInfo.extra_size = 22;

				//static char extra[] = {0x04, 0x00, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0x40, 0x00, 0x01, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				////04 00 00 f4 01 00 02 00 40 00 01 00 1e 00 00 00 00 00 00 00 00 00
				////if (m_audioMediaInfo.extra_size > 0)
				////{
				////	free(m_audioMediaInfo.extra);
				////}
				//m_audioMediaInfo.extra = extra;
				//m_audioMediaInfo.extra_size = 22;
			}
			break;
		case  FOURCC( 'a', '5', '2', ' ' ):
			{
				FormatTag = 0x2000;
				pmWaveInfo->nSamplesPerSec = 0;
				audioSubType.Data1 = FormatTag;
			}
			break;
		case FOURCC( 't', 'w', 'o', 's' ):
			{
				audioSubType.Data1 = 0x736f7774;
				FormatTag = 0x736f7774;
				pmWaveInfo->nBlockAlign  = 1;
				pmWaveInfo->wBitsPerSample  = 16;
				pmWaveInfo->cbSize = 0;
			}
			break;
		case FOURCC( 'a', 'r', 'a', 'w' ):
			{
				audioSubType.Data1 = 0x20776172;
				FormatTag = 0x20776172;
				pmWaveInfo->nBlockAlign = 1;
				pmWaveInfo->wBitsPerSample = 8;
				pmWaveInfo->cbSize = 0;
			}
			break;
		case FOURCC( 'u', 'l', 'a', 'w' ):
			{
				FormatTag = 0x07;
				audioSubType.Data1 = FormatTag;
				pmWaveInfo->nAvgBytesPerSec = 8000;
				pmWaveInfo->nBlockAlign = 1;
				pmWaveInfo->wBitsPerSample = 8;
				pmWaveInfo->cbSize = 0;
			}
			break;
		case FOURCC( 'a', 'l', 'a', 'w' ):
			{
				audioSubType.Data1 = 0x06;
				FormatTag = 0x06;
				pmWaveInfo->nAvgBytesPerSec = 8000;
				pmWaveInfo->nBlockAlign = 1;
				pmWaveInfo->wBitsPerSample = 8;
				pmWaveInfo->cbSize = 0;
			}
			break;
		case FOURCC( 'g', '7', '2', '6' ):
			break;
		case FOURCC( 's', 'a', 'm', 'r' ):
			{
				FormatTag = m_audioMediaInfo.i_format;
				audioSubType.Data1 = 0x726D6173;
			}
			break;
		case FOURCC( 's', 'a', 'w', 'b' ):
			{
				FormatTag = m_audioMediaInfo.i_format;
				audioSubType.Data1 = 0x62776173;
			}
			break;
		case FOURCC( 'm', 'p', '4', 'a' ):
			{
				memcpy(&audioSubType,  &MEDIASUBTYPE_AAC, sizeof(GUID));
				pmWaveInfo->nChannels = 2;
				FormatTag = 0x00ff;
#if 1//for debug
				m_audioMediaInfo.extra_size = 2;
				pmWaveInfo->nSamplesPerSec = 44100;
				pmWaveInfo->nAvgBytesPerSec = 16000;
				pmWaveInfo->nBlockAlign = 0;
				pmWaveInfo->cbSize = 2;
				pmWaveInfo->wBitsPerSample = 16;
#endif
			}
			break;
		default:
			audioSubType = GUID_NULL;
			break;
		}

		pmWaveInfo->wFormatTag = FormatTag; 

		pmt->SetFormatType(&FORMAT_WaveFormatEx);
		if (m_audioMediaInfo.extra_size < 0)
		{
			m_audioMediaInfo.extra_size = 0;
		}
		pdata = pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX)+m_audioMediaInfo.extra_size);
		if (pdata == NULL)
		{
			return E_FAIL;
		}
		memcpy(pdata, pmWaveInfo, sizeof(WAVEFORMATEX));
		ptr = (char *)pdata + sizeof(WAVEFORMATEX);
		memcpy(ptr, m_audioMediaInfo.extra, m_audioMediaInfo.extra_size);

		pmt->SetType(&MEDIATYPE_Audio);
		pmt->SetTemporalCompression(FALSE);
		pmt->SetSubtype(&audioSubType);
		pmt->SetSampleSize(1);
	}

	return NOERROR;
} // GetMediaType

HRESULT CRTSPStreamOutputPin::DecideBufferSize( IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties) 
{
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pProperties, E_POINTER);

	CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;

	if (this->m_codecType == CODEC_TYPE_VIDEO)
	{
		pProperties->cBuffers = 20;
		pProperties->cbBuffer = 65535;
	}
	else
	{
		pProperties->cBuffers = 20;
		pProperties->cbBuffer = 4096;
	}
	
	ASSERT(pProperties->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties, &Actual);
	if(FAILED(hr))
	{
		return hr;
	}

	// Is this allocator unsuitable
	if(Actual.cbBuffer < pProperties->cbBuffer)
	{
		return E_FAIL;
	}

	return NOERROR;
}

STDMETHODIMP CRTSPStreamOutputPin::Notify(IBaseFilter * pSender, Quality q)
{
	return NOERROR;		
}

// overriden to expose IMediaPosition and IMediaSeeking control interfaces
STDMETHODIMP CRTSPStreamOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv) 
{
	CheckPointer(ppv,E_POINTER);
	ValidateReadWritePtr(ppv,sizeof(PVOID));
	*ppv = NULL;

	// See what interface the caller is interested in.
	if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) 
	{
		return CSourceSeeking::NonDelegatingQueryInterface(riid, ppv);
	} 
	else
		return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

void CRTSPStreamOutputPin::SetDuration(LONGLONG rtDuration)
{
	m_rtDuration = rtDuration;
	m_rtStart = 0;
	m_rtStop = rtDuration;
	m_tStop = rtDuration;
}

// Let derived class know when the output pin is connected
HRESULT CRTSPStreamOutputPin::CompleteConnect(IPin *pReceivePin) 
{
	SetDuration((LONGLONG)this->m_videoMediaInfo.duration*1E7);	
	
	return CBaseOutputPin::CompleteConnect(pReceivePin);
}

HRESULT CRTSPStreamOutputPin::ChangeStart()
{
	//if (m_pFilter->Seek(m_rtStart) < 0)
	//{
	//	return S_FALSE;
	//}
	m_pFilter->m_rtStart = m_rtStart;
	m_bIsSeek = TRUE;
	//m_rtSampleTime -= m_rtStart;
	
	return S_OK;
}
HRESULT CRTSPStreamOutputPin::ChangeStop()
{
	return S_OK;
}
HRESULT CRTSPStreamOutputPin::ChangeRate()
{
	return S_OK;
}



//------------------------------------------------------------------------------
// Forward Declarations

#define E_SEEK  (0x80008888L)
 
// {2AEF7912-7BB6-4577-802E-74A8F9C9CFFF}
DEFINE_GUID(CLSID_DeltaRtspSourceFilter, 
			0x2aef7912, 0x7bb6, 0x4577, 0x80, 0x2e, 0x74, 0xa8, 0xf9, 0xc9, 0xcf, 0xff);

// {0099A10A-FF34-4563-B146-27A0AFA0519B}
DEFINE_GUID(CLSID_RtspUrlProp, 
			0x99a10a, 0xff34, 0x4563, 0xb1, 0x46, 0x27, 0xa0, 0xaf, 0xa0, 0x51, 0x9b);



typedef enum _RtspFilterState
{	
	StateStopped	= 0,
	StateOpened,
	StatePaused,
	StateRunning
} RTSP_FILTER_STATE;
//------------------------------------------------------------------------------
class CRTSPStreamOutputPin;
//------------------------------------------------------------------------------
class CRTSPStreamFilter : public CBaseFilter, public CAMThread, public IFileSourceFilter, public ISpecifyPropertyPages, public IRtspStreaming
{
public:
	CStreamMedia*		m_pStreamMedia;
	wchar_t				m_wszFileName[1024];
	RTSP_FILTER_STATE	m_RtspState;
	REFERENCE_TIME		m_rtStart;
	REFERENCE_TIME		m_rtDuration;
	BOOL				m_bIsSeeking;
	 
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

	// IFileSource Interface
	DECLARE_IUNKNOWN	
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP GetCurFile(LPOLESTR* outFileName, AM_MEDIA_TYPE* outMediaType);
	STDMETHODIMP Load(LPCOLESTR inFileName, const AM_MEDIA_TYPE* inMediaType);

	// ISpecifyPropertyPages
	STDMETHODIMP GetPages(CAUUID *pPages);

	// IRtspStreaming
	STDMETHODIMP SetStreamURL(WCHAR* pwszURL);
	STDMETHODIMP GetStreamURL(WCHAR* pwszURL);
	STDMETHODIMP SetResolution(int nWidth, int nHeight);
	STDMETHODIMP SetAccount(WCHAR* pwszAccount);
	STDMETHODIMP SetPassword(WCHAR* pwszPass);

	int				GetPinCount(void);
	CBasePin		*GetPin(int n);
	STDMETHODIMP	FindPin(LPCWSTR Id, IPin ** ppPin);
	CCritSec*		pStateLock(void) { return &m_cStateLock; }
	STDMETHODIMP	GetState(DWORD dwMSecs, FILTER_STATE *State);

	STDMETHODIMP	Run(REFERENCE_TIME tStart);
	STDMETHODIMP	Stop(void);
	STDMETHODIMP	Pause(void);
	HRESULT			SetPin(void);
	//STDMETHODIMP  GetState(DWORD dwMSecs, FILTER_STATE *State);

	virtual HRESULT OnThreadCreate(void);/* {return NOERROR;};*/
	virtual HRESULT OnThreadDestroy(void) {return NOERROR;};
	virtual HRESULT OnThreadStartPlay(void);

	int GetTargetWidth(void){ return m_nWidth; }
	int GetTargetHeight(void){ return m_nHeight; }

public:
	// thread commands
	enum Command {CMD_INIT, CMD_PAUSE, CMD_RUN, CMD_STOP, CMD_EXIT};
	Command GetRequest(void) { return (Command) CAMThread::GetRequest(); }
	BOOL    CheckRequest(Command *pCom) { return CAMThread::CheckRequest( (DWORD *) pCom); }
	HRESULT ThreadInit(void) { return CallWorker(CMD_INIT); }
	HRESULT ThreadExit(void) { return CallWorker(CMD_EXIT); }
	HRESULT ThreadRun(void) { return CallWorker(CMD_RUN); }
	HRESULT ThreadPause(void) { return CallWorker(CMD_PAUSE); }
	HRESULT ThreadStop(void) { return CallWorker(CMD_STOP); }

	virtual DWORD ThreadProc(void);  		// the thread function

	virtual HRESULT DoBufferProcessingLoop(void);    // the loop executed whilst running
	virtual HRESULT FillBuffer(IMediaSample **pSamp);

	int Seek(REFERENCE_TIME rtStart);

private:
	int						m_nPins;
	CCritSec				m_cStateLock;	
	CCritSec				m_cSharedState;
	CCritSec				m_csSharedData;
	CRTSPStreamOutputPin	*videoOutPin;
	CRTSPStreamOutputPin	*audioOutPin;
	CRTSPStreamOutputPin	*pcurOutPin;

	int						m_nWidth;
	int						m_nHeight;

    // It is only allowed to create these objects with CreateInstance
    CRTSPStreamFilter(LPUNKNOWN lpunk, HRESULT *phr);

public:
	~CRTSPStreamFilter();
};

class CRTSPStreamOutputPin : public CBaseOutputPin,  public CSourceSeeking
{
friend class CRTSPStreamFilter;

	CRTSPStreamFilter	*m_pFilter;
	CodecType			m_codecType;
	MediaInfo			m_videoMediaInfo;
	MediaInfo			m_audioMediaInfo;
	CCritSec			m_cStateLock;	

public:
	long				m_lDefaultDealt;
	CRefTime			m_rtSampleTime;
	CRefTime			m_rtStartSampleTime;
	//CRefTime m_rtStart;
	long				m_lLast_timeStamp;
	BOOL				m_bDiscontinuity;
	BOOL				m_bIsSeek;

public:
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// --- CSourceSeeking ------------

	HRESULT		ChangeStart();
	HRESULT		ChangeStop();
	HRESULT		ChangeRate();
	void		SetDuration(LONGLONG rtDuration);

	HRESULT		CompleteConnect(IPin *pReceivePin);
	// Grab and release extra interfaces if required
    CRTSPStreamOutputPin(CodecType codeType, long defaultDealt, HRESULT *phr, CRTSPStreamFilter *pParent, LPCWSTR pPinName);
	//HRESULT CheckConnect(IPin *pPin);
	//HRESULT BreakConnect();
	//HRESULT CompleteConnect(IPin *pReceivePin);

	// check that we can support this output type
	HRESULT CheckMediaType(const CMediaType* mtOut);

	// set the connection media type
	//HRESULT SetMediaType(const CMediaType *pmt);

	// called from CBaseOutputPin during connection to ask for
	// the count and size of buffers we need.
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);

	// returns the preferred formats for a pin
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// inherited from IQualityControl via CBasePin
	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);
};


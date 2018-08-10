#pragma once

#include "AVC1Parser.h"
#include "DeltaCudaSettings.h"
#include "FloatingAverage.h"

#define DELTA_CUDA_REGISTRY_KEY L"Software\\Delta\\Cuda"
#define MAX_DECODE_FRAMES 20
#define DISPLAY_DELAY	2
#define USE_ASYNC_COPY 1

typedef enum
{
    ITU601 = 1,
    ITU709 = 2
} eColorSpace;

[uuid("62D767FE-4F1B-478B-B350-8ACE9E4DB00E")]
class CDeltaCudaH264Decoder : public CTransformFilter, public ISpecifyPropertyPages, public IDeltaCudaSettings
{
public:
	// constructor method used by class factory
	static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);

	// IUnknown
	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// ISpecifyPropertyPages
	STDMETHODIMP GetPages(CAUUID *pPages);

	// CTransformFilter
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut );
	HRESULT BreakConnect(PIN_DIRECTION dir);

	HRESULT Receive(IMediaSample *pIn);
	HRESULT EndOfStream();
	HRESULT BeginFlush();
	HRESULT EndFlush();
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);

	// IDeltaCudaSettings
	STDMETHODIMP_(DWORD) GetDeinterlaceMode();
	STDMETHODIMP SetDeinterlaceMode(DWORD dwMode);

	STDMETHODIMP_(BOOL) GetFrameDoubling();
	STDMETHODIMP SetFrameDoubling(BOOL bFrameDoubling);

	STDMETHODIMP_(DWORD) GetFieldOrder();
	STDMETHODIMP SetFieldOrder(DWORD dwFieldOrder);

	STDMETHODIMP GetFormatConfiguration(bool *bFormat);
	STDMETHODIMP SetFormatConfiguration(bool *bFormat);

	STDMETHODIMP_(BOOL) GetStreamAR();
	STDMETHODIMP SetStreamAR(BOOL bFLag);

	STDMETHODIMP_(BOOL) GetDXVA();
	STDMETHODIMP SetDXVA(BOOL bDXVA);

	STDMETHODIMP_(DWORD) GetOutputFormat();
	STDMETHODIMP SetOutputFormat(DWORD dwOutput);

	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Stop(void);
	STDMETHODIMP Pause(void);

public:
	// Pin Configuration
	const static AMOVIESETUP_MEDIATYPE    sudPinTypesIn[];
	const static int                      sudPinTypesInCount;
	const static AMOVIESETUP_MEDIATYPE    sudPinTypesOut[];
	const static int                      sudPinTypesOutCount;

private:
	CDeltaCudaH264Decoder(LPUNKNOWN pUnk, HRESULT* phr);
	~CDeltaCudaH264Decoder();

	CMediaType CreateMediaType(CMediaType &mtIn, const GUID &subtype) const;

	HRESULT QueueFormatChange(CUVIDEOFORMAT *vidformat);
	HRESULT ReconnectOutput();

	HRESULT InitCUDA();
	HRESULT InitCodec(cudaVideoCodec cudaCodec);
	HRESULT CreateCUVIDDecoder(cudaVideoCodec codec, DWORD dwWidth, DWORD dwHeight, DWORD dwDisplayWidth, DWORD dwDisplayHeight, RECT rcDisplayArea);
	HRESULT DestroyCUDA(bool bFull = true);

	HRESULT DecodeExtraData();

	// CUDA Callbacks
	static int CUDAAPI HandleVideoSequence(void *obj, CUVIDEOFORMAT *cuvidfmt);
	static int CUDAAPI HandlePictureDecode(void *obj, CUVIDPICPARAMS *cuvidpic);
	static int CUDAAPI HandlePictureDisplay(void *obj, CUVIDPARSERDISPINFO *cuviddisp);

	HRESULT GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData);
	HRESULT Display(CUVIDPARSERDISPINFO *cuviddisp);
	HRESULT Deliver(CUVIDPARSERDISPINFO *cuviddisp, int field = 0);

	HRESULT LoadSettings();
	HRESULT SaveSettings();

	BOOL modInitCTX(const char *filename);
	void cudaNV12toARGB(CUdeviceptr *ppDecodedFrame, size_t nDecodedPitch,
						CUdeviceptr *ppTextureData,  size_t nTexturePitch,
						CUmodule cuModNV12toARGB,
						CUfunction fpCudaKernel, CUstream streamID);

	void setColorSpaceMatrix(eColorSpace CSC, float *hueCSC, float hue);
	CUresult  updateConstantMemory_drvapi(CUmodule module, float *hueCSC);
	CUresult  cudaLaunchNV12toARGBDrv(	CUdeviceptr d_srcNV12,	size_t nSourcePitch,
										CUdeviceptr d_dstARGB,	size_t nDestPitch,
										UINT width,				UINT height,
										CUfunction fpFunc,		CUstream streamID);

private:
	CUVIDEOFORMAT			*m_newFormat;
	BOOL					m_bInterlaced;

	IDirect3D9				*m_pD3D;              // Direct3D9 Instance
	IDirect3DDevice9		*m_pD3DDevice;        // Direct3D9Device
	D3DPRESENT_PARAMETERS	m_d3dpp;

	CUcontext				m_cudaContext;
	CUvideoctxlock			m_cudaCtxLock;

	CUmodule				m_cuModule;
	CUfunction				m_fpCuda;
	CUdeviceptr				m_pInteropFrame;
	bool					m_bUpdate;

	CUvideoparser			m_hParser;            // handle to the CUDA video-parser
	CUstream				m_hStream;
	CUvideodecoder			m_hDecoder;
	CUVIDDECODECREATEINFO	m_VideoDecoderInfo;  // Info used to create the CUDA video decoder
	BOOL					m_bForceSequenceUpdate;

	BYTE					*m_pbRawNV12;
	int						m_cRawNV12;

	BYTE					*m_pbRawRGBA;
	int						m_cRawRGBA;

	CUVIDPARSERDISPINFO		m_DisplayQueue[DISPLAY_DELAY];
	int						m_DisplayPos;
	BOOL					m_bFlushing;

	CAVC1Parser				*m_avc1Parser;

	int						m_aspectX;
	int						m_aspectY;

	BOOL					m_bVDPAULevelC;

	REFERENCE_TIME			m_rtStartPrev;
	REFERENCE_TIME			m_rtStopPrev;

	REFERENCE_TIME			m_rtStartCalc;
	BOOL					m_bVC1EVO;

	FloatingAverage<REFERENCE_TIME> m_faTime;

	int						m_nTargetWidth;
	int						m_nTargetHeight;

	// Settings
	struct Settings 
	{
		bool bFormats[cudaVideoCodec_NumCodecs];
		BOOL bStreamAR;
		DWORD dwDeinterlace;
		BOOL bFrameDoubling;
		DWORD dwFieldOrder;
		BOOL bDXVA;
		DWORD dwOutput;
	} m_settings;
};

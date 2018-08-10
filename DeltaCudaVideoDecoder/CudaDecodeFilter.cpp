#include "stdafx.h"
#include "CudaDecodeFilter.h"
#include "GlobalAPI.h"
#include "registry.h"
#include "DeltaCudaSettingsProp.h"

#include "otheruuids.h"

#include "cutil_inline.h"

#include "stdint.h"

#include <vector_types.h>
#include "Tools.h"

#pragma comment(lib,"comsuppw")

using namespace std;

#define __constant__ \
        __location__(constant)

__constant__ float  constAlpha;

typedef struct 
{
	const CLSID*         clsMinorType;
	const cudaVideoCodec nCUDACodec;
} CUDA_SUBTYPE_MAP;

CUDA_SUBTYPE_MAP cuda_video_codecs[] = 
{
	// H264
	{ &MEDIASUBTYPE_H264, cudaVideoCodec_H264 },
	{ &MEDIASUBTYPE_AVC1, cudaVideoCodec_H264 },
	{ &MEDIASUBTYPE_avc1, cudaVideoCodec_H264 },
	{ &MEDIASUBTYPE_CCV1, cudaVideoCodec_H264 },

	// VC1
	{ &MEDIASUBTYPE_WVC1, cudaVideoCodec_VC1 },
	{ &MEDIASUBTYPE_wvc1, cudaVideoCodec_VC1 },
	{ &MEDIASUBTYPE_WMVA, cudaVideoCodec_VC1 },
	{ &MEDIASUBTYPE_wmva, cudaVideoCodec_VC1 },

	// MPEG2
	{ &MEDIASUBTYPE_MPEG2_VIDEO, cudaVideoCodec_MPEG2 },

	// MPEG4 ASP
	{ &MEDIASUBTYPE_XVID, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_xvid, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_DIVX, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_divx, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_DX50, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_dx50, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_MP4V, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_mp4v, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_M4S2, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_m4s2, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_MP4S, cudaVideoCodec_MPEG4 },
	{ &MEDIASUBTYPE_mp4s, cudaVideoCodec_MPEG4 },
};

// Define Input Media Types
const AMOVIESETUP_MEDIATYPE CDeltaCudaH264Decoder::sudPinTypesIn[] = 
{
	// H264
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_H264 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_AVC1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_avc1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_CCV1 },

	// VC1
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WVC1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wvc1 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_WMVA },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_wmva },

	// MPEG2
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG2_VIDEO },

	// MPEG4
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_XVID },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_xvid },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DIVX },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_divx },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_DX50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_dx50 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP4V },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp4v },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_M4S2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_m4s2 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_MP4S },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_mp4s },
};
const int CDeltaCudaH264Decoder::sudPinTypesInCount = countof(CDeltaCudaH264Decoder::sudPinTypesIn);

// Define Output Media Types
const AMOVIESETUP_MEDIATYPE CDeltaCudaH264Decoder::sudPinTypesOut[] = 
{
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_NV12 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_YV12 },
	{ &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 },
};
const int CDeltaCudaH264Decoder::sudPinTypesOutCount = countof(CDeltaCudaH264Decoder::sudPinTypesOut);

// static constructor
CUnknown* WINAPI CDeltaCudaH264Decoder::CreateInstance(LPUNKNOWN pUnk, HRESULT* phr)
{
	return new CDeltaCudaH264Decoder(pUnk, phr);
}

static CUresult cuvidFlushParser(CUvideoparser hParser)
{
	CUVIDSOURCEDATAPACKET pCuvidPacket;
	memset(&pCuvidPacket, 0, sizeof(pCuvidPacket));

	pCuvidPacket.flags |= CUVID_PKT_ENDOFSTREAM;
	CUresult result = CUDA_SUCCESS;

	__try 
	{
		result = cuvidParseVideoData(hParser, &pCuvidPacket);
	} 
	__except (1) 
	{
		DbgLog((LOG_ERROR, 10, L"cuvidFlushParser(): cuvidParseVideoData threw an exception"));
		result = CUDA_ERROR_UNKNOWN;
	}
	return result;
}

// Constructor
CDeltaCudaH264Decoder::CDeltaCudaH264Decoder(LPUNKNOWN pUnk, HRESULT* phr)
  : CTransformFilter(NAME("Delta Cuda H264 Decoder"), 0, __uuidof(CDeltaCudaH264Decoder))
  , m_pD3D(NULL)
  , m_pD3DDevice(NULL)
  , m_cudaContext(0)
  , m_cudaCtxLock(0)
  , m_cuModule(0)
  , m_fpCuda(NULL)
  , m_pInteropFrame(NULL)
  , m_bUpdate(FALSE)
  , m_hParser(0)
  , m_hStream(0)
  , m_hDecoder(0)
  , m_pbRawNV12(NULL)
  , m_cRawNV12(0)
  , m_pbRawRGBA(NULL)
  , m_cRawRGBA(0)
  , m_newFormat(NULL)
  , m_avc1Parser(NULL)
  , m_bFlushing(FALSE)
  , m_bInterlaced(FALSE)
  , m_bForceSequenceUpdate(FALSE)
  , m_aspectX(0)
  , m_aspectY(0)
  , m_bVDPAULevelC(FALSE)
  , m_rtStartPrev(0)
  , m_rtStopPrev(0)
  , m_rtStartCalc(INT64_MIN)
  , m_faTime(50)
  , m_bVC1EVO(FALSE)
  , m_nTargetWidth(1280)
  , m_nTargetHeight(720)
{
	LoadSettings();

#ifdef DEBUG
	DbgSetModuleLevel (LOG_TRACE, DWORD_MAX);
	//DbgSetModuleLevel (LOG_CUSTOM1, DWORD_MAX); // CUDA messages
	DbgSetModuleLevel (LOG_ERROR, DWORD_MAX);
#endif
}

CDeltaCudaH264Decoder::~CDeltaCudaH264Decoder()
{
	/*if(m_cuModule)
		cuModuleUnload(m_cuModule);*/

	DestroyCUDA(true);
}

HRESULT CDeltaCudaH264Decoder::LoadSettings()
{
	HRESULT hr;
	DWORD dwVal;

	OSVERSIONINFO os;
	memset(&os, 0, sizeof(os));
	os.dwOSVersionInfoSize = sizeof(os);
	GetVersionEx(&os);

	CreateRegistryKey(HKEY_CURRENT_USER, DELTA_CUDA_REGISTRY_KEY);
	CRegistry reg = CRegistry(HKEY_CURRENT_USER, DELTA_CUDA_REGISTRY_KEY, hr);
	// We don't check if opening succeeded, because the read functions will set their hr accordingly anyway,
	// and we need to fill the settings with defaults.
	// ReadString returns an empty string in case of failure, so thats fine!

	// Deinterlacing
	dwVal = reg.ReadDWORD(L"deinterlaceMode", hr);
	m_settings.dwDeinterlace = SUCCEEDED(hr) ? dwVal : cudaVideoDeinterlaceMode_Weave;

	// Double Frame Rate Deinterlacing
	dwVal = reg.ReadDWORD(L"FrameDoubling", hr);
	m_settings.bFrameDoubling = SUCCEEDED(hr) ? dwVal : TRUE;

	// Deinterlacing
	dwVal = reg.ReadDWORD(L"FieldOrder", hr);
	m_settings.dwFieldOrder = SUCCEEDED(hr) ? dwVal : 0;

	// Video Engine
	dwVal = reg.ReadDWORD(L"DXVA", hr);
	m_settings.bDXVA = SUCCEEDED(hr) ? dwVal : 0;

	// Default all Codecs to enabled
	for(int i = 0; i < cudaVideoCodec_NumCodecs; ++i)
		m_settings.bFormats[i] = true;

	BYTE *buf = reg.ReadBinary(L"Formats", dwVal, hr);
	if (SUCCEEDED(hr)) 
	{
		memcpy(&m_settings.bFormats, buf, min(dwVal, sizeof(m_settings.bFormats)));
		SAFE_CO_FREE(buf);
	}

	// Stream AR
	dwVal = reg.ReadDWORD(L"StreamAR", hr);
	m_settings.bStreamAR = SUCCEEDED(hr) ? dwVal : TRUE;

	// Output Format (0 = Auto/Both, 1 = NV12, 2 = YV12)
	dwVal = reg.ReadDWORD(L"OutputFormat", hr);
	m_settings.dwOutput = SUCCEEDED(hr) ? dwVal : 0;

	return S_OK;
}

HRESULT CDeltaCudaH264Decoder::SaveSettings()
{
	HRESULT hr;
	CRegistry reg = CRegistry(HKEY_CURRENT_USER, DELTA_CUDA_REGISTRY_KEY, hr);
	if (SUCCEEDED(hr)) 
	{
		reg.WriteDWORD(L"deinterlaceMode", m_settings.dwDeinterlace);
		reg.WriteBOOL(L"FrameDoubling", m_settings.bFrameDoubling);
		reg.WriteDWORD(L"FieldOrder", m_settings.dwFieldOrder);
		reg.WriteBinary(L"Formats", (BYTE *)m_settings.bFormats, sizeof(m_settings.bFormats));
		reg.WriteBOOL(L"StreamAR", m_settings.bStreamAR);
		reg.WriteBOOL(L"DXVA", m_settings.bDXVA);
		reg.WriteDWORD(L"OutputFormat", m_settings.dwOutput);
	}
	return S_OK;
}

BOOL CDeltaCudaH264Decoder::modInitCTX(const char *filename)
{
	string module_path;
	string ptx_source;
	CUresult cuStatus;

	module_path = filename;

	FILE *fp = fopen(module_path.c_str(), "rb");
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    char *buf = new char[file_size+1];
    fseek(fp, 0, SEEK_SET);
    fread(buf, sizeof(char), file_size, fp);
    fclose(fp);
    buf[file_size] = '\0';
    ptx_source = buf;
    delete [] buf;

	if (module_path.rfind(".txt") != string::npos)
    {
        // in this branch we use compilation with parameters
        const unsigned int jitNumOptions = 3;
        CUjit_option *jitOptions = new CUjit_option[jitNumOptions];
        void **jitOptVals = new void *[jitNumOptions];

        // set up size of compilation log buffer
        jitOptions[0] = CU_JIT_INFO_LOG_BUFFER_SIZE_BYTES;
        int jitLogBufferSize = 1024;
        jitOptVals[0] = (void *)(size_t)jitLogBufferSize;

        // set up pointer to the compilation log buffer
        jitOptions[1] = CU_JIT_INFO_LOG_BUFFER;
        char *jitLogBuffer = new char[jitLogBufferSize];
        jitOptVals[1] = jitLogBuffer;

        // set up pointer to set the Maximum # of registers for a particular kernel
        jitOptions[2] = CU_JIT_MAX_REGISTERS;
        int jitRegCount = 32;
        jitOptVals[2] = (void *)(size_t)jitRegCount;

        cuStatus = cuModuleLoadDataEx(&m_cuModule, ptx_source.c_str(), jitNumOptions, jitOptions, (void **)jitOptVals);

        if (cuStatus != CUDA_SUCCESS)
        {
            printf("cuModuleLoadDataEx error!\n");
			return FALSE;
        }
    }
    else
    {
        cuStatus = cuModuleLoad(&m_cuModule, module_path.c_str());

        if (cuStatus != CUDA_SUCCESS)
        {
            printf("cuModuleLoad error!\n");
			return FALSE;
        }
    }

	return TRUE;
}

void CDeltaCudaH264Decoder::cudaNV12toARGB(	CUdeviceptr *ppDecodedFrame, size_t nDecodedPitch, CUdeviceptr *ppTextureData,  size_t nTexturePitch,
											CUmodule cuModNV12toARGB, CUfunction fpCudaKernel, CUstream streamID)
{
	UINT unWidth  = m_nTargetWidth;
    UINT unHeight = m_nTargetHeight;

	if(!m_bUpdate)
	{
		float hueColorSpaceMat[9];
		setColorSpaceMatrix(ITU709, hueColorSpaceMat, 0.0f);
		updateConstantMemory_drvapi(cuModNV12toARGB, hueColorSpaceMat);

		m_bUpdate = true;
	}

	// Final Stage: NV12toARGB color space conversion
    CUresult eResult;
    eResult = cudaLaunchNV12toARGBDrv(*ppDecodedFrame, nDecodedPitch,
                                      *ppTextureData,  nTexturePitch,
                                      unWidth, unHeight, fpCudaKernel, streamID);
}

void CDeltaCudaH264Decoder::setColorSpaceMatrix(eColorSpace CSC, float *hueCSC, float hue)
{
    float hueSin = sin(hue);
    float hueCos = cos(hue);

    if (CSC == ITU601)
    {
        //CCIR 601
        hueCSC[0] = 1.1644f;
        hueCSC[1] = hueSin * 1.5960f;
        hueCSC[2] = hueCos * 1.5960f;
        hueCSC[3] = 1.1644f;
        hueCSC[4] = (hueCos * -0.3918f) - (hueSin * 0.8130f);
        hueCSC[5] = (hueSin *  0.3918f) - (hueCos * 0.8130f);
        hueCSC[6] = 1.1644f;
        hueCSC[7] = hueCos *  2.0172f;
        hueCSC[8] = hueSin * -2.0172f;
    }
    else if (CSC == ITU709)
    {
        //CCIR 709
        hueCSC[0] = 1.0f;
        hueCSC[1] = hueSin * 1.57480f;
        hueCSC[2] = hueCos * 1.57480f;
        hueCSC[3] = 1.0;
        hueCSC[4] = (hueCos * -0.18732f) - (hueSin * 0.46812f);
        hueCSC[5] = (hueSin *  0.18732f) - (hueCos * 0.46812f);
        hueCSC[6] = 1.0f;
        hueCSC[7] = hueCos *  1.85560f;
        hueCSC[8] = hueSin * -1.85560f;
    }
}

CUresult CDeltaCudaH264Decoder::updateConstantMemory_drvapi(CUmodule module, float *hueCSC)
{
    CUdeviceptr  d_constHueCSC, d_constAlpha;
    size_t       d_cscBytes, d_alphaBytes;

    // First grab the global device pointers from the CUBIN
    cuModuleGetGlobal(&d_constHueCSC,  &d_cscBytes  , module, "HueColorSpaceAry");
    cuModuleGetGlobal(&d_constAlpha ,  &d_alphaBytes, module, "constAlpha");

    CUresult error = CUDA_SUCCESS;

    // Copy the constants to video memory
    cuMemcpyHtoD(d_constHueCSC,
                 reinterpret_cast<const void *>(hueCSC),
                 d_cscBytes);
    //getLastCudaDrvErrorMsg("cuMemcpyHtoD (d_constHueCSC) copy to Constant Memory failed");


    UINT cudaAlpha      = ((UINT)0xff<< 24);

    cuMemcpyHtoD(constAlpha,
                 reinterpret_cast<const void *>(&cudaAlpha),
                 d_alphaBytes);
    //getLastCudaDrvErrorMsg("cuMemcpyHtoD (constAlpha) copy to Constant Memory failed");

    return error;
}

// We call this function to launch the CUDA kernel (NV12 to ARGB).
CUresult CDeltaCudaH264Decoder::cudaLaunchNV12toARGBDrv(CUdeviceptr d_srcNV12,	size_t nSourcePitch,
														CUdeviceptr d_dstARGB,	size_t nDestPitch,
														UINT width,				UINT height,
														CUfunction fpFunc,		CUstream streamID)
{
    CUresult status;
    // Each thread will output 2 pixels at a time.  The grid size width is half
    // as large because of this
    dim3 block(32, 16, 1);
    dim3 grid((width+(2*block.x-1))/(2*block.x), (height+(block.y-1))/block.y, 1);

    // This is the new CUDA 4.0 API for Kernel Parameter passing and Kernel Launching (simpler method)
    void *args[] = { &d_srcNV12, &nSourcePitch,
                     &d_dstARGB, &nDestPitch,
                     &width,	 &height
                   };

    // new CUDA 4.0 Driver API Kernel launch call
    status = cuLaunchKernel(fpFunc, grid.x, grid.y, grid.z,
                            block.x, block.y, block.z,
                            0, streamID,
                            args, NULL);

    if (CUDA_SUCCESS != status)
    {
        fprintf(stderr, "cudaLaunchNV12toARGBDrv() failed to launch Kernel Function %08x, retval = %d\n", (unsigned int)fpFunc, status);
        return status;
    }

    return status;
}

// IUnknown
STDMETHODIMP CDeltaCudaH264Decoder::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
	CheckPointer(ppv, E_POINTER);

	*ppv = NULL;

	return QI2(ISpecifyPropertyPages) QI2(IDeltaCudaSettings) __super::NonDelegatingQueryInterface(riid, ppv);
}

// ISpecifyPropertyPages
STDMETHODIMP CDeltaCudaH264Decoder::GetPages(CAUUID *pPages)
{
	CheckPointer(pPages, E_POINTER);

	pPages->cElems = 1;
	pPages->pElems = (GUID *)CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
	if (pPages->pElems == NULL) 
	{
		return E_OUTOFMEMORY;
	}
	pPages->pElems[0] = CLSID_DeltaCudaSettingsProp;

	return S_OK;
}

HRESULT CDeltaCudaH264Decoder::CheckInputType(const CMediaType* mtIn)
{
	for(int i = 0; i < sudPinTypesInCount; ++i) 
	{
		if(	*sudPinTypesIn[i].clsMajorType == mtIn->majortype && 
			*sudPinTypesIn[i].clsMinorType == mtIn->subtype && 
			(mtIn->formattype == FORMAT_VideoInfo || mtIn->formattype == FORMAT_VideoInfo2 || mtIn->formattype == FORMAT_MPEG2Video)) 
		{
			cudaVideoCodec codec;
			bool bCodecFound = false;
			for (int i = 0; i < countof(cuda_video_codecs); ++i) 
			{
				if (mtIn->subtype == *cuda_video_codecs[i].clsMinorType) 
				{
					codec = cuda_video_codecs[i].nCUDACodec;
					bCodecFound = true;
					break;
				}
			}
			// Check for supported profiles
			if (bCodecFound && codec == cudaVideoCodec_H264 && mtIn->formattype == FORMAT_MPEG2Video) 
			{
				MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)mtIn->Format();
				// H264 Profiles > 100 (High) are not supported
				if (mp2vi->dwProfile > 100) 
				{
					return VFW_E_TYPE_NOT_ACCEPTED;
				}
			} 
			else if (bCodecFound && codec == cudaVideoCodec_MPEG2) 
			{
				BYTE *extradata = NULL;
				unsigned extralen;
				getExtraDataPtr(mtIn->Format(), mtIn->FormatType(), &extradata, &extralen);
				if (extradata && extralen) 
				{
					uint32_t state = -1;
					uint8_t *data  = extradata;
					int size = extralen;
					while(size > 2 && state != 0x000001b5) 
					{
						state = (state << 8) | *data++;
						size--;
					}
					if (state == 0x000001b5) 
					{
						int start_code = (*data) >> 4;
						if (start_code == 1) 
						{
							data++;
							int chroma = ((*data) & 0x6) >> 1;
							if (chroma >= 2) 
							{
								DbgLog((LOG_TRACE, 10, L"::CheckInputType(): Detected MPEG-2 4:2:2, refusing connection"));
								return VFW_E_TYPE_NOT_ACCEPTED;
							}
						}
					}
				}
			}
			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

static BOOL validSubtype(const GUID &subtype, DWORD dwOutputFormatConfig) 
{
	for(int i = 0; i < countof(CDeltaCudaH264Decoder::sudPinTypesOut); ++i) 
	{
		if (*CDeltaCudaH264Decoder::sudPinTypesOut[i].clsMinorType == subtype && (dwOutputFormatConfig == 0 || (i+1) == (dwOutputFormatConfig)))
			return TRUE;
	}
	return FALSE;
}

HRESULT CDeltaCudaH264Decoder::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
  return SUCCEEDED(	CheckInputType(mtIn)) &&
					mtOut->majortype == MEDIATYPE_Video && 
					/*validSubtype(mtOut->subtype, m_settings.dwOutput) &&*/
					(*mtOut->FormatType() == FORMAT_VideoInfo2 || *mtOut->FormatType() == FORMAT_VideoInfo)
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CDeltaCudaH264Decoder::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	DbgLog((LOG_TRACE, 5, L"GetMediaType"));

	if(m_pInput->IsConnected() == FALSE) 
	{
		return E_UNEXPECTED;
	}

	if(iPosition < 0) 
	{
		return E_INVALIDARG;
	}
	if(iPosition >= countof(CDeltaCudaH264Decoder::sudPinTypesOut)) 
	{
		return VFW_S_NO_MORE_ITEMS;
	}

	*pMediaType = CreateMediaType(m_pInput->CurrentMediaType(), *CDeltaCudaH264Decoder::sudPinTypesOut[iPosition].clsMinorType);
	return S_OK;
}

HRESULT CDeltaCudaH264Decoder::BreakConnect(PIN_DIRECTION dir)
{
	DbgLog((LOG_TRACE, 10, L"::BreakConnect(): Lost connection to %s pin", (dir == PINDIR_INPUT) ? L"input" : L"output"));

	if (dir == PINDIR_INPUT) 
	{
		DestroyCUDA(true);
	}

	return __super::BreakConnect(dir);
}

CMediaType CDeltaCudaH264Decoder::CreateMediaType(CMediaType &mtIn, const GUID &subtype) const
{
	LONG biWidth, biHeight;
	DWORD biAspectX = 0, biAspectY = 0;
	REFERENCE_TIME avgTime = 0;
	if (mtIn.formattype == FORMAT_VideoInfo) 
	{
		VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)mtIn.pbFormat;
		biWidth = vih->bmiHeader.biWidth;
		biHeight = vih->bmiHeader.biHeight;
		avgTime = vih->AvgTimePerFrame;
	} 
	else if (mtIn.formattype == FORMAT_VideoInfo2) 
	{
		VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2 *)mtIn.pbFormat;
		biWidth = vih2->bmiHeader.biWidth;
		biHeight = vih2->bmiHeader.biHeight;
		avgTime = vih2->AvgTimePerFrame;
		biAspectX = vih2->dwPictAspectRatioX;
		biAspectY = vih2->dwPictAspectRatioY;
	} 
	else if (mtIn.formattype == FORMAT_MPEG2Video) 
	{
		MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)mtIn.pbFormat;
		biWidth = mp2vi->hdr.bmiHeader.biWidth;
		biHeight = mp2vi->hdr.bmiHeader.biHeight;
		avgTime = mp2vi->hdr.AvgTimePerFrame;
		biAspectX = mp2vi->hdr.dwPictAspectRatioX;
		biAspectY = mp2vi->hdr.dwPictAspectRatioY;
	}

	if (biAspectX == 0) 
	{
		biAspectX = biWidth;
		biAspectY = biHeight;
	}

	CMediaType mt;

	mt.majortype = MEDIATYPE_Video;
	mt.subtype = subtype;

	if(IsEqualGUID(subtype, MEDIASUBTYPE_RGB24))
	{
		mt.formattype = FORMAT_VideoInfo;

		VIDEOINFOHEADER vih;
		memset(&vih, 0, sizeof(vih));

		vih.AvgTimePerFrame = avgTime;
		vih.rcSource.right = vih.rcTarget.right = biWidth;
		vih.rcSource.bottom = vih.rcTarget.bottom = biHeight;
		vih.bmiHeader.biSize = sizeof(vih.bmiHeader);
		vih.bmiHeader.biWidth = biWidth;
		vih.bmiHeader.biHeight = biHeight;
		vih.bmiHeader.biPlanes = 1;
		vih.bmiHeader.biBitCount = 24;
		vih.bmiHeader.biCompression = BI_RGB;
		vih.bmiHeader.biSizeImage = biWidth * biHeight * vih.bmiHeader.biBitCount / 8;

		mt.SetSampleSize(vih.bmiHeader.biSizeImage);
		mt.SetFormat((BYTE *)&vih, sizeof(vih));
	}
	else
	{
		mt.formattype = FORMAT_VideoInfo2;

		VIDEOINFOHEADER2 vih;
		memset(&vih, 0, sizeof(vih));

		vih.AvgTimePerFrame = avgTime;
		vih.rcSource.right = vih.rcTarget.right = biWidth;
		vih.rcSource.bottom = vih.rcTarget.bottom = biHeight;
		vih.bmiHeader.biSize = sizeof(vih.bmiHeader);
		vih.bmiHeader.biWidth = biWidth;
		vih.bmiHeader.biHeight = biHeight;
		vih.bmiHeader.biPlanes = 3;
		vih.bmiHeader.biBitCount = 12;
		vih.bmiHeader.biCompression = mt.subtype.Data1;
		vih.bmiHeader.biSizeImage = biWidth * biHeight * vih.bmiHeader.biBitCount / 8;
		vih.dwPictAspectRatioX = biAspectX;
		vih.dwPictAspectRatioY = biAspectY;

		mt.SetSampleSize(vih.bmiHeader.biSizeImage);
		mt.SetFormat((BYTE *)&vih, sizeof(vih));
	}
  
	return mt;
}

HRESULT CDeltaCudaH264Decoder::QueueFormatChange(CUVIDEOFORMAT *vidformat)
{
	SAFE_FREE(m_newFormat);

	m_newFormat = (CUVIDEOFORMAT *)malloc(sizeof(CUVIDEOFORMAT));
	memcpy(m_newFormat, vidformat, sizeof(CUVIDEOFORMAT));

	return S_OK;
}

HRESULT CDeltaCudaH264Decoder::DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	if(m_pInput->IsConnected() == FALSE) 
	{
		return E_UNEXPECTED;
	}

	BITMAPINFOHEADER *pBIH = NULL;
	CMediaType mtOut = m_pOutput->CurrentMediaType();
	formatTypeHandler(mtOut.Format(), mtOut.FormatType(), &pBIH, NULL);

	pprop->cBuffers = 4;
	pprop->cbBuffer = pBIH->biSizeImage;
	pprop->cbAlign = 1;
	pprop->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
	if(FAILED(hr = pAllocator->SetProperties(pprop, &Actual))) 
	{
		return hr;
	}

	return pprop->cBuffers > Actual.cBuffers || pprop->cbBuffer > Actual.cbBuffer ? E_FAIL : NOERROR;
}

HRESULT CDeltaCudaH264Decoder::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
	if (direction == PINDIR_INPUT) 
	{
		HRESULT hr = S_OK;

		cudaVideoCodec codec;
		bool bCodecFound = false;
		for (int i = 0; i < countof(cuda_video_codecs); ++i) 
		{
			if (pmt->subtype == *cuda_video_codecs[i].clsMinorType) 
			{
				codec = cuda_video_codecs[i].nCUDACodec;
				bCodecFound = true;
				break;
			}
		}
		if (!bCodecFound) 
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}

		// Initialize CUDA Device
		hr = InitCodec(codec);
		if (FAILED(hr)) 
		{
			return VFW_E_TYPE_NOT_ACCEPTED;
		}
	} 
	else if (direction == PINDIR_OUTPUT) 
	{
		DbgLog((LOG_TRACE, 1, "Setting new output type"));
	}

	return __super::SetMediaType(direction, pmt);
}

HRESULT CDeltaCudaH264Decoder::DestroyCUDA(bool bFull)
{
	if (m_hDecoder) 
	{
		cuvidDestroyDecoder(m_hDecoder);
		m_hDecoder = 0;
	}

	if (m_hParser) 
	{
		cuvidDestroyVideoParser(m_hParser);
		m_hParser = 0;
	}

	if (m_hStream) 
	{
		cuStreamDestroy(m_hStream);
		m_hStream = 0;
	}

	if (m_pbRawNV12) 
	{
		cuMemFreeHost(m_pbRawNV12);
		m_pbRawNV12 = NULL;
		m_cRawNV12 = 0;
	}

	if (m_pbRawRGBA) 
	{
		cuMemFreeHost(m_pbRawRGBA);
		m_pbRawRGBA = NULL;
		m_cRawRGBA = 0;
	}

	// Full destroys stuff thats not codec dependent
	if (bFull) 
	{
		if (m_cudaCtxLock) 
		{
			cuvidCtxLockDestroy(m_cudaCtxLock);
			m_cudaCtxLock = 0;
		}

		if (m_cudaContext) 
		{
			cuCtxDestroy(m_cudaContext);
			m_cudaContext = 0;
		}

		if (m_pInteropFrame)
		{
			cuMemFree(m_pInteropFrame);
		}

		SafeRelease(&m_pD3DDevice);
		SafeRelease(&m_pD3D);
	}

	return S_OK;
}

#define LEVEL_C_LOW_LIMIT 0x0A20

static DWORD LevelCBlacklist[] = 
{
	0x0A22, 0x0A67,     // Geforce 315, no VDPAU at all
	0x0A68, 0x0A69,     // Geforce G105M, only B
	0x0CA0, 0x0CA7,     // Geforce GT 330, only A
	0x0CAC,             // Geforce GT 220, no VDPAU
	0x10C3              // Geforce 8400GS, only A
};

static DWORD LevelCWhitelist[] = 
{
	0x06C0,             // Geforce GTX 480
	0x06C4,             // Geforce GTX 465
	0x06CA,             // Geforce GTX 480M
	0x06CD,             // Geforce GTX 470
	0x06D1,             // Tesla C2050 / C2070
	0x06D2,             // Tesla M2070
	0x06DE,             // Tesla T20 Processor
	0x06DF,             // Tesla M2070-Q

	0x06D8, 0x06DC,     // Quadro 6000
	0x06D9,             // Quadro 5000
	0x06DA,             // Quadro 5000M
	0x06DD,             // Quadro 4000
};

static BOOL IsLevelC(DWORD deviceId)
{
	int idx = 0;
	if (deviceId >= LEVEL_C_LOW_LIMIT) 
	{
		for(idx = 0; idx < countof(LevelCBlacklist); idx++) 
		{
			if (LevelCBlacklist[idx] == deviceId)
				return FALSE;
		}
		return TRUE;
	} 
	else 
	{
		for(idx = 0; idx < countof(LevelCWhitelist); idx++) 
		{
			if (LevelCWhitelist[idx] == deviceId)
				return TRUE;
		}
		return FALSE;
	}
}

HRESULT CDeltaCudaH264Decoder::InitCUDA()
{
	CUresult cuStatus = CUDA_SUCCESS;
	HRESULT hr = S_OK;

	cuStatus = cuInit(0);
	if (cuStatus != CUDA_SUCCESS) 
	{
		return E_FAIL;
	}

	// Find the best CUDA capable adapter
	int best_device = cutilDrvGetMaxGflopsGraphicsDeviceId();
	int device = best_device;

	// we check to make sure we have found a cuda-compatible D3D device to work on
	if (best_device == -1)
	{
		DbgLog((LOG_CUSTOM1, 1, L"  No CUDA-compatible device available"));
		return E_FAIL;
	}

	m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!m_pD3D) 
	{
		DbgLog((LOG_ERROR, 10, L"InitCUDA(): Failed to acquire IDirect3D9"));
	} 
	else 
	{
		D3DADAPTER_IDENTIFIER9 d3dId;
		unsigned uAdapterCount = m_pD3D->GetAdapterCount();
		for (unsigned lAdapter=0; lAdapter<uAdapterCount; lAdapter++) 
		{
			// Create the D3D Display Device
			ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));

			m_d3dpp.Windowed               = TRUE;
			m_d3dpp.MultiSampleType        = D3DMULTISAMPLE_NONE;
			m_d3dpp.BackBufferWidth        = 1280;
			m_d3dpp.BackBufferHeight       = 720;
			m_d3dpp.BackBufferCount        = 1;
			m_d3dpp.BackBufferFormat       = D3DFMT_X8R8G8B8;
			m_d3dpp.SwapEffect             = D3DSWAPEFFECT_COPY;
			m_d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;
			m_d3dpp.Flags                  = D3DPRESENTFLAG_VIDEO;

			IDirect3DDevice9 *pDev = NULL;
			CUcontext cudaCtx = 0;
			hr = m_pD3D->CreateDevice(lAdapter, D3DDEVTYPE_HAL, GetDesktopWindow(), D3DCREATE_FPU_PRESERVE | D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &m_d3dpp, &pDev);
			if (SUCCEEDED(hr)) 
			{
				m_pD3D->GetAdapterIdentifier(lAdapter, 0, &d3dId);
				cuStatus = cuD3D9CtxCreate(&cudaCtx, &device, CU_CTX_SCHED_BLOCKING_SYNC | CU_CTX_MAP_HOST, pDev);
				if (cuStatus == CUDA_SUCCESS) 
				{
					DbgLog((LOG_TRACE, 10, L"InitCUDA(): Created D3D Device on adapter %S (%d), using CUDA device %d", d3dId.Description, lAdapter, device));
          
					BOOL isLevelC = IsLevelC(d3dId.DeviceId);
					DbgLog((LOG_TRACE, 10, L"InitCUDA(): D3D Device with Id 0x%x is level C: %d", d3dId.DeviceId, isLevelC));

					if (m_bVDPAULevelC && !isLevelC) 
					{
						DbgLog((LOG_TRACE, 10, L"InitCUDA(): We already had a Level C+ device, this one is not, skipping"));
						continue;
					}

					// Release old resources
					SafeRelease(&m_pD3DDevice);
					if (m_cudaContext)
						cuCtxDestroy(m_cudaContext);

					// Store resources
					m_pD3DDevice = pDev;
					m_cudaContext = cudaCtx;
					m_bVDPAULevelC = isLevelC;
					// Is this the one we want?
					if (device == best_device)
						break;
				} 
				else 
				{
					DbgLog((LOG_TRACE, 10, L"InitCUDA(): D3D Device on adapter %d is not CUDA capable", lAdapter));
					SafeRelease(&pDev);
				}
			}
		}
	}    

	cuStatus = CUDA_SUCCESS;

	if (!m_pD3DDevice) 
	{
		DbgLog((LOG_TRACE, 10, L"InitCUDA(): No D3D device available, building non-D3D context on device %d", best_device));

		SafeRelease(&m_pD3D);
		cuStatus = cuCtxCreate(&m_cudaContext, CU_CTX_SCHED_BLOCKING_SYNC | CU_CTX_MAP_HOST, best_device);
    
		int major, minor;
		cuDeviceComputeCapability(&major, &minor, best_device);
		m_bVDPAULevelC = (major >= 2);

		DbgLog((LOG_TRACE, 10, L"InitCUDA(): pure CUDA context of device with compute %d.%d", major, minor));
	}

	TCHAR szImagePath[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, szImagePath);
	int utf8Size = WideCharToMultiByte(CP_UTF8, 0, szImagePath, -1, NULL, 0, NULL, false);
	char* utf8Str = new char[utf8Size+1];
	WideCharToMultiByte(CP_UTF8, 0, szImagePath, -1, utf8Str, utf8Size, NULL, false);

#ifdef _EnCryFile_
#if 0
	string strFile = "\\kernel.ptx";
	string strPath(utf8Str);
	strPath.append(strFile);
	BSTR bstrPath = _com_util::ConvertStringToBSTR(strPath.c_str());
#endif

	string strEncryFile = "\\kernel.txt";
	string strEncryPath(utf8Str);
	strEncryPath.append(strEncryFile);

	string strDecryFile = "\\kernel_d.txt";
	string strDecryPath(utf8Str);
	strDecryPath.append(strDecryFile);

	BSTR bstrDecryPath = _com_util::ConvertStringToBSTR(strDecryPath.c_str());
	BSTR bstrEncryPath = _com_util::ConvertStringToBSTR(strEncryPath.c_str());

#if 0
	EnCryptFile((LPTSTR)bstrEncryPath, (LPTSTR)bstrDecryPath, DEF_PASSWORD);
#endif

	DeCryptFile((LPTSTR)bstrEncryPath, (LPTSTR)bstrDecryPath, DEF_PASSWORD);

	if(modInitCTX(strDecryPath.c_str()))
#else
	string strFile = "\\kernel.ptx";
	string strPath(utf8Str);
	strPath.append(strFile);

	if(modInitCTX(strPath.c_str()))
#endif
	{
		CUresult cuStatus = cuModuleGetFunction(&m_fpCuda, m_cuModule, "NV12ToARGB");

		cuMemAlloc(&m_pInteropFrame, m_nTargetWidth * m_nTargetHeight * 4);
	}

	delete [] utf8Str;
#ifdef _EnCryFile_
	DeleteFile((LPCTSTR)bstrDecryPath);
#endif

	if (cuStatus == CUDA_SUCCESS) 
	{
		// Switch to a floating context
		CUcontext curr_ctx = NULL;
		cuStatus = cuCtxPopCurrent(&curr_ctx);
		if (cuStatus != CUDA_SUCCESS) 
		{
			DbgLog((LOG_ERROR, 10, L"::InitCuda(): Storing context on the stack failed with error %d", cuStatus));
			return E_FAIL;
		}
		cuStatus = cuvidCtxLockCreate(&m_cudaCtxLock, m_cudaContext);
		if (cuStatus != CUDA_SUCCESS) 
		{
			DbgLog((LOG_ERROR, 10, L"::InitCUDA(): Creation of floating context failed with error %d", cuStatus));
			return E_FAIL;
		}
	} 
	else 
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CDeltaCudaH264Decoder::InitCodec(cudaVideoCodec cudaCodec)
{
	CUresult cuStatus = CUDA_SUCCESS;
	HRESULT hr = S_OK;

	if(!m_settings.bFormats[cudaCodec]) 
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	CMediaType mtIn = m_pInput->CurrentMediaType();

	if (!m_cudaContext) 
	{
		CHECK_HR(hr = InitCUDA());
	}

	DestroyCUDA(false);
	SAFE_DELETE(m_avc1Parser);

	if (!m_bVDPAULevelC && cudaCodec == cudaVideoCodec_MPEG4)
		return E_FAIL;

	memset(&m_DisplayQueue, 0, sizeof(m_DisplayQueue));
	for (int i = 0; i < DISPLAY_DELAY; i++)
	m_DisplayQueue[i].picture_index = -1;
	m_DisplayPos = 0;

	// Create the CUDA Video Parser
	CUVIDPARSERPARAMS oVideoParserParameters;
	memset(&oVideoParserParameters, 0, sizeof(CUVIDPARSERPARAMS));
	oVideoParserParameters.CodecType              = cudaCodec;
	oVideoParserParameters.ulMaxNumDecodeSurfaces = MAX_DECODE_FRAMES;
	oVideoParserParameters.ulMaxDisplayDelay      = DISPLAY_DELAY;	                   // this flag is needed so the parser will push frames out to the decoder as quickly as it can
	oVideoParserParameters.pUserData              = this;
	oVideoParserParameters.pfnSequenceCallback    = CDeltaCudaH264Decoder::HandleVideoSequence;    // Called before decoding frames and/or whenever there is a format change
	oVideoParserParameters.pfnDecodePicture       = CDeltaCudaH264Decoder::HandlePictureDecode;    // Called when a picture is ready to be decoded (decode order)
	oVideoParserParameters.pfnDisplayPicture      = CDeltaCudaH264Decoder::HandlePictureDisplay;   // Called whenever a picture is ready to be displayed (display order)
	oVideoParserParameters.ulErrorThreshold       = 0;

	CUVIDEOFORMATEX oVideoFormatEx;
	memset(&oVideoFormatEx, 0, sizeof(CUVIDEOFORMATEX));

	BITMAPINFOHEADER *bmi = NULL;
	formatTypeHandler(mtIn.Format(), mtIn.FormatType(), &bmi, NULL);

	if (*mtIn.FormatType() == FORMAT_MPEG2Video && (bmi->biCompression == MAKEFOURCC('a','v','c','1') || bmi->biCompression == MAKEFOURCC('A','V','C','1') || bmi->biCompression == MAKEFOURCC('C','C','V','1'))) 
	{
		m_avc1Parser = new CAVC1Parser();
		MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)mtIn.Format();
		if (FAILED(m_avc1Parser->ParseExtradata((const BYTE *)mp2vi->dwSequenceHeader, mp2vi->cbSequenceHeader, mp2vi->dwFlags))) 
		{
			SAFE_DELETE(m_avc1Parser);
		} 
		else 
		{
			m_avc1Parser->CopyExtra(oVideoFormatEx.raw_seqhdr_data, &oVideoFormatEx.format.seqhdr_data_length);
		}
	} 
	else 
	{
		getExtraData(mtIn.Format(), mtIn.FormatType(), oVideoFormatEx.raw_seqhdr_data, &oVideoFormatEx.format.seqhdr_data_length);
	}

	oVideoParserParameters.pExtVideoInfo = &oVideoFormatEx;
	CUresult oResult = cuvidCreateVideoParser(&m_hParser, &oVideoParserParameters);
	if (oResult != CUDA_SUCCESS) 
	{
		DbgLog((LOG_ERROR, 10, L"::InitCodec(): Creating parser for type %d failed with code %d", cudaCodec, oResult));
		hr = E_FAIL;
		goto done;
	}

	{
		CCtxAutoLock lck(m_cudaCtxLock);
		oResult = cuStreamCreate(&m_hStream, 0);
		if (oResult != CUDA_SUCCESS) 
		{
			DbgLog((LOG_ERROR, 10, L"::InitCodec(): Creating stream failed"));
			hr = E_FAIL;
			goto done;
		}
	}

	{
		m_nTargetWidth = bmi->biWidth;
		m_nTargetHeight = bmi->biHeight;
		RECT rcDisplayArea = {0, 0, bmi->biWidth, bmi->biHeight};
		CHECK_HR(hr = CreateCUVIDDecoder(cudaCodec, bmi->biWidth, bmi->biHeight, bmi->biWidth, bmi->biHeight, rcDisplayArea));
	}

	m_bForceSequenceUpdate = TRUE;
	m_bVC1EVO = FALSE;
	DecodeExtraData();

done:
	if (FAILED(hr)) 
	{
		DestroyCUDA(true);
	}

	return hr;
}

HRESULT CDeltaCudaH264Decoder::CreateCUVIDDecoder(cudaVideoCodec codec, DWORD dwWidth, DWORD dwHeight, DWORD dwDisplayWidth, DWORD dwDisplayHeight, RECT rcDisplayArea)
{
	CUVIDDECODECREATEINFO *dci = &m_VideoDecoderInfo;

	if (m_hDecoder) 
	{
		cuvidDestroyDecoder(m_hDecoder);
		m_hDecoder = 0;
	}

	memset(dci, 0, sizeof(CUVIDDECODECREATEINFO));
	dci->ulWidth = dwWidth;
	dci->ulHeight = dwHeight;
	dci->ulNumDecodeSurfaces = MAX_DECODE_FRAMES;
	dci->CodecType = codec;
	dci->ChromaFormat = cudaVideoChromaFormat_420;
	dci->OutputFormat = cudaVideoSurfaceFormat_NV12;
	dci->DeinterlaceMode = (cudaVideoDeinterlaceMode)m_settings.dwDeinterlace;
	dci->ulNumOutputSurfaces = 1;

	dci->ulTargetWidth = dwDisplayWidth;
	dci->ulTargetHeight = dwDisplayHeight;

	dci->display_area.left   = (short)rcDisplayArea.left;
	dci->display_area.right  = (short)rcDisplayArea.right;
	dci->display_area.top    = (short)rcDisplayArea.top;
	dci->display_area.bottom = (short)rcDisplayArea.bottom;

	dci->ulCreationFlags     = (m_pD3DDevice && m_settings.bDXVA) ? cudaVideoCreate_PreferDXVA : cudaVideoCreate_PreferCUVID;
	dci->vidLock             = m_cudaCtxLock;

	// create the decoder
	CUresult oResult = cuvidCreateDecoder(&m_hDecoder, dci);
	if (oResult != CUDA_SUCCESS) 
	{
		DbgLog((LOG_ERROR, 10, L"::InitCUDADecoder(): Creation of decoder for type %d failed with code %d", dci->CodecType, oResult));
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CDeltaCudaH264Decoder::DecodeExtraData()
{
	CUresult oResult;

	CMediaType mtIn = m_pInput->CurrentMediaType();

	// Init Decoder
	CUVIDSOURCEDATAPACKET pCuvidPacket;
	memset(&pCuvidPacket, 0, sizeof(pCuvidPacket));

	if (m_avc1Parser) 
	{
		m_avc1Parser->GetExtra((BYTE **)&pCuvidPacket.payload, (unsigned int *)&pCuvidPacket.payload_size);
	} 
	else 
	{
		getExtraDataPtr(mtIn.Format(), mtIn.FormatType(), (BYTE **)&pCuvidPacket.payload, (unsigned int *)&pCuvidPacket.payload_size);
	}

	if (pCuvidPacket.payload && pCuvidPacket.payload_size)
		oResult = cuvidParseVideoData(m_hParser, &pCuvidPacket);

	return S_OK;
}

int CUDAAPI CDeltaCudaH264Decoder::HandleVideoSequence(void *obj, CUVIDEOFORMAT *pFormat)
{
	DbgLog((LOG_CUSTOM1, 10, L"CUDA HandleVideoSequence called"));

	CDeltaCudaH264Decoder *filter = reinterpret_cast<CDeltaCudaH264Decoder *>(obj);

	CUVIDDECODECREATEINFO *dci = &filter->m_VideoDecoderInfo;

	if ((pFormat->codec != dci->CodecType)
		|| (pFormat->coded_width != dci->ulWidth)
		|| (pFormat->coded_height != dci->ulHeight)
		|| (pFormat->display_area.right != dci->ulTargetWidth)
		|| (pFormat->display_area.bottom != dci->ulTargetHeight)
		|| (pFormat->chroma_format != dci->ChromaFormat)
		|| (pFormat->display_aspect_ratio.x != filter->m_aspectX && filter->m_settings.bStreamAR)
		|| (pFormat->display_aspect_ratio.y != filter->m_aspectY && filter->m_settings.bStreamAR)
		|| filter->m_bForceSequenceUpdate)
	{
		filter->m_bForceSequenceUpdate = FALSE;
		RECT rcDisplayArea = {pFormat->display_area.left, pFormat->display_area.top, pFormat->display_area.right, pFormat->display_area.bottom};
		filter->CreateCUVIDDecoder(pFormat->codec, pFormat->coded_width, pFormat->coded_height, pFormat->display_area.right, pFormat->display_area.bottom, rcDisplayArea);

		filter->m_bInterlaced = !pFormat->progressive_sequence;
		filter->QueueFormatChange(pFormat);
	}

	return TRUE;
}

int CUDAAPI CDeltaCudaH264Decoder::HandlePictureDecode(void *obj, CUVIDPICPARAMS *cuvidpic)
{
	DbgLog((LOG_CUSTOM1, 10, L"CUDA HandlePictureDecode called"));

	CDeltaCudaH264Decoder *filter = reinterpret_cast<CDeltaCudaH264Decoder *>(obj);

	if (filter->m_bFlushing)
		return FALSE;

	int flush_pos = filter->m_DisplayPos;
	for (;;) 
	{
		bool frame_in_use = false;
		for (int i = 0; i < DISPLAY_DELAY; ++i)
		{
			if (filter->m_DisplayQueue[i].picture_index == cuvidpic->CurrPicIdx) 
			{
				frame_in_use = true;
				break;
			}
		}
		if (!frame_in_use)
		{
			// No problem: we're safe to use this frame
			break;
		}
		// The target frame is still pending in the display queue:
		// Flush the oldest entry from the display queue and repeat
		if (filter->m_DisplayQueue[flush_pos].picture_index >= 0) 
		{
			filter->Display(&filter->m_DisplayQueue[flush_pos]);
			filter->m_DisplayQueue[flush_pos].picture_index = -1;
		}

		flush_pos = (flush_pos + 1) % DISPLAY_DELAY;
	}

	__try 
	{
		CUresult cuStatus = cuvidDecodePicture(filter->m_hDecoder, cuvidpic);
#ifdef DEBUG
		if (cuStatus != CUDA_SUCCESS) 
		{
			DbgLog((LOG_ERROR, 10, L"::HandlePictureDecode(): cuvidDecodePicture returned error code %d", cuStatus));
		}
#endif
	} 
	__except(1) 
	{
		DbgLog((LOG_ERROR, 10, L"::HandlePictureDecode(): cuvidDecodePicture threw an exception"));
	}

	return TRUE;
}

int CUDAAPI CDeltaCudaH264Decoder::HandlePictureDisplay(void *obj, CUVIDPARSERDISPINFO *cuviddisp)
{
	DbgLog((LOG_TRACE, 10, L"CUDA HandlePictureDisplay called"));
	CDeltaCudaH264Decoder *filter = reinterpret_cast<CDeltaCudaH264Decoder *>(obj);

	if (filter->m_bFlushing) 
	{
		return FALSE;
	}

	if (filter->m_newFormat && FAILED(filter->ReconnectOutput())) 
	{
		return FALSE;
	}

	if (filter->m_VideoDecoderInfo.CodecType == cudaVideoCodec_VC1 || filter->m_VideoDecoderInfo.CodecType == cudaVideoCodec_MPEG4) 
	{
		REFERENCE_TIME rtDur = 0;
		CMediaType &mtOut = filter->m_pOutput->CurrentMediaType();
		formatTypeHandler(mtOut.Format(), mtOut.FormatType(), NULL, &rtDur);

		if (filter->m_rtStartCalc == INT64_MIN)
			filter->m_rtStartCalc = cuviddisp->timestamp;

		filter->m_faTime.Sample(filter->m_rtStartCalc - cuviddisp->timestamp);
		const REFERENCE_TIME avg = filter->m_faTime.Average();
		if (abs(avg) > rtDur) 
		{
			REFERENCE_TIME correction = rtDur;
			if (avg > 0) 
			{
				correction = -correction;
			}
			filter->m_rtStartCalc += correction;
			filter->m_faTime.OffsetValues(correction);

			DbgLog((LOG_TRACE, 10, L"::HandlePictureDisplay(): Correcting timestamp offset; avg: %I64d, correcting by: %I64d", avg, correction));
		}

		cuviddisp->timestamp = filter->m_rtStartCalc;

		filter->m_rtStartCalc += rtDur;
		if (filter->m_bInterlaced && filter->m_settings.bFrameDoubling && filter->m_settings.dwDeinterlace != cudaVideoDeinterlaceMode_Weave) 
		{
			filter->m_rtStartCalc += rtDur;
		}

		if (filter->m_VideoDecoderInfo.CodecType == cudaVideoCodec_VC1 && cuviddisp->repeat_first_field == 1 && !filter->m_bVC1EVO) 
		{
			DbgLog((LOG_TRACE, 10, L"::HandlePictureDisplay(): repeat_first_field is set, switching to VC-1 EVO mode"));

			filter->m_bVC1EVO = TRUE;
		}

		if (filter->m_bVC1EVO) 
		{
			filter->m_rtStartCalc += (rtDur + 1) / 2;
		}
	}

	// Drop samples with negative timestamps (preroll)
	if (cuviddisp->timestamp < 0)
		return TRUE;

	if (filter->m_DisplayQueue[filter->m_DisplayPos].picture_index >= 0) 
	{
		filter->Display(&filter->m_DisplayQueue[filter->m_DisplayPos]);
		filter->m_DisplayQueue[filter->m_DisplayPos].picture_index = -1;
	}

	filter->m_DisplayQueue[filter->m_DisplayPos] = *cuviddisp;
	filter->m_DisplayPos = (filter->m_DisplayPos + 1) % DISPLAY_DELAY;

	DbgLog((LOG_TRACE, 10, L"HandlePictureDisplay End"));

	return TRUE;
}

HRESULT CDeltaCudaH264Decoder::EndOfStream()
{
	DbgLog((LOG_TRACE, 10, L"STATE: EndOfStream received"));

	cuvidFlushParser(m_hParser);

	return __super::EndOfStream();
}

HRESULT CDeltaCudaH264Decoder::BeginFlush()
{
	DbgLog((LOG_TRACE, 10, L"::BeginFlush()"));

	m_bFlushing = TRUE;

	return __super::BeginFlush();
}

HRESULT CDeltaCudaH264Decoder::EndFlush()
{
	DbgLog((LOG_TRACE, 10, L"::EndFlush()"));

	m_bFlushing = FALSE;

	return __super::EndFlush();
}

HRESULT CDeltaCudaH264Decoder::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	DbgLog((LOG_TRACE, 10, L"::NewSegment(): rtStart: %I64d, rtStop: %I64d, rate: %.2f", tStart, tStop, dRate));

	CAutoLock cAutoLock(&m_csReceive);

	// Block delivery
	m_bFlushing = TRUE;

	// Flush everything out of the parser/decoder
	cuvidFlushParser(m_hParser);

	// Flush display queue
	for (int i=0; i<DISPLAY_DELAY; ++i) 
	{
		if (m_DisplayQueue[m_DisplayPos].picture_index >= 0) 
		{
			m_DisplayQueue[m_DisplayPos].picture_index = -1;
		}
		m_DisplayPos = (m_DisplayPos + 1) % DISPLAY_DELAY;
	}

	// Reset values
	m_rtStartPrev = 0;
	m_rtStopPrev = 0;
	m_rtStartCalc = INT64_MIN;

	m_bFlushing = FALSE;

	// Decode Extradata to init decoder
	DecodeExtraData();

	DbgLog((LOG_TRACE, 10, L"::NewSegment(): finished flushing"));

	return __super::NewSegment(tStart, tStop, dRate);
}

static CUresult safe_cuvidParseVideoData(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket)
{
	CUresult result = CUDA_ERROR_UNKNOWN;
	__try 
	{
		result = cuvidParseVideoData(obj, pPacket);
	}
	__except(1) 
	{
		DbgLog((LOG_ERROR, 10, L"cuvidParseVideoData threw an exception"));
	}

	return result;
}

HRESULT CDeltaCudaH264Decoder::Receive(IMediaSample *pIn)
{
	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;
	CUresult cuStatus;
	BYTE *pbBuffer = NULL;

	AM_SAMPLE2_PROPERTIES const *pProps = m_pInput->SampleProps();
	if(pProps->dwStreamId != AM_STREAM_MEDIA) 
	{
		return m_pOutput->Deliver(pIn);
	}

	AM_MEDIA_TYPE *pmt = NULL;
	if (SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt) 
	{
		DbgLog((LOG_TRACE, 10, L"::Receive(): Sample contained new media type"));

		cuvidFlushParser(m_hParser);
		CMediaType mt = *pmt;
		m_pInput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	BYTE *pDataIn = NULL;
	if(FAILED(hr = pIn->GetPointer(&pDataIn))) 
	{
		return hr;
	}

	long len = pIn->GetActualDataLength();

	CUVIDSOURCEDATAPACKET pCuvidPacket;
	memset(&pCuvidPacket, 0, sizeof(pCuvidPacket));

	if (m_avc1Parser) 
	{
		int buf_size = 0;
		hr = m_avc1Parser->Parse(&pbBuffer, &buf_size, pDataIn, len);

		pCuvidPacket.payload = pbBuffer;
		pCuvidPacket.payload_size = buf_size;
	} 
	else 
	{
		pCuvidPacket.payload = pDataIn;
		pCuvidPacket.payload_size = len;
	}

	REFERENCE_TIME rtStart = _I64_MIN, rtStop = _I64_MIN;
	hr = pIn->GetTime(&rtStart, &rtStop);
	if (SUCCEEDED(hr)) 
	{
		pCuvidPacket.flags |= CUVID_PKT_TIMESTAMP;
		pCuvidPacket.timestamp = rtStart;
	}

	if (pIn->IsDiscontinuity() == S_OK) 
	{
		pCuvidPacket.flags |= CUVID_PKT_DISCONTINUITY;
	}

	cuStatus = safe_cuvidParseVideoData(m_hParser, &pCuvidPacket);
	if (cuStatus != CUDA_SUCCESS) 
	{
		DbgLog((LOG_ERROR, 10, L"::Receive(): cuvidParseVideoData returned error %d", cuStatus));
	}

	SAFE_FREE(pbBuffer);

	return S_OK;
}

HRESULT CDeltaCudaH264Decoder::GetDeliveryBuffer(IMediaSample** pSample, BYTE** pData)
{
	HRESULT hr;

	if (m_newFormat && FAILED(hr = ReconnectOutput())) 
	{
		return hr;
	}

	*pData = NULL;
	if(	FAILED(hr = m_pOutput->GetDeliveryBuffer(pSample, NULL, NULL, 0)) || 
		FAILED(hr = (*pSample)->GetPointer(pData))) 
	{
		return hr;
	}

	AM_MEDIA_TYPE* pmt = NULL;
	if(SUCCEEDED((*pSample)->GetMediaType(&pmt)) && pmt) 
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
		pmt = NULL;
	}

	(*pSample)->SetDiscontinuity(FALSE);
	(*pSample)->SetSyncPoint(TRUE);

	return S_OK;
}

HRESULT CDeltaCudaH264Decoder::ReconnectOutput()
{
	CMediaType& mt = m_pOutput->CurrentMediaType();
	VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2 *)mt.Format();

	HRESULT hr = S_FALSE;

	BOOL bSizeChanged = FALSE;
	BOOL bFrameRateChanged = FALSE;

	if (m_newFormat) 
	{
		if (vih2->bmiHeader.biWidth != m_newFormat->display_area.right
			|| vih2->bmiHeader.biHeight != m_newFormat->display_area.bottom
			|| m_settings.bStreamAR && vih2->dwPictAspectRatioX != m_newFormat->display_aspect_ratio.x
			|| m_settings.bStreamAR && vih2->dwPictAspectRatioY != m_newFormat->display_aspect_ratio.y)
			bSizeChanged = TRUE;
		vih2->bmiHeader.biWidth = m_newFormat->display_area.right;
		vih2->bmiHeader.biHeight = m_newFormat->display_area.bottom;
		if (m_settings.bStreamAR) 
		{
			vih2->dwPictAspectRatioX = m_aspectX = m_newFormat->display_aspect_ratio.x;
			vih2->dwPictAspectRatioY = m_aspectY = m_newFormat->display_aspect_ratio.y;
		}

		if (m_bInterlaced && m_newFormat->frame_rate.numerator != 0 && m_newFormat->frame_rate.denominator != 0)
		{
			double dFrameTime = 10000000.0 / ((double)m_newFormat->frame_rate.numerator / m_newFormat->frame_rate.denominator);
			if (m_bInterlaced && m_settings.bFrameDoubling && m_settings.dwDeinterlace != cudaVideoDeinterlaceMode_Weave)
			dFrameTime /= 2.0;
			REFERENCE_TIME rtNew = (REFERENCE_TIME)floor(dFrameTime + 0.5);

			if (rtNew != vih2->AvgTimePerFrame)
			bFrameRateChanged = TRUE;
			vih2->AvgTimePerFrame = rtNew;
		}

		SetRect(&vih2->rcSource, 0, 0, m_newFormat->display_area.right, m_newFormat->display_area.bottom);
		SetRect(&vih2->rcTarget, 0, 0, m_newFormat->display_area.right, m_newFormat->display_area.bottom);

		vih2->bmiHeader.biSizeImage = vih2->bmiHeader.biWidth * vih2->bmiHeader.biHeight * vih2->bmiHeader.biBitCount >> 3;

		if(!bSizeChanged && !bFrameRateChanged)
			goto done;

		hr = m_pOutput->GetConnected()->QueryAccept(&mt);
		if(SUCCEEDED(hr = m_pOutput->GetConnected()->ReceiveConnection(m_pOutput, &mt))) 
		{
			IMediaSample *pOut = NULL;
			if (SUCCEEDED(hr = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0))) 
			{
				AM_MEDIA_TYPE *pmt = NULL;
				if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt) 
				{
					CMediaType mt = *pmt;
					m_pOutput->SetMediaType(&mt);
#ifdef DEBUG
					VIDEOINFOHEADER2 *vih2new = (VIDEOINFOHEADER2 *)mt.Format();
					DbgLog((LOG_TRACE, 10, L"New MediaType negotiated; actual width: %d - renderer requests: %ld", m_newFormat->display_area.right, vih2new->bmiHeader.biWidth));
#endif
					DeleteMediaType(pmt);
				} 
				else 
				{ // no request? Oh Well!
					DbgLog((LOG_TRACE, 10, L"We did not get a stride request, sending width no stride; width: %d", m_newFormat->display_area.right));
				}
				pOut->Release();
			}
		}

		if (bSizeChanged)
			NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(m_newFormat->display_area.right, m_newFormat->display_area.bottom), 0);

		hr = S_OK;
	}

done:
	SAFE_FREE(m_newFormat);
	return hr;
}

HRESULT CDeltaCudaH264Decoder::Display(CUVIDPARSERDISPINFO *cuviddisp)
{
	if (m_bInterlaced && m_settings.bFrameDoubling && m_settings.dwDeinterlace != cudaVideoDeinterlaceMode_Weave) 
	{
		if (cuviddisp->progressive_frame) 
		{
			Deliver(cuviddisp, 2);
		} 
		else 
		{
			Deliver(cuviddisp, 0);
			Deliver(cuviddisp, 1);
		}
	} 
	else 
	{
		Deliver(cuviddisp);
	}

	return S_OK;
}

HRESULT CDeltaCudaH264Decoder::Deliver(CUVIDPARSERDISPINFO *cuviddisp, int field)
{
	HRESULT hr = S_OK;

	CUdeviceptr devPtr = 0;
	unsigned int pitch = 0, width = 0, height = 0;
	CUVIDPROCPARAMS vpp;
	CUresult cuStatus = CUDA_SUCCESS;

	memset(&vpp, 0, sizeof(vpp));
	vpp.progressive_frame = cuviddisp->progressive_frame;

	if (m_settings.dwFieldOrder == 0)
		vpp.top_field_first = cuviddisp->top_field_first;
	else
		vpp.top_field_first = (m_settings.dwFieldOrder == 1);

	vpp.second_field = (field == 1);

	CCtxAutoLock lck(m_cudaCtxLock);
	cuCtxPushCurrent(m_cudaContext);

	cuStatus = cuvidMapVideoFrame(m_hDecoder, cuviddisp->picture_index, &devPtr, &pitch, &vpp);
	if (cuStatus != CUDA_SUCCESS) 
	{
		DbgLog((LOG_CUSTOM1, 1, L"cuvidMapVideoFrame failed on index %d", cuviddisp->picture_index));
		return E_FAIL;
	}

	width = m_VideoDecoderInfo.display_area.right;
	height = m_VideoDecoderInfo.display_area.bottom;

	CMediaType &mtOut = m_pOutput->CurrentMediaType();

	size_t nTexturePitch = width * 4;
	int size = pitch * height * 3 / 2;
	int sizergba = width * height * 4;
	if(mtOut.subtype == MEDIASUBTYPE_RGB24)
	{
		cudaNV12toARGB(&devPtr, pitch, &m_pInteropFrame, nTexturePitch, m_cuModule, m_fpCuda, m_hStream);

		if(!m_pbRawRGBA || sizergba > m_cRawRGBA) 
		{
			if (m_pbRawRGBA) 
			{
				cuMemFreeHost(m_pbRawRGBA);
				m_pbRawRGBA = NULL;
				m_cRawRGBA = 0;
			}
			cuStatus = cuMemAllocHost((void **)&m_pbRawRGBA, sizergba);
			if (cuStatus != CUDA_SUCCESS) 
			{
				DbgLog((LOG_CUSTOM1, 1, L"cuMemAllocHost failed to allocate %d bytes (%d)", sizergba, cuStatus));
			}
			m_cRawRGBA = sizergba;
		}
		if (m_pbRawRGBA) 
		{
#if USE_ASYNC_COPY
			cuStatus = cuMemcpyDtoHAsync(m_pbRawRGBA, m_pInteropFrame, sizergba, m_hStream);
			if (cuStatus != CUDA_SUCCESS) 
			{
				DbgLog((LOG_ERROR, 10, L"Async Memory Transfer failed"));
				return E_FAIL;
			}
			while (CUDA_ERROR_NOT_READY == cuStreamQuery(m_hStream)) 
			{
				Sleep(1);
			}
#else
			cuStatus = cuMemcpyDtoH(m_pbRawRGBA, m_pInteropFrame, sizergba);
#endif
		}
	}
	else
	{
		if(!m_pbRawNV12 || size > m_cRawNV12) 
		{
			if (m_pbRawNV12) 
			{
				cuMemFreeHost(m_pbRawNV12);
				m_pbRawNV12 = NULL;
				m_cRawNV12 = 0;
			}
			cuStatus = cuMemAllocHost((void **)&m_pbRawNV12, size);
			if (cuStatus != CUDA_SUCCESS) 
			{
				DbgLog((LOG_CUSTOM1, 1, L"cuMemAllocHost failed to allocate %d bytes (%d)", size, cuStatus));
			}
			m_cRawNV12 = size;
		}
		if (m_pbRawNV12) 
		{
#if USE_ASYNC_COPY
			cuStatus = cuMemcpyDtoHAsync(m_pbRawNV12, devPtr, size, m_hStream);
			if (cuStatus != CUDA_SUCCESS) 
			{
				DbgLog((LOG_ERROR, 10, L"Async Memory Transfer failed"));
				return E_FAIL;
			}
			while (CUDA_ERROR_NOT_READY == cuStreamQuery(m_hStream)) 
			{
				Sleep(1);
			}
#else
			cuStatus = cuMemcpyDtoH(m_pbRawNV12, devPtr, size);
#endif
		}
	}

	cuvidUnmapVideoFrame(m_hDecoder, devPtr);
	cuCtxPopCurrent(NULL);

	IMediaSample *pOut;
	BYTE *pDataOut = NULL;
	if(FAILED(GetDeliveryBuffer(&pOut, &pDataOut)))
	{
		return E_FAIL;
	}

	REFERENCE_TIME rtDur = 0;
	BITMAPINFOHEADER *bmi = NULL;
	formatTypeHandler(mtOut.Format(), mtOut.FormatType(), &bmi, &rtDur);

	int out_pitch = bmi->biWidth;
	int out_size = out_pitch * height * 3 / 2;
	REFERENCE_TIME rtStart, rtStop;
	rtStart = cuviddisp->timestamp;
	if (field == 1) 
	{
		rtStart += rtDur;
	}

	rtStop = rtStart + rtDur;
	if (field == 2)
		rtStop += rtDur;

	DbgLog((LOG_TRACE, 10, L"::Deliver(): Delivering frame at %I64d, diff: %I64d, avg: %I64d", rtStart, rtStart-m_rtStartPrev, m_faTime.Average()));
	m_rtStartPrev = rtStart;
	m_rtStopPrev = rtStop;

	pOut->SetTime(&rtStart, &rtStop);
	pOut->SetMediaTime(NULL, NULL);

	if (mtOut.subtype == MEDIASUBTYPE_YV12) 
	{
		NV12ToYV12(pDataOut, m_pbRawNV12, width, height, pitch, out_pitch);
	} 
	else if (mtOut.subtype == MEDIASUBTYPE_NV12) 
	{
		if (pitch == out_pitch) 
		{
			memcpy(pDataOut, m_pbRawNV12, size);
		} 
		else 
		{
			NV12ChangeStride(pDataOut, m_pbRawNV12, height, pitch, out_pitch);
		}
	} 
	else if(mtOut.subtype == MEDIASUBTYPE_RGB24)
	{
		RGBTRIPLE *prgb = (RGBTRIPLE*)pDataOut;
		RGBQUAD *prgba = (RGBQUAD*)m_pbRawRGBA;
		for(int j = 0; j < height; ++j)
		{
			for(int i = 0; i < width; ++i)
			{
				prgb[j*width+i].rgbtBlue = prgba[(height-1-j)*width+i].rgbBlue;
				prgb[j*width+i].rgbtGreen = prgba[(height-1-j)*width+i].rgbGreen;
				prgb[j*width+i].rgbtRed = prgba[(height-1-j)*width+i].rgbRed;
			}
		}
	}
	else 
	{
		ASSERT(0);
	}

	hr = m_pOutput->Deliver(pOut);
	SafeRelease(&pOut);

	return hr;
}

STDMETHODIMP_(DWORD) CDeltaCudaH264Decoder::GetDeinterlaceMode()
{
	return m_settings.dwDeinterlace;
}

STDMETHODIMP CDeltaCudaH264Decoder::SetDeinterlaceMode(DWORD dwMode)
{
	m_settings.dwDeinterlace = dwMode;
	SaveSettings();
	return S_OK;
}

STDMETHODIMP_(BOOL) CDeltaCudaH264Decoder::GetFrameDoubling()
{
	return m_settings.bFrameDoubling;
}

STDMETHODIMP CDeltaCudaH264Decoder::SetFrameDoubling(BOOL bFrameDoubling)
{
	m_settings.bFrameDoubling = bFrameDoubling;
	SaveSettings();
	return S_OK;
}

STDMETHODIMP_(DWORD) CDeltaCudaH264Decoder::GetFieldOrder()
{
	return m_settings.dwFieldOrder;
}

STDMETHODIMP CDeltaCudaH264Decoder::SetFieldOrder(DWORD dwFieldOrder)
{
	m_settings.dwFieldOrder = dwFieldOrder;
	SaveSettings();
	return S_OK;
}

STDMETHODIMP CDeltaCudaH264Decoder::GetFormatConfiguration(bool *bFormat)
{
	CheckPointer(bFormat, E_POINTER);

	memcpy(bFormat, &m_settings.bFormats, sizeof(m_settings.bFormats));
	return S_OK;
}

STDMETHODIMP CDeltaCudaH264Decoder::SetFormatConfiguration(bool *bFormat)
{
	CheckPointer(bFormat, E_POINTER);

	memcpy(&m_settings.bFormats, bFormat, sizeof(m_settings.bFormats));
	SaveSettings();
	return S_OK;
}

STDMETHODIMP_(BOOL) CDeltaCudaH264Decoder::GetStreamAR()
{
	return m_settings.bStreamAR;
}

STDMETHODIMP CDeltaCudaH264Decoder::SetStreamAR(BOOL bFlag)
{
	m_settings.bStreamAR = bFlag;
	SaveSettings();
	return S_OK;
}

STDMETHODIMP_(BOOL) CDeltaCudaH264Decoder::GetDXVA()
{
	return m_settings.bDXVA;
}

STDMETHODIMP CDeltaCudaH264Decoder::SetDXVA(BOOL bDXVA)
{
	m_settings.bDXVA = bDXVA;
	SaveSettings();
	return S_OK;
}

STDMETHODIMP_(DWORD) CDeltaCudaH264Decoder::GetOutputFormat()
{
	return m_settings.dwOutput;
}

STDMETHODIMP CDeltaCudaH264Decoder::SetOutputFormat(DWORD dwOutput)
{
	m_settings.dwOutput = dwOutput;
	SaveSettings();
	return S_OK;
}

STDMETHODIMP CDeltaCudaH264Decoder::Run(REFERENCE_TIME tStart)
{
	return CBaseFilter::Run(tStart);
}


STDMETHODIMP CDeltaCudaH264Decoder::Stop(void)
{
	return CBaseFilter::Stop();
}


STDMETHODIMP CDeltaCudaH264Decoder::Pause(void)
{
	return CBaseFilter::Pause();
}

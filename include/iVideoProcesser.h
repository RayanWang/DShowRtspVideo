#pragma once

#define COLORCHART_FAILED	WM_USER + 0x0200

#define STANDARD_CALLBACK		0
#define CALIBRATED_CALLBACK		1
#define STANDARD_POLYNOMIAL		2
#define CALIBRATED_POLYNOMIAL	3
#define STANDARD_SIGMOID		4
#define CALIBRATED_SIGMOID		5

typedef enum ColorSpace
{
	Space_RGB		= 0,
	Space_RGB_XYZ	= 1,
	Space_XYZ		= 2,
	Space_Lab		= 3,
	Space_HSL		= 4,
} ColorSpace;

// {F2F3C837-E4C1-4FC7-B8F3-E1647A497D08}
static const GUID  CLSID_DeltaVideoProcesser = 
{ 0xf2f3c837, 0xe4c1, 0x4fc7, { 0xb8, 0xf3, 0xe1, 0x64, 0x7a, 0x49, 0x7d, 0x8 } };

typedef int (__cdecl __RPC_FAR *CCMCallback)(long lMainEventType, long lSubEventType, void* pReserved, void* pData);

typedef struct BoundaryParam
{
	int		nMinColorCheckerArea;
	int		nMaxArea;
	int		nMinArea;
	float	fMaxRatio;
	float	fMinRatio;

	BoundaryParam(){}
	~BoundaryParam(){}

	void operator = (BoundaryParam &oBoundaryParam)
    {
		nMinColorCheckerArea	= oBoundaryParam.nMinColorCheckerArea;
		nMaxArea				= oBoundaryParam.nMaxArea;
		nMinArea				= oBoundaryParam.nMinArea;
		fMaxRatio				= oBoundaryParam.fMaxRatio;
		fMinRatio				= oBoundaryParam.fMinRatio;
    };
} BoundaryParam;

const TCHAR* VIDEO_PROCESS_FILTER_NAME = L"Delta Video Process Filter";

#ifdef __cplusplus
extern "C" {
#endif

	// {E3ADEBC8-AD6B-43A5-A61F-D70318EA6AB2}
	DEFINE_GUID(IID_ICameraCalibration, 
	0xe3adebc8, 0xad6b, 0x43a5, 0xa6, 0x1f, 0xd7, 0x3, 0x18, 0xea, 0x6a, 0xb2);

    DECLARE_INTERFACE_(ICameraCalibration, IUnknown)
    {
		STDMETHOD(Initialize) (THIS_
					BOOL		bIsStandardCamera
                 ) PURE;

		STDMETHOD(EnableCalibrate) (THIS_
                    BOOL		bEnable
                 ) PURE;

		STDMETHOD(EnableDemodulate) (THIS_
                    BOOL		bEnable
                 ) PURE;

		STDMETHOD(SetColorSpace) (THIS_
                    ColorSpace	color
                 ) PURE;

		STDMETHOD(GetCCMatrix) (THIS_
                    double		(&CCMatrix)[3][3]
                 ) PURE;

		STDMETHOD(SetCallback) (THIS_
                    CCMCallback fpCCMCallback, 
					LPVOID		pUserParam
                 ) PURE;

		STDMETHOD(SetChartBoundary) (THIS_
                    BoundaryParam oChartBoundary
                 ) PURE;

		STDMETHOD(SetRegressionAlg) (THIS_
                    BSTR		bstrAlg
                 ) PURE;

		STDMETHOD(SnapShot) (THIS_
					BSTR		bstrImgPath
                 ) PURE;
    };

#ifdef __cplusplus
}
#endif
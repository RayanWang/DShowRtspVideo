#pragma once

#include "colorspace.h"

class CColorSpaceRGB : public CColorSpace
{
private:
	DECLARE_RUNTIME(CColorSpaceRGB);

public:
	CColorSpaceRGB(void);
	virtual ~CColorSpaceRGB(void);

	static void* CreateInstance()
	{
		return new CColorSpaceRGB;
	}

	BOOL CalibratedRGB(RGBTRIPLE *prgbIpCamRGB);

	BOOL CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB);
};


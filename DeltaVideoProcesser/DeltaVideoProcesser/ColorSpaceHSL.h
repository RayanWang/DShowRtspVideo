#pragma once

#include "colorspace.h"

class CColorSpaceHSL : public CColorSpace
{
private:
	DECLARE_RUNTIME(CColorSpaceHSL);

public:
	CColorSpaceHSL(void);
	virtual ~CColorSpaceHSL(void);

	static void* CreateInstance()
	{
		return new CColorSpaceHSL;
	}

	BOOL CalibratedRGB(RGBTRIPLE *prgbIpCamRGB);

	BOOL CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB);
};


#pragma once

#include "colorspace.h"

class CColorSpaceLab : public CColorSpace
{
private:
	DECLARE_RUNTIME(CColorSpaceLab);

public:
	CColorSpaceLab(void);
	virtual ~CColorSpaceLab(void);

	static void* CreateInstance()
	{
		return new CColorSpaceLab;
	}

	BOOL CalibratedRGB(RGBTRIPLE *prgbIpCamRGB);

	BOOL CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB);
};


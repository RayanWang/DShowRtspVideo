#pragma once

#include "colorspace.h"

class CColorSpaceXYZ : public CColorSpace
{
private:
	DECLARE_RUNTIME(CColorSpaceXYZ);

public:
	CColorSpaceXYZ(void);
	virtual ~CColorSpaceXYZ(void);

	static void* CreateInstance()
	{
		return new CColorSpaceXYZ;
	}

	BOOL CalibratedRGB(RGBTRIPLE *prgbIpCamRGB);

	BOOL CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB);
};


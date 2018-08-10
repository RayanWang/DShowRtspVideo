#pragma once

#include "colorspace.h"

class CColorSpaceRGB2XYZ : public CColorSpace
{
private:
	DECLARE_RUNTIME(CColorSpaceRGB2XYZ);

public:
	CColorSpaceRGB2XYZ(void);
	virtual ~CColorSpaceRGB2XYZ(void);

	static void* CreateInstance()
	{
		return new CColorSpaceRGB2XYZ;
	}

	BOOL CalibratedRGB(RGBTRIPLE *prgbIpCamRGB);

	BOOL CalculateCCMatrix(RGBTRIPLE *prgbPatchStandardRGB, RGBTRIPLE *prgbPatchIpCamRGB);
};


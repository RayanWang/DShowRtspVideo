#pragma once

#include "regression.h"

class CRegressionLeastSquare : public CRegression
{
private:
	DECLARE_REGRESSION_RUNTIME(CRegressionLeastSquare);

public:
	CRegressionLeastSquare(void);
	virtual ~CRegressionLeastSquare(void);

	static void* CreateInstance()
	{
		return new CRegressionLeastSquare;
	}

	BOOL GetCCMfromRegression(double **ppStandardMatrix,
							  double **ppPatchIpCam,
							  int	 nStandardPatch,
							  double (&MatrixIpCam)[3][3]);

private:
	// Calculate the inverse matrix
	void FindMatrixInverse(const double (&Matrix)[3][3], double (&MatrixInverse)[3][3]);
};


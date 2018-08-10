#pragma once

#include "regression.h"

class CRegressionElmLinear : public CRegression
{
private:
	DECLARE_REGRESSION_RUNTIME(CRegressionElmLinear);

public:
	CRegressionElmLinear(void);
	virtual ~CRegressionElmLinear(void);

	static void* CreateInstance()
	{
		return new CRegressionElmLinear;
	}
};


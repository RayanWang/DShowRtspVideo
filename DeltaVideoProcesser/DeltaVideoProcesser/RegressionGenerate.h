#pragma once

#include <string>

using namespace std;

class CRegression;

class CRegressionGenerate
{
public:
	CRegressionGenerate(void);
	virtual ~CRegressionGenerate(void);

	CRegression* CreateRegressionObject(string Alg);
};


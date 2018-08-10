#pragma once

#include <string>

using namespace std;

class CColorSpace;

class CColorGenerate
{
public:
	CColorGenerate(void);
	virtual ~CColorGenerate(void);

	CColorSpace* CreateColorObject(void);

	void SetColorSpace(string name) { m_strClassName = name; }

private:
	string	m_strClassName;
};


#pragma once

#include <string>
#include <map>

using namespace std;

typedef void* (*CreateRegressionClass)(void);

class RegressionClassFactory
{
public:
	RegressionClassFactory(){};

	static void* GetClassByName(string name)
	{
		map<string, CreateRegressionClass>::const_iterator iter;
		iter = m_classMap.find(name);
		if(iter == m_classMap.end())
			return NULL;
		else
			return iter->second();
	}

	static void RegistClass(string name, CreateRegressionClass method)
	{
		m_classMap.insert(pair<string, CreateRegressionClass>(name, method));
	}

private:
	static map<string, CreateRegressionClass> m_classMap;
};
#pragma once

#include <string>
#include <map>

using namespace std;

typedef void* (*CreateClass)(void);

class ClassFactory
{
public:
	ClassFactory(){};

	static void* GetClassByName(string name)
	{
		map<string, CreateClass>::const_iterator iter;
		iter = m_classMap.find(name);
		if(iter == m_classMap.end())
			return NULL;
		else
			return iter->second();
	}

	static void RegistClass(string name, CreateClass method)
	{
		m_classMap.insert(pair<string, CreateClass>(name, method));
	}

private:
	static map<string, CreateClass> m_classMap;
};
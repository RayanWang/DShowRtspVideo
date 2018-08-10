#pragma once

bool CreateRegistryKey(HKEY hKeyRoot, LPCTSTR pszSubKey);

class CRegistry
{
public:
	CRegistry();
	CRegistry(HKEY hkeyRoot, LPCTSTR pszSubKey, HRESULT &hr);
	~CRegistry();
  
	HRESULT Open(HKEY hkeyRoot, LPCTSTR pszSubKey);

	std::wstring ReadString(LPCTSTR pszKey, HRESULT &hr);
	HRESULT WriteString(LPCTSTR pszKey, LPCTSTR pszValue);

	DWORD ReadDWORD(LPCTSTR pszKey, HRESULT &hr);
	HRESULT WriteDWORD(LPCTSTR pszKey, DWORD dwValue);

	BOOL ReadBOOL(LPCTSTR pszKey, HRESULT &hr);
	HRESULT WriteBOOL(LPCTSTR pszKey, BOOL bValue);

	BYTE *ReadBinary(LPCTSTR pszKey, DWORD &dwSize, HRESULT &hr);
	HRESULT WriteBinary(LPCTSTR pszKey, const BYTE *pbValue, int iLen);

private:
	HKEY *m_key;
};

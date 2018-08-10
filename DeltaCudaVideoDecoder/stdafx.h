#pragma once

#define QI(i) (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :
#define QI2(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :
#define countof( array ) ( sizeof( array )/sizeof( array[0] ) )

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <Commctrl.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <tchar.h>
#include <comutil.h> 
#include <d3d9.h>
#include <DShow.h>
#include <streams.h>

#ifdef USE_CUDA31_API
#define CUDA_FORCE_API_VERSION 3010
#endif

#include <cuda.h>
#include <cudad3d9.h>
#include <nvcuvid.h>

// SafeRelease Template, for type safety
template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }
#define SAFE_FREE(pPtr) { free(pPtr); pPtr = NULL; }
#define SAFE_DELETE(pPtr) { delete pPtr; pPtr = NULL; }
#define SAFE_CO_FREE(pPtr) { CoTaskMemFree(pPtr); pPtr = NULL; }

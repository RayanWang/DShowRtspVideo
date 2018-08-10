#include "stdafx.h"
#include "GlobalAPI.h"
#include "otheruuids.h"

#include <emmintrin.h>

// Enable MMX YV12 conversion
#define YV12_SSE2_INTRIN 1
#ifdef _M_X64
#define YV12_MMX 0
#else
#define YV12_MMX 1
#endif

void formatTypeHandler(const BYTE *format, const GUID *formattype, BITMAPINFOHEADER **pBMI, REFERENCE_TIME *prtAvgTime)
{
	REFERENCE_TIME rtAvg = 0;
	BITMAPINFOHEADER *bmi = NULL;

	if (*formattype == FORMAT_VideoInfo) 
	{
		VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)format;
		rtAvg = vih->AvgTimePerFrame;
		bmi = &vih->bmiHeader;
	} 
	else if (*formattype == FORMAT_VideoInfo2) 
	{
		VIDEOINFOHEADER2 *vih2 = (VIDEOINFOHEADER2 *)format;
		rtAvg = vih2->AvgTimePerFrame;
		bmi = &vih2->bmiHeader;
	} 
	else if (*formattype == FORMAT_MPEG2Video) 
	{
		MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)format;
		rtAvg = mp2vi->hdr.AvgTimePerFrame;
		bmi = &mp2vi->hdr.bmiHeader;
	}

	if (pBMI) 
	{
		*pBMI = bmi;
	}

	if (prtAvgTime) 
	{
		*prtAvgTime = rtAvg;
	}
}

void getExtraData(const BYTE *format, const GUID *formattype, BYTE *extra, unsigned int *extralen)
{
	if (*formattype == FORMAT_VideoInfo || *formattype == FORMAT_VideoInfo2) 
	{
		BITMAPINFOHEADER *bmi = NULL;
		formatTypeHandler(format, formattype, &bmi, NULL);
		memcpy(extra, (BYTE *)bmi + sizeof(BITMAPINFOHEADER), bmi->biSize - sizeof(BITMAPINFOHEADER));
		*extralen = bmi->biSize - sizeof(BITMAPINFOHEADER);
	} 
	else if (*formattype == FORMAT_MPEG2Video) 
	{
		MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)format;
		memcpy(extra, &mp2vi->dwSequenceHeader[0], mp2vi->cbSequenceHeader);
		*extralen = mp2vi->cbSequenceHeader;
	}
}

void getExtraDataPtr(const BYTE *format, const GUID *formattype, BYTE **extra, unsigned int *extralen)
{
	if (*formattype == FORMAT_VideoInfo || *formattype == FORMAT_VideoInfo2) 
	{
		BITMAPINFOHEADER *bmi = NULL;
		formatTypeHandler(format, formattype, &bmi, NULL);
		*extra = (BYTE *)bmi + sizeof(BITMAPINFOHEADER);
		*extralen = bmi->biSize - sizeof(BITMAPINFOHEADER);
	} 
	else if (*formattype == FORMAT_MPEG2Video) 
	{
		MPEG2VIDEOINFO *mp2vi = (MPEG2VIDEOINFO *)format;
		*extra = (BYTE *)&mp2vi->dwSequenceHeader[0];
		*extralen = mp2vi->cbSequenceHeader;
	}
}

// This can hopefully be made alot faster at some point.
void NV12ChangeStride(BYTE *pOut, const BYTE *pIn, int h, int srcStride, int dstStride)
{
	int i, w = min(srcStride, dstStride);
	for (i = 0; i < (h+h/2); i++) 
	{
		memcpy(pOut, pIn, w);
		pIn += srcStride;
		pOut += dstStride;
	}
}

#define ALIGN(x,a) (((x)+(a)-1UL)&~((a)-1UL))

#if YV12_MMX
// constant for U/V masking
__declspec(align(8)) static const __int64 bm01_64 = 0x00FF00FF00FF00FFLL;
#endif

#if YV12_SSE2_INTRIN
__declspec(align(16)) static const __int64 bm01_128[2] = { 0x00FF00FF00FF00FFLL, 0x00FF00FF00FF00FFLL };
#endif

void NV12ToYV12(BYTE *pOut, const BYTE *pIn, int w, int h, int srcStride, int dstStride)
{
	// Copy Y, adjusting stride if neccessary
	if (srcStride == dstStride) {
	memcpy(pOut, pIn, srcStride * h);
	pIn  += srcStride * h;
	pOut += srcStride * h;
	} else {
	int i, w = min(srcStride, dstStride);
	for (i = 0; i < h; i++) {
		memcpy(pOut, pIn, w);
		pIn  += srcStride;
		pOut += dstStride;
	}
	}
	// unpack U/V
	const int dstUVstride = dstStride / 2;
	const int halfWidth = w / 2;

	const BYTE *inUV = pIn;
	BYTE *outV = pOut;
	BYTE *outU = outV + (dstUVstride * h/2);

#if YV12_SSE2_INTRIN
	// Check if input and output stride are 128-bit aligned for SSE usage
	// The CUVID decoder always aligns the image appropriately, however any post-processor
	// or decoder coming after this may not be so nice.
	int alignedWidth     = ALIGN(w, 32);
	int unalignedBytes   = 0;
	if(alignedWidth > srcStride || alignedWidth > dstStride) 
	{
		alignedWidth = w & ~31;
		unalignedBytes = w - alignedWidth;
	}

	// SSE2 NV12->YV12 conversion using Intrinsics
	__m128i mask = *(const __m128i *)bm01_128;
	for(int line = 0; line < h/2; ++line) 
	{
		const __m128i *src =  (const __m128i *)(inUV + srcStride * line);
		__m128i *dstV = (__m128i *)(outV + dstUVstride * line);
		__m128i *dstU = (__m128i *)(outU + dstUVstride * line);
		for (int i = 0; i < alignedWidth; i += 32) 
		{
			// Load 128-bit data operands (we assume the source is always properly aligned)
			__m128i m0 = *src++;
			__m128i m1 = *src++;
			__m128i m2 = m0;
			__m128i m3 = m1;
			// null out the high-order bytes to get the U values
			m0 = _mm_and_si128(m0, mask);
			m1 = _mm_and_si128(m1, mask);
			// right shift the V values
			m2 = _mm_srli_epi16(m2, 8);
			m3 = _mm_srli_epi16(m3, 8);
			// unpack the values
			m0 = _mm_packus_epi16(m0, m1);
			m2 = _mm_packus_epi16(m2, m3);

			// Check if the memory is properly aligned to support aligned memory instructions
			if (unalignedBytes) {
			_mm_storeu_si128(dstU, m0);
			_mm_storeu_si128(dstV, m2);
			} else {
			_mm_stream_si128(dstU, m0);
			_mm_stream_si128(dstV, m2);
			}
			dstU++;
			dstV++;
		}
		if (unalignedBytes) 
		{
			const BYTE *src = inUV + srcStride * line;
			BYTE *dstV = outV + dstUVstride * line;
			BYTE *dstU = outU + dstUVstride * line;

			for (int i = alignedWidth / 2; i < halfWidth; i++) 
			{
				dstU[i] = src[2*i+0];
				dstV[i] = src[2*i+1];
			}
		}
	}
#elif YV12_MMX
	__asm movq mm4, bm01_64;

	for(int line = 0; line < h/2; ++line) 
	{
		const BYTE *src = inUV + srcStride * line;
		BYTE *dstV = outV + dstUVstride * line;
		BYTE *dstU = outU + dstUVstride * line;
		__asm 
		{
			// Base source address
			mov      esi, [src];
			// Base destination Addresses
			mov      eax, [dstU];
			mov      ebx, [dstV];
			// Counter
			mov      ecx, 0h;
		_start:
			// Note: byte order in registers is inversed, so its V2 U2 V1 U1
			// Load 64-bit operands
			movq     mm0, [esi+ecx*2h];     // VUVU
			movq     mm1, [esi+ecx*2h+8h];  // VUVU
			movq     mm2, mm0;              // VUVU
			movq     mm3, mm1;              // VUVU
			// null out the high-order bytes to get the U values
			pand     mm0, mm4;              // 0U0U
			pand     mm1, mm4;              // 0U0U
			// right shift the V values
			psrlw    mm2, 8h;               // 0V0V
			psrlw    mm3, 8h;               // 0V0V
			// unpack the values
			packuswb mm0, mm1;              // UUUU
			packuswb mm2, mm3;              // VVVV
			// Move back into memory
			movq     [eax+ecx], mm0;
			movq     [ebx+ecx], mm2;
			// Counter & Loop
			add      ecx, 8h;
			cmp      ecx, halfWidth;
			jl       _start;
		}
	}

	// Clear MMX registers
	__asm emms;
#else
	for(int line = 0; line < h/2; ++line) 
	{
		const BYTE *src = inUV + srcStride * line;
		BYTE *dstV = outV + dstUVstride * line;
		BYTE *dstU = outU + dstUVstride * line;

		for (int i = 0; i < halfWidth; i++) 
		{
			dstU[i] = src[2*i+0];
			dstV[i] = src[2*i+1];
		}
	}
#endif
}

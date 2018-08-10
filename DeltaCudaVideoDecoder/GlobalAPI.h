#pragma once

void formatTypeHandler(const BYTE *format, const GUID *formattype, BITMAPINFOHEADER **pBMI, REFERENCE_TIME *prtAvgTime);
void getExtraData(const BYTE *format, const GUID *formattype, BYTE *extra, unsigned int *extralen);
void getExtraDataPtr(const BYTE *format, const GUID *formattype, BYTE **extra, unsigned int *extralen);

void NV12ChangeStride(BYTE *pOut, const BYTE *pIn, int h, int srcStride, int dstStride);
void NV12ToYV12(BYTE *pOut, const BYTE *pIn, int w, int h, int srcStride, int dstStride);

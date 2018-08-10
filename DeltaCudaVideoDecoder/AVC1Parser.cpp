#include "stdafx.h"
#include "AVC1Parser.h"

#include "stdint.h"

/************************************************************************/
/* DISCLAIMER:                                                          */
/* This code is based on FFmpeg (libavcodec/h264_mp4toannexb_bsf.c)     */
/* Licensed under the LGPL v2.1                                         */
/************************************************************************/

#define AV_RB16(x)                        \
  ((((const uint8_t*)(x))[0] << 8) |      \
  ((const uint8_t*)(x))[1])

#define AV_RB32(x)                        \
  ((((const uint8_t*)(x))[0] << 24) |     \
  (((const uint8_t*)(x))[1] << 16) |      \
  (((const uint8_t*)(x))[2] <<  8) |      \
  ((const uint8_t*)(x))[3])

#define AV_WB32(p, d) do {                \
  ((uint8_t*)(p))[3] = (d);               \
  ((uint8_t*)(p))[2] = (d)>>8;            \
  ((uint8_t*)(p))[1] = (d)>>16;           \
  ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)

#define INPUT_PADDING 8

static HRESULT alloc_and_copy(uint8_t **poutbuf, int *poutbuf_size, const uint8_t *in, uint32_t in_size) 
{
	uint32_t offset = *poutbuf_size;
	uint8_t nal_header_size = offset ? 3 : 4;
	void *tmp;

	*poutbuf_size += in_size+nal_header_size;
	tmp = realloc(*poutbuf, *poutbuf_size);
	if (!tmp)
		return E_POINTER;
	*poutbuf = (uint8_t *)tmp;
	memcpy(*poutbuf+nal_header_size+offset, in, in_size);
	if (!offset) 
	{
		AV_WB32(*poutbuf, 1);
	} 
	else 
	{
		(*poutbuf+offset)[0] = (*poutbuf+offset)[1] = 0;
		(*poutbuf+offset)[2] = 1;
	}

	return 0;
}

CAVC1Parser::CAVC1Parser()
  : m_pExtra(NULL)
  , m_nNALUSize(0)
{
}

CAVC1Parser::~CAVC1Parser()
{
	SAFE_FREE(m_pExtra);
}

HRESULT CAVC1Parser::ParseExtradata(const BYTE *extra, int extralen, int nalu_length)
{
	m_nNALUSize = nalu_length;
	if (nalu_length > 4 || nalu_length < 1)
		return E_FAIL;

	uint8_t unit_size;
	unsigned total_size = 0;
	uint8_t *out = NULL;
	const uint8_t *extradata = extra;
	static const uint8_t nalu_header[4] = {0, 0, 0, 1};

	const uint8_t *end = extra + extralen;

	while(extradata+2 < end) 
	{
		void *tmp;

		unit_size = AV_RB16(extradata);
		total_size += unit_size+4;

		if(total_size > INT_MAX || extradata + 2 + unit_size > end) 
		{
			SAFE_FREE(out);
			return E_FAIL;
		}

		tmp = realloc(out, total_size + INPUT_PADDING);
		if (!tmp) 
		{
			SAFE_FREE(out);
			return E_POINTER;
		}
		out = (uint8_t *)tmp;
		memcpy(out+total_size-unit_size-4, nalu_header, 4);
		memcpy(out+total_size-unit_size,   extradata+2, unit_size);
		extradata += 2+unit_size;
	}

	if(out)
		memset(out + total_size, 0, INPUT_PADDING);

	m_pExtra = out;
	m_nExtra = total_size;

	return S_OK;
}

HRESULT CAVC1Parser::CopyExtra(BYTE *buf, unsigned int *buf_size)
{
	CheckPointer(buf, E_POINTER);
	CheckPointer(buf_size, E_POINTER);
  
	memcpy(buf, m_pExtra, m_nExtra);
	*buf_size = m_nExtra;

	return S_OK;
}

HRESULT CAVC1Parser::Parse(BYTE **poutbuf, int *poutbuf_size, const BYTE *buf, int buf_size)
{
	uint8_t unit_type;
	int32_t nal_size;
	int32_t cumul_size = 0;
	const uint8_t *buf_end = buf + buf_size;

	if (!m_pExtra) 
	{
		*poutbuf = (BYTE *)buf;
		*poutbuf_size = buf_size;
		return S_OK;
	}

	*poutbuf_size = 0;
	*poutbuf = NULL;

	do 
	{
		if (buf + m_nNALUSize > buf_end)
			goto fail;

		if (m_nNALUSize == 1) 
		{
			nal_size = buf[0];
		} 
		else if (m_nNALUSize == 2) 
		{
			nal_size = AV_RB16(buf);
		} 
		else 
		{
			nal_size = AV_RB32(buf);
			if (m_nNALUSize == 3)
			nal_size >>= 8;
		}

		buf += m_nNALUSize;
		unit_type = *buf & 0x1f;

		if (buf + nal_size > buf_end || nal_size < 0)
			goto fail;

		if (alloc_and_copy(poutbuf, poutbuf_size, buf, nal_size) < 0)
			goto fail;

		buf += nal_size;
		cumul_size += nal_size + m_nNALUSize;
	} while (cumul_size < buf_size);

	return S_OK;

fail:
	SAFE_FREE(*poutbuf);
	return E_FAIL;
}

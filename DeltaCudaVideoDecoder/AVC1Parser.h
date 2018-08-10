#pragma once

class CAVC1Parser
{
public:
	CAVC1Parser();
	~CAVC1Parser();

	HRESULT ParseExtradata(const BYTE *extra, int extralen, int nalu_length);
	HRESULT Parse(BYTE **poutbuf, int *poutbuf_size, const BYTE *buf, int buf_size);

	HRESULT CopyExtra(BYTE *buf, unsigned int *buf_size);
	HRESULT GetExtra(BYTE **buf, unsigned int *buf_size) { *buf = m_pExtra; *buf_size = m_nExtra; return S_OK; }

private:
	BYTE   *m_pExtra;
	unsigned m_nExtra;
	unsigned m_nNALUSize;
};

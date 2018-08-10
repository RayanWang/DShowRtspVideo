#pragma once

#include "stdafx.h"

// Error code
#define SUCCESS                             1
#define FAIL                                0
#define ERR_INVLID_PARAMETER                -1
#define ERR_OPEN_FILE_FAIL                  -2
#define ERR_CLOSE_FILE_FAIL                 -3
#define ERR_READ_FILE_FAIL                  -4
#define ERR_WRITE_FILE_FAIL                 -5
#define ERR_CRYPT_FAIL                      -6
#define ERR_KEY_GENERATE_FAIL               -7
#define ERR_INIT_SBOX_FAIL                  -8

// define key
#define DEF_PASSWORD _T("KingRayan")

BOOL CheckIsWow64()
{
	typedef UINT (WINAPI* GETSYSTEMWOW64DIRECTORY)(LPTSTR, UINT);
    GETSYSTEMWOW64DIRECTORY getSystemWow64Directory;
    HMODULE hKernel32;
    TCHAR Wow64Directory[MAX_PATH];

    hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));

    if (hKernel32 == NULL) 
	{
        //
        // This shouldn't happen, but if we can't get 
        // kernel32's module handle then assume we are 
        //on x86.  We won't ever install 32-bit drivers
        // on 64-bit machines, we just want to catch it 
        // up front to give users a better error message.
        //
        return FALSE;
    }
 
    getSystemWow64Directory = (GETSYSTEMWOW64DIRECTORY) GetProcAddress(hKernel32, "GetSystemWow64DirectoryW");
 
    if (getSystemWow64Directory == NULL) 
	{
        //
        // This most likely means we are running 
        // on Windows 2000, which didn't have this API 
        // and didn't have a 64-bit counterpart.
        //
        return FALSE;
    }
 
    if((getSystemWow64Directory(Wow64Directory, sizeof(Wow64Directory)/sizeof(TCHAR)) == 0) &&
        (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)) 
	{
        return FALSE;
    }
    
    //
    // GetSystemWow64Directory succeeded 
    // so we are on a 64-bit OS.
    //
    return TRUE;
}



// Model of crypt
typedef enum _CRYPT {
    MODE_ENCRYPT = 0,
    MODE_DECRYPT
} CRYPTMODEL;


// Constant
const int BLOCK_SIZE        = 256;
const BYTE DEFAULT_VECTOR   = 0xEF;


///////////////////////////////////////////////////////////////////////////////
// Implementation

void SWAP(BYTE& byX, BYTE& byY)
{
    BYTE byTemp = byX;
    byX = byY;
    byY = byTemp;
}

// Length of key and S-Box must be same
BOOL InitSBox(LPBYTE pKeyBox, LPBYTE pSBox, const unsigned int unLength)
{
    ASSERT(NULL != pKeyBox);
    ASSERT(NULL != pSBox);
    ASSERT(0 < unLength);

    if (!pSBox || !pKeyBox || 0 >= unLength)
        return FALSE;

    unsigned int i;

    // Initial S-Box
    for (i = 0; i < unLength; i ++)
        pSBox[i] = i;

    // Generating
    unsigned int j = 0;
    for (i = 0; i < unLength; i ++)
    {
        j = (j + pSBox[i] + pKeyBox[i]) % unLength;
        SWAP(pSBox[i], pSBox[j]);
    }
    return TRUE;
}

// RC4 cryptogrphy algorithm
// Encrypt/Decrypt
// return cipher or plaintext
BYTE GetCrypt(LPBYTE pKeyBox, LPBYTE pSBox, BYTE byPCH, const unsigned int unLength)
{
    ASSERT(NULL != pKeyBox);
    ASSERT(NULL != pSBox);
    ASSERT(0 < unLength);

    if (!pSBox || !pKeyBox || 0 >= unLength)
        return 0;

    unsigned int i = 0, j = 0;

    i = (i + 1) % unLength;
    j = (j + pSBox[i]) % unLength;
    SWAP(pSBox[i], pSBox[j]);
    unsigned int unKey = pSBox[(pSBox[i] + pSBox[j]) % unLength];

    return (unKey^byPCH);
}

// Expand key data to required length
BOOL ExpandKeyBox(LPBYTE pKeyBox, const int nKeyBoxLen, LPBYTE pKey, const int nKeyLen)
{
    ASSERT(NULL != pKeyBox);
    ASSERT(NULL != pKey);
    ASSERT(0 < nKeyBoxLen);
    ASSERT(0 < nKeyLen);

    if (!pKeyBox || !pKey || 0 >= nKeyBoxLen || 0 >= nKeyLen)
        return FALSE;

    int j = 0;
    BOOL bReverse = FALSE;
    // fill and expand key into key box
    for (int i = 0; i < nKeyBoxLen; i++)
    {
        pKeyBox[i] = pKey[j];

        if (bReverse)
            j--;
        else
            j++;

        if (0 == j)
        {
            bReverse = FALSE;
            pKeyBox[++i] = i;
        }
        else if (nKeyLen == j)
        {
            bReverse = TRUE;
            pKeyBox[++i] = j;
        }
    }

    return TRUE;
}

// Apply CBC mode for stream encryption and decryption
BOOL CBCCrypt(LPBYTE pSrc, 
              LPBYTE pDes, 
              LPBYTE pKeyBox,
              LPBYTE pSBox,
              BYTE byIV, 
              const int nDataLen, 
              CRYPTMODEL mode)
{
    ASSERT(NULL != pSrc);
    ASSERT(NULL != pDes);
    ASSERT(NULL != pKeyBox);
    ASSERT(NULL != pSBox);
    ASSERT(0 < nDataLen);

    if (!pSrc || !pDes || !pKeyBox || !pSBox || 0 >= nDataLen)
        return FALSE;

    int i;
    BYTE byBuf;
    // Encrypt data. 
    if (MODE_ENCRYPT == mode)
    {
        for (i = 0; i < nDataLen; i++)
        {
            byBuf = (0 < i)?(pSrc[i] ^ pDes[i - 1]):(pSrc[i] ^ byIV);
            pDes[i] = GetCrypt(pKeyBox, pSBox, byBuf, nDataLen);
        }
    }
    else
    {
        byBuf = pSrc[0];
        for (i = 0; i < nDataLen; i++)
        {
            byBuf = GetCrypt(pKeyBox, pSBox, pSrc[i], nDataLen);
            pDes[i] = (0 < i)?(byBuf ^ pSrc[i - 1]):(byBuf ^ byIV);
        }
    }

    return TRUE;
}

// If success, return TRUE. Otherwise, return FALSE.
int CryptFile(char* pszSourceFile, char* pszDestinationFile, char* pszPassword, CRYPTMODEL mode)
{
    ASSERT(NULL != pszSourceFile);
    ASSERT(NULL != pszDestinationFile);
    ASSERT(NULL != pszPassword);
    if (!pszSourceFile || !pszDestinationFile || !pszPassword)
        return ERR_INVLID_PARAMETER;

    // Declare and initialize local variables.
    FILE* pSrcFile = NULL; 
    FILE* pDesFile = NULL; 

    // Open source file. 
#if _MSC_VER >= 1400
    if (fopen_s(&pSrcFile, pszSourceFile, "rb") != 0)
        return ERR_OPEN_FILE_FAIL;
#else
    pSrcFile = fopen(pszSourceFile, "rb");
#endif

    if (!pSrcFile)
        return ERR_OPEN_FILE_FAIL;

    // Open destination file. 
#if _MSC_VER >= 1400
    if (fopen_s(&pDesFile, pszDestinationFile, "wb") != 0)
        pDesFile = NULL;
#else
    pDesFile = fopen(pszDestinationFile, "wb");
#endif

    if (!pDesFile)
    {
        fclose(pSrcFile);
        return ERR_OPEN_FILE_FAIL;
    }

    // Generate Key
    BYTE pbKeyBox[BLOCK_SIZE];
    if (!ExpandKeyBox(pbKeyBox, BLOCK_SIZE, (LPBYTE)pszPassword, strlen(pszPassword)))
        return ERR_KEY_GENERATE_FAIL;

    // Generate S-Box
    BYTE pbSBox[BLOCK_SIZE];
    if (!InitSBox(pbKeyBox, pbSBox, BLOCK_SIZE))
        return ERR_INIT_SBOX_FAIL;

    int nCount;
    BYTE byIV = DEFAULT_VECTOR;
    BYTE pbSrcData[BLOCK_SIZE];
    BYTE pbDesData[BLOCK_SIZE];
    // In a do loop, encrypt the source file, and write to the source file. 
    do 
    { 
        // Initial memory
        memset(pbSrcData, 0, BLOCK_SIZE);
        memset(pbDesData, 0, BLOCK_SIZE);

       // Read up to dwBlockLen bytes from the source file. 
        nCount = fread(pbSrcData, 1, BLOCK_SIZE, pSrcFile); 
        if (ferror(pSrcFile))
        {
            fclose(pDesFile);
            fclose(pSrcFile);
            return ERR_READ_FILE_FAIL;
        }

        // Data transformation
        if (!CBCCrypt(pbSrcData, pbDesData, pbKeyBox, pbSBox, byIV, nCount, mode))
        {
            fclose(pDesFile);
            fclose(pSrcFile);
            return ERR_CRYPT_FAIL;
        }
 
        // Write data to the destination file. 
        fwrite(pbDesData, 1, nCount, pDesFile); 
        if (ferror(pDesFile))
        {
            fclose(pDesFile);
            fclose(pSrcFile);
            return ERR_WRITE_FILE_FAIL;
        }

    } while (!feof(pSrcFile)); 

    // close files
    if (fclose(pDesFile))
    {
        if (fclose(pSrcFile))
            return ERR_CLOSE_FILE_FAIL;
        return ERR_CLOSE_FILE_FAIL;
    }
    if (fclose(pSrcFile))
        return ERR_CLOSE_FILE_FAIL;

    return TRUE;
}

// Encrypt file
int EnCryptFile(LPTSTR pszSourceFile, LPTSTR pszDestinationFile, LPTSTR pszPassword)
{
#ifdef UNICODE
    char szSrcFN[MAX_PATH] = {0,};
    WideCharToMultiByte(CP_ACP, 0, pszSourceFile, -1, szSrcFN, MAX_PATH, NULL, NULL);
    char szDstFN[MAX_PATH] = {0,};
    WideCharToMultiByte(CP_ACP, 0, pszDestinationFile, -1, szDstFN, MAX_PATH, NULL, NULL);
    char szPsw[MAX_PATH] = {0,};
    WideCharToMultiByte(CP_ACP, 0, pszPassword, -1, szPsw, MAX_PATH, NULL, NULL);

    return CryptFile(szSrcFN, szDstFN, szPsw, MODE_ENCRYPT);
#else
    return CryptFile(pszSourceFile, pszDestinationFile, pszPassword, MODE_ENCRYPT);
#endif //UNICODE
}

// Decrypt file
int DeCryptFile(LPTSTR pszSourceFile, LPTSTR pszDestinationFile, LPTSTR pszPassword)
{
#ifdef UNICODE
    char szSrcFN[MAX_PATH] = {0,};
    WideCharToMultiByte(CP_ACP, 0, pszSourceFile, -1, szSrcFN, MAX_PATH, NULL, NULL);
    char szDstFN[MAX_PATH] = {0,};
    WideCharToMultiByte(CP_ACP, 0, pszDestinationFile, -1, szDstFN, MAX_PATH, NULL, NULL);
    char szPsw[MAX_PATH] = {0,};
    WideCharToMultiByte(CP_ACP, 0, pszPassword, -1, szPsw, MAX_PATH, NULL, NULL);

    return CryptFile(szSrcFN, szDstFN, szPsw, MODE_DECRYPT);
#else
    return CryptFile(pszSourceFile, pszDestinationFile, pszPassword, MODE_DECRYPT);
#endif //UNICODE
}
#include "RegressionLeastSquare.h"


CRegressionLeastSquare::CRegressionLeastSquare(void)
{
}


CRegressionLeastSquare::~CRegressionLeastSquare(void)
{
}


BOOL CRegressionLeastSquare::GetCCMfromRegression(double **ppStandardMatrix, double **ppPatchIpCam, int	nStandardPatch, double (&MatrixIpCam)[3][3])
{
	CheckPointer(ppStandardMatrix, FALSE);
	CheckPointer(ppPatchIpCam, FALSE);

	double **ppMatrixBAT = new double*[3];
	InitMatrix(ppMatrixBAT, 3, 3);

	double **ppMatrixAAT = new double*[3];
	InitMatrix(ppMatrixAAT, 3, 3);

	// Get Nx3 AT matrix
	double **ppTransPatchIpCamHSL = new double*[nStandardPatch];
	InitMatrix(ppTransPatchIpCamHSL, nStandardPatch, 3);
	Transpose(ppPatchIpCam, ppTransPatchIpCamHSL, 3, nStandardPatch);

	// Calculate B*AT => 3x3 matrix ppMatrixBAT
	MatrixMutiply(ppStandardMatrix, ppTransPatchIpCamHSL, ppMatrixBAT, 3, nStandardPatch, 3);

	// Calculate A*AT => 3x3 matrix ppMatrixAAT
	MatrixMutiply(ppPatchIpCam, ppTransPatchIpCamHSL, ppMatrixAAT, 3, nStandardPatch, 3);

	for(int i = 0; i < nStandardPatch; ++i)
		SAFE_DELETE_ARRAY(ppTransPatchIpCamHSL[i]);
	SAFE_DELETE_ARRAY(ppTransPatchIpCamHSL);

	// Caculate A*AT's inverse => MatrixInverse
	double MatrixTmp[3][3];
	InitMatrix(MatrixTmp);
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
			MatrixTmp[i][j] = ppMatrixAAT[i][j];
	}

	double MatrixInverse[3][3];
	InitMatrix(MatrixInverse);
	FindMatrixInverse(MatrixTmp, MatrixInverse);

	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppMatrixAAT[i]);
	SAFE_DELETE_ARRAY(ppMatrixAAT);

	// Get Result
	InitMatrix(MatrixTmp);
	for(int i = 0; i < 3; ++i)
	{
		for(int j = 0; j < 3; ++j)
			MatrixTmp[i][j] = ppMatrixBAT[i][j];
	}
	for(int i = 0; i < 3; ++i)
		SAFE_DELETE_ARRAY(ppMatrixBAT[i]);
	SAFE_DELETE_ARRAY(ppMatrixBAT);

	MatrixMutiply(MatrixTmp, MatrixInverse, MatrixIpCam);

	return TRUE;
}


void CRegressionLeastSquare::FindMatrixInverse(const double (&Matrix)[3][3], double (&MatrixInverse)[3][3])
{
	double temp[3][3], A[3][3];
	double determinant = 1;  
 
	for (int i = 0; i < 3; ++i) 
		for (int j = 0; j < 3; ++j)
		{
			temp[i][j] = Matrix[i][j];
			A[i][j] = Matrix[i][j];
			if (i == j) 
				MatrixInverse[i][j] = 1;
			else 
				MatrixInverse[i][j] = 0;
		}
 
	int d = 0;
	do
	{
		if(A[d][d] == 0)
		{
			if(d == 3) 
			{
				determinant = 0;
			}
			else
			{
				int t = d + 1;
				do
				{
					if (A[d][t] != 0)
					{
						determinant = 1;
						for (int k = d; k < 3; ++k) 
						{
							A[k][d] = A[k][t] + A[k][d]; 
							MatrixInverse[k][d] = MatrixInverse[k][t] + MatrixInverse[k][d];
						}
					}	//set d-row  = d-row + t-row 
					else 
					{
						++t; 
						determinant = 0;
					}
			    } while(determinant == 0 && t < 3); 
			} //往 (i+1)-row 開始找, A[i][t] 是否有非 0 的元素
		}
 
		if(determinant == 1)
		{
			for(int j = 0; j < 3; ++j)
				for(int k = 0; k < 3; ++k)
					if (k != d)
					{ 
						A[j][k] = A[j][k] - A[j][d]*temp[d][k]/A[d][d];
						MatrixInverse[j][k] = MatrixInverse[j][k] - MatrixInverse[j][d]*temp[d][k]/A[d][d];
					}
 
			for(int i = 0; i < 3; ++i)
				for(int j = 0; j < 3; ++j)
				{
					temp[i][j] = A[i][j];
				}
			++d;
		}
	} while(d < 3 && determinant == 1); 
 
	if(determinant == 1)
	{
		for(int i = 0; i < 3; ++i)
			determinant = A[i][i]*determinant;
 
		for(int i = 0; i < 3; ++i)
			if (A[i][i] != 1)
				for(int j = 0; j < 3; ++j)
					MatrixInverse[j][i] = MatrixInverse[j][i]/A[i][i];
	}
}

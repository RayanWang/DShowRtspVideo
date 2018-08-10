#pragma once

#define SAFE_DELETE(p) if(p) {delete p; p = NULL;}
#define SAFE_DELETE_ARRAY(p) if(p) {delete [] p; p = NULL;}
#define CheckPointer(p,ret) {if((p)==NULL) return (ret);}

template<typename T>
void	InitMatrix(T **ppMatrix, int M, int N)
{
	if(NULL == ppMatrix)
		return;

	for (int i = 0; i < M; ++i)
	{
		ppMatrix[i] = new T [N];
		for (int j = 0; j < N; ++j)
			ppMatrix[i][j] = 0;
	}
}

template<typename T, size_t M, size_t N>
void	InitMatrix(T (&Matrix)[M][N])
{
	for(int i = 0; i < M; ++i) 
		for(int j = 0; j < N; ++j) 
			Matrix[i][j] = 0; 
}

template<typename T, size_t M, size_t N>
void	Transpose(T (&MatrixOrig)[M][N], T (&MatrixTrans)[N][M])
{
	int i, j;

	for (i = 0; i < M; ++i) 
	{
		for (j = 0; j < N; ++j) 
		{
			MatrixTrans[j][i] = MatrixOrig[i][j];
		}
	}
}

template<typename T>
void	Transpose(T **ppArrOrig, T **ppArrTrans, int M, int N)
{
	if(NULL == ppArrOrig || NULL == *ppArrOrig || NULL == ppArrTrans || NULL == *ppArrTrans)
		return;

	int i, j;

	for (i = 0; i < M; ++i) 
	{
		for (j = 0; j < N; ++j) 
		{
			ppArrTrans[j][i] = ppArrOrig[i][j];
		}
	}
}

template<typename T, size_t M, size_t N, size_t P>
void	MatrixMutiply(T (&Matrix1)[M][N], T (&Matrix2)[N][P], T (&MatrixResult)[M][P])
{
	int i, j, k;
	T Sum;

	for(i = 0; i < M; ++i)
		for(j = 0; j < P; ++j)
		{
			Sum = 0;
			for(k = 0; k < N; ++k)
				Sum += (T)(Matrix1[i][k]*Matrix2[k][j]);
			MatrixResult[i][j] = Sum;
		}
}

template<typename T1, typename T2>
void	MatrixMutiply(T1 **ppArr1, T2 **ppArr2, T1 **ppArrResult, int M, int N, int P)
{
	if(NULL == ppArr1 || NULL == *ppArr1 || NULL == ppArr2 || NULL == *ppArr2 || NULL == ppArrResult || NULL == *ppArrResult)
		return;

	int i, j, k;
	T1 Sum;

	for(i = 0; i < M; ++i)
		for(j = 0; j < P; ++j)
		{
			Sum = 0;
			for(k = 0; k < N; ++k)
				Sum += (T1)(ppArr1[i][k]*(T1)(ppArr2[k][j]));
			ppArrResult[i][j] = Sum;
		}
}
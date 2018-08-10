#pragma once

class CCurveFitting
{
public:
	CCurveFitting(void);
	virtual ~CCurveFitting(void);

	// Find a polynomial of the curve which satisfying the given points (x, y), a[] sorting in ascending
	// This function is refered to the algorithm of polyfit in Matlab
	void polyfit(int n, double x[], double y[], int poly_n, double a[]);

private:
	void gauss_solve(int n, double A[], double x[], double b[]);
};


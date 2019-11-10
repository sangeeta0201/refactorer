#define DATA_TYPE double 

int foo(posit32_t  x){
	int n = 10;
	int m = 10;
	int i,j,k;
	DATA_TYPE A[10][10], R[10][10], Q[10][10];
	posit32_t nrm = convertDoubleToP32 (0);
	posit32_t f = convertDoubleToP32 (0);
			A[i][j] = convertDoubleToP32 (2.299999999999999822364316059975E+0);;
			R[i][j] = convertDoubleToP32 (3.200000000000000177635683940025E+0);;
			posit32_t tmp5 = convertDoubleToP32 (i);
			nrm = p32_add(nrm,tmp5);
			posit32_t tmp3 = p32_mul(nrm,A[0][0]);
			f = p32_add(f,tmp3);
			 tmp1 = convertP32ToDouble (f);
		posit32_t tmp2;
		double tmp1 = convertP32ToDouble (tmp2);
		double tmp2 = convertP32ToDouble (tmp2);
		printf("%d %e, %e", n, tmp1, tmp2);
}

#include "softposit.h"
#include "softposit.h"
int main(){
	posit32_t z;
 	posit32_t  sum = z = convertDoubleToP32 (4.5E+0);;
}

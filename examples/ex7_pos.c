#define DATA_TYPE double 
/*
int foo(double x){
	int n = 10;
	int m = 10;
	int i,j,k;
	DATA_TYPE A[10][10], R[10][10], Q[10][10];
	double nrm = 0;
  double f = 0;
  A[i][j] = 2.3;
  R[i][j] = 3.2;
  nrm += (double)i;
  f += nrm * A[0][0];
  printf("%d %e, %e", n, f, nrm);
  }
*/
#include "softposit.h"
int main(){
	posit32_t z;posit32_t sum;
 	rapl_p32_force_store(&sum,z);
 	 tmp1 = convertDoubleToP32 (4.5E+0);
 	rapl_p32_force_store(&z,tmp1);
 	
}

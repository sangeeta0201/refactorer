posit32_t xx;
posit32_t 2_t  B[3][3], C[3][3];
struct foos{
  posit32_t x;
  posit32_t y;
  posit32_t sum;
};
struct foos foo(double *x, double *y){
  struct foos f;
  posit32_t tmp11 = convertDoubleToP32 (1);
  posit32_t tmp12 = p32_add(tmp11,xx);
  f.x = tmp12;
  
  posit32_t tmp8 = convertDoubleToP32 (1);
  posit32_t tmp9 = p32_add(tmp8,*y);
  posit32_t tmp10 = p32_add(tmp9,f.x);
  f.y = tmp10;
  posit32_t tmp7 = p32_add(f.x,f.y);
  f.sum = tmp7;
  return f;   
}

void testMat(double **A, double B[][3]){
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      posit32_t tmp6 = p32_add((A[i][j]),(B[i][j]));
      C[i][j] = tmp6;
    }
  }
}
#include "softposit.h"
int main() {
  posit32_t x;posit32_t y;
	struct foos f;
  double **A = (posit32_t **)malloc(3*sizeof(posit32_te *));
  for(int i =0; i< 3; i++) {
    A[i] = (posit32_t *)malloc(3*sizeof(posit32_t)); 
    }

  x = convertDoubleToP32 (1.1000000000000000888178419700125E+0);;
  y = p32_sqrt(x);
  f = foo(&x, &y);
  double tmp14 = convertP32ToDouble (f.x);
  printf("%e\n", tmp14);
  double tmp15 = convertP32ToDouble (f.x);
  printf("%e\n", tmp15);
  double tmp16 = convertP32ToDouble (f.y);
  printf("%e\n", tmp16);
  double tmp17 = convertP32ToDouble (f.sum);
  printf("%e\n", tmp17);
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      posit32_t tmp3 = convertDoubleToP32 (2.9999999999999998889776975374843E-1);
      posit32_t tmp4 = p32_mul(i,tmp3);
      posit32_t tmp5 = p32_add(tmp4,i);
      A[i][j] = tmp5;
      posit32_t tmp1 = convertDoubleToP32 (5.5000000000000004440892098500626E-1);
      posit32_t tmp2 = p32_mul((i + j),tmp1);
      B[i][j] = tmp2;
    }
  }

  testMat(A, B);
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      double tmp18 = convertP32ToDouble (C[i][j]);
      printf("%e ", tmp18);
    }
    printf("\n");
  }

  return 0;
}

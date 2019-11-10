# 1 "ex7.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "ex7.c"
# 1 "test.h" 1
# 2 "ex7.c" 2

posit32_t B[10];


int foo(){
 int n = 10;
 int m = 10;
 int i,j;
 posit32_t A[10][10];
 for (i = 0; i < m; i++){
    for (j = 0; j < n; j++) {
   posit32_t tmp1 = convertDoubleToP32 (((i * j) % m));
   posit32_t tmp2 = p32_div(tmp1,m);
   posit32_t tmp3 = p32_mul(tmp2,100);
   posit32_t tmp4 = p32_add(tmp3,10);
   A[i][j] = tmp4;


  }
 }
}

#include "softposit.h"
int main(){
 foo();
}

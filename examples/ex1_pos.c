/*void bar(float *x){
}
void foo1(float *x){
//  *x = sqrt(*x) ; //there should be error here and it should tace back to float xx
  bar(x);
}
*/
/*
void bar2(float *x){
    *x = (*x) * (*x); //there should be error here and it should tace back to float xx
      printf("%e\n", *x);  //(xx+x)*(xx+x)
}
float cordic_sin(float theta){
}
*/
posit32_t  foo(int n){
 // float z,di;
  posit32_t R;posit32_t Q;posit32_t CR2;posit32_t CQ3;posit32_t a;posit32_t b;posit32_t sgnR;posit32_t R2;posit32_t Q3;
//  double *x0, *x1;
  /*
  if (R == 0 && Q == 0)
  {
    *x0 = - a / 3 ;
  }
  else if (CR2 == CQ3)
  {
    *x1 = - a / 3 ;
  }*/
  posit32_t tmp1 = rapl_p32_force_load(&(R2));
  
  posit32_t tmp2 = rapl_p32_force_load(&(Q3));
  
  posit32_t tmp3 = p32_sub(tmp1,tmp2);
  posit32_t tmp4 = convertDoubleToP32 (1.0E+0);
  posit32_t tmp5 = convertDoubleToP32 (3.0E+0);
  posit32_t tmp6 = p32_div(tmp4,tmp5);
  posit32_t tmp7 = rapl_p32_force_load(&(R));
  
  posit32_t tmp8 = p32_fabs(tmp7);
  posit32_t tmp10 = p32_sqrt(tmp3);
  posit32_t tmp12 = p32_add(tmp8,tmp10);
  posit32_t tmp13 = convertDoubleToP32 (0);
  posit32_t tmp14 = rapl_p32_force_load(&(sgnR));
  
  posit32_t tmp15 = p32_sub(tmp13,tmp14);
  posit32_t tmp16 = p32_pow(tmp12,tmp6);
  posit32_t tmp18 = p32_mul(tmp15,tmp16);
  posit32_t A;
  rapl_p32_force_store(&(A),tmp18);
  
 // float theta, sin;
//  theta = -sin;
//  double sgnR = (R >= 0 ? 1 : -1);
//  double q = (a * a - 3 * b);
//   *x0 = - a / 3 ;
//  for (float theta = sin; theta < 1.54; theta = theta + 0.00000065) {
//    cos += theta;
//  }
 // if (fabsf(diffX) != 0.0) {
//    cordic_sin(theta);
//  }
/*
  if (z >= 0) di = 1.0;
          else 
            di = -1.0;
*/
}


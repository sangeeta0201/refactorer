
posit32_t  arr[2] ;
__attribute__ ((constructor)) void __init_arr(void){
posit32_t tmp1 = convertDoubleToP32 (1.1999999999999999555910790149937E+0);
rapl_p32_force_store(&arr[0],tmp1);
posit32_t tmp2 = convertDoubleToP32 (3.3999999999999999111821580299875E+0);
rapl_p32_force_store(&arr[1],tmp2);

}

posit32_t u1[10];

posit32_t u2[10];

posit32_t v1[10];

posit32_t v2[10];

posit32_t x[10];

posit32_t y[10];

posit32_t z[10];

posit32_t w[10];

posit32_t A[10][10];


int ni=10;
int nj=10;
int nk=10;
int j = 10;
int n = 10;
posit32_t  bar(posit32_t tmp3, posit32_t tmp5){
  posit32_t tmp4 = rapl_p32_get_arg(tmp3);
  posit32_t  x;
  rapl_p32_force_store(&x,tmp4);

  posit32_t tmp6 = rapl_p32_get_arg(tmp5);
  posit32_t  y;
  rapl_p32_force_store(&y,tmp6);

  posit32_t tmp7 = rapl_p32_force_load(&x);
  
  posit32_t tmp8 = rapl_p32_force_load(&y);
  
  bool tmp9 = (tmp7,tmp8);
  posit32_t tmp10 = convertDoubleToP32 (tmp9);
  posit32_t tmp12 = rapl_p32_set_ret(tmp10);
  return tmp12;
}
posit32_t  foo(posit32_t tmp13, posit32_t tmp15, posit32_t tmp17){
  posit32_t tmp14 = rapl_p32_get_arg(tmp13);
  posit32_t  zz;
  rapl_p32_force_store(&zz,tmp14);

  posit32_t tmp16 = rapl_p32_get_arg(tmp15);
  posit32_t  kk;
  rapl_p32_force_store(&kk,tmp16);

  posit32_t tmp18 = rapl_p32_get_arg(tmp17);
  posit32_t  tt;
  rapl_p32_force_store(&tt,tmp18);

  /*
  double fn = (double)n;
  for (int i = 0; i < n; i++)
  {    
    u1[i] = i;
    u2[i] = ((i+1)/fn)/2.0;
    u2[i] = ((i+1)/fn)/2.0;
    v1[i] = ((i+1)/fn)/4.0;
    v2[i] = ((i+1)/fn)/6.0;
    y[i] = ((i+1)/fn)/8.0;
    z[i] = ((i+1)/fn)/9.0;
    x[i] = 0.0;
    w[i] = 0.0;
    for (j = 0; j < n; j++)
      A[i][j] = (double) (i*j % n) / n;
  } 
  */
  posit32_t tmp22 = rapl_p32_set_arg(kk,1);
  posit32_t tmp23 = rapl_p32_set_arg(tt,2);
  posit32_t tmp25 = bar(tmp22,tmp23);
  posit32_t tmp26 = rapl_p32_get_ret(tmp25);
  rapl_p32_force_store(&zz,tmp26);
  
  posit32_t tmp19 = rapl_p32_force_load(&zz);
  
  posit32_t tmp21 = rapl_p32_set_ret(tmp19);
  return tmp21;
}

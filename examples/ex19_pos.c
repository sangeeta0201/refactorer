#define fma(a,b,c) {b = b *b; c = c*c; a = b + c;}

void update_u( posit32_t  eps ){
	unsigned margin = 4;
	unsigned size   = 2*margin*sizeof(posit32_t double );
  posit32_t tmp5 = convertDoubleToP32 (1.024E+3);
  posit32_t tmp6 = convertDoubleToP32 (1.024E+3);
  posit32_t tmp7 = p32_mul(tmp5,tmp6);
  posit32_t tmp8 = p32_div(size,tmp7);
  posit32_t tmp9 = convertDoubleToP32 (1.024E+3);
  posit32_t tmp10 = convertDoubleToP32 (1.024E+3);
  posit32_t tmp11 = p32_mul(tmp9,tmp10);
  posit32_t tmp12 = convertDoubleToP32 (3.200000000000000177635683940025E+0);
  posit32_t tmp13 = p32_mul(tmp8,tmp12);
  posit32_t tmp14 = p32_div(size,tmp11);
  printf( "LBM_allocateGrid: allocated %.1f MByte\n",
          tmp14, tmp13 );
}

void foo(){
	posit32_t eps = convertDoubleToP32 (3.3999999999999999111821580299875E+0);
	posit32_t tmp1 = convertDoubleToP32 (6.5E+0);
	posit32_t tmp2 = p32_mul(4,tmp1);
	posit32_t tmp3 = convertDoubleToP32 (3.3999999999999999111821580299875E+0);
	posit32_t tmp4 = p32_div(tmp2,tmp3);
	update_u(tmp4);
}

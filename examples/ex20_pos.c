#define PI 3.141592653589793238462643383279502884197169399375105820974944592308 

int foo(){
	posit32_t z;
	posit32_t y = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	int i = 0;
	posit32_t tmp4 = convertDoubleToP32 (3.1415926535897931159979634685442E+0);
	posit32_t tmp5 = convertDoubleToP32 (1.8E+1);
	posit32_t tmp6 = p32_div(tmp4,tmp5);
	posit32_t radconst = tmp6;
	posit32_t tmp3 = p32_mul(y,z);
	posit32_t  k = p32_cos(tmp3);
	posit32_t tmp1 = convertDoubleToP32 ((i + 1));
	posit32_t tmp2 = p32_mul(radconst,tmp1);
	posit32_t  x = p32_cos(tmp2);
	
}

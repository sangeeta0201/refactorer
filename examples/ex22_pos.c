int plus1mod3[3] = {1, 2, 0};

int foo(){
	posit32_t tmp1 = convertDoubleToP32 (2.9999999999999998889776975374843E-1);
	posit32_t tmp2 = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	posit32_t tmp3 = p32_mul(tmp1,tmp2);
	posit32_t maxlen = tmp3; 
	posit32_t tmp4 = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	posit32_t tmp5 = convertDoubleToP32 (0.0E+0);
	if(p32_le(tmp4,tmp5)) {
		return 0;
		}
}

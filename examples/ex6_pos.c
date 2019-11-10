int foo(){
	posit32_t y[3][3];
	
	posit32_t tmp1 = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	rapl_p32_force_store(&x,tmp1);
	
	posit32_t tmp2 = rapl_p32_set_arg(x,1);
	posit32_t tmp4 = p32_sqrt(tmp2);
	posit32_t tmp5 = rapl_p32_get_ret(tmp4);
	
	int i = 1;
	int j = 1;
	double tmp6 = convertP32ToDouble (y[i][j]);
	printf("%e      ", tmp6);
}

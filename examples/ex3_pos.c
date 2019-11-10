int foo(){
	posit32_t tmp1 = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	rapl_p32_force_store(&x,tmp1);
	
	posit32_t tmp2 = convertDoubleToP32 (3.3999999999999999111821580299875E+0);
	rapl_p32_force_store(&y,tmp2);
	 
	posit32_t z;
	posit32_t sum;
	posit32_t tmp3 = rapl_p32_force_load(&x);
	
	posit32_t tmp4 = rapl_p32_force_load(&y);
	
	posit32_t tmp5 = p32_add(tmp3,tmp4);
	posit32_t tmp6 = convertDoubleToP32 (3.1000000000000000888178419700125E+0);
	posit32_t tmp7 = p32_add(tmp5,tmp6);
	rapl_p32_force_store(&z,tmp7);
	 
	posit32_t tmp8 = rapl_p32_set_arg(z,1);
	posit32_t tmp10 = p32_sqrt(tmp8);
	posit32_t tmp11 = rapl_p32_get_ret(tmp10);
	rapl_p32_force_store(&sum,tmp11);
	
}

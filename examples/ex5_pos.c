#define N_CELL_ENTRIES 100
int foo(){
	int j;
	int i =5;
	posit32_t px;
	posit32_t tmp20 = convertDoubleToP32 (100);
	posit32_t tmp21 = convertDoubleToP32 (5.0E-1);
	posit32_t tmp22 = p32_div(tmp20,tmp21);
	posit32_t tmp23 = convertDoubleToP32 (1.0E+0);
	posit32_t tmp24 = p32_sub(tmp22,tmp23);
	rapl_p32_force_store(&px,tmp24);
	
	posit32_t tmp13 = convertDoubleToP32 (5.0E-1);
	posit32_t tmp14 = convertDoubleToP32 ((1 * (100)) - 1);
	posit32_t tmp15 = p32_mul(tmp13,tmp14);
	posit32_t tmp16 = convertDoubleToP32 ((i / 100) % (1 * (100)));
	posit32_t tmp17 = p32_div(tmp16,tmp15);
	posit32_t tmp18 = convertDoubleToP32 (1.0E+0);
	posit32_t tmp19 = p32_sub(tmp17,tmp18);
	rapl_p32_force_store(&px,tmp19);
	
	posit32_t tmp6 = convertDoubleToP32 (5.0E-1);
	posit32_t tmp7 = convertDoubleToP32 ((1 * (100)) - 1);
	posit32_t tmp8 = p32_mul(tmp6,tmp7);
	posit32_t tmp9 = convertDoubleToP32 ((i / 100) % (1 * (100)));
	posit32_t tmp10 = p32_div(tmp9,tmp8);
	posit32_t tmp11 = convertDoubleToP32 (1.0E+0);
	posit32_t tmp12 = p32_sub(tmp10,tmp11);
	rapl_p32_force_store(&px,tmp12);
	
	posit32_t tmp1 = convertDoubleToP32 (5.0E-1);
	posit32_t tmp2 = convertDoubleToP32 (1);
	posit32_t tmp3 = p32_sub(tmp1,tmp2);
	posit32_t tmp4 = convertDoubleToP32 (i % 100);
	posit32_t tmp5 = p32_div(tmp4,tmp3);
	rapl_p32_force_store(&px,tmp5);
	
}

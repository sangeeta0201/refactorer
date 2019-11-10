int foo(struct behavior *b){
	posit32_t nearestpoweroftwo;posit32_t segmentlength;posit32_t split;posit32_t acutedest;
	nearestpoweroftwo = convertDoubleToP32 (1.0E+0);;
	posit32_t tmp10 = convertDoubleToP32 (3.0E+0);
	posit32_t tmp11 = p32_mul(tmp10,nearestpoweroftwo);
	while (!p32_le(segmentlength,tmp11)) {
		posit32_t tmp8 = convertDoubleToP32 (2.0E+0);
		nearestpoweroftwo = p32_mul(nearestpoweroftwo,tmp8);
		
	}
	posit32_t tmp6 = convertDoubleToP32 (1.5E+0);
	posit32_t tmp7 = p32_mul(tmp6,nearestpoweroftwo);
	while (p32_lt(segmentlength,tmp7)) {
		posit32_t tmp4 = convertDoubleToP32 (5.0E-1);
		nearestpoweroftwo = p32_mul(nearestpoweroftwo,tmp4);
		
	}

	posit32_t tmp3 = p32_div(nearestpoweroftwo,segmentlength);
	split = tmp3;

	posit32_t tmp12 = convertDoubleToP32 (2.0E+0);
	if (p32_eq(acutedest,tmp12)) {
		posit32_t tmp1 = convertDoubleToP32 (1.0E+0);
		posit32_t tmp2 = p32_sub(tmp1,split);
		split = tmp2;
	}
}

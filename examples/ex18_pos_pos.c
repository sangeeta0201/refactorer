#define fma(a,b,c) {b = b *b; c = c*c; a = b + c;}

void update_u( posit32_t  eps ){
	posit32_t t2;posit32_t t3;posit32_t t4;posit32_t t5;posit32_t t6;
	t2 = convertDoubleToP32 (2.0E+0);;
	t3 = convertDoubleToP32 (3.0E+0);;
	t4 = convertDoubleToP32 (4.0E+0);;
	fma(posit32_t tmp1 = p32_add(t3,t4);
	t2,posit32_t tmp3 = p32_mul(t3,t3);
	t3,posit32_t tmp2 = p32_mul(t4,t4);
	t4);
//	t2 = t2 *t2; 	t3 = t3*t3; 	t4 = t2 + t3;
}

int plus1mod3[3] = {1, 2, 0};
struct behavior {
	posit32_t minangle;posit32_t goodangle;posit32_t offconstant;
	posit32_t maxarea;
};

posit32_t  bar(posit32_t  x){
	return x;
}
#define PI 3.141592653589793238462643383279502884197169399375105820974944592308
int foo(struct behavior *b){
	posit32_t x = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	posit32_t y = convertDoubleToP32 (2.3999999999999999111821580299875E+0);

	//double k ;
	posit32_t tmp19 = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	bar(tmp19); 

	posit32_t tmp17 = p32_mul(x,y);
	posit32_t tmp18 = p32_mul(x,bar(tmp17));
	posit32_t z = tmp18; 
	posit32_t tmp20 = convertDoubleToP32 (1.8E+2);
	p32_sqrt(tmp20);

	posit32_t tmp14 = convertDoubleToP32 (3.1415926535897931159979634685442E+0);
	posit32_t tmp15 = convertDoubleToP32 (1.8E+2);
	posit32_t tmp16 = p32_div(tmp14,tmp15);
	p32_cos(tmp16);


	posit32_t tmp11 = convertDoubleToP32 (3.1415926535897931159979634685442E+0);
	posit32_t tmp12 = convertDoubleToP32 (1.8E+2);
	posit32_t tmp13 = p32_div(tmp11,tmp12);
	posit32_t  goodangle = p32_cos(tmp13);
	posit32_t g1; 
	posit32_t tmp8 = convertDoubleToP32 (3.1415926535897931159979634685442E+0);
	posit32_t tmp9 = convertDoubleToP32 (1.8E+2);
	posit32_t tmp10 = p32_div(tmp8,tmp9);
	g1 = p32_cos(tmp10);

	posit32_t tmp21 = convertDoubleToP32 (1.8E+2);
	p32_cos(tmp21);
	posit32_t tmp24 = convertDoubleToP32 (1.8E+2);
	posit32_t  goodangle2 = p32_cos(tmp24);
	posit32_t goodangle3;
	posit32_t tmp25 = convertDoubleToP32 (1.8E+2);
	goodangle3 = p32_cos(tmp25);

	double tmp26 = convertP32ToDouble (goodangle);
	printf("%e", tmp26);
	posit32_t tmp22 = convertDoubleToP32 (1.0E+0);
	if(p32_eq(b->offconstant,tmp22)) {
	posit32_t tmp1 = convertDoubleToP32 (1.0E+0);
	posit32_t tmp2 = p32_sub(tmp1,b->goodangle);
	posit32_t tmp3 = convertDoubleToP32 (1.0E+0);
	posit32_t tmp4 = p32_add(tmp3,b->goodangle);
	posit32_t tmp5 = p32_div(tmp4,tmp2);
	posit32_t tmp6 = convertDoubleToP32 (4.7499999999999997779553950749687E-1);
	posit32_t tmp7 = p32_mul(tmp6,p32_sqrt(tmp5));
	b->offconstant = tmp7;
	}

}

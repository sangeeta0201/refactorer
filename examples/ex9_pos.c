#define LOCAL( grid, C  ) A[0][0]
#define DFL1 +2.3*4.3
int foo(){
	int n = 10;
	int m = 10;
	int i,j;
	posit32_t A[10][10];
	doposit32_t z;posit32_t u;u = convertDoubleToP32 (3.3999999999999999111821580299875E+0);;
	
	for (i = 0; i < m; i++){
    for (j = 0; j < n; j++) {
//			LOCAL( grid, C  ) = DFL1;
	//		z = +u * 3;
//			z = +u * -3;
			posit32_t tmp1=convertDoubleToP32 (5.0E-1);
			posit32_t tmp2 = p32_mul(tmp1,((1 * (100)) - 1));
			z = tmp2;
			//z = +u * -3.0;
		}
	}
}

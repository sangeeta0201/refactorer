#define LOCAL( grid, C  ) A[0][0]
#define DFL1 +2.3*4.3
#define N_CELL_ENTRIES 10
int foo(){
	int n = 10;
	int m = 10;
	int i = 2;
	int j;
//	double A[10][10];
//	double z, u, px;
//	u = 3.4;
	posit32_t z = convertDoubleToP32 (0);
	posit32_t minU2 = convertDoubleToP32 (1.000000000000000019884624838656E+30);posit32_t maxU2;posit32_t ux;posit32_t rho = convertDoubleToP32 (3.3999999999999999111821580299875E+0);

	posit32_t tmp1 = p32_sub((minU2),((maxU2)));
	ux = tmp1;
  ux = p32_div(ux,rho);
	printf( "LBM_compareVelocityField: maxDiff ==>  %s\n\n",
          p16_sqrt( minU2 ) > 1e-5 ? "##### ERROR #####" : "OK" );
}

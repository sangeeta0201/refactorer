#define LOCAL( grid, C  ) A[0][0]
#define DFL1 2.3*4.3
#define N_CELL_ENTRIES 100
int foo(){
	int n = 10;
	int m = 10;
	int i,j;
	posit32_t A[10][10];
	doposit32_t ux = convertDoubleToP32 (2.299999999999999822364316059975E+0);posit32_t uy = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	posit32_t uz = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	posit32_t u2;posit32_t px;
//	int size   = sizeof( float ) + 2*sizeof( double );
	for (i = 0; i < m; i++){
    for (j = 0; j < n; j++) {
//			A[i][j] = (( ((i*j) % m) / m )*100) + 10 + 0.3;
			posit32_t tmp1=convertDoubleToP32 (5.0E-1);
			posit32_t tmp2 = p32_mul(tmp1,((1 * (100)) - 1));
			posit32_t tmp3 = p32_div(((i / 100) % (1 * (100))),tmp2);
			posit32_t tmp4=convertDoubleToP32 (1.0E+0);
			posit32_t tmp5 = p32_sub(tmp3,tmp4);
			px = tmp5;
//			A[i][j] /= A[i][j];
			
			//u2 = 1.5 * (ux*ux + uy*uy + uz*uz);
//			u2 =  (ux*ux + uy*uy) * (0.3)/(1024.0*1024.0);
//	 printf( "LBM_allocateGrid: could not allocate %.1f MByte\n",
  //          2.3*54 );
//		LOCAL( grid, C  ) = DFL1;
//			A[i][j] = (double) (i*100) + 10;
		}
	}
}

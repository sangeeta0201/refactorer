#define LOCAL( grid, C  ) A[0][0]
#define DFL1 2.3*4.3
#define N_CELL_ENTRIES 100
int foo(){
	int n = 10;
	int m = 10;
	int i,j;
	double A[10][10];
	double ux = 2.3;
	double uy = 2.3;
	double uz = 2.3;
	double u2, px;
//	int size   = sizeof( float ) + 2*sizeof( double );
	for (i = 0; i < m; i++){
    for (j = 0; j < n; j++) {
//			A[i][j] = (( ((i*j) % m) / m )*100) + 10 + 0.3;
			px = (((i / N_CELL_ENTRIES) % (1*(100))) / (0.5*((1*(100))-1))) - 1.0;
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

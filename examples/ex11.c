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
	double z = 0;
	double minU2  = 1e+30, maxU2  = -1e+30, ux, rho = 3.4;

	ux = (+ (minU2)) - ((maxU2));
  ux /= rho;
	printf( "LBM_compareVelocityField: maxDiff ==>  %s\n\n",
          sqrt( minU2 ) > 1e-5 ? "##### ERROR #####" : "OK" );
}

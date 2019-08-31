#define LOCAL( grid, C  ) A[0][0]
#define DFL1 +2.3*4.3
#define N_CELL_ENTRIES 10
#define SIZE_Z 10
#define SIZE_Y 10
#define SIZE_X 10
typedef double LBM_Grid[SIZE_Z*SIZE_Y*SIZE_X*N_CELL_ENTRIES];
//int foo(LBM_Grid grid, char t){
int foo( double** ptr, double xx ){
	int n = 10;
	int m = 10;
	int i = 2;
	int j;
	int BB[10];
	double B[3] = {2.3, 4.5, 6.7};
	double A[10][10];
	double *uu;
	double **uuu;
	int **pp;
	double u, px;
	u = 3.4;
	double z = 0;
	double min;
	double minU2  = 1e+30, maxU2  = 1e+30, u2;
	//double minU2  = 1e+30, maxU2  = -1e+30, u2;
	if( minU2 > maxU2 )min=maxU2;   
//	for (i = 0; i < m; i++){
 //   for (j = 0; j < n; j++) {
//			px = (((i / N_CELL_ENTRIES) % (1*(100))) / (0.5*((1*(100))-1))) - 1.0;
//		}
//	}
}

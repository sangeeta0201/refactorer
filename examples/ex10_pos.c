#define LOCAL( grid, C  ) A[0][0]
#define DFL1 +2.3*4.3
#define N_CELL_ENTRIES 10
#define SIZE_Z 10
#define SIZE_Y 10
#define SIZE_X 10
typedef double LBM_Grid[SIZE_Z*SIZE_Y*SIZE_X*N_CELL_ENTRIES];
int foo( posit16_t ** ptr, posit16_t  xx ){
	int n = 10;
	int m = 10;
	int i = 2;
	int j;
	int BB[10];
	posit16_t tmp1 = convertDoubleToP16 (2.299999999999999822364316059975E+0);
	posit16_t tmp2 = convertDoubleToP16 (4.5E+0);
	posit16_t tmp3 = convertDoubleToP16 (6.700000000000000177635683940025E+0);
	posit16_t  B[3] = {tmp1, tmp2, tmp3};
	posit16_t  A[10][10];
	posit16_t  *uu;
	posit16_t  **uuu;
	int **pp;
	posit16_t u;posit16_t px;
	u = convertDoubleToP16 (3.3999999999999999111821580299875E+0);;
	posit16_t z = convertDoubleToP16 (0);
	posit16_t min;
	posit16_t minU2 = convertDoubleToP16 (1.000000000000000019884624838656E+30);posit16_t maxU2 = convertDoubleToP16 (1.000000000000000019884624838656E+30);posit16_t u2;
}

#define LOCAL( grid, C  ) A[0][0]
#define DFL1 +2.3*4.3
int foo(){
	int n = 10;
	int m = 10;
	int i,j;
	double A[10][10];
	double z, u;
	u = 3.4;
	
	for (i = 0; i < m; i++){
    for (j = 0; j < n; j++) {
//			LOCAL( grid, C  ) = DFL1;
	//		z = +u * 3;
			z = +u * -3;
			//z = +u * -3.0;
		}
	}
}

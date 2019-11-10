double xx;
double B[3][3], C[3][3];
struct foos{
  double x;
  double y;
  double sum;
};
struct foos foo(double *x, double *y){
  struct foos f;
  f.x = xx++;
  f.y = *y++ + f.x;
  f.sum = f.x + f.y;
  return f;   
}

void testMat(double **A, double B[][3]){
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      C[i][j] = (A[i][j]) + (B[i][j]);
    }
  }
}
int main() {
  double x,y;
	struct foos f;
  double **A = (double **)malloc(3*sizeof(double *));
  for(int i =0; i< 3; i++)
    A[i] = (double *)malloc(3*sizeof(double)); 

  x = 1.1;
  y = sqrt(x);
  f = foo(&x, &y);
  printf("%e\n", f.x);
  printf("%e\n", f.x);
  printf("%e\n", f.y);
  printf("%e\n", f.sum);
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      A[i][j] = i*0.3+i;
      B[i][j] = (i+j)*0.55;
    }
  }

  testMat(A, B);
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      printf("%e ", C[i][j]);
    }
    printf("\n");
  }

  return 0;
}

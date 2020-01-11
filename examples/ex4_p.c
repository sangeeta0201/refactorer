# 1 "ex4.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 341 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "ex4.c" 2

double C[3][3];
void testMat(double **A, double B[][3], double alpha){
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      for(int k =0; k<3; k++){

        C[i][j] += alpha ;
      }
    }
  }
}

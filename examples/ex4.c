double C[3][3];
void testMat(double **A, double B[][3]){
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      C[i][j] = A[i][j] + B[i][j];
    }
  }
}

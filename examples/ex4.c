#define DATA_TYPE double
double C[3][3];
void testMat(double **A, double B[][3], DATA_TYPE alpha){
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      for(int k =0; k<3; k++){
//        C[i][j] = A[i][j] + B[i][j];
        C[i][j] += alpha ;
      }
    }
  }
}

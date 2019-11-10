posit32_t C[3][3];

void testMat(posit32_t  **A, double B[][3]){
  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      posit32_t tmp1 = rapl_p32_force_load(&A[i][j]);
      
      posit32_t tmp2 = rapl_p32_force_load(&B[i][j]);
      
      posit32_t tmp3 = p32_add(tmp1,tmp2);
      rapl_p32_force_store(&C[i][j],tmp3);
      
    }
  }
}

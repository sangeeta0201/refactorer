# 1 "ex4.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 341 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "ex4.c" 2

posit32_t C[3][3];

void testMat(posit32_t  **A, double B[][3], posit32_t tmp1){
  posit32_t tmp2 = rapl_p32_get_arg(tmp1);
  posit32_t  alpha;
  rapl_p32_force_store(&(alpha),tmp2);

  for(int i =0; i<3; i++){
    for(int j =0; j<3; j++){
      for(int k =0; k<3; k++){

        posit32_t tmp3 = rapl_p32_force_load(&C[i][j]);
        
        posit32_t tmp4 = rapl_p32_force_load(&alpha);
        
        posit32_t tmp6 = p32_add(tmp3,tmp4);
        rapl_p32_force_store(&(C[i][j]),tmp6);
        
      }
    }
  }
}

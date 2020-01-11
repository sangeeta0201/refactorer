posit32_t  f(posit32_t tmp1) {
  posit32_t tmp2 = rapl_p32_get_arg(tmp1);
  posit32_t  x;
  rapl_p32_force_store(&(x),tmp2);

}
void foo(){
  posit32_t oldX;posit32_t x;
  posit32_t tmp3 = convertDoubleToP32 (2.5E+1);
  rapl_p32_force_store(&(x),tmp3);
  
  posit32_t eps;
  posit32_t tmp4 = convertDoubleToP32 (9.9999997473787516355514526367188E-6);
  rapl_p32_force_store(&(eps),tmp4);
  
  int maxStep = 100;
  int iter = 0;
  do {
    /*
    oldX = x;
    float diffX = df(x);
    if (fabsf(diffX) != 0.0 && diffX != 0) {
      x = x - f(x) / diffX;
    }
    else {
      printf("Unexpected slope\n");
      exit(1);
    }
   // iter ++;
   */
  posit32_t tmp5 = rapl_p32_force_load(&(oldX));
  
  posit32_t tmp6 = rapl_p32_force_load(&(x));
  
  posit32_t tmp7 = p32_sub(tmp5,tmp6);
  posit32_t tmp8 = rapl_p32_set_arg(tmp7,1);
  posit32_t tmp9 = p32_fabsf(tmp8);
  posit32_t tmp10 = rapl_p32_get_ret(tmp9);
  posit32_t tmp11 = rapl_p32_force_load(&(eps));
  
  } while (!p32_le(tmp10,tmp11) && iter < maxStep);

//  for(float eps = 0.00001f; fabsf(oldX - x) > eps && iter < maxStep; eps = fabsf(oldX - x)){
//  }
}

typedef double **triangle;
struct otri {
    triangle *tri;
      int orient;
};
struct mesh {
  int areaboundindex;
};
int foo(posit32_t tmp1){
  posit32_t tmp2 = rapl_p32_get_arg(tmp1);
  posit32_t  z;
  rapl_p32_force_store(&z,tmp2);

  posit32_t B[4];
  posit32_t C1[8];
  
  posit32_t *finnow;posit32_t *finother;posit32_t *finswap;
  posit32_t area;posit32_t *regionlist;
  regionlist = (posit32_t  *) malloc(4 * (int) sizeof(posit32_t ));
  posit32_t x;posit32_t exponent;posit32_t length;
  struct otri *newotri;
  struct mesh *m;
  posit32_t tmp25 = rapl_p32_force_load(&z);
  
  posit32_t tmp26 = rapl_p32_force_load(&exponent);
  
  posit32_t tmp27 = p32_mul(tmp25,tmp26);
  rapl_p32_force_store(&x,tmp27);
  
  char workstring[10]={'2', '.', '3'};
  posit32_t tmp22 = rapl_p32_force_load(&x);
  
  posit32_t tmp23 = rapl_p32_force_load(&x);
  
  posit32_t tmp24 = p32_add(tmp22,tmp23);
  rapl_p32_force_store(&x,tmp24);
  
  
  
  posit32_t tmp28 = convertDoubleToP32 (0);
  posit32_t tmp29 = convertDoubleToP32 (1.0);
  posit32_t tmp30 = p32_sub(tmp28,tmp29);
  rapl_p32_force_store(&((doposit32_t )(*newotri).tri)[m->areaboundindex],tmp30);
  
  posit32_t tmp32 = rapl_p32_force_load(&area);
  
  posit32_t tmp33 = rapl_p32_force_load(&((double *)(*newotri).tri)[m->areaboundindex]);
  
  if ( (!p32_le(tmp32,tmp33))){
    posit32_t tmp17 = convertDoubleToP32 (0);
    posit32_t tmp18 = convertDoubleToP32 (1.0);
    posit32_t tmp19 = p32_sub(tmp17,tmp18);
    posit32_t tmp20 = rapl_p32_force_load(&((double *)(*newotri).tri)[m->areaboundindex]);
    
    posit32_t tmp21 = p32_posit32_t p19,tmp20);
    rapl_p32_force_store(&((double *)(*newotri).tri)[m->areaboundindex],tmp21);
    
  }
  posit32_t tmp9 = convertDoubleToP32 (2.0E+0);
  posit32_t tmp10 = rapl_p32_force_load(&exponent);
  
  posit32_t tmp11 = p32_mul(tmp9,tmp10);
  posit32_t tmp12 = rapl_p32_force_load(&length);
  
  posit32_t tmp13 = convertDoubleToP32 (1.4142135623730951454746218587388E+0);
  bool tmp14 = !p32_le(tmp12,tmp13);
  posit32_t tmp15 = convertDoubleToP32 (tmp14);
  posit32_t tmp16 = p32_add(tmp11,tmp15);
  rapl_p32_force_store(&exponent,tmp16);
  
  posit32_t tmp3 = convertDoubleToP32 (2.0E+0);
  posit32_t tmp4 = rapl_p32_force_load(&length);
  
  posit32_t tmp5 = convertDoubleToP32 (1.4142135623730951454746218587388E+0);
  bool tmp6 = !p32_le(tmp4,tmp5);
  posit32_t tmp7 = convertDoubleToP32 (tmp6);
  posit32_t tmp8 = p32_mul(tmp3,tmp7);
  rapl_p32_force_store(&exponent,tmp8);
  
}

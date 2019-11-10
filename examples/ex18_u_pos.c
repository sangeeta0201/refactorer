# 1 "examples/ex18.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 341 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "examples/ex18.c" 2


void update_u( posit32_t  eps ){
 posit32_t t2;posit32_t t3;posit32_t t4;posit32_t t5;posit32_t t6;
 t2 = convertDoubleToP32 (2.0E+0);;
 t3 = convertDoubleToP32 (3.0E+0);;
 t4 = convertDoubleToP32 (4.0E+0);;
 {posit32_t tmp3 = p32_mul(t3,t3);
 t3 = tmp3; posit32_t tmp2 = p32_mul(t4,t4);
 t4 = tmp2; posit32_t tmp1 = p32_add(t3,t4);
 t2 = tmp1;};

}


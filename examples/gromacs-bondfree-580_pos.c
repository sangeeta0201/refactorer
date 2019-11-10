#include <math.h>
#include <stdio.h>
 
posit32_t  inline float iprod(posit32_t * a,posit32_t * b)
{
  posit32_t tmp31 = p32_mul(a[1],b[1]);
  posit32_t tmp32 = p32_mul(a[0],b[0]);
  posit32_t tmp33 = p32_mul(a[2],b[2]);
  posit32_t tmp34 = p32_add(tmp32,tmp31);
  posit32_t tmp35 = p32_add(tmp34,tmp33);
  return (tmp35);
}

posit32_t tmp1 = convertDoubleToP32 (268453443323035648);
posit32_t tmp2 = convertDoubleToP32 (7.16530621051788330078125E-1);
posit32_t tmp3 = convertDoubleToP32 (1.389514892578125E+3);
posit32_t tmp4 = convertDoubleToP32 (2.0770905828637609147335751913488E-10);
posit32_t tmp5 = convertDoubleToP32 (4.5552733528942065652909995415826E-24);
posit32_t tmp6 = convertDoubleToP32 (2.79381101586650970602931920439E-10);
posit32_t tmp7 = convertDoubleToP32 (9.2040725052356719970703125E-2);
posit32_t tmp8 = convertDoubleToP32 (2.2920905612409114837646484375E-3);
posit32_t tmp9 = convertDoubleToP32 (5.4301075065642613301441921492933E-21);
posit32_t tmp10 = convertDoubleToP32 (943433600);
posit32_t tmp11 = convertDoubleToP32 (5.7629808612905003153595430473599E-30);
posit32_t tmp12 = convertDoubleToP32 (1.974807886819034953540684389843E-40);
posit32_t tmp13 = convertDoubleToP32 (3.7158469012865680269896984100342E-7);
posit32_t tmp14 = convertDoubleToP32 (1.84290313720703125E+2);
posit32_t tmp15 = convertDoubleToP32 (1.8683431344172884485215940021189E-29);
posit32_t tmp16 = convertDoubleToP32 (4.2083037499196507269516587257385E-8);
posit32_t tmp17 = convertDoubleToP32 (1.8677865192031556993830263903784E-39);
posit32_t tmp18 = convertDoubleToP32 (3.7274539151040134086571206915512E-43);
posit32_t tmp19 = convertDoubleToP32 (5.8237964177339397467590201481529E-42);
posit32_t tmp20 = convertDoubleToP32 (4.9045446251368597482330535415147E-44);
posit32_t tmp21 = convertDoubleToP32 (3.2664267203411485923232136586488E-42);
posit32_t tmp22 = convertDoubleToP32 (3.40728424072265625E+2);
posit32_t tmp23 = convertDoubleToP32 (2.7634128318965167636633850634098E-10);
posit32_t tmp24 = convertDoubleToP32 (1.2054989013671875E+3);
posit32_t tmp25 = convertDoubleToP32 (1056530038784);
posit32_t tmp26 = convertDoubleToP32 (1067773132800);
posit32_t tmp27 = convertDoubleToP32 (4.8185892015429297610733233166724E-37);
posit32_t tmp28 = convertDoubleToP32 (5.3226999007165431976318359375E-3);
posit32_t tmp29 = convertDoubleToP32 (1.2250422945568828905260918292789E-22);
posit32_t tmp30 = convertDoubleToP32 (8.1286535331538631898314414904777E-39);
posit32_t r_kj[10][3] = {
  {tmp1, tmp2, tmp3},
  {tmp4, tmp5, tmp6},
  {tmp7, tmp8, tmp9},
  {tmp10, tmp11, tmp12},
  {tmp13, tmp14, tmp15},
  {tmp16, tmp17, tmp18},
  {tmp19, tmp20, tmp21},
  {tmp22, tmp23, tmp24},
  {tmp25, tmp26, tmp27},
  {tmp28, tmp29, tmp30}
};

#include "softposit.h"
int main() {
  posit32_t nrkj2;posit32_t nrkj;
  for (int i = 0; i < 10; i++){
    nrkj2 = iprod(r_kj[i], r_kj[i]);
    nrkj = p16_sqrt(nrkj2);
    double tmp36 = convertP32ToDouble (nrkj);
    printf("%e\n", tmp36);
  }
  return 0;
}

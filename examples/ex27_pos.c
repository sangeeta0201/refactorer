#include<stdio.h>
#define REAL double

typedef REAL **triangle;
struct otri {                                                                   
  triangle *tri;
  int orient;
};

struct mesh {
	int areaboundindex;  
};

REAL foo(){
	REAL dxoa, dxda, dxod;
  REAL dyoa;
  REAL oalen, dalen, odlen, det;
  REAL maxlen;
  REAL triorg[10];
  REAL triapex[10];
	REAL acxtail, acytail, bcxtail, bcytail; 
	struct mesh *m;
	int plus1mod3[3] = {1, 2, 0};
	int minus1mod3[3] = {2, 0, 1};
	struct otri *t;

	typedef REAL *vertex;
	vertex printvertex;	
	printvertex = (vertex) (*t).tri[plus1mod3[(*t).orient] + 3];

	posit32_t tmp11 = convertDoubleToP32 (0.0E+0);
	if (p32_eq(*printvertex,tmp11)) {
		posit32_t tmp13 = ((posit32_t *)(*t).tri)[m->areaboundindex];
		double tmp14 = convertP32ToDouble (tmp13);
		printf("    Area constraint:  %.4g\n", tmp14);    
  }

  if (printvertex == (vertex) NULL) {
   printf("    Origin[%d] = NULL\n", (t->orient + 1) % 3 + 3);

  } else {
	 printvertex = (vertex) (*t).tri[plus1mod3[(*t).orient] + 3];
	 }

    double tmp28 = convertP32ToDouble (printvertex[0]);
    double tmp29 = convertP32ToDouble (printvertex[1]);
    printf("    Origin[%d] = x%lx  (%.12g, %.12g)\n",
           (t->orient + 1) % 3 + 3, (unsigned long) printvertex,
           tmp28, tmp29);

  printvertex = (vertex) (*t).tri[minus1mod3[(*t).orient] + 3];
  if (printvertex == (vertex) NULL) {
    printf("    Dest  [%d] = NULL\n", (t->orient + 2) % 3 + 3);
  } else {
    double tmp30 = convertP32ToDouble (printvertex[0]);
    double tmp31 = convertP32ToDouble (printvertex[1]);
    printf("    Dest  [%d] = x%lx  (%.12g, %.12g)\n",
           (t->orient + 2) % 3 + 3, (unsigned long) printvertex,
           tmp30, tmp31);
           }
  printvertex = (vertex) (*t).tri[(*t).orient + 3];
  if (printvertex == (vertex) NULL) {
    printf("    Apex  [%d] = NULL\n", t->orient + 3);
  } else {
    double tmp32 = convertP32ToDouble (printvertex[0]);
    double tmp33 = convertP32ToDouble (printvertex[1]);
    printf("    Apex  [%d] = x%lx  (%.12g, %.12g)\n",
           t->orient + 3, (unsigned long) printvertex,
           tmp32, tmp33);
           }

  posit32_t tmp10 = p32_sub(triorg[0],triapex[0]);
  dxoa = tmp10;
  posit32_t tmp9 = p32_sub(triorg[1],triapex[1]);
  dyoa = tmp9;
	int x =0;
	if(x > 5) {
    return 1;
    }
	posit32_t tmp2 = p32_mul(triorg[1],triorg[1]);
	posit32_t tmp3 = p32_mul(triorg[0],triorg[0]);
	posit32_t tmp4 = p32_add(tmp3,tmp2);
	posit32_t tmp5 = convertDoubleToP32 (5.0000000000000002775557561562891E-2);
	posit32_t tmp6 = p32_mul(tmp5,tmp4);
	posit32_t tmp7 = convertDoubleToP32 (2.0000000000000000416333634234434E-2);
	posit32_t tmp8 = p32_add(tmp6,tmp7);
	if (!p32_le(maxlen,tmp8)) {
    return 1;
  } else {
    return 0;
  }

	posit32_t tmp17 = convertDoubleToP32 (1);
	posit32_t tmp18 = p32_sub(tmp17,dxoa);
	if ((!p32_lt(dxoa,oalen)) || (!p32_lt(tmp18,oalen))) { 
  	posit32_t tmp1 = p32_sub(triorg[1],triapex[1]);
  	dyoa = tmp1;
  	}
	posit32_t tmp20 = convertDoubleToP32 (0.0E+0);
	posit32_t tmp22 = convertDoubleToP32 (0.0E+0);
	posit32_t tmp24 = convertDoubleToP32 (0.0E+0);
	posit32_t tmp26 = convertDoubleToP32 (0.0E+0);
	if ((p32_eq(acxtail,tmp20)) && (p32_eq(acytail,tmp22))
      && (p32_eq(bcxtail,tmp24)) && (p32_eq(bcytail,tmp26))) {
    return det;
  }

}

//#include<stdio.h> 
#define REAL double                                                             
//FILE *outfile;
typedef REAL **triangle;
struct otri {                                                                   
  triangle *tri;
  int orient;
};

struct mesh {
  int areaboundindex;  
	posit32_t minangle;
	int vararea;
	int elemattribindex; 
	int eextras; 
	REAL xminextreme;
	REAL xmin, xmax, ymin, ymax;
};
typedef REAL *vertex; 
REAL splitter;
REAL epsilon;                                                                   
REAL resulterrbound;
REAL ccwerrboundA, ccwerrboundB, ccwerrboundC;                                  
REAL iccerrboundA, iccerrboundB, iccerrboundC;
REAL o3derrboundA, o3derrboundB, o3derrboundC;

struct otri *newotri;
struct otri newbotright, botright;
struct mesh *b;
struct otri *testtri;
posit32_t  **triangletraverse(m)
struct mesh *m;

{
	vertex vertexloop;
	double tmp9 = convertP32ToDouble (vertexloop[0]);
	double tmp10 = convertP32ToDouble (vertexloop[1]);
	printf(" %.17g  %.17g  %.17g\n", tmp9, tmp10,       
              0.0);
}
REAL foo(m, dyingtriangle)
struct mesh *m;
triangle *dyingtriangle;
{

	REAL det, acytail, detsum, errbound, acx, bcytail, detleft,detright, 
		bcy, acxtail, acy, bcxtail, bcx, fnow, enow, Q, area,minedge;
	int eindex, elen , findex, flen, hindex;
  /*
	REAL h[10];
	REAL *xi;                                                                    
	REAL *eta;
	vertex torg, tapex, tdest; 
	REAL xdo, ydo, xao, yao;                                                      
  REAL dodist, aodist, dadist;
  REAL denominator;
  REAL dx, dy, dxoff, dyoff, dyy, length;
	REAL dxa, dya, dxb, dyb;

	REAL leftccw, multiplier;
	vertex newvertex; 
	vertex eorg, edest, eapex;
	REAL radconst, degconst, biggestangle; 

	biggestangle = degconst * acos(sqrt(biggestangle));

	m->xminextreme = 10 * m->xmin - 9 * m->xmax;
	leftccw += multiplier * (edest[1] - eorg[1]);
	newvertex[0] += multiplier * (edest[1] - eorg[1]);
	int leftflag;
	leftflag  = leftccw > 0.0;

	struct otri triangleloop;
	int i = 0;
	int j = 0;
	((REAL *) (triangleloop).tri)[m->elemattribindex + (j)] = 0;

	for (int i = 0; i < m->eextras; i++) {
    ((REAL *) (*newotri).tri)[m->elemattribindex + (i)] = 0.0;
  }


	if ((b->vararea) && (area > ((REAL *) (*testtri).tri)[m->areaboundindex]) &&
        (((REAL *) (*testtri).tri)[m->areaboundindex] > 0.0)) {

      enqueuebadtri(m, b, testtri, minedge, tapex, torg, tdest);
    }

		int exponent;
		exponent = 2.0 * exponent + (length > 1.4142135623730950488016887242096980785696718753769480732); 
		dyy = (yao * dx - xao * dy) * (2.0 * denominator);
  	*eta = (2.0 * denominator);
		errbound = ccwerrboundC * detsum + resulterrbound * ((det) >= 0.0 ? (det) : -(det));
	if ((Q != 0.0) || (hindex == 0)) {
		h[hindex++] = Q;
	}
	if ((eindex < elen) && (findex < flen)) {
		enow = fnow * enow;
 		if ((fnow > enow) == (fnow > -enow)) {
			if (detleft > det == detleft > 0.0) {
				return dya * dxb  > dxa * dxa;
			}
		}
	}
	if (detright <= 0.0) {
      return det;
    } else {
      detsum = detleft + detright;
	}
	if (detleft < 0.0) {
    if (detright >= 0.0) {
      return det;
    } else {
      detsum = -detleft - detright;
    }
  } else {
    return det;
  }

	if (errbound > 0.0) {
    if (errbound <= 0.0) {
      return det;
    } else {
      detsum = detleft + errbound;
    }
  } else if (detleft < 0.0) {
    if (errbound >= 0.0) {
      return det;
    } else {
      detsum = -detleft - errbound;
    }
  } 

	if (errbound > 0.0) {
		double d = 0.5 / bar(det, detsum);
    int k;
		k =  bar(det, 2.0 * detsum);
    return 0.5 / bar(det, detsum);
  }

	if (dxoff * dxoff + dyoff * dyoff <
          (dx - xdo) * (dx - xdo) + (dy - ydo) * (dy - ydo)) {
        dx = xdo + dxoff;
        dy = ydo + dyoff;
	}

  det += (acx * bcytail + bcy * acxtail)
       - (acy * bcxtail + bcx * acytail);
 	if ((det >= errbound) || (-det >= errbound)) {
    return det;
  }
*/
  posit32_t c;posit32_t bdheight;
  posit32_t attrib;
  struct otri top, horiz;

  if ((b->vararea) && (!p32_le(area,((posit32_t  *)(*testtri).tri)[m->areaboundindex]))){
    posit32_t tmp8 = p32_mul(splitter,bdheight);
    c = (posit32_t ) (tmp8);  
  }
  posit32_t tmp1 = convertDoubleToP32 (0);
  posit32_t tmp2 = convertDoubleToP32 (1.0);
  posit32_t tmp3 = p32_sub(tmp1,tmp2);
  ((posit32_t  *) (*newotri).tri)[m->areaboundindex] = tmp3;
  ((int *) (*newotri).tri)[m->areaboundindex] = 1;
  double tmp11 = convertP32ToDouble (((posit32_t  *)(*newotri).tri)[m->areaboundindex]);
  printf("    Area constraint:  %.4g\n", tmp11);
  
  posit32_t tmp7 = p32_mul(splitter,bdheight);
  c = (posit32_t ) (tmp7);  
 
  ((posit32_t  *) (newbotright).tri) [m->elemattribindex + (0)]   = ((posit32_t  *) (botright).tri)[m->elemattribindex + (0)];
 
  posit32_t tmp4 = p32_add(((posit32_t  *)(top).tri)[m->elemattribindex + (0)],((posit32_t  *)(horiz).tri)[m->elemattribindex + (0)]);
  posit32_t tmp5 = convertDoubleToP32 (5.0E-1);
  posit32_t tmp6 = p32_mul(tmp5,tmp4);
  attrib = tmp6;
  ((posit32_t  *) (top).tri)[m->elemattribindex + (0)] = attrib;
  ((posit32_t  *) (horiz).tri)[m->elemattribindex + (0)] = attrib;

/*
	det = (acx * bcytail + bcy * acxtail) - 
				(acy * bcxtail + bcx * acytail);
	acytail = -acytail;
  m->minangle = -2.0;
	((double *) (*newotri).tri)[m->areaboundindex] = -1.0;
	if ((det >= errbound) || (-det >= errbound)) {
    return det;
  }
	det = detsum *  ((det) >= 0.0 ? (det) : -(det));

	errbound = ccwerrboundC * detsum + resulterrbound * ((det) >= 0.0 ? (det) : -(det));

	det = (bcytail + bcy * acxtail) - ( bcytail  + bcx * acytail);
	det = (acx * bcytail + bcy * acxtail) - 
				(acy * bcxtail + bcx * acytail);
        */
}

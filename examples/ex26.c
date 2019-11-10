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
	double minangle;
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
double **triangletraverse(m)
struct mesh *m;

{
	vertex vertexloop;
	printf(" %.17g  %.17g  %.17g\n", vertexloop[0], vertexloop[1],       
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
  double c, bdheight;
  double attrib;
  struct otri top, horiz;

  if ((b->vararea) && (area > ((double *) (*testtri).tri)[m->areaboundindex])){
    c = (double) (splitter * bdheight);  
  }
  ((double *) (*newotri).tri)[m->areaboundindex] = -1.0;
  ((int *) (*newotri).tri)[m->areaboundindex] = 1;
  printf("    Area constraint:  %.4g\n", ((double *) (*newotri).tri)[m->areaboundindex]);
  
  c = (double) (splitter * bdheight);  
 
  ((double *) (newbotright).tri) [m->elemattribindex + (0)]   = ((double *) (botright).tri)[m->elemattribindex + (0)];
 
  attrib = 0.5 * (((double *) (top).tri)[m->elemattribindex + (0)] + ((double *) (horiz).tri)[m->elemattribindex + (0)]);
  ((double *) (top).tri)[m->elemattribindex + (0)] = attrib;
  ((double *) (horiz).tri)[m->elemattribindex + (0)] = attrib;

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

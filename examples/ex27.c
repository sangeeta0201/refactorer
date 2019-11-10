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

	if (*printvertex == 0.0) {
		printf("    Area constraint:  %.4g\n", ((REAL *) (*t).tri)[m->areaboundindex]);    
  }

  if (printvertex == (vertex) NULL)
   printf("    Origin[%d] = NULL\n", (t->orient + 1) % 3 + 3);

  else
	 printvertex = (vertex) (*t).tri[plus1mod3[(*t).orient] + 3];

    printf("    Origin[%d] = x%lx  (%.12g, %.12g)\n",
           (t->orient + 1) % 3 + 3, (unsigned long) printvertex,
           printvertex[0], printvertex[1]);

  printvertex = (vertex) (*t).tri[minus1mod3[(*t).orient] + 3];
  if (printvertex == (vertex) NULL)
    printf("    Dest  [%d] = NULL\n", (t->orient + 2) % 3 + 3);
  else
    printf("    Dest  [%d] = x%lx  (%.12g, %.12g)\n",
           (t->orient + 2) % 3 + 3, (unsigned long) printvertex,
           printvertex[0], printvertex[1]);
  printvertex = (vertex) (*t).tri[(*t).orient + 3];
  if (printvertex == (vertex) NULL)
    printf("    Apex  [%d] = NULL\n", t->orient + 3);
  else
    printf("    Apex  [%d] = x%lx  (%.12g, %.12g)\n",
           t->orient + 3, (unsigned long) printvertex,
           printvertex[0], printvertex[1]);

  dxoa = triorg[0] - triapex[0];
  dyoa = triorg[1] - triapex[1];
	int x =0;
	if(x > 5)
    return 1;
	if (maxlen > 0.05 * (triorg[0] * triorg[0] + triorg[1] * triorg[1]) + 0.02) {
    return 1;
  } else {
    return 0;
  }

	if ((dxoa >= oalen) || (-dxoa >= oalen)) 
  	dyoa = triorg[1] - triapex[1];
	if ((acxtail == 0.0) && (acytail == 0.0)
      && (bcxtail == 0.0) && (bcytail == 0.0)) {
    return det;
  }

}

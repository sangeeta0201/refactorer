#include<stdio.h> 
#define REAL double                                                             
FILE *outfile;
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
double **triangletraverse(m)
struct mesh *m;

{
	vertex vertexloop;
	struct otri triangleloop;
	int p1,vedgenumber;
	vertex torg, tdest, tapex;  
//	printf(" %.17g  %.17g  %.17g\n", vertexloop[0], vertexloop[1],       
  //            0.0);
//	fprintf(outfile, "  %.17g", ((REAL *) (triangleloop).tri)[m->elemattribindex + (0)]);
	fprintf(outfile, "%4ld   %d  %d   %.17g  %.17g\n", vedgenumber,
                  p1, -1, tdest[1] - torg[1], torg[0] - tdest[0]);
}

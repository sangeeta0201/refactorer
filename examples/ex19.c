#define fma(a,b,c) {b = b *b; c = c*c; a = b + c;}

void update_u( double eps ){
	unsigned margin = 4;
	unsigned size   = 2*margin*sizeof( double );
  printf( "LBM_allocateGrid: allocated %.1f MByte\n",
          size / (1024.0*1024.0), size / (1024.0*1024.0) *3.2 );
}

void foo(){
	double eps = 3.4;
	update_u(4*6.5/3.4);
}

# 1 "ex7.c"
# 1 "<built-in>"
# 1 "<command-line>"
# 31 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 32 "<command-line>" 2
# 1 "ex7.c"
# 1 "test.h" 1
# 2 "ex7.c" 2

double B[10];


int foo(){
 int n = 10;
 int m = 10;
 int i,j;
 double A[10][10];
 for (i = 0; i < m; i++){
    for (j = 0; j < n; j++) {
   A[i][j] = (((double) ((i*j) % m) / m )*100) + 10;


  }
 }
}

int main(){
 foo();
}

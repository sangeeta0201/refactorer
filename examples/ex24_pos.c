#include<stdlib.h>
#include "softposit.h"
int main(){
char szOrbits[] = "365.24 29.53";
  char* pEnd;
  posit32_t d1;posit32_t d2;
  d1 = p32_strtod (szOrbits, &pEnd);
  d2 = p32_strtod (pEnd, NULL);
  posit32_t tmp1 = p32_div(d1,d2);
  double tmp2 = convertP32ToDouble (tmp1);
  printf ("The moon completes %.2f orbits per Earth year.\n", tmp2);
}

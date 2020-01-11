float f(float x) {
}
void foo(){
  float oldX, x = 25.0f;
  float eps = 0.00001f;
  int maxStep = 100;
  int iter = 0;
  do {
    /*
    oldX = x;
    float diffX = df(x);
    if (fabsf(diffX) != 0.0 && diffX != 0) {
      x = x - f(x) / diffX;
    }
    else {
      printf("Unexpected slope\n");
      exit(1);
    }
   // iter ++;
   */
  } while (fabsf(oldX - x) > eps && iter < maxStep);

//  for(float eps = 0.00001f; fabsf(oldX - x) > eps && iter < maxStep; eps = fabsf(oldX - x)){
//  }
}

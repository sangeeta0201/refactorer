int plus1mod3[3] = {1, 2, 0};

int foo(){
	double dalen = 3.2;
	double oalen = 2.3;
	double odlen = 21.3;
	double triorg[2];
	double maxlen; 


	if(dalen > oalen) 
		maxlen= dalen;
	else
		maxlen= oalen;


  maxlen = (odlen*3.5 > maxlen) ? odlen : maxlen;

  if (maxlen > 0.05 * (triorg[0] * triorg[0] + triorg[1] * triorg[1]) + 0.02) {
    return 1;
  } else {
    return 0;
  }

}

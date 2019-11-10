int plus1mod3[3] = {1, 2, 0};

int foo(){
	posit32_t dalen = convertDoubleToP32 (3.200000000000000177635683940025E+0);
	posit32_t oalen = convertDoubleToP32 (2.299999999999999822364316059975E+0);
	posit32_t odlen = convertDoubleToP32 (2.13000000000000007105427357601E+1);
	posit32_t  triorg[2];
	posit32_t maxlen; 


	if(!p32_le(dalendalen,oalenoalen)) { 
		maxlen= dalen;
	} else {
		maxlen= oalen;
		}


  posit32_t tmp8 = convertDoubleToP32 (3.5E+0);
  posit32_t tmp9 = p32_mul(odlenodlen,tmp83.5);
  maxlen = (!p32_le(tmp9odlen * 3.5,maxlenmaxlen)) ? odlen : maxlen;

  posit32_t tmp1 = p32_mul(triorg[1],triorg[1]);
  posit32_t tmp2 = p32_mul(triorg[0],triorg[0]);
  posit32_t tmp3 = p32_add(tmp2triorg[0] * triorg[0],tmp1triorg[1] * triorg[1]);
  posit32_t tmp4 = convertDoubleToP32 (5.0000000000000002775557561562891E-2);
  posit32_t tmp5 = p32_mul(tmp40.050000000000000003,tmp3triorg[0] * triorg[0] + triorg[1] * triorg[1]);
  posit32_t tmp6 = convertDoubleToP32 (2.0000000000000000416333634234434E-2);
  posit32_t tmp7 = p32_add(tmp50.050000000000000003 * (triorg[0] * triorg[0] + triorg[1] * triorg[1]),tmp60.02);
  if (!p32_le(maxlenmaxlen,tmp70.050000000000000003 * (triorg[0] * triorg[0] + triorg[1] * triorg[1]) + 0.02)) {
    return 1;
  } else {
    return 0;
  }

}


//float arr[2] = {1.2, 2.3};
posit16_t  arr1[2] ;
__attribute__ ((constructor)) void __init_arr1(void){
        arr1[0] = convertDoubleToP16 (1.1999999999999999555910790149937E+0);
        arr1[1] = convertDoubleToP16 (2.299999999999999822364316059975E+0);

}

//float arr2[2][2] = {{1.2, 3.2},
	//									{2.2, 3}};

posit16_t  arr3[3][3] ;
__attribute__ ((constructor)) void __init_arr3(void){
        arr3[0][0] = convertDoubleToP16 (1.1999999999999999555910790149937E+0);
        arr3[0][1] = convertDoubleToP16 (3.200000000000000177635683940025E+0);
        arr3[0][2] = convertDoubleToP16 (5.5E+0);
        arr3[1][0] = convertDoubleToP16 (2.200000000000000177635683940025E+0);
        arr3[1][1] = convertDoubleToP16 (3.3999999999999999111821580299875E+0);
        arr3[1][2] = convertDoubleToP16 (5.5999999999999996447286321199499E+0);
        arr3[2][0] = convertDoubleToP16 (2.200000000000000177635683940025E+0);
        arr3[2][1] = convertDoubleToP16 (3.3999999999999999111821580299875E+0);
        arr3[2][2] = convertDoubleToP16 (5.5999999999999996447286321199499E+0);

}



int foo(struct behavior *b){
	double nearestpoweroftwo, segmentlength, split, acutedest;
	nearestpoweroftwo = 1.0;
	while (segmentlength > 3.0 * nearestpoweroftwo) {
		nearestpoweroftwo *= 2.0;
	}
	while (segmentlength < 1.5 * nearestpoweroftwo) {
		nearestpoweroftwo *= 0.5;
	}

	split = nearestpoweroftwo / segmentlength;

	if (acutedest == 2.0) {
		split = 1.0 - split;
	}
}

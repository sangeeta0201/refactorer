#define N_CELL_ENTRIES 100
int foo(){
	int j;
	int i =5;
	double px;
	px = ((N_CELL_ENTRIES)  / 0.5) - 1.0;
	px = (((i / N_CELL_ENTRIES) % (1*(100))) / (0.5*((1*(100))-1))) - 1.0;
	px = (((i / N_CELL_ENTRIES) % (1*(100))) / (0.5*((1*(100))-1))) - 1.0;
	px = (i % 100 / (0.5-1)) ;
}

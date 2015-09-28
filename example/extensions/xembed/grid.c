#include <stdio.h>


static int grid(int w, int h, int gridsize, int x0, int y0, int w0, int h0)
{
	int i, j;
	int remainder;
	int _x = x0;
	int _y = y0;
	int _w = w0;
	int _h = h0;

	remainder = x0 % gridsize;
	x0 -= remainder;
	w0 += remainder;
	remainder = w0 % gridsize;
	if (remainder > 0)
		w0 += (gridsize - remainder);
	
	remainder = y0 % gridsize;
	y0 -= remainder;
	h0 += remainder;
	remainder = h0 % gridsize;
	if (remainder > 0)
		h0 += (gridsize - remainder);

	printf("(%d, %d, %d, %d) -> (%d, %d, %d, %d)\n", _x, _y, _w, _h, x0, y0, w0, h0);
	for (i=y0; i<y0+h0; i+=gridsize)
		for (j=x0; j<x0+w0; j+=gridsize)
		{
			int gw = gridsize;
			int gh = gridsize;
			if (j+gw > w)
				gw = w - j;
			if (i+gh > h)
				gh = h - i;
			
			printf("%d %d %d %d\n", j, i, gw, gh);
		}

	return 1;
}

int main(void)
{
	grid(175, 127, 40, 1, 2, 167, 98);
	return 0;
}

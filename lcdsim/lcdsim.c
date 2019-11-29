/*

For compiling and execution--
cc -o lcdsim lcdsim.c -lncurses; ./lcdsim < /dev/ttyUSB0

	 Set row & column codes 
	uint8_t* p = pbcb->pbuf;
	*p++ = (254); // move cursor command

	// determine position
	if (row == 0) {
		*p = (128 + col);
	} else if (row == 1) {
		*p = (192 + col);
	} else if (row == 2) {
		*p = (148 + col);
	} else if (row == 3) {
		*p = (212 + col);
	}

*/


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>                  /*  for sleep()  */
#include <curses.h>
#include <string.h>

int main(void) {

    WINDOW * mainwin;
	int c;
	int row = 0;
	int col = 0;
	int state = 3;
	int count = 0;
	char t[2];

	/* Emtpy line */
	char linex[32] = {' '};
	linex[31] = 0;

	
    
    /*  Initialize ncurses  */

    if ( (mainwin = initscr()) == NULL ) {
	fprintf(stderr, "Error initializing ncurses.\n");
	exit(EXIT_FAILURE);
    }

	while(1==1)
	{
		c = fgetc(stdin);
count += 1;

		if (c == 0xFE) // Beginning of new line
			state = 1;

		switch (state)
		{
		case 1: // Beginning of new line
			state = 2;
//	    	refresh();
			col = 0;
			break;
		case 2: // Char following beginning of line
			if (c == 128) row = 0;
			if (c == 192) row = 1;
			if (c == 148) row = 2;
			if (c == 212) row = 3;
		   mvaddstr(row+1,0,linex); // Clear new line
			state = 3;
			break;
		case 3: // Update for each char received
		
		   mvaddch(row+1,col, c);
//mvprintw(row+1, 33, "count: %2d",col+1); // debugging
			col += 1;
			if (col >= 31)
			{ // Exceeded something
			   mvaddstr(row+1,0,linex); // Clear old line
//		    	refresh();
				col = 0;
			}
			break;
		}
	
//mvprintw(6, 0, "Count %d",count);
    	refresh();

	}

    /*  Clean up after ourselves  */

    delwin(mainwin);
    endwin();
    refresh();

    return EXIT_SUCCESS;
}



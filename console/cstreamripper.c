/* cstreamripper.c
 * Curses front end for streamripper
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "srconfig.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sr_cdk.h"

/* GCS FIX: need to set HAVE_XCURSES as needed */
#ifdef HAVE_XCURSES
char *XCursesProgramName = "scroll_ex";
#endif

//#include "srtypes.h"
//#include "rip_manager.h"
//#include "mchar.h"
//#include "filelib.h"
//#include "debug.h"

int
main (int argc, char* argv[])
{
    /* Declare variables. */
    CDKSCREEN *cdkscreen = 0;
    CDKSCROLL *scrollList = 0;
    WINDOW *cursesWin = 0;
    char *title = "<C></5>Pick a file";
    char **list_items;
    int count;
    char *mesg[5], temp[256];
    int selection;

    CDK_PARAMS params;

    CDKparseParams (argc, argv, &params, "cs:t:" CDK_CLI_PARAMS);

    /* GCS FIX: Only set if not defined by env */
    /* GCS FIX: This constant is only defined for ncurses */
#if defined (NCURSES_VERSION)
    ESCDELAY = 50;
#endif

    /* Set up CDK. */
    cursesWin = initscr ();

    /* GCS FIX: Not all curses implementations support this */
    use_default_colors();
    cdkscreen = initCDKScreen (cursesWin);

    /* Set up CDK Colors. */
    initCDKColor ();

    /* Set up list */
    list_items = (char**) malloc (sizeof(char*)*20);
    list_items[0] = "Hello world";
    list_items[1] = "Goodbye world";
    count = 2;

    /* Use the current diretory list to fill the radio list. */
    //    count = CDKgetDirectoryContents (".", &item);

    /* Create the scrolling list. */
    scrollList = newCDKScroll 
	    (cdkscreen,
	     CENTER,      /* X posn */
	     3,           /* Y posn */
	     RIGHT,	  /* Scroll location */
	     -3,          /* Height */
	     0,           /* Width */
	     "",          /* Title */
	     list_items,  /* List of items */
	     count,       /* Number of items in list */
	     FALSE,       /* Leading number? */
	     A_REVERSE,   /* Display attributes */
	     TRUE,        /* Box around list? */
	     FALSE);      /* Shadow? */

    /* Is the scrolling list null? */
    if (scrollList == 0) {
	/* Exit CDK. */
	destroyCDKScreen (cdkscreen);
	endCDK ();

	/* Print out a message and exit. */
	printf	("Oops. Could not make scrolling list. Is the window too small?\n");
	exit (EXIT_FAILURE);
    }

    {
	int pos = 1;
	nodelay (cursesWin,TRUE);
	drawCDKScroll (scrollList, pos);
	//	setCDKScrollPosition (scrollList, 10);
	//	drawCDKScroll (scrollList, 1);
	refreshCDKScreen (cdkscreen);
	while (1) {
	    int p = getch();
	    if (p == ERR) {
		/* GCS FIX usleep() only defined for unix */
		//usleep (100);
	    } else if (p == 'q') {
		break;
	    } else if (p == 'j') {
		if (++pos >= count) pos = count-1;
		setCDKScrollPosition (scrollList, pos);
		drawCDKScroll (scrollList, 1);
		/* GCS FIX usleep() only defined for unix */
		//usleep (10);
	    } else if (p == 'k') {
		if (--pos < 0) pos = 0;
		setCDKScrollPosition (scrollList, pos);
		drawCDKScroll (scrollList, 1);
		/* GCS FIX usleep() only defined for unix */
		//usleep (10);
	    }
	}
    }

    /* Clean up. */
    //    CDKfreeStrings (item);
    destroyCDKScroll (scrollList);
    destroyCDKScreen (cdkscreen);
    endCDK ();
    exit (EXIT_SUCCESS);
}

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <string.h>
#include <mem.h>
#include <math.h>
#include "function.h"

/**************************************************************************
 *  Main                                                                  *
 **************************************************************************/
main()
{
	FILE *fp;
	BITMAP stlbmp;      // default palette
	BITMAP wallpaper;   // wallpaper
	BITMAP mp_normal;   // normal mouse pointer
	BITMAP mp_loading;  // loading mouse pointer
	WINDOW loginWnd;	  // login window structure
	BUTTON loginBtn;	  // login button sturcture
	BUTTON shutdownBtn; // shutdown button structure

	int mx, my, mbutton, system_state = 1, system_state2 = 1;
	char *sfilenm = "styles\\";

	loginWnd.x 		 	 = (SCREEN_WIDTH / 2 - 120);
	loginWnd.y 		 	 = (SCREEN_HEIGHT / 2 - 20);
	loginWnd.width  	 = 230;
	loginWnd.height 	 = 100;
	loginWnd.name 	 	 = "Secure-GUI Login";

	loginBtn.text      = "Login";
	loginBtn.x    	    = loginWnd.x + 10;
	loginBtn.y	  	    = loginWnd.y + 70;
	loginBtn.action    = 1;

	shutdownBtn.text   = "Shutdown";
	shutdownBtn.x    	 = loginWnd.x + 150;
	shutdownBtn.y	  	 = loginWnd.y + 70;
	shutdownBtn.action = 1;

	/* check mouse */
	if (initmouse() == 0)
	{
		closegraph();
		restorecrtmode();
		printf ("\nMouse driver not loaded");
		exit(1);
	}

	/* open setting file and read settings */
	if((fp = fopen("cfg\\settings.cfg", "r+b")) == NULL)
	{
		fprintf(stderr, "\nError opening settings file.\nUsing default settings.\n\n-Press any key to continue.-");
		getch();
	}
	else
	{
		fread(&style, sizeof(style), 1, fp);
		fclose(fp);
	}

	set_mode(VGA_256_COLOR_MODE);       /* set the video mode. */
	init_virscr();

	/* PCalculate plasma animation */
	Pre_Calc();

	/* load style palette */
	strcat(sfilenm, style.paletteFile);
	Do_Plasma(); 									// Continues plasma animation on screen
	load_bmp(sfilenm,&stlbmp);
	Do_Plasma(); 									// Continues plasma animation on screen
	set_palette(stlbmp.palette);
	Do_Plasma(); 									// Continues plasma animation on screen
	free(stlbmp.data);
	Do_Plasma(); 									// Continues plasma animation on screen

	/* load wallpaper and mouse pointers */
	if(style.useWallpaper == 1)
		load_bmp(style.wallpaper, &wallpaper);
	Do_Plasma(); 									// Continues plasma animation on screen
	load_bmp(style.mpNormal, &mp_normal);
	Do_Plasma(); 									// Continues plasma animation on screen
	load_bmp(style.mpLoading, &mp_loading);

	/* set mouse settings */
	setmouse(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
	Do_Plasma(); // Continues plasma animation on screen
	restrictmouseptr (1, 1, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);
	Do_Plasma(); // Continues plasma animation on screen
	system_state = 0;

	while (1)                       /* start main loop */
	{
		wait_for_retrace();
		clearscreen_db();
		if(style.useWallpaper == 1)
			draw_bitmap(&wallpaper, 0, 0); //draw wallpaper
		else
			Do_Plasma2();
		/**************
		*  code here  *
		**************/
		draw_window(loginWnd.x, loginWnd.y, loginWnd.width, loginWnd.height, loginWnd.name);
		loginWnd.x 	  = WindMx;
		loginWnd.y 	  = WindMy;
		loginBtn.x    = loginWnd.x + 10;
		loginBtn.y	  = loginWnd.y + 70;
		shutdownBtn.x = loginWnd.x + 150;
		shutdownBtn.y = loginWnd.y + 70;

		if((draw_button(loginBtn.x, loginBtn.y, loginBtn.text)) == loginBtn.action)
			system_state^=system_state2^=system_state^=system_state2;
		if((draw_button(shutdownBtn.x, shutdownBtn.y, shutdownBtn.text)) == shutdownBtn.action)
			break;
		/**************
		*  code here  *
		**************/

		//drawing mouse pointer
		getmousepos(&mbutton, &mx, &my);
		if(system_state == 0)
		{
			if(((mx + 16) > SCREEN_WIDTH) || ((my + 20) > SCREEN_HEIGHT))
				plot_pixel(mx, my, 255);
			else
				draw_transparent_bitmap(&mp_normal, mx, my);
		}
		if(system_state == 1)
		{
			if(((mx + 16) > SCREEN_WIDTH) || ((my + 20) > SCREEN_HEIGHT))
				plot_pixel(mx, my, 255);
			else
				draw_transparent_bitmap(&mp_loading, mx, my);
		}
		drawscreen_db();
	}											  /* end while loop */

	free(wallpaper.data);

	if(style.saveSettings != 0)
	{
		if((fp = fopen("cfg\\settings.cfg", "w+b")) == NULL)
		{
			fprintf(stderr, "Error writing setting file");
			getch();
		}
		else
		{
			fwrite(&style, sizeof(style), 1, fp);
			fclose(fp);
		}
	}
	fcloseall();
	set_mode(TEXT_MODE);                /* set the video mode back to
													  text mode. */

	return 0;
}

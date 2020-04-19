#define VIDEO_INT           0x10      /* the BIOS video interrupt. */
#define SET_MODE            0x00      /* BIOS func to set the video mode. */
#define VGA_256_COLOR_MODE  0x13      /* use to set 256-color mode. */
#define TEXT_MODE           0x03      /* use to set 80x25 text mode. */

#define PALETTE_INDEX       0x03c8
#define PALETTE_DATA        0x03c9
#define INPUT_STATUS        0x03da
#define VRETRACE            0x08

#define SCREEN_WIDTH        320       /* width in pixels of mode 0x13 320 */
#define SCREEN_HEIGHT       200       /* height in pixels of mode 0x13 200 */
#define NUM_COLORS          256       /* number of colors in mode 0x13 */

#define sgn(x) ((x<0)?-1:((x>0)?1:0)) /* macro to return the sign of a
													  number (draw_line function uses) */
#define PI 3.141592654

int cosinus[256];
unsigned char p1,p2,p3,p4,t1,t2,t3,t4;
int plasmax, plasmay, plasmacol;

typedef unsigned char  byte;
typedef unsigned short word;
typedef long           fixed16_16;

fixed16_16 SIN_ACOS[1024];
byte *VGA=(byte *)0xA0000000L;        			// this points to video memory.
unsigned char *dbuffer = NULL; 		  			// Will point to the double buffer
int mxlast = 0, mylast = 0, mbuttonlast = 0; // last mouse check variables
int dragging = 0; 									// for drag handling
int WindMx; /* next two are for tracking movement of window */
int WindMy;

union REGS i, o;

typedef struct                         /* the structure for a bitmap. */
{
  word width;
  word height;
  byte palette[256*3];
  byte *data;
} BITMAP;

struct WStyle {
	char *paletteFile;  // default 256 color palette file name
	int windowBorder;   // widow border color
	int windowFill;     // window fill color
	int windowTBheight; // window title bar height
	int windowTBbord;   // window title bar border color
	int windowTBFlC;    // window title bar Fill color
	int buttonBorder;   // button border color
	int buttonFill;     // button fill color
	int mAction;   	  // default mouse action button 0 = no button pressed, 1 = left, 2 = right, 3 = both
	int mDrag;			  // default mouse drag button
	int useWallpaper;   // 0 = use plasma efect on wallpaper, 1 = use image on wallpaper
	char *wallpaper;    // specifies wallpaper filename
	char *mpNormal;     // normal mouse pointer transparent image (black is transparent)
	char *mpLoading;    // busy mouse pointer transparent image (black is transparent)
	int saveSettings;   // save settings on exit (only for debugging)
} style = {
	/* default palette file name */ 		"default\\basic.bmp",
	/* widow border color */ 				6,
	/* window fill color */ 				8,
	/* window title bar height */			10,
	/* window title bar border color */	6,
	/* window title bar Fill color */	10,
	/* button border color */				10,
	/* button fill color */					11,
	/* default mouse action */				1,
	/* default mouse drag */				3,
	/* use wallpaper */						1,
	/* wallpaper filename */				"styles\\default\\default.bmp",
	/* normal mouse pointer */				"styles\\default\\defmpn.bmp",
	/* busy mouse pointer */				"styles\\default\\defmpl.bmp",
	/* save settings on exit */			0
	};

typedef struct {
	char *name;
	int x;
	int y;
	int width;
	int height;
} WINDOW;

typedef struct {
	char *text;
	int x;
	int y;
	int action;
} BUTTON;

typedef struct {
	char *text;
	int x;
	int y;
} TEXT;

typedef struct {
	int width;
	int height;
	int data[1][1][1];
	int x;
	int y;
} PAINT;


int init_virscr() // Allocate memory for the double buffer
{
	dbuffer = (unsigned char *)malloc(320 * 200); // 320 columns, 200 rows in mode 13h
	if (dbuffer == NULL) return(0);
		return(1);
}

// Clear the double buffer
void clearscreen_db()
{
	memset(dbuffer, 0, 320 * 200);
}

// Copy the double buffer into the video memory
void drawscreen_db()
{
	memcpy(VGA, dbuffer, 320 * 200);
}

// Free the memory allocated for the double buffer
void free_virscr()
{
	free(dbuffer);
}

/**************************************************************************
 *  fskip                                                                 *
 *     Skips bytes in a file.                                             *
 **************************************************************************/
void fskip(FILE *fp, int num_bytes)
{
	int i;
	for (i=0; i<num_bytes; i++)
		fgetc(fp);
}

/* Set the video mode. */
void set_mode(byte mode)
{
  union REGS regs;

  regs.h.ah = SET_MODE;
  regs.h.al = mode;
  int86(VIDEO_INT, &regs, &regs);
}

/* Drawing a pixel on a 320X200X256 the buffer. */
void plot_pixel(int x, int y, unsigned char col)
{
	if((x >= 0) && (x <= SCREEN_WIDTH-1) && (y >= 0) && (y <= SCREEN_HEIGHT-1))
		dbuffer[(y << 6) + (y << 8) + x] = col;
}

/**************************************************************************
 *  load_bmp                                                              *
 *    Loads a bitmap file into memory.                                    *
 **************************************************************************/
void load_bmp(char *file,BITMAP *b)
{
	FILE *fp;
	long index;
	word num_colors;
	int x;

	/* open the file */
	if((fp = fopen(file,"rb")) == NULL)
	{
		printf("Error opening file %s.\nPress any key to halt.",file);
		getch();
		exit(1);
	}

	/* check to see if it is a valid bitmap file */
	if(fgetc(fp)!='B' || fgetc(fp)!='M')
	{
		fclose(fp);
		printf("%s is not a bitmap file.\nPress any key to halt.",file);
		getch();
		exit(1);
	}

	/* read in the width and height of the image, and the
		number of colors used; ignore the rest */
	fskip(fp,16);
	fread(&b->width, sizeof(word), 1, fp);
	fskip(fp,2);
	fread(&b->height,sizeof(word), 1, fp);
	fskip(fp,22);
	fread(&num_colors,sizeof(word), 1, fp);
	fskip(fp,6);

	/* assume we are working with an 8-bit file */
	if (num_colors==0) num_colors=256;

	/* try to allocate memory */
	if ((b->data = (byte *) malloc((word)(b->width*b->height))) == NULL)
	{
		fclose(fp);
		printf("Error allocating memory for file %s.\nPress any key to halt.",file);
		getch();
		exit(1);
	}

	/* read the palette information */
	for(index=0;index<num_colors;index++)
	{
		 b->palette[(int)(index*3+2)] = fgetc(fp) >> 2;
		 b->palette[(int)(index*3+1)] = fgetc(fp) >> 2;
		 b->palette[(int)(index*3+0)] = fgetc(fp) >> 2;
		 x=fgetc(fp);
	}

	/* read the bitmap */
	for(index=(b->height-1)*b->width;index>=0;index-=b->width)
		for(x=0;x<b->width;x++)
			b->data[(word)(index+x)]=(byte)fgetc(fp);

	fclose(fp);
}

/**************************************************************************
 *  set_palette                                                           *
 *    Sets all 256 colors of the palette.                                 *
 **************************************************************************/
void set_palette(byte *palette)
{
	int i;

	outp(PALETTE_INDEX,0);              /* tell the VGA that palette data
													  is coming. */
	for(i=0;i<256*3;i++)
		outp(PALETTE_DATA,palette[i]);    /* write the data */
}

/* Draw a bitmap. */
void draw_bitmap(BITMAP *bmp,int x,int y)
{
  int j;
  word screen_offset = (y<<8)+(y<<6)+x;
  word bitmap_offset = 0;

  for(j=0;j<bmp->height;j++)
  {
	 memcpy(&dbuffer[screen_offset],&bmp->data[bitmap_offset],bmp->width);

	 bitmap_offset+=bmp->width;
	 screen_offset+=SCREEN_WIDTH;
  }
}

/**************************************************************************
 *  draw_transparent_bitmap                                               *
 *    Draws a transparent bitmap.                                         *
 **************************************************************************/
void draw_transparent_bitmap(BITMAP *bmp,int x,int y)
{
  int i,j;
  word screen_offset = (y<<8)+(y<<6);
  word bitmap_offset = 0;
  byte data;

  for(j = 0;j < bmp->height; j++)
  {
	 for(i = 0; i < bmp->width; i++, bitmap_offset++)
	 {
		data = bmp->data[bitmap_offset];
		if((bmp->width >= (SCREEN_WIDTH + x)) || ((bmp->height + y) >= SCREEN_HEIGHT))
		{} else
			if (data) dbuffer[screen_offset+x+i] = data;
	 }
	 screen_offset+=SCREEN_WIDTH;
  }
}

/**************************************************************************
 *  wait_for_retrace                                                      *
 *    Wait until the *beginning* of a vertical retrace cycle (60hz).      *
 **************************************************************************/
void wait_for_retrace(void)
{
	 /* wait until done with vertical retrace */
	 while  ((inp(INPUT_STATUS) & VRETRACE));
	 /* wait until done refreshing */
	 while (!(inp(INPUT_STATUS) & VRETRACE));
}

/* Initialize mouse */
initmouse()
{
i.x.ax = 0;
int86 ( 0x33, &i, &o );
return (o.x.ax);
}

/* gets mouse coordinates and button status */
getmousepos(int *button, int *x, int *y)
{
	i.x.ax = 3;
	int86 (0x33, &i, &o);
	*button = o.x.bx;
	*x = o.x.cx;
	*y = o.x.dx;
}

/* Set mouse position */
setmouse(int x, int y)
{
	_asm {
	mov ax, 4
	mov cx, x
	mov dx, y
	int 33h
	}
}

/* restricts mouse movement */
restrictmouseptr(int x1, int y1, int x2, int y2)
{
	i.x.ax = 7;
	i.x.cx = x1;
	i.x.dx = x2;
	int86 (0x33, &i, &o);
	i.x.ax = 8;
	i.x.cx = y1;
	i.x.dx = y2;
	int86 (0x33, &i, &o);
}

/* draw line */
void draw_line(int x1, int y1, int x2, int y2, unsigned char color)
{
  int i, dx, dy, sdx, sdy, dxabs, dyabs, x, y, px, py;

  dx = x2 - x1;      /* the horizontal distance of the line */
  dy = y2 - y1;      /* the vertical distance of the line */
  dxabs = abs(dx);
  dyabs = abs(dy);
  sdx = sgn(dx);
  sdy = sgn(dy);
  x = dyabs >> 1;
  y = dxabs >> 1;
  px = x1;
  py = y1;

  if (dxabs >= dyabs) /* the line is more horizontal than vertical */
  {
	 for(i = 0; i < dxabs; i++)
	 {
		y += dyabs;
		if (y >= dxabs)
		{
		  y -= dxabs;
		  py += sdy;
		}
		px += sdx;
		plot_pixel(px, py, color);
	 }
  }
  else /* the line is more vertical than horizontal */
  {
	 for(i = 0; i < dyabs; i++)
	 {
		x += dxabs;
		if (x >= dyabs)
		{
			x -= dyabs;
			px += sdx;
		}
		py += sdy;
		plot_pixel(px, py, color);
	 }
  }
}

/* draw box */
draw_box(int x, int y, int ex, int ey, unsigned char border, unsigned char fillcol, int filled)
{
	int n, nb;
	word y_offset,ey_offset,i,temp;
	word top_offset,bottom_offset,width;
	int left, top, right, bottom;
	if (y>ey)
	{
		temp=y;
		y=ey;
		ey=temp;
	}
	if (x>ex)
	{
		temp=x;
		x=ex;
		ex=temp;
	}

	y_offset=(y<<8)+(y<<6);
	ey_offset=(ey<<8)+(ey<<6);

	for(i = x; i <= ex; i++)
	{
		if(i <= SCREEN_WIDTH)
		{
			dbuffer[y_offset+i]=border;
			dbuffer[ey_offset+i]=border;
		}
	}
	for(i = y_offset; i <= ey_offset; i += SCREEN_WIDTH)
	{
		if((i <= SCREEN_HEIGHT) || (x <= SCREEN_WIDTH))
		{
			dbuffer[x+i]=border;
			dbuffer[ex+i]=border;
		}
	}

	left   = x  + 1;
	top    = y  + 1;
	right  = ex - 1;
	bottom = ey - 1;

	/* fill box */
	if(filled == 1)
		{
		if(top>bottom)
		{
			temp=top;
			top=bottom;
			bottom=temp;
		}
		if(left>right)
		{
			temp=left;
			left=right;
			right=temp;
		}

		top_offset=(top<<8)+(top<<6)+left;
		bottom_offset=(bottom<<8)+(bottom<<6)+left;
		width=right-left+1;

		for(i=top_offset;i<=bottom_offset;i+=SCREEN_WIDTH)
			memset(&dbuffer[i],fillcol,width);
	}
}

int draw_window(int x, int y, int width, int height, char *name)
{
	int mbutton, mx, my;
	WindMx = 0;
	WindMy = 0;

	getmousepos(&mbutton, &mx, &my);

	if((mx != mxlast) && (my != mylast) && (mbutton == style.mDrag))
	{
		if((mx >= x) && (mx <= (width + x)) && (my >= y) && (my <= (height + y)) && (mbutton == style.mDrag) || (dragging == 1))
		{
			x = mx;
			y = my;
			draw_box(x, y, width + x, height + y, style.windowBorder, style.windowFill, 1);
			draw_box(x, y, width + x, y + style.windowTBheight, style.windowTBbord, style.windowTBFlC, 1);
			dragging = 1;
		}
	}
	else
		dragging = 0;
	if(dragging == 0)
	{
		draw_box(x, y, width + x, height + y, style.windowBorder, style.windowFill, 1);
		draw_box(x, y, width + x, y + style.windowTBheight, style.windowTBbord, style.windowTBFlC, 1);
	}
	WindMx = x;
	WindMy = y;
}

/* draw_button */
draw_button(int x, int y, char *text)
{
	size_t txt_lenght;
	int lenght;
	int height;
	int mbutton, mx, my;

	txt_lenght = strlen(text);

	lenght = 8 * txt_lenght + 4 + x;
	height = 16 + 6 + y;

	getmousepos(&mbutton, &mx, &my);

	if((mx >= x) && (mx <= lenght) && (my >= y) && (my <= height) && (mbutton != mbuttonlast))
	{
		draw_box(x+1, y+1, lenght+1, height+1, style.buttonBorder+1, style.buttonFill, 1);
		if(((mx != mxlast) && (my != mylast) &&  mbutton != mbuttonlast))
		{
			mxlast = mx; mylast = my; mbuttonlast = mbutton;
			return mbutton;
		}
		mxlast = mx; mylast = my; mbuttonlast = mbutton;
	}
	else if((mx >= x) && (mx <= lenght) && (my >= y) && (my <= height) && (mbutton != 0))
		draw_box(x+1, y+1, lenght+1, height+1, style.buttonBorder+1, style.buttonFill, 1);
	else
	{
		draw_box(x+1, y+1, lenght+1, height+1, style.buttonBorder, style.buttonFill, 1);
		draw_box(x, y, lenght, height, style.buttonBorder+1, style.buttonFill, 1);
		return 0;
	}
}
//---------------------------------
// Plasma loading screen functions
//---------------------------------
void setpal(int which[768])
{
	int i;
	outp(0x3c8,8);
	for (i=0;i<768;i+=3)
	{
		outp(0x3c9,which[i]);
		outp(0x3c9,which[i+1]);
		outp(0x3c9,which[i+2]);
	}
}

Do_Plasma()
{
	wait_for_retrace();
	clearscreen_db();
	t1=p1;
	t2=p2;
	for (plasmay = 0; plasmay < 200; plasmay++)
	{
		t3=p3;
		t4=p4;
		for (plasmax = 0; plasmax < 320; plasmax++)
		{
			plasmacol=cosinus[t1]+cosinus[t2]+cosinus[t3]+cosinus[t4];
			plot_pixel(plasmax,plasmay,plasmacol);
			t3+=1;
			t4+=3;
		}
		t1+=2;
		t2+=1;
	}
	p1+=1;
	p2-=2;
	p3+=3;
	p4-=4;
	drawscreen_db();
}

Do_Plasma2()
{
	t1=p1;
	t2=p2;
	for (plasmay = 0; plasmay < 200; plasmay++)
	{
		t3=p3;
		t4=p4;
		for (plasmax = 0; plasmax < 320; plasmax++)
		{
			plasmacol = 64 - (cosinus[t1] + cosinus[t2] + cosinus[t3] + cosinus[t4]);
			plot_pixel(plasmax, plasmay, plasmacol);
			t3+=1;
			t4+=3;
		}
		t1+=2;
		t2+=1;
	}
	p1+=1;
	p2-=2;
	p3+=3;
	p4-=4;
}

Pre_Calc()
{
	int i;
	for (i=0;i<256;i++)
	cosinus[i]=30*(cos(i*PI/64));
}
#define PI 3.141592654

int cosinus[256];
unsigned char p1,p2,p3,p4,t1,t2,t3,t4;
int x,y,col;
int pal[768];

//---------------------------------
// Plasmas Stuff
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

void Do_Plasma(void)
{
	//WaitRetRP();
	t1=p1;
	t2=p2;
	for (y=0;y<200;y++)
	{
		t3=p3;
		t4=p4;
		for (x=0;x<320;x++)
		{
			col=cosinus[t1]+cosinus[t2]+cosinus[t3]+cosinus[t4];
			putpixel(x,y,col);
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

void Prep_Pal(void)
{
	int i;
	for (x=0,y=0;x<63*3;x+=3,y++) pal[x]=y;
	for (x=63*3,y=63;x<127*3;x+=3,y--) pal[x]=y;
	for (x=127*3,y=0;x<191*3;x+=3,y++) pal[x+1]=y;
	for (x=191*3,y=191;x<255*3;x+=3,y--) pal[x+1]=y;
}

void Pre_Calc(void)
{
	int i;
	for (i=0;i<256;i++)
	cosinus[i]=30*(cos(i*PI/64));
}
#include <unistd.h>
#include "common.h"
#include "minitouch.h"
#include "inputassist.h"

#define MAX_POINTS 20
struct MT_POINT {
	bool pressed;
	int point_x;
	int point_y;
} mt_points[MAX_POINTS];

// q,w,e,r,t,y,u,i,o,p,a,s,d,f,g,h,j,k,l,z,x,c,v,b,n,m
int qwerty[] = {30,48,46,32,18,33,34,35,23,36,37,38,50,49,24,25,16,19,31,20,22,47,17,45,21,44};
//  ,!,",#,$,%,&,',(,),*,+,,,-,.,/
int spec1[] = {57,2,40,4,5,6,8,40,10,11,9,13,51,12,52,52};
int spec1sh[] = {0,1,1,1,1,1,1,0,1,1,1,1,0,0,0,1};
// :,;,<,=,>,?,@
int spec2[] = {39,39,227,13,228,53,215};
int spec2sh[] = {1,0,1,1,1,1,0};
// [,\,],^,_,`
int spec3[] = {26,43,27,7,12,399};
int spec3sh[] = {0,0,0,1,1,0};
// {,|,},~
int spec4[] = {26,43,27,215,14};
int spec4sh[] = {1,1,1,1,0};

int keycode(int c, bool &sh, bool &alt, bool real)
{
	if ('a' <= c && c <= 'z')
		return qwerty[c-'a'];
	if ('A' <= c && c <= 'Z')
	{
		sh = true;
		return qwerty[c-'A'];
	}
	if ('1' <= c && c <= '9')
		return c-'1'+2;
	if (c == '0')
		return 11;
	if (32 <= c && c <= 47)
	{
		sh = spec1sh[c-32];
		return spec1[c-32];
	}
	if (58 <= c && c <= 64)
	{
		sh = spec2sh[c-58];
		return spec2[c-58];
	}
	if (91 <= c && c <= 96)
	{
		sh = spec3sh[c-91];
		return spec3[c-91];
	}
	if (123 <= c && c <= 127)
	{
		sh = spec4sh[c-123];
		return spec4[c-123];
	}
	switch(c)
	{
		case 263: return 14;// backspace
		case 9: return 15;// tab
		case 1: alt=1; return 34;// ctrl+a
		case 3: alt=1; return 46;// ctrl+c
		case 4: alt=1; return 32;// ctrl+d
		case 18: alt=1; return 31;// ctrl+r
		case 10: return 28;// enter
		case 27: return 158;// esc -> back
		case 260: return 105;// left -> DPAD_LEFT
		case 261: return 106;// right -> DPAD_RIGHT
		case 258: return 108;// down -> DPAD_DOWN
		case 259: return 103;// up -> DPAD_UP
		case 360: return 232;// end -> DPAD_CENTER (ball click)
		case 262: return 102;// home
		case 330: return 158;// del -> back
		case 265: return 229;// F1 -> menu
		case 266: return 127;// F2 -> search
		case 267: return 61;// F3 -> call
		case 268: return 107;// F4 -> endcall
		case 338: return 114;// PgDn -> VolDn
		case 339: return 115;// PgUp -> VolUp
		case 275: return 232;// F11 -> DPAD_CENTER (ball click)
		case 269: return 211;// F5 -> focus
		case 270: return 212;// F6 -> camera
		case 271: return 150;// F7 -> explorer
		case 272: return 155;// F8 -> envelope

		case 50081:
		case 225: alt = 1;
			if (real) return 48; //a with acute
			return 30; //a with acute -> a with ring above
		case 50049:
		case 193:sh = 1; alt = 1;
			if (real) return 48; //A with acute
			return 30; //A with acute -> a with ring above
		case 50089:
		case 233: alt = 1; return 18; //e with acute
		case 50057:
		case 201:sh = 1; alt = 1; return 18; //E with acute
		case 50093:
		case 237: alt = 1;
			if (real) return 36; //i with acute
			return 23; //i with acute -> i with grave
		case 50061:
		case 205: sh = 1; alt = 1;
			if (real) return 36; //I with acute
			return 23; //I with acute -> i with grave
		case 50099:
		case 243:alt = 1;
			if (real) return 16; //o with acute
			return 24; //o with acute -> o with grave
		case 50067:
		case 211:sh = 1; alt = 1;
			if (real) return 16; //O with acute
			return 24; //O with acute -> o with grave
		case 50102:
		case 246: alt = 1; return 25; //o with diaeresis
		case 50070:
		case 214: sh = 1; alt = 1; return 25; //O with diaeresis
		case 50577:
		case 245:alt = 1;
			if (real) return 19; //Hungarian o
			return 25; //Hungarian o -> o with diaeresis
		case 50576:
		case 213: sh = 1; alt = 1;
			if (real) return 19; //Hungarian O
			return 25; //Hungarian O -> O with diaeresis
		case 50106:
		case 250: alt = 1;
			if (real) return 17; //u with acute
			return 22; //u with acute -> u with grave
		case 50074:
		case 218: sh = 1; alt = 1;
			if (real) return 17; //U with acute
			return 22; //U with acute -> u with grave
		case 50108:
		case 252: alt = 1; return 47; //u with diaeresis
		case 50076:
		case 220:sh = 1; alt = 1; return 47; //U with diaeresis
		case 50609:
		case 251: alt = 1;
			if (real) return 45; //Hungarian u
			return 47; //Hungarian u -> u with diaeresis
		case 50608:
		case 219: sh = 1; alt = 1;
			if (real) return 45; //Hungarian U
			return 47; //Hungarian U -> U with diaeresis

	}
	return 0;
}

int inputfd = -1;

InputAssist::InputAssist()
{

}

InputAssist::~InputAssist()
{

}

int InputAssist::KeyDown(int iVirtKey)
{
	char cmd[100];
	sprintf(cmd, "sendevent /dev/input/event0 1 %d 1", iVirtKey);
	system(cmd);
	return ERR_NONE;
}

int InputAssist::KeyUp(int iVirtKey)
{
	char cmd[100];
	sprintf(cmd, "sendevent /dev/input/event0 1 %d 1", iVirtKey);
	system(cmd);
	return ERR_NONE;
}

int InputAssist::KeyPress(int iVirtKey, uint32_t duration)
{
	char cmd[100];
	sprintf(cmd, "input keyevent %d", iVirtKey);
	system(cmd);
	return ERR_NONE;
}

inline void InputAssist::transform_touch_xy(int *x, int *y)
{
	int old_x = *x, old_y = *y;
	int max_x, max_y;
	minitouch_get_max_xy(&max_x, &max_y);
	int width, height, rotation;
	m_deviceInfo->GetDisplayInfo(width, height, rotation);

	if (rotation == 0)
	{
		*x = old_x * max_x / width;
		*y = old_y * max_y / height;
	}
	else if (rotation == 90)
	{
		*x = (height - old_y) * max_x / height;
		*y = old_x * max_y / width;
	}
	else if (rotation == 180)
	{
		*x = (width - old_x) * max_x / width;
		*y = (height - old_y) * max_y / height;
	}
	else if (rotation == 270)
	{
		*x = old_y * max_x / height;
		*y = (width - old_x) * max_y / width;
	}
}

int InputAssist::MouseMove(int iX, int iY, int iFlags)
{
	return ERR_NONE;
}

int InputAssist::MouseDown(int iVirtKey)
{
	return ERR_NONE;
}

int InputAssist::MouseUp(int iVirtKey)
{
	return ERR_NONE;
}

int InputAssist::MouseClick(int iVirtKey, uint32_t duration)
{
	return ERR_NONE;
}

int InputAssist::TouchMove(int iPointId, int iX, int iY, int iFlags)
{
	transform_touch_xy(&iX, &iY);
	touch_move(iPointId, iX, iY, 50);
	minitouch_commit();
	return ERR_NONE;
}

int InputAssist::TouchDown(int iPointId, int iX, int iY)
{
	transform_touch_xy(&iX, &iY);
	touch_down(iPointId, iX, iY, 50);
	minitouch_commit();
	return ERR_NONE;
}

int InputAssist::TouchUp(int iPointId)
{
	touch_up(iPointId);
	minitouch_commit();
	return ERR_NONE;
}

int InputAssist::TouchTap(int iPointId, int iX, int iY)
{
	transform_touch_xy(&iX, &iY);
	touch_down(iPointId, iX, iY, 50);
	minitouch_commit();
	usleep(50 * 1000);
	touch_up(iPointId);
	minitouch_commit();
	return ERR_NONE;
}

int InputAssist::TouchSwipe(int iPointId, int iX1, int iY1, int iX2, int iY2, uint32_t duration)
{
	transform_touch_xy(&iX1, &iY1);
	transform_touch_xy(&iX2, &iY2);
	touch_down(iPointId, iX1, iY1, 50);
	minitouch_commit();

	int n = duration / 50;
	if (n > 0)
	{
		usleep(50 * 1000);
		int unitx = (iX2 - iX1) / n;
		int unity = (iY2 - iY1) / n;
		for (int i = 1; i < n; i++)
		{
			touch_move(iPointId, iX1 + unitx * i, iY1 + unity * i, 50);
			minitouch_commit();
			usleep(50 * 1000);
		}

		touch_move(iPointId, iX2, iY2, 50);
		minitouch_commit();
	}

	touch_up(iPointId);
	minitouch_commit();
	return ERR_NONE;
}

int InputAssist::SendString(uint32_t iSendMode, const char *pContent)
{
	std::string cmd = "input text ";
	cmd += pContent;
	system(cmd.c_str());
	return ERR_NONE;
}


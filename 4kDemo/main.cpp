#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>

#ifdef _DEBUG
#include <iostream>
#endif

#include "map.hpp"

const static PIXELFORMATDESCRIPTOR pfd =
{
	0, 0, PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//screen definitions
#define WINDOWSIZEX 1366.f
#define WINDOWSIZEY 768.f

//input layout
#define NEO
//#define QWERT
#ifdef NEO
#define KEY_W 0x56
#define KEY_A 0x55
#define KEY_S 0x49
#define KEY_D 0x41
#endif
#ifdef QWERT
#define KEY_W 0x57
#define KEY_A 0x41
#define KEY_S 0x53
#define KEY_D 0x44
#endif

//the map
Map map;

Vec2 g_playerLoc;
Vec2 g_playerRot; // two axis are enough
Vec2 g_playerFront; // effectively cos and sin of the players plane rotation
Vec2 g_playerFrontUp; // cos and sin of the players up rotation
Vec2 g_playerViewDir;
float g_currentEyeHeight = EYEHEIGHT;
float g_playerHealth = MAXHEALTH;
float g_playerHealthPrev;
int COUNTDOWN = 512;
bool bIsDead = false;
bool bIsFiring;
POINT g_oldCursor;

inline void input()
{
	Vec2 dir(0.f,0.f);
	if (GetAsyncKeyState(KEY_W))//W
		dir.x += 0.1f;
	if (GetAsyncKeyState(KEY_S))//A
		dir.x -= 0.1f;
	if (GetAsyncKeyState(KEY_A))//S
		dir.y -= 0.1f;
	if (GetAsyncKeyState(KEY_D))//D
		dir.y += 0.1f;

	bool firing = GetAsyncKeyState(VK_LBUTTON);
	if (firing != bIsFiring)
	{
		glDisable(GL_LIGHT2);
		bIsFiring = firing;
	}

	// collision
	int oldx = int(g_playerLoc.x); if (oldx % 2) oldx++;
	int oldy = int(g_playerLoc.y); if (oldy % 2) oldy++;
	dir = dir.rotate(g_playerFront);
	int x = int((g_playerLoc.x + dir.x)); if (x % 2) x++;
	int y = int((g_playerLoc.y + dir.y)); if (y % 2) y++;
	Vec2 cubePos((float)x, (float)y);

	//ignore collision when dead
	if (!bIsDead && x >= 0 && y >= 0 && map.data[x/2 + y/2 * MAPLEN])
	{
		if (oldx < x) g_playerLoc.x = cubePos.x - 1.2f;
		if (oldx > x) g_playerLoc.x = cubePos.x + 1.2f;
		if (oldy < y) g_playerLoc.y = cubePos.y - 1.2f;
		if (oldy > y) g_playerLoc.y = cubePos.y + 1.2f;
	}
	else g_playerLoc = g_playerLoc + dir;

	POINT cursor;
	GetCursorPos(&cursor);
	x = cursor.x - g_oldCursor.x;
	y = cursor.y - g_oldCursor.y;
	//make sure that we will not hit a screen border
	SetCursorPos((int)WINDOWSIZEX / 2, (int)WINDOWSIZEY / 2);

	g_playerRot.x += 0.001f * x;
	g_playerRot.y += 0.001f * y;
	//clamp to not get inverse
	if (g_playerRot.y > PI) g_playerRot.y = PI;
	else if (g_playerRot.y < 0.f) g_playerRot.y = 0.f;
	g_playerFront.x = _cos(g_playerRot.x);
	g_playerFront.y = _sin(g_playerRot.x);
	g_playerFrontUp.x = _cos(g_playerRot.y);
	g_playerFrontUp.y = _sin(g_playerRot.y);
}

GLint faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
	{ 0, 1, 3, 2 }, { 2, 3, 7, 6 }, { 5, 4, 6, 7 },
	{ 1, 5, 7, 3 }, { 4, 5, 1, 0 }, { 4, 0, 2, 6 } };
GLfloat v[8][3];  // X,Y,Z vertex offset

GLfloat n[6][3] = {  /* Normals for the 6 faces of a cube. */
	{ 0.0, 0.0, -1.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 0.0, 1.0 },
	{ 1.0, 0.0, 0.0 }, { 0.0, -1.0, 0.0 }, { -1.0, 0.0, 0.0 } };

// light properties
GLfloat light_ambient[] = { 0.05, 0.05, 0.05, 1.0 };
GLfloat light_diffuse[] = { 0.3, 0.3, 0.3, 0.0 };
GLfloat light_specular[] = { 1.0, 1.0, 1.0, 0.2 };
GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };  /* Infinite light location. */

GLfloat light_direction[] = { 0.0, 0.0, -1.0, 1.0 };
GLfloat light_position2[] = { 0.0, EYEHEIGHT - 0.2f, 0.0, 1.0 };
GLfloat light_diffuse2[] = { 1.0, 1.0, 1.0, 0.0 };

struct GLVec
{
	GLfloat data[3];
};

GLVec calcVec(GLfloat* _vec, const Vec2& _off, float _height)
{
	GLVec res;
	res.data[0] = _vec[0] + _off.x;
	res.data[1] = _vec[1] + (_vec[1] != -1.f ? _height : 0.f);
	res.data[2] = _vec[2] + _off.y;

	return res;
}

GLVec avg(GLVec& _0, GLVec& _1)
{
	GLVec res;
	res.data[0] = (_0.data[0] + _1.data[0]) / 2.f;
	res.data[1] = (_0.data[1] + _1.data[1]) / 2.f;
	res.data[2] = (_0.data[2] + _1.data[2]) / 2.f;

	return res;
}

void drawQuad(GLVec& _0, GLVec& _1, GLVec& _2, GLVec& _3, GLfloat* _normal, int _depth)
{
	if (_depth)
	{
		GLVec avg0 = avg(_0, _1);
		GLVec avg1 = avg(_1, _2);
		GLVec avg2 = avg(_2, _3);
		GLVec avg3 = avg(_3, _0);
		GLVec avgM = avg(avg0, avg2);
		--_depth;
		drawQuad(_0, avg0, avgM, avg3, _normal, _depth);
		drawQuad(_1, avg1, avgM, avg0, _normal, _depth);
		drawQuad(_2, avg2, avgM, avg1, _normal, _depth);
		drawQuad(_3, avg3, avgM, avg2, _normal, _depth);
	}
	else
	{
		glBegin(GL_QUADS);
		glColor3f(0.3, 0.3, 0.2);
		glNormal3fv(_normal);
		glVertex3fv(_0.data);
		glVertex3fv(_1.data);
		glVertex3fv(_2.data);
		glVertex3fv(_3.data);

		glEnd();
	}
}

inline void drawRect(const Vec2& _loc, float _height)
{
	for (int i = 0; i < 6; i++) {
	//	GLVec testNorm;
	//	testNorm.data[0] = 0.1f*(_loc.x - g_playerLoc.x) + n[i][0];
	//	testNorm.data[1] = 0.f;
	//	testNorm.data[2] = 0.1f*(_loc.y - g_playerLoc.y) + n[i][2];

		drawQuad(calcVec(&v[faces[i][0]][0], _loc, _height),
			calcVec(&v[faces[i][1]][0], _loc, _height),
			calcVec(&v[faces[i][2]][0], _loc, _height),
			calcVec(&v[faces[i][3]][0], _loc, _height),
			&n[i][0], 2);
/*		glBegin(GL_QUADS);
		glNormal3fv(&n[i][0]);
		glColor3f(0.3, 0.3, 0.2);
		glVertexOff(&v[faces[i][0]][0], _loc, _height);
		glVertexOff(&v[faces[i][1]][0], _loc, _height);
		glVertexOff(&v[faces[i][2]][0], _loc, _height);
		glVertexOff(&v[faces[i][3]][0], _loc, _height);
		glEnd();*/
	}
}

inline void drawPlane(float _height)
{
	glBegin(GL_QUADS);
	glColor3f(0.2, 0.2, 0.2);
	glNormal3f(0.f, 1.f, 0.f);
	glVertex3f(0.f - 300.f, _height, 0.0f - 300.f);
	glVertex3f(0.f + 300.f, _height, 0.0f - 300.f);
	glVertex3f(0.f + 300.f, _height, 0.0f + 300.f);
	glVertex3f(0.f - 300.f, _height, 0.0f + 300.f);
	glEnd();
}

inline void drawEnemy(Vec2& _loc)
{
	GLVec invNorm;
	invNorm.data[0] = _loc.x - g_playerLoc.x;
	invNorm.data[1] = 0.f;
	invNorm.data[2] = _loc.y - g_playerLoc.y;
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(1.0, 1.0, 1.0);
	glNormal3fv(invNorm.data);
	glVertex3f(_loc.x, 0.1f, _loc.y);
	glVertex3f(_loc.x + 0.3f, 1.2f, _loc.y);
	glVertex3f(_loc.x - 0.3f, 1.2f, _loc.y);
	glVertex3f(_loc.x, 0.1f, _loc.y);
	glVertex3f(_loc.x + 0.3f, 1.2f, _loc.y);
	glVertex3f(_loc.x - 0.15f, 1.2f, _loc.y + 0.3f);
	glVertex3f(_loc.x, 0.1f, _loc.y);
	glVertex3f(_loc.x - 0.3f, 1.2f, _loc.y);
	glVertex3f(_loc.x - 0.15f, 1.2f, _loc.y + 0.3f);
	glEnd();
}

void drawWeapon()
{
//	glRectf(-0.5f, -0.5f, 0.5f, 0.5f);
//	glBegin(GL_POLYGON);
//	glNormal3fv(&n[i][0]);
/*	glVertex3f(&v[faces[i][0]][0]);
	glVertex3fv(&v[faces[i][1]][0]);
	glVertex3fv(&v[faces[i][2]][0]);
	glVertex3fv(&v[faces[i][3]][0]);*/
//	glEnd();
}

inline void drawRay()
{
	//bling bling!
	if (rnd() % 8) glEnable(GL_LIGHT2);
	else glDisable(GL_LIGHT2);

	glLineWidth(8.f);
	glBegin(GL_LINE_LOOP);
	glNormal3fv(&n[3][0]);
	float orgX = g_playerLoc.x + g_playerViewDir.x;
	float orgY = g_playerLoc.y + g_playerViewDir.y;
	float orgZ = EYEHEIGHT / 4.8f + g_playerFrontUp.x;

	for (int i = 0; i < 5; ++i)
	{
		glColor3f(0.1, 0.2, 1.0);
		glVertex3f(orgX, orgZ, orgY);
		glColor3f(0.4, 0.7, 0.0);
		glVertex3f(orgX + (0.0625 - (rnd() % 64) / 512.f), orgZ + (0.015625 - (rnd() % 32) / 1024.f), orgY + (0.0625 - (rnd() % 64) / 512.f));
		glColor3f(0.1, 0.2, 1.0);
		glVertex3f(g_playerLoc.x + g_playerViewDir.x * 50.f, EYEHEIGHT + g_playerFrontUp.x * 50.f, g_playerLoc.y + g_playerViewDir.y * 50.f);
	}
	glEnd();
}

inline void draw()
{
	for (int i = 0; i < MAPSIZE; ++i)
	{
		Vec2 pos;
		pos.x = (i % MAPLEN) * 2.f;
		pos.y = (i / MAPLEN) * 2.f;

		if (map.data[i])
			drawRect(pos, map.data[i]);
	}
	for (int i = 0; i < ENEMYCOUNT; ++i)
		if (map.enemys[i].isAlive) drawEnemy(map.enemys[i].loc);

	if (bIsFiring) drawRay();
}

// **************************************************** //

// logic
inline void process()
{
	for (int i = 0; i < ENEMYCOUNT * 2; ++i)
	{
		if (!map.enemys[i].isAlive)
		{
			//has been placed but not spawned
			if (COUNTDOWN <= 0 && map.enemys[i].health == 132.f)
			{
				COUNTDOWN = 256;
				map.enemys[i].isAlive = true;
			}
			continue;
		}
		float d = map.enemys[i].loc.x - g_playerLoc.x;
		float d2 = map.enemys[i].loc.y - g_playerLoc.y; 
		d = d * d + d2*d2;

		if (d < DMGRADIUS)
		{
			g_playerHealth -= 1.f;
		}

		if (d < PERCEPTIONAI)
		{
			int x = (int)(map.enemys[i].loc.x / 2.f);
			int px = (int)(g_playerLoc.x / 2.f);
			int y = (int)(map.enemys[i].loc.y / 2.f);
			int py = (int)(g_playerLoc.y / 2.f);

			Vec2 mov(0.f, 0.f);

			if (x < px && !map.data[index(x + 1, y)]) mov.x = 0.1f;
			else if (x > px && !map.data[index(x - 1, y)]) mov.x = -0.1f;

			if (y < py && !map.data[index(x, y+1)]) mov.y = 0.1f;
			else if (y > py && !map.data[index(x, y - 1)]) mov.y = -0.1f;

			map.enemys[i].loc = map.enemys[i].loc + mov;
		}
	}

	//death event
	if (!bIsDead && g_playerHealth <= 0.f)
	{
		bIsDead = true;
		COUNTDOWN = 512;
	}
}


__declspec(naked) void winmain()
{
	// Prolog
	__asm enter 0x10, 0;
	__asm pushad;

	{ // Extra scope to make compiler 
		//accept the __decalspec(naked) 
		//with local variables

		// Create window and initialize OpenGL

		HWND hwnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);
		HDC hdc = GetDC(hwnd);

		SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd);
		wglMakeCurrent(hdc, wglCreateContext(hdc));

/*		GLuint prog = ((PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram"))();
		GLuint shader = ((PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader"))(GL_FRAGMENT_SHADER);
		((PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource"))(shader, 1, &g_ShaderCode, 0);
		((PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader"))(shader);
		((PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader"))(prog, shader);
		((PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram"))(prog);
		((PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram"))(prog);*/

		ShowCursor(FALSE);

		// init world
		map.generate();
		g_playerLoc.x = 60.f;
		g_playerLoc.y = 34.f;
		g_playerRot.x = 0.f;
		g_playerRot.y = PI / 2.f; //look forward

		g_oldCursor.x = (int)WINDOWSIZEX / 2;
		g_oldCursor.y = (int)WINDOWSIZEY / 2;

		// v0 {-1 -1 1}
		// v1 {-1 -1 }
		/* Setup cube vertex data. */
		v[0][0] = -1; v[0][1] = -1; v[0][2] = -1;
		v[1][0] = 1; v[1][1] = -1; v[1][2] = -1;
		v[2][0] = -1; v[2][1] = 1; v[2][2] = -1;
		v[3][0] = 1; v[3][1] = 1; v[3][2] = -1;

		v[4][0] = -1; v[4][1] = -1; v[4][2] = 1;
		v[5][0] = 1; v[5][1] = -1; v[5][2] = 1;
		v[6][0] = -1; v[6][1] = 1; v[6][2] = 1;
		v[7][0] = 1; v[7][1] = 1; v[7][2] = 1;

		//setup lights
		glEnable(GL_DEPTH_TEST);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	//	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
		glEnable(GL_LIGHT0);

		glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 16.f);
		glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 6.f);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse2);
		glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, light_direction);
		glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
		glEnable(GL_LIGHT1);

	//	glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 16.f);
	//	glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 14.f);
		glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, light_direction);
		glLightfv(GL_LIGHT2, GL_SPECULAR, light_specular);

		glEnable(GL_LIGHTING);
//		glColorMaterial(GL_BACK_RIGHT, GL_AMBIENT_AND_DIFFUSE);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glEnable(GL_COLOR_MATERIAL);

		glFrontFace(GL_CW);
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);

		glMatrixMode(GL_PROJECTION);
		gluPerspective( /* field of view in degree */ 70,
			/* aspect ratio */ WINDOWSIZEX / WINDOWSIZEY,
			/* Z near */ 0.1, /* Z far */ 100.0);

		int tick = 0xFFFFFFFF;
		for (;;)
		{
			COUNTDOWN--;
			input();

			if (!bIsDead)
			{
				process();

				if (bIsFiring)
				{
					Pawn* pwn = map.rayCast(g_playerLoc, g_playerFront, g_playerFrontUp.x);
					if (pwn)
					{
						pwn->health -= 10.f;
						if (pwn->health <= 0.f) pwn->isAlive = false;
					}
				}
			}
			else
			{
				if (COUNTDOWN < 400)
				{
					g_currentEyeHeight = 25.f;
					light_ambient[0] = 0.2f;
					light_ambient[1] = 0.2f;
					light_ambient[2] = 0.2f;
				}
			}

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glLightfv(GL_LIGHT1, GL_POSITION, light_position2);
			glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, light_direction);

			glLightfv(GL_LIGHT2, GL_POSITION, light_position2);
			glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, light_direction);

			g_playerViewDir.x = g_playerFront.x * g_playerFrontUp.y;
			g_playerViewDir.y = g_playerFront.y * g_playerFrontUp.y;
		//	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, light_direction);
			
			//took damage this frame
			bool shake = g_playerHealthPrev != g_playerHealth;
			g_playerHealthPrev = g_playerHealth;
			gluLookAt(g_playerLoc.x + (shake ? 0.15 * (rnd() % 16) / 16.f : 0.f), g_currentEyeHeight, g_playerLoc.y + (shake ? 0.15 * (rnd() % 16) / 16.f : 0.f),
				g_playerLoc.x + g_playerViewDir.x, g_currentEyeHeight + g_playerFrontUp.x, g_playerLoc.y + g_playerViewDir.y,
				0.0, 1.0, 0.0);      /* up is in positive Y direction */

			glLightfv(GL_LIGHT0, GL_POSITION, light_position);
			light_ambient[0] = 1.f - (g_playerHealth) / MAXHEALTH;
			glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
			
			if (GetAsyncKeyState(VK_ESCAPE) || bIsDead && COUNTDOWN <= 0)
				break;

			//clear background
			glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			//ground
			drawPlane(0.f);

			draw();
	//		glColor3b(255, 0, 0);

			SwapBuffers(hdc);

			//let windows work
			MSG msg;
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
			//	DispatchMessage(&msg); fuck this
			}

			tick = GetTickCount() - tick;
			if (tick < 17 && tick > 0)Sleep(17 - tick);
			tick = GetTickCount();
		}

		ExitProcess(0);
	}
}
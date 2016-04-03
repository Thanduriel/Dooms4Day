#include "pawn.hpp"

#define MAPLEN 32
//MAPLEN²
#define MAPSIZE 1024

//game constants
#define EYEHEIGHT 0.5
#define ENEMYHEIGHT 0.6f
#define MAXHEALTH 400.f

//ai
// all radius as r²
#define PERCEPTIONAI 128.f 
#define DMGRADIUS 4.f

inline int index(int x, int y)
{
	return x + MAPLEN * y;
}

inline unsigned int rnd()
{
	static unsigned int num = 0x136A4F13;
	num ^= num << 13;
	num ^= num >> 17;
	num ^= num << 5;

	return num;
}

/* abs(x) = 0.25 - nothing
 * x < 0 - already visited
 */


struct Map
{
	void generate();

	Pawn* rayCast(const Vec2& _loc, Vec2 _dir, float _z);

	int x;
	int y;
	//make the buffers a little bigger to prevent overflows
	float data[MAPSIZE + 2 * MAPLEN + 1];
	Pawn enemys[ENEMYCOUNT * 4];
	int enemyCount;
};
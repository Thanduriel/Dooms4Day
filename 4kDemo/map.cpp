#include "map.hpp"

Pawn g_enemys[ENEMYCOUNT];

void Map::generate()
{
	enemyCount = 0;
	for (int i = 0; i < MAPSIZE; ++i) data[i] = 1.f + (float)(rnd() % 8) / 5.f;
	for (int i = 0; i < MAPSIZE; ++i)
	{
		x = i % MAPLEN;
		y = i / MAPLEN;

		bool flip = true;
		if (x % 2 && y % 2)
		{
			data[i] = 0.f;
			if (rnd() % 2)
			{
				data[i + 1] = 0.f;
			}
			else
			{
				data[i + MAPLEN] = 0.f;
			}

			//1/4 off the tiles is a possible spawn location
			if (ENEMYCOUNT * 2 / (MAPSIZE * 0.25) >= (float)(rnd() % 512) / 512.f)
			{
				Pawn& pawn = enemys[enemyCount++];
				pawn.isAlive = flip;
				flip = !flip;
				pawn.health = 124.f;
				pawn.loc = Vec2((float)x*2.f, (float)y*2.f);
			}
		}
	}
}

Pawn* Map::rayCast(const Vec2& _loc, Vec2 _dir, float _z)
{
	Vec2 current(_loc);
	_dir.x *= 1.9f;
	_dir.y *= 1.9f;
	
	//hit is behind the map = no hit
	Vec2 hit(MAPLEN + 1.f, MAPLEN + 1.f);
	do{
		if (data[index((int)(current.x/2.f), (int)(current.y/2.f))])
		{
			hit = current;
			break;
		}
		
		current = current + _dir;
	} while (current.x < MAPLEN * 2.f && current.y < MAPLEN *2.f);

	for (int i = 0; i < ENEMYCOUNT * 2.f; ++i)
	{
		if (!enemys[i].isAlive) continue;

		float a = dot(_dir, _z, _dir, _z);
		float b = 2.0 * (_dir.x *  (_loc.x - enemys[i].loc.x) + _dir.y *  (_loc.y - enemys[i].loc.y) + _z *  (EYEHEIGHT - ENEMYHEIGHT));
		float c = dot(enemys[i].loc, ENEMYHEIGHT, enemys[i].loc, ENEMYHEIGHT)
			+ dot(_loc, EYEHEIGHT, _loc, EYEHEIGHT)
			- 2.0 * dot(enemys[i].loc, ENEMYHEIGHT, _loc, EYEHEIGHT)
			- 0.4f;//r

		float test = b*b - 4.0*a*c;
		if (test >= 0.f)
		{
			Vec2 dif = hit - _loc;
			float len = dif.x * dif.x + dif.y * dif.y;
			dif = enemys[i].loc - _loc;
			if (len > dif.x * dif.x + dif.y * dif.y) return &enemys[i];
		}
	}

	return nullptr;
}

/* depth search
x = 0;
y = 1;

int tmp;
int count = 0;
for (;;)
{
int oldX = x;
int oldY = y;
movDir(rand() % 4);
tmp = INDEX(x, y);
count++;

if (tmp < 0 || tmp >= MAPSIZE || data[tmp] < 0.f) continue;


if (count > 32) break;
}*/

#pragma once

#define PI 3.1415

inline float _sin(float x)
{
	__asm
	{
		fld x
		fsin
		fstp x
	}
	return x;
}

inline float _cos(float x)
{
	__asm
	{
		fld x
		fcos
		fstp x
	}
	return x;
}

inline float _sqrt(float x)
{
	__asm
	{
		fld x
		fsqrt
		fstp x
	}
	return x;
}

struct Vec2
{
	float x;
	float y;

	Vec2() = default;
	Vec2(float _x, float _y) : x(_x), y(_y){}

	Vec2 operator-(const Vec2& _oth)
	{
		Vec2 res;
		res.x = x - _oth.x;
		res.y = y - _oth.y;

		return res;
	}

	Vec2 operator+(const Vec2& _oth)
	{
		Vec2 res;
		res.x = x + _oth.x;
		res.y = y + _oth.y;

		return res;
	}

	Vec2& rotate(const Vec2& _angle)
	{
		float oldX = x;
		x = x * _angle.x - y * _angle.y;
		y = oldX * _angle.y + y * _angle.x;

		return *this;
	}
};
//3d dot
inline float dot(const Vec2& _0, float _z0, const Vec2& _1, float _z1)
{
	return _0.x * _1.x + _0.y * _1.y + _z0 * _z1;
}

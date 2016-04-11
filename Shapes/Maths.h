#pragma once

#define PI 3.1415926535897932384626433832795
#define DEGTORAD (2*PI)/ (double)360

// Fast sqrt using assembly fsqrt
// different method discussed here : http://www.codeproject.com/Articles/69941/Best-Square-Root-Method-Algorithm-Function-Precisi
double inline __declspec (naked) __fastcall fast_sqrt(double n)
{
	_asm fld qword ptr[esp + 4]
		_asm fsqrt
	_asm ret 8
}

//Fast sin using assembly fsin
//Better than Chebyshev Approximation 
double inline __declspec (naked) __fastcall fast_sin(double n)
{
	_asm fld qword ptr[esp + 4]
		_asm fsin
	_asm ret 8
}

//Fast cos using assembly fsin
//Better than Chebyshev Approximation 
double inline __declspec (naked) __fastcall fast_cos(double n)
{
	_asm fld qword ptr[esp + 4]
		_asm fcos
	_asm ret 8
}


struct Vec2
{
	Vec2(float x, float y) : x(x), y(y) {}
	Vec2() : x(0.f), y(0.f) {}
		
	float x;
	float y;

	friend std::ostream& operator << (std::ostream& out, const Vec2& v);
};
Vec2 operator- (const Vec2& a, const Vec2& b) { return (Vec2(a.x - b.x, a.y - b.y)); }
Vec2 operator+ (const Vec2& a, const Vec2& b) { return (Vec2(a.x + b.x, a.y + b.y)); }
Vec2 operator* (int a, const Vec2& b) { return (Vec2(a*b.x, a*b.y)); }
Vec2 operator* (const Vec2& a, int b) { return (Vec2(b*a.x, b*a.y)); }
Vec2 operator* (float a, const Vec2& b) { return (Vec2(a*b.x, a*b.y)); }
Vec2 operator* (const Vec2& a, float b) { return (Vec2(b*a.x, b*a.y)); }
bool operator== (const Vec2& a, const Vec2& b) { if (a.x == b.x && a.y == b.y) return true; return false; }
bool operator== (const Vec2& a, Vec2& b) { if (a.x == b.x && a.y == b.y) return true; return false; }
//Vec2 operator^ (const Vec2& a, const Vec2& b) { return ( ) }
std::ostream& operator << (std::ostream& out,const Vec2& v){ out << "(" << v.x << " , " << v.y << ")"; return out; }

struct Color
{
	Color() :r(0), g(0), b(0) {}
	Color(int r, int g, int b):r(r), g(g), b(b) {}
	int r, g, b;

	friend std::ostream& operator << (std::ostream& out, const Color& c);
};
bool operator== (const Color& a, const Color& b) { return (a.r == b.r && a.g == b.g && a.b == b.b); }
//bool operator== (Color& a, Color& b) { if (a.r == b.r && a.g == b.g && a.b == b.b) return true; return false; }
bool operator< (const Color a, const Color b) { 
	float l1 = (a.r * 256 + a.g) * 256 + a.b;
	float l2 = (a.r * 256 + a.g) * 256 + a.b;
	return (l1 < l2) ;
}  // needed for use in map
std::ostream& operator << (std::ostream& out, const Color& c)
{
	out << "(" << c.r << " , " << c.g << " , " << c.b << ")";
	return out;
}

float dot(Vec2 a, Vec2 b)
{
	return(a.x * b.x + a.y * b.y);
}

float norm(Vec2 a)
{
	return (fast_sqrt((a.x * a.x) + (a.y * a.y)));
}
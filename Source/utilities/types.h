#pragma once

#include "platform/platform.h"
#include <math.h>

// Fundamental Data Types

#define MAX_FLOAT          3.402823466e+38F        // max value
#define MIN_FLOAT          1.175494351e-38F        // min normalized positive value

#define INV_UINT32_MAX	   2.328306437e-10F

typedef signed char        int8;
typedef short              int16;
typedef int                int32;
typedef long long          int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

typedef bool b8;
typedef int b32;

typedef float f32;
typedef double f64;
//-----------------------


// Operations

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

template <typename t>
inline t min(t a, t b)
{
	return ((a > b) ? b : a);
}

template <typename t>
inline t max(t a, t b)
{
	return ((a > b) ? a : b);
}

template <typename T> 
inline void swap(T& a, T& b)
{
	T tmp = a;
	a = b;
	b = tmp;
}


template <typename T>
inline T sqr(T n)
{
	return ((n) * (n));
}



//-----------------------

//-----------------------
// Collections
//----------------------
// DYNAMIC BUFFER
template<typename t, int32 capacity__ = 1,int32 overflow__ = 5>
struct DBuffer
{
	int32 length = 0;
	int32 capacity = capacity__;
	int32 overflow_addon = overflow__;
	t* front = 0;
	
	inline void add(t new_member)
	{
		length++;
		if (front == 0)		//Buffer was not initilized and is being initialized here. 
		{
			front = (t*)calloc(capacity , sizeof(t));
		}
		if (length > capacity)
		{
			capacity = capacity + overflow_addon;
			t* temp = (t*)realloc(front, capacity * sizeof(t));
			ASSERT(temp);	//Not enough memory to realloc, or buffer was never initialized and realloc is trying to allocate to null pointer
			front = temp;
		}

		t* temp = front;
		temp = temp + (length - 1);
		*temp = new_member;
	}

	//Same as add but doesn't perform copy. (Use for BIG objects)
	inline void add_nocpy(t &new_member)
	{
		length++;
		if (front == 0)		//Buffer was not initilized and is being initialized here. 
		{
			front = (t*)calloc(capacity, sizeof(t));
		}
		if (length > capacity)
		{
			capacity = capacity + overflow_addon;
			t* temp = (t*)realloc(front, capacity * sizeof(t));
			ASSERT(temp);	//Not enough memory to realloc, or buffer was never initialized and realloc is trying to allocate to null pointer
			front = temp;
		}

		t* temp = front;
		temp = temp + (length - 1);
		*temp = new_member;
	}

	//Clears memory and resets length.
	inline void clear_buffer()
	{
		length = 0;
		free(front);
	}

	inline t& operator [](int32 index)
	{
		ASSERT(index >= 0 && index < length);
		return (front[index]);
	}
};

// FIXED DYNAMIC BUFFER
//A wrapper for a non-resizable dynamic buffer 
template<typename t, typename size_type = int32>
struct FDBuffer
{
	size_type size;
	t* front = 0;

	inline t* init(int32 size_)
	{
		size = size_;
		front = (t*)calloc(size, sizeof(t));
		return front;
	}
	inline void clear()
	{
		size = 0;
		free(front);
	}
	inline t& operator [](int32 index)
	{
		ASSERT(index >= 0 && index < size);
		return (front[index]);
	}
};


//-----------------------
// Math Data Types
//----------------------
// VEC2
template <typename t>
struct Vec2
{
	union
	{
		struct { t x, y; };
		t raw[2];
	};

	Vec2() :x(0), y(0) {}
	Vec2(t _x, t _y) : x(_x), y(_y) {}

	inline Vec2<t> operator + (Vec2<t> n) { return Vec2<t>(x + n.x, y + n.y); };
	inline void operator += (Vec2<t> n) { x += n.x; y += n.y; };

	inline Vec2<t> operator - (Vec2<t> n) { return Vec2<t>(x - n.x, y - n.y); };
	inline void operator -= (Vec2<t> n) { x -= n.x; y -= n.y; };

	inline Vec2<t> operator * (f32 n) { return Vec2<t>(x * n, y * n); }
	inline void operator *= (f32 n) { x *= n; y *= n; };

	inline Vec2<t> operator / (f32 n) { return Vec2<t>(x / n, y / n); };

};
typedef Vec2<f32> vec2f;
typedef Vec2<uint32> vec2ui;
typedef Vec2<int32> vec2i;
typedef Vec2<uint8> vec2b;

inline f32 mag2(vec2f& vector)
{
	return { (vector.x * vector.x) + (vector.y * vector.y) };
}

inline f32 mag(vec2f& vector)
{
	return { sqrtf(mag2(vector)) };
}

//passing by value
inline f32 mag2_val(vec2f vector)
{
	return { (vector.x * vector.x) + (vector.y * vector.y) };
}

inline f32 mag_val(vec2f vector)
{
	return { sqrtf(mag2_val(vector)) };
}

//----------------------
// VEC3
template <typename t>
struct Vec3
{
	union
	{
		struct {
			t x;
			t y;
			t z;
		};
		struct {
			t r;
			t g;
			t b;
		};
		t raw[3] = { 0,0,0 };
	};

	Vec3() : x(0), y(0), z(0) {}
	Vec3(t _x, t _y, t _z) : x(_x), y(_y), z(_z) {}

	inline Vec3<t> operator + (Vec3<t> n) { return Vec3<t>(x + n.x, y + n.y, z + n.z); };
	inline void operator += (Vec3<t> n) { x += n.x; y += n.y; z += n.z; };

	inline Vec3<t> operator - () { return Vec3<t>(-x, -y, -z); };

	inline Vec3<t> operator - (Vec3<t> n) { return Vec3<t>(x - n.x, y - n.y, z - n.z); };
	inline void operator -= (Vec3<t> n) { x -= n.x; y -= n.y; z -= n.z; };

	inline bool operator != (Vec3<t> b) { return (x == b.x && (y == b.y && z == b.z)); };

	inline Vec3<t> operator * (f32 n) { return Vec3<t>(x * n, y * n, z * n); };
	inline void operator *= (f32 n) { x *= n; y *= n; z *= n; };

	inline Vec3<t> operator / (f32 n) { return Vec3<t>(x / n, y / n, z / n); };

};
typedef Vec3<f32> vec3f;
typedef Vec3<uint32> vec3ui;
typedef Vec3<int32> vec3i;
typedef Vec3<uint8> vec3b;


inline f32 mag2( vec3f& vector)
{
	return { (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z) };
}

inline f32 mag( vec3f& vector)
{
	return { sqrtf(mag2(vector)) };
}

//passing by value
inline f32 mag2_val(vec3f vector)
{
	return { (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z) };
}

inline f32 mag_val(vec3f vector)
{
	return { sqrtf(mag2_val(vector)) };
}

inline b32 normalize(vec3f& v)
{
	if (mag2(v) == 1.0f)
	{
		return false;
	}
	v = v / mag(v);
	return true;
}

//----------------------
// VEC4
template <typename t>
struct Vec4
{
	union
	{
		t raw[4] = { 0 };
		struct {
			t x;
			t y;
			t z;
			t w;
		};
		struct {
			Vec2<t> left_top, right_bottom;
		};
		struct {
			t r;
			t g;
			t b;
			t a;
		};
	};

	Vec4() : x(0),y(0),z(0),w(0) {}
	Vec4(Vec2<t> left_top_,Vec2<t> right_bottom_) : left_top(left_top), right_bottom(right_bottom_) {}
	Vec4(t _x, t _y, t _z, t _w) : x(_x), y(_y), z(_z), w(_w) {}
	Vec4( Vec3<t> &replace, t _w) : x(replace.x), y(replace.y), z(replace.z), w(_w) {}

	inline Vec4<t> operator + (Vec4<t> n) { return Vec4<t>(x + n.x, y + n.y, z + n.z, w + n.w); }
	inline void operator += (Vec4<t> n) { x += n.x; y += n.y; z += n.z; w += n.w; }

	inline Vec4<t> operator - (Vec4<t> n) { return Vec4<t>(x - n.x, y - n.y, z - n.z, w - n.w); }
	inline void operator -= (Vec4<t> n) { x -= n.x; y -= n.y; z -= n.z; w -= n.w; }

	inline Vec4<t> operator * (f32 n) { return Vec4<t>(x * n, y * n, z * n, w * n); }
	inline void operator *= (f32 n) { x *= n; y *= n; z *= n; w *= n; }

	inline Vec4<t> operator / (f32 n) { return Vec4<t>(x / n, y / n, z / n, w / n); }
	inline f32 operator * ( Vec4<t> n) { return (x * n.x) + (y * n.y) + (z * n.z); }

};
typedef Vec4<f32> vec4f;
typedef Vec4<uint32> vec4ui;
typedef Vec4<int32> vec4i;
typedef Vec4<uint8> vec4b;
typedef Vec4<int32> Tile;

inline f32 mag2( vec4f& vector)
{
	return { (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z) + (vector.w * vector.w) };
}

inline f32 mag( vec4f& vector)
{
	return { sqrtf(mag2(vector)) };
}

//passing by value
inline f32 mag2_val(vec4f vector)
{
	return { (vector.x * vector.x) + (vector.y * vector.y) + (vector.z * vector.z) + (vector.w * vector.w) };
}

inline f32 mag_val(vec4f vector)
{
	return { sqrtf(mag2_val(vector)) };
}

inline b32 normalize(vec4f& v)
{
	if (mag2(v) == 1.0f)
	{
		return false;
	}
	v = v / mag(v);
	return true;
}


//----------------------
// MAT3x3
template<typename t>
struct Mat33
{
	union
	{
		t raw[3][3] = { 1,0,0,
						0,1,0,
						0,0,1 };

		struct
		{
			t xi;
			t xj;
			t xk;
			t yi;
			t yj;
			t yk;
			t zi;
			t zj;
			t zk;
		};
	};

	Mat33(t a, t b, t c, t d, t e, t f, t g, t i, t j, t k)
	{
		raw[0] = a;
		raw[1] = b;
		raw[2] = c;
		raw[3] = e;
		raw[4] = f;
		raw[5] = g;
		raw[6] = i;
		raw[7] = j;
		raw[8] = k;
	}

	inline Mat33<t> operator + (Mat33<t>& n) {
		return Mat33<t>(
			raw[0] + n.raw[0], raw[1] + n.raw[1], raw[2] + n.raw[2],
			raw[3] + n.raw[3], raw[4] + n.raw[4], raw[5] + n.raw[5],
			raw[6] + n.raw[6], raw[7] + n.raw[7], raw[8] + n.raw[8]);
	}

	inline Mat33<t> operator - (Mat33<t>& n) {
		return Mat33<t>(
			raw[0] - n.raw[0], raw[1] - n.raw[1], raw[2] - n.raw[2],
			raw[3] - n.raw[3], raw[4] - n.raw[4], raw[5] - n.raw[5],
			raw[6] - n.raw[6], raw[7] - n.raw[7], raw[8] - n.raw[8]);
	}

	inline Mat33<t> operator * (t n) {
		return Mat33<t>(
			raw[0] * n, raw[1] * n, raw[2] * n,
			raw[3] * n, raw[4] * n, raw[5] * n,
			raw[6] * n, raw[7] * n, raw[8] * n
			);
	}

	inline Vec3<t> operator * (Vec3<t>& n) {
		return Vec3<t>(xi * n.x + xj * n.y + xk * n.z, yi * n.x + yj * n.y + yk * n.z, zi * n.x + zj * n.y + zk * n.z);

	}

	inline Mat33<t> operator * (Mat33<t>& n) {
		return (
			Mat33<t>(
				xi * n.xi + xj * n.yi + xk * n.zi,
				xi * n.xj + xj * n.yj + xk * n.zj,
				xi * n.xk + xj * n.yk + xk * n.zk,
				yi * n.xi + yj * n.yi + yk * n.zi,
				yi * n.xj + yj * n.yj + yk * n.zj,
				yi * n.xk + yj * n.yk + yk * n.zk,
				zi * n.xi + zj * n.yi + zk * n.zi,
				zi * n.xj + zj * n.yj + zk * n.zj,
				zi * n.xk + zj * n.yk + zk * n.zk
				)
			);
	}
};
//----------------------
// MAT4x4
template<typename t>
struct Mat44
{
	t raw[4 * 4] = { 1,0,0,0,
					 0,1,0,0,
					 0,0,1,0,
					 0,0,0,1 };



	Mat44(t a, t b, t c, t d, t e, t f, t g, t h, t i, t j, t k, t l, t m, t n, t o, t p, t q)
	{
		raw[0] = a;
		raw[1] = b;
		raw[2] = c;
		raw[3] = e;
		raw[4] = f;
		raw[5] = g;
		raw[6] = h;
		raw[7] = i;
		raw[8] = j;
		raw[9] = k;
		raw[10] = l;
		raw[11] = m;
		raw[12] = n;
		raw[13] = o;
		raw[14] = p;
		raw[15] = q;

	}

	inline t& at(int32 row, int32 column) { return raw[row * 4 + column]; }

	inline Mat44<t> operator + (Mat44<t>& n) {
		return Mat44<t>(
			raw[0] + n.raw[0], raw[1] + n.raw[1], raw[2] + n.raw[2], raw[3] + n.raw[3],
			raw[4] + n.raw[4], raw[5] + n.raw[5], raw[6] + n.raw[6], raw[7] + n.raw[7],
			raw[8] + n.raw[8], raw[9] + n.raw[9], raw[10] + n.raw[10], raw[11] + n.raw[11],
			raw[12] + n.raw[12], raw[13] + n.raw[13], raw[14] + n.raw[14], raw[15] + n.raw[15]
			);
	}

	inline Mat44<t> operator - (Mat44<t>& n) {
		return Mat44<t>(
			raw[0] - n.raw[0], raw[1] - n.raw[1], raw[2] - n.raw[2], raw[3] - n.raw[3],
			raw[4] - n.raw[4], raw[5] - n.raw[5], raw[6] - n.raw[6], raw[7] - n.raw[7],
			raw[8] - n.raw[8], raw[9] - n.raw[9], raw[10] - n.raw[10], raw[11] - n.raw[11],
			raw[12] - n.raw[12], raw[13] - n.raw[13], raw[14] - n.raw[14], raw[15] - n.raw[15]
			);
	}

	inline Mat44<t> operator * (t n) {
		return Mat44<t>(
			raw[0] * n, raw[1] * n, raw[2] * n, raw[3] * n,
			raw[4] * n, raw[5] * n, raw[6] * n, raw[7] * n,
			raw[8] * n, raw[9] * n, raw[10] * n, raw[11] * n,
			raw[12] * n, raw[13] * n, raw[14] * n, raw[15] * n
			);
	}

	inline Vec4<t> operator * (Vec4<t>& n) {
		return Vec4<t>(
			raw[0] * n.raw[0] + raw[1] * n.raw[1] + raw[2] * n.raw[2] + raw[3] * n.raw[3],
			raw[4] * n.raw[0] + raw[5] * n.raw[1] + raw[6] * n.raw[2] + raw[7] * n.raw[3],
			raw[8] * n.raw[0] + raw[9] * n.raw[1] + raw[10] * n.raw[2] + raw[11] * n.raw[3],
			raw[12] * n.raw[0] + raw[13] * n.raw[1] + raw[14] * n.raw[2] + raw[15] * n.raw[3]
			);

	}

	Mat44<t> operator * (Mat44<t>& n) {
		Mat44<t> r;
		for (int32 j = 0; j < 4; j++)
		{

			r.raw[j * 4 + 0] = raw[j * 4 + 0] * n.raw[0] + raw[j * 4 + 1] * n.raw[1 * 4 + j] + raw[j * 4 + 2] * n.raw[8] + raw[j * 4 + 3] * n.raw[12];
			r.raw[j * 4 + 1] = raw[j * 4 + 0] * n.raw[1] + raw[j * 4 + 1] * n.raw[1 * 4 + j] + raw[j * 4 + 2] * n.raw[9] + raw[j * 4 + 3] * n.raw[13];
			r.raw[j * 4 + 2] = raw[j * 4 + 0] * n.raw[2] + raw[j * 4 + 1] * n.raw[1 * 4 + j] + raw[j * 4 + 2] * n.raw[10] + raw[j * 4 + 3] * n.raw[14];
			r.raw[j * 4 + 3] = raw[j * 4 + 0] * n.raw[3] + raw[j * 4 + 1] * n.raw[1 * 4 + j] + raw[j * 4 + 2] * n.raw[11] + raw[j * 4 + 3] * n.raw[15];

		}
		return r;
	}
};
//----------------------
//----------------------


//----------------------
// General Math
//----------------------

//Returns |p|*|n|*cos(theta) 
inline f32 dot(vec3f& p, vec3f& n) { return (p.x * n.x) + (p.y * n.y) + (p.z * n.z); };

//Returns vector as result of multiplication of individual components
inline vec3f hadamard(vec3f& a, vec3f& b) { return vec3f(a.x * b.x, a.y * b.y, a.z * b.z); }

//Returns |a|*|b|* sin(theta) * n_cap(n_cap is normalized perpendicular to a and b)
inline vec3f cross(vec3f& a, vec3f& b) { return vec3f(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }

//used to interpolate between vectors. value should be between 0 and 1.
inline vec2f lerp(vec2f& start, vec2f& towards, f32 interpolation)
{
	return { (towards - start) / interpolation };
}

//used to interpolate between vectors. value should be between 0 and 1.
inline vec3f lerp(vec3f& start, vec3f& towards, f32 interpolation)
{
	return { ((towards - start) * interpolation) + start };
}

//used to interpolate between vectors. value should be between 0 and 1.
inline vec4f lerp(vec4f& start, vec4f& towards, f32 interpolation)
{
	return { ((towards - start) * interpolation) + start };
}

//used to convert float 0.0 to 1.0 rgb values to a byte vector
inline vec3b rgb_float_to_byte(vec3f& color)
{
	return vec3b((uint8)(color.r * 255.0), (uint8)(color.g * 255.0), (uint8)(color.b * 255.0));
}


struct RNG_Stream
{
	uint64 state;	//Used to keep track of streams position
	uint64 stream;	//the actual "seed". Determines which stream the RNG uses. 
					//NOTE: stream must be odd value (sequential streams will produce same result)
};

#if _MSC_VER > 1800
#pragma warning(disable:4146)
#pragma warning(disable:4244)
#endif
//implemented from https://www.pcg-random.org/
inline uint32 random_u32(RNG_Stream* stream)
{
	//updating the stream state
	uint64 oldstate = stream->state;
	stream->state = oldstate * 6364136223846793005ULL + (stream->stream | 1); //"stream" must be odd value
	
	//generating number from stream:
	uint32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u; 
	uint32 rot = oldstate >> 59u; 
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
#if _MSC_VER > 1800
#pragma warning(default:4244)  //disables the "loss of data u32 to u64" conversion warning
#pragma warning(default:4146)  //disables the negation on unsigned int (rot) warning
#endif

//returns random number between 0 and 1
inline f32 rand_uni(RNG_Stream* stream)
{
	//NOTE: multiplying with 2.328306437e-10F to avoid division (make fast).
	//		this leads to 1.0f not being achievable
	f32 rd = (f32)random_u32(stream) * INV_UINT32_MAX;

	return rd;
}

//returns random number between -1 and 1
inline f32 rand_bi(RNG_Stream* stream)
{
	//NOTE: rand_uni multiplying with 2.328306437e-10F to avoid division (make fast).
	//		this leads to 1.0f, 0.0f and -1.0f not being achievable
	f32 rd = -1.0f + 2.0f * rand_uni(stream);
	return rd;
}
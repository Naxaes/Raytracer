// Scalar
inline u32
RoundToU32(f32 x)
{
	u32 result = (u32) (x + 0.5f);
	return result;
}


// 3D Vector
union v3
{
	struct
	{
		f32 x, y, z;	
	};
	struct
	{
		f32 r, g, b;	
	};
};

inline v3
V3(f32 x, f32 y, f32 z)
{
	v3 result = {x, y, z};
	return result;
}

inline v3
operator- (v3 a)
{
	v3 result = {-a.x, -a.y, -a.z};
	return result;
}

inline v3
operator- (v3 a, v3 b)
{
	v3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
	return result;
}

inline v3
operator+ (v3 a, v3 b)
{
	v3 result = {a.x + b.x, a.y + b.y, a.z + b.z};
	return result;
}

inline v3
operator* (v3 a, f32 b)
{
	v3 result = {a.x * b, a.y * b, a.z * b};
	return result;
}

inline f32
Length(v3 a)
{
	return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
}

inline v3
NormalizeOrZero(v3 a)
{
	v3 result = {};

	f32 length = Length(a);
	if (length <= 0)
	{
		return result;
	}

	result = {a.x/length, a.y/length, a.z/length};
	
	return result;
}

inline v3
Cross(v3 a, v3 b)
{
	v3 result;

	result.x = a.y*b.z - a.z*b.y;
	result.y = a.z*b.x - a.x*b.z;
	result.z = a.x*b.y - a.y*b.x;

	return result;
}

inline f32
Inner(v3 a, v3 b)
{
	f32 result = a.x*b.x + a.y*b.y + a.z*b.z;

	return result;
}

inline v3
Hadamard(v3 a, v3 b)
{
	v3 result = {a.x*b.x, a.y*b.y, a.z*b.z};

	return result;
}

inline v3
Lerp(v3 a, f32 factor, v3 b)
{
	ASSERT(0 <= factor && factor <= 1.0f);
	v3 result = a * (1-factor) + b * factor;

	return result;
}



// 4D Vector
union v4
{
	struct
	{
		f32 x, y, z, w;
	};
	struct
	{
		f32 r, g, b, a;	
	};
};

inline v4
V4(f32 x, f32 y, f32 z, f32 w)
{
	v4 result = {x, y, z, w};
	return result;
}

inline v4
V4(v3 a, f32 w)
{
	v4 result = {a.x, a.y, a.z, w};
	return result;
}

inline v4
operator- (v4 a, v4 b)
{
	v4 result = {a.x - b.x, a.y - b.y, a.z - b.z, a.w * b.w};
	return result;
}

inline v4
operator+ (v4 a, v4 b)
{
	v4 result = {a.x + b.x, a.y + b.y, a.z + b.z, a.w * b.w};
	return result;
}

inline v4
operator* (v4 a, f32 b)
{
	v4 result = {a.x * b, a.y * b, a.z * b, a.w * b};
	return result;
}

inline u32
PackRGBA(v4 color)
{
	u32 result = (RoundToU32(color.a) << 24) |
				 (RoundToU32(color.b) << 16) |
				 (RoundToU32(color.g) << 8)  |
				 (RoundToU32(color.r) << 0);

	return result;
}

inline u32
PackBGRA(v4 color)
{
	u32 result = (RoundToU32(color.a) << 24) |
				 (RoundToU32(color.r) << 16) |
				 (RoundToU32(color.g) << 8)  |
				 (RoundToU32(color.b) << 0);

	return result;
}
#define LANE_COUNT 1


internal_function u64 
XORShift64(random_series* random)
{
	// https://en.wikipedia.org/wiki/Xorshift
	u64 x = random->state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	random->state = x;

	return x;
}

internal_function f32
RandomUnilateral(random_series* random)
{
	f32 result = (f32)XORShift64(random) / (f32)U64_MAX;
	// f32 result = (f32)rand() / (f32)RAND_MAX;
	return result;
}

internal_function f32
RandomBilateral(random_series* random)
{
	f32 result = -1.0f + 2.0f * RandomUnilateral(random);
	return result;
}


#if LANE_COUNT == 1

typedef v3  lane_v3;

typedef s8  lane_s8;
typedef s16 lane_s16;
typedef s32 lane_s32;
typedef s64 lane_s64;

typedef u8  lane_u8;
typedef u16 lane_u16;
typedef u32 lane_u32;
typedef u64 lane_u64;

typedef f32 lane_f32;
typedef f64 lane_f64;


internal_function lane_f32
RandomUnilateralLane(random_series* random)
{
	lane_f32 result = RandomUnilateral(random);
	return result;
}

internal_function lane_f32
RandomBilateralLane(random_series* random)
{
	f32 result = RandomBilateral(random);
	return result;
}

internal_function void
ConditionalAssign(lane_u32* destination, lane_u32 mask, lane_u32 source)
{
	mask = mask ? 0xFFFFFFFF : 0;
	*destination = (*destination & ~mask) | (source & mask);
}

internal_function void
ConditionalAssign(lane_f32* destination, lane_u32 mask, lane_f32 source)
{
	ConditionalAssign((lane_u32*)destination, mask, *(lane_u32*)&source);
}

internal_function void
ConditionalAssign(lane_v3* destination, lane_u32 mask, lane_v3 source)
{
	ConditionalAssign(&destination->x, mask, source.x);
	ConditionalAssign(&destination->y, mask, source.y);
	ConditionalAssign(&destination->z, mask, source.z);
}

internal_function lane_f32
Max(lane_f32 a, lane_f32 b)
{
	lane_f32 result = (a > b) ? a : b;
	return result;
}

internal_function u32
MaskIsZero(lane_u32 mask)
{
	u32 result = (mask == 0);
	return result;
}


internal_function u32
HorizontalAdd(lane_u32 a)
{
	u32 result = a;
	return result;
}

internal_function f32
HorizontalAdd(lane_f32 a)
{
	f32 result = a;
	return result;
}

internal_function v3
HorizontalAdd(lane_v3 a)
{
	v3 result = {
		HorizontalAdd(a.x),
		HorizontalAdd(a.y),
		HorizontalAdd(a.z)
	};

	return result;
}




#else
	#error "The current LANE_COUNT is not supported."
#endif

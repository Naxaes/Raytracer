typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

#define U64_MAX ((u64)-1)
#define U32_MAX ((u32)-1)
#define F32_MAX FLT_MAX

#define ASSERT(x) assert(x)
#define ArrayCount(array) (sizeof(array)/sizeof((array)[0]))

#include "maths.h"


#pragma pack(push, 1)
struct bitmap_header
{
	u16 file_type;
	u32 file_size;
	u16 reserved_1;
	u16 reserved_2;
	u32 bitmap_offset;
	u32 size;
	s32 width;
	s32 height;  // Negative numbers gives a direction of top-down instead of bottom-up.
	u16 planes;
	u16 bits_per_pixel;
	u32 compression;
	u32 size_of_bitmap;
	s32 horizontal_resolution;
	s32 vertical_resolution;
	u32 colors_used;
	u32 colors_important;
};
#pragma pack(pop)

struct image_buffer_u32
{
	u32 width;
	u32 height;
	u32* pixels;
};

struct material
{
	f32 specular;
	v3  emission_color;
	v3  reflection_color;
};

struct plane
{
	v3  normal;
	f32 offset;
	u32 material_index;
};

struct sphere
{
	v3  position;
	f32 radius;
	u32 material_index;
};

struct world_state
{
	u32 material_count;
	material* materials;

	u32 plane_count;
	plane* planes;

	u32 sphere_count;
	sphere* spheres;
};

struct random_series 
{
	u64 state;
};

struct work_order
{
	world_state*     world;
	image_buffer_u32 image;	

	u32 x_start;
	u32 y_start;
	u32 x_stop;
	u32 y_stop;

	random_series entropy;
};

struct work_queue
{
	u32 work_order_count;
	work_order* work_orders;

	volatile u64 current_worker_index;
	volatile u64 tiles_processed;
	volatile u64 bounce_count;
};

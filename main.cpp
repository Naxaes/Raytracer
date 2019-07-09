#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <time.h>

#define internal_function  static
#define global_variable    static
#define locally_persistent static

#include "main.h"



// ---- OS-SPECIFIC ----
internal_function u64
LockedAddAndReturnPreviousValue(volatile u64* value, u64 to_add);

internal_function u32
GetCoreCount();

internal_function void
CreateRenderTileThread(work_queue* queue);


// ---- OS-AGNOSTIC ----
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

internal_function u32
SizeOfImage(image_buffer_u32 image)
{
	u32 result = image.width * image.height * sizeof(u32);
	return result;
}

internal_function u32*
GetImagePointer(image_buffer_u32 image, u32 x, u32 y)
{
	u32* result = image.pixels + x + y*image.width;
	return result;
}

internal_function image_buffer_u32
AllocateImage(u32 width, u32 height)
{
	image_buffer_u32 image = {};
	image.width  = width;
	image.height = height;

	image.pixels = (u32*) malloc(SizeOfImage(image));

	return image;
}

internal_function s32
WriteImage(image_buffer_u32 image, const char* file_name)
{

	u32 size_of_image = SizeOfImage(image);

	bitmap_header header = {};

	header.file_type = 0x4D42;
	header.file_size = sizeof(header) + size_of_image;
	header.bitmap_offset = sizeof(header);
	header.size   = sizeof(header) - 14;
	header.width  = image.width; 
	header.height = image.height;  // Negative numbers gives a direction of top-down instead of bottom-up.
	header.planes = 1;
	header.bits_per_pixel = 32;
	header.compression    = 0;
	header.size_of_bitmap = size_of_image;
	header.horizontal_resolution = 0;
	header.vertical_resolution   = 0;
	header.colors_used = 0;
	header.colors_important = 0;


	FILE* file = fopen(file_name, "wb");
	if (file)
	{
		fwrite(&header, sizeof(header), 1, file);
		fwrite(image.pixels, size_of_image, 1, file);
		fclose(file);
		return 1;
	}
	else
	{
		fprintf(stderr, "[Error]: Unable to open output file.\n");
		return 0;
	}
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

internal_function f32
ExactLinearTosRGB(f32 L)
{
	if (L < 0.0f)  L = 0.0f;
	else if (L > 1.0f)  L = 1.0f;

	f32 S = L * 12.92f;
	if (L > 0.0031308f)	S = 1.055f * powf(L, 1.0f/2.4f) - 0.055f;

	return S;
}

internal_function v3
RayCast(work_queue* queue, world_state* world, v3 ray_origin, v3 ray_direction, random_series* entropy)
{
	v3  result = {};

	v3  attenuation = V3(1, 1, 1);
	f32 tolerance = 0.0001f;
	f32 minimum_distance = 0.001f;
	u32 bounce_count = 8;

	u64 bounces_processed = 0;

	for (u32 i = 0; i < bounce_count; ++i)
	{
		++bounces_processed;

		f32 closest_hit_distance = F32_MAX;

		v3 next_normal = {};
		u32 hit_material_index = 0;

		// Raycast planes
		for (u32 plane_index = 0; plane_index < world->plane_count; ++plane_index)
		{
			plane current = world->planes[plane_index];

			f32 denominator = Inner(current.normal, ray_direction);
			if ((denominator < -tolerance) || (denominator > tolerance))
			{
				f32 hit_distance = (-current.offset - Inner(current.normal, ray_origin)) / denominator;
				if (hit_distance > minimum_distance && hit_distance < closest_hit_distance)
				{
					closest_hit_distance = hit_distance;
					hit_material_index = current.material_index;

					next_normal = current.normal;
				}
			}
		}

		// Raycast spheres
		for (u32 sphere_index = 0; sphere_index < world->sphere_count; ++sphere_index)
		{
			sphere current = world->spheres[sphere_index];

			v3 spheres_relative_origin = ray_origin - current.position;
			f32 a = Inner(ray_direction, ray_direction);
			f32 b = 2.0f * Inner(ray_direction, spheres_relative_origin);
			f32 c = Inner(spheres_relative_origin, spheres_relative_origin) - current.radius*current.radius;

			f32 root_term = sqrtf(b*b - 4.0f*a*c);

			if (root_term > tolerance)
			{
				f32 t0 = (-b + root_term) / (2.0f*a);
				f32 t1 = (-b - root_term) / (2.0f*a);

				f32 hit_distance = t0;
				if (t1 > minimum_distance && t1 < t0)
				{
					hit_distance = t1;
				}

				if (hit_distance > minimum_distance && hit_distance < closest_hit_distance)
				{
					closest_hit_distance = hit_distance;
					hit_material_index = current.material_index;

					next_normal = NormalizeOrZero(spheres_relative_origin + ray_direction * hit_distance);
				}
			}
		}


		if (hit_material_index)
		{
			material hit_material = world->materials[hit_material_index];

			f32 cosine_attenuation = Inner(-ray_direction, next_normal);
			if (cosine_attenuation < 0)
			{
				cosine_attenuation = 0;
			}

			result = result + Hadamard(attenuation, hit_material.emission_color);
			attenuation = Hadamard(attenuation, hit_material.reflection_color * cosine_attenuation);

			ray_origin = ray_origin + ray_direction * closest_hit_distance;

			v3 pure_reflactance = ray_direction - next_normal * Inner(ray_direction, next_normal) * 2.0f;
			v3 random_reflectance = NormalizeOrZero(next_normal + V3(RandomBilateral(entropy), RandomBilateral(entropy), RandomBilateral(entropy)));
			ray_direction = NormalizeOrZero(Lerp(random_reflectance, hit_material.specular, pure_reflactance));
		}
		else
		{
			material hit_material = world->materials[0];

			result = result + Hadamard(attenuation, hit_material.emission_color);
			attenuation = Hadamard(attenuation, hit_material.reflection_color);
			
			break;
		}
	}

	LockedAddAndReturnPreviousValue(&queue->bounce_count, bounces_processed);

	return result;
}

internal_function u32
RenderTile(work_queue* queue)
{
	u64 work_order_index = LockedAddAndReturnPreviousValue(&queue->current_worker_index, 1);
	if (work_order_index >= queue->work_order_count)
	{
		return false;
	}

	work_order* active_work_order = queue->work_orders + work_order_index;

	world_state* world 	   = active_work_order->world;
	image_buffer_u32 image = active_work_order->image;

	u32 x_start = active_work_order->x_start;
	u32 y_start = active_work_order->y_start;
	u32 x_stop  = active_work_order->x_stop;
	u32 y_stop  = active_work_order->y_stop;

	random_series entropy = active_work_order->entropy;


	v3 camera_position  = V3(0, -10, 1);
	v3 camera_z_axis = NormalizeOrZero(camera_position - V3(0, 0, 0));
	v3 camera_x_axis = NormalizeOrZero(Cross(V3(0, 0, 1), camera_z_axis));
	v3 camera_y_axis = NormalizeOrZero(Cross(camera_z_axis, camera_x_axis));

	f32 film_distance = 1.0f;
	f32 film_width    = 1.0f;
	f32 film_height   = 1.0f;

	if (image.width > image.height)
	{
		film_height = film_width * ((f32) image.height / (f32) image.width);
	}
	else if (image.width < image.height)
	{
		film_width = film_height * ((f32) image.width / (f32) image.height);
	}

	f32 film_half_width  = film_width  * 0.5f;
	f32 film_half_height = film_height * 0.5f;
	v3  film_center = camera_position - camera_z_axis * film_distance;

	f32 half_pixel_width  = 0.5f / image.width;
	f32 half_pixel_height = 0.5f / image.height;

	u32 rays_per_pixel = 512;

	for (u32 y = y_start; y < y_stop; ++y)
	{
		u32* output = GetImagePointer(image, x_start, y);

		f32 film_y = -1.0f + 2.0f * ((f32)y / (f32)image.height);
		for (u32 x = x_start; x < x_stop; ++x)
		{
			f32 film_x = -1.0f + 2.0f * ((f32)x / (f32)image.width);

			f32 contribution = 1.0f / rays_per_pixel;
			v3 color = V3(0, 0, 0);
			for (u32 ray_index = 0; ray_index < rays_per_pixel; ++ray_index)
			{

				// Anti-aliasing
				f32 x_offset = film_x + half_pixel_width  * RandomBilateral(&entropy);
				f32 y_offset = film_y + half_pixel_height * RandomBilateral(&entropy);

				v3 film_position = film_center + camera_x_axis*film_half_width*x_offset + camera_y_axis*film_half_height*y_offset;

				v3 ray_origin = camera_position; 
				v3 ray_direction = NormalizeOrZero(film_position - camera_position);

				color = color + RayCast(queue, world, ray_origin, ray_direction, &entropy) * contribution;
			}

			*output++ = PackBGRA(V4(
				ExactLinearTosRGB(color.r) * 255,
				ExactLinearTosRGB(color.g) * 255,
				ExactLinearTosRGB(color.b) * 255,
				255.0f
			));
		}
	}

	LockedAddAndReturnPreviousValue(&queue->tiles_processed, 1);

	return true;
}




#include "macos_platform.cpp"

int main(int argument_count, char** argument_array)
{
	u32 number_of_threads = 1;
	if (argument_count == 2)
	{
		sscanf(argument_array[1], "%d", &number_of_threads);
	}


	image_buffer_u32 image = AllocateImage(1280, 720);

	material materials[7] = {};
	materials[0].emission_color   = V3(0.3f, 0.4f, 0.5f);
	materials[1].reflection_color = V3(0.5f, 0.5f, 0.5f);
	materials[2].reflection_color = V3(0.7f, 0.5f, 0.3f);
	materials[3].emission_color   = V3(4.0f, 0.0f, 0.0f);
	materials[4].reflection_color = V3(0.2f, 0.8f, 0.2f);
	materials[4].specular = 0.7f;
	materials[5].reflection_color = V3(0.4f, 0.8f, 0.9f);
	materials[5].specular = 0.85f;
	materials[6].reflection_color = V3(0.95f, 0.95f, 0.95f);
	materials[6].specular = 1.0f;

	plane planes[1] = {};
	planes[0].normal = V3(0, 0, 1);
	planes[0].offset = 0;
	planes[0].material_index = 1;

	sphere spheres[5] = {};
	spheres[0].position = V3(0, 0, 0);
	spheres[0].radius = 1.0f;
	spheres[0].material_index = 2;
	spheres[1].position = V3(3, -2, 0);
	spheres[1].radius = 1.0f;
	spheres[1].material_index = 3;
	spheres[2].position = V3(-2, -1, 2);
	spheres[2].radius = 1.0f;
	spheres[2].material_index = 4;
	spheres[3].position = V3(1, -1, 3);
	spheres[3].radius = 1.0f;
	spheres[3].material_index = 5;
	spheres[4].position = V3(-2, 3, 0);
	spheres[4].radius = 1.0f;
	spheres[4].material_index = 6;

	world_state world = {};
	world.material_count = ArrayCount(materials);
	world.materials = materials;
	world.plane_count = ArrayCount(planes);
	world.planes = planes;
	world.sphere_count = ArrayCount(spheres);
	world.spheres = spheres;

	u32 core_count   = GetCoreCount();  // Query this!
	u32 tile_width   = 64; // image.width / number_of_threads;
	u32 tile_height  = tile_width;
	u32 tile_count_x = (image.width  + tile_width  - 1) / tile_width;
	u32 tile_count_y = (image.height + tile_height - 1) / tile_height;
	u32 tile_count   = tile_count_x * tile_count_y;
	u32 processed_tiles = 0;

	printf("Configuration: %d thread(s) with %d %dx%d tiles\n", number_of_threads, tile_count, tile_width, tile_height);


	work_queue queue = {};
	queue.work_orders = (work_order*) malloc(sizeof(work_order) * tile_count);

	for (u32 tile_y = 0; tile_y < tile_count_y; ++tile_y)
	{

		u32 min_y = tile_y * tile_height;
		u32 max_y = min_y  + tile_height;

		if (max_y > image.height)  max_y = image.height;

		for (u32 tile_x = 0; tile_x < tile_count_x; ++tile_x)
		{

			u32 min_x = tile_x * tile_width;
			u32 max_x = min_x  + tile_width;

			if (max_x > image.width)  max_x = image.width;

			work_order* current_work_order = queue.work_orders + queue.work_order_count++;

			current_work_order->world   = &world;
			current_work_order->image   = image;	
			current_work_order->x_start = min_x;
			current_work_order->y_start = min_y;
			current_work_order->x_stop  = max_x;
			current_work_order->y_stop  = max_y;
			current_work_order->entropy.state = tile_x * 25422 + tile_y * 516502;
		}
	}

	assert(queue.work_order_count == tile_count);
	LockedAddAndReturnPreviousValue(&queue.current_worker_index, 0);

	clock_t start_time = clock();

	for (u32 i = 1; i < number_of_threads; ++i)
	{
		CreateRenderTileThread(&queue);
	}

	while (queue.tiles_processed < tile_count)
	{
		RenderTile(&queue);
		printf("\rRaycasting %d %%  ", u32((100.0f * queue.tiles_processed) / f32(tile_count)));
		fflush(stdout);
	}

	clock_t stop_time  = clock();
	clock_t total_time = stop_time - start_time;

	printf("\rRaycasting completed!    \n");
	printf("Total time: %dms\n", (u32)total_time);
	printf("Total bounces: %llu\n", queue.bounce_count);
	printf("Time per bounce: %f\n", (f32)total_time / queue.bounce_count);

	WriteImage(image, "output.bmp");

	return 0;
}


/*
Configuration: 1 thread(s) with 240 64x64 tiles
Raycasting 100 %
Complete!
Total time: 29121ms
Total bounces: 28431859
Time per bounce: 0.001024

Configuration: 8 thread(s) with 240 64x64 tiles
Raycasting completed!
Total time: 5390ms
Total bounces: 28436151
Time per bounce: 0.000190

Configuration: 16 thread(s) with 240 64x64 tiles
Raycasting 100 %
Complete!
Total time: 5465ms
Total bounces: 28426167
Time per bounce: 0.000192


---- Using rand ----
Configuration: 16 thread(s) with 240 64x64 tiles
Raycasting completed!
Total time: 1048ms
Total bounces: 28428707
Time per bounce: 0.000037

---- Using XOR-shift ----
Configuration: 16 thread(s) with 240 64x64 tiles
Raycasting completed!
Total time: 1035ms
Total bounces: 28429027
Time per bounce: 0.000036

*/

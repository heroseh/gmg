
#include "das.h"
#include "das.c"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GMG_ENABLE_VULKAN
#define GMG_ENABLE_XLIB
#define GMG_ENABLE_XCB
#define GMG_ENABLE_WAYLAND
#include "../../gmg.h"
#include "../../gmg.c"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_syswm.h>

typedef struct AppVec2 AppVec2;
struct AppVec2 {
	float x, y;
};
#define AppVec2_init(x, y) ((AppVec2) { x, y })

typedef struct AppVec3 AppVec3;
struct AppVec3 {
	float x, y, z;
};
#define AppVec3_init(x, y, z) ((AppVec3) { x, y , z })

AppVec3 AppVec3_add(AppVec3 a, AppVec3 b) { return AppVec3_init(a.x + b.x, a.y + b.y, a.z + b.z); }
AppVec3 AppVec3_mul(AppVec3 a, AppVec3 b) { return AppVec3_init(a.x * b.x, a.y * b.y, a.z * b.z); }
AppVec3 AppVec3_mul_scalar(AppVec3 v, float s) { return AppVec3_init(v.x * s, v.y * s, v.z * s); }
AppVec3 AppVec3_norm(AppVec3 v) {
	float k = 1.0 / sqrtf((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
	return AppVec3_init(v.x * k, v.y * k, v.z * k);
}
float AppVec3_dot(AppVec3 a, AppVec3 b) {
	float p = 0.0;
	p += a.x * b.x;
	p += a.y * b.y;
	p += a.z * b.z;
	return p;
}
AppVec3 AppVec3_mul_cross(AppVec3 a, AppVec3 b) {
	return AppVec3_init(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}
AppVec3 AppVec3_rotate(AppVec3 point, AppVec3 rotation_vector, float angle) {
	AppVec3 rotation_vector_norm = AppVec3_norm(rotation_vector);
	float cos_angle = cosf(angle);
	float dot = AppVec3_dot(rotation_vector_norm, point);
	AppVec3 cross = AppVec3_mul_cross(rotation_vector_norm, point);
	float comp = (1.0f - cos_angle) * dot;
	AppVec3 vec =
		AppVec3_add(
			AppVec3_add(
				AppVec3_mul_scalar(rotation_vector_norm, comp),
				AppVec3_mul_scalar(point, cos_angle)
			),
			AppVec3_mul_scalar(cross, sinf(angle))
		);
	return vec;
}



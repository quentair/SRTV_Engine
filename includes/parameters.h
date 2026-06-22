#pragma once

#include <stdint.h>

// radius distance for which chunk we can see around the camera
constexpr int MAX_VIEW_DISTANCE = 20;
constexpr uint32_t BASE_VIEW_DISTANCE = 6;

// chunk distance after which we display 2x2x2 voxel chunks instead of full 8x8x8 chunks
constexpr uint32_t BASE_LOD_DISTANCE_2x2x2 = 3;
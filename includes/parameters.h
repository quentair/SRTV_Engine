#pragma once

#include <stdint.h>

// radius distance for which chunk we can see around the camera
constexpr uint32_t VIEWDISTANCE = 4;

// chunk distance after which we display 2x2x2 voxel chunks instead of full 8x8x8 chunks
constexpr uint32_t LODDISTANCE_2x2x2 = 3;
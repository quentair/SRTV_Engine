#pragma once

#include "brick_chunk.h"

#include <glm/glm.hpp>

#include <memory>

constexpr uint8_t REGION_SIZE_XZ = 32; // 32*Y*32 chunks into our region

constexpr uint8_t REGION_SIZE_Y = 4; // X*4*Z chunks into our region

constexpr uint16_t REGION_VOXEL_RESOLUTION_XZ = CHUNK_SIZE * BRICKMAP_SIZE * REGION_SIZE_XZ; // number of voxels in X or Z dimension in our region

constexpr uint16_t REGION_VOXEL_RESOLUTION_Y = CHUNK_SIZE * BRICKMAP_SIZE * REGION_SIZE_Y; // number of voxels in Y dimension in our region

namespace srtv_engine::worldgen {

// x : horizontal axis ; y : vertical axis ; z : depth axis
class WorldRegion {
public:
	std::vector<std::unique_ptr<BrickChunk>> _chunks;

	glm::ivec3 _worldPos = glm::ivec3{};

	glm::ivec3 indexToLocalPos(int index) const;

	glm::ivec3 relativeWorldPosToChunkGridPosition(float x, float y, float z) const;

	uint32_t localPosToIndex(uint8_t x, uint8_t y, uint8_t z) const;

	void init(int x, int y, int z) {
		const uint32_t regionCount = REGION_SIZE_XZ * REGION_SIZE_Y * REGION_SIZE_XZ;
		_chunks.resize(regionCount);
		_worldPos = glm::ivec3(x * REGION_VOXEL_RESOLUTION_XZ, y * REGION_VOXEL_RESOLUTION_Y, z * REGION_VOXEL_RESOLUTION_XZ);
	}

	void generate(std::atomic<bool>& stopGeneration);
};

} // namespace srtv_engine::worldgen
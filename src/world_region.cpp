#include "world_region.h"

namespace srtv_engine::worldgen {

glm::ivec3 WorldRegion::indexToLocalPos(int index) const
{
	// return the region local grid position of the chunk at given index
	// because we operate in world pos, input index can be negative (so the chunk is out of the region, so the returned local position is negative too)
	int x = index % REGION_SIZE_XZ;
	int z = index / REGION_SIZE_XZ % REGION_SIZE_XZ;
	int y = index / REGION_SIZE_XZ / REGION_SIZE_XZ;

	return glm::ivec3(x, y, z);
}

int WorldRegion::worldPosToIndex(float x, float y, float z) const
{
	// return the index of the chunk at given world position
	// because we operate in world pos, returned index can be negative (so the sampled position is out of the region)
	// note : we floor the division id the position is negative, otherwise, x position -2.9 will be rounded to -2 instead of -3 (so it makes us skip a chunk that should be rendered, and the error could propagate) 

	return floor(x / CHUNK_VOXEL_RESOLUTION) + floor(z / float(CHUNK_VOXEL_RESOLUTION)) * REGION_SIZE_XZ + floor(y / float(CHUNK_VOXEL_RESOLUTION)) * REGION_SIZE_XZ * REGION_SIZE_XZ;
}

uint32_t WorldRegion::localPosToIndex(uint8_t x, uint8_t y, uint8_t z) const
{
	// x = (0,CHUNK_SIZE - 1) ; y = (0,CHUNK_SIZE - 1) , z = (0,CHUNK_SIZE - 1) for CHUNK_SIZE^3 brickmaps chunk (indexed from 0 to CHUNK_SIZE^3 - 1)

	// return the index of the chunk at given local grid position
	return x + z * REGION_SIZE_XZ + y * REGION_SIZE_XZ * REGION_SIZE_XZ;
}
	
void WorldRegion::generate()
{
	const uint32_t regionCount = REGION_SIZE_XZ * REGION_SIZE_Y * REGION_SIZE_XZ;
	_chunks.resize(regionCount);

	// TODO multi-threaded generation

	for (int i = 0; i < regionCount; i++) {
		_chunks[i] = std::make_unique<BrickChunk>();
		glm::ivec3 pos = indexToLocalPos(i);
		_chunks[i]->generate(pos.x, pos.y, pos.z);
	}
}

} // namespace srtv_engine::worldgen
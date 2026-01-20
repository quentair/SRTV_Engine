#include "world_region.h"

namespace srtv_engine::worldgen {

glm::ivec3 WorldRegion::indexToLocalPos(int index) const
{
	// return the region's chunks grid position of the chunk at given index (index must be between 0 and the maximum number of chunks in the region)
	int xGrid = index % REGION_SIZE_XZ;
	int zGrid = index / REGION_SIZE_XZ % REGION_SIZE_XZ;
	int yGrid = index / REGION_SIZE_XZ / REGION_SIZE_XZ;

	return glm::ivec3(xGrid, yGrid, zGrid);
}

glm::ivec3 WorldRegion::relativeWorldPosToChunkGridPosition(float x, float y, float z) const
{
	// return the region's chunks grid position of a certain chunk
	// the chunk is situated at given relative world position
	// because we operate in world position, returned grid positions can be negative (so the sampled chunk is out of the region)
	// note : we floor the division, otherwise, x position -2.9 will be rounded to -2 instead of -3 (so it makes us skip a chunk that should be rendered, and the error could propagate)

	return glm::ivec3(floor(x / CHUNK_VOXEL_RESOLUTION), floor(y / CHUNK_VOXEL_RESOLUTION), floor(z / CHUNK_VOXEL_RESOLUTION));
}

uint32_t WorldRegion::localPosToIndex(uint8_t x, uint8_t y, uint8_t z) const
{
	// x = (0,REGION_SIZE_XZ - 1) ; y = (0,REGION_SIZE_Y - 1) , z = (0,REGION_SIZE_XZ - 1) for REGION_SIZE_XZ*REGION_SIZE_XZ*REGION_SIZE_Y chunks region (indexed from 0 to REGION_SIZE_XZ*REGION_SIZE_XZ*REGION_SIZE_Y - 1)

	// return the index of the chunk at given region's chunks grid position
	return x + z * REGION_SIZE_XZ + y * REGION_SIZE_XZ * REGION_SIZE_XZ;
}
	
void WorldRegion::generate(std::atomic<bool>& stopGeneration)
{
	// generate all the chunks inside the region
	for (int y = REGION_SIZE_Y-1; y >= 0; y--) {
		for (int x = 0; x < REGION_SIZE_XZ; x++) {
			for (int z = 0; z < REGION_SIZE_XZ; z++) {
				if (stopGeneration.load()) {
					return;
				}
				const uint32_t index = localPosToIndex(x, y, z);
				_chunks[index] = std::make_unique<BrickChunk>();
				glm::ivec3 chunkWorldPos = glm::ivec3(_worldPos.x + x * CHUNK_VOXEL_RESOLUTION, _worldPos.y + y * CHUNK_VOXEL_RESOLUTION, _worldPos.z + z * CHUNK_VOXEL_RESOLUTION);
				_chunks[index]->generate(chunkWorldPos.x, chunkWorldPos.y, chunkWorldPos.z);
			}
		}
	}
}

} // namespace srtv_engine::worldgen
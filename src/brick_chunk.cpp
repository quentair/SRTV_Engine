#include "brick_chunk.h"

#include <random>

namespace srtv_engine::worldgen {

uint32_t BrickChunk::posToIndex(uint8_t x, uint8_t y, uint8_t z) const
{
	// x = (0,CHUNK_SIZE - 1) ; y = (0,CHUNK_SIZE - 1) , z = (0,CHUNK_SIZE - 1) for CHUNK_SIZE^3 brickmaps chunk (indexed from 0 to CHUNK_SIZE^3 - 1)

	// return the index of the brickmap at given chunk's brickmaps grid position (so range between 0 and 4095)
	return x + z * CHUNK_SIZE + y * CHUNK_SIZE * CHUNK_SIZE;
}

uint32_t BrickChunk::getBrickmapState(uint32_t index) const
{
	// returns if the brickmap at given indices inside our chunk is loaded, unloaded or requested by GPU

	uint32_t stateMask = 0b11100000000000000000000000000000;

	return index & stateMask;
}

void BrickChunk::generate(uint16_t x, uint16_t y, uint16_t z)
{
	// generate a chunk made of brickmaps at given world position

	// for each column of voxel inside our chunk, store the height of the column
	// so it describes the shape of our terrain
	std::vector<float> heightValues;
	int nVoxelsInChunk = CHUNK_SIZE * BRICKMAP_SIZE * CHUNK_SIZE * BRICKMAP_SIZE;
	heightValues.reserve(nVoxelsInChunk);
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> dist(127.0, 200.0);
	for (int i = 0; i < nVoxelsInChunk; ++i)
		heightValues.push_back(dist(mt));

	for (uint8_t chunkX = 0; chunkX < CHUNK_SIZE; chunkX++) {
		for (uint8_t chunkY = 0; chunkY < CHUNK_SIZE; chunkY++) {
			for (uint8_t chunkZ = 0; chunkZ < CHUNK_SIZE; chunkZ++) {
				Brickmap brick{};

				bool generated = false;

				// 4x4x4 voxel brick lod (needs 64 bits)
				//uint32_t lod_4x4x4 = 0;
				// 2x2x2 voxel brick lod (needs 8 bits)
				uint32_t lod_2x2x2 = 0;

				// generate voxels inside brickmap with world pos and noise or whatever else
				////////////////////////////////////
				for (uint8_t brickmapX = 0; brickmapX < BRICKMAP_SIZE; brickmapX++) {
					for (uint8_t brickmapZ = 0; brickmapZ < BRICKMAP_SIZE; brickmapZ++) {

						float height = heightValues[(chunkX * BRICKMAP_SIZE + brickmapX) + ((chunkZ * BRICKMAP_SIZE + brickmapZ) * CHUNK_VOXEL_RESOLUTION)];
						
						for (uint8_t brickmapY = 0; brickmapY < BRICKMAP_SIZE; brickmapY++) {
							// generate only if there are voxels to generate in the brickmap (this means the random height reached this brickmap lowest xz plan)
							if (y + chunkY * BRICKMAP_SIZE + brickmapY < height) {
								brick.notifyVoxelPresence(brickmapX, brickmapY, brickmapZ);
								generated = true;
								//lod_4x4x4 |= 1 << (((brickmapX & 0b110) >> 1) + ((brickmapZ & 0b110) << 1) + ((brickmapY & 0b110) << 3));
								lod_2x2x2 |= 1 << (((brickmapX & 0b100) >> 2) + ((brickmapZ & 0b100) >> 1) + (brickmapY & 0b100));
							}
						}
					}

				}

				// add brickmap to our chunk and declare it as loaded
				if (generated) {
					_brickmaps.push_back(brick);
					_indices[posToIndex(chunkX, chunkY, chunkZ)] = uint32_t(_brickmaps.size() - 1) | BRICKMAP_LOADED_BIT | (lod_2x2x2 << 12);
				}
			}
		}
	}

	// declare chunk as generated
	_generated = true;
}

} // namespace srtv_engine::worldgen
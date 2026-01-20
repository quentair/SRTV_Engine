#pragma once

#include "brickmap.h"

#include <glm/glm.hpp>

#include <vector>
#include <stdint.h>

constexpr uint8_t CHUNK_SIZE = 16; // 16*16*16 bricks into our chunk (4096 indices takes 12 bits)

constexpr uint8_t CHUNK_VOXEL_RESOLUTION = CHUNK_SIZE * BRICKMAP_SIZE; // number of voxels per dimensions in our chunk

constexpr  uint32_t BRICKMAP_LOADED_BIT = 0b00100000000000000000000000000000;

constexpr  uint32_t BRICKMAP_UNLOADED_BIT = 0b01000000000000000000000000000000;

constexpr  uint32_t BRICKMAP_REQUESTED_BIT = 0b10000000000000000000000000000000;

constexpr  uint32_t BRICKMAP_INDEX_BITS = 0b00000000000000000000111111111111; // right most 12 bits are for brickmap index in the chunk (from 0 to 4095)

constexpr  uint32_t BRICKMAP_LOD_BITS = 0b00000000000011111111000000000000;

constexpr int INITIAL_GPU_BRICKMAPS_NUMBER = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE; // initial number of brickmaps allocated for a chunk in the GPU (1 area worth of brickmaps here)

namespace srtv_engine::worldgen {

// x : horizontal axis ; y : vertical axis ; z : depth axis
class BrickChunk {

  public:
	std::vector<Brickmap> _brickmaps;
	// instead of pointers to brickmaps, use uint32 to pack together the brickmap index inside the chunk, load state and LOD
	std::vector<uint32_t> _indices = std::vector<uint32_t>(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE); // 4096 indices takes 12 bits, so the leftmost 20 bits can be used for something else 

	// pointer to allocated GPU data of the chunk
	//std::array<Brickmap*, INITIAL_GPU_BRICKMAPS_NUMBER> _gpuChunkData{};
	//std::array<uint32_t*, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE> _gpuIndices;

	// GPU data size for the chunk
	//int _gpuBrickmapsCount = INITIAL_GPU_BRICKMAPS_NUMBER; // number of brickmaps we can send to the GPU (start with one surface worth of brickmaps)
	//int _gpuHighestBrickmapNumber = 0; // highest encountered number of brickmaps to load for the GPU

	uint32_t posToIndex(uint8_t x, uint8_t y, uint8_t z) const;

	uint32_t getBrickmapState(uint32_t index) const;

	void generate(uint16_t x, uint16_t y, uint16_t z);

	bool isGenerated() const {
		return _generated;
	}

  private :
	  bool _generated = false;
};

} // namespace srtv_engine::worldgen
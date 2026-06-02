#pragma once

#include "block_type.h"

#include <glm/glm.hpp>

#include <array>
#include <stdint.h>

constexpr uint8_t BRICKMAP_SIZE = 8; // 8*8*8 voxels into our brickmap

constexpr uint8_t N_BITS_UINT32 = sizeof(uint32_t) * 8;

constexpr uint8_t N_SUMBASKS = (BRICKMAP_SIZE*BRICKMAP_SIZE*BRICKMAP_SIZE) / (N_BITS_UINT32); // number of 32 bits masks we need to represent BRICKMAP_SIZE^3 bitmask for voxels presence (our solid mask)

constexpr uint8_t N_PALETTE_ENTRIES = BRICKMAP_SIZE * BRICKMAP_SIZE * BRICKMAP_SIZE / 4;

namespace srtv_engine::worldgen {

// x : horizontal axis ; y : vertical axis ; z : depth axis
class Brickmap {

public: 
	std::array<uint32_t, N_SUMBASKS> _presenceMask; // BRICKMAP_SIZE^3 bitmask for voxels presence in the brickmap, divided as 32 bits submasks (voxel 0 to 31 are in the first submask at index 0, and so on)

	std::array<uint32_t, N_PALETTE_ENTRIES> _blockPalette; // store an index for each voxels of the brickmap, this index is matched to a block type that is present in the block type atlas (e.g : voxel number 3 is an air block, so _blockPalette[3] = 1) (0 to 255 block types so 8 bits per voxel, the data is 32 bits for GPU alignement so we have 4 voxels per index)

	uint8_t posToIndexInSubmask(uint8_t x, uint8_t y, uint8_t z) const;

	uint8_t getSubmaskIndex(uint8_t x, uint8_t y, uint8_t z) const;

	uint32_t indexToSubmask(uint8_t index) const;

	uint32_t posToSubmask(uint8_t x, uint8_t y, uint8_t z) const;

	uint32_t& getPresenceSubmask(uint8_t submaskIndex);

	uint32_t& getPresenceSubmask(uint8_t x, uint8_t y, uint8_t z);

	void notifyVoxelPresence(uint8_t x, uint8_t y, uint8_t z);

	void setBlockType(uint8_t x, uint8_t y, uint8_t z, uint8_t typeId);

	uint8_t getBlockType(uint8_t x, uint8_t y, uint8_t z);

};

} // namespace srtv_engine::worldgen
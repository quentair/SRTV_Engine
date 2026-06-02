#include "brickmap.h"

namespace srtv_engine::worldgen {

uint8_t Brickmap::posToIndexInSubmask(uint8_t x, uint8_t y, uint8_t z) const
{
	// x = (0,BRICKMAP_SIZE - 1) ; y = (0,BRICKMAP_SIZE - 1) , z = (0,BRICKMAP_SIZE - 1) for BRICKMAP_SIZE^3 voxel brickmap (indexed from 0 to BRICKMAP_SIZE^3 - 1)
	// we divide the BRICKMAP_SIZE^3 bits presence mask into buckets of 32 bits voxels presence submasks

	// return the index of a voxel inside his SUBMASK (so range between 0 and 31)
	// this voxel is situated at given brickmap's voxels grid position
	return ((x + z * BRICKMAP_SIZE + y * BRICKMAP_SIZE * BRICKMAP_SIZE) % (N_BITS_UINT32));
}

uint8_t Brickmap::getSubmaskIndex(uint8_t x, uint8_t y, uint8_t z) const
{
	// x = (0,BRICKMAP_SIZE - 1) ; y = (0,BRICKMAP_SIZE - 1) , z = (0,BRICKMAP_SIZE - 1) for BRICKMAP_SIZE^3 voxel brickmap (indexed from 0 to BRICKMAP_SIZE^3 - 1)
	// we divide the BRICKMAP_SIZE^3 bits presence mask into buckets of 32 bits voxels presence submasks

	// return the index of the submask a voxel belongs to (so range between 0 and N_SUMBASKS)
	// this voxel is situated at given brickmap's voxels grid position
	return ((x + z * BRICKMAP_SIZE + y * BRICKMAP_SIZE * BRICKMAP_SIZE) / (N_BITS_UINT32));
}

uint32_t Brickmap::indexToSubmask(uint8_t index) const
{
	// return the presence submask of 1 voxel at given index
	// for example, voxel at index 3 has a submask of (uint32_t)1000
	return ((uint32_t)1 << index);
}

uint32_t Brickmap::posToSubmask(uint8_t x, uint8_t y, uint8_t z) const
{
	// return the presence submask of 1 voxel 
	// this voxel is situated at given brickmap's voxels grid position
	// for example, (2, 0, 0) voxel has a submask of (uint32_t)100
	return indexToSubmask(posToIndexInSubmask(x, y, z));
}

uint32_t& Brickmap::getPresenceSubmask(uint8_t submaskIndex)
{
	// return one of the 16 presence submask
	return _presenceMask[submaskIndex];
}

uint32_t& Brickmap::getPresenceSubmask(uint8_t x, uint8_t y, uint8_t z)
{
	// return the presence submask where a voxel should belong to
	// this voxel is situated at given brickmap's voxels grid position
	// for example, (0, 0, 0) voxel is assigned to the first submask
	return getPresenceSubmask(getSubmaskIndex(x, y, z));
}

void Brickmap::notifyVoxelPresence(uint8_t x, uint8_t y, uint8_t z)
{
	// add the voxel presence submask of 1 voxel into its associated submasks
	// this voxel is situated at given brickmap's voxels grid position
	// so it is notified as present in the brickmap
	getPresenceSubmask(x, y, z) |= posToSubmask(x, y, z);
}

void Brickmap::setBlockType(uint8_t x, uint8_t y, uint8_t z, uint8_t typeId)
{
	// set the block type of a voxel at specified index

	uint8_t paletteIndex = (x + z * BRICKMAP_SIZE + y * BRICKMAP_SIZE * BRICKMAP_SIZE) / 4; // because we need int32 for the GPU data but the palette format is int8 (256 block types), this means we can map up to 4 voxels per index (4 * 8 = 32)
	uint8_t offset = (x + z * BRICKMAP_SIZE + y * BRICKMAP_SIZE * BRICKMAP_SIZE) % 4 * 8; // e.g, voxel at subindex 1 has an offset of 8 bits (bits 8 to 15), block at subindex 2 an offset of 16 bits (bits 16 to 23)...
	_blockPalette[paletteIndex] |= typeId << offset;

}

uint8_t Brickmap::getBlockType(uint8_t x, uint8_t y, uint8_t z)
{
	// set the block type of a voxel at specified index
	uint8_t paletteIndex = (x + z * BRICKMAP_SIZE + y * BRICKMAP_SIZE * BRICKMAP_SIZE) / 4; // because we need int32 for the GPU data but the palette format is int8 (256 block types), this means we can map up to 4 voxels per index (4 * 8 = 32)
	uint8_t offset = (x + z * BRICKMAP_SIZE + y * BRICKMAP_SIZE * BRICKMAP_SIZE) % 4 * 8; // e.g, voxel at subindex 1 has an offset of 8 bits (bits 8 to 15), block at subindex 2 an offset of 16 bits (bits 16 to 23)...
	return (uint8_t)(_blockPalette[paletteIndex] >> offset); // return first 8 bits
}

} // namespace srtv_engine::worldgen
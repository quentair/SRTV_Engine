#include "brickmap.h"

namespace srtv_engine::worldgen {

uint8_t Brickmap::posToIndex(uint8_t x, uint8_t y, uint8_t z) const
{
	// x = (0,BRICKMAP_SIZE - 1) ; y = (0,BRICKMAP_SIZE - 1) , z = (0,BRICKMAP_SIZE - 1) for BRICKMAP_SIZE^3 voxel brickmap (indexed from 0 to BRICKMAP_SIZE^3 - 1)
	// we divide the BRICKMAP_SIZE^3 bits presence mask into buckets of 32 bits voxels presence submasks

	// return the index of a voxel inside his submask (so range between 0 and 31)
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
	return indexToSubmask(posToIndex(x, y, z));
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

} // namespace srtv_engine::worldgen
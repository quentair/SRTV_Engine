#include "world_generator.h"

#include <algorithm>

namespace srtv_engine::worldgen {

void WorldGenerator::generateRegions(std::vector<WorldRegion*>& regions, glm::vec3 playerWorldPos, std::atomic<bool>& stopGeneration)
{
    // generate all chunks in given registered regions that are in the player generation range
    for (auto& r : regions) {
        if (r == nullptr) {
            continue;
        }
        if (stopGeneration.load()) {
            return;
        }
        generateRegion(*r, playerWorldPos, stopGeneration);
    }
}

void WorldGenerator::generateRegion(WorldRegion& region, glm::vec3 playerWorldPos, std::atomic<bool>& stopGeneration)
{
    // Gernerate chunks of a certain region around player position
    // view distance and pre-load radius defines which chunks are generated

    glm::ivec3 regionPosition = region._worldPos;

    // compute view distance area corners in world coordinates, in relation to the region world position
    glm::vec3 viegGridBottomLeftCornerCoordinates = glm::vec3(floor(playerWorldPos.x) - GENERATE_DISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.x, 0, floor(playerWorldPos.z) - GENERATE_DISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.z);
    glm::vec3 viewGridUpperRightCornerCoordinates = glm::vec3(floor(playerWorldPos.x) + GENERATE_DISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.x, 0, floor(playerWorldPos.z) + GENERATE_DISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.z);

    // convert corner positions to chunks grid coordinates (y axis is defaulted because viewdistance is a 2D grid on XZ plane)
    glm::ivec3 viegGridBottomLeftCorner = region.relativeWorldPosToChunkGridPosition(viegGridBottomLeftCornerCoordinates.x, 0, viegGridBottomLeftCornerCoordinates.z);
    glm::ivec3 viewGridUpperRightCorner = region.relativeWorldPosToChunkGridPosition(viewGridUpperRightCornerCoordinates.x, 0, viewGridUpperRightCornerCoordinates.z);

    // return if we are out of region for both corners
    if (viegGridBottomLeftCorner.x > REGION_SIZE_XZ) {
        return;
    }
    else if (viewGridUpperRightCorner.x < 0) {
        return;
    }
    else if (viegGridBottomLeftCorner.z > REGION_SIZE_XZ) {
        return;
    }
    else if (viewGridUpperRightCorner.z < 0) {
        return;
    }

    // limit grid coordinates to sample between [0, 0, 0] and [REGION_SIZE_XZ - 1, 0, REGION_SIZE_XZ - 1]
    glm::ivec3 chunkGridBottomLeftCorner = glm::ivec3( std::clamp(viegGridBottomLeftCorner.x, 0, REGION_SIZE_XZ - 1), 0, std::clamp(viegGridBottomLeftCorner.z, 0, REGION_SIZE_XZ - 1) );
    glm::ivec3 chungGirdUpperRightCorner = glm::ivec3( std::clamp(viewGridUpperRightCorner.x, 0, REGION_SIZE_XZ - 1), 0, std::clamp(viewGridUpperRightCorner.z, 0, REGION_SIZE_XZ - 1) );

    // load all chunks of the region inside generation distance
    for (int yRegion = REGION_SIZE_Y - 1; yRegion >= 0; yRegion--) {
        for (int xRegion = chunkGridBottomLeftCorner.x; xRegion <= chungGirdUpperRightCorner.x; xRegion++) {
            for (int zRegion = chunkGridBottomLeftCorner.z; zRegion <= chungGirdUpperRightCorner.z; zRegion++) {

                if (stopGeneration.load()) {
                    return;
                }

                const uint32_t index = region.localPosToIndex(xRegion, yRegion, zRegion);

                // if chunk is not created, create and generate it
                if (region._chunks[index].get() == nullptr) {
                    region._chunks[index] = std::make_unique<BrickChunk>();
                    glm::ivec3 chunkWorldPos = glm::ivec3(region._worldPos.x + xRegion * CHUNK_VOXEL_RESOLUTION, region._worldPos.y + yRegion * CHUNK_VOXEL_RESOLUTION, region._worldPos.z + zRegion * CHUNK_VOXEL_RESOLUTION);
                    region._chunks[index]->generate(chunkWorldPos.x, chunkWorldPos.y, chunkWorldPos.z);
                }
            }
        }
    }
}

} // namespace srtv_engine::worldgen
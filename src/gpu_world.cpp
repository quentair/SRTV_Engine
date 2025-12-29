#include "gpu_world.h"

namespace srtv_engine::worldgen {

    bool GpuWorld::loadChunks(WorldRegion& region, glm::vec3 playerWorldPos)
    {
        // Prepare chunk content for GPU
        // at first feedback loop, all chunks are not loaded to GPU, the GPU request those needed afterward
        // view distance grid send to GPU is ogranised into column of chunks

        // compute view distance area corners in world position
        glm::vec3 viegGridBottomLeftCornerCoordinates = glm::vec3(floor(playerWorldPos.x) - VIEWDISTANCE * CHUNK_VOXEL_RESOLUTION, 0, floor(playerWorldPos.z) - VIEWDISTANCE * CHUNK_VOXEL_RESOLUTION);
        glm::vec3 viewGridUpperRightCornerCoordinates = glm::vec3(floor(playerWorldPos.x) + VIEWDISTANCE * CHUNK_VOXEL_RESOLUTION, 0, floor(playerWorldPos.z) + VIEWDISTANCE * CHUNK_VOXEL_RESOLUTION);

        // convert corner positions to chunks grid coordinates (y axis is defaulted because viewdistance is a 2D grid on XZ plane)
        glm::ivec3 viegGridBottomLeftCorner = region.indexToLocalPos(region.worldPosToIndex(viegGridBottomLeftCornerCoordinates.x, 0, viegGridBottomLeftCornerCoordinates.z));
        glm::ivec3 viewGridUpperRightCorner = region.indexToLocalPos(region.worldPosToIndex(viewGridUpperRightCornerCoordinates.x, 0, viewGridUpperRightCornerCoordinates.z));

        // return if we are out of region for both corners
        if (viegGridBottomLeftCorner.x > REGION_SIZE_XZ) {
            return false;
        }
        else if (viewGridUpperRightCorner.x < 0) {
            return false;
        }
        else if (viegGridBottomLeftCorner.z > REGION_SIZE_XZ) {
            return false;
        }
        else if (viewGridUpperRightCorner.z < 0) {
            return false;
        }

        // limit grid coordinates to sample between [0, 0, 0] and [REGION_SIZE_XZ - 1, 0, REGION_SIZE_XZ - 1]
        glm::ivec3 chunkGridBottomLeftCorner = glm::ivec3( std::min(std::max(viegGridBottomLeftCorner.x, 0), REGION_SIZE_XZ - 1), 0, std::min(std::max(viegGridBottomLeftCorner.z, 0), REGION_SIZE_XZ - 1) );
        glm::ivec3 chungGirdUpperRightCorner = glm::ivec3( std::min(std::max(viewGridUpperRightCorner.x, 0), REGION_SIZE_XZ - 1), 0, std::min(std::max(viewGridUpperRightCorner.z, 0), REGION_SIZE_XZ - 1) );

        // temporary chunk pointer and chunk data vector to displace them instead of reloading them if they are still in the grid but moved from position (cell)
        std::vector<std::pair<BrickChunk*, ChunkGpuData>> tempChunks;

        // fill the temporary buffer 
        // do this before actual reload algorithm because the load order depends on player displacement (if player goes backward, back row to front row reload is needed, if player goes frontward, front row to top row reload is needed, same with columns if player goes left or right)
        // so entirely filling it before will get rid of this at the cost of perdormances when reload is needed (so we just need a basic all around back row to front row reload)
        for (int xRegion = chunkGridBottomLeftCorner.x; xRegion <= chungGirdUpperRightCorner.x; xRegion++) {
            for (int zRegion = chunkGridBottomLeftCorner.z; zRegion <= chungGirdUpperRightCorner.z; zRegion++) {

                // convert chunk grid position to view grid position
                glm::ivec2 viewGridPos = glm::ivec2(chunkGridBottomLeftCorner.x - viegGridBottomLeftCorner.x + xRegion - chunkGridBottomLeftCorner.x, chunkGridBottomLeftCorner.z - viegGridBottomLeftCorner.z + zRegion - chunkGridBottomLeftCorner.z);
                int viewGridIndex = viewGridPos.x + viewGridPos.y * (VIEWDISTANCE * 2 + 1);

                for (int yRegion = 0; yRegion < REGION_SIZE_Y; yRegion++) {

                    // retrieve chunk data from region
                    int index = region.localPosToIndex(xRegion, yRegion, zRegion);
                    BrickChunk* chunk = region._chunks[index].get();

                    // get chunk data in the chunk column given its y position
                    ChunkGpuData* chunkData = &_chunksDataColumns[viewGridIndex]._chunksInColumn[yRegion];

                    // check if the chunk was already loaded in the gpu at the right position, if so, no need to save it for hot-reload
                    int loadedMapIndex = viewGridIndex + yRegion * (VIEWDISTANCE * 2 + 1) * (VIEWDISTANCE * 2 + 1);
                    if (_gpuLoadedChunks.at(loadedMapIndex) == chunk) {
                        continue;
                    }

                    // save gpu chunk and its data for possible hot-reload
                    if (_gpuLoadedChunks.at(loadedMapIndex) != nullptr)
                    {
                        tempChunks.push_back(std::make_pair(_gpuLoadedChunks.at(loadedMapIndex), *chunkData));
                    }
                }
            }
        }

        // generate all chunks of the region inside view distance
        for (int xRegion = chunkGridBottomLeftCorner.x; xRegion <= chungGirdUpperRightCorner.x; xRegion++) {
            for (int zRegion = chunkGridBottomLeftCorner.z; zRegion <= chungGirdUpperRightCorner.z; zRegion++) {

                // convert chunk grid position to view grid position
                glm::ivec2 viewGridPos = glm::ivec2(chunkGridBottomLeftCorner.x - viegGridBottomLeftCorner.x + xRegion - chunkGridBottomLeftCorner.x, chunkGridBottomLeftCorner.z - viegGridBottomLeftCorner.z + zRegion - chunkGridBottomLeftCorner.z);
                int viewGridIndex = viewGridPos.x + viewGridPos.y * (VIEWDISTANCE * 2 + 1);

                _viewDistanceGrid[viewGridIndex] = 1; // indicates chunks presence in the world, we must then retrieve the data

                for (int yRegion = 0; yRegion < REGION_SIZE_Y; yRegion++) {

                    // retrieve chunk data from region
                    int index = region.localPosToIndex(xRegion, yRegion, zRegion);
                    BrickChunk* chunk = region._chunks[index].get();

                    // get chunk data in the chunk column given its y position
                    ChunkGpuData* chunkData = &_chunksDataColumns[viewGridIndex]._chunksInColumn[yRegion];

                    // check if the chunk was already loaded in the gpu at the right position, if so, no need to check for hot-reload or replace it
                    int loadedMapIndex = viewGridIndex + yRegion * (VIEWDISTANCE * 2 + 1) * (VIEWDISTANCE * 2 + 1);
                    if (_gpuLoadedChunks.at(loadedMapIndex) == chunk) {
                        continue;
                    }

                    // save chunk pointer and mark it as loaded in the GPU
                    _gpuLoadedChunks.at(loadedMapIndex) = chunk;

                    // if the chunk was an already loaded chunk but changed from position (cell), just copy back the content
                    bool copied = false;
                    for (int i = 0; i < tempChunks.size(); i++) {
                        if (tempChunks[i].first == chunk) {
                            chunkData->_brickmapsData = tempChunks[i].second._brickmapsData;
                            chunkData->_brickmaps = tempChunks[i].second._brickmaps;
                            copied = true;
                            break;
                        }
                    }
                    if (copied)
                        continue;

                    for (int i = 0; i < chunk->_indices.size(); i++) {
                        // if brickmap inside the chunk has been generated, just load the LOD and mark it as unloaded for the GPU
                        int brickIndex = chunk->_indices[i] & BRICKMAP_INDEX_BITS;
                        if (chunk->_indices[i] & BRICKMAP_LOADED_BIT) {
                            chunkData->_brickmaps[i] = BRICKMAP_UNLOADED_BIT | (chunk->_indices[i] & BRICKMAP_LOD_BITS) | (chunk->_indices[i] & BRICKMAP_INDEX_BITS);
                            //chunkData->_brickmaps[i] = BRICKMAP_LOADED_BIT | chunk->_indices[i];
                            //chunkData->_brickmapsData[i] = chunk->_brickmaps[brickIndex];
                        }
                        else {
                            chunkData->_brickmaps[i] = 0;
                        }
                    }

                    //chunk->_gpuChunkData = chunkData->_brickmapsData.data();
                    //chunk->_gpuIndices = chunkData->_brickmaps.data();
                }
            }
        }
        
        return true;
    }

} // namespace srtv_engine::worldgen
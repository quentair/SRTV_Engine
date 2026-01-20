#include "gpu_world.h"

namespace srtv_engine::worldgen {

void GpuWorld::loadRegions(std::vector<WorldRegion*> &regions, glm::vec3 playerWorldPos)
{
    // load to GPU all chunks in given generated regions
    _tempChunks.clear();
    saveChunks(playerWorldPos);

    for (auto& r : regions) {
        loadChunks(*r, playerWorldPos);
    }
}

void GpuWorld::saveChunks(glm::vec3 playerWorldPos)
{
    // fill the temporary chunk pointer and data buffer 
    // do this before actual chunks load algorithm because the load order depends on player displacement, otherwise we could overwrite and delete data that should be re-used (if player goes backward, back row to front row reload is needed, if player goes frontward, front row to top row reload is needed, same with columns if player goes left or right)
    // so entirely filling this vector before will get rid of the potential lost of data at the cost of perdormances when reload is needed

    for (int i = 0; i < _gpuLoadedChunks.size(); i++) {
        if (_gpuLoadedChunks[i] == nullptr)
            continue;

        int xGrid = i % (VIEWDISTANCE * 2 + 1);
        int zGrid = i / (VIEWDISTANCE * 2 + 1) % (VIEWDISTANCE * 2 + 1);
        int yGrid = i / (VIEWDISTANCE * 2 + 1) / (VIEWDISTANCE * 2 + 1);
        
        int viewGridIndex = xGrid + zGrid * (VIEWDISTANCE * 2 + 1);
        int yRegion = yGrid;

        ChunkGpuData* chunkData = &_chunksDataColumns[viewGridIndex]._chunksInColumn[yRegion];
        _tempChunks.push_back(std::make_pair(_gpuLoadedChunks[i], *chunkData));
    }
}

bool GpuWorld::loadChunks(WorldRegion &region, glm::vec3 playerWorldPos)
{
    // Prepare chunk content for GPU
    // at first feedback loop, all chunks are not loaded to GPU, the GPU request those needed afterward
    // view distance grid send to GPU is ogranised into column of chunks

    glm::ivec3 regionPosition = region._worldPos;

    // compute view distance area corners in world coordinates, in relation to the region world position
    glm::vec3 viegGridBottomLeftCornerCoordinates = glm::vec3(floor(playerWorldPos.x) - VIEWDISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.x, 0, floor(playerWorldPos.z) - VIEWDISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.z);
    glm::vec3 viewGridUpperRightCornerCoordinates = glm::vec3(floor(playerWorldPos.x) + VIEWDISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.x, 0, floor(playerWorldPos.z) + VIEWDISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.z);

    // convert corner positions to chunks grid coordinates (y axis is defaulted because viewdistance is a 2D grid on XZ plane)
    glm::ivec3 viegGridBottomLeftCorner = region.relativeWorldPosToChunkGridPosition(viegGridBottomLeftCornerCoordinates.x, 0, viegGridBottomLeftCornerCoordinates.z);
    glm::ivec3 viewGridUpperRightCorner = region.relativeWorldPosToChunkGridPosition(viewGridUpperRightCornerCoordinates.x, 0, viewGridUpperRightCornerCoordinates.z);

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

    // load all chunks of the region inside view distance
    for (int xRegion = chunkGridBottomLeftCorner.x; xRegion <= chungGirdUpperRightCorner.x; xRegion++) {
        for (int zRegion = chunkGridBottomLeftCorner.z; zRegion <= chungGirdUpperRightCorner.z; zRegion++) {

            // convert chunk grid position to view grid position
            glm::ivec2 viewGridPos = glm::ivec2(chunkGridBottomLeftCorner.x - viegGridBottomLeftCorner.x + xRegion - chunkGridBottomLeftCorner.x, chunkGridBottomLeftCorner.z - viegGridBottomLeftCorner.z + zRegion - chunkGridBottomLeftCorner.z);
            int viewGridIndex = viewGridPos.x + viewGridPos.y * (VIEWDISTANCE * 2 + 1);

            _viewDistanceGrid[viewGridIndex] = 1; // indicates chunks presence in the world, we must then retrieve the data

            for (int yRegion = REGION_SIZE_Y-1; yRegion >= 0; yRegion--) {

                // retrieve chunk data from region
                int index = region.localPosToIndex(xRegion, yRegion, zRegion);
                BrickChunk* chunk = region._chunks[index].get();

                if (chunk == nullptr || chunk->isGenerated() == false) {
                    _viewDistanceGrid[viewGridIndex] = 0; // indicates that this chunk is not generated yet
                    continue;
                }

                // get chunk data in the chunk column given its y position
                ChunkGpuData* chunkData = &_chunksDataColumns[viewGridIndex]._chunksInColumn[yRegion];

                // check if the chunk was already loaded in the gpu at the right position, if so, no need to check for hot-reload or replace it
                int loadedMapIndex = viewGridIndex + yRegion * (VIEWDISTANCE * 2 + 1) * (VIEWDISTANCE * 2 + 1);
                if (_gpuLoadedChunks[loadedMapIndex] == chunk) {
                    continue;
                }

                // save chunk pointer and mark it as loaded in the GPU
                _gpuLoadedChunks[loadedMapIndex] = chunk;

                // if the chunk was an already loaded chunk but changed from position (cell), just copy back the content
                bool copied = false;
                for (int i = 0; i < _tempChunks.size(); i++) {
                    if (_tempChunks[i].first == chunk) {
                        chunkData->_brickmapsData = _tempChunks[i].second._brickmapsData;
                        chunkData->_brickmaps = _tempChunks[i].second._brickmaps;
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
                        _viewDistanceGrid[viewGridIndex] = 0; // indicates that this chunk is not generated yet
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
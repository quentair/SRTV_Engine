#include "gpu_world.h"

#include <algorithm>

namespace srtv_engine::worldgen {

void GpuWorld::loadRegions(std::vector<WorldRegion*> &regions, glm::vec3 playerWorldPos)
{
    glm::ivec3 centalChunkWorldPos = glm::ivec3(floor(playerWorldPos.x / CHUNK_VOXEL_RESOLUTION) * CHUNK_VOXEL_RESOLUTION, floor(playerWorldPos.y / CHUNK_VOXEL_RESOLUTION) * CHUNK_VOXEL_RESOLUTION, floor(playerWorldPos.z / CHUNK_VOXEL_RESOLUTION) * CHUNK_VOXEL_RESOLUTION);

    _playerChangedChunk = centalChunkWorldPos != _playerLastChunk;

    if (_playerChangedChunk) {

        for (int i = 0; i < _brickmapsData.size(); i++) {
            // mark all the brickmap datas as unused for this frame, we will mark them back as used as we retrieve them when we roll the chunks (so that the datas of the chunks that are now out of range are mark as unused at the end)
            _brickmapsData[i]._used = false;
        }

        // clear queues to prevent out of date brickmap access on GPU readbacks
        _dirtyBrickmapsQueue = std::vector<std::vector<glm::ivec4>>(2);

    // save the chunks datas that we send to GPU in case the player moved from one chunk to another (so we don't erase datas when we load back new and old chunks to the GPU)
        saveChunks(playerWorldPos);
        _playerLastChunk = centalChunkWorldPos;
        _viewgridAnchorWorldPos = glm::vec2(float(centalChunkWorldPos.x) - MAX_VIEW_DISTANCE * CHUNK_VOXEL_RESOLUTION, float(centalChunkWorldPos.z) - MAX_VIEW_DISTANCE * CHUNK_VOXEL_RESOLUTION);
    }

    // load to GPU all chunks in given generated regions that are in the player view range
    for (auto& r : regions) {
        if (r == nullptr) {
            continue;
        }
        loadChunks(*r, playerWorldPos);
    }
}

void GpuWorld::saveChunks(glm::vec3 playerWorldPos)
{
    // fill the temporary chunk data hashmap 
    // do this before actual chunks load algorithm because the load order depends on player displacement, otherwise we could overwrite and delete data that should be re-used (if player goes backward, back row to front row reload is needed, if player goes frontward, front row to top row reload is needed, same with columns if player goes left or right)
    // so entirely filling this hashmap before will get rid of the potential lost of data at the cost of perdormances when reload is needed

    for (int i = 0; i < _gpuLoadedChunks.size(); i++) {
        if (_gpuLoadedChunks[i] == nullptr)
            continue;

        // retrieve already loaded chunk data
        int xGrid = i % (MAX_VIEW_DISTANCE * 2 + 1);
        int zGrid = i / (MAX_VIEW_DISTANCE * 2 + 1) % (MAX_VIEW_DISTANCE * 2 + 1);
        int yGrid = i / (MAX_VIEW_DISTANCE * 2 + 1) / (MAX_VIEW_DISTANCE * 2 + 1);
        
        int viewGridIndex = xGrid + zGrid * (MAX_VIEW_DISTANCE * 2 + 1);
        int yRegion = yGrid;

        ChunkCpuData* chunkData = &_chunksDataColumns[viewGridIndex]._chunksInColumn[yRegion];

        // because the player changed from chunk on this frame and the loaded chunks are not yet up to date, the central chunk of the loaded chunks is the chunk where the player was situated on the precedent frame
        glm::ivec3 centalChunkWorldPos = _playerLastChunk;

        glm::ivec3 gpuLoadedChunkWorldPos = glm::ivec3(centalChunkWorldPos.x + (xGrid - MAX_VIEW_DISTANCE) * CHUNK_VOXEL_RESOLUTION, yGrid * CHUNK_VOXEL_RESOLUTION, centalChunkWorldPos.z + (zGrid - MAX_VIEW_DISTANCE) * CHUNK_VOXEL_RESOLUTION);
        
        // save loaded chunk data
        _tempChunks[gpuLoadedChunkWorldPos] = *chunkData;

        // clear the pointer to the chunk to avoid using outdated datas (pointer will be updated later if needed)
        _gpuLoadedChunks[i] = nullptr;

        // clear GPU chunk datas so we don't accidentaly read datas from a chunk that is no longer at this position
        chunkData->_brickmaps = std::array<uint32_t, CHUNK_SIZE* CHUNK_SIZE* CHUNK_SIZE>{};
        chunkData->_dataIndex = -1;

        // chunk data was erased, so mark the chunk as dirty
        markChunkAsDirty(i);
    }
}

void GpuWorld::loadChunks(WorldRegion &region, glm::vec3 playerWorldPos)
{
    // Prepare chunk content for GPU
    // at first feedback loop, all chunks are not loaded to GPU, the GPU request those needed afterward
    // view distance grid send to GPU is ogranised into column of chunks

    glm::ivec3 regionPosition = region._worldPos;

    // compute view distance area corners in world coordinates, in relation to the region world position (remember that this is a subset of our maximum viewing range)
    glm::vec3 viegGridBottomLeftCornerCoordinates = glm::vec3(floor(playerWorldPos.x) - _viewDistance * CHUNK_VOXEL_RESOLUTION - regionPosition.x, 0, floor(playerWorldPos.z) - _viewDistance * CHUNK_VOXEL_RESOLUTION - regionPosition.z);
    glm::vec3 viewGridUpperRightCornerCoordinates = glm::vec3(floor(playerWorldPos.x) + _viewDistance * CHUNK_VOXEL_RESOLUTION - regionPosition.x, 0, floor(playerWorldPos.z) + _viewDistance * CHUNK_VOXEL_RESOLUTION - regionPosition.z);

    // convert corner positions to chunks grid coordinates (y axis is defaulted because viewdistance is a 2D grid on XZ plane)
    glm::ivec3 viegGridBottomLeftCorner = region.relativeWorldPosToChunkGridPosition(viegGridBottomLeftCornerCoordinates.x, 0, viegGridBottomLeftCornerCoordinates.z);
    glm::ivec3 viewGridUpperRightCorner = region.relativeWorldPosToChunkGridPosition(viewGridUpperRightCornerCoordinates.x, 0, viewGridUpperRightCornerCoordinates.z);

    // same for maximum viewing range to sample loaded chunks on the right indices
    glm::vec3 maxViegGridBottomLeftCornerCoordinates = glm::vec3(floor(playerWorldPos.x) - MAX_VIEW_DISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.x, 0, floor(playerWorldPos.z) - MAX_VIEW_DISTANCE * CHUNK_VOXEL_RESOLUTION - regionPosition.z);
    glm::ivec3 maxViegGridBottomLeftCorner = region.relativeWorldPosToChunkGridPosition(maxViegGridBottomLeftCornerCoordinates.x, 0, maxViegGridBottomLeftCornerCoordinates.z);

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

    // load all chunks of the region inside view distance
    for (int xRegion = chunkGridBottomLeftCorner.x; xRegion <= chungGirdUpperRightCorner.x; xRegion++) {
        for (int zRegion = chunkGridBottomLeftCorner.z; zRegion <= chungGirdUpperRightCorner.z; zRegion++) {

            // convert chunk grid position to view grid position
            glm::ivec2 viewGridPos = glm::ivec2(chunkGridBottomLeftCorner.x - maxViegGridBottomLeftCorner.x + xRegion - chunkGridBottomLeftCorner.x, chunkGridBottomLeftCorner.z - maxViegGridBottomLeftCorner.z + zRegion - chunkGridBottomLeftCorner.z);
            int viewGridIndex = viewGridPos.x + viewGridPos.y * (MAX_VIEW_DISTANCE * 2 + 1);

            for (int yRegion = REGION_SIZE_Y-1; yRegion >= 0; yRegion--) {

                // retrieve chunk data from region
                const uint32_t index = region.localPosToIndex(xRegion, yRegion, zRegion);
                BrickChunk* chunk = region._chunks[index].get();

                // check if the chunk was already loaded in the gpu at the right position, if so, no need to check for hot-reload or replace it
                int loadedMapIndex = viewGridIndex + yRegion * (MAX_VIEW_DISTANCE * 2 + 1) * (MAX_VIEW_DISTANCE * 2 + 1);
                if (_gpuLoadedChunks[loadedMapIndex] == chunk && chunk != nullptr) {
                    _viewDistanceGrid[viewGridIndex] = 1; // indicates chunks column presence in the world (because at least one chunk of the column is present)
                    continue;
                }
                else if (_gpuLoadedChunks[loadedMapIndex] == chunk && chunk == nullptr) {
                    // if chunk was empty and stays empty, skip it too
                    _viewDistanceGrid[viewGridIndex] |= 0; // indicates that this chunk column might not be generated yet (if at least on chunk of the column is present, it is marked as generated, otherwise, it is not marked)
                    continue;
                }

                // replace or reaload needed

                // get chunk data in the chunk column given its y position
                ChunkCpuData* chunkData = &_chunksDataColumns[viewGridIndex]._chunksInColumn[yRegion];
                
                if (chunk == nullptr || chunk->isGenerated() == false) {
                    _viewDistanceGrid[viewGridIndex] |= 0; // indicates that this chunk column might not be generated yet (if at least on chunk of the column is present, it is marked as generated, otherwise, it is not marked)
                    _gpuLoadedChunks[loadedMapIndex] = nullptr; // update array of pointer to chunks that are sent to GPU to avoid unupdated data retrieval later
                    continue;
                }

                // save chunk pointer for quick access
                _gpuLoadedChunks[loadedMapIndex] = chunk;

                // if the chunk was an already loaded chunk but changed from position (cell), just copy back the content we saved earlier in the hashmap
                glm::ivec3 gpuLoadedChunkWorldPosition = glm::ivec3(regionPosition.x + xRegion * CHUNK_VOXEL_RESOLUTION, yRegion * CHUNK_VOXEL_RESOLUTION, regionPosition.z + zRegion * CHUNK_VOXEL_RESOLUTION);
                if (auto search = _tempChunks.find(gpuLoadedChunkWorldPosition); search != _tempChunks.end())
                {
                    _viewDistanceGrid[viewGridIndex] = 1; // indicates chunks column presence in the world (because at least one chunk of the column is present)
                    *chunkData = search->second;
                    if (chunkData->_dataIndex >= 0) {
                        _brickmapsData[chunkData->_dataIndex]._used = true; // mark the chunk data as used so the array cell is reserved and not replaced later
                    }
                    continue;
                }

                for (int i = 0; i < chunk->_indices.size(); i++) {
                    // if brickmap inside the chunk has been generated, just load the LOD and mark it as unloaded for the GPU
                    if (chunk->_indices[i] & BRICKMAP_LOADED_BIT) {
                        // chunk data is updated, so mark the chunk as dirty
                        markChunkAsDirty(loadedMapIndex);
                        chunkData->_brickmaps[i] = BRICKMAP_UNLOADED_BIT | (chunk->_indices[i] & BRICKMAP_LOD_BITS) | (chunk->_indices[i] & BRICKMAP_INDEX_BITS);
                        _viewDistanceGrid[viewGridIndex] = 1; // indicates chunks column presence in the world (because at least one chunk of the column is present)
                    }
                    else {
                        _viewDistanceGrid[viewGridIndex] |= 0; // indicates that this chunk column might not be generated yet (if at least on chunk of the column is present, it is marked as generated, otherwise, it is not marked)
                        chunkData->_brickmaps[i] = 0;
                    }
                }

                //chunk->_gpuChunkData = chunkData->_brickmapsData.data();
                //chunk->_gpuIndices = chunkData->_brickmaps.data();
            }
        }
    }
}

} // namespace srtv_engine::worldgen
#pragma once

#include "brickmap.h"
#include "brick_chunk.h"
#include "world_region.h"
#include "buffer.h"

#include <memory>
#include <unordered_map>

// max number of brickmaps to send to GPU for loading
constexpr int BRICKMAP_QUEUE_SIZE = 1024;

// radius distance for which chunk we can see around the camera
constexpr int VIEWDISTANCE = 3;
constexpr int VIEW_GRID_SIZE = (VIEWDISTANCE * 2 + 1) * (VIEWDISTANCE * 2 + 1);

namespace srtv_engine::worldgen {

struct ChunkGpuData {
	std::array<uint32_t, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> _brickmaps; // array of brickmap indices to retrieve from brickmap buffer

	std::array<Brickmap, INITIAL_GPU_BRICKMAPS_NUMBER> _brickmapsData;
};

struct ChunksColumnGpuData {
	std::array<ChunkGpuData, REGION_SIZE_Y> _chunksInColumn; // array of brickmap indices to retrieve from brickmap buffer
};

struct WorldGpuData {
	//std::array<Brickmap, BRICKMAP_QUEUE_SIZE> _bricksQueue;
	//std::array<uint32_t, BRICKMAP_QUEUE_SIZE> _indicesQueue;

	std::array<glm::ivec4, BRICKMAP_QUEUE_SIZE> _brickLoadQueue;
	uint32_t _brickLoadQueueCount{ 0 };
};

// x : horizontal axis ; y : vertical axis ; z : depth axis
class GpuWorld {
  public:

	// Buffer1 : index for the chunks data buffer
	WorldGpuData _worldData;

	// Buffer2 : presence grid for the chunks data
	std::vector<uint32_t> _viewDistanceGrid = std::vector<uint32_t>(VIEW_GRID_SIZE); // array of chunk presence, parse and retrieve data at same incides from chunk buffer

	// Buffer3 : data of chunks organised into columns of chunks
	std::vector<ChunksColumnGpuData> _chunksDataColumns = std::vector<ChunksColumnGpuData>(VIEW_GRID_SIZE);

	//  quick access to chunks cached for the GPU
	std::array<BrickChunk*, VIEW_GRID_SIZE * REGION_SIZE_Y> _gpuLoadedChunks;

	bool loadChunks(WorldRegion& region, glm::vec3 playerWorldPos);

	void resetChunks() {
		for (int i = 0; i < VIEW_GRID_SIZE; i++) {
			_viewDistanceGrid[i] = 0;
		}
	}

	void resetQueues() {
		//_worldData._bricksQueue.fill(Brickmap());
		//_worldData._indicesQueue.fill(0);
		//_worldData._brickLoadQueue.fill(glm::ivec4(0, 0, 0, 0));
		_worldData._brickLoadQueueCount = 0;
	}
};

} // namespace srtv_engine::worldgen
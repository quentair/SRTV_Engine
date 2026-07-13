#pragma once

#include "parameters.h"
#include "brickmap.h"
#include "brick_chunk.h"
#include "world_region.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>
#include <vector>
#include <unordered_map>
#include <thread>

// max number of brickmaps to send to GPU for loading
constexpr uint32_t BRICKMAP_QUEUE_SIZE = 512;

// area of chunks we send to GPU
constexpr uint32_t MAX_VIEW_GRID_SIZE = (MAX_VIEW_DISTANCE * 2 + 1) * (MAX_VIEW_DISTANCE * 2 + 1);

namespace srtv_engine::worldgen {

// GPU layout

struct BrickmapsGpuData {
	Brickmap _brickmapData;
};

struct ChunkGpuData {
	std::array<uint32_t, CHUNK_BRICKMAP_RESOLUTION> _brickmaps; // array of brickmap infos to parse

	std::array<int32_t, CHUNK_BRICKMAP_RESOLUTION> _dataIndices; // indicates where the brickmap datas are situated in the brickmap data buffer
};

struct ChunksColumnGpuData {
	std::array<ChunkGpuData, REGION_SIZE_Y> _chunksInColumn;
};

struct WorldGpuData {
	//std::array<Brickmap, BRICKMAP_QUEUE_SIZE> _bricksQueue;
	//std::array<uint32_t, BRICKMAP_QUEUE_SIZE> _indicesQueue;

	std::array<glm::ivec4, BRICKMAP_QUEUE_SIZE> _brickLoadQueue;
	uint32_t _brickLoadQueueCount{ 0 };
};

// CPU layout

struct BrickmapsCpuData {
	Brickmap* _brickmapData;

	bool _used = false; // specify if the data is currently used (that is, referenced in the chunk grid) or not (and in this case, data will be replaced)
};

struct ChunkCpuData {
	std::array<uint32_t, CHUNK_BRICKMAP_RESOLUTION> _brickmaps; // array of brickmap infos to parse

	std::array<int32_t, CHUNK_BRICKMAP_RESOLUTION> _dataIndices; // indicates where the brickmap data are situated in the brickmap data buffer

	// default constructor to properly fill arrays
	ChunkCpuData() {
		_brickmaps.fill(0);
		_dataIndices.fill(-1);
	}
};

struct ChunksColumnCpuData {
	std::array<ChunkCpuData, REGION_SIZE_Y> _chunksInColumn;
};

// x : horizontal axis ; y : vertical axis ; z : depth axis
class GpuWorld {
  public:

	// Buffer1 : index for the chunks data buffer
	//WorldGpuData _worldData;

	// Buffer2 : presence grid for the chunks data
	std::vector<uint32_t> _viewDistanceGrid = std::vector<uint32_t>(MAX_VIEW_GRID_SIZE); // array of chunk presence, parse and retrieve data at same incides from chunk buffer

	// Buffer3 : data of chunks organised into columns of chunks
	std::vector<ChunksColumnCpuData> _chunksDataColumns = std::vector<ChunksColumnCpuData>(MAX_VIEW_GRID_SIZE);

	// Buffer 4 : brickmaps datas (chunk surface is 16 * 16 brickmap, the surface is the maximum view distance, times 4 to have some room)
	std::vector<BrickmapsCpuData> _brickmapsData = std::vector<BrickmapsCpuData>(MAX_VIEW_GRID_SIZE * CHUNK_SIZE * CHUNK_SIZE * 4);
	// list of available brickmaps datas vector cells that can be write on (on readback step)
	std::list<unsigned int> _availableBrickmapsIndex;

	std::vector<BrickChunk*> _gpuLoadedChunks = std::vector<BrickChunk*>(MAX_VIEW_GRID_SIZE * REGION_SIZE_Y); //  pointer to chunks whose datas are cached for the GPU
	std::unordered_map<glm::ivec3, ChunkCpuData> _tempChunks; // temporary chunk data hashmap to displace the chunks instead of reloading them if they are still in the view range

	bool _playerChangedChunk = false;

	// threads
	std::thread _chunkLoadingThread;
	std::atomic<bool> _processingChunk = false; // signal that the rolling operation is done on the thread and chunk can be copied again

	// world position of the chunk where the player was last situated
	glm::ivec3 _playerLastChunk{ 0, -1, 0 };

	// world position of the bottom left chunk of our viewgrid
	glm::vec2 _viewgridAnchorWorldPos{0,0};

	uint32_t _viewDistance = BASE_VIEW_DISTANCE;

	// indicates which chunk on the GPU needs to be updated on which frame (buffer 3 update)
	std::vector<std::vector<bool>> _dirtyChunksIndicator = std::vector<std::vector<bool>>(2, std::vector<bool>(MAX_VIEW_GRID_SIZE* REGION_SIZE_Y, false));

	// indicates which brickmap on the GPU needs to be updated on which frame (buffer 4 update)
	std::vector<std::vector<glm::ivec4>> _dirtyBrickmapsQueue = std::vector<std::vector<glm::ivec4>>(2);

	GpuWorld() {
		// at start, all brickmap datas vector cells are available
		for (int i = 0; i < _brickmapsData.size(); i++) {
			_availableBrickmapsIndex.push_back(i);
		}
	}

	void loadRegions(std::vector<WorldRegion*> &regions, glm::vec3 playerWorldPos);

	void rollChunks(std::vector<WorldRegion*>& regions, glm::vec3 playerWorldPos, glm::vec3 newCentralChunkWorldPos);

	void saveChunks(glm::vec3 newCentralChunkWorldPos);

	void loadChunks(WorldRegion& region, glm::vec3 playerWorldPos);

	void retrieveUnusedBrickmapsDatas();

	void changeViewDistance(int& newViewDistance) {
		_viewDistance = newViewDistance;
	}

	void resetViewGrid() {
		for (int i = 0; i < MAX_VIEW_GRID_SIZE; i++) {
			_viewDistanceGrid[i] = 0;
		}
	}

	// check on specific frame
	bool isChunkDirty(int frame, int chunkNumber) {
		return _dirtyChunksIndicator[frame][chunkNumber];
	}

	// check on all frame
	bool isChunkDirty(int chunkNumber) {
		for (auto& frame : _dirtyChunksIndicator) {
			if (frame[chunkNumber] == true)
				return true;
		}
		return false;
	}

	// mark on specific frame
	void markChunkAsDirty(int frame, int chunkNumber) {
		_dirtyChunksIndicator[frame][chunkNumber] = true;
	}
	
	// mark on every frame
	void markChunkAsDirty(int chunkNumber) {
		for (auto& frame : _dirtyChunksIndicator) {
			frame[chunkNumber] = true;
		}
	}

	// mark on specific frame
	void markChunkAsUpdated(int frame, int chunkNumber) {
		_dirtyChunksIndicator[frame][chunkNumber] = false;
	}

	// mark on every frame
	void markChunkAsUpdated(int chunkNumber) {
		for (auto& frame : _dirtyChunksIndicator) {
			frame[chunkNumber] = false;
		}
	}
};

} // namespace srtv_engine::worldgen
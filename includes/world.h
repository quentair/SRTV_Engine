#pragma once

#include "world_region.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <memory>
#include <unordered_map>
#include <mutex>

namespace srtv_engine::worldgen {

	// x : horizontal axis ; y : vertical axis ; z : depth axis
	class World {
	public:
		std::unordered_map<glm::ivec3, std::unique_ptr<WorldRegion>> _regionsHashmap; // world is a region grid that is stored in this hashmap
		std::vector<WorldRegion*> _generatedRegions; // for sharing generated regions and their positions on the regions grid

		std::mutex _generatedRegionsMutex; // mutex for save operations on _generatedRegions vector above

		void generateRegionsAroundPosition(float x, float y, float z, std::atomic<bool>& stopGeneration);

		WorldRegion* getRegion(int x, int y, int z);

		void generateRegion(int x, int y, int z, std::atomic<bool>& stopGeneration);

		void clearRegionsAroundPlayer() {
			_generatedRegions.clear();
		}

		void clearRegionsFarFromPositon(float x, float y, float z);
	};

} // namespace srtv_engine::worldgen
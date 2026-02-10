#pragma once

#include "world_region.h"
#include "world_generator.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_map>
#include <mutex>

namespace srtv_engine::worldgen {

	// x : horizontal axis ; y : vertical axis ; z : depth axis
	class World {
	public:
		std::unordered_map<glm::ivec3, std::unique_ptr<WorldRegion>> _regionsHashmap; // world is a region grid that is stored in this hashmap
		std::vector<WorldRegion*> _registeredRegions; // for sharing registered regions and their positions on the regions grid

		std::mutex _registeredRegionsMutex; // mutex for save operations on _registeredRegions vector above

		void generateRegionsAroundPlayerPosition(float xPlayer, float yPlayer, float zPlayer, WorldGenerator& worldGenerator, std::atomic<bool>& stopGeneration);

		WorldRegion* getRegion(int x, int y, int z);

		void generateRegion(float xPlayer, float yPlayer, float zPlayer, int xRegion, int yRegion, int zRegion, WorldGenerator& worldGenerator, std::atomic<bool>& stopGeneration);

		void clearRegionsAroundPlayer() {
			_registeredRegions.clear();
		}

		void clearRegionsFarFromPositon(float x, float y, float z);
	};

} // namespace srtv_engine::worldgen
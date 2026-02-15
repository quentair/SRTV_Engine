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

// added radius distance for which chunk will be generated around player (in addition to the one he already sees)
constexpr uint32_t PRE_LOADING_DISTANCE = 2;

// radius distance for which chunk will be generated around player
constexpr uint32_t BASE_GENERATING_DISTANCE = BASE_VIEW_DISTANCE + PRE_LOADING_DISTANCE;

namespace srtv_engine::worldgen {

// x : horizontal axis ; y : vertical axis ; z : depth axis
class WorldGenerator {
  public:
	
	uint32_t _viewDistance = BASE_VIEW_DISTANCE;

	uint32_t _generatingDistance = BASE_GENERATING_DISTANCE;

	void generateRegions(std::vector<WorldRegion*>& regions, glm::vec3 playerWorldPos, std::atomic<bool>& stopGeneration);

	void generateRegion(WorldRegion& region, glm::vec3 playerWorldPos, std::atomic<bool>& stopGeneration);

	void changeViewDistance(int& newViewDistance) {
		_viewDistance = newViewDistance;
		int newGeneratingDistance = newViewDistance + PRE_LOADING_DISTANCE;
		changeGeneratingDistance(newGeneratingDistance);
	}

	// private ?
	void changeGeneratingDistance(int& newGeneratingDistance) {
		_generatingDistance = newGeneratingDistance;
	}
};

} // namespace srtv_engine::worldgen
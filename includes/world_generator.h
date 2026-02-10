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

// radius distance for which chunk will be generated around player in addition to the one he already sees (for pre-loading)
constexpr int PRE_LOADING_DISTANCE = 2;

constexpr int GENERATE_DISTANCE = VIEWDISTANCE + PRE_LOADING_DISTANCE;

namespace srtv_engine::worldgen {

// x : horizontal axis ; y : vertical axis ; z : depth axis
class WorldGenerator {
  public:

	void generateRegions(std::vector<WorldRegion*>& regions, glm::vec3 playerWorldPos, std::atomic<bool>& stopGeneration);

	void generateRegion(WorldRegion& region, glm::vec3 playerWorldPos, std::atomic<bool>& stopGeneration);
};

} // namespace srtv_engine::worldgen
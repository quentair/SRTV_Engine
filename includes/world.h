#pragma once

#include "world_region.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <memory>
#include <unordered_map>

namespace srtv_engine::worldgen {

	// x : horizontal axis ; y : vertical axis ; z : depth axis
	class World {
	public:
		std::unordered_map<glm::ivec3, std::unique_ptr<WorldRegion>> _regions;

		WorldRegion* getRegion(unsigned int x, unsigned int y, unsigned int z);

		void generateRegion(unsigned int x, unsigned int y, unsigned int z);
	};

} // namespace srtv_engine::worldgen
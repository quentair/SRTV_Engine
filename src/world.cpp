#include "world.h"

namespace srtv_engine::worldgen {

WorldRegion* World::getRegion(unsigned int x, unsigned int y, unsigned int z)
{
    if (auto search = _regions.find(glm::ivec3(x, y, z)); search != _regions.end())
        return search->second.get();
    else
        return nullptr;
}

void World::generateRegion(unsigned int x, unsigned int y, unsigned int z)
{
    if (getRegion(x, y, z) == nullptr) {
        _regions[glm::ivec3(x, y, z)] = std::make_unique<WorldRegion>();
        _regions[glm::ivec3(x, y, z)]->generate();
    }
}

} // namespace srtv_engine::worldgen
#include "world.h"

namespace srtv_engine::worldgen {

void World::generateRegionsAroundPosition(float x, float y, float z, std::atomic<bool>& stopGeneration)
{
    // get the 8 regions around the given world position as well as the center region and generate them if needed

    glm::ivec3 actualRegion = glm::ivec3(floor(x / REGION_VOXEL_RESOLUTION_XZ), floor(y / REGION_VOXEL_RESOLUTION_Y), floor(z / REGION_VOXEL_RESOLUTION_XZ));
    
    for (int dx = -1; dx < 2; dx++) {
        for (int dz = -1; dz < 2; dz++) {
            if (stopGeneration.load()) {
                return;
            }
            generateRegion(actualRegion.x + dx, actualRegion.y, actualRegion.z + dz, stopGeneration);
        }
    }
}

WorldRegion* World::getRegion(int x, int y, int z)
{
    // return a pair of its region and its position in the world's region grid
    glm::ivec3 regionPosition = glm::ivec3(x, y, z);

    if (auto search = _regionsHashmap.find(regionPosition); search != _regionsHashmap.end())
        return search->second.get();
    else
        return nullptr;
}

void World::generateRegion(int x, int y, int z, std::atomic<bool>& stopGeneration)
{
    // generate the region (chunk and all) at the given position on the world's region grid

    WorldRegion* region = getRegion(x, y, z);
    if (region == nullptr) {

        _regionsHashmap[glm::ivec3(x, y, z)] = std::make_unique<WorldRegion>();

        _regionsHashmap[glm::ivec3(x, y, z)]->init(x, y, z);

        // lock generated region vector access and write on it, we will read it on the main thread after the write operation
        {
            const std::lock_guard<std::mutex> lock(_generatedRegionsMutex);
            _generatedRegions.push_back(getRegion(x, y, z));
        }
        
        _regionsHashmap[glm::ivec3(x, y, z)]->generate(stopGeneration);
    }
}

void World::clearRegionsFarFromPositon(float x, float y, float z)
{
    // if the region is too far away from given world position, unload it

    // TODO : save regions in files before clearing

    float maxDistance = 10000;

    for (auto it = _regionsHashmap.begin(); it != _regionsHashmap.end();) {
        glm::vec3 regionPos = it->first;

        float distanceToRegion = glm::distance(regionPos, glm::vec3(x, y, z));

        if (distanceToRegion > maxDistance) {
            it = _regionsHashmap.erase(it);
        }
        else {
            ++it;
        }
    }

    // don't forget to also clear from generated regions sharing vector to avoid dangling pointers
    for (auto it = _generatedRegions.begin(); it != _generatedRegions.end();) {
        if (*it == nullptr) {
            it = _generatedRegions.erase(it);
        }
        else {
            ++it;
        }
    }
}

} // namespace srtv_engine::worldgen
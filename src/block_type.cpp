#include "block_type.h"

namespace srtv_engine::worldgen {

std::array<BlockType, MAX_NUMBER_OF_BLOCK_TYPE> blockTypeAtlas =
{
	BlockType(0, glm::vec4(0.0, 1.0, 1.0, 1.0)), // undefined block always at index 0 (just in case for now)
	BlockType(1, glm::vec4(0.0, 0.0, 0.0, 0.0)), // air
	BlockType(2, glm::vec4(0.57, 0.83, 0.34, 1.0)), // grass
	BlockType(3, glm::vec4(0.78, 0.78, 0.78, 1.0)) // stone
};

} // namespace srtv_engine::worldgen
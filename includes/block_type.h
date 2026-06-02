#pragma once

#include <glm/glm.hpp>

#include <array>
#include <stdint.h>

constexpr uint8_t MAX_NUMBER_OF_BLOCK_TYPE = 255;

constexpr uint8_t NUMBER_OF_BLOCK_TYPE = 4;

namespace srtv_engine::worldgen {

class BlockType {

  public:

	BlockType() : _typeId(1), _blockColor(glm::vec4(0.0, 0.0, 0.0, 0.0)) {} // base block is air

	BlockType(uint8_t id, glm::vec4 color) {
		_typeId = id;
		_blockColor = color;
	}

	uint8_t _typeId; // e.g : 0 = undef, 1 = air, 2 = stone...
	
	glm::vec4 _blockColor; // rgba color

};

class BlockPaletteEntry {

  public:

	BlockType _type;

	int _count = 0; // number of times this block type appears in the brick
};

// list of all block types that are present in the engine
extern std::array<BlockType, MAX_NUMBER_OF_BLOCK_TYPE> blockTypeAtlas;


} // namespace srtv_engine::worldgen
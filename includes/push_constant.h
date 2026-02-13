#pragma once

#include <glm/glm.hpp>

#include <stdint.h>

namespace srtv_engine {

struct ComputeShaderPushConstant {
  public :
	glm::vec4 _cameraPosition;
	glm::vec4 _cameraFront;
	glm::vec4 _cameraUp;
	glm::vec4 _cameraRight;
	uint32_t _viewDistance;
	uint32_t _lodDistance2x2x2;
};

} // namespace srtv_engine
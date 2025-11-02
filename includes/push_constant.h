#pragma once

#include <glm/glm.hpp>

namespace srtv_engine {

struct ComputeShaderCameraPushConstant {
  public :
	glm::vec4 _cameraPosition;
	glm::vec4 _cameraFront;
	glm::vec4 _cameraUp;
	glm::vec4 _cameraRight;
};

} // namespace srtv_engine
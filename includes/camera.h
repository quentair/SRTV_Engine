#pragma once

#include <glm/glm.hpp>

namespace srtv_engine {

class Camera {
  public :
	void orthographicProjection(float left, float right, float top, float bottom, float near, float far);

	void perspectiveProjection(float fovY, float aspectRatio, float near, float far);

	void setCameraDirection(glm::vec3 cameraPosition, glm::vec3 cameraDirection, glm::vec3 cameraUp = glm::vec3 {0.f, -1.f, 0.f});

	void setCameraTarget(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp = glm::vec3{ 0.f, -1.f, 0.f });

	void setCameraYXZ(glm::vec3 cameraPosition, glm::vec3 cameraRotation);

	const glm::mat4& getProjectionMatrix() const { return _projectionMatrix; }
	const glm::mat4& getViewnMatrix() const { return _viewMatrix; }

  private :
	glm::mat4 _projectionMatrix{ 1.f };
	glm::mat4 _viewMatrix{ 1.f };
};

} // namespace srtv_engine
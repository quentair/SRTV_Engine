#pragma once

#include "camera.h"

#include <SDL3/SDL_events.h>
#include <glm/glm.hpp>

namespace srtv_engine {

// free-mode camera controls
class CameraControl {
public:
	enum Mapping {
		moveForwardKey = SDL_SCANCODE_W,
		moveBackwardKey = SDL_SCANCODE_S,
		moveLeftKey = SDL_SCANCODE_A,
		moveRightKey = SDL_SCANCODE_D
	};

	CameraControl(Camera& camera) : _camera(camera) {}

	float _moveSpeed{ 3.5f };

	float _rotationSpeed{ 3.5f };

	glm::vec3 _cameraPosition{ 0.f };

	glm::vec3 _cameraFront{ 0.f, 0.f, 1.f };

	glm::vec3 _cameraUp{ 0.f, -1.f, 0.f };

	glm::vec3 _cameraRight{ 1.f, 0.f, 0.f };

	void update(float deltaTime);

  private:
	float _yaw{ 0.f };
	float _pitch{ 0.f };

	Camera& _camera;

	void moveCamera(float deltaTime);

	void updateCameraAxis(float deltaTime);
};

} // namespace srtv_engine
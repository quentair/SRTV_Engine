#include "camera_control.h"

#include <limits>

namespace srtv_engine {

void CameraControl::update(float deltaTime)
{
	moveCamera(deltaTime);
	updateCameraAxis(deltaTime);

	//_camera.setCameraDirection(_cameraPosition, _cameraFront);
}
	
void CameraControl::moveCamera(float deltaTime)
{
	glm::vec3 translation{ 0.f };

	const bool* key_states = SDL_GetKeyboardState(NULL);

	// writing our code such that it sees opposite keys are pressed and cancels each other out
	// translate in camera space
	if (key_states[moveForwardKey]) {
		translation += _cameraFront;
	}

	if (key_states[moveBackwardKey]) {
		translation -= _cameraFront;
	}

	if (key_states[moveLeftKey]) {
		translation -= _cameraRight;
	}

	if (key_states[moveRightKey]) {
		translation += _cameraRight;
	}

	// check if vector is not null to avoid weird behaviour
	if (glm::dot(translation, translation) > std::numeric_limits<float>::epsilon()) {
		_cameraPosition += glm::normalize(translation) * _moveSpeed * deltaTime;
	}
}
	
void CameraControl::updateCameraAxis(float deltaTime)
{
	float xRel;
	float yRel;

	SDL_GetRelativeMouseState(&xRel, &yRel);

	_yaw += xRel * _rotationSpeed * deltaTime;
	_pitch -= yRel * _rotationSpeed * deltaTime; // negative because SDL screen y axis is upside down

	// limit angles
	if (_pitch > 89.f)
		_pitch = 89.f;
	else if (_pitch < -89.f)
		_pitch = -89.f;

	// avoid yaw angle overflow
	_yaw = fmod(_yaw, 360);

	glm::vec3 viewDirection{ 0.f };
	viewDirection.x = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	viewDirection.y = sin(glm::radians(_pitch));
	viewDirection.z = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));

	_cameraFront = glm::normalize(viewDirection);

	_cameraRight = glm::normalize(glm::cross(_cameraFront, glm::vec3{0.f, -1.f, 0.f}));

	_cameraUp = glm::normalize(glm::cross(_cameraRight, _cameraFront));
}

} // namespace srtv_engine
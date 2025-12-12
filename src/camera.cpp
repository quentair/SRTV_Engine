#include "camera.h"

namespace srtv_engine {

void srtv_engine::Camera::orthographicProjection(float left, float right, float top, float bottom, float near, float far)
{
	_projectionMatrix = glm::mat4{ 1.0f };

	_projectionMatrix[0][0] = 2.f / (right - left);

	_projectionMatrix[1][1] = 2.f / (bottom - top);

	_projectionMatrix[2][2] = 1.f / (far - near);

	_projectionMatrix[3][0] = -(right + left) / (right - left);
	_projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
	_projectionMatrix[3][2] = -near / (far - near);
}

void srtv_engine::Camera::perspectiveProjection(float fovY, float aspectRatio, float near, float far)
{
	const float tanHalfFovY = tan(fovY / 2.f);

	_projectionMatrix = glm::mat4{ 0.0f };

	_projectionMatrix[0][0] = 1.f / (aspectRatio * tanHalfFovY);

	_projectionMatrix[1][1] = 1.f / (tanHalfFovY);

	_projectionMatrix[2][2] = far / (far - near);
	_projectionMatrix[2][3] = 1.f;

	_projectionMatrix[3][2] = -(far * near) / (far - near);
}

void Camera::setCameraDirection(glm::vec3 cameraPosition, glm::vec3 cameraDirection, glm::vec3 cameraUp)
{
	// construct a rotation matrix with camera orthonormal basis
	// it is transposed to rotate from camera orientation to Vulkan axis aligned orientation
	const glm::vec3 w{ glm::normalize(cameraDirection) };
	const glm::vec3 u{ glm::normalize(glm::cross(w, cameraUp)) };
	const glm::vec3 v{ glm::cross(w, u) };

	_viewMatrix = glm::mat4{ 1.f };
	_viewMatrix[0][0] = u.x;
	_viewMatrix[1][0] = u.y;
	_viewMatrix[2][0] = u.z;
	_viewMatrix[0][1] = v.x;
	_viewMatrix[1][1] = v.y;
	_viewMatrix[2][1] = v.z;
	_viewMatrix[0][2] = w.x;
	_viewMatrix[1][2] = w.y;
	_viewMatrix[2][2] = w.z;
	_viewMatrix[3][0] = -glm::dot(u, cameraPosition);
	_viewMatrix[3][1] = -glm::dot(v, cameraPosition);
	_viewMatrix[3][2] = -glm::dot(w, cameraPosition);
}

void Camera::setCameraTarget(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp)
{
	// camera direction is just the vector from camera to target
	setCameraDirection(cameraPosition, cameraTarget - cameraPosition, cameraUp);
}

void Camera::setCameraYXZ(glm::vec3 cameraPosition, glm::vec3 cameraRotation)
{
	// use Tait-Bryan Y(1), X(2), Z(3) angles, same as model matrix
	const float c1 = glm::cos(cameraRotation.y);
	const float c2 = glm::cos(cameraRotation.x);
	const float c3 = glm::cos(cameraRotation.z);
	const float s1 = glm::sin(cameraRotation.y);
	const float s2 = glm::sin(cameraRotation.x);
	const float s3 = glm::sin(cameraRotation.z);

	// inverse = transpose for rotation matrix
	// using model matrix transpose means we go from camera orientation to Vulkan Z axis-aligned orientation
	const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
	const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
	const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };

	_viewMatrix = glm::mat4{ 1.f };
	_viewMatrix[0][0] = u.x;
	_viewMatrix[1][0] = u.y;
	_viewMatrix[2][0] = u.z;
	_viewMatrix[0][1] = v.x;
	_viewMatrix[1][1] = v.y;
	_viewMatrix[2][1] = v.z;
	_viewMatrix[0][2] = w.x;
	_viewMatrix[1][2] = w.y;
	_viewMatrix[2][2] = w.z;
	_viewMatrix[3][0] = -glm::dot(u, cameraPosition);
	_viewMatrix[3][1] = -glm::dot(v, cameraPosition);
	_viewMatrix[3][2] = -glm::dot(w, cameraPosition);
}

} // namespace srtv_engine
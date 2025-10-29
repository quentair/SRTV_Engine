#pragma once

#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>

namespace srtv_engine {

class TransformComponent {
  public:
	glm::vec3 _translation{ 0.f, 0.f, 0.f };
	glm::vec3 _scale{ 1.f, 1.f, 1.f };
	glm::vec3 _rotation{};

	glm::mat4 getMat4() {
		// classic translate * Ry * Rx * Rz * scale transform matrix
		// use Tait-Bryan Y(1), X(2), Z(3) angles
		float c1 = cos(_rotation.y);
		float c2 = cos(_rotation.x);
		float c3 = cos(_rotation.z);
		float s1 = sin(_rotation.y);
		float s2 = sin(_rotation.x);
		float s3 = sin(_rotation.z);

		// glm uses column major
		glm::mat4 transform = {
			{
				_scale.x * (c1 * c3 + s1 * s2 * s3),
				_scale.x * (c2 * s3),
				_scale.x * (c1 * s2 * s3 - c3 * s1),
				0.f
			},
			{
				_scale.y * (c3 * s1 * s2 - c1 * s3),
				_scale.y * (c2 * c3),
				_scale.y * (c1 * c3 * s2 + s1 * s3),
				0.f
			},
			{
				_scale.z * (c2 * s1),
				_scale.z * (-s2),
				_scale.z * (c1 * c2),
				0.f
			},
			{_translation, 1.f}
		};

		//glm::mat4 transform = glm::mat4{ 1.f };
		//transform = glm::translate(transform, translation);
		//transform = glm::rotate(transform, rotation.y, { 0.f, 1.f, 0.f });
		//transform = glm::rotate(transform, rotation.x, { 1.f, 0.f, 0.f });
		//transform = glm::rotate(transform, rotation.z, { 0.f, 0.f, 1.f });
		//transform = glm::scale(transform, scale);

		return transform;
	}



};

} // namespace srtv_engine
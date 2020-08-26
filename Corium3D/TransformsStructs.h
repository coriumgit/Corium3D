#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <complex>

namespace Corium3D {
	struct Transform3D {
		glm::vec3 translate = { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
		glm::quat rot = { 1.0, 0.0f, 0.0f, 0.0f };

		Transform3D() {};
		Transform3D(glm::vec3 _translate, glm::vec3 _scale, glm::quat _rot) : translate(_translate), scale(_scale), rot(_rot) {}		
	};

	struct Transform2D {
		glm::vec2 translate = { 0.0f, 0.0f };
		glm::vec2 scale = { 1.0f, 1.0f };
		std::complex<float> rot = std::complex<float>(1.0f, 0.0);

		//Transform2D() {};
		//Transform2D(glm::vec2 _translate, glm::vec2 _scale, std::complex<float> _rot) : translate(_translate), scale(_scale), rot(_rot) {}
		//Transform2D(Transform3D t) : translate(t.translate.x, t.translate.y), scale(t.scale.x, t.scale.y), rot(t.rot.w, t.rot.z) {}
	};	

	struct Rect {
		glm::vec2 lowerLeft;
		glm::vec2 upperRight;
	};
}
#pragma once

#include <glm/glm.hpp>

namespace Interops = System::Runtime::InteropServices;
namespace Media3D = System::Windows::Media::Media3D;
namespace Win = System::Windows;

namespace Corium3D {

	inline glm::vec3 marshalPoint3D(Media3D::Point3D^ src) {
		return glm::vec3{ src->X, src->Y ,src->Z };
	}

	inline glm::vec3 marshalVector3D(Media3D::Vector3D^ src) {
		return glm::vec3{ src->X, src->Y ,src->Z };
	}

	inline glm::vec2 marshalPoint(Win::Point^ src) {
		return glm::vec2{ src->X, src->Y };
	}

	inline glm::vec2 marshalVector(Win::Vector^ src) {
		return glm::vec2{ src->X, src->Y };
	}

	inline glm::quat marshalQuat(Media3D::Quaternion^ src) {
		return glm::quat{ (float)src->W, (float)src->X, (float)src->Y, (float)src->Z };
	}

	inline char* systemStringToAnsiString(System::String^ str) {
		return static_cast<char*>(Interops::Marshal::StringToHGlobalAnsi(str).ToPointer());
	}
}
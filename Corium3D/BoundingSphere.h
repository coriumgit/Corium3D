#pragma once

#include "TransformsStructs.h"

#include <glm/glm.hpp>

namespace Corium3D {

	class BoundingSphere {
	public:
		BoundingSphere();
		BoundingSphere(glm::vec3 center, float radius);
		glm::vec3 const& getCenter() const { return c; }
		float getRadius() const { return r; }
		BoundingSphere& transform(Transform3DUS const& transform);
		BoundingSphere& translate(glm::vec3 const& translate);
		BoundingSphere& scale(float scaleFactor);
		BoundingSphere& rotate(glm::quat const& rot);
		BoundingSphere& combine(BoundingSphere& combinedSphere);

		static BoundingSphere calcBoundingSphereExact(glm::vec3 verticesArr[], unsigned int verticesNr);
		static BoundingSphere calcBoundingSphereEfficient(glm::vec3 verticesArr[], unsigned int verticesNr);
		static BoundingSphere calcTransformedBoundingSphere(BoundingSphere const& original, Transform3DUS const& transform);
		static BoundingSphere calcTransformedBoundingSphere(BoundingSphere const& original, Transform3D const& transform);		
		static BoundingSphere calcCombinedBoundingSphere(BoundingSphere const& sphere1, BoundingSphere const& sphere2);

	private:
		BoundingSphere(glm::vec3* p1);
		BoundingSphere(glm::vec3* p1, glm::vec3* p2);
		BoundingSphere(glm::vec3* p1, glm::vec3* p2, glm::vec3* p3);
		BoundingSphere(glm::vec3* p1, glm::vec3* p2, glm::vec3* p3, glm::vec3* p4);
		glm::vec3 c; // center
		float r; // radius	
		glm::vec3 offset; // offset from parent center;

		static BoundingSphere recurseCalcBoundingSphere(glm::vec3** verticesArr, size_t verticesNr, size_t spherePointsNr);
	};

} //namespace Corium3D
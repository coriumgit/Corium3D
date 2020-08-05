#pragma once

#include <glm/glm.hpp>

namespace Corium3D {

	class BoundingSphere {
	public:
		BoundingSphere();
		BoundingSphere(glm::vec3 center, float radius);
		glm::vec3 const& getCenter() const { return c; }
		float getRadius() const { return r; }
		BoundingSphere& transform(glm::vec3 const& translate, glm::vec3 const& scale);
		BoundingSphere& translate(glm::vec3 const& translate);
		BoundingSphere& scale(glm::vec3 const& scale);
		BoundingSphere& combine(BoundingSphere& combinedSphere);

		static BoundingSphere calcBoundingSphereExact(glm::vec3 verticesArr[], unsigned int verticesNr);
		static BoundingSphere calcBoundingSphereEfficient(glm::vec3 verticesArr[], unsigned int verticesNr);
		static BoundingSphere calcTransformedBoundingSphere(BoundingSphere const& original, glm::vec3 translate, glm::vec3 scale);
		static BoundingSphere calcCombinedBoundingSphere(BoundingSphere const& sphere1, BoundingSphere const& sphere2);

	private:
		BoundingSphere(glm::vec3* p1);
		BoundingSphere(glm::vec3* p1, glm::vec3* p2);
		BoundingSphere(glm::vec3* p1, glm::vec3* p2, glm::vec3* p3);
		BoundingSphere(glm::vec3* p1, glm::vec3* p2, glm::vec3* p3, glm::vec3* p4);
		glm::vec3 c; // center
		float r; // radius	

		static BoundingSphere recurseCalcBoundingSphere(glm::vec3** verticesArr, size_t verticesNr, size_t spherePointsNr);
	};

} //namespace Corium3D
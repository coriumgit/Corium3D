#include "AABB.h"

#include <glm/gtc/matrix_access.hpp>

namespace Corium3D {

	AABB3D::AABB3D(glm::vec3 const& _minVertex, glm::vec3 const& _maxVertex) : minVertex(_minVertex), maxVertex(_maxVertex) {
		glm::vec3 sidesLens = maxVertex - minVertex;
		surface = sidesLens.x * sidesLens.y * sidesLens.z;
	}

	AABB3D& AABB3D::transform(glm::vec3 const& _translate, glm::vec3 const& _scale) {
		translate(_translate);
		scale(_scale);

		return *this;
	}

	AABB3D& AABB3D::translate(glm::vec3 const& translate) {
		minVertex += translate;
		maxVertex += translate;

		return *this;
	}

	AABB3D& AABB3D::scale(glm::vec3 const& scale) {
		surface *= scale.x * scale.y * scale.z;
		glm::vec3 midPoint = 0.5f * (minVertex + maxVertex);
		glm::vec3 halfDiagonalVec = 0.5f * scale * (maxVertex - minVertex);
		minVertex = midPoint - halfDiagonalVec;
		maxVertex = midPoint + halfDiagonalVec;

		return *this;
	}

	bool AABB3D::isIntersectedBySeg(glm::vec3 const& segStart, glm::vec3 const& segEnd) {
		glm::vec3 extent = maxVertex - minVertex;
		glm::vec3 centerAbs = abs(0.5f * (minVertex + maxVertex));
		for (unsigned int axIdx = 0; axIdx < 3; axIdx++) {
			if (abs(segStart[axIdx] + segEnd[axIdx]) > abs(segStart[axIdx] - segEnd[axIdx]) + 2 * (centerAbs[axIdx] + extent[axIdx]))
				return false;
		}

		// vi = ei X segVec
		// testing for each vi:
		//		|vi*segStart| > |vi*extent|
		glm::vec3 segVec = segEnd - segStart;
		if (abs(segVec.z * segStart.y - segVec.y * segStart.z) > abs(segVec.z * extent.y - segVec.y * extent.z) ||
			abs(-segVec.z * segStart.x + segVec.x * segStart.z) > abs(-segVec.z * extent.x + segVec.x * extent.z) ||
			abs(segVec.y * segStart.x - segVec.x * segStart.y) > abs(segVec.y * extent.x - segVec.x * extent.y))
			return false;

		return true;
	}

	AABB3D AABB3D::calcAABB(glm::vec3 verticesArr[], size_t verticesNr) {
		glm::vec3 minVertex(verticesArr[0]), maxVertex(verticesArr[0]);

		for (unsigned int vertexIdx = 1; vertexIdx < verticesNr; vertexIdx++) {
			glm::vec3 vertex = verticesArr[vertexIdx];
			if (vertex.x < minVertex.x)
				minVertex.x = vertex.x;
			else if (vertex.x > maxVertex.x)
				maxVertex.x = vertex.x;

			if (vertex.y < minVertex.y)
				minVertex.y = vertex.y;
			else if (vertex.y > maxVertex.y)
				maxVertex.y = vertex.y;

			if (vertex.z < minVertex.z)
				minVertex.z = vertex.z;
			else if (vertex.z > maxVertex.z)
				maxVertex.z = vertex.z;
		}

		return AABB3D(minVertex, maxVertex);
	}

	AABB3D AABB3D::calcTransformedAABB(AABB3D const& original, glm::vec3 const& translate, glm::vec3 const& scale) {
		AABB3D newAABB(original);
		return newAABB.transform(translate, scale);
	}

	AABB3D AABB3D::calcTranslatedAABB(AABB3D const& original, glm::vec3 const& translate) {
		AABB3D newAABB(original);
		return newAABB.translate(translate);
	}

	AABB3D AABB3D::calcScaledAABB(AABB3D const& original, glm::vec3 const& scale) {
		AABB3D newAABB(original);
		return newAABB.scale(scale);
	}

	AABB3D AABB3D::calcCombinedAABB(AABB3D const& aabb1, AABB3D const& aabb2) {
		AABB3D newAABB(aabb1);
		return newAABB.combine(aabb2);
	}

	AABB3D& AABB3D::combine(AABB const& _other) {
		AABB3D const& other = dynamic_cast<AABB3D const&>(_other);
		maxVertex.x = fmax(maxVertex.x, other.maxVertex.x);
		maxVertex.y = fmax(maxVertex.y, other.maxVertex.y);
		maxVertex.z = fmax(maxVertex.z, other.maxVertex.z);
		minVertex.x = fmin(minVertex.x, other.minVertex.x);
		minVertex.y = fmin(minVertex.y, other.minVertex.y);
		minVertex.z = fmin(minVertex.z, other.minVertex.z);
		glm::vec3 SidesLens = maxVertex - minVertex;
		surface = SidesLens.x * SidesLens.y * SidesLens.z;

		return *this;
	}

	float AABB3D::calcCombinedAabbSurface(AABB const& other) const {
		AABB3D combinedAABB(*this);
		return combinedAABB.combine(other).surface;
	}

	bool AABB3D::doesContain(AABB const& _other) const {
		AABB3D const& other = dynamic_cast<AABB3D const&>(_other);
		return (maxVertex.x > other.maxVertex.x) &&
			(maxVertex.y > other.maxVertex.y) &&
			(maxVertex.z > other.maxVertex.z) &&
			(minVertex.x < other.minVertex.x) &&
			(minVertex.y < other.minVertex.y) &&
			(minVertex.z < other.minVertex.z);
	}

	bool AABB3D::doesIntersect(AABB const& _other) const {
		AABB3D const& other = dynamic_cast<AABB3D const&>(_other);
		return !(maxVertex.x < other.minVertex.x || other.maxVertex.x < minVertex.x ||
			maxVertex.y < other.minVertex.y || other.maxVertex.y < minVertex.y ||
			maxVertex.z < other.minVertex.z || other.maxVertex.z < minVertex.z);
	}

	AABB3DRotatable::AABB3DRotatable(glm::vec3 const& _minVertex, glm::vec3 const& _maxVertex) :
		AABB3D(_minVertex, _maxVertex), unrotatedHalfDims(0.5f * (maxVertex - minVertex)) {}

	AABB3DRotatable& AABB3DRotatable::transform(glm::vec3 const& translate, glm::mat3& rotMat) {
		minVertex += translate;
		maxVertex += translate;
		/* find the extreme points by considering the product of the */
		/* min and max with each component of M. */
		for (unsigned int rowIdx = 0; rowIdx < 3; rowIdx++) {
			glm::vec3 rotMatCol(row(rotMat, rowIdx));
			for (unsigned int colIdx = 0; colIdx < 3; colIdx++) {
				float a = rotMatCol[colIdx] * minVertex[colIdx];
				float b = rotMatCol[colIdx] * maxVertex[colIdx];
				if (b < a) {
					maxVertex += a;
					minVertex += b;
				}
				else {
					maxVertex += b;
					minVertex += a;
				}
			}
		}

		return *this;
	}

	AABB3DRotatable& AABB3DRotatable::transform(Corium3D::Transform3D const& transformDelta) {
		unrotatedHalfDims *= transformDelta.scale;
		surface *= transformDelta.scale.x * transformDelta.scale.y * transformDelta.scale.z;
		rotate(transformDelta.rot);
		maxVertex += transformDelta.translate;
		minVertex += transformDelta.translate;

		return *this;
	}

	AABB3DRotatable& AABB3DRotatable::translate(glm::vec3 const& translate) {
		minVertex += translate;
		maxVertex += translate;

		return *this;
	}

	AABB3DRotatable& AABB3DRotatable::scale(glm::vec3 const& scale) {
		unrotatedHalfDims *= scale;
		surface *= scale.x * scale.y * scale.z;
		if (rotQuat.w != 1.0f && rotQuat.w != -1.0f)
			rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)); // reuse the resize code in rotate(quat&)
		else {
			glm::vec3 midPoint = 0.5f * (minVertex + maxVertex);
			minVertex = midPoint - unrotatedHalfDims;
			maxVertex = midPoint + unrotatedHalfDims;
		}

		return *this;
	}

	AABB3DRotatable& AABB3DRotatable::rotate(glm::quat const& rot) {
		rotQuat = rot * rotQuat;
		glm::vec3 vecToNewMax(glm::abs(rotQuat * glm::vec3(unrotatedHalfDims.x, 0.0f, 0.0f)) +
							  glm::abs(rotQuat * glm::vec3(0.0f, unrotatedHalfDims.y, 0.0f)) +
							  glm::abs(rotQuat * glm::vec3(0.0f, 0.0f, unrotatedHalfDims.z)));

		glm::vec3 midPoint = 0.5f * (minVertex + maxVertex);
		minVertex = midPoint - vecToNewMax;
		maxVertex = midPoint + vecToNewMax;
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", (std::string("minVertex = (") + std::to_string(minVertex.x) + std::string(", ") + std::to_string(minVertex.y) + std::string(", ") + std::to_string(minVertex.z) + std::string(")")).c_str());
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", (std::string("maxVertex = (") + std::to_string(maxVertex.x) + std::string(", ") + std::to_string(maxVertex.y) + std::string(", ") + std::to_string(maxVertex.z) + std::string(")")).c_str());
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", "----------------------------------------------------------------------------------------");

		return *this;
	}

	AABB3DRotatable AABB3DRotatable::calcAABB(glm::vec3 verticesArr[], size_t verticesNr) {
		AABB3D const& newAABB(AABB3D::calcAABB(verticesArr, verticesNr));
		return AABB3DRotatable(newAABB.getMinVertex(), newAABB.getMaxVertex());
	}

	AABB3DRotatable AABB3DRotatable::calcTransformedAABB(AABB3DRotatable const& original, Corium3D::Transform3D const& transform) {
		AABB3DRotatable newAABB(original);
		return newAABB.transform(transform);
	}

	AABB3DRotatable AABB3DRotatable::calcTranslatedAABB(AABB3DRotatable const& original, glm::vec3 const& translate) {
		AABB3DRotatable newAABB(original);
		return newAABB.translate(translate);
	}

	AABB3DRotatable AABB3DRotatable::calcScaledAABB(AABB3DRotatable const& original, glm::vec3 const& scale) {
		AABB3DRotatable newAABB(original);
		return newAABB.scale(scale);
	}

	AABB3DRotatable AABB3DRotatable::calcRotatedAABB(AABB3DRotatable const& original, glm::quat const& rot) {
		AABB3DRotatable newAABB(original);
		return newAABB.rotate(rot);
	}

	AABB3DRotatable AABB3DRotatable::calcCombinedAABB(AABB3DRotatable const& aabb1, AABB3DRotatable const& aabb2) {
		AABB3DRotatable newAABB(aabb1);
		return newAABB.combine(aabb2);
	}

	AABB3DRotatable& AABB3DRotatable::combine(AABB const& combinedAABB) {
		AABB3D::combine(combinedAABB);
		unrotatedHalfDims = maxVertex - minVertex;

		return *this;
	}

	AABB2D::AABB2D(glm::vec2 const& _minVertex, glm::vec2 const& _maxVertex) : minVertex(_minVertex), maxVertex(_maxVertex) {
		glm::vec2 sidesLens = maxVertex - minVertex;
		surface = sidesLens.x * sidesLens.y;
	}

	AABB2D& AABB2D::transform(glm::vec2 const& _translate, glm::vec2 const& _scale) {
		translate(_translate);
		scale(_scale);

		return *this;
	}

	AABB2D& AABB2D::translate(glm::vec2 const& translate) {
		minVertex += translate;
		maxVertex += translate;

		return *this;
	}

	AABB2D& AABB2D::scale(glm::vec2 const& scale) {
		surface *= scale.x * scale.y;
		glm::vec2 midPoint = 0.5f * (minVertex + maxVertex);
		glm::vec2 halfDiagonalVec = 0.5f * scale * (maxVertex - minVertex);
		minVertex = midPoint - halfDiagonalVec;
		maxVertex = midPoint + halfDiagonalVec;

		return *this;
	}

	AABB2D AABB2D::calcAABB(glm::vec3 verticesArr[], size_t verticesNr) {
		glm::vec2 minVertex(verticesArr[0]), maxVertex(verticesArr[0]);

		for (unsigned int vertexIdx = 1; vertexIdx < verticesNr; vertexIdx++) {
			glm::vec3 vertex = verticesArr[vertexIdx];
			if (vertex.x < minVertex.x)
				minVertex.x = vertex.x;
			else if (vertex.x > maxVertex.x)
				maxVertex.x = vertex.x;

			if (vertex.y < minVertex.y)
				minVertex.y = vertex.y;
			else if (vertex.y > maxVertex.y)
				maxVertex.y = vertex.y;
		}

		return AABB2D(minVertex, maxVertex);
	}

	AABB2D AABB2D::calcTransformedAABB(AABB2D const& original, glm::vec2 const& translate, glm::vec2 const& scale) {
		AABB2D newAABB(original);
		return newAABB.transform(translate, scale);
	}

	AABB2D AABB2D::calcTranslatedAABB(AABB2D const& original, glm::vec2 const& translate) {
		AABB2D newAABB(original);
		return newAABB.translate(translate);
	}

	AABB2D AABB2D::calcScaledAABB(AABB2D const& original, glm::vec2 const& scale) {
		AABB2D newAABB(original);
		return newAABB.scale(scale);
	}

	AABB2D AABB2D::calcCombinedAABB(AABB2D const& aabb1, AABB2D const& aabb2) {
		AABB2D newAABB(aabb1);
		return newAABB.combine(aabb2);
	}

	AABB2D& AABB2D::combine(AABB const& _other) {
		AABB2D const& other = dynamic_cast<AABB2D const&>(_other);
		maxVertex.x = fmax(maxVertex.x, other.maxVertex.x);
		maxVertex.y = fmax(maxVertex.y, other.maxVertex.y);
		minVertex.x = fmin(minVertex.x, other.minVertex.x);
		minVertex.y = fmin(minVertex.y, other.minVertex.y);
		glm::vec2 SidesLens = maxVertex - minVertex;
		surface = SidesLens.x * SidesLens.y;

		return *this;
	}

	float AABB2D::calcCombinedAabbSurface(AABB const& other) const {
		AABB2D combinedAABB(*this);
		return combinedAABB.combine(other).surface;
	}

	bool AABB2D::doesContain(AABB const& _other) const {
		AABB2D const& other = dynamic_cast<AABB2D const&>(_other);
		return (maxVertex.x > other.maxVertex.x) &&
			(maxVertex.y > other.maxVertex.y) &&
			(minVertex.x < other.minVertex.x) &&
			(minVertex.y < other.minVertex.y);
	}

	bool AABB2D::doesIntersect(AABB const& _other) const {
		AABB2D const& other = dynamic_cast<AABB2D const&>(_other);
		return !(maxVertex.x < other.minVertex.x || other.maxVertex.x < minVertex.x ||
			maxVertex.y < other.minVertex.y || other.maxVertex.y < minVertex.y);
	}

	AABB2DRotatable::AABB2DRotatable(glm::vec2 const& minVertex, glm::vec2 const& maxVertex) :
		AABB2D(minVertex, maxVertex), unrotatedHalfDims(0.5f * (maxVertex - minVertex)) {}

	AABB2DRotatable& AABB2DRotatable::transform(glm::vec2 const& translate, glm::mat2 const& rotMat) {
		minVertex += translate;
		maxVertex += translate;
		/* find the extreme points by considering the product of the */
		/* min and max with each component of M. */
		for (unsigned int rowIdx = 0; rowIdx < 2; rowIdx++) {
			glm::vec2 rotMatCol(row(rotMat, rowIdx));
			for (unsigned int colIdx = 0; colIdx < 2; colIdx++) {
				float a = rotMatCol[colIdx] * minVertex[colIdx];
				float b = rotMatCol[colIdx] * maxVertex[colIdx];
				if (b < a) {
					maxVertex += a;
					minVertex += b;
				}
				else {
					maxVertex += b;
					minVertex += a;
				}
			}
		}

		return *this;
	}

	AABB2DRotatable& AABB2DRotatable::transform(Corium3D::Transform2D const& transformDelta) {
		unrotatedHalfDims *= transformDelta.scale;
		surface *= transformDelta.scale.x * transformDelta.scale.y;
		rotate(transformDelta.rot);
		maxVertex += transformDelta.translate;
		minVertex += transformDelta.translate;

		return *this;
	}

	AABB2DRotatable& AABB2DRotatable::translate(glm::vec2 const& translate) {
		minVertex += translate;
		maxVertex += translate;

		return *this;
	}

	AABB2DRotatable& AABB2DRotatable::scale(glm::vec2 const& scale) {
		unrotatedHalfDims *= scale;
		surface *= scale.x * scale.y;
		if (rotComplex.real() != 1.0f)
			rotate(std::complex<float>(1.0f, 0.0f)); // reuse the resize code in rotate(complex&)
		else {
			glm::vec2 midPoint = 0.5f * (minVertex + maxVertex);
			minVertex = midPoint - unrotatedHalfDims;
			maxVertex = midPoint + unrotatedHalfDims;
		}

		return *this;
	}

	inline glm::vec2 operator*(std::complex<float> const& c, glm::vec2 const& v) {
		return glm::vec2(c.real() * v.x - c.imag() * v.y, c.real() * v.y + c.imag() * v.x);
	}

	AABB2DRotatable& AABB2DRotatable::rotate(std::complex<float> const& rot) {
		rotComplex = rot * rotComplex;
		glm::vec2 vecToNewMax(glm::abs(rotComplex * glm::vec2(unrotatedHalfDims.x, 0.0f)) +
			glm::abs(rotComplex * glm::vec2(0.0f, unrotatedHalfDims.y)));

		glm::vec2 midPoint = 0.5f * (minVertex + maxVertex);
		minVertex = midPoint - vecToNewMax;
		maxVertex = midPoint + vecToNewMax;
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", (std::string("minVertex = (") + std::to_string(minVertex.x) + std::string(", ") + std::to_string(minVertex.y) + std::string(", ") + std::to_string(minVertex.z) + std::string(")")).c_str());
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", (std::string("maxVertex = (") + std::to_string(maxVertex.x) + std::string(", ") + std::to_string(maxVertex.y) + std::string(", ") + std::to_string(maxVertex.z) + std::string(")")).c_str());
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", "----------------------------------------------------------------------------------------");

		return *this;
	}

	AABB2DRotatable AABB2DRotatable::calcAABB(glm::vec3 verticesArr[], size_t verticesNr) {
		AABB2D const& newAABB(AABB2D::calcAABB(verticesArr, verticesNr));
		return AABB2DRotatable(newAABB.getMinVertex(), newAABB.getMaxVertex());
	}

	AABB2DRotatable AABB2DRotatable::calcTransformedAABB(AABB2DRotatable const& original, Corium3D::Transform2D const& transform) {
		AABB2DRotatable newAABB(original);
		return newAABB.transform(transform);
	}

	AABB2DRotatable AABB2DRotatable::calcTranslatedAABB(AABB2DRotatable const& original, glm::vec2 const& translate) {
		AABB2DRotatable newAABB(original);
		return newAABB.translate(translate);
	}

	AABB2DRotatable AABB2DRotatable::calcScaledAABB(AABB2DRotatable const& original, glm::vec2 const& scale) {
		AABB2DRotatable newAABB(original);
		return newAABB.scale(scale);
	}

	AABB2DRotatable AABB2DRotatable::calcRotatedAABB(AABB2DRotatable const& original, std::complex<float> rot) {
		AABB2DRotatable newAABB(original);
		return newAABB.rotate(rot);
	}

	AABB2DRotatable AABB2DRotatable::calcCombinedAABB(AABB2DRotatable const& aabb1, AABB2DRotatable const& aabb2) {
		AABB2DRotatable newAABB(aabb1);
		return newAABB.combine(aabb2);
	}

	AABB2DRotatable& AABB2DRotatable::combine(AABB const& combinedAABB) {
		AABB2D::combine(combinedAABB);
		unrotatedHalfDims = maxVertex - minVertex;

		return *this;
	}

} // namespace Corium3D
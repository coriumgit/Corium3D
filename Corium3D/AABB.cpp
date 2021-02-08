#include "AABB.h"

#include <glm/gtc/matrix_access.hpp>

namespace Corium3D {

	AABB3D::AABB3D(glm::vec3 const& _minVertex, glm::vec3 const& _maxVertex) : minVertex(_minVertex), maxVertex(_maxVertex), offset(0.5f * (_minVertex + _maxVertex)) {
		glm::vec3 sidesLens = maxVertex - minVertex;
		surface = sidesLens.x * sidesLens.y * sidesLens.z;
	}	

	AABB3D& AABB3D::transform(glm::vec3 const& _translate, float scaleFactor) {
		translate(_translate);
		scale(scaleFactor);		

		return *this;
	}

	AABB3D& AABB3D::translate(glm::vec3 const& translate) {
		minVertex += translate;
		maxVertex += translate;

		return *this;
	}

	AABB3D& AABB3D::scale(float scaleFactor) {
		scale(glm::vec3(scaleFactor, scaleFactor, scaleFactor));

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

	AABB3D AABB3D::calcTransformedAABB(AABB3D const& original, glm::vec3 const& translate, float scaleFactor) {
		AABB3D newAABB(original);
		return newAABB.transform(translate, scaleFactor);
	}

	AABB3D AABB3D::calcTransformedAABB(AABB3D const& original, glm::vec3 const& translate, glm::vec3 const& scaleFactor)
	{
		AABB3D newAABB(original);
		newAABB.scale(scaleFactor);
		newAABB.translate(translate);

		return newAABB;
	}

	AABB3D AABB3D::calcTranslatedAABB(AABB3D const& original, glm::vec3 const& translate) {
		AABB3D newAABB(original);
		return newAABB.translate(translate);
	}

	AABB3D AABB3D::calcScaledAABB(AABB3D const& original, float scaleFactor) {
		AABB3D newAABB(original);
		return newAABB.scale(scaleFactor);
	}

	AABB3D AABB3D::calcScaledAABB(AABB3D const& original, glm::vec3 const& scaleFactor)
	{
		AABB3D newAABB(original);
		newAABB.scale(scaleFactor);

		return newAABB;
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

	void AABB3D::scale(glm::vec3 const& scaleFactor)
	{
		surface *= scaleFactor.x * scaleFactor.y * scaleFactor.z;
		glm::vec3 midPoint = 0.5f * (minVertex + maxVertex) - offset;
		offset *= scaleFactor;
		midPoint += offset;
		glm::vec3 halfDiagonalVec = 0.5f * scaleFactor * (maxVertex - minVertex);
		minVertex = midPoint - halfDiagonalVec;
		maxVertex = midPoint + halfDiagonalVec;		
	}

	AABB3DRotatable::AABB3DRotatable(glm::vec3 const& _minVertex, glm::vec3 const& _maxVertex) :
		AABB3D(_minVertex, _maxVertex), unrotatedHalfDims(0.5f * (maxVertex - minVertex)) {}

	/*
	AABB3DRotatable& AABB3DRotatable::transform(glm::vec3 const& translate, glm::mat3& rotMat) {
		minVertex += translate;
		maxVertex += translate;
		// find the extreme points by considering the product of the
		// min and max with each component of M.
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
	*/

	AABB3DRotatable& AABB3DRotatable::transform(Transform3DUS const& transform3DUS) {
		Transform3D transform3D { transform3DUS.translate, glm::vec3(transform3DUS.scale, transform3DUS.scale , transform3DUS.scale), transform3DUS.rot };
		transform(transform3D);

		return *this;
	}

	AABB3DRotatable& AABB3DRotatable::translate(glm::vec3 const& translate) {
		minVertex += translate;
		maxVertex += translate;

		return *this;
	}

	AABB3DRotatable& AABB3DRotatable::scale(float scaleFactor) {
		scale(glm::vec3(scaleFactor, scaleFactor, scaleFactor));

		return *this;
	}

	AABB3DRotatable& AABB3DRotatable::rotate(glm::quat const& rot) {
		rotQuat = rot * rotQuat;		
		glm::vec3 midPoint = 0.5f * (minVertex + maxVertex) - offset;
		offset = rotQuat * offset;
		midPoint += offset;		
		glm::vec3 vecToNewMax = compVecToMax();
		minVertex = midPoint - vecToNewMax;
		maxVertex = midPoint + vecToNewMax;
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", (std::string("minVertex = (") + std::to_string(minVertex.x) + std::string(", ") + std::to_string(minVertex.y) + std::string(", ") + std::to_string(minVertex.z) + std::string(")")).c_str());
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", (std::string("maxVertex = (") + std::to_string(maxVertex.x) + std::string(", ") + std::to_string(maxVertex.y) + std::string(", ") + std::to_string(maxVertex.z) + std::string(")")).c_str());
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", "----------------------------------------------------------------------------------------");

		return *this;
	}

	glm::vec3 AABB3DRotatable::compVecToMax()
	{
		return glm::vec3(glm::abs(rotQuat * glm::vec3(unrotatedHalfDims.x, 0.0f, 0.0f)) +
						 glm::abs(rotQuat * glm::vec3(0.0f, unrotatedHalfDims.y, 0.0f)) +
						 glm::abs(rotQuat * glm::vec3(0.0f, 0.0f, unrotatedHalfDims.z)));
	}

	void AABB3DRotatable::transform(Transform3D const& transform)
	{
		unrotatedHalfDims *= transform.scale;
		surface *= transform.scale.x * transform.scale.y * transform.scale.z;
		rotQuat = transform.rot * rotQuat;
		glm::vec3 midPoint = 0.5f * (minVertex + maxVertex) - offset;
		offset = transform.rot * (offset * transform.scale);
		midPoint += offset;
		glm::vec3 vecToMax = compVecToMax();
		minVertex = midPoint - vecToMax + transform.translate;
		maxVertex = midPoint + vecToMax + transform.translate;
	}

	void AABB3DRotatable::scale(glm::vec3 const& scaleFactor)
	{
		unrotatedHalfDims *= scaleFactor;
		surface *= scaleFactor.x * scaleFactor.y * scaleFactor.z;
		glm::vec3 midPoint = 0.5f * (minVertex + maxVertex) - offset;
		offset *= scaleFactor;
		midPoint += offset;
		glm::vec3 vecToMax = compVecToMax();
		minVertex = midPoint - vecToMax;
		maxVertex = midPoint + vecToMax;
	}

	AABB3DRotatable AABB3DRotatable::calcAABB(glm::vec3 verticesArr[], size_t verticesNr) {
		AABB3D const& newAABB(AABB3D::calcAABB(verticesArr, verticesNr));
		return AABB3DRotatable(newAABB.getMinVertex(), newAABB.getMaxVertex());
	}

	AABB3DRotatable AABB3DRotatable::calcTransformedAABB(AABB3DRotatable const& original, Transform3DUS const& transform) {
		AABB3DRotatable newAABB(original);
		newAABB.transform(transform);

		return newAABB;
	}

	AABB3DRotatable AABB3DRotatable::calcTransformedAABB(AABB3DRotatable const& original, Transform3D const& transform)
	{
		AABB3DRotatable newAABB(original);
		newAABB.transform(transform);		

		return newAABB;
	}

	AABB3DRotatable AABB3DRotatable::calcTranslatedAABB(AABB3DRotatable const& original, glm::vec3 const& translate) {
		AABB3DRotatable newAABB(original);
		newAABB.translate(translate);

		return newAABB;
	}

	AABB3DRotatable AABB3DRotatable::calcScaledAABB(AABB3DRotatable const& original, float scaleFactor) {
		AABB3DRotatable newAABB(original);
		newAABB.scale(scaleFactor);

		return newAABB;
	}

	AABB3DRotatable AABB3DRotatable::calcScaledAABB(AABB3DRotatable const& original, glm::vec3 const& scaleFactor)
	{
		AABB3DRotatable newAABB(original);
		newAABB.scale(scaleFactor);
		
		return newAABB;
	}

	AABB3DRotatable AABB3DRotatable::calcRotatedAABB(AABB3DRotatable const& original, glm::quat const& rot) {
		AABB3DRotatable newAABB(original);
		newAABB.rotate(rot);

		return newAABB;
	}

	AABB3DRotatable AABB3DRotatable::calcCombinedAABB(AABB3DRotatable const& aabb1, AABB3DRotatable const& aabb2) {
		AABB3DRotatable newAABB(aabb1);
		newAABB.combine(aabb2);

		return newAABB;
	}

	AABB3DRotatable& AABB3DRotatable::combine(AABB const& combinedAABB) {
		AABB3D::combine(combinedAABB);
		unrotatedHalfDims = maxVertex - minVertex;

		return *this;
	}

	AABB2D::AABB2D(glm::vec2 const& _minVertex, glm::vec2 const& _maxVertex) : minVertex(_minVertex), maxVertex(_maxVertex), offset(0.5f * (_minVertex + _maxVertex)) {
		glm::vec2 sidesLens = maxVertex - minVertex;
		surface = sidesLens.x * sidesLens.y;
	}

	AABB2D& AABB2D::transform(glm::vec2 const& _translate, float scaleFactor) {
		translate(_translate);
		scale(scaleFactor);

		return *this;
	}

	AABB2D& AABB2D::translate(glm::vec2 const& translate) {
		minVertex += translate;
		maxVertex += translate;

		return *this;
	}

	AABB2D& AABB2D::scale(float scaleFactor) {
		scale(glm::vec2(scaleFactor, scaleFactor));

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

	AABB2D AABB2D::calcTransformedAABB(AABB2D const& original, glm::vec2 const& translate, float scaleFactor) {
		AABB2D newAABB(original);
		newAABB.transform(translate, scaleFactor);

		return newAABB;
	}

	AABB2D AABB2D::calcTransformedAABB(AABB2D const& original, glm::vec2 const& translate, glm::vec2 const& scaleFactor)
	{
		AABB2D newAABB(original);
		newAABB.scale(scaleFactor);
		newAABB.translate(translate);

		return newAABB;
	}

	AABB2D AABB2D::calcTranslatedAABB(AABB2D const& original, glm::vec2 const& translate) {
		AABB2D newAABB(original);
		newAABB.translate(translate);

		return newAABB;
	}

	AABB2D AABB2D::calcScaledAABB(AABB2D const& original, float scaleFactor) {
		AABB2D newAABB(original);
		newAABB.scale(scaleFactor);

		return newAABB;
	}

	AABB2D AABB2D::calcScaledAABB(AABB2D const& original, glm::vec2 const& scaleFactor)
	{
		AABB2D newAABB(original);
		newAABB.scale(scaleFactor);

		return newAABB;
	}

	AABB2D AABB2D::calcCombinedAABB(AABB2D const& aabb1, AABB2D const& aabb2) {
		AABB2D newAABB(aabb1);
		newAABB.combine(aabb2);

		return newAABB;
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

	void AABB2D::scale(glm::vec2 const& scaleFactor)
	{		
		surface *= scaleFactor.x * scaleFactor.y;
		glm::vec2 midPoint = 0.5f * (minVertex + maxVertex) - offset;
		offset *= scaleFactor;
		midPoint += offset;
		glm::vec2 halfDiagonalVec = 0.5f * scaleFactor * (maxVertex - minVertex);
		minVertex = midPoint - halfDiagonalVec;
		maxVertex = midPoint + halfDiagonalVec;
	}

	AABB2DRotatable::AABB2DRotatable(glm::vec2 const& minVertex, glm::vec2 const& maxVertex) :
		AABB2D(minVertex, maxVertex), unrotatedHalfDims(0.5f * (maxVertex - minVertex)) {}

	/*
	AABB2DRotatable& AABB2DRotatable::transform(glm::vec2 const& translate, glm::mat2 const& rotMat) {
		minVertex += translate;
		maxVertex += translate;
		// find the extreme points by considering the product of the
		// min and max with each component of M.
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
	*/

	/*
	*	unrotatedHalfDims *= transform.scale;
		surface *= transform.scale * transform.scale * transform.scale;
		rotQuat = transform.rot * rotQuat;
		glm::vec3 vecToMax;
		if (rotQuat.w != 1.0f && rotQuat.w != -1.0f)
			vecToMax = compVecToMax();
		else
			vecToMax = unrotatedHalfDims;

		glm::vec3 midPoint = 0.5f * (minVertex + maxVertex) - offset;
		offset = rotQuat * offset * transform.scale;
		midPoint += offset;		
		minVertex = midPoint - vecToMax + transform.translate;
		maxVertex = midPoint + vecToMax + transform.translate;
	*/

	AABB2DRotatable& AABB2DRotatable::transform(Transform2DUS const& transform2DUS) {
		Transform2D transform2D{ transform2DUS.translate, glm::vec2(transform2DUS.scale, transform2DUS.scale), transform2DUS.rot };
		transform(transform2D);

		return *this;
	}	

	AABB2DRotatable& AABB2DRotatable::translate(glm::vec2 const& translate) {
		minVertex += translate;
		maxVertex += translate;

		return *this;
	}

	AABB2DRotatable& AABB2DRotatable::scale(float scaleFactor) {
		scale(glm::vec2(scaleFactor, scaleFactor));

		return *this;
	}

	inline glm::vec2 operator*(std::complex<float> const& c, glm::vec2 const& v) {
		return glm::vec2(c.real() * v.x - c.imag() * v.y, c.real() * v.y + c.imag() * v.x);
	}
	
	AABB2DRotatable& AABB2DRotatable::rotate(std::complex<float> const& rot) {
		rotComplex = rot * rotComplex;
		glm::vec2 midPoint = 0.5f * (minVertex + maxVertex) - offset;
		offset = rotComplex * offset;
		midPoint += offset;
		glm::vec2 vecToNewMax = compVecToMax();		
		minVertex = midPoint - vecToNewMax;
		maxVertex = midPoint + vecToNewMax;
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", (std::string("minVertex = (") + std::to_string(minVertex.x) + std::string(", ") + std::to_string(minVertex.y) + std::string(", ") + std::to_string(minVertex.z) + std::string(")")).c_str());
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", (std::string("maxVertex = (") + std::to_string(maxVertex.x) + std::string(", ") + std::to_string(maxVertex.y) + std::string(", ") + std::to_string(maxVertex.z) + std::string(")")).c_str());
		//ServiceLocator::getLogger().logd("AABBRotatable::rotate", "----------------------------------------------------------------------------------------");

		return *this;
	}

	glm::vec2 AABB2DRotatable::compVecToMax()
	{
		return glm::vec2(glm::abs(rotComplex * glm::vec2(unrotatedHalfDims.x, 0.0f)) +
						 glm::abs(rotComplex * glm::vec2(0.0f, unrotatedHalfDims.y)));
	}

	void AABB2DRotatable::transform(Transform2D const& transform)
	{
		unrotatedHalfDims *= transform.scale;
		surface *= transform.scale.x * transform.scale.y;
		rotComplex = transform.rot * rotComplex;
		glm::vec2 midPoint = 0.5f * (minVertex + maxVertex) - offset;
		offset = transform.rot * (offset * transform.scale);
		midPoint += offset;
		glm::vec2 vecToMax = compVecToMax();
		minVertex = midPoint - vecToMax + transform.translate;
		maxVertex = midPoint + vecToMax + transform.translate;
	}

	void AABB2DRotatable::scale(glm::vec2 const& scaleFactor)
	{
		unrotatedHalfDims *= scaleFactor;
		surface *= scaleFactor.x * scaleFactor.y;
		glm::vec2 midPoint = 0.5f * (minVertex + maxVertex) - offset;
		offset *= scaleFactor;
		midPoint += offset;
		glm::vec2 vecToMax = compVecToMax();
		minVertex = midPoint - vecToMax;
		maxVertex = midPoint + vecToMax;
	}

	AABB2DRotatable AABB2DRotatable::calcAABB(glm::vec3 verticesArr[], size_t verticesNr) {
		AABB2D const& newAABB(AABB2D::calcAABB(verticesArr, verticesNr));
		return AABB2DRotatable(newAABB.getMinVertex(), newAABB.getMaxVertex());
	}

	AABB2DRotatable AABB2DRotatable::calcTransformedAABB(AABB2DRotatable const& original, Transform2DUS const& transform) {
		AABB2DRotatable newAABB(original);
		newAABB.transform(transform);

		return newAABB;
	}

	AABB2DRotatable AABB2DRotatable::calcTransformedAABB(AABB2DRotatable const& original, Transform2D const& transform)
	{
		AABB2DRotatable newAABB(original);
		newAABB.transform(transform);

		return newAABB;	
	}

	AABB2DRotatable AABB2DRotatable::calcTranslatedAABB(AABB2DRotatable const& original, glm::vec2 const& translate) {
		AABB2DRotatable newAABB(original);
		newAABB.translate(translate);

		return newAABB;
	}

	AABB2DRotatable AABB2DRotatable::calcScaledAABB(AABB2DRotatable const& original, float scaleFactor) {
		AABB2DRotatable newAABB(original);
		newAABB.scale(scaleFactor);

		return newAABB;
	}

	AABB2DRotatable AABB2DRotatable::calcScaledAABB(AABB2DRotatable const& original, glm::vec2 const& scaleFactor)
	{
		AABB2DRotatable newAABB(original);
		newAABB.scale(scaleFactor);

		return newAABB;
	}

	AABB2DRotatable AABB2DRotatable::calcRotatedAABB(AABB2DRotatable const& original, std::complex<float> rot) {
		AABB2DRotatable newAABB(original);
		newAABB.rotate(rot);

		return newAABB;
	}

	AABB2DRotatable AABB2DRotatable::calcCombinedAABB(AABB2DRotatable const& aabb1, AABB2DRotatable const& aabb2) {
		AABB2DRotatable newAABB(aabb1);
		newAABB.combine(aabb2);

		return newAABB;
	}

	AABB2DRotatable& AABB2DRotatable::combine(AABB const& combinedAABB) {
		AABB2D::combine(combinedAABB);
		unrotatedHalfDims = maxVertex - minVertex;

		return *this;
	}

} // namespace Corium3D
#include "BoundingSphere.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/fast_square_root.hpp>
#include <math.h>

namespace Corium3D {

	const float EPSILON = 1e-4f;

	BoundingSphere::BoundingSphere() : c(0.0f, 0.0f, 0.0f), r(EPSILON) {}

	BoundingSphere::BoundingSphere(glm::vec3 center, float radius) : c(center), r(radius) {}

	BoundingSphere::BoundingSphere(glm::vec3* p1) : c(*p1), r(EPSILON) {}

	BoundingSphere::BoundingSphere(glm::vec3* p1, glm::vec3* p2) :
		c(0.5f * (*p1 + *p2)), r(length((c - *p1)) + EPSILON) {}

	BoundingSphere::BoundingSphere(glm::vec3* p1, glm::vec3* p2, glm::vec3* p3) {
		glm::vec3 a = *p1 - *p3;
		glm::vec3 b = *p2 - *p3;
		glm::vec3 aXb = glm::cross(a, b);
		float denominator = 2.0f * dot(aXb, aXb);
		glm::vec3 o = (dot(b, b) * glm::cross(aXb, a)) + (dot(a, a) * glm::cross(b, aXb)) / denominator;
		c = o + *p3;
		r = length(o) + EPSILON;
	}

	BoundingSphere::BoundingSphere(glm::vec3* p1, glm::vec3* p2, glm::vec3* p3, glm::vec3* p4) {
		const glm::vec3 a = *p1 - *p4;
		const glm::vec3 b = *p2 - *p4;
		const glm::vec3 d = *p3 - *p4;
		// determinant of a.x, a.y, a.z
		//                b.x, b.y, b.z
		//                d.x, d.y, d.z
		//float denominator = 2.0f*(a.x*(b.y*c.z - b.z*c.y) - a.y*(b.x*c.z - b.z*c.x) + a.z*(b.x*c.y - b.y*c.x));
		glm::vec3 bXd = glm::cross(b, d);
		float denominator = 2.0f * (dot(a, bXd));
		glm::vec3 o = (dot(d, d) * glm::cross(a, b) + dot(b, b) * glm::cross(d, a) + dot(a, a) * bXd) / denominator;
		c = o + *p4;
		r = length(o) + EPSILON;
	}

	BoundingSphere& BoundingSphere::transform(glm::vec3 const& translate, glm::vec3 const& scale) {
		c += translate;
		r *= scale.x > scale.y ? (scale.x > scale.z ? scale.x : scale.z) : (scale.y > scale.z ? scale.y : scale.z);
		return *this;
	}

	BoundingSphere& BoundingSphere::translate(glm::vec3 const& translate) {
		c += translate;
		return *this;
	}

	BoundingSphere& BoundingSphere::scale(glm::vec3 const& scale) {
		r *= scale.x > scale.y ? (scale.x > scale.z ? scale.x : scale.z) : (scale.y > scale.z ? scale.y : scale.z);
		return *this;
	}

	BoundingSphere BoundingSphere::calcBoundingSphereExact(glm::vec3 verticesArr[], unsigned int verticesNr) {
		glm::vec3** verticesPtrsArr = new glm::vec3 * [verticesNr];
		for (int vertexIdx = 0; vertexIdx < verticesNr; vertexIdx++) {
			verticesPtrsArr[vertexIdx] = &verticesArr[vertexIdx];
		}

		BoundingSphere boundingSphere = recurseCalcBoundingSphere(verticesPtrsArr, verticesNr, 0);

		delete[] verticesPtrsArr;
		return boundingSphere;
	}

	BoundingSphere BoundingSphere::recurseCalcBoundingSphere(glm::vec3** verticesArr, size_t verticesNr, size_t spherePointsNr) {
		BoundingSphere boundingSphere(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f);

		switch (spherePointsNr) {
		case 0:
			break;
		case 1:
			boundingSphere = BoundingSphere(verticesArr[-1]);
			break;
		case 2:
			boundingSphere = BoundingSphere(verticesArr[-1], verticesArr[-2]);
			break;
		case 3:
			boundingSphere = BoundingSphere(verticesArr[-1], verticesArr[-2], verticesArr[-3]);
			break;
		case 4:
			return BoundingSphere(verticesArr[-1], verticesArr[-2], verticesArr[-3], verticesArr[-4]);
		}

		for (int vertexIdx = 0; vertexIdx < verticesNr; vertexIdx++) {
			if (glm::distance2(*(verticesArr[vertexIdx]), boundingSphere.c) > boundingSphere.r * boundingSphere.r) {
				for (int i = vertexIdx; i > 0; i--) {
					glm::vec3* temp = verticesArr[vertexIdx];
					verticesArr[vertexIdx] = verticesArr[vertexIdx - 1];
					verticesArr[vertexIdx - 1] = temp;
				}

				boundingSphere = recurseCalcBoundingSphere(verticesArr + 1, verticesNr - 1, spherePointsNr + 1);
			}
		}

		return boundingSphere;
	}

	inline float distSqrd(glm::vec3& p1, glm::vec3& p2) {
		return ((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y) + (p1.z - p2.z) * (p1.z - p2.z));
	}

	// according to Jack Ritter's algo for efficient bounding sphere
	BoundingSphere BoundingSphere::calcBoundingSphereEfficient(glm::vec3 verticesArr[], unsigned int verticesNr) {
		// first passs
		unsigned int minXVertexIdx = 0; unsigned int maxXvertexIdx = 0;
		unsigned int minYVertexIdx = 0; unsigned int maxYvertexIdx = 0;
		unsigned int minZVertexIdx = 0; unsigned int maxZvertexIdx = 0;
		for (unsigned int vertexIdx = 1; vertexIdx < verticesNr; vertexIdx++) {
			glm::vec3& comparedVertex = verticesArr[vertexIdx];
			if (comparedVertex.x < verticesArr[minXVertexIdx].x)
				minXVertexIdx = vertexIdx;
			else if (verticesArr[maxXvertexIdx].x < comparedVertex.x)
				maxXvertexIdx = vertexIdx;

			if (comparedVertex.y < verticesArr[minXVertexIdx].y)
				minYVertexIdx = vertexIdx;
			else if (verticesArr[maxXvertexIdx].y < comparedVertex.y)
				maxYvertexIdx = vertexIdx;

			if (comparedVertex.z < verticesArr[minXVertexIdx].z)
				minZVertexIdx = vertexIdx;
			else if (verticesArr[maxXvertexIdx].z < comparedVertex.z)
				maxZvertexIdx = vertexIdx;
		}
		glm::vec3 firstPassPotentialPoints[6] = { verticesArr[minXVertexIdx], verticesArr[maxXvertexIdx],
												  verticesArr[minYVertexIdx], verticesArr[maxYvertexIdx],
												  verticesArr[minZVertexIdx], verticesArr[maxZvertexIdx] };
		float maxInterPointsDistSqrd = 0.0f;
		unsigned int firstPassResPointsIdxs[2];
		for (unsigned int i = 0; i < 5; i++) {
			for (unsigned int j = i + 1; j < 6; j++) {
				float comparedDistSqrd = distSqrd(firstPassPotentialPoints[i], firstPassPotentialPoints[j]);
				if (comparedDistSqrd > maxInterPointsDistSqrd) {
					maxInterPointsDistSqrd = comparedDistSqrd;
					firstPassResPointsIdxs[0] = i;
					firstPassResPointsIdxs[1] = j;
				}
			}
		}

		// second pass
		glm::vec3 p0 = firstPassPotentialPoints[firstPassResPointsIdxs[0]];
		glm::vec3 p1 = firstPassPotentialPoints[firstPassResPointsIdxs[1]];
		glm::vec3 resC = (p0 + p1) / 2.0f;
		float resRSqrd = dot(resC - p0, resC - p0);
		float resR = sqrt(resRSqrd);
		for (unsigned int vertexIdx = 0; vertexIdx < verticesNr; vertexIdx++) {
			glm::vec3 comparedP = verticesArr[vertexIdx];
			glm::vec3 comparedVertexToResCVec = resC - comparedP;
			float comparedVertexToResCVecLenSqrd = dot(comparedVertexToResCVec, comparedVertexToResCVec);
			if (resRSqrd < comparedVertexToResCVecLenSqrd) {
				float comparedVertexToResCVecLen = sqrt(comparedVertexToResCVecLenSqrd);
				p0 = comparedP + comparedVertexToResCVec * (resR + comparedVertexToResCVecLen) / comparedVertexToResCVecLen;
				p1 = comparedP;
				resRSqrd = comparedVertexToResCVecLenSqrd;
				resR = sqrt(comparedVertexToResCVecLenSqrd);
				resC = (p0 + p1) / 2.0f;
			}
		}

		return BoundingSphere(resC, resR);
	}

	BoundingSphere BoundingSphere::calcTransformedBoundingSphere(BoundingSphere const& original, glm::vec3 translate, glm::vec3 scale) {
		BoundingSphere newBoundingSphere(original);
		return newBoundingSphere.transform(translate, scale);
	}

	BoundingSphere BoundingSphere::calcCombinedBoundingSphere(BoundingSphere const& sphere1, BoundingSphere const& sphere2) {
		glm::vec3 centersVec = sphere1.c - sphere2.c;
		float centersVecLen = glm::fastLength(centersVec);
		if (centersVecLen > EPSILON)
			return BoundingSphere((sphere1.c + sphere2.c + (centersVec / centersVecLen) * (sphere1.r - sphere2.r)) / 2.0f, (centersVecLen + sphere1.r + sphere2.r) / 2.0f);
		else
			return BoundingSphere(sphere1.c, std::fmax(sphere1.r, sphere2.r));
	}

} // namespace Corium3D
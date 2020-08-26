#include "BoundingSphere.h"

#include <math.h>
#include <list>
#include <vector>

using namespace DirectX;

namespace CoriumDirectX {

	const float EPSILON_ZERO = 1e-4f;

	inline float dist(XMVECTOR const& v1, XMVECTOR const& v2) {
		return XMVectorGetX(XMVector3Length(v1 - v2));
	}

	inline float vecLen(XMVECTOR const& v) {
		return XMVectorGetX(XMVector3Length(v));
	}

	BoundingSphere::BoundingSphere(XMVECTOR const& center, float radius) : c(center), r(radius) {}

	BoundingSphere& BoundingSphere::transform(DirectX::XMVECTOR const& translate, float scaleFactor) {
		c += translate;
		r *= scaleFactor;

		return *this;
	}

	BoundingSphere& BoundingSphere::translate(XMVECTOR const& translate) {
		c += translate;

		return *this;
	}

	BoundingSphere& BoundingSphere::setTranslation(DirectX::XMVECTOR const& translation) {
		c = translation;

		return *this;
	}

	BoundingSphere& BoundingSphere::scale(float scaleFactor) {
		r *= scaleFactor;

		return *this;
	}

	BoundingSphere& BoundingSphere::setRadius(float radius) {
		r = radius;

		return *this;
	}

	float BoundingSphere::calcCombinedSphereRadius(BoundingSphere const& other) const {
		BoundingSphere combinedBS = calcCombinedBoundingSphere(*this, other);
		return combinedBS.r;
	}

	bool BoundingSphere::doesContain(BoundingSphere const& other) const {
		return dist(c, other.c) <= fabs(r - other.r);
	}

	BoundingSphere BoundingSphere::calcCombinedBoundingSphere(BoundingSphere const& sphere1, BoundingSphere const& sphere2) {
		XMVECTOR centersVec = sphere1.c - sphere2.c;
		float centersVecLen = vecLen(centersVec);
		if (centersVecLen > EPSILON_ZERO)
			return BoundingSphere((sphere1.c + sphere2.c + (centersVec / centersVecLen) * (sphere1.r - sphere2.r)) / 2.0f, (centersVecLen + sphere1.r + sphere2.r) / 2.0f);
		else
			return BoundingSphere(sphere1.c, fmax(sphere1.r, sphere2.r));
	}

	BoundingSphere BoundingSphere::calcFattenedBoundingSphere(BoundingSphere const& original, float fatteningFactor) {
		return BoundingSphere(original.c, fatteningFactor * original.r);
	}

	BoundingSphere BoundingSphere::calcTransformedBoundingSphere(BoundingSphere const& original, DirectX::XMVECTOR const& translate, float scaleFactor) {
		BoundingSphere newBoundingSphere(original);
		return newBoundingSphere.transform(translate, scaleFactor);
	}
}
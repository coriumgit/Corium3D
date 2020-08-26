#pragma once

#include <DirectXMath.h>

namespace CoriumDirectX {

	class BoundingSphere {
	public:
		BoundingSphere() {}
		BoundingSphere(DirectX::XMVECTOR const& center, float radius);
		DirectX::XMVECTOR const& getCenter() const { return c; }
		float getRadius() const { return r; }		
		BoundingSphere& transform(DirectX::XMVECTOR const& translate, float scale);
		BoundingSphere& translate(DirectX::XMVECTOR const& translate);
		BoundingSphere& setTranslation(DirectX::XMVECTOR const& translation);
		BoundingSphere& scale(float factor);
		BoundingSphere& setRadius(float radius);		
		float calcCombinedSphereRadius(BoundingSphere const& other) const;
		bool doesContain(BoundingSphere const& other) const;

		static BoundingSphere calcCombinedBoundingSphere(BoundingSphere const& sphere1, BoundingSphere const& sphere2);
		static BoundingSphere calcFattenedBoundingSphere(BoundingSphere const& original, float fatteningFactor);
		static BoundingSphere calcTransformedBoundingSphere(BoundingSphere const& original, DirectX::XMVECTOR const& translate, float scaleFactor);

	private:		
		DirectX::XMVECTOR c; // center
		float r; // radius			
	};

} //namespace Corium3D
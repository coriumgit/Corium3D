#pragma once

#include "TransformsStructs.h"

#include <glm/glm.hpp>

namespace Corium3D {

	class AABB {
	public:
		AABB() {}	
		float getSurface() const { return surface; }
		virtual AABB& combine(AABB const& other) = 0;
		virtual float calcCombinedAabbSurface(AABB const& other) const = 0;
		virtual bool doesContain(AABB const& other) const = 0;
		virtual bool doesIntersect(AABB const& other) const = 0;		
	
	protected:	
		float surface = 0.0f;
	};

	class AABB3D : public AABB {
	public:	
		AABB3D() : AABB() {}
		AABB3D(glm::vec3 const& minVertex, glm::vec3 const& maxVertex);
		glm::vec3 getMinVertex() const { return minVertex; }
		glm::vec3 getMaxVertex() const { return maxVertex; }
		glm::vec3 calcCenter() const { return 0.5f*(minVertex + maxVertex); }
		glm::vec3 calcExtents() const { return 0.5f*(maxVertex - minVertex); }
		glm::vec3 calcSz() const { return (maxVertex - minVertex); }
		float getSurface() const { return surface; }
		AABB3D& transform(glm::vec3 const& translate, glm::vec3 const& scale);
		AABB3D& translate(glm::vec3 const& translate);
		AABB3D& scale(glm::vec3 const& scale);	
		bool isIntersectedBySeg(glm::vec3 const& segStart, glm::vec3 const& segEnd);
	
		static AABB3D calcAABB(glm::vec3 verticesArr[], size_t verticesNr);
		static AABB3D calcTransformedAABB(AABB3D const& original, glm::vec3 const& translate, glm::vec3 const& scale);
		static AABB3D calcTranslatedAABB(AABB3D const& original, glm::vec3 const& translate);
		static AABB3D calcScaledAABB(AABB3D const& original, glm::vec3 const& scale);
		static AABB3D calcCombinedAABB(AABB3D const& original, AABB3D const& other);

		AABB3D& combine(AABB const& other) override;
		float calcCombinedAabbSurface(AABB const& other) const override;
		bool doesContain(AABB const& other) const override;
		bool doesIntersect(AABB const& other) const override;

	protected:
		glm::vec3 minVertex;
		glm::vec3 maxVertex;
	};

	// TODO: Fix combine & inherited getMin/MaxVertex, doesContain, doesIntersect 
	class AABB3DRotatable : public AABB3D {
	public:
		AABB3DRotatable() {}
		AABB3DRotatable(glm::vec3 const&  minVertex, glm::vec3 const& maxVertex);
		AABB3DRotatable& transform(glm::vec3 const& translate, glm::mat3& rotMat);
		AABB3DRotatable& transform(Transform3D const& transformDelta);
		AABB3DRotatable& translate(glm::vec3 const& translate);
		AABB3DRotatable& scale(glm::vec3 const& scale);
		AABB3DRotatable& rotate(glm::quat const& rot);

		static AABB3DRotatable calcAABB(glm::vec3 verticesArr[], size_t verticesNr);
		static AABB3DRotatable calcTransformedAABB(AABB3DRotatable const& original, Transform3D const& transform);
		static AABB3DRotatable calcTranslatedAABB(AABB3DRotatable const& original, glm::vec3 const& translate);
		static AABB3DRotatable calcScaledAABB(AABB3DRotatable const& original, glm::vec3 const& scale);
		static AABB3DRotatable calcRotatedAABB(AABB3DRotatable const& original, glm::quat const& rot);
		static AABB3DRotatable calcCombinedAABB(AABB3DRotatable const& original, AABB3DRotatable const& other);

		AABB3DRotatable& combine(AABB const& other) override;

	private:
		glm::vec3 unrotatedHalfDims;
		glm::quat rotQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	};

	class AABB2D : public AABB {
	public:
		AABB2D() : AABB() {}
		AABB2D(glm::vec2 const& minVertex, glm::vec2 const& maxVertex);
		glm::vec2 getMinVertex() const { return minVertex; }
		glm::vec2 getMaxVertex() const { return maxVertex; }
		glm::vec2 calcCenter() const { return 0.5f * (minVertex + maxVertex); }
		glm::vec2 calcExtents() const { return 0.5f * (maxVertex - minVertex); }
		glm::vec2 calcSz() const { return (maxVertex - minVertex); }
		float getSurface() const { return surface; }
		AABB2D& transform(glm::vec2 const& translate, glm::vec2 const& scale);
		AABB2D& translate(glm::vec2 const& translate);
		AABB2D& scale(glm::vec2 const& scale);

		static AABB2D calcAABB(glm::vec3 verticesArr[], size_t verticesNr);
		static AABB2D calcTransformedAABB(AABB2D const& original, glm::vec2 const& translate, glm::vec2 const& scale);
		static AABB2D calcTranslatedAABB(AABB2D const& original, glm::vec2 const& translate);
		static AABB2D calcScaledAABB(AABB2D const& original, glm::vec2 const& scale);
		static AABB2D calcCombinedAABB(AABB2D const& aabb1, AABB2D const& aabb2);

		AABB2D& combine(AABB const& other) override;
		float calcCombinedAabbSurface(AABB const& other) const override;
		bool doesContain(AABB const& other) const override;
		bool doesIntersect(AABB const& other) const override;
		bool isValid() const { return minVertex.x < maxVertex.x; }

	protected:
		glm::vec2 minVertex = { 0.0f, 0.0f };
		glm::vec2 maxVertex = { 0.0f, 0.0f };
	};

	class AABB2DRotatable : public AABB2D {
	public:
		AABB2DRotatable() {}
		AABB2DRotatable(glm::vec2 const& minVertex, glm::vec2 const& maxVertex);
		AABB2DRotatable& transform(glm::vec2 const& translate, glm::mat2 const& rotMat);
		AABB2DRotatable& transform(Transform2D const& transformDelta);
		AABB2DRotatable& translate(glm::vec2 const& translate);
		AABB2DRotatable& scale(glm::vec2 const& scale);
		AABB2DRotatable& rotate(std::complex<float> const& rot);

		static AABB2DRotatable calcAABB(glm::vec3 verticesArr[], size_t verticesNr);
		static AABB2DRotatable calcTransformedAABB(AABB2DRotatable const& original, Transform2D const& transform);
		static AABB2DRotatable calcTranslatedAABB(AABB2DRotatable const& original, glm::vec2 const& translate);
		static AABB2DRotatable calcScaledAABB(AABB2DRotatable const& original, glm::vec2 const& scale);
		static AABB2DRotatable calcRotatedAABB(AABB2DRotatable const& original, std::complex<float> rot);
		static AABB2DRotatable calcCombinedAABB(AABB2DRotatable const& original, AABB2DRotatable const& other);

		AABB2DRotatable& combine(AABB const& other) override;

	private:
		glm::vec2 unrotatedHalfDims;
		std::complex<float> rotComplex = std::complex<float>(1.0f, 0.0f);
	};

} // namespace Corium3D
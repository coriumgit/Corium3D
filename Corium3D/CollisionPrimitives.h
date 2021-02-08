#pragma once

#include "TransformsStructs.h"
#include "ObjPool.h"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <queue>

namespace Corium3D {

	const unsigned int SIMPLE_VERTEX_DEGREE_MAX = 8;

	class CollisionPrimitivesFactory;
	//class CollisionPolytope;
	class CollisionBox;
	class CollisionSphere;
	class CollisionCapsule;
	//class CollisionPolygon;
	class CollisionRect;
	class CollisionCircle;
	class CollisionStadium;

	template <class V>
	class CollisionPrimitive {
	public:
		friend CollisionPrimitivesFactory;

		struct ContactManifold {
			V normal;
			float penetrationDepth;
			unsigned int pointsNr = 0;
			V points[8];
		};

		// Visitor pattern: accept functions
		virtual bool testCollision(CollisionPrimitive* other) = 0;
		virtual bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) = 0;
		// Visitor pattern: visit functions
		virtual bool testCollision(CollisionBox* box) = 0;
		virtual bool testCollision(CollisionBox* box, ContactManifold& manifoldOut) = 0;
		virtual bool testCollision(CollisionSphere* sphere) = 0;
		virtual bool testCollision(CollisionSphere* sphere, ContactManifold& manifoldOut) = 0;
		virtual bool testCollision(CollisionCapsule* capsule) = 0;
		virtual bool testCollision(CollisionCapsule* capsule, ContactManifold& manifoldOut) = 0;
		virtual bool testCollision(CollisionRect* rect) = 0;
		virtual bool testCollision(CollisionRect* rect, ContactManifold& manifoldOut) = 0;
		virtual bool testCollision(CollisionCircle* circle) = 0;
		virtual bool testCollision(CollisionCircle* circle, ContactManifold& manifoldOut) = 0;
		virtual bool testCollision(CollisionStadium* stadium) = 0;
		virtual bool testCollision(CollisionStadium* stadium, ContactManifold& manifoldOut) = 0;

	protected:
		struct GjkOut {
			V vOut;
			V closestPointThis;
			V closestPointOther;
			V simplex[4];
			unsigned int simplexVerticesNr;
		};

		//TODO: Implement this with an ObjPool
		//std::priority_queue< EpaTriangle, std::vector<EpaTriangle>, std::greater<EpaTriangle> > epaTrianglesQueue;
		class GjkJohnsonsDistanceIterator {
		public:
			void init(V const& aFirst, V const& bFirst);
			virtual V iterate(V const& aAdded, V const& bAdded) = 0;
			float getWNormSqrdMax() const { return WNormSqrdMax; };
			bool doesContain(V const& vec);
			unsigned int wsNr() const { return W_sz; }
			V const* getW() { return W; }
			V getCollisionPointA();
			V getCollisionPointB();

		protected:
			V W[4];
			unsigned int W_sz = 1;
			V A[4];
			V B[4];
			float DiX[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
			unsigned int Iw[4] = { 0, 0, 0, 0 };
			unsigned char b = 0x1;
			unsigned int Y_sz = 0;
			unsigned int Iy[2] = { 0, 0 };

			// vec3 d[4][4]; // d[i][j] = W[i] - W[j]
			// float DiX[16][4]; // DiX[j][i] = delta-i over subset X such that X is identified by j that equals
								 // the bit-array that stands for the subset X
			float WNormSqrdMax;
			float WNormsSqrd[4];
		};

		//const unsigned int GjkJohnsonsDistanceIterator::X_SZ_2_Jx[3][2] = { {1,2}, {0,2}, {0,1} };
		//const unsigned int GjkJohnsonsDistanceIterator::nChoose2[2] = { 1, 3 };
		//const unsigned int GjkJohnsonsDistanceIterator::X_SZ_3_Ix[3][2] = { {0,1}, {0,2}, {1,2} };
		//const unsigned int GjkJohnsonsDistanceIterator::X_SZ_3_Jx[3] = { 2, 1, 0 };

		CollisionPrimitive* lastCollisionOtherVolume = NULL;
		V lastCollisionV;

		virtual V supportMap(V const& vec) const = 0;
		virtual V getArbitraryV() const = 0;
		virtual CollisionPrimitive* clone(CollisionPrimitivesFactory& collisionPrimitivesFactory, Transform3D const& newCloneParentTransform) const = 0;
		virtual void destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) = 0;
		float gjkShallowPenetrationTest(CollisionPrimitive& other, float objsMarginsSum, GjkJohnsonsDistanceIterator& johnsonDistIt, GjkOut* gjkOut);
		bool gjkIntersectionTest(CollisionPrimitive& other, GjkJohnsonsDistanceIterator& johnsonDistIt);
		float gjkEpaHybridTest(CollisionPrimitive& other);
	};

	// Reminder: all input vectors are in parent's space
	class CollisionPrimitivesFactory {
	public:
		CollisionPrimitivesFactory(unsigned int* primitive3DInstancesNrsMaxima, unsigned int* primitive2DInstancesNrsMaxima);
		~CollisionPrimitivesFactory();
		template <class V>
		CollisionPrimitive<V>* genCollisionPrimitive(CollisionPrimitive<V> const& prototypeCollisionPrimitive, Transform3D const& newCloneParentTransform);
		//CollisionVolume* genCollisionVolume(CollisionVolume const& prototypeCollisionVolume);
		//CollisionPerimeter* genCollisionPerimeter(CollisionPerimeter const& prototypeCollisionPerimeter);
		CollisionBox* genCollisionBox(glm::vec3 const& center, glm::vec3 scale);
		CollisionSphere* genCollisionSphere(glm::vec3 const& center, float radius);
		CollisionCapsule* genCollisionCapsule(glm::vec3 const& center1, glm::vec3 const& axisVec, float radius);
		CollisionRect* genCollisionRect(glm::vec2 const& center, glm::vec2 scale);
		CollisionCircle* genCollisionCircle(glm::vec2 const& center, float radius);
		CollisionStadium* genCollisionStadium(glm::vec2 const& center1, glm::vec2 const& axisVec, float radius);

		template <class V>
		void destroyCollisionPrimitive(CollisionPrimitive<V>* collisionPrimitive);
		//void genCollisionVolume(CollisionVolume* collisionVolume);
		//void genCollisionPerimeter(CollisionPerimeter* collisionPerimeter);
		void destroyCollisionBox(CollisionBox* collisionBox);
		void destroyCollisionSphere(CollisionSphere* collisionSphere);
		void destroyCollisionCapsule(CollisionCapsule* collisionCapsule);
		void destroyCollisionRect(CollisionRect* collisionRect);
		void destroyCollisionCircle(CollisionCircle* collisionCircle);
		void destroyCollisionStadium(CollisionStadium* collisionStadium);

	private:
		Corium3DUtils::ObjPool<CollisionBox>* collisionBoxesPool;
		Corium3DUtils::ObjPool<CollisionSphere>* collisionSpheresPool;
		Corium3DUtils::ObjPool<CollisionCapsule>* collisionCapsulesPool;
		Corium3DUtils::ObjPool<CollisionRect>* collisionRectsPool;
		Corium3DUtils::ObjPool<CollisionCircle>* collisionCirclesPool;
		Corium3DUtils::ObjPool<CollisionStadium>* collisionStadiumsPool;
	};

	class CollisionVolume : public CollisionPrimitive<glm::vec3> {
	public:
		friend class CollisionPrimitivesFactory;

		CollisionVolume() {}
		virtual ~CollisionVolume() {}
		virtual void translate(glm::vec3 const& translation) = 0;		
		virtual void scale(float scaleFactor) = 0;
		virtual void rotate(glm::quat const& rot) = 0;
		virtual void transform(Transform3DUS const& transform) = 0;
		virtual bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) = 0;
		virtual bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) = 0;

	protected:
		static constexpr unsigned int X_SZ_2_Jx[3][2] = { {1,2}, {0,2}, {0,1} };
		static constexpr unsigned int nChoose2[2] = { 1, 3 };
		static constexpr unsigned int X_SZ_3_Ix[3][2] = { {0,1}, {0,2}, {1,2} };
		static constexpr unsigned int X_SZ_3_Jx[3] = { 2, 1, 0 };

		class GjkJohnsonsDistanceIterator3D : public CollisionPrimitive<glm::vec3>::GjkJohnsonsDistanceIterator {
			glm::vec3 iterate(glm::vec3 const& aAdded, glm::vec3 const& bAdded) override;
		};
	};

	// TODO: Implement this
	class CollisionPolytope : public CollisionVolume {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionPolytope>;

		void translate(glm::vec3 const& translation) override;		
		void scale(float scale) override;
		void rotate(glm::quat const& rot) override;
		void transform(Transform3DUS const& transform) override;
		glm::vec3 supportMap(glm::vec3 const& vec) const override;
		glm::vec3 getArbitraryV() const override;
		virtual bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) override;
		virtual bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) override;

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other->testCollision(this, manifoldOut); }

	private:
		glm::vec3* vertices;
		unsigned int verticesNr;
		// adjGraph = adjacency graph 
		// adjGraph[i] -> array of adjacent vertices indices to vertex i
		unsigned int** adjGraph;
		// adjVerticesNrs[i] -> number of adjacent vertices to vertex i
		unsigned int adjVerticesNrs;
		// slimmedAdjGraph = slimmed adjacency graph
		unsigned int** slimmedAdjGraph;
		unsigned int AdjVerticesSlimmedNrs;
		// polytopeLayers[i] -> array of vertices indices comprising layer i
		unsigned int** polytopeLayers;
		unsigned int polytopeLayersNr;
		unsigned int* polytopeLayersVerticesNrs;
		// complexVertices -> vertices with a degree higher than SIMPLE_VERTEX_DEGREE_MAX
		glm::vec3* complexVertices;
		unsigned int complexVerticesNr;
		glm::mat4 transformat;

		CollisionPolytope(glm::vec3 const* vertices, unsigned int verticesNr);
		~CollisionPolytope() override;
	};

	class CollisionBox : public CollisionVolume {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionBox>;

		void translate(glm::vec3 const& translation) override { c += translation; }		
		void scale(float scaleFactor) override  
		{ 
			s *= scaleFactor;  
			c -= offset; 
			offset *= scaleFactor; 
			c += offset;
		}

		void rotate(glm::quat const& rot) override 
		{ 
			glm::mat3 rotationMat = mat3_cast(rot); 
			r = rotationMat * r; 
			c -= offset; 
			offset = rotationMat * offset; 
			c += offset; 
		}

		void transform(Transform3DUS const& transform) override { scale(transform.scale); rotate(transform.rot); translate(transform.translate); }
		glm::vec3 supportMap(glm::vec3 const& vec) const override;
		glm::vec3 getArbitraryV() const override;
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) override;
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) override;

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other->testCollision(this, manifoldOut); }

		glm::vec3 const& getC() const { return c; }
		glm::vec3 const& getS() const { return s; }
		glm::mat3 const& getR() const { return r; }

	private:
		struct FacePenetrationDesc {
			unsigned int penetrationMinAxIdxThis;
			unsigned int penetrationMinAxIdxOther;
			float penetrationThis = -std::numeric_limits<float>::max();
			float penetrationOther = -std::numeric_limits<float>::max();
		};

		struct EdgePenetrationDesc {
			unsigned int penetrationMinAxEdgeIdxThis;
			unsigned int penetrationMinAxEdgeIdxOther;
		};

		glm::vec3 c; // center
		glm::vec3 s; // scale
		glm::mat3 r; // rotation matrix	
		glm::vec3 offset; // offset translation from object center
		bool wasLastFrameSeparated = true;
		unsigned int lastCollisionResLmntIdx = 6; // lmnts [0,2] -> edges; lmnts [3,5] -> faces; 6 -> no collision
		static unsigned int axIdxs1[3];
		static unsigned int axIdxs2[3];

		CollisionBox() {}
		CollisionBox(glm::vec3 const& center, glm::vec3 const& scale);
		~CollisionBox() {}
		CollisionVolume* clone(CollisionPrimitivesFactory& collisionPrimitivesFactory, Transform3D const& newCloneParentTransform) const override;
		void destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) override;

		inline static void movePlaneAxIdxInBuffer(unsigned int idx);
		inline static void moveEdgesAxsIdxsInBuffers(unsigned int idx1, unsigned int idx2);
		inline static void revertPlaneIdxToBufferStart(unsigned int idx);
		inline static void revertEdgesIdxsToBuffersStart(unsigned int idx1, unsigned int idx2);
		inline glm::vec3 supportMapMinusDirection(glm::vec3 const& vec, unsigned int directionIdx);
		inline void clipCapsuleVec(CollisionCapsule const* capsule, unsigned int facenAxIdx, glm::vec3 const& faceNormal, glm::vec3* clipPointsOut);		
		//inline bool getSegmentIntersectionPointWithFace(glm::vec3 const& segmentC, glm::vec3 const& segmentV, glm::vec3 const& faceNormal, glm::vec3 const& faceCenter, glm::vec3 const& faceVec1, float faceHalfSide1Len, glm::vec3 const& faceVec2, float faceHalfSide2Len, glm::vec3& intersectionPointOut);	

		// Visitor pattern: visitors	
		bool testCollision(CollisionBox* other) override;
		bool testCollision(CollisionBox* other, ContactManifold& manifoldOut) override;
		bool testCollision(CollisionSphere* sphere) override;
		bool testCollision(CollisionSphere* sphere, ContactManifold& manifoldOut) override;
		bool testCollision(CollisionCapsule* capsule) override;
		bool testCollision(CollisionCapsule* capsule, ContactManifold& manifoldOut) override;
		bool testCollision(CollisionRect* rect) override { return false; }
		bool testCollision(CollisionRect* rect, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionCircle* circle) override { return false; }
		bool testCollision(CollisionCircle* circle, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionStadium* stadium) override { return false; }
		bool testCollision(CollisionStadium* stadium, ContactManifold& manifoldOut) override { return false; }

		bool testFacesSeparation(CollisionBox& other, FacePenetrationDesc& penetrationDescOut);
		bool testEdgesSeparation(CollisionBox& other, EdgePenetrationDesc& penetrationDescOut);
		void createFaceContact(CollisionBox& other, unsigned int penetrationMinAxIdx, float penetration, ContactManifold& manifoldOut);
	};

	class CollisionSphere : public CollisionVolume {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionSphere>;

		void translate(glm::vec3 const& translation) override { c += translation; }		
		void scale(float scaleFactor) override 
		{
			r *= scaleFactor; 
			c -= offset; 
			offset *= scaleFactor; 
			c += offset; 
		}

		void rotate(glm::quat const& rot) override 
		{ 
			c -= offset; 
			offset = rot * offset; 
			c += offset; 
		}

		void transform(Transform3DUS const& transform) override { scale(transform.scale); translate(transform.translate); }
		glm::vec3 supportMap(glm::vec3 const& vec) const override { return c; }
		glm::vec3 getArbitraryV() const override { return c; }
		glm::vec3 const& getC() const { return c; }
		float getR() const { return r; }
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) override;
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) override;

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other->testCollision(this, manifoldOut); }

	private:
		glm::vec3 c; // center
		float r; // radius	
		glm::vec3 offset; // offset translation from object center

		CollisionSphere() {}
		CollisionSphere(glm::vec3 const& center, float radius);
		~CollisionSphere() {}
		CollisionVolume* clone(CollisionPrimitivesFactory& collisionPrimitivesFactory, Transform3D const& newCloneParentTransform) const override;
		void destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) override;		

		// Visitor pattern: visitors
		bool testCollision(CollisionBox* box) override { return false; }
		bool testCollision(CollisionBox* box, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionSphere* other) override { return glm::length2(other->c - c) <= (r + other->r) * (r + other->r); }
		bool testCollision(CollisionSphere* other, ContactManifold& manifoldOut) override;
		bool testCollision(CollisionCapsule* capsule) override;
		bool testCollision(CollisionCapsule* capsule, ContactManifold& manifoldOut) override;
		bool testCollision(CollisionRect* rect) override { return false; }
		bool testCollision(CollisionRect* rect, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionCircle* circle) override { return false; }
		bool testCollision(CollisionCircle* circle, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionStadium* stadium) override { return false; }
		bool testCollision(CollisionStadium* stadium, ContactManifold& manifoldOut) override { return false; }
	};

	class CollisionCapsule : public CollisionVolume {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionCapsule>;

		void translate(glm::vec3 const& translation) override { c1 += translation; }		
		void scale(float scaleFactor) override 
		{ 
			v *= scaleFactor; 
			c1 -= offset; 
			offset *= scaleFactor; 
			c1 += 0.5f * (1 - scaleFactor) * v + offset; 
			r *= scaleFactor; 
		}

		void rotate(glm::quat const& rot) override 
		{ 
			glm::vec3 rotatedV = rot * v; 
			c1 -= offset; 
			offset = rot * offset; 
			c1 += 0.5f * (v - rotatedV) + offset; 
			v = rotatedV; 
		}

		void transform(Transform3DUS const& transform) override { scale(transform.scale); rotate(transform.rot); translate(transform.translate); }
		glm::vec3 supportMap(glm::vec3 const& vec) const override { return glm::dot(vec, v) > 0.0f ? c1 + v : c1; }
		glm::vec3 getArbitraryV() const override { return c1; }
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) override;
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) override;

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other->testCollision(this, manifoldOut); }

		glm::vec3 const& getC1() const { return c1; }
		glm::vec3 const& getV() const { return v; }
		float getR() const { return r; }

	private:
		glm::vec3 c1; // center 1
		glm::vec3 v; // axis
		float r; // radius	
		glm::vec3 offset; // offset translation from object center

		CollisionCapsule() {}
		CollisionCapsule(glm::vec3 const& center1, glm::vec3 const& axisVec, float radius);
		~CollisionCapsule() {}
		CollisionVolume* clone(CollisionPrimitivesFactory& collisionPrimitivesFactory, Transform3D const& newCloneParentTransform) const override;
		void destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) override;

		// Visitor pattern: visitors
		bool testCollision(CollisionBox* box) override { return false; }
		bool testCollision(CollisionBox* box, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionSphere* sphere) override { return false; }
		bool testCollision(CollisionSphere* sphere, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionCapsule* other) override;
		bool testCollision(CollisionCapsule* other, ContactManifold& manifoldOut) override;
		bool testCollision(CollisionRect* rect) override { return false; }
		bool testCollision(CollisionRect* rect, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionCircle* circle) override { return false; }
		bool testCollision(CollisionCircle* circle, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionStadium* stadium) override { return false; }
		bool testCollision(CollisionStadium* stadium, ContactManifold& manifoldOut) override { return false; }
	};

	class CollisionCone : public CollisionVolume {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionCone>;

		void translate(glm::vec3 const& translation) override;		
		void scale(float scaleFactor) override;
		void rotate(glm::quat const& rot) override;
		void transform(Transform3DUS const& transform) override;
		glm::vec3 supportMap(glm::vec3 const& vec) const override;
		glm::vec3 getArbitraryV() const override;
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) override;
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) override;

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other->testCollision(this, manifoldOut); }

	private:
		glm::vec3 c; // cone center
		float r; // base radius
		float e; // half height
		glm::vec3 u; //axis unit vector
		float sint; //sin(top angle)	

		CollisionCone() {}
		CollisionCone(glm::vec3 const& center, float baseRadius, float halfHeight, glm::vec3 const& axisVec);
		~CollisionCone() {}
		CollisionVolume* clone(CollisionPrimitivesFactory& collisionPrimitivesFactory, Transform3D const& newCloneParentTransform) const override;
		void destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) override;
	};

	class CollisionCylinder : public CollisionVolume {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionCylinder>;

		void translate(glm::vec3 const& translation) override;		
		void scale(float scaleFactor) override;
		void rotate(glm::quat const& rot) override;
		void transform(Transform3DUS const& transform) override;
		glm::vec3 supportMap(glm::vec3 const& vec) const override;
		glm::vec3 getArbitraryV() const override;
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) override;
		bool testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) override;

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other - testCollision(this, manifoldOut); }

	private:
		glm::vec3 c; // cone center
		float r; // base radius
		float e; // half height
		glm::vec3 u; //axis unit vector	

		CollisionCylinder() {}
		CollisionCylinder(glm::vec3 const& center, float radius, float halfHeight);
		~CollisionCylinder() {}
		CollisionVolume* clone(CollisionPrimitivesFactory& collisionPrimitivesFactory, Transform3D const& newCloneParentTransform) const override;
		void destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) override;
	};

	class CollisionPerimeter : public CollisionPrimitive<glm::vec2> {
	public:
		friend class CollisionPrimitivesFactory;

		CollisionPerimeter() {}
		virtual ~CollisionPerimeter() {}
		virtual void translate(glm::vec2 const& translation) = 0;		
		virtual void scale(float scaleFactor) = 0;
		virtual void rotate(std::complex<float> const& rot) = 0;
		virtual void transform(Transform2DUS const& transform) = 0;
		virtual bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest, float& segFactorOnCollisionOut) = 0;
		virtual bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest) = 0;

	protected:
		static constexpr unsigned int X_SZ_2_Jx[3][2] = { {1,2}, {0,2}, {0,1} };
		static constexpr unsigned int nChoose2[2] = { 1, 3 };
		static constexpr unsigned int X_SZ_3_Ix[3][2] = { {0,1}, {0,2}, {1,2} };
		static constexpr unsigned int X_SZ_3_Jx[3] = { 2, 1, 0 };

		class GjkJohnsonsDistanceIterator2D : public CollisionPrimitive<glm::vec2>::GjkJohnsonsDistanceIterator {
			glm::vec2 iterate(glm::vec2 const& aAdded, glm::vec2 const& bAdded) override;
		};
	};

	// TODO: Implement this
	class CollisionPolygon : public CollisionPerimeter {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionPolytope>;

		void translate(glm::vec2 const& translation) override;		
		void scale(float scale) override;
		void rotate(std::complex<float> const& rot) override;
		void transform(Transform2DUS const& transform) override;
		glm::vec2 supportMap(glm::vec2 const& vec) const override;
		glm::vec2 getArbitraryV() const override;
		virtual bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest, float& segFactorOnCollisionOut) override;
		virtual bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest) override;

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other->testCollision(this, manifoldOut); }

	private:
		glm::vec2* vertices;
		unsigned int verticesNr;
		// adjGraph = adjacency graph 
		// adjGraph[i] -> array of adjacent vertices indices to vertex i
		unsigned int** adjGraph;
		// adjVerticesNrs[i] -> number of adjacent vertices to vertex i
		unsigned int adjVerticesNrs;
		// slimmedAdjGraph = slimmed adjacency graph
		unsigned int** slimmedAdjGraph;
		unsigned int AdjVerticesSlimmedNrs;
		// polytopeLayers[i] -> array of vertices indices comprising layer i
		unsigned int** polytopeLayers;
		unsigned int polytopeLayersNr;
		unsigned int* polytopeLayersVerticesNrs;
		// complexVertices -> vertices with a degree higher than SIMPLE_VERTEX_DEGREE_MAX
		glm::vec2* complexVertices;
		unsigned int complexVerticesNr;
		glm::mat3 transformat;

		CollisionPolygon(glm::vec2 const* vertices, unsigned int verticesNr);
		~CollisionPolygon() override;
	};

	class CollisionRect : public CollisionPerimeter {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionRect>;

		void translate(glm::vec2 const& translation) override { c += translation; }		
		void scale(float scaleFactor) override 
		{ 
			s *= scaleFactor;
			c -= offset; 
			offset *= scaleFactor; 
			c += offset; 
		}

		void rotate(std::complex<float> const& rot) override 
		{ 
			glm::mat2 rotMat(rot.real(), rot.imag(), -rot.imag(), rot.real());
			r = rotMat * r; 
			c -= offset; 
			offset = rotMat * offset; 
			c += offset; 
		}

		void transform(Transform2DUS const& transform) override { scale(transform.scale); rotate(transform.rot); translate(transform.translate); }
		glm::vec2 supportMap(glm::vec2 const& vec) const override;
		glm::vec2 getArbitraryV() const override;
		bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest, float& segFactorOnCollisionOut) override;
		bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest) override;

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other->testCollision(this, manifoldOut); }

		glm::vec2 const& getC() const { return c; }
		glm::vec2 const& getS() const { return s; }
		glm::mat2 const& getR() const { return r; }

	private:
		struct SidePenetrationDesc {
			unsigned int penetrationMinAxIdxThis;
			unsigned int penetrationMinAxIdxOther;
			float penetrationThis = -std::numeric_limits<float>::max();
			float penetrationOther = -std::numeric_limits<float>::max();
		};

		glm::vec2 c; // center
		glm::vec2 s; // scale
		glm::mat2 r; // rotation matrix	
		glm::vec2 offset; // offset translation from object center
		bool wasLastFrameSeparated = true;
		bool didOwnLastSeparationAx = false;

		CollisionRect() {}
		CollisionRect(glm::vec2 const& center, glm::vec2 extent);
		~CollisionRect() {}
		CollisionPerimeter* clone(CollisionPrimitivesFactory& collisionPrimitivesFactory, Transform3D const& newCloneParentTransform) const override;
		void destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) override;

		inline void clipStadiumVec(CollisionStadium const* stadium, unsigned int sideAxIdx, glm::vec2 const& faceNormal, glm::vec2* clipPointsOut);

		// Visitor pattern: visitors	
		bool testCollision(CollisionBox* collisionBox) override { return false; }
		bool testCollision(CollisionBox* collisionBox, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionSphere* sphere) override { return false; }
		bool testCollision(CollisionSphere* sphere, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionCapsule* capsule) override { return false; }
		bool testCollision(CollisionCapsule* capsule, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionRect* other) override;
		bool testCollision(CollisionRect* other, ContactManifold& manifoldOut) override;
		bool testCollision(CollisionCircle* circle) override;
		bool testCollision(CollisionCircle* circle, ContactManifold& manifoldOut) override;
		bool testCollision(CollisionStadium* stadium) override;
		bool testCollision(CollisionStadium* stadium, ContactManifold& manifoldOut) override;

		bool testSidesSeparation(CollisionRect& other, SidePenetrationDesc& penetrationDescOut);
		void createSidesContact(CollisionRect& other, unsigned int penetrationMinAxIdx, float penetration, ContactManifold& manifoldOut);
	};

	class CollisionCircle : public CollisionPerimeter {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionCircle>;

		void translate(glm::vec2 const& translation) override { c += translation; }		
		void scale(float scaleFactor) override
		{ 
			r *= scaleFactor; 
			c -= offset; 
			offset *= scaleFactor; 
			c += offset; 
		}

		void rotate(std::complex<float> const& rot) override 
		{
			glm::mat2 rotationMat(rot.real(), rot.imag(), -rot.imag(), rot.real());
			c -= offset;
			offset = rotationMat * offset;
			c += offset;
		}

		void transform(Transform2DUS const& transform) override { scale(transform.scale); translate(transform.translate); }
		glm::vec2 supportMap(glm::vec2 const& vec) const override { return c; }
		glm::vec2 getArbitraryV() const override { return c; }
		bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest, float& segFactorOnCollisionOut) override;
		bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest) override;

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other->testCollision(this, manifoldOut); }

		glm::vec2 const& getC() const { return c; }
		float getR() const { return r; }

	private:
		glm::vec2 c; // center
		float r; // radius	
		glm::vec2 offset; // offset translation from object center

		CollisionCircle() {}
		CollisionCircle(glm::vec2 const& center, float radius);
		~CollisionCircle() {}
		CollisionPerimeter* clone(CollisionPrimitivesFactory& collisionPrimitivesFactory, Transform3D const& newCloneParentTransform) const override;
		void destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) override;

		// Visitor pattern: visitors
		bool testCollision(CollisionBox* box) override { return false; }
		bool testCollision(CollisionBox* box, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionSphere* collisionSphere) override { return false; }
		bool testCollision(CollisionSphere* collisionSphere, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionCapsule* capsule) override { return false; }
		bool testCollision(CollisionCapsule* capsule, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionRect* rect) override { return false; }
		bool testCollision(CollisionRect* rect, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionCircle* other) override { return glm::length2(other->c - c) <= (r + other->r) * (r + other->r); }
		bool testCollision(CollisionCircle* other, ContactManifold& manifoldOut) override;
		bool testCollision(CollisionStadium* stadium) override;
		bool testCollision(CollisionStadium* stadium, ContactManifold& manifoldOut) override;
	};

	class CollisionStadium : public CollisionPerimeter {
	public:
		friend class CollisionPrimitivesFactory;
		friend class Corium3DUtils::ObjPool<CollisionStadium>;

		void translate(glm::vec2 const& translation) override { c1 += translation; }		
		void scale(float scaleFactor) override
		{ 
			v *= scaleFactor; 
			c1 -= offset; 
			offset *= scaleFactor; 
			c1 += 0.5f * (1 - scaleFactor) * v + offset; 
			r *= scaleFactor;			 
		}

		void rotate(std::complex<float> const& rot) override 
		{
			glm::mat2 rotationMat(rot.real(), rot.imag(), -rot.imag(), rot.real());			
			glm::vec2 rotatedV = rotationMat * v;
			c1 -= offset;
			offset = rotationMat * offset;
			c1 += 0.5f * (v - rotatedV) + offset;
			v = rotatedV;
		}

		void transform(Transform2DUS const& transform) override {
			scale(transform.scale);
			rotate(transform.rot);
			translate(transform.translate);
		}

		glm::vec2 supportMap(glm::vec2 const& vec) const override { return glm::dot(vec, v) > 0.0f ? c1 + v : c1; }
		glm::vec2 getArbitraryV() const override { return c1; }
		bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest, float& segFactorOnCollisionOut) override;
		bool testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest) override;

		glm::vec2 const& getC1() const { return c1; }
		glm::vec2 const& getV() const { return v; }
		float getR() const { return r; }

		// Visitor pattern: acceptors
		bool testCollision(CollisionPrimitive* other) override { return other->testCollision(this); }
		bool testCollision(CollisionPrimitive* other, ContactManifold& manifoldOut) override { return other->testCollision(this, manifoldOut); }

	private:
		glm::vec2 c1; // center 1
		glm::vec2 v; // axis
		float r; // radius	
		glm::vec2 offset; // offset translation from object center

		CollisionStadium() {}
		CollisionStadium(glm::vec2 const& center1, glm::vec2 const& axisVec, float radius);
		~CollisionStadium() {}
		CollisionPerimeter* clone(CollisionPrimitivesFactory& collisionPrimitivesFactory, Transform3D const& newCloneParentTransform) const override;
		void destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) override;

		// Visitor pattern: visitors
		bool testCollision(CollisionBox* box) override { return false; }
		bool testCollision(CollisionBox* box, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionSphere* collisionSphere) override { return false; }
		bool testCollision(CollisionSphere* collisionSphere, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionCapsule* capsule) override { return false; }
		bool testCollision(CollisionCapsule* capsule, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionRect* rect) override { return false; }
		bool testCollision(CollisionRect* rect, ContactManifold& manifoldOut) override { return false; }
		bool testCollision(CollisionCircle* other) override { return false; }
		bool testCollision(CollisionCircle* other, ContactManifold& manifoldOut) { return false; }
		bool testCollision(CollisionStadium* stadium) override;
		bool testCollision(CollisionStadium* stadium, ContactManifold& manifoldOut) override;
	};

	template <class V>
	CollisionPrimitive<V>* CollisionPrimitivesFactory::genCollisionPrimitive(CollisionPrimitive<V> const& prototypeCollisionPrimitive, Transform3D const& newCloneParentTransform) {
		return prototypeCollisionPrimitive.clone(*this, newCloneParentTransform);
	}

	template <class V>
	void CollisionPrimitivesFactory::destroyCollisionPrimitive(CollisionPrimitive<V>* collisionPrimitive) {
		collisionPrimitive->destroy(*this);
	}

} // namespace Corium3D
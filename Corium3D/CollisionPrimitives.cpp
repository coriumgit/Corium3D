#include "ServiceLocator.h"
#include "CollisionPrimitives.h"
#include "AssetsOps.h"
#include <string>
#include <sstream>

namespace Corium3D {

	using namespace glm;
	using namespace std;	
	using std::to_string;
	using namespace Corium3DUtils;

	const float EPSILON_RELATIVE_SQRD = 1E-6f;
	const float EPSILON_TOLERANCE_SQRD = 100 * 2E-24f;
	const float EPSILON_ZERO = 1E-5f;
	const float FACE_TO_EDGE_COMPARISON_ZERO = 5E-5f;
	const float EPSILON_ZERO_SQRD = EPSILON_ZERO * EPSILON_ZERO;

	inline string to_string(vec3 v) {
		return string("(") + to_string(v.x) + string(", ") + to_string(v.y) + string(", ") + to_string(v.z) + string(")");
	}

	inline string to_string(vec2 v) {
		return string("(") + to_string(v.x) + string(", ") + to_string(v.y) + string(")");
	}

	struct EpaTriangle {
		glm::vec3 y[3]; // triangle vertices
		glm::vec3 v; // support point
		float vLen;
		float l[3]; // lambdas
		EpaTriangle* adj[3]; // adjacent triangles
		unsigned int j[3]; // adj[i]->adj[this->j[i]] == this
		bool isObsolete = false;
		EpaTriangle* silhouetteNext = NULL;

		bool operator<(EpaTriangle const& other) const { return vLen < other.vLen; }
		bool operator<=(EpaTriangle const& other) const { return vLen <= other.vLen; }
		bool operator>(EpaTriangle const& other) const { return vLen > other.vLen; }
		bool operator>=(EpaTriangle const& other) const { return vLen >= other.vLen; }
	};

	template <class V>
	void CollisionPrimitive<V>::GjkJohnsonsDistanceIterator::init(V const& aFirst, V const& bFirst) {
		W[0] = aFirst - bFirst; A[0] = aFirst; B[0] = bFirst;
		WNormSqrdMax = WNormsSqrd[0] = length2(W[0]);
	}

	template <class V>
	bool CollisionPrimitive<V>::GjkJohnsonsDistanceIterator::doesContain(V const& vec) {
		for (unsigned int wIdx = 0; wIdx < W_sz; wIdx++) {
			if (length2(vec - W[Iw[wIdx]]) < EPSILON_ZERO_SQRD) {
				ServiceLocator::getLogger().logd("GJK", (string("Iteration: v = ") + to_string(vec) + string(" was contained. W: ")).c_str());
				for (unsigned int wIdx = 0; wIdx < W_sz; wIdx++) {			
					ServiceLocator::getLogger().logd("GJK", (string("W[") + to_string(Iw[wIdx]) + string("] =") + to_string(W[Iw[wIdx]])).c_str());
				}
				return true;
			}
		}
	
		string IyString;
		for (unsigned int yIdx = 0; yIdx < Y_sz; yIdx++) {
			IyString += to_string(Iy[yIdx]) + string(" ");
		}
		ServiceLocator::getLogger().logd("GJK", (string("Iteration: Iy = ") + IyString).c_str());
		for (unsigned int yIdx = 0; yIdx < Y_sz; yIdx++) {		
			if (length2(vec - W[Iy[yIdx]]) < EPSILON_ZERO_SQRD) {
				ServiceLocator::getLogger().logd("GJK", (string("Iteration: v = ") + to_string(vec) + string(" was contained. Y: ")).c_str());
				for (unsigned int wIdx = 0; wIdx < Y_sz; wIdx++) {
					ServiceLocator::getLogger().logd("GJK", (string("Y[") + to_string(yIdx) + string("] =") + to_string(W[Iy[yIdx]].x)).c_str());
				}
				return true;
			}
		}			

		return false;
	}

	template <class V>
	V CollisionPrimitive<V>::GjkJohnsonsDistanceIterator::getCollisionPointA() {
		V a(0.0f);
		float DX = 0.0f;
		for (unsigned int IwIdx = 0; IwIdx < W_sz; IwIdx++) {
			unsigned int iw = Iw[IwIdx];
			a += DiX[iw] * A[iw];
			DX += DiX[iw];
		}
		return a / DX;
	}

	template <class V>
	V CollisionPrimitive<V>::GjkJohnsonsDistanceIterator::getCollisionPointB() {
		V b(0.0f);
		float DX = 0.0f;
		for (unsigned int IwIdx = 0; IwIdx < W_sz; IwIdx++) {
			unsigned int iw = Iw[IwIdx];
			b += DiX[iw] * B[iw];
			DX += DiX[iw];
		}
		return b / DX;
	}

	// returns:
	// 0: there is no contact
	// penetration depth: if there is a shallow penetration
	// -1: if there is a deep penetration
	template <class V>
	float CollisionPrimitive<V>::gjkShallowPenetrationTest(CollisionPrimitive& other, float objsMarginsSum, GjkJohnsonsDistanceIterator& johnsoDistIt, GjkOut* gjkOut) {
		V v;
		if (lastCollisionOtherVolume == &other)
			v = lastCollisionV;
		else {
			v = getArbitraryV() - other.getArbitraryV();
			lastCollisionOtherVolume = &other;
			other.lastCollisionOtherVolume = this;
		}

		V a = supportMap(-v);
		V b = other.supportMap(v);
		johnsoDistIt.init(a, b);
		V w = a - b;
		v = w;
		float vNorm2 = length2(v);
		float vNorm2Bound = numeric_limits<float>::max(); // v^2 upper bound
		const float marginsSumSqrd = objsMarginsSum * objsMarginsSum;	
		std::ostringstream out1, out2;
		out1.precision(20);
		out2.precision(20);
		do {
			ServiceLocator::getLogger().logd("GJK", "-------------------------------------------------------------------------");
			ServiceLocator::getLogger().logd("GJK", (string("Iteration: v = ") + to_string(v)).c_str());
			a = supportMap(-v);
			b = other.supportMap(v);		
			ServiceLocator::getLogger().logd("GJK", (string("Iteration: a = ") + to_string(a)).c_str());
			ServiceLocator::getLogger().logd("GJK", (string("Iteration: b = ") + to_string(b)).c_str());
			w = a - b;
			float vwDot = dot(v, w);
			if (vwDot > 0 && vwDot * vwDot / vNorm2 > marginsSumSqrd) {
				lastCollisionV = other.lastCollisionV = v;
				return 0.0f;
			}

			if (johnsoDistIt.doesContain(w) || vNorm2Bound - vwDot <= EPSILON_RELATIVE_SQRD * vNorm2Bound) {
				if (gjkOut) {
					gjkOut->vOut = v;
					gjkOut->closestPointThis = johnsoDistIt.getCollisionPointA();
					gjkOut->closestPointOther = johnsoDistIt.getCollisionPointB();
				}
				out1.str(string());
				out1 << std::fixed << vNorm2Bound - vwDot;
				out2.str(string());
				out2 << std::fixed << EPSILON_RELATIVE_SQRD * vNorm2Bound;
				ServiceLocator::getLogger().logd("GJK", (string("SUCCEEDED: vNorm2Bound - vwDot = ") + out1.str() + string("; EPSILON*vNorm2Bound = ") + out2.str()).c_str());
				lastCollisionV = other.lastCollisionV = v;
				float l= length(v);
				ServiceLocator::getLogger().logd("GJK", (string("SUCCEEDED: objsMarginsSum - length(v) = ") + to_string(objsMarginsSum - length(v))).c_str());	
				ServiceLocator::getLogger().logd("GJK", (string("SUCCEEDED: vOut = ") + to_string(v)).c_str());
				if (gjkOut)
					ServiceLocator::getLogger().logd("GJK", (string("SUCCEEDED: closestPointThis = ") + to_string(gjkOut->closestPointThis)).c_str());
				return fmax(objsMarginsSum - length(v), 0.0f);
			}
			else {
				out1.str(string());
				out1 << std::fixed << vNorm2Bound - vwDot;
				out2.str(string());
				out2 << std::fixed << EPSILON_RELATIVE_SQRD * vNorm2Bound;
				ServiceLocator::getLogger().logd("GJK", (string("Iteration: stop condition failed. vNorm2Bound - vwDot = ") + out1.str() + string("; EPSILON*vNorm2Bound = ") + out2.str()).c_str());
			}

			v = johnsoDistIt.iterate(a, b);
			vNorm2Bound = vNorm2 = length2(v);				
		} while ((johnsoDistIt.wsNr() < 4) && (vNorm2 > EPSILON_TOLERANCE_SQRD * johnsoDistIt.getWNormSqrdMax()));	

		return -1.0f;
	}

	template <class V>
	bool CollisionPrimitive<V>::gjkIntersectionTest(CollisionPrimitive& other, GjkJohnsonsDistanceIterator& johnsonDistIt) {
		V v;
		if (lastCollisionOtherVolume == &other)
			v = lastCollisionV;
		else {
			v = getArbitraryV() - other.getArbitraryV();
			lastCollisionOtherVolume = &other;
			other.lastCollisionOtherVolume = this;
		}

		V a = supportMap(-v);
		V b = other.supportMap(v);
		johnsonDistIt.init(a, b);
		V w = a - b;
		v = w;		
		do {
			a = supportMap(-v);
			b = other.supportMap(v);
			w = a - b;
			float vwDot = dot(v, w);
			if (johnsonDistIt.doesContain(w) || dot(v, w) > 0) {
				lastCollisionV = other.lastCollisionV = v;
				return false;
			}
		
			v = johnsonDistIt.iterate(a, b);
		} while ((johnsonDistIt.wsNr() < 4) && (length2(v) > EPSILON_TOLERANCE_SQRD * johnsonDistIt.getWNormSqrdMax()));

		return true;
	}

	/*
	float CollisionVolume::gjkEpaHybridTest(CollisionVolume& other) {
		// GJK
		vec3 v;
		if (lastTestedCollisionVolume == &other)
			v = lastV;
		else
			v = getArbitraryV() - other.getArbitraryV();

		GjkOut gjkOut;
		runGjkShallowPenetrationTest(CollisionVolume const& volumeA, CollisionVolume const& volumeB, double objsMarginsSum, v, &gjkOut);

		// EPA
		if (distanceIt.wsNr() < 4) {
			vec3 hexahedron[5];
			memcpy(hexahedron, distanceIt.getW(), sizeof(vec3)*distanceIt.wsNr());
			if (distanceIt.wsNr() == 1)
				return 0.0f;
			else if (distanceIt.wsNr() == 2) {
				vec3 d = hexahedron[0] - hexahedron[1];
				vec3 dAbs = abs(d);
				vec3 v1 = cross(d, dAbs.x < dAbs.y ? (dAbs.x < dAbs.z ? vec3(d.x, 0.0f, 0.0f) : vec3(0.0f, 0.0f, d.z)) : (dAbs.y < dAbs.z ? vec3(0.0f, d.y, 0.0f) : vec3(0.0f, 0.0f, d.z)));
				mat3 R = mat3_cast(angleAxis((float)M_PI / 3, normalize(d)));
				vec3 v2 = R * v1;
				vec3 v3 = R * v2;
				hexahedron[2] = supportMap(-v1) - other.supportMap(v1);
				hexahedron[3] = supportMap(-v2) - other.supportMap(v2);
				hexahedron[4] = supportMap(-v3) - other.supportMap(v3);
				// TODO: continue here the EPA
			}
			else if (distanceIt.wsNr() == 3) {

			}
		}

		return 0;
	}

	CollisionVolume::EpaTriangle* CollisionVolume::silhouette(EpaTriangle* epaTriangle, glm::vec3 const& w) {
		EpaTriangle headBuffer;	
		silhouetteRecurse(epaTriangle->adj[2], epaTriangle->j[2], w, silhouetteRecurse(epaTriangle->adj[1], epaTriangle->j[1], w, silhouetteRecurse(epaTriangle->adj[0], epaTriangle->j[0], w, &headBuffer)));
		return headBuffer.silhouetteNext;
	}

	CollisionVolume::EpaTriangle* CollisionVolume::silhouetteRecurse(EpaTriangle* epaTriangle, unsigned int testedEdgeIdx, glm::vec3 const& w, EpaTriangle* silhouetteListTail) {
		if (!epaTriangle->isObsolete) {
			if (dot(epaTriangle->v, w) < length2(epaTriangle->v)) // epaTriangle is visible 
				return silhouetteListTail->silhouetteNext = epaTriangle;		
			else {
				epaTriangle->isObsolete = true;
				unsigned int nextEdgeIdx = (testedEdgeIdx + 1) % 3;
				unsigned int nextNextEdgeIdx = (nextEdgeIdx + 1) % 3;
				return silhouetteRecurse(epaTriangle->adj[nextEdgeIdx], epaTriangle->j[nextEdgeIdx], w, silhouetteRecurse(epaTriangle->adj[nextNextEdgeIdx], epaTriangle->j[nextNextEdgeIdx], w, silhouetteListTail));
			}
		}
		else
			return silhouetteListTail;
	}
	*/

	CollisionPrimitivesFactory::CollisionPrimitivesFactory(unsigned int* primitive3DInstancesNrsMaxima, unsigned int* primitive2DInstancesNrsMaxima) :
		collisionBoxesPool(new ObjPool<CollisionBox>(primitive3DInstancesNrsMaxima[CollisionPrimitive3DType::BOX])),
		collisionSpheresPool(new ObjPool<CollisionSphere>(primitive3DInstancesNrsMaxima[CollisionPrimitive3DType::SPHERE])),
		collisionCapsulesPool(new ObjPool<CollisionCapsule>(primitive3DInstancesNrsMaxima[CollisionPrimitive3DType::CAPSULE])),
		collisionRectsPool(new ObjPool<CollisionRect>(primitive2DInstancesNrsMaxima[CollisionPrimitive2DType::RECT])),
		collisionCirclesPool(new ObjPool<CollisionCircle>(primitive2DInstancesNrsMaxima[CollisionPrimitive2DType::CIRCLE])),
		collisionStadiumsPool(new ObjPool<CollisionStadium>(primitive2DInstancesNrsMaxima[CollisionPrimitive2DType::STADIUM])) {}

	CollisionPrimitivesFactory::~CollisionPrimitivesFactory() {
		delete collisionBoxesPool;
		delete collisionSpheresPool;
		delete collisionCapsulesPool;
		delete collisionRectsPool;
		delete collisionCirclesPool;
		delete collisionStadiumsPool;
	}

	//template <class V>
	//CollisionPrimitive<V>* CollisionPrimitivesFactory::genCollisionPrimitive(CollisionPrimitive<V> const& prototypeCollisionPrimitive) {
	//	return prototypeCollisionPrimitive.clone(*this);
	//}

	CollisionBox* CollisionPrimitivesFactory::genCollisionBox(glm::vec3 const& center, glm::vec3 scale) {
		return collisionBoxesPool->acquire(center, scale);
	}

	CollisionSphere* CollisionPrimitivesFactory::genCollisionSphere(glm::vec3 const& center, float radius) {
		return collisionSpheresPool->acquire(center, radius);
	}

	CollisionCapsule* CollisionPrimitivesFactory::genCollisionCapsule(glm::vec3 const& center1, glm::vec3 const & axisVec, float radius) {
		return collisionCapsulesPool->acquire(center1, axisVec, radius);
	}

	CollisionRect* CollisionPrimitivesFactory::genCollisionRect(glm::vec2 const& center, glm::vec2 scale) {
		return collisionRectsPool->acquire(center, scale);
	}

	CollisionCircle* CollisionPrimitivesFactory::genCollisionCircle(glm::vec2 const& center, float radius) {
		return collisionCirclesPool->acquire(center, radius);
	}

	CollisionStadium* CollisionPrimitivesFactory::genCollisionStadium(glm::vec2 const& center1, glm::vec2 const& axisVec, float radius) {
		return collisionStadiumsPool->acquire(center1, axisVec, radius);
	}

	//template <class V>
	//void CollisionPrimitivesFactory::destroyCollisionPrimitive(CollisionPrimitive<V>* collisionPrimitive) {
	//	collisionPrimitive->destroy(*this);
	//}

	void CollisionPrimitivesFactory::destroyCollisionBox(CollisionBox* collisionBox) {
		collisionBoxesPool->release(collisionBox);
	}

	void CollisionPrimitivesFactory::destroyCollisionSphere(CollisionSphere* collisionSphere) {
		collisionSpheresPool->release(collisionSphere);
	}

	void CollisionPrimitivesFactory::destroyCollisionCapsule(CollisionCapsule* collisionCapsule) {
		collisionCapsulesPool->release(collisionCapsule);
	}

	void CollisionPrimitivesFactory::destroyCollisionRect(CollisionRect* collisionRect) {
		collisionRectsPool->release(collisionRect);
	}

	void CollisionPrimitivesFactory::destroyCollisionCircle(CollisionCircle* collisionCircle) {
		collisionCirclesPool->release(collisionCircle);
	}

	void CollisionPrimitivesFactory::destroyCollisionStadium(CollisionStadium* collisionStadium) {
		collisionStadiumsPool->release(collisionStadium);
	}

	vec3 CollisionVolume::GjkJohnsonsDistanceIterator3D::iterate(vec3 const& aAdded, vec3 const& bAdded) {
		vec3 wAdded = aAdded - bAdded;
		vec3 d[4];

		// testing X = {wAdded}	
		bool wasSubsetFound = true;
		bool wasIwFound = false;
		unsigned int wAddedIw = 4;
		for (unsigned int wIdx = 0; wIdx < W_sz + 1; wIdx++) {
			if (!wasIwFound && !(b & 1 << wIdx)) {
				W[wIdx] = wAdded; A[wIdx] = aAdded; B[wIdx] = bAdded;
				wAddedIw = wIdx;
				wasIwFound = true;
			}

			unsigned int iw = Iw[wIdx];
			if (iw != wAddedIw) {
				d[iw] = W[iw] - wAdded;
				if (wasSubsetFound && dot(-d[iw], wAdded) > 0)
					wasSubsetFound = false;
			}
		}
		if (wasSubsetFound) {
			ServiceLocator::getLogger().logd("GJK", string("EARLY SUCCESS: |W| = 0").c_str());
			Y_sz = W_sz;
			memcpy(Iy, Iw, Y_sz * sizeof(unsigned int));
			Iw[0] = wAddedIw;
			b = 1 << wAddedIw;
			W_sz = 1;
			WNormSqrdMax = WNormsSqrd[wAddedIw] = length2(wAdded);
			return wAdded;
		}

		// testing X = W U {wAdded} where |W| == 1
		for (unsigned int IwIdx = 0; IwIdx < W_sz; IwIdx++) {
			unsigned int iw = Iw[IwIdx];
			vec3 x = W[iw];
			DiX[wAddedIw] = dot(d[iw], x);
			if (1 < W_sz && DiX[wAddedIw] < EPSILON_ZERO) {
				ServiceLocator::getLogger().logd("GJK", (to_string(iw) + string(" [|W| = 1] FAILURE: DiX[wAddedIw] = ") + to_string(DiX[wAddedIw])).c_str());
				continue;
			}
			DiX[iw] = -dot(d[iw], wAdded);
			if (1 < W_sz && DiX[iw] < EPSILON_ZERO) {
				ServiceLocator::getLogger().logd("GJK", (to_string(iw) + string(" [|W| = 1] FAILURE: DiX[") + to_string(iw) + string("] = ") + to_string(DiX[iw])).c_str());
				continue;
			}

			bool wasSubsetFound = true;
			Y_sz = 0;
			for (unsigned int X_sz_2_Jx_idx = 0; X_sz_2_Jx_idx < W_sz - 1; X_sz_2_Jx_idx++) {
				unsigned int jw = Iw[X_SZ_2_Jx[IwIdx][X_sz_2_Jx_idx]];
				vec3 dMinus = -d[jw];
				float s = DiX[wAddedIw] * dot(dMinus, wAdded) + DiX[iw] * dot(dMinus, x);
				if (s > EPSILON_ZERO) {
					ServiceLocator::getLogger().logd("GJK", (to_string(iw) + string(" [|W| = 1] FAILURE: sigma = ") + to_string(s)).c_str());
					wasSubsetFound = false;
					break;
				}
				else
					Iy[Y_sz++] = jw;
			}

			if (wasSubsetFound) {
				vec3 v = (DiX[wAddedIw] * wAdded + DiX[iw] * x) / (DiX[wAddedIw] + DiX[iw]);
				ServiceLocator::getLogger().logd("GJK", (to_string(iw) + string(" [|W| = 1] SUCCESS: v = ") + to_string(v)).c_str());
				Iw[0] = wAddedIw; Iw[1] = iw;
				W_sz = 2;
				b = (1 << wAddedIw) | (1 << iw);
				WNormsSqrd[wAddedIw] = length2(wAdded);
				WNormSqrdMax = fmaxf(WNormsSqrd[wAddedIw], WNormsSqrd[iw]);
				return (DiX[wAddedIw] * wAdded + DiX[iw] * x) / (DiX[wAddedIw] + DiX[iw]);
			}
		}

		// testing X = W U {wAdded} where |W| == 2
		for (unsigned int X_sz_3_Ix_idx = 0; X_sz_3_Ix_idx < nChoose2[W_sz - 2]; X_sz_3_Ix_idx++) {
			unsigned int iw1 = Iw[X_SZ_3_Ix[X_sz_3_Ix_idx][0]];
			unsigned int iw2 = Iw[X_SZ_3_Ix[X_sz_3_Ix_idx][1]];
			vec3 x1 = W[iw1];
			vec3 x2 = W[iw2];
			DiX[wAddedIw] = determinant(mat2(dot(d[iw1], x1), dot(d[iw2], x1), dot(d[iw1], x2), dot(d[iw2], x2)));
			if (2 < W_sz && DiX[wAddedIw] < EPSILON_ZERO) {
				ServiceLocator::getLogger().logd("GJK", (to_string(iw1) + string(" and ") + to_string(iw2) + string(" [|W| = 2] FAILURE: DiX[wAddedIw] =") + to_string(DiX[wAddedIw])).c_str());
				continue;
			}
			DiX[iw1] = -determinant(mat2(dot(d[iw1], wAdded), dot(d[iw2], wAdded), dot(d[iw1], x2), dot(d[iw2], x2)));
			if (2 < W_sz && DiX[iw1] < EPSILON_ZERO) {
				ServiceLocator::getLogger().logd("GJK", (to_string(iw1) + string(" and ") + to_string(iw2) + string(" [|W| = 2] FAILURE: DiX[") + to_string(iw1) + string("] =") + to_string(DiX[iw1])).c_str());
				continue;
			}
			DiX[iw2] = determinant(mat2(dot(d[iw1], wAdded), dot(d[iw2], wAdded), dot(d[iw1], x1), dot(d[iw2], x1)));
			if (2 < W_sz && DiX[iw2] < EPSILON_ZERO) {
				ServiceLocator::getLogger().logd("GJK", (to_string(iw1) + string(" and ") + to_string(iw2) + string(" [|W| = 2] FAILURE: DiX[") + to_string(iw2) + string("] =") + to_string(DiX[iw2])).c_str());
				continue;
			}

			bool wasSubsetFound = true;
			float s = std::numeric_limits<float>::max();
			Y_sz = 0;
			if (W_sz == 3) {
				unsigned int jw = Iw[X_SZ_3_Jx[X_sz_3_Ix_idx]];
				vec3 dMinus = -d[jw];
				s = DiX[wAddedIw] * dot(dMinus, wAdded) + DiX[iw1] * dot(dMinus, x1) + DiX[iw2] * dot(dMinus, x2);
				if (s > EPSILON_ZERO)
					wasSubsetFound = false;
				else
					Iy[Y_sz++] = jw;
			}

			if (wasSubsetFound) {
				vec3 v = (DiX[wAddedIw] * wAdded + DiX[iw1] * x1 + DiX[iw2] * x2) / (DiX[wAddedIw] + DiX[iw1] + DiX[iw2]);
				if (W_sz == 3) {
					ServiceLocator::getLogger().logd("GJK", (to_string(iw1) + string(" and ") + to_string(iw2) + string(" [|W| = 2] SUCCESS: sigma = ") + to_string(s)).c_str());
					ServiceLocator::getLogger().logd("GJK", (string("v = ") + to_string(v)).c_str());
				}
				else {
					ServiceLocator::getLogger().logd("GJK", (to_string(iw1) + string(" and ") + to_string(iw2) + string(" [|W| = 2] SUCCESS: |W| = 2")).c_str());
					ServiceLocator::getLogger().logd("GJK", (string("v = ") + to_string(v)).c_str());
				}
				Iw[0] = wAddedIw; Iw[1] = iw1; Iw[2] = iw2;
				W_sz = 3;
				b = (1 << wAddedIw) | (1 << iw1) | (1 << iw2);
				WNormsSqrd[wAddedIw] = length2(wAdded);
				WNormSqrdMax = fmaxf(WNormsSqrd[wAddedIw], fmaxf(WNormsSqrd[iw1], WNormsSqrd[iw2]));
				return (DiX[wAddedIw] * wAdded + DiX[iw1] * x1 + DiX[iw2] * x2) / (DiX[wAddedIw] + DiX[iw1] + DiX[iw2]);
			}
			ServiceLocator::getLogger().logd("GJK", (to_string(iw1) + string(" and ") + to_string(iw2) + string(" [|W| = 2] FAILURE: sigma = ") + to_string(s)).c_str());
		}

		//W[3] = wAdded;
		W_sz = 4;
		//WNormsSqrd[3] = length2(wAdded);
		//WNormSqrdMax = fmaxf(WNormSqrdMax, WNormsSqrd[3]);
		ServiceLocator::getLogger().logd("GJK", string("FAILURE. W:").c_str());
		for (unsigned int wIdx = 0; wIdx < 3; wIdx++) {
			ServiceLocator::getLogger().logd("GJK", (string("W[") + to_string(Iw[wIdx]) + string("] =") + to_string(W[Iw[wIdx]])).c_str());
		}
		ServiceLocator::getLogger().logd("GJK", (string("W[added] =") + to_string(wAdded)).c_str());

		return vec3(0.0f, 0.0f, 0.0f);
	}

	unsigned int CollisionBox::axIdxs1[3] = { 0, 1, 2 };
	unsigned int CollisionBox::axIdxs2[3] = { 0, 1, 2 };

	vec3 CollisionBox::supportMap(glm::vec3 const& vec) const {
		vec3 ex = r[0] * s.x, ey = r[1] * s.y, ez = r[2] * s.z;
		return c + vec3(dot(vec, ex) < 0.0f ? -ex : ex) + vec3(dot(vec, ey) < 0.0f ? -ey : ey) + vec3(dot(vec, ez) < 0.0f ? -ez : ez);
	}

	vec3 CollisionBox::getArbitraryV() const {
		return c + r[0]*s.x + r[1]*s.y + r[2]*s.z;
	}

	inline unsigned char calcOutCode(glm::vec3 const& vertex, glm::vec3 const& boxMin, glm::vec3 const& boxMax) {
		return ((vertex.x < boxMin.x)) | ((boxMax.x < vertex.x) << 1) |
			   ((vertex.y < boxMin.y) << 2) | ((boxMax.y < vertex.y) << 3) |
			   ((vertex.z < boxMin.z) << 4) | ((boxMax.z < vertex.z) << 5);
	}

	bool CollisionBox::testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) {
		mat3 rTransposed = transpose(r);
		vec3 segOriginRel = rTransposed * (segOrigin - c);
		vec3 segDestRel = rTransposed * (segDest - c);	
		unsigned char segOriginOutCode = calcOutCode(segOriginRel, -s, s);
		unsigned char segDestOutCode = calcOutCode(segDestRel, -s, s);

		if (segOriginOutCode & segDestOutCode)
			return false;

		float segEnterFactorMax = 0.0f;
		float segExitFactorMin = 1.0f;
		if (segOriginOutCode & 0x01)
			segEnterFactorMax = fmax(segEnterFactorMax, (-s.x - segOriginRel.x) / (segDestRel.x - segOriginRel.x));
		else if (segDestOutCode & 0x01)
			segExitFactorMin = fmin(segExitFactorMin, (-s.x - segOriginRel.x) / (segDestRel.x - segOriginRel.x));

		if (segOriginOutCode & 0x02)
			segEnterFactorMax = fmax(segEnterFactorMax, (s.x - segOriginRel.x) / (segDestRel.x - segOriginRel.x));
		else if (segDestOutCode & 0x02)
			segExitFactorMin = fmin(segExitFactorMin, (s.x - segOriginRel.x) / (segDestRel.x - segOriginRel.x));

		if (segOriginOutCode & 0x04)
			segEnterFactorMax = fmax(segEnterFactorMax, (-s.y - segOriginRel.y) / (segDestRel.y - segOriginRel.y));
		else if (segDestOutCode & 0x04)
			segExitFactorMin = fmin(segExitFactorMin, (-s.y - segOriginRel.y) / (segDestRel.y - segOriginRel.y));

		if (segOriginOutCode & 0x08)
			segEnterFactorMax = fmax(segEnterFactorMax, (s.y - segOriginRel.y) / (segDestRel.y - segOriginRel.y));
		else if (segDestOutCode & 0x08)
			segExitFactorMin = fmin(segExitFactorMin, (s.y - segOriginRel.y) / (segDestRel.y - segOriginRel.y));

		if (segOriginOutCode & 0x10)
			segEnterFactorMax = fmax(segEnterFactorMax, (-s.z - segOriginRel.z) / (segDestRel.z - segOriginRel.z));
		else if (segDestOutCode & 0x10)
			segExitFactorMin = fmin(segExitFactorMin, (-s.z - segOriginRel.z) / (segDestRel.z - segOriginRel.z));

		if (segOriginOutCode & 0x20)
			segEnterFactorMax = fmax(segEnterFactorMax, (s.z - segOriginRel.z) / (segDestRel.z - segOriginRel.z));
		else if (segDestOutCode & 0x20)
			segExitFactorMin = fmin(segExitFactorMin, (s.z - segOriginRel.z) / (segDestRel.z - segOriginRel.z));

		if (segEnterFactorMax <= segExitFactorMin) {
			segFactorOnCollisionOut = segEnterFactorMax;
			return true;
		}
		else
			return false;
	}

	bool CollisionBox::testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) {	
		mat3 rTransposed = transpose(r);
		vec3 segOriginRel = rTransposed * (segOrigin - c);
		vec3 segDestRel = rTransposed * (segDest - c);
		for (unsigned int axIdx = 0; axIdx < 3; axIdx++) {
			float originProj = segOriginRel[axIdx];
			float destProj = segDestRel[axIdx];
			if (abs(originProj + destProj) > abs(originProj - destProj) + 2*s[axIdx])
				return false;
		}
	
		vec3 rayDirectionRel = segDestRel - segOriginRel;
		glm::vec3 unitVec(0.0f, 0.0f, 0.0f);
		for (unsigned int crossProductAxIdx = 0; crossProductAxIdx < 3; crossProductAxIdx++) {		
			unitVec[crossProductAxIdx] = 1.0f;
			vec3 crossProductAx = cross(unitVec, rayDirectionRel);
			if ( abs(dot(crossProductAx, segOriginRel)) > dot(abs(crossProductAx), s) )
				return false;
			unitVec[crossProductAxIdx] = 0.0f;
		}

		return true;
	}

	// Reminder: crashed on a slow shallow penetration with a capsule when r[2] was initialized to {0.0f, EPSILON_ZERO, 1.0f} 
	CollisionBox::CollisionBox(glm::vec3 const& center, glm::vec3 scale) :
		c(center), r{ {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, s{ abs(scale.x), abs(scale.y), abs(scale.z) } {}

	CollisionVolume* CollisionBox::clone(CollisionPrimitivesFactory& collisionPrimitivesFactory) const {
		return collisionPrimitivesFactory.genCollisionBox(c, s);
	}

	void CollisionBox::destroy(CollisionPrimitivesFactory& CollisionVolumesFactory) {
		CollisionVolumesFactory.destroyCollisionBox(this);
	}

	inline bool areVecsParallel(vec3 const& pointA, vec3 const& pointB) {
		return !(abs(pointA.y * pointB.z - pointA.z * pointB.y) > EPSILON_ZERO ||
			abs(pointA.x * pointB.z - pointA.z * pointB.x) > EPSILON_ZERO ||
			abs(pointA.x * pointB.y - pointA.y * pointB.x) > EPSILON_ZERO);
	}

	inline float pointPlaneDist(vec3 const& point, vec3 const& planeNormal, vec3 const& planePoint) {
		return dot(planeNormal, point - planePoint);
	}

	inline float pointPlaneDist(vec3 const& point, vec3 const& planeNormal, float planeD) {
		return dot(planeNormal, point) + planeD;
	}

	inline void CollisionBox::movePlaneAxIdxInBuffer(unsigned int idx) {
		axIdxs1[idx] = 0;
		axIdxs1[0] = idx;
	}

	inline void CollisionBox::moveEdgesAxsIdxsInBuffers(unsigned int idx1, unsigned int idx2) {
		axIdxs1[idx1] = 0;
		axIdxs1[0] = idx1;
		axIdxs2[idx2] = 0;
		axIdxs2[0] = idx2;
	}

	inline void CollisionBox::revertPlaneIdxToBufferStart(unsigned int idx) {
		axIdxs1[0] = 0;
		axIdxs1[idx] = idx;
	}

	inline void CollisionBox::revertEdgesIdxsToBuffersStart(unsigned int idx1, unsigned int idx2) {
		axIdxs1[0] = 0;
		axIdxs1[idx1] = idx1;
		axIdxs2[0] = 0;
		axIdxs2[idx2] = idx2;
	}

	inline vec3 CollisionBox::supportMapMinusDirection(vec3 const& vec, unsigned int minusDirectionIdx) {
		vec3 penetrationEdgePerpAx1 = r[(minusDirectionIdx + 1) % 3] * s[(minusDirectionIdx + 1) % 3];
		vec3 penetrationEdgePerpAx2 = r[(minusDirectionIdx + 2) % 3] * s[(minusDirectionIdx + 2) % 3];
		return c + vec3(dot(vec, penetrationEdgePerpAx1) < 0.0f ? -penetrationEdgePerpAx1 : penetrationEdgePerpAx1) +
				   vec3(dot(vec, penetrationEdgePerpAx2) < 0.0f ? -penetrationEdgePerpAx2 : penetrationEdgePerpAx2) -
				   r[minusDirectionIdx] * s[minusDirectionIdx];
	}

	inline void clipCapsuleVecWithBoxPlane(vec3* clipPoints, vec3 const& planeN, float planeD) {
		float c1DistFromPlane = pointPlaneDist(clipPoints[0], planeN, planeD);
		float c2DistFromPlane = pointPlaneDist(clipPoints[1], planeN, planeD);
		if (c1DistFromPlane > 0.0f)
			clipPoints[0] = clipPoints[0] + (c1DistFromPlane / (c1DistFromPlane - c2DistFromPlane))*(clipPoints[1] - clipPoints[0]);
		else if (c2DistFromPlane > 0.0f)
			clipPoints[1] = clipPoints[1] + (c2DistFromPlane / (c2DistFromPlane - c1DistFromPlane))*(clipPoints[0] - clipPoints[1]);
	}

	void CollisionBox::clipCapsuleVec(CollisionCapsule const* capsule, unsigned int facenAxIdx, vec3 const& faceNormal, vec3* clipPointsOut) {
		clipPointsOut[0] = capsule->getC1();
		clipPointsOut[1] = capsule->getC1() + capsule->getV();
		vec3 capsuleVNormalized = capsule->getV() / length(capsule->getV());
		for (unsigned int oppositeClipPlanesDuoIdx = 1; oppositeClipPlanesDuoIdx <= 2; oppositeClipPlanesDuoIdx++) {
			unsigned int clipPlaneIdx = (facenAxIdx + oppositeClipPlanesDuoIdx) % 3;
			vec3 n = r[clipPlaneIdx]; // clip plane normal
			float d = -dot(n, c + s * n); // clip plane d					
			clipCapsuleVecWithBoxPlane(clipPointsOut, n, d);
			clipCapsuleVecWithBoxPlane(clipPointsOut, -n, d + 2.0f*dot(c, n));
		}

		// project the clip points onto the reference plane
		vec3 referencePlanePoint = c + faceNormal * s[facenAxIdx];
		for (unsigned int pointIdx = 0; pointIdx < 2; pointIdx++)				
			clipPointsOut[pointIdx] = clipPointsOut[pointIdx] - pointPlaneDist(clipPointsOut[pointIdx], faceNormal, referencePlanePoint)*faceNormal;
	}

	bool CollisionBox::testCollision(CollisionBox* other) { 
		GjkJohnsonsDistanceIterator3D johnsonDistIt;
		return gjkIntersectionTest(*other, johnsonDistIt);
	}

	bool CollisionBox::testCollision(CollisionBox* other, ContactManifold& manifoldOut) {
		FacePenetrationDesc facesPenetrationDesc;
		EdgePenetrationDesc edgesPenetrationDesc;
		manifoldOut.pointsNr = 0;
		if (lastCollisionOtherVolume == other && other->lastCollisionOtherVolume == this) {
			if (!wasLastFrameSeparated) {
				if (lastCollisionResLmntIdx < 3) {
					// last collision was an edges collision

				}
				else {
					// last collision was a faces collision				

				}
			}

			// test separation		
			if (lastCollisionResLmntIdx < 3) {
				unsigned int cachedLastCollisionResLmntIdx = lastCollisionResLmntIdx;
				unsigned int cachedOtherLastCollisionResLmntIdx = other->lastCollisionResLmntIdx;
				moveEdgesAxsIdxsInBuffers(cachedLastCollisionResLmntIdx, cachedOtherLastCollisionResLmntIdx);
				bool areEdgesSeparation = testEdgesSeparation(*other, edgesPenetrationDesc);
				revertEdgesIdxsToBuffersStart(cachedLastCollisionResLmntIdx, cachedOtherLastCollisionResLmntIdx);
				if (areEdgesSeparation)
					return false;

				if (testFacesSeparation(*other, facesPenetrationDesc))
					return false;
			}
			else {
				if (lastCollisionResLmntIdx < 6) {
					unsigned int cachedLastCollisionResLmntIdx = lastCollisionResLmntIdx - 3;
					movePlaneAxIdxInBuffer(cachedLastCollisionResLmntIdx);
					bool areFacesSeparated = testFacesSeparation(*other, facesPenetrationDesc);
					revertPlaneIdxToBufferStart(cachedLastCollisionResLmntIdx);
					if (areFacesSeparated)
						return false;
				}
				else if (other->lastCollisionResLmntIdx < 6) {
					unsigned int cachedLastCollisionResLmntIdx = other->lastCollisionResLmntIdx - 3;
					movePlaneAxIdxInBuffer(cachedLastCollisionResLmntIdx);
					bool areFacesSeparated = other->testFacesSeparation(*this, facesPenetrationDesc);				
					revertPlaneIdxToBufferStart(cachedLastCollisionResLmntIdx);
					if (areFacesSeparated)
						return false;
					else {
						std::swap(facesPenetrationDesc.penetrationMinAxIdxThis, facesPenetrationDesc.penetrationMinAxIdxOther);
						std::swap(facesPenetrationDesc.penetrationThis, facesPenetrationDesc.penetrationOther);
					}
				}
				else if (testFacesSeparation(*other, facesPenetrationDesc))
					return false;

				if (testEdgesSeparation(*other, edgesPenetrationDesc))
					return false;
			}
		}
		else if (testFacesSeparation(*other, facesPenetrationDesc) || testEdgesSeparation(*other, edgesPenetrationDesc))
			return false;

		// Couldn't find A separating axis => boxes are colliding	
		wasLastFrameSeparated = other->wasLastFrameSeparated = false;

		// calculate edges penetration	
		vec3 edgeThis = r[edgesPenetrationDesc.penetrationMinAxEdgeIdxThis];
		vec3 edgeOther = other->r[edgesPenetrationDesc.penetrationMinAxEdgeIdxOther];
		vec3 edgesCollisionNormal = normalize(cross(edgeThis, edgeOther));
		if (dot(edgesCollisionNormal, c - other->c) > 0)
			edgesCollisionNormal = -edgesCollisionNormal;			
		vec3 edgePointThis = supportMapMinusDirection(edgesCollisionNormal, edgesPenetrationDesc.penetrationMinAxEdgeIdxThis);
		vec3 edgePointOther = other->supportMapMinusDirection(-edgesCollisionNormal, edgesPenetrationDesc.penetrationMinAxEdgeIdxOther);
		vec3 pointThisPointOtherVec = edgePointOther - edgePointThis;
		float edgesPenetration = dot(edgesCollisionNormal, pointThisPointOtherVec);
	
		// calculate the contact manifold
		if (facesPenetrationDesc.penetrationThis - edgesPenetration >= -FACE_TO_EDGE_COMPARISON_ZERO || facesPenetrationDesc.penetrationOther - edgesPenetration >= -FACE_TO_EDGE_COMPARISON_ZERO) {
			// face contact
			if (facesPenetrationDesc.penetrationThis >= facesPenetrationDesc.penetrationOther)		
				createFaceContact(*other, facesPenetrationDesc.penetrationMinAxIdxThis, facesPenetrationDesc.penetrationThis, manifoldOut);
			else		
				other->createFaceContact(*this, facesPenetrationDesc.penetrationMinAxIdxOther, facesPenetrationDesc.penetrationOther, manifoldOut);				
		}
		else {
			// edge contact
			manifoldOut.penetrationDepth = edgesPenetration;
			manifoldOut.pointsNr = 1;	
			manifoldOut.normal = edgesCollisionNormal;	
				
			float v1p = dot(edgeThis, pointThisPointOtherVec);
			float v2p = dot(edgeOther, pointThisPointOtherVec);
			float v1v2 = dot(edgeThis, edgeOther);
			float v1v1 = dot(edgeThis, edgeThis);
			float v2v2 = dot(edgeOther, edgeOther);
			float denominator = v1v1*v2v2 - v1v2 * v1v2;
			float factorThis = fmin(fmax(v1p*v2v2 - v1v2*v2p, 0.0f) / denominator, 2.0f*s[edgesPenetrationDesc.penetrationMinAxEdgeIdxThis]);
			float factorOther = fmin(fmax(v1v2*v1p - v2p*v1v1, 0.0f) / denominator, 2.0f*other->s[edgesPenetrationDesc.penetrationMinAxEdgeIdxOther]);
			manifoldOut.points[0] = 0.5f*(edgePointThis + factorThis*edgeThis + edgePointOther + factorOther*edgeOther);
			//manifoldOut.points[1] = edgePointThis;
			//manifoldOut.points[2] = edgePointOther;
			manifoldOut.points[1] = edgePointThis + edgeThis * factorThis;
			manifoldOut.points[2] = edgePointOther + edgeOther * factorOther;				
		}
	
		return true;
	}

	// Reminder: the separation calculated here is the actual separation
	bool CollisionBox::testFacesSeparation(CollisionBox& other, FacePenetrationDesc& penetrationDescOut) {
		//constexpr float separationMax = -std::numeric_limits<float>::max();	
		mat3 thisRTransposed = transpose(r);
		mat3 otherRelR = thisRTransposed*other.r;
		mat3 otherRelRp = { abs(otherRelR[0][0]) + EPSILON_ZERO, abs(otherRelR[0][1]) + EPSILON_ZERO, abs(otherRelR[0][2]) + EPSILON_ZERO,
							abs(otherRelR[1][0]) + EPSILON_ZERO, abs(otherRelR[1][1]) + EPSILON_ZERO, abs(otherRelR[1][2]) + EPSILON_ZERO,
							abs(otherRelR[2][0]) + EPSILON_ZERO, abs(otherRelR[2][1]) + EPSILON_ZERO, abs(otherRelR[2][2]) + EPSILON_ZERO };
		vec3 otherRelC = thisRTransposed*(other.c - c);
	
		for (unsigned int axIdxIdx = 0; axIdxIdx < 3; axIdxIdx++) {
			unsigned int axIdx = axIdxs1[axIdxIdx];
			float separation = abs(otherRelC[axIdx]) - (s[axIdx] + other.s[0]*otherRelRp[0][axIdx] + other.s[1]*otherRelRp[1][axIdx] + other.s[2]*otherRelRp[2][axIdx]);
			if (separation > 0.0f) {			
				lastCollisionOtherVolume = &other;
				other.lastCollisionOtherVolume = this;
				lastCollisionResLmntIdx = axIdx + 3;
				other.lastCollisionResLmntIdx = 6;					
				return wasLastFrameSeparated = other.wasLastFrameSeparated = true;
			}
			else if (separation > penetrationDescOut.penetrationThis) {
				penetrationDescOut.penetrationMinAxIdxThis = axIdx;
				penetrationDescOut.penetrationThis = separation;
			}
		}

		for (unsigned int axIdxIdx = 0; axIdxIdx < 3; axIdxIdx++) {
			unsigned int axIdx = axIdxs1[axIdxIdx];		
			float separation = abs(dot(otherRelC, otherRelR[axIdx])) - (other.s[axIdx] + dot(s, otherRelRp[axIdx]));		
			if (separation > 0.0f) {
				lastCollisionOtherVolume = &other;
				other.lastCollisionOtherVolume = this;
				lastCollisionResLmntIdx = 6;
				other.lastCollisionResLmntIdx = axIdx + 3;			
				return wasLastFrameSeparated = other.wasLastFrameSeparated = true;
			}
			else if (separation > penetrationDescOut.penetrationOther) {
				penetrationDescOut.penetrationMinAxIdxOther = axIdx;
				penetrationDescOut.penetrationOther = separation;			
			}
		}
			
		return false;
	}

	// Reminder: the separation value calculated here does not correspond to the real separation/penetration
	//			 due to the axis tested here being calculated via cross product which does not produce a normalized vector 
	//			 for non-perpendicular argument vectors
	bool CollisionBox::testEdgesSeparation(CollisionBox& other, EdgePenetrationDesc& penetrationDescOut) {
		mat3 thisRTransposed = transpose(r);
		mat3 otherRelR = thisRTransposed*other.r;
		mat3 otherRelRp = { abs(otherRelR[0][0]) + EPSILON_ZERO, abs(otherRelR[0][1]) + EPSILON_ZERO, abs(otherRelR[0][2]) + EPSILON_ZERO,
							abs(otherRelR[1][0]) + EPSILON_ZERO, abs(otherRelR[1][1]) + EPSILON_ZERO, abs(otherRelR[1][2]) + EPSILON_ZERO,
							abs(otherRelR[2][0]) + EPSILON_ZERO, abs(otherRelR[2][1]) + EPSILON_ZERO, abs(otherRelR[2][2]) + EPSILON_ZERO };
		vec3 otherRelC = thisRTransposed*(other.c - c);

		float separationValMax = -std::numeric_limits<float>::max();
		for (unsigned int thisII = 0; thisII < 3; thisII++) {
			unsigned int thisI = axIdxs1[thisII];
			for (unsigned int otherII = 0; otherII < 3; otherII++) {
				unsigned int otherI = axIdxs2[otherII];
				if (areVecsParallel(r[thisI], other.r[otherI]))
					continue;
				float separationVal = abs(otherRelC[(thisI + 2)%3]*otherRelR[otherI][(thisI + 1)%3] - otherRelC[(thisI + 1)%3]*otherRelR[otherI][(thisI + 2)%3]) -
								   (s[(thisI + 1)%3]*otherRelRp[otherI][(thisI + 2)%3] + s[(thisI + 2)%3]*otherRelRp[otherI][(thisI + 1)%3]) -
								   (other.s[(otherI + 1)%3]*otherRelRp[(otherI + 2)%3][thisI] + other.s[(otherI + 2)%3]*otherRelRp[(otherI + 1)%3][thisI]);
				if (separationVal > 0) {
					lastCollisionOtherVolume = &other;
					other.lastCollisionOtherVolume = this;
					lastCollisionResLmntIdx = thisI;
					other.lastCollisionResLmntIdx = otherI;					
					return wasLastFrameSeparated = other.wasLastFrameSeparated = true;
				}
				else if (separationVal > separationValMax) {
					separationValMax = separationVal;
					penetrationDescOut.penetrationMinAxEdgeIdxThis = lastCollisionResLmntIdx = thisI;
					penetrationDescOut.penetrationMinAxEdgeIdxOther = other.lastCollisionResLmntIdx = otherI;								
				}			
			}
		}
	
		return false;
	}

	// In: 
	// * n -> clip plane normal
	// * p -> clip plane d
	// * v -> polygon vertices
	// * vNr -> number of vertices (must be >= 3)
	// Out:
	// Updated vertices number after clip
	inline unsigned int clipPolygonByPlane(vec3 const& n, float d, vec3* v, unsigned int vNr) {
		//if (vNr > 2) {
	#if DEBUG
		if (vNr < 3)
			throw invalid_argument("Bad argument to clipPolygonByPlane. vNr must be >= 3.");
	#endif
		vec3 vStart = v[0];
		v[vNr] = v[0];
		float vStartDist = pointPlaneDist(vStart, n, d);
		unsigned int updatedVerticesNr = 0;
		for (unsigned int vIdx = 0; vIdx < vNr; vIdx++) {
			vec3 vEnd = v[vIdx + 1];
			float vEndDist = pointPlaneDist(vEnd, n, d);
			if (vStartDist <= 0.0f) { // start in
				v[updatedVerticesNr++] = vStart;
				if (0.0f < vEndDist) // start in & end out
					v[updatedVerticesNr++] = vStart + (vStartDist / (vStartDist - vEndDist))*(vEnd - vStart);
			}
			else if (vEndDist <= 0.0f) // start out & end in
				v[updatedVerticesNr++] = vStart + (vStartDist / (vStartDist - vEndDist))*(vEnd - vStart);

			vStart = vEnd;
			vStartDist = vEndDist;
		}

		return updatedVerticesNr;

		//}
		//else {
		//	float v1DistFromPlane = pointPlaneDist(v[0], n, d);
		//	float v2DistFromPlane = pointPlaneDist(v[1], n, d);
		//	if (v1DistFromPlane > 0.0f && v2DistFromPlane > 0.0f)
		//		return 0;
		//	else if (v1DistFromPlane > 0.0f)
		//		v[0] += (v1DistFromPlane / (v1DistFromPlane - v2DistFromPlane))*(v[1] - v[0]);		
		//	else if (v2DistFromPlane > 0.0f)
		//		v[1] += (v2DistFromPlane / (v2DistFromPlane - v1DistFromPlane))*(v[0] - v[1]);
		//						
		//	return 2;
		//}
	}

	void CollisionBox::createFaceContact(CollisionBox& other, unsigned int penetrationMinAxIdx, float penetration, ContactManifold& manifoldOut) {					
		manifoldOut.penetrationDepth = penetration;
		lastCollisionResLmntIdx = penetrationMinAxIdx + 3;		

		// calculate the reference plane normal
		vec3 referencePlaneNormal = r[penetrationMinAxIdx];
		if (dot(referencePlaneNormal, c - other.c) > 0)
			referencePlaneNormal = -referencePlaneNormal;
		manifoldOut.normal = referencePlaneNormal;

		// calculate the incident plane normal
		float r0OtherDotReference = abs(dot(other.r[0], referencePlaneNormal));
		float r1OtherDotReference = abs(dot(other.r[1], referencePlaneNormal));
		unsigned int incidentPlaneNormalIdx = r0OtherDotReference >= r1OtherDotReference ? (r0OtherDotReference >= abs(dot(other.r[2], referencePlaneNormal)) ? 0 : 2) : (r1OtherDotReference >= abs(dot(other.r[2], referencePlaneNormal)) ? 1 : 2);
		other.lastCollisionResLmntIdx = incidentPlaneNormalIdx + 3;
		vec3 incidentPlaneNormal = other.r[incidentPlaneNormalIdx];
		if (dot(incidentPlaneNormal, referencePlaneNormal) > 0)
			incidentPlaneNormal = -incidentPlaneNormal;
	
		// clip the incident plane - Sutherland-Hodgeman algo	
		unsigned int incidentPlanePerpVec1Idx = (incidentPlaneNormalIdx + 1) % 3;
		vec3 incidentPlanePerpVec1 = other.r[incidentPlanePerpVec1Idx]*other.s[incidentPlanePerpVec1Idx];
		unsigned int incidentPlanePerpVec2Idx = (incidentPlaneNormalIdx + 2) % 3;
		vec3 incidentPlanePerpVec2 = other.r[incidentPlanePerpVec2Idx]*other.s[incidentPlanePerpVec2Idx];
		// Reminder: why 9 -> 8 possible clip points + 1 extra space for the v[vNr] = v[0] copy at the start of clipPolygonByPlane
		vec3 contactPoints[9];
		vec3 incidentPlaneCenter = other.c + other.s[incidentPlaneNormalIdx]*incidentPlaneNormal;
		contactPoints[0] = incidentPlaneCenter + incidentPlanePerpVec1 + incidentPlanePerpVec2;
		contactPoints[1] = incidentPlaneCenter + incidentPlanePerpVec1 - incidentPlanePerpVec2;
		contactPoints[2] = incidentPlaneCenter - incidentPlanePerpVec1 - incidentPlanePerpVec2;
		contactPoints[3] = incidentPlaneCenter - incidentPlanePerpVec1 + incidentPlanePerpVec2;
		unsigned int clipPointsNr = 4;
		for (unsigned int oppositeClipPlanesDuoIdx = 1; oppositeClipPlanesDuoIdx <= 2; oppositeClipPlanesDuoIdx++) {
			unsigned int clipPlaneIdx = (penetrationMinAxIdx + oppositeClipPlanesDuoIdx) % 3;
			vec3 n = r[clipPlaneIdx]; // clip plane normal
			float d = -dot(n, c + s * n); // clip plane d
			clipPointsNr = clipPolygonByPlane(n, d, contactPoints, clipPointsNr);
			clipPointsNr = clipPolygonByPlane(-n, d + 2.0f*dot(c,n), contactPoints, clipPointsNr);
		}

		// discard points above the reference plane and project the rest onto the reference plane
		vec3 referencePlanePoint = c + referencePlaneNormal*s[penetrationMinAxIdx];
		for (unsigned int pointIdx = 0; pointIdx < clipPointsNr; pointIdx++) {
			float pointDistFromReference = pointPlaneDist(contactPoints[pointIdx], referencePlaneNormal, referencePlanePoint);
			if (pointDistFromReference < 0)
				manifoldOut.points[manifoldOut.pointsNr++] = contactPoints[pointIdx] - pointDistFromReference*referencePlaneNormal;
		}	

		// TODO: Experiment with this -> Contact points reduction
		// if (manifoldOut.pointsNr > 4) {}		
	}

	bool CollisionBox::testCollision(CollisionSphere* sphere) {	
		GjkJohnsonsDistanceIterator3D johnsonDistIt;
		if (gjkShallowPenetrationTest(*sphere, sphere->getR(), johnsonDistIt, NULL) == 0.0f)
			return false;
		else
			return true;
	}

	bool CollisionBox::testCollision(CollisionSphere* sphere, ContactManifold& manifoldOut) {
		GjkJohnsonsDistanceIterator3D johnsonDistIt;
		GjkOut gjkOut;
		float penetrationDepth = gjkShallowPenetrationTest(*sphere, sphere->getR(), johnsonDistIt , &gjkOut);
		if (penetrationDepth == 0.0f) {
			manifoldOut.pointsNr = 0;
			return false;
		}
		else if (penetrationDepth > 0.0f) {
			// shallow penetration
			manifoldOut.pointsNr = 1;
			manifoldOut.normal = -normalize(gjkOut.vOut);
			manifoldOut.points[0] = gjkOut.closestPointThis;
			manifoldOut.penetrationDepth = -penetrationDepth;	
		}
		else {
			// deep penetration
			vec3 sphereCboxCVec = c - sphere->getC();
			float projR0 = dot(sphereCboxCVec, r[0]);
			float projR1 = dot(sphereCboxCVec, r[1]);
			float projR2 = dot(sphereCboxCVec, r[2]);
		
			vec3 r0, r1, r2;
			if (projR0 < 0) {
				r0 = r[0];
				projR0 = -projR0;
			}
			else
				r0 = -r[0];
			if (projR1 < 0) {
				r1 = r[1];
				projR1 = -projR1;
			}
			else
				r1 = -r[1];
			if (projR2 < 0) {
				r2 = r[2];
				projR2 = -projR2;
			}
			else
				r2 = -r[2];

			float penetrationR0 = projR0 - s[0];
			float penetrationR1 = projR1 - s[1];
			float penetrationR2 = projR2 - s[2];
			if (penetrationR0 > penetrationR1) {
				if (penetrationR0 > penetrationR2) {
					manifoldOut.penetrationDepth = penetrationR0;
					manifoldOut.normal = r0;
				}
				else {
					manifoldOut.penetrationDepth = penetrationR2;
					manifoldOut.normal = r2;
				}
			}
			else { // (penetrationR0 < penetrationR1)
				if (penetrationR1 > penetrationR2) {
					manifoldOut.penetrationDepth = penetrationR1;
					manifoldOut.normal = r1;
				}
				else {
					manifoldOut.penetrationDepth = penetrationR2;
					manifoldOut.normal = r2;
				}
			}	

			manifoldOut.pointsNr = 1;
			manifoldOut.points[0] = sphere->getC() - manifoldOut.normal*manifoldOut.penetrationDepth;
		}

		return true;
	}

	bool CollisionBox::testCollision(CollisionCapsule* capsule) {
		GjkJohnsonsDistanceIterator3D johnsonDistIt;
		if (gjkShallowPenetrationTest(*capsule, capsule->getR(), johnsonDistIt, NULL) == 0.0f)
			return false;
		else
			return true;
	}

	bool CollisionBox::testCollision(CollisionCapsule* capsule, ContactManifold & manifoldOut) {
		GjkJohnsonsDistanceIterator3D johnsonDistIt;
		GjkOut gjkOut;
		float penetrationDepth = gjkShallowPenetrationTest(*capsule, capsule->getR(), johnsonDistIt, &gjkOut);
		//ServiceLocator::getLogger().logd("GJK", (string("penetration: ") + std::to_string(penetrationDepth)).c_str());
		if (penetrationDepth == 0.0f) {
			manifoldOut.pointsNr = 0;
			return false;
		}
		else if (penetrationDepth > 0.0f) {
			// shallow penetration		
			manifoldOut.normal = -normalize(gjkOut.vOut);		
			manifoldOut.penetrationDepth = -penetrationDepth;
			unsigned int minPenetrationFaceAxIdx = 3;
			for (unsigned int faceAxIdx = 0; faceAxIdx < 3; faceAxIdx++) {
				if (areVecsParallel(r[faceAxIdx], manifoldOut.normal)) {
					minPenetrationFaceAxIdx = faceAxIdx;
					break;
				}
			}

			if (minPenetrationFaceAxIdx == 3) {
				manifoldOut.pointsNr = 1;
				manifoldOut.points[0] = gjkOut.closestPointThis;			
			}
			else {													
				manifoldOut.pointsNr = 2;
				clipCapsuleVec(capsule, minPenetrationFaceAxIdx, manifoldOut.normal, manifoldOut.points);
			}
		}
		else {
			// deep penetration					
			mat3 rTransposed = transpose(r);
		
			float capsuleVLen = length(capsule->getV());
			vec3 capsuleAxesLens = { capsuleVLen/2 + capsule->getR(), capsule->getR(), capsule->getR() };
		
			vec3 capsuleVRel = rTransposed * capsule->getV() / capsuleVLen;
			// calculating the axes matrix according to capsuleAxesRel = 
			// {capsuleVRel, _|_ capsuleVRel with z = 0, cross product between the previous two}
			mat3 capsuleAxesRel = { capsuleVRel, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }; // initialized to the result vectors for when capsuleVRel = {0, 0, 1}
			if (capsuleVRel.x > EPSILON_ZERO || capsuleVRel.y > EPSILON_ZERO) {
				float a = sqrtf(capsuleVRel.x*capsuleVRel.x + capsuleVRel.y*capsuleVRel.y);
				capsuleAxesRel[1] = { -capsuleVRel.y / a, capsuleVRel.x / a, 0.0f };
				capsuleAxesRel[2] = { capsuleVRel.x*capsuleVRel.z / a, capsuleVRel.y*capsuleVRel.z / a, -(capsuleVRel.x*capsuleVRel.x + capsuleVRel.y*capsuleVRel.y) / a };
			}
			mat3 capsuleAxesRelP = { abs(capsuleAxesRel[0][0]) + EPSILON_ZERO, abs(capsuleAxesRel[0][1]) + EPSILON_ZERO, abs(capsuleAxesRel[0][2]) + EPSILON_ZERO,
									 abs(capsuleAxesRel[1][0]) + EPSILON_ZERO, abs(capsuleAxesRel[1][1]) + EPSILON_ZERO, abs(capsuleAxesRel[1][2]) + EPSILON_ZERO,
									 abs(capsuleAxesRel[2][0]) + EPSILON_ZERO, abs(capsuleAxesRel[2][1]) + EPSILON_ZERO, abs(capsuleAxesRel[2][2]) + EPSILON_ZERO };

			vec3 boxCenterCapsuleCenterVec = capsule->getC1() + 0.5f*capsule->getV() - c;
			vec3 capsuleCenterRel = rTransposed * boxCenterCapsuleCenterVec;

			unsigned int minPenetrationFaceAxIdx;
			float facePenetrationMax = -std::numeric_limits<float>::max();
			for (unsigned int axIdx = 0; axIdx < 3; axIdx++) {
				float penetration = abs(capsuleCenterRel[axIdx]) - (s[axIdx] + capsuleAxesLens[0] * capsuleAxesRelP[0][axIdx] + capsuleAxesLens[1] * capsuleAxesRelP[1][axIdx] + capsuleAxesLens[2] * capsuleAxesRelP[2][axIdx]);
				if (penetration > facePenetrationMax) {
					facePenetrationMax = penetration;
					minPenetrationFaceAxIdx = axIdx;
				}
			}
						
			unsigned int minEdgePenetrationAxIdx;
			float edgePenetrationValMax = -std::numeric_limits<float>::max();		
			for (unsigned int axIdx = 0; axIdx < 3; axIdx++) {								
				float penetrationVal = abs(capsuleCenterRel[(axIdx + 2) % 3]*capsuleAxesRel[0][(axIdx + 1) % 3] - capsuleCenterRel[(axIdx + 1) % 3]*capsuleAxesRel[0][(axIdx + 2) % 3]) -
					(s[(axIdx + 1) % 3]*capsuleAxesRelP[0][(axIdx + 2) % 3] + s[(axIdx + 2) % 3]*capsuleAxesRelP[0][(axIdx + 1) % 3]) -
					(capsuleAxesLens[1]*capsuleAxesRelP[2][axIdx] + capsuleAxesLens[2]*capsuleAxesRelP[1][axIdx]);
				if (penetrationVal > edgePenetrationValMax) {
					edgePenetrationValMax = penetrationVal;
					minEdgePenetrationAxIdx = axIdx;
				}
			}
		
			// calculate edges penetration					
			vec3 edgeCollisionNormal;
			vec3 pointBox;
			vec3 pointCapsule;
			vec3 pointBoxPointCapsuleVec;
			vec3 edgeBox = r[minEdgePenetrationAxIdx];
			float edgePenetration;
			if (!areVecsParallel(edgeBox, capsule->getV())) {
				edgeCollisionNormal = normalize(cross(edgeBox, capsule->getV()));
				if (dot(edgeCollisionNormal, boxCenterCapsuleCenterVec) > 0)
					edgeCollisionNormal = -edgeCollisionNormal;
				pointBox = supportMapMinusDirection(edgeCollisionNormal, minEdgePenetrationAxIdx);
				pointCapsule = capsule->getC1() + capsule->getR()*edgeCollisionNormal;
				pointBoxPointCapsuleVec = pointCapsule - pointBox;
				edgePenetration = dot(edgeCollisionNormal, pointBoxPointCapsuleVec);
			}
			else
				edgePenetration = std::numeric_limits<float>::max();

			// calculate the contact manifold
			if (facePenetrationMax < edgePenetration) {
				// face contact
				manifoldOut.penetrationDepth = facePenetrationMax;
				vec3 referencePlaneNormal = r[minPenetrationFaceAxIdx];
				if (dot(referencePlaneNormal, boxCenterCapsuleCenterVec) < 0)
					referencePlaneNormal = -referencePlaneNormal;
				manifoldOut.normal = referencePlaneNormal;		
				manifoldOut.pointsNr = 2;
				clipCapsuleVec(capsule, minPenetrationFaceAxIdx, manifoldOut.normal, manifoldOut.points);
			}
			else {
				// edge contact
				manifoldOut.penetrationDepth = edgePenetration;
				manifoldOut.pointsNr = 1;
				manifoldOut.normal = edgeCollisionNormal;

				vec3 capsuleV = capsule->getV();
				float v1p = dot(edgeBox, pointBoxPointCapsuleVec);
				float v2p = dot(capsuleV, pointBoxPointCapsuleVec);
				float v1v2 = dot(edgeBox, capsuleV);
				float v1v1 = dot(edgeBox, edgeBox);
				float v2v2 = dot(capsuleV, capsuleV);
				float denominator = v1v1*v2v2 - v1v2*v1v2;
				float factorThis = fmin(fmax(v1p*v2v2 - v1v2*v2p, 0.0f) / denominator, 2.0f*s[minEdgePenetrationAxIdx]);
				float factorOther = fmin(fmax(v1v2*v1p - v2p*v1v1, 0.0f) / denominator, 1.0f);
				manifoldOut.points[0] = 0.5f*(pointBox + factorThis*edgeBox + pointCapsule + factorOther*capsuleV);
			}		
		}	

		return true;
	}

	// TODO: calculate collisionPointOut
	bool CollisionSphere::testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) {
		vec3 rayOriginCVec = c - segOrigin;
		vec3 rayDirection = segDest - segOrigin;
		segFactorOnCollisionOut = fmax(dot(rayOriginCVec, rayDirection) / length2(rayDirection), 0.0f);
		if (length2(rayOriginCVec - segFactorOnCollisionOut * rayDirection) <= r * r)// || pcLen2 < EPSILON_ZERO_SQRD)		
			return true;	
		else
			return false;
	}

	bool CollisionSphere::testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) {
	
		return false;
	}

	CollisionSphere::CollisionSphere(glm::vec3 const& center, float radius) : c(center), r(radius) {}

	CollisionVolume* CollisionSphere::clone(CollisionPrimitivesFactory& collisionPrimitivesFactory) const {
		return collisionPrimitivesFactory.genCollisionSphere(c, r);
	}

	void CollisionSphere::destroy(CollisionPrimitivesFactory& CollisionVolumesFactory) {
		CollisionVolumesFactory.destroyCollisionSphere(this);
	}

	bool CollisionSphere::testCollision(CollisionSphere* other, ContactManifold& manifoldOut) {
		vec3 centersVec = other->c - c;
		float centersVecLen2 = length2(centersVec);
		if (centersVecLen2 > (r + other->r)*(r + other->r)) // || centersVecLen2 < EPSILON_ZERO_SQRD
			return false;
		else {
			float centersVecLen = sqrt(centersVecLen2);		
			manifoldOut.normal = centersVec / centersVecLen;
			manifoldOut.penetrationDepth = centersVecLen - r - other->r;
			manifoldOut.pointsNr = 1;
			manifoldOut.points[0] = c + (r + 0.5f*manifoldOut.penetrationDepth)*manifoldOut.normal;	

			return true;
		}
	}

	// [segment origin -> c1, segment direction -> v, segment factor -> t]
	// looking for p = (c1 + tv) such that pc * v = 0, and p is clamped to the segment [c, c+v] 
	// (same as t clamped to the range [0.0, 1.0f]):
	// c1c*v - t|v|^2 = 0
	// t = c1c*v / |v|^2
	inline void segmentFactorOfClosestPointToPoint(vec3 const& segDirection, vec3 const& segOriginPointVec, float& segFactorOut) {	
		segFactorOut = fmax(fmin(dot(segOriginPointVec, segDirection) / length2(segDirection), 1.0f), 0.0f);
	}

	bool CollisionSphere::testCollision(CollisionCapsule* capsule) {
		if (lastCollisionOtherVolume != capsule) {
			lastCollisionOtherVolume = capsule;
			//capsule->lastCollisionOtherVolume = this;
		}

		vec3 capsuleC1 = capsule->getC1();
		vec3 capsuleV = capsule->getV();	
		float t;
		segmentFactorOfClosestPointToPoint(capsuleV, c - capsuleC1, t);	
		float pcLen2 = length2(c - capsuleC1 - t * capsuleV);
		float capsuleR = capsule->getR();
		if (pcLen2 > (r + capsuleR)*(r + capsuleR)) // || pcLen2 < EPSILON_ZERO_SQRD
			return false;
		else
			return true;
	}

	bool CollisionSphere::testCollision(CollisionCapsule* capsule, ContactManifold& manifoldOut) {
		if (lastCollisionOtherVolume != capsule) {
			lastCollisionOtherVolume = capsule;
			//capsule->lastCollisionOtherVolume = this;
		}

		vec3 capsuleC1 = capsule->getC1();
		vec3 capsuleV = capsule->getV();
		float capsuleR = capsule->getR();
		float t;
		segmentFactorOfClosestPointToPoint(capsuleV, c - capsuleC1, t);
		vec3 pc = c - capsuleC1 - t*capsuleV;
		float pcLen2 = length2(pc);
		if (pcLen2 > (r + capsuleR)*(r + capsuleR)) //  || pcLen2 < EPSILON_ZERO_SQRD
			return false;
		else {
			float pcLen = sqrt(pcLen2);
			manifoldOut.normal = -pc / pcLen;
			manifoldOut.penetrationDepth = pcLen - r - capsuleR;
			manifoldOut.pointsNr = 1;
			manifoldOut.points[0] = c + (r + 0.5f*manifoldOut.penetrationDepth)*manifoldOut.normal;
		
			return true;
		}	
	}

	inline float getClampedRoot(float slope, float p0, float p1) {
		if (p0 < 0.0f) {
			if (p1 > 0.0f) {
				float r = -p0 / slope;
				if (r > 1.0f)
					return 0.5f;
				else
					return r;
			}
			else
				return 1.0f;
		}
		else
			return 0.0f;
	}

	inline float getClampedRoot(float slope, float p0) {
		if (p0 < 0.0f)		
			return -p0 / slope;			
		else
			return 0.0f;
	}

	enum Edge { left, right, bottom, top };

	bool CollisionCapsule::testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest, float& segFactorOnCollisionOut) {
		vec3 rayDirection = segDest - segOrigin;
		vec3 thisC1RayOrigin = segOrigin - c1;
		float a = dot(v, v); // v1v1
		float b = dot(v, rayDirection); // v1v2		
		float c = dot(rayDirection, rayDirection); // v2v2	
		float d = dot(v, thisC1RayOrigin); // v1p
		float e = dot(rayDirection, thisC1RayOrigin); // v2p

		// R(s,t) = |thisVLine(s) - otherVLine(t)| = as^2 - 2bst + ct^2 - 2ds + 2et + f
		// where: s -> factorThis, t -> factorRay. Looking to minimize R(s,t).
		// F(s,t) = dR/ds = as - bt - d, G(s,t) = dR/dt = -bs + ct + e
		float F00 = -d;
		float F10 = F00 + a;
		float G00 = e;
		float G10 = G00 - b;

		// t0, t1 -> the solutions of dR/dt(0,t) = 0 and dR/dt(1,t) = 0	
		float t0 = getClampedRoot(c, G00);
		float t1 = getClampedRoot(c, G10);
		float factorThis;
		if (t0 <= 0.0f && t1 <= 0.0f) {
			// The minimum must occur on t = 0 for 0 <= s <= 1.
			segFactorOnCollisionOut = 0.0f;
			factorThis = getClampedRoot(a, F00, F10);
		}
		else {
			// e0, e1 -> the intersection points of dR/dt(s,t) = 0 with [0,1] X [0,inf)
			vec2 e0, e1;
			Edge edge0, edge1;
			if (t0 <= 0.0f) {
				edge0 = Edge::bottom;
				e0.t = 0.0f;
				e0.s = G00 / b;
				if (e0.s < 0.0f || e0.s > 1.0f)
					e0.s = 0.5f;

				edge1 = Edge::right;
				e1.t = t1;
				e1.s = 1.0f;
			}
			else if (t1 <= 0.0f) {
				edge0 = Edge::left;
				e0.t = t0;
				e0.s = 0.0f;

				edge1 = Edge::bottom;
				e1.t = 0.0f;
				e1.s = G00 / b;
				if (e1.s < 0.0f || e1.s > 1.0f)
					e1.s = 0.5f;
			}
			else { // (0.0f < t0) && (0.0f < t1)
				edge0 = Edge::left;
				e0.t = t0;
				e0.s = 0.0f;

				edge1 = Edge::right;
				e1.t = t1;
				e1.s = 1.0f;
			}

			// The directional derivative of R along the segment of intersection is:
			//   H(z) = (e1.s - e0.s)*dR/ds((1-z)*e0 + z*e1), for z in [0,1].  		
			float delta = e1.s - e0.s;
			float h0 = delta * (a * e0.s - b * e0.t - d);
			float h1 = delta * (a * e1.s - b * e1.t - d);
			if (h0 * h1 > -EPSILON_ZERO) {
				if (h0 > -EPSILON_ZERO) {
					factorThis = e0.s;
					segFactorOnCollisionOut = e0.t;
				}
				else {
					factorThis = e1.s;
					segFactorOnCollisionOut = e1.t;
				}
			}
			else {
				float z = fmin(fmax(h0 / (h0 - h1), 0.0f), 1.0f);
				float omz = 1.0f - z;
				factorThis = omz * e0.s + z * e1.s;
				segFactorOnCollisionOut = omz * e0.t + z * e1.t;
			}
		}

		vec3 thisPoint = c1 + factorThis * v;
		vec3 otherPoint = segOrigin + segFactorOnCollisionOut*rayDirection;
		vec3 thisPOtherPVec = otherPoint - thisPoint;
		if (length2(thisPOtherPVec) <= r * r)
			return true;
		else
			return false;
	}

	//TODO: <<fill this>>
	bool CollisionCapsule::testSegCollision(glm::vec3 const& segOrigin, glm::vec3 const& segDest) {
	
		return false;
	}

	CollisionCapsule::CollisionCapsule(glm::vec3 const& center1, glm::vec3 const& axisVec, float radius) : c1(center1), v(axisVec), r(radius) {}

	CollisionVolume* CollisionCapsule::clone(CollisionPrimitivesFactory& collisionPrimitivesFactory) const {
		return collisionPrimitivesFactory.genCollisionCapsule(c1, v, r);
	}

	void CollisionCapsule::destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) {
		collisionPrimitivesFactory.destroyCollisionCapsule(this);
	}

	inline void segmentSegmentClosestPointsFactors(vec3 const& seg1Direction, vec3 const& seg2Direction, vec3 const& seg1OriginSeg2OriginVec, float& seg1FactorOut, float& seg2FactorOut) {
		float a = dot(seg1Direction, seg1Direction); // v1v1
		float b = dot(seg1Direction, seg2Direction); // v1v2		
		float c = dot(seg2Direction, seg2Direction); // v2v2	
		float d = dot(seg1Direction, seg1OriginSeg2OriginVec); // v1p
		float e = dot(seg2Direction, seg1OriginSeg2OriginVec); // v2p
		// float f = dot(thisC1OtherC1Vec, thisC1OtherC1Vec); // pp
		// float denominator = a*c - b*b;

		// R(s,t) = |thisVLine(s) - otherVLine(t)| = as^2 - 2bst + ct^2 - 2ds + 2et + f
		// where: s -> factorThis, t -> factorOther. Looking to minimize R(s,t).
		// F(s,t) = dR/ds = as - bt - d, G(s,t) = dR/dt = -bs + ct + e
		float F00 = -d;
		float F10 = F00 + a;
		float F01 = F00 - b;
		float F11 = F10 - b;
		float G00 = e;
		float G10 = G00 - b;
		float G01 = G00 + c;
		float G11 = G10 + c;

		// s0, s1 -> the solutions of dR/ds(s,0) = 0 and dR/ds(s,1) = 0		
		float s0 = getClampedRoot(a, F00, F10);
		float s1 = getClampedRoot(a, F01, F11);
	
		if (s0 <= 0.0f && s1 <= 0.0f) {
			// The minimum must occur on s = 0 for 0 <= t <= 1.
			seg1FactorOut = 0.0f;
			seg2FactorOut = getClampedRoot(c, G00, G01);
		}
		else if (1.0f <= s0 && 1.0f <= s1) {
			// The minimum must occur on s = 1 for 0 <= t <= 1.
			seg1FactorOut = 1.0f;
			seg2FactorOut = getClampedRoot(c, G10, G11);
		}
		else {
			// e0, e1 -> the intersection points of dR/ds(s,t) = 0 with [0,1] X [0,1]
			vec2 e0, e1;
			Edge edge0, edge1;
			if (s0 <= 0.0f) {
				edge0 = Edge::left;
				e0.s = 0.0f;
				e0.t = F00 / b;
				if (e0.t < 0.0f || e0.t > 1.0f)
					e0.t = 0.5f;

				if (0.0f < s1 && s1 < 1.0f) {
					edge1 = Edge::top;
					e1.s = s1;
					e1.t = 1.0f;
				}
				else { // s1 >= 1.0f	
					edge1 = Edge::right;
					e1.s = 1.0f;
					e1.t = F10 / b;
					if (e1.t < 0.0f || e1.t > 1.0f)
						e1.t = 0.5f;
				}
			}
			else if (s0 < 1.0f) {
				edge0 = Edge::bottom;
				e0.s = s0;
				e0.t = 0.0f;
				if (s1 <= 0.0f) {
					edge1 = Edge::left;
					e1.s = 0.0f;
					e1.t = F00 / b;
					if (e1.t < 0.0f || e1.t > 1.0f)
						e1.t = 0.5f;
				}
				else if (s1 < 1.0f) {
					edge1 = Edge::top;
					e1.s = s1;
					e1.t = 1.0f;
				}
				else { // 1.0f <= s1	
					edge1 = Edge::right;
					e1.s = 1.0f;
					e1.t = F10 / b;
					if (e1.t < 0.0f || e1.t > 1.0f)
						e1.t = 0.5f;
				}
			}
			else { //  (1.0f < s0)			
				edge0 = Edge::right;
				e0.s = 1.0f;
				e0.t = F10 / b;
				if (e0.t < 0.0f || e0.t > 1.0f)
					e0.t = 0.5f;

				if (0.0f < s1 && s1 < 1.0f) {
					edge1 = Edge::top;
					e1.s = s1;
					e1.t = 1.0f;
				}
				else { // s1 < 0.0f
					edge1 = Edge::left;
					e1.s = 0.0f;
					e1.t = F00 / b;
					if (e1.t < 0.0f || e1.t > 1.0f)
						e1.t = 0.5f;

				}
			}

			// The directional derivative of R along the segment of intersection is:
			//   H(z) = (e1.t - e0.t)*dR/dt((1-z)*e0 + z*e1) for z in [0,1].  
			// The formula uses the fact that dR/ds = 0 on the segment.  
			// Compute the minimum of H on [0,1].
			float delta = e1.t - e0.t;
			float h0 = delta * (-b * e0.s + c * e0.t + e);
			if (h0 >= 0.0f) {
				if (edge0 == Edge::left) {
					seg1FactorOut = 0.0f;
					seg2FactorOut = getClampedRoot(c, G00, G01);
				}
				else if (edge0 == Edge::right) {
					seg1FactorOut = 1.0f;
					seg2FactorOut = getClampedRoot(c, G10, G11);
				}
				else {
					seg1FactorOut = e0.s;
					seg2FactorOut = e0.t;
				}
			}
			else {
				float h1 = delta * (-b * e1.s + c * e1.t + e);
				if (h1 <= 0.0f) {
					if (edge1 == Edge::left) {
						seg1FactorOut = 0.0f;
						seg2FactorOut = getClampedRoot(c, G00, G01);
					}
					else if (edge1 == Edge::right) {
						seg1FactorOut = 1.0f;
						seg2FactorOut = getClampedRoot(c, G10, G11);
					}
					else {
						seg1FactorOut = e1.s;
						seg2FactorOut = e1.t;
					}
				}
				else { // h0 < 0 and h1 > 0				
					float z = fmin(fmax(h0 / (h0 - h1), 0.0f), 1.0f);
					float omz = 1.0f - z;
					seg1FactorOut = omz * e0.s + z * e1.s;
					seg2FactorOut = omz * e0.t + z * e1.t;
				}
			}
		}	
	}

	bool CollisionCapsule::testCollision(CollisionCapsule* other) {
		vec3 thisC1OtherC1Vec = other->c1 - c1;
		vec3 otherV = other->getV();
		if (areVecsParallel(v, otherV)) {
			// TODO: Optimize this algo according to CollisionStadium
			vec3 thisVOtherVVec = thisC1OtherC1Vec - (dot(thisC1OtherC1Vec, v) / length2(v))*v;
			if (dot(c1 + thisVOtherVVec - other->c1 - otherV, otherV) > 0.0f) {
				if (length2(other->c1 + otherV - c1) <= (r + other->r)*(r + other->r))
					return true;			
				else
					return false;
			}
			else if (dot(c1 + v + thisVOtherVVec - other->c1, other->v) < 0.0f) {
				vec3 thisC2OtherC1Vec = other->c1 - c1 - v;
				if (length2(thisC2OtherC1Vec) <= (r + other->r)*(r + other->r))
					return true;		
				else
					return false;
			}
			else {
				float thisVOtherVVecLen = length(thisVOtherVVec);
				if (thisVOtherVVecLen <= r + other->r)
					return true;			
				else
					return false;
			}
		}
		else {
			float factorThis, factorOther;
			segmentSegmentClosestPointsFactors(v, otherV, thisC1OtherC1Vec, factorThis, factorOther);		
			if (length2(other->c1 + factorOther*otherV - (c1 + factorThis*v)) <= (r + other->r) * (r + other->r))
				return true;		
			else
				return false;
		}
	}

	bool CollisionCapsule::testCollision(CollisionCapsule* other, ContactManifold& manifoldOut) {	
		vec3 thisC1OtherC1Vec = other->c1 - c1;
		vec3 otherV = other->getV();
		if (areVecsParallel(v, otherV)) {
			// TODO: Optimize this algo according to CollisionStadium
			vec3 thisVOtherVVec = thisC1OtherC1Vec - (dot(thisC1OtherC1Vec, v)/length2(v))*v;							
			vec3 c1ClipPoint = c1 + thisVOtherVVec;
			vec3 c2ClipPoint = c1 + v + thisVOtherVVec;
			if (dot(c1ClipPoint - other->c1 - otherV, otherV) > 0.0f) {
				vec3 thisC1OtherC2Vec = other->c1 + otherV - c1;
				if (length2(thisC1OtherC2Vec) <= (r + other->r)*(r + other->r)) {
					float centersVecLen = length(thisC1OtherC2Vec);
					//if (centersVecLen > EPSILON_ZERO)
						manifoldOut.normal = thisC1OtherC2Vec / centersVecLen;
					//else
					//	manifoldOut.normal = -v / length(v);
					manifoldOut.penetrationDepth = centersVecLen - r - other->r;
					manifoldOut.pointsNr = 1;
					manifoldOut.points[0] = c1 + (r + 0.5f*manifoldOut.penetrationDepth)*manifoldOut.normal;
				}
				else
					return false;
			}
			else if (dot(c2ClipPoint - other->c1, other->v) < 0.0f) {
				vec3 thisC2OtherC1Vec = other->c1 - c1 - v;
				if (length2(thisC2OtherC1Vec) <= (r + other->r)*(r + other->r)) {
					float centersVecLen = length(thisC2OtherC1Vec);				
					//if (centersVecLen > EPSILON_ZERO)
						manifoldOut.normal = thisC2OtherC1Vec / centersVecLen;
					//else
					//	manifoldOut.normal = -v / length(v);				
					manifoldOut.penetrationDepth = centersVecLen - r - other->r;
					manifoldOut.pointsNr = 1;
					manifoldOut.points[0] = c1 + v + (r + 0.5f*manifoldOut.penetrationDepth)*manifoldOut.normal;
				}
				else
					return false;
			}
			else {			
				float thisVOtherVVecLen = length(thisVOtherVVec);
				if (thisVOtherVVecLen <= r + other->r) {
					if (thisVOtherVVecLen > EPSILON_ZERO)					
						manifoldOut.normal = thisVOtherVVec / thisVOtherVVecLen;									
					else if (dot(0.5f*(c1ClipPoint + c2ClipPoint - 2.0f*other->c1 - other->v), v) > 0.0f)
						manifoldOut.normal = -normalize(v);				
					else 
						manifoldOut.normal = normalize(v);
				
					manifoldOut.penetrationDepth = thisVOtherVVecLen - r - other->r;
					manifoldOut.pointsNr = 2;
					manifoldOut.points[0] = dot(c1ClipPoint - other->c1, other->v) > 0.0f ? c1  + 0.5f*thisVOtherVVec: other->c1 - 0.5f*thisVOtherVVec;
					manifoldOut.points[1] = dot(c2ClipPoint - other->c1 - other->v, other->v) > 0.0f ? other->c1 + otherV - 0.5f*thisVOtherVVec : c1 + v + 0.5f*thisVOtherVVec;
				}
				else
					return false;
			}
		}
		else {
			float factorThis, factorOther;
			segmentSegmentClosestPointsFactors(v, otherV, thisC1OtherC1Vec, factorThis, factorOther);
			vec3 thisPoint = c1 + factorThis*v;
			vec3 otherPoint = other->c1 + factorOther*otherV;
			vec3 thisPOtherPVec = otherPoint - thisPoint;
			if (length2(thisPOtherPVec) <= (r + other->r) * (r + other->r)) {
				float thisPOtherPVecLen = length(thisPOtherPVec);
				manifoldOut.normal = thisPOtherPVec / thisPOtherPVecLen;
				manifoldOut.penetrationDepth = thisPOtherPVecLen - r - other->r;
				manifoldOut.pointsNr = 1;
				manifoldOut.points[0] = 0.5f*(thisPoint + otherPoint);
			}
			else
				return false;
		}
		
		return true;	
	}

	/*
	bool CollisionCone::testRayCollision(vec3 const& rayOrigin, vec3 const& rayDirection, vec3& collisionPointOut) {
		return false;
	}

	bool CollisionCylinder::testRayCollision(vec3 const& rayOrigin, vec3 const& rayDirection, vec3& collisionPointOut) {
		return false;
	}
	*/

	vec2 CollisionPerimeter::GjkJohnsonsDistanceIterator2D::iterate(vec2 const& aAdded, vec2 const& bAdded) {
		vec2 wAdded = aAdded - bAdded;
		vec2 d[3];

		// testing X = {wAdded}	
		bool wasSubsetFound = true;
		bool wasIwFound = false;
		unsigned int wAddedIw = 3;
		for (unsigned int wIdx = 0; wIdx < W_sz + 1; wIdx++) {
			if (!wasIwFound && !(b & 1 << wIdx)) {
				W[wIdx] = wAdded; A[wIdx] = aAdded; B[wIdx] = bAdded;
				wAddedIw = wIdx;
				wasIwFound = true;
			}

			unsigned int iw = Iw[wIdx];
			if (iw != wAddedIw) {
				d[iw] = W[iw] - wAdded;
				if (wasSubsetFound && dot(-d[iw], wAdded) > 0)
					wasSubsetFound = false;
			}
		}
		if (wasSubsetFound) {
			ServiceLocator::getLogger().logd("GJK", string("EARLY SUCCESS: |W| = 0").c_str());
			Y_sz = W_sz;
			memcpy(Iy, Iw, Y_sz * sizeof(unsigned int));
			Iw[0] = wAddedIw;
			b = 1 << wAddedIw;
			W_sz = 1;
			WNormSqrdMax = WNormsSqrd[wAddedIw] = length2(wAdded);
			return wAdded;
		}

		// testing X = W U {wAdded} where |W| == 1
		for (unsigned int IwIdx = 0; IwIdx < W_sz; IwIdx++) {
			unsigned int iw = Iw[IwIdx];
			vec2 x = W[iw];
			DiX[wAddedIw] = dot(d[iw], x);
			if (1 < W_sz && DiX[wAddedIw] < EPSILON_ZERO) {
				ServiceLocator::getLogger().logd("GJK", (to_string(iw) + string(" [|W| = 1] FAILURE: DiX[wAddedIw] = ") + to_string(DiX[wAddedIw])).c_str());
				continue;
			}
			DiX[iw] = -dot(d[iw], wAdded);
			if (1 < W_sz && DiX[iw] < EPSILON_ZERO) {
				ServiceLocator::getLogger().logd("GJK", (to_string(iw) + string(" [|W| = 1] FAILURE: DiX[") + to_string(iw) + string("] = ") + to_string(DiX[iw])).c_str());
				continue;
			}

			bool wasSubsetFound = true;
			Y_sz = 0;
			for (unsigned int X_sz_2_Jx_idx = 0; X_sz_2_Jx_idx < W_sz - 1; X_sz_2_Jx_idx++) {
				unsigned int jw = Iw[X_SZ_2_Jx[IwIdx][X_sz_2_Jx_idx]];
				vec2 dMinus = -d[jw];
				float s = DiX[wAddedIw] * dot(dMinus, wAdded) + DiX[iw] * dot(dMinus, x);
				if (s > EPSILON_ZERO) {
					ServiceLocator::getLogger().logd("GJK", (to_string(iw) + string(" [|W| = 1] FAILURE: sigma = ") + to_string(s)).c_str());
					wasSubsetFound = false;
					break;
				}
				else
					Iy[Y_sz++] = jw;
			}

			if (wasSubsetFound) {
				vec2 v = (DiX[wAddedIw] * wAdded + DiX[iw] * x) / (DiX[wAddedIw] + DiX[iw]);
				ServiceLocator::getLogger().logd("GJK", (to_string(iw) + string(" [|W| = 1] SUCCESS: v = ") + to_string(v)).c_str());
				Iw[0] = wAddedIw; Iw[1] = iw;
				W_sz = 2;
				b = (1 << wAddedIw) | (1 << iw);
				WNormsSqrd[wAddedIw] = length2(wAdded);
				WNormSqrdMax = fmaxf(WNormsSqrd[wAddedIw], WNormsSqrd[iw]);
				return (DiX[wAddedIw] * wAdded + DiX[iw] * x) / (DiX[wAddedIw] + DiX[iw]);
			}
		}	

		//W[2] = wAdded;
		W_sz = 3;
		//WNormsSqrd[2] = length2(wAdded);
		//WNormSqrdMax = fmaxf(WNormSqrdMax, WNormsSqrd[2]);
		ServiceLocator::getLogger().logd("GJK", string("FAILURE. W:").c_str());
		for (unsigned int wIdx = 0; wIdx < 2; wIdx++) {
			ServiceLocator::getLogger().logd("GJK", (string("W[") + to_string(Iw[wIdx]) + string("] =") + to_string(W[Iw[wIdx]])).c_str());
		}
		ServiceLocator::getLogger().logd("GJK", (string("W[added] =") + to_string(wAdded)).c_str());

		return vec2(0.0f, 0.0f);
	}

	glm::vec2 CollisionRect::supportMap(glm::vec2 const& vec) const {
		vec2 ex = r[0] * s.x, ey = r[1] * s.y;
		return c + vec2(dot(vec, ex) < 0.0f ? -ex : ex) + vec2(dot(vec, ey) < 0.0f ? -ey : ey);			
	}

	glm::vec2 CollisionRect::getArbitraryV() const {
		return c + r[0]*s.x + r[1]*s.y;
	}

	inline unsigned char calcOutCode(glm::vec2 const& vertex, glm::vec2 const& rectMin, glm::vec2 const& rectMax) {
		return ((vertex.x < rectMin.x)) | ((rectMax.x < vertex.x) << 1) |
			   ((vertex.y < rectMin.y) << 2) | ((rectMax.y < vertex.y) << 3);
	}

	bool CollisionRect::testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest, float& segFactorOnCollisionOut) {
		mat2 rTransposed = transpose(r);
		vec2 segOriginRel = rTransposed * (segOrigin - c);
		vec2 segDestRel = rTransposed * (segDest - c);
		unsigned char segOriginOutCode = calcOutCode(segOriginRel, -s, s);
		unsigned char segDestOutCode = calcOutCode(segDestRel, -s, s);

		if (segOriginOutCode & segDestOutCode)
			return false;

		float segEnterFactorMax = 0.0f;
		float segExitFactorMin = 1.0f;
		if (segOriginOutCode & 0x01)
			segEnterFactorMax = fmax(segEnterFactorMax, (-s.x - segOriginRel.x) / (segDestRel.x - segOriginRel.x));
		else if (segDestOutCode & 0x01)
			segExitFactorMin = fmin(segExitFactorMin, (-s.x - segOriginRel.x) / (segDestRel.x - segOriginRel.x));

		if (segOriginOutCode & 0x02)
			segEnterFactorMax = fmax(segEnterFactorMax, (s.x - segOriginRel.x) / (segDestRel.x - segOriginRel.x));
		else if (segDestOutCode & 0x02)
			segExitFactorMin = fmin(segExitFactorMin, (s.x - segOriginRel.x) / (segDestRel.x - segOriginRel.x));

		if (segOriginOutCode & 0x04)
			segEnterFactorMax = fmax(segEnterFactorMax, (-s.y - segOriginRel.y) / (segDestRel.y - segOriginRel.y));
		else if (segDestOutCode & 0x04)
			segExitFactorMin = fmin(segExitFactorMin, (-s.y - segOriginRel.y) / (segDestRel.y - segOriginRel.y));

		if (segOriginOutCode & 0x08)
			segEnterFactorMax = fmax(segEnterFactorMax, (s.y - segOriginRel.y) / (segDestRel.y - segOriginRel.y));
		else if (segDestOutCode & 0x08)
			segExitFactorMin = fmin(segExitFactorMin, (s.y - segOriginRel.y) / (segDestRel.y - segOriginRel.y));	

		if (segEnterFactorMax <= segExitFactorMin) {
			segFactorOnCollisionOut = segEnterFactorMax;
			return true;
		}
		else
			return false;
	}

	bool CollisionRect::testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest) {
		mat2 rTransposed = transpose(r);
		vec2 segOriginRel = rTransposed * (segOrigin - c);
		vec2 segDestRel = rTransposed * (segDest - c);
		for (unsigned int axIdx = 0; axIdx < 2; axIdx++) {
			float originProj = segOriginRel[axIdx];
			float destProj = segDestRel[axIdx];
			if (abs(originProj + destProj) > abs(originProj - destProj) + 2 * s[axIdx])
				return false;
		}	

		return true;
	}

	CollisionRect::CollisionRect(glm::vec2 const& center, glm::vec2 extent) :
		c(center), r{ {1.0f, 0.0f}, {0.0f, 1.0f} }, s{ abs(extent.x), abs(extent.y) } {}

	CollisionPerimeter* CollisionRect::clone(CollisionPrimitivesFactory& collisionPrimitivesFactory) const {
		return collisionPrimitivesFactory.genCollisionRect(c, s);
	}

	void CollisionRect::destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) {
		collisionPrimitivesFactory.destroyCollisionRect(this);
	}

	inline float pointLineDist(vec2 const& point, vec2 const& lineNormal, vec2 const& linePoint) {
		return dot(lineNormal, point - linePoint);
	}

	inline float pointLineDist(vec2 const& point, vec2 const& lineNormal, float lineC) {
		return dot(lineNormal, point) + lineC;
	}

	inline void clipStadiumVecWithRectSide(vec2* clipPoints, vec2 const& sideN, float sideC) {
		float c1DistFromSide = pointLineDist(clipPoints[0], sideN, sideC);
		float c2DistFromSide = pointLineDist(clipPoints[1], sideN, sideC);
		if (c1DistFromSide > 0.0f)
			clipPoints[0] = clipPoints[0] + (c1DistFromSide / (c1DistFromSide - c2DistFromSide)) * (clipPoints[1] - clipPoints[0]);
		else if (c2DistFromSide > 0.0f)
			clipPoints[1] = clipPoints[1] + (c2DistFromSide / (c2DistFromSide - c1DistFromSide)) * (clipPoints[0] - clipPoints[1]);
	}

	inline void CollisionRect::clipStadiumVec(CollisionStadium const* stadium, unsigned int sideAxIdx, glm::vec2 const& sideNormal, glm::vec2* clipPointsOut) {
		clipPointsOut[0] = stadium->getC1();
		clipPointsOut[1] = stadium->getC1() + stadium->getV();
		vec2 stadiumVNormalized = stadium->getV() / length(stadium->getV());
			
		vec2 n = r[1 - sideAxIdx]; // clip side normal
		float d = -dot(n, c + s*n); // clip plane d					
		clipStadiumVecWithRectSide(clipPointsOut, n, d);
		clipStadiumVecWithRectSide(clipPointsOut, -n, d + 2.0f*dot(c, n));
	
		// project the clip points onto the reference side
		vec2 referenceSidePoint = c + sideNormal*s[sideAxIdx];
		for (unsigned int pointIdx = 0; pointIdx < 2; pointIdx++)
			clipPointsOut[pointIdx] = clipPointsOut[pointIdx] - pointLineDist(clipPointsOut[pointIdx], sideNormal, referenceSidePoint)*sideNormal;

	}

	inline bool areVecsParallel(vec2 const& pointA, vec2 const& pointB) {
		return abs(pointA.x * pointB.y - pointA.y * pointB.x) <= EPSILON_ZERO;
	}

	bool CollisionRect::testCollision(CollisionRect* other) {
		GjkJohnsonsDistanceIterator2D johnsonDistIt;
		return gjkIntersectionTest(*other, johnsonDistIt);
	}

	bool CollisionRect::testCollision(CollisionRect* other, ContactManifold& manifoldOut) {
		SidePenetrationDesc sidesPenetrationDesc;

		manifoldOut.pointsNr = 0;
		if (lastCollisionOtherVolume == other && other->lastCollisionOtherVolume == this) {
			if (!wasLastFrameSeparated) {

			}

			// test separation				
			if (didOwnLastSeparationAx) {
				if (testSidesSeparation(*other, sidesPenetrationDesc))
					return false;
			}
			else {
				if (other->testSidesSeparation(*this, sidesPenetrationDesc))
					return false;
				else {
					std::swap(sidesPenetrationDesc.penetrationMinAxIdxThis, sidesPenetrationDesc.penetrationMinAxIdxOther);
					std::swap(sidesPenetrationDesc.penetrationThis, sidesPenetrationDesc.penetrationOther);
				}
			}
		}
		else if (testSidesSeparation(*other, sidesPenetrationDesc) || other->testSidesSeparation(*this, sidesPenetrationDesc))
			return false;

		// Couldn't find A separating axis => rects are colliding	
		wasLastFrameSeparated = other->wasLastFrameSeparated = false;	
		// calculate the contact manifold
		if (sidesPenetrationDesc.penetrationThis >= sidesPenetrationDesc.penetrationOther)						
			createSidesContact(*other, sidesPenetrationDesc.penetrationMinAxIdxThis, sidesPenetrationDesc.penetrationThis, manifoldOut);
		else
			other->createSidesContact(*this, sidesPenetrationDesc.penetrationMinAxIdxOther, sidesPenetrationDesc.penetrationOther, manifoldOut);
	
		return true;
	}

	bool CollisionRect::testCollision(CollisionCircle* circle) {
		GjkJohnsonsDistanceIterator2D johnsonDistIt;
		if (gjkShallowPenetrationTest(*circle, circle->getR(), johnsonDistIt, NULL) == 0.0f)
			return false;
		else
			return true;
	}

	bool CollisionRect::testCollision(CollisionCircle* circle, ContactManifold& manifoldOut) {
		GjkJohnsonsDistanceIterator2D johnsonDistIt;
		GjkOut gjkOut;
		float penetrationDepth = gjkShallowPenetrationTest(*circle, circle->getR(), johnsonDistIt, &gjkOut);
		if (penetrationDepth == 0.0f) {
			manifoldOut.pointsNr = 0;
			return false;
		}
		else if (penetrationDepth > 0.0f) {
			// shallow penetration
			manifoldOut.pointsNr = 1;
			manifoldOut.normal = -normalize(gjkOut.vOut);
			manifoldOut.points[0] = gjkOut.closestPointThis;
			manifoldOut.penetrationDepth = -penetrationDepth;
		}
		else {
			// deep penetration
			vec2 circleCRectCVec = c - circle->getC();
			float projR0 = dot(circleCRectCVec, r[0]);
			float projR1 = dot(circleCRectCVec, r[1]);		

			vec2 r0, r1;
			if (projR0 < 0) {
				r0 = r[0];
				projR0 = -projR0;
			}
			else
				r0 = -r[0];
			if (projR1 < 0) {
				r1 = r[1];
				projR1 = -projR1;
			}
			else
				r1 = -r[1];		

			float penetrationR0 = projR0 - s[0];
			float penetrationR1 = projR1 - s[1];		
			if (penetrationR0 >= penetrationR1) {			
				manifoldOut.penetrationDepth = penetrationR0;
				manifoldOut.normal = r0;			
			}
			else { // (penetrationR0 < penetrationR1)			
				manifoldOut.penetrationDepth = penetrationR1;
				manifoldOut.normal = r1;			
			}

			manifoldOut.pointsNr = 1;
			manifoldOut.points[0] = circle->getC() - manifoldOut.normal*manifoldOut.penetrationDepth;
		}

		return true;
	}

	bool CollisionRect::testCollision(CollisionStadium* stadium) {
		GjkJohnsonsDistanceIterator2D johnsonDistIt;
		if (gjkShallowPenetrationTest(*stadium, stadium->getR(), johnsonDistIt, NULL) == 0.0f)
			return false;
		else
			return true;
	}

	bool CollisionRect::testCollision(CollisionStadium* stadium, ContactManifold& manifoldOut) {
		GjkJohnsonsDistanceIterator2D johnsonDistIt;
		GjkOut gjkOut;
		float penetrationDepth = gjkShallowPenetrationTest(*stadium, stadium->getR(), johnsonDistIt, &gjkOut);
		//ServiceLocator::getLogger().logd("GJK", (string("penetration: ") + std::to_string(penetrationDepth)).c_str());
		if (penetrationDepth == 0.0f) {
			manifoldOut.pointsNr = 0;
			return false;
		}
		else if (penetrationDepth > 0.0f) {
			// shallow penetration		
			manifoldOut.normal = -normalize(gjkOut.vOut);
			manifoldOut.penetrationDepth = -penetrationDepth;
			unsigned int minPenetrationSideAxIdx = 2;
			for (unsigned int sideAxIdx = 0; sideAxIdx < 2; sideAxIdx++) {
				if (areVecsParallel(r[sideAxIdx], manifoldOut.normal)) {
					minPenetrationSideAxIdx = sideAxIdx;
					break;
				}
			}

			if (minPenetrationSideAxIdx == 2) {
				manifoldOut.pointsNr = 1;
				manifoldOut.points[0] = gjkOut.closestPointThis;
			}
			else {
				manifoldOut.pointsNr = 2;
				clipStadiumVec(stadium, minPenetrationSideAxIdx, manifoldOut.normal, manifoldOut.points);
			}
		}
		else {
			// deep penetration						
			mat2 rTransposed = transpose(r);

			float stadiumVLen = length(stadium->getV());
			vec2 stadiumAxesLens = { stadiumVLen / 2 + stadium->getR(), stadium->getR() };

			vec2 stadiumVRel = rTransposed * stadium->getV() / stadiumVLen;
			mat2 stadiumAxesRel = { stadiumVRel, {-stadiumVRel.y, stadiumVRel.x} };
		
			mat2 stadiumAxesRelP = { abs(stadiumAxesRel[0][0]) + EPSILON_ZERO, abs(stadiumAxesRel[0][1]) + EPSILON_ZERO,
									 abs(stadiumAxesRel[1][0]) + EPSILON_ZERO, abs(stadiumAxesRel[1][1]) + EPSILON_ZERO };

			vec2 rectCenterStadiumCenterVec = stadium->getC1() + 0.5f*stadium->getV() - c;
			vec2 stadiumCenterRel = rTransposed * rectCenterStadiumCenterVec;

			unsigned int minPenetrationSideAxIdx;
			float penetrationMax = -std::numeric_limits<float>::max();
			for (unsigned int axIdx = 0; axIdx < 2; axIdx++) {
				float penetration = abs(stadiumCenterRel[axIdx]) - (s[axIdx] + stadiumAxesLens[0]*stadiumAxesRelP[0][axIdx] + stadiumAxesLens[1]*stadiumAxesRelP[1][axIdx]);
				if (penetration > penetrationMax) {
					penetrationMax = penetration;
					minPenetrationSideAxIdx = axIdx;
				}
			}
							
			manifoldOut.penetrationDepth = penetrationMax;
			vec2 referenceSideNormal = r[minPenetrationSideAxIdx];
			if (dot(referenceSideNormal, rectCenterStadiumCenterVec) < 0)
				referenceSideNormal = -referenceSideNormal;
			manifoldOut.normal = referenceSideNormal;
			manifoldOut.pointsNr = 2;
			clipStadiumVec(stadium, minPenetrationSideAxIdx, manifoldOut.normal, manifoldOut.points);
		}

		return true;
	}

	bool CollisionRect::testSidesSeparation(CollisionRect& other, SidePenetrationDesc& penetrationDescOut) {
		mat2 thisRTransposed = transpose(r);
		mat2 otherRelR = thisRTransposed * other.r;
		mat2 otherRelRp = { abs(otherRelR[0][0]) + EPSILON_ZERO, abs(otherRelR[0][1]) + EPSILON_ZERO,
							abs(otherRelR[1][0]) + EPSILON_ZERO, abs(otherRelR[1][1]) + EPSILON_ZERO };
		vec2 otherRelC = thisRTransposed * (other.c - c);
	
		for (unsigned int axIdx = 0; axIdx < 2; axIdx++) {
			float separation = abs(otherRelC[axIdx]) - (s[axIdx] + other.s[0]*otherRelRp[0][axIdx] + other.s[1]*otherRelRp[1][axIdx]);				
			if (separation > 0.0f) {
				lastCollisionOtherVolume = &other;
				other.lastCollisionOtherVolume = this;
				didOwnLastSeparationAx = true;
				other.didOwnLastSeparationAx = false;
				return wasLastFrameSeparated = other.wasLastFrameSeparated = true;
			}
			else if (separation > penetrationDescOut.penetrationThis) {			
				penetrationDescOut.penetrationMinAxIdxThis = axIdx;			
				penetrationDescOut.penetrationThis = separation;
			}		
		}

		for (unsigned int axIdx = 0; axIdx < 2; axIdx++) {		
			float separation = abs(dot(otherRelC, otherRelR[axIdx])) - (other.s[axIdx] + dot(s, otherRelRp[axIdx]));
			if (separation > 0.0f) {
				lastCollisionOtherVolume = &other;
				other.lastCollisionOtherVolume = this;
				didOwnLastSeparationAx = false;
				other.didOwnLastSeparationAx = true;
				return wasLastFrameSeparated = other.wasLastFrameSeparated = true;
			}
			else if (separation > penetrationDescOut.penetrationOther) {
				penetrationDescOut.penetrationMinAxIdxOther = axIdx;
				penetrationDescOut.penetrationOther = separation;
			}
		}

		return false;
	}

	inline void clipLineByLine(vec2 const& clipLineNormal, float clipLineC, vec2* clippedLinePoints) {
		float point0ClipDist = pointLineDist(clippedLinePoints[0], clipLineNormal, clipLineC);
		float point1ClipDist = pointLineDist(clippedLinePoints[1], clipLineNormal, clipLineC);
		if (point0ClipDist > 0.0f)
			clippedLinePoints[0] = clippedLinePoints[1] - point1ClipDist / (point0ClipDist - point1ClipDist) * (clippedLinePoints[0] - clippedLinePoints[1]);
		else if (point1ClipDist > 0.0f)
			clippedLinePoints[1] = clippedLinePoints[0] - point0ClipDist / (point1ClipDist - point0ClipDist) * (clippedLinePoints[1] - clippedLinePoints[0]);
	}

	void CollisionRect::createSidesContact(CollisionRect& other, unsigned int penetrationMinAxIdx, float penetration, ContactManifold& manifoldOut) {
		manifoldOut.penetrationDepth = penetration;	

		// calculate the reference plane normal
		vec2 referenceSideNormal = r[penetrationMinAxIdx];
		if (dot(referenceSideNormal, c - other.c) > 0)
			referenceSideNormal = -referenceSideNormal;
		manifoldOut.normal = referenceSideNormal;

		// calculate the incident plane normal
		unsigned int incidentSideNormalIdx = abs(dot(other.r[0], referenceSideNormal)) >= abs(dot(other.r[1], referenceSideNormal)) ? 0 : 1;	
		vec2 incidentSideNormal = other.r[incidentSideNormalIdx];
		if (dot(incidentSideNormal, referenceSideNormal) > 0)
			incidentSideNormal = -incidentSideNormal;

		// clip the incident plane - Sutherland-Hodgeman algo	
		unsigned int incidentSidePerpVecIdx = 1 - incidentSideNormalIdx;
		vec2 contactPoints[2];
		vec2 incidentSidePerpVec = other.r[incidentSidePerpVecIdx]*other.s[incidentSidePerpVecIdx];		
		vec2 incidentSideCenter = other.c + other.s[incidentSideNormalIdx] * incidentSideNormal;	
		contactPoints[0] = incidentSideCenter + incidentSidePerpVec;
		contactPoints[1] = incidentSideCenter - incidentSidePerpVec;	
		vec2 clipSideNormal = r[1 - penetrationMinAxIdx];
		float clipSideC = -dot(clipSideNormal, c + s* clipSideNormal);
		clipLineByLine(clipSideNormal, clipSideC, contactPoints);
		clipLineByLine(-clipSideNormal, clipSideC + 2.0f*dot(c, clipSideNormal), contactPoints);

		// discard points above the reference plane and project the rest onto the reference plane
		vec2 referenceLinePoint = c + referenceSideNormal*s[penetrationMinAxIdx];
		for (unsigned int pointIdx = 0; pointIdx < 2; pointIdx++) {
			float pointDistFromReference = pointLineDist(contactPoints[pointIdx], referenceSideNormal, referenceLinePoint);
			if (pointDistFromReference < 0)
				manifoldOut.points[manifoldOut.pointsNr++] = contactPoints[pointIdx] - pointDistFromReference*referenceSideNormal;
		}

		// TODO: Experiment with this -> Contact points reduction
		// if (manifoldOut.pointsNr > 2) {}	
	}

	CollisionCircle::CollisionCircle(glm::vec2 const& center, float radius) : c(center), r(radius) {}

	bool CollisionCircle::testCollision(CollisionCircle* other, ContactManifold& manifoldOut) {
		vec2 centersVec = other->c - c;
		float centersVecLen2 = length2(centersVec);
		if (centersVecLen2 > (r + other->r) * (r + other->r)) // || centersVecLen2 < EPSILON_ZERO_SQRD
			return false;
		else {
			float centersVecLen = sqrt(centersVecLen2);
			manifoldOut.normal = centersVec / centersVecLen;
			manifoldOut.penetrationDepth = centersVecLen - r - other->r;
			manifoldOut.pointsNr = 1;
			manifoldOut.points[0] = c + (r + 0.5f*manifoldOut.penetrationDepth)*manifoldOut.normal;

			return true;
		}
	}

	bool CollisionCircle::testCollision(CollisionStadium* stadium) {			
		glm::vec2 w = c - stadium->getC1();
		glm::vec2 stadiumV = stadium->getV();
		float wDotV = glm::dot(w, stadiumV);
		float radiiSum = r + stadium->getR();
		float vDotV;
		if (wDotV <= 0.0f)
			return (glm::length2(w) <= radiiSum * radiiSum);	
		else if (wDotV < (vDotV = glm::dot(stadiumV, stadiumV))) {
			float stadiumVPerpDotC = stadiumV.x * c.y - stadiumV.y * c.x;
			return ((stadiumVPerpDotC * stadiumVPerpDotC) / vDotV <= radiiSum * radiiSum);			
		}
		else
			return (glm::distance2(c, stadium->getC1() + stadiumV) <= radiiSum * radiiSum);
	}

	bool CollisionCircle::testCollision(CollisionStadium* stadium, ContactManifold& manifoldOut) {
		glm::vec2 stadiumC1 = stadium->getC1();
		glm::vec2 stadiumV = stadium->getV();
		float stadiumR = stadium->getR();
		glm::vec2 w = c - stadiumC1;
		float wDotV = glm::dot(w, stadiumV);
		float radiiSum = r + stadiumR;
		float vDotV;
		if (wDotV <= 0.0f) {
			if (glm::length2(w) <= radiiSum * radiiSum) {
				float wLen = glm::length(w);
				manifoldOut.normal = w / wLen;
				manifoldOut.penetrationDepth = radiiSum - wLen;
				manifoldOut.points[0] = (stadiumR*c + r*stadiumC1) / radiiSum;
				manifoldOut.pointsNr = 1;
				return true;
			}
			else
				return false;
		}
		else if (wDotV < (vDotV = glm::dot(stadiumV, stadiumV))) {
			float stadiumVPerpDotC = stadiumV.x*c.y - stadiumV.y*c.x;
			if ((stadiumVPerpDotC * stadiumVPerpDotC) / vDotV <= radiiSum * radiiSum) {
				glm::vec2 normalVec = w - glm::dot(w, stadiumV) / glm::length2(stadiumV) * stadiumV;
				float normalVecLen = glm::length(normalVec);
				manifoldOut.normal = normalVec / normalVecLen;
				manifoldOut.penetrationDepth = radiiSum - normalVecLen;
				manifoldOut.points[0] = c - 0.5f* normalVec;
				manifoldOut.pointsNr = 1;
				return true;
			}
			else
				return false;
		}
		else {
			glm::vec2 stadiumC2 = stadiumC1 + stadiumV;
			if (glm::distance2(c, stadiumC2) <= radiiSum * radiiSum) {
				w = c - stadiumC2;
				float wLen = glm::length(w);
				manifoldOut.normal = w / wLen;
				manifoldOut.penetrationDepth = radiiSum - wLen;
				manifoldOut.points[0] = (stadiumR*c + r*stadiumC2) / radiiSum;
				manifoldOut.pointsNr = 1;
				return true;
			}
			else
				return false;
		}
	}

	bool CollisionCircle::testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest, float& segFactorOnCollisionOut) {
		vec2 rayOriginCVec = c - segOrigin;
		vec2 rayDirection = segDest - segOrigin;
		segFactorOnCollisionOut = fmax(dot(rayOriginCVec, rayDirection) / length2(rayDirection), 0.0f);
		if (length2(rayOriginCVec - segFactorOnCollisionOut * rayDirection) <= r * r)
			return true;
		else
			return false;
	}

	bool CollisionCircle::testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest) {
		vec2 rayOriginCVec = c - segOrigin;
		vec2 rayDirection = segDest - segOrigin;	
		if (length2(rayOriginCVec - dot(rayOriginCVec, rayDirection) / length2(rayDirection) * rayDirection) <= r * r)
			return true;
		else
			return false;
	}

	CollisionPerimeter* CollisionCircle::clone(CollisionPrimitivesFactory& collisionPrimitivesFactory) const {
		return collisionPrimitivesFactory.genCollisionCircle(c, r);
	}

	void CollisionCircle::destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) {
		collisionPrimitivesFactory.destroyCollisionCircle(this);
	}

	CollisionStadium::CollisionStadium(glm::vec2 const& center1, glm::vec2 const& axisVec, float radius) : c1(center1), v(axisVec), r(radius) {}

	bool CollisionStadium::testCollision(CollisionStadium* other) {
		vec2 w = other->c1 - c1;
		vec2 u = other->v;
		float vPerpDotU = v.x*u.y - v.y*u.x;
		float radiiSum = r + other->r;
		if (abs(vPerpDotU) < EPSILON_ZERO) {
			vec2 thisVOtherVVec = w - (glm::dot(w, v) / length2(v)) * v;
			if (glm::dot(c1 + thisVOtherVVec - other->c1 - u, u) > 0.0f) {
				if (length2(other->c1 + u - c1) <= radiiSum * radiiSum)
					return true;
				else
					return false;
			}
			else if (glm::dot(c1 + v + thisVOtherVVec - other->c1, u) < 0.0f) {
				vec2 thisC2OtherC1Vec = other->c1 - c1 - v;
				if (length2(thisC2OtherC1Vec) <= radiiSum * radiiSum)
					return true;
				else
					return false;
			}
			else {		
				if (length(thisVOtherVVec) <= radiiSum)
					return true;
				else
					return false;
			}
		}

		float intersectionFactorThis = (u.y*w.x - u.x*w.y) / vPerpDotU;
		float intersectionFactorOther = (v.y*w.x - v.x*w.y) / vPerpDotU;
		if (0.0f <= intersectionFactorThis && intersectionFactorThis <= 1.0f && 0.0f <= intersectionFactorOther && intersectionFactorOther <= 1.0f)
			return true;

		glm::vec2 criticalPointThis, vThis;
		if (intersectionFactorThis <= 0.5f) {
			criticalPointThis = c1;
			vThis = v;
		}
		else {
			criticalPointThis = c1 + v;
			vThis = -v;
		}
		glm::vec2 criticalPointOther, vOther;
		if (intersectionFactorOther <= 0.5f) {
			criticalPointOther = other->c1;
			vOther = u;
		}
		else {
			criticalPointOther = other->c1 + u;
			vOther = - u;
		}

		w = criticalPointThis - criticalPointOther;
		float wDotVOther = glm::dot(w,vOther);
		float vDotV;
		if (wDotVOther <= 0.0f) {
			float wNegDotVThis = glm::dot(-w, vThis);
			if (wNegDotVThis <= 0.0f)
				return glm::length2(w) <= radiiSum * radiiSum;		
			else if (wNegDotVThis < (vDotV = glm::dot(vThis, vThis))) {			
				float vPerpDotCriticalPointOther = vThis.x*criticalPointOther.y - vThis.y*criticalPointOther.x;
				return (vPerpDotCriticalPointOther * vPerpDotCriticalPointOther) / vDotV <= radiiSum * radiiSum;
			}
			else 			
				return glm::distance2(criticalPointOther, criticalPointThis + vThis) <= radiiSum * radiiSum;		
		}
		else if (wDotVOther < (vDotV = glm::dot(vOther, vOther))) {		
			float vPerpDotCriticalPointThis = vOther.x*criticalPointThis.y - vOther.y*criticalPointThis.x;
			return (vPerpDotCriticalPointThis * vPerpDotCriticalPointThis) / vDotV <= radiiSum * radiiSum;
		}
		else		
			return glm::distance2(criticalPointThis, criticalPointOther + vOther) <= radiiSum * radiiSum;
	}

	bool CollisionStadium::testCollision(CollisionStadium* other, ContactManifold& manifoldOut) {
		vec2 w = other->c1 - c1;
		vec2 u = other->getV();
		float vPerpDotU = v.x * u.y - v.y * u.x;
		float radiiSum = r + other->r;
		if (abs(vPerpDotU) < EPSILON_ZERO) {
			vec2 thisVOtherVVec = w - (glm::dot(w, v) / length2(v)) * v;
			if (glm::dot((w = c1 - other->c1 - u) + thisVOtherVVec, u) > 0.0f) {
				if (length2(w) <= radiiSum * radiiSum) {
					float wLen = glm::length(w);
					manifoldOut.normal = w / wLen;
					manifoldOut.penetrationDepth = radiiSum - wLen;
					manifoldOut.points[0] = c1 - 0.5f*w;
					manifoldOut.pointsNr = 1;
					return true;
				}
				else
					return false;
			}
			else if (glm::dot((w = c1 + v - other->c1) + thisVOtherVVec, u) < 0.0f) {			
				if (length2(w) <= radiiSum * radiiSum) {
					float wLen = glm::length(w);
					manifoldOut.normal = w / wLen;
					manifoldOut.penetrationDepth = radiiSum - wLen;
					manifoldOut.points[0] = other->c1 + 0.5f*w;
					manifoldOut.pointsNr = 1;
					return true;
				}
				else
					return false;
			}
			else {
				if (length2(thisVOtherVVec) <= radiiSum * radiiSum) {
					float thisVOtherVVecLen = glm::length(thisVOtherVVec);				
					if (thisVOtherVVecLen > EPSILON_ZERO)
						manifoldOut.normal = thisVOtherVVec / thisVOtherVVecLen;
					else if (dot(0.5f * (2.0f*c1 + v - 2.0f * other->c1 - u), v) > 0.0f)
						manifoldOut.normal = -normalize(v);
					else
						manifoldOut.normal = normalize(v);

					manifoldOut.penetrationDepth = thisVOtherVVecLen - radiiSum;				
					manifoldOut.points[0] = dot(c1 - other->c1, u) > 0.0f ? c1 + 0.5f * thisVOtherVVec : other->c1 - 0.5f*thisVOtherVVec;
					manifoldOut.points[1] = dot(c1 + v - other->c1 - u, u) > 0.0f ? other->c1 + u - 0.5f*thisVOtherVVec : c1 + v + 0.5f*thisVOtherVVec;				
					manifoldOut.pointsNr = 2;
					return true;
				}
				else
					return false;
			}															
		}

		float intersectionFactorThis = (u.y * w.x - u.x * w.y) / vPerpDotU;
		float intersectionFactorOther = (v.y * w.x - v.x * w.y) / vPerpDotU;
		if (0.0f <= intersectionFactorThis && intersectionFactorThis <= 1.0f && 0.0f <= intersectionFactorOther && intersectionFactorOther <= 1.0f) {
			manifoldOut.normal = { 0.0f, 0.0f };
			manifoldOut.penetrationDepth = radiiSum;
			manifoldOut.points[0] = c1 + intersectionFactorThis*v;
			manifoldOut.pointsNr = 1;
			return true;
		}

		glm::vec2 criticalPointThis, vThis;
		if (intersectionFactorThis <= 0.5f) {
			criticalPointThis = c1;
			vThis = v;
		}
		else {
			criticalPointThis = c1 + v;
			vThis = -v;
		}
		glm::vec2 criticalPointOther, vOther;
		if (intersectionFactorOther <= 0.5f) {
			criticalPointOther = other->c1;
			vOther = u;
		}
		else {
			criticalPointOther = other->c1 + u;
			vOther = -u;
		}

		w = criticalPointThis - criticalPointOther;
		float wDotVOther = glm::dot(w, vOther);
		float vDotV;
		if (wDotVOther <= 0.0f) {
			float wNegDotVThis = glm::dot(-w, vThis);
			if (wNegDotVThis <= 0.0f) {
				if (glm::length2(w) <= radiiSum * radiiSum) {
					float wLen = glm::length(w);
					manifoldOut.normal = w / wLen;
					manifoldOut.penetrationDepth = radiiSum - wLen;
					manifoldOut.points[0] = (other->r*criticalPointThis + r*criticalPointOther) / radiiSum;
					manifoldOut.pointsNr = 1;
					return true;
				}
				else
					return false;
			}
			else if (wNegDotVThis < (vDotV = glm::dot(vThis, vThis))) {			
				float pointOtherVThisDistNominator = dot(glm::vec2(vThis.y, -vThis.x), w);
				if ((pointOtherVThisDistNominator * pointOtherVThisDistNominator) / vDotV <= radiiSum * radiiSum) {
					glm::vec2 normalVec = -w - glm::dot(-w, vThis)/glm::length2(vThis)*vThis;
					float normalVecLen = glm::length(normalVec);
					manifoldOut.normal = normalVec / normalVecLen;
					manifoldOut.penetrationDepth = radiiSum - normalVecLen;
					manifoldOut.points[0] = criticalPointOther - 0.5f*normalVec;
					manifoldOut.pointsNr = 1;
					return true;
				}
				else
					return false;
			}
			else {
				glm::vec2 secondaryPointThis = criticalPointThis + vThis;
				if (glm::distance2(criticalPointOther, secondaryPointThis) <= radiiSum * radiiSum) {				
					w = secondaryPointThis - criticalPointOther;
					float wLen = glm::length(w);
					manifoldOut.normal = w / wLen;
					manifoldOut.penetrationDepth = radiiSum - wLen;
					manifoldOut.points[0] = (other->r*secondaryPointThis + r*criticalPointOther) / radiiSum;
					manifoldOut.pointsNr = 1;
					return true;
				}
				else
					return false;
			}		
		}
		else if (wDotVOther < (vDotV = glm::dot(vOther, vOther))) {		
			float pointThisVOtherDistNominator = dot(glm::vec2(vOther.y, -vOther.x), w);
			if ((pointThisVOtherDistNominator * pointThisVOtherDistNominator) / vDotV <= radiiSum * radiiSum) {
				glm::vec2 normalVec = w - glm::dot(w, vOther)/glm::length2(vOther)*vOther;
				float normalVecLen = glm::length(normalVec);
				manifoldOut.normal = normalVec / normalVecLen;
				manifoldOut.penetrationDepth = radiiSum - normalVecLen;
				manifoldOut.points[0] = criticalPointThis - 0.5f*normalVec;
				manifoldOut.pointsNr = 1;
				return true;
			}
			else
				return false;
		}
		else {
			glm::vec2 secondaryPointOther = criticalPointOther + vOther;
			if (glm::distance2(criticalPointThis, secondaryPointOther) <= radiiSum * radiiSum) {
				w = secondaryPointOther - criticalPointThis;
				float wLen = glm::length(w);
				manifoldOut.normal = w / wLen;
				manifoldOut.penetrationDepth = radiiSum - wLen;
				manifoldOut.points[0] = (r*secondaryPointOther + other->r*criticalPointThis) / radiiSum;
				manifoldOut.pointsNr = 1;
				return true;
			}
			else
				return false;
		}
	}

	/*
	bool CollisionCapsule::testCollision(CollisionCapsule* other) {
		vec3 thisC1OtherC1Vec = other->c1 - c1;
		vec3 otherV = other->getV();
		if (areVecsParallel(v, otherV)) {
			vec3 thisVOtherVVec = thisC1OtherC1Vec - (dot(thisC1OtherC1Vec, v) / length2(v)) * v;
			if (dot(c1 + thisVOtherVVec - other->c1 - otherV, otherV) > 0.0f) {
				if (length2(other->c1 + otherV - c1) <= (r + other->r) * (r + other->r))
					return true;
				else
					return false;
			}
			else if (dot(c1 + v + thisVOtherVVec - other->c1, other->v) < 0.0f) {
				vec3 thisC2OtherC1Vec = other->c1 - c1 - v;
				if (length2(thisC2OtherC1Vec) <= (r + other->r) * (r + other->r))
					return true;
				else
					return false;
			}
			else {
				float thisVOtherVVecLen = length(thisVOtherVVec);
				if (thisVOtherVVecLen <= r + other->r)
					return true;
				else
					return false;
			}
		}
		else {
			float factorThis, factorOther;
			segmentSegmentClosestPointsFactors(v, otherV, thisC1OtherC1Vec, factorThis, factorOther);
			if (length2(other->c1 + factorOther * otherV - (c1 + factorThis * v)) <= (r + other->r) * (r + other->r))
				return true;
			else
				return false;
		}
	}

	bool CollisionCapsule::testCollision(CollisionCapsule* other, ContactManifold& manifoldOut) {
		vec3 thisC1OtherC1Vec = other->c1 - c1;
		vec3 otherV = other->getV();
		if (areVecsParallel(v, otherV)) {
			vec3 thisVOtherVVec = thisC1OtherC1Vec - (dot(thisC1OtherC1Vec, v) / length2(v)) * v;
			vec3 c1ClipPoint = c1 + thisVOtherVVec;
			vec3 c2ClipPoint = c1 + v + thisVOtherVVec;
			if (dot(c1ClipPoint - other->c1 - otherV, otherV) > 0.0f) {
				vec3 thisC1OtherC2Vec = other->c1 + otherV - c1;
				if (length2(thisC1OtherC2Vec) <= (r + other->r) * (r + other->r)) {
					float centersVecLen = length(thisC1OtherC2Vec);
					//if (centersVecLen > EPSILON_ZERO)
					manifoldOut.normal = thisC1OtherC2Vec / centersVecLen;
					//else
					//	manifoldOut.normal = -v / length(v);
					manifoldOut.penetrationDepth = centersVecLen - r - other->r;
					manifoldOut.pointsNr = 1;
					manifoldOut.points[0] = c1 + (r + 0.5f * manifoldOut.penetrationDepth) * manifoldOut.normal;
				}
				else
					return false;
			}
			else if (dot(c2ClipPoint - other->c1, other->v) < 0.0f) {
				vec3 thisC2OtherC1Vec = other->c1 - c1 - v;
				if (length2(thisC2OtherC1Vec) <= (r + other->r) * (r + other->r)) {
					float centersVecLen = length(thisC2OtherC1Vec);
					//if (centersVecLen > EPSILON_ZERO)
					manifoldOut.normal = thisC2OtherC1Vec / centersVecLen;
					//else
					//	manifoldOut.normal = -v / length(v);				
					manifoldOut.penetrationDepth = centersVecLen - r - other->r;
					manifoldOut.pointsNr = 1;
					manifoldOut.points[0] = c1 + v + (r + 0.5f * manifoldOut.penetrationDepth) * manifoldOut.normal;
				}
				else
					return false;
			}
			else {
				float thisVOtherVVecLen = length(thisVOtherVVec);
				if (thisVOtherVVecLen <= r + other->r) {
					if (thisVOtherVVecLen > EPSILON_ZERO)
						manifoldOut.normal = thisVOtherVVec / thisVOtherVVecLen;
					else if (dot((c1ClipPoint + c2ClipPoint - 2.0f * other->c1 - other->v) / 2.0f, v) > 0.0f)
						manifoldOut.normal = -normalize(v);
					else
						manifoldOut.normal = normalize(v);

					manifoldOut.penetrationDepth = thisVOtherVVecLen - r - other->r;
					manifoldOut.pointsNr = 2;
					manifoldOut.points[0] = dot(c1ClipPoint - other->c1, other->v) > 0.0f ? c1 + 0.5f * thisVOtherVVec : other->c1 - 0.5f * thisVOtherVVec;
					manifoldOut.points[1] = dot(c2ClipPoint - other->c1 - other->v, other->v) > 0.0f ? other->c1 + otherV - 0.5f * thisVOtherVVec : c1 + v + 0.5f * thisVOtherVVec;
				}
				else
					return false;
			}
		}
		else {
			float factorThis, factorOther;
			segmentSegmentClosestPointsFactors(v, otherV, thisC1OtherC1Vec, factorThis, factorOther);
			vec3 thisPoint = c1 + factorThis * v;
			vec3 otherPoint = other->c1 + factorOther * otherV;
			vec3 thisPOtherPVec = otherPoint - thisPoint;
			if (length2(thisPOtherPVec) <= (r + other->r) * (r + other->r)) {
				float thisPOtherPVecLen = length(thisPOtherPVec);
				manifoldOut.normal = thisPOtherPVec / thisPOtherPVecLen;
				manifoldOut.penetrationDepth = thisPOtherPVecLen - r - other->r;
				manifoldOut.pointsNr = 1;
				manifoldOut.points[0] = 0.5f * (thisPoint + otherPoint);
			}
			else
				return false;
		}

		return true;
	}

	*/

	//TODO: <<fill this>>
	bool CollisionStadium::testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest, float& segFactorOnCollisionOut) {

		return false;
	}

	//TODO: <<fill this>>
	bool CollisionStadium::testSegCollision(glm::vec2 const& segOrigin, glm::vec2 const& segDest) {
	
		return false;
	}

	CollisionPerimeter* CollisionStadium::clone(CollisionPrimitivesFactory& collisionPrimitivesFactory) const {
		return collisionPrimitivesFactory.genCollisionStadium(c1, v, r);
	}

	void CollisionStadium::destroy(CollisionPrimitivesFactory& collisionPrimitivesFactory) {
		collisionPrimitivesFactory.destroyCollisionStadium(this);
	}

} // namespace Corium3D
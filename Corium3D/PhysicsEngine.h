#pragma once

#include "ObjPool.h"
#include "TransformsStructs.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <functional>
#include <complex.h>

namespace Corium3D {

	// REMINDER: Time units - seconds !


	class PhysicsEngine {
	public:
		class MobilityInterface;
		typedef std::function<void(Transform3DUS const&)> OnMovementMadeCallback3D;
		typedef std::function<void(Transform2DUS const&)> OnMovementMadeCallback2D;

		PhysicsEngine(unsigned int mobilityInterfacesNrMax, float secsPerUpdate);
		PhysicsEngine(PhysicsEngine const&) = delete;
		MobilityInterface* addMobileGameLmnt(
			Transform3D const& initTransform,
			OnMovementMadeCallback3D* listeners3D, unsigned int listeners3DNr,
			std::complex<float> initTransform2DRot,
			OnMovementMadeCallback2D* listeners2D, unsigned int listeners2DNr);
		MobilityInterface* addMobileGameLmnt(Transform3D const& initTransform, OnMovementMadeCallback3D* listeners3D, unsigned int listeners3DNr);
		void removeMobileGameLmnt(MobilityInterface* removedMobilityInterface);
		void update(float time);
		void update();

	private:
		Corium3DUtils::ObjPoolIteratable<MobilityInterface> mobilityInterfacesPool;
		Corium3DUtils::ObjPoolIteratable<MobilityInterface>::ObjPoolIt mobilityInterfacesIt;
		float secsPerUpdate;
	};

	class PhysicsEngine::MobilityInterface {
	public:
		friend class PhysicsEngine;
		friend class Corium3DUtils::ObjPoolIteratable<MobilityInterface>;

		void translate(glm::vec3 const& translate) { transform.translate += translate; }
		void scale(float scaleFactor) { transform.scale *= scaleFactor; }
		void rot(float rot, glm::vec3 const& rotAx) { transform.rot = glm::angleAxis(rot * (float)M_PI / 180.0f, glm::normalize(rotAx)) * transform.rot; }
		void rot(glm::quat const& rot) { transform.rot = rot * transform.rot; }
		void rot2D(float rot) { transform2DRot = std::polar(1.0f, rot * (float)M_PI / 180.0f) * transform2DRot; }
		void rot2D(std::complex<float> const& rot) { transform2DRot = rot * transform2DRot; }
		void setLinVel(glm::vec3 const& _linVel) {
			linVel = _linVel;
			transformDeltaPerUpdate.translate = linVel * secsPerUpdate + 0.5f * linAccel * secsPerUpdate * secsPerUpdate;
		}
		void setAngVel(float _angVelMag, glm::vec3 const& _angVelAx) {
			angVelMag = _angVelMag * (float)M_PI / 180.0f;
			angVelAx = glm::normalize(_angVelAx);
			transformDeltaPerUpdate.rot = glm::angleAxis(angVelMag * secsPerUpdate, glm::normalize(angVelAx));
		}
		void setAngVel2D(float _angVelMag2D) {
			angVelMag2D = _angVelMag2D * (float)M_PI / 180.0f;
			transform2DRotDeltaPerUpdate = std::polar(1.0f, angVelMag2D * secsPerUpdate);
		}
		void setLinAccel(glm::vec3 const& _linAccel) {
			linAccel = _linAccel;
			linAccelPerUpdate = linAccel * secsPerUpdate;
			transformDeltaPerUpdate.translate = linAccelPerUpdate + 0.5f * linAccel * secsPerUpdate * secsPerUpdate;
		}
		void setAngAccel(glm::vec3 const& angAccel) {}
		void setLinVelX(float x) {
			linVel.x = x;
			transformDeltaPerUpdate.translate.x = x * secsPerUpdate + 0.5f * linAccel.x * secsPerUpdate * secsPerUpdate;
		}
		void setLinVelY(float y) {
			linVel.y = y;
			transformDeltaPerUpdate.translate.y = y * secsPerUpdate + 0.5f * linAccel.y * secsPerUpdate * secsPerUpdate;
		}
		void setLinVelZ(float z) {
			linVel.z = z;
			transformDeltaPerUpdate.translate.z = z * secsPerUpdate + 0.5f * linAccel.z * secsPerUpdate * secsPerUpdate;
		}

		glm::vec3 getTranslate() const { return transform.translate; }
		glm::vec3 getScale() const { return transform.scale; }
		glm::quat getRot() const { return transform.rot; }
		std::complex<float> getRot2D() const { return transform2DRot; }
		glm::mat4 getTransformat(float extraTime) {
			return genTransformat({ transform.translate + extraTime * linVel + 0.5f * linAccel * extraTime * extraTime,
								   transform.scale, transform.rot * glm::angleAxis(angVelMag * extraTime, angVelAx) });
		}
		glm::mat4 getTransformat() const { return genTransformat(transform); }

	private:
		glm::vec3 linVel, linAccel; // linear velocity, linear acceleration
		glm::vec3 linAccelPerUpdate;
		float angVelMag = 0.0f; // angular velocity 3D magnitude
		glm::vec3 angVelAx; // angular velocity 3D axis	
		float angVelMag2D = 0.0f; // angular velocity 2D magnitude

		Transform3D transform;
		Transform3DUS transformDeltaPerUpdate;
		std::complex<float> transform2DRot = std::complex<float>(1.0f, 0.0f);
		std::complex<float> transform2DRotDeltaPerUpdate = std::complex<float>(1.0f, 0.0f);

		OnMovementMadeCallback3D* listeners3D;
		unsigned int listeners3DNr;
		OnMovementMadeCallback2D* listeners2D;
		unsigned int listeners2DNr;

		float secsPerUpdate;

		MobilityInterface(Transform3D const& initTransform,
			OnMovementMadeCallback3D* listeners3D, unsigned int listenersNr3D,
			std::complex<float> initTransform2DRot,
			OnMovementMadeCallback2D* listeners2D, unsigned int listenersNr2D,
			float secsPerUpdate);
		MobilityInterface(Transform3D const& initTransform, OnMovementMadeCallback3D* listeners3D, unsigned int listenersNr3D, float secsPerUpdate);
		MobilityInterface(MobilityInterface const& mobilityInterface);
		~MobilityInterface();
		void update(float time);
		void update();
		void setTranslate(glm::vec3 const& translate) { transform.translate = translate; }		
		// void setScale(float scaleFactor) { transform.scale = scaleFactor; }
		void setRot(float rot, glm::vec3 const& rotAx) { transform.rot = glm::angleAxis(rot * (float)M_PI / 180.0f, glm::normalize(rotAx)); }
		void setRot(glm::quat const& rot) { transform.rot = rot; }
		void setRot2D(float rot) { transform2DRot = std::polar(1.0f, rot * (float)M_PI / 180.0f); }
		void setRot2D(std::complex<float> const& rot) { transform2DRot = rot; }
		static glm::mat4 genTransformat(Transform3D const& transform) {
			return glm::translate(transform.translate) * glm::mat4_cast(transform.rot) * glm::scale(transform.scale);
		}
	};

} // namespace Corium3D
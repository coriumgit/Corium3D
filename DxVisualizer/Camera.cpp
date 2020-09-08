#include "Camera.h"
#include <iostream>

using namespace DirectX;

namespace CoriumDirectX {

	Camera::Camera(float fov, float _screenWidth, float _screenHeight, float _nearZ, float _farZ) :
			fovVert(fov * XM_PI / 180.0f), screenWidth(_screenWidth), screenHeight(_screenHeight),
			nearZ(_nearZ), farZ(_farZ) {
		screenWidthDiv2 = 0.5f * screenWidth;
		screenHeightDiv2 = 0.5f * screenHeight;
		recompFovHorizon();
		updateFrustumSidePlanes();
		updateFrustumNearPlane();
		updateFrustumFarPlane();
		recompProjRectAndMat();
		recompViewMat();
	}

	void Camera::panViaViewportDrag(float x, float y) {
		pos += -(x * CAMERA_TRANSLATION_PER_CURSOR_MOVE * rightVec) + (y * CAMERA_TRANSLATION_PER_CURSOR_MOVE * upVec);

		updateFrustumDs();
		recompViewMat();
	}

	void Camera::rotateViaViewportDrag(float x, float y) {
		float rotAng = sqrt(x*x + y*y) * CAMERA_ROT_PER_CURSOR_MOVE;
		XMVECTOR rotAxViewport = XMVectorSet(y, x, 0.0f, 0.0f);
		XMVECTOR rotAx = XMVector3Transform(rotAxViewport, XMMATRIX(rightVec, upVec, lookAtVec, XMVectorSet(0.0, 0.0f, 0.0f, 1.0f)));
		rotate(rotAx, rotAng);
	} 

	void Camera::rotate(FXMVECTOR rotAx, float ang) {
		XMVECTOR quat = XMQuaternionRotationAxis(rotAx, ang);
		lookAtVec = XMVector3Rotate(lookAtVec, quat);
		rightVec = XMVector3Rotate(rightVec, quat);
		upVec = XMVector3Cross(lookAtVec, rightVec);
		double t = -XMVectorGetY(rightVec) / XMVectorGetY(upVec);
		//generating a new cameraLeftDirection so that it parallels the XZ plane
		rightVec = XMVectorSet(XMVectorGetX(rightVec) + t * XMVectorGetX(upVec),
			0.0f,
			XMVectorGetZ(rightVec) + t * XMVectorGetZ(upVec),
			0.0f);
		rightVec = XMVector3Normalize(rightVec);
		upVec = XMVector3Cross(lookAtVec, rightVec);

		updateFrustumSidePlanes();
		updateFrustumNearPlane();
		updateFrustumFarPlane();

		recompViewMat();
	}

	void Camera::zoom(float amount) {
		pos += amount * CAMERA_ZOOM_PER_MOUSE_WHEEL_TURN * lookAtVec;

		updateFrustumDs();
		recompViewMat();
	}

	void Camera::updateFov(float fov) {
		fovVert = fov * XM_PI / 180.0f;
		recompFovHorizon();
		updateFrustumSidePlanes();
		recompProjRectAndMat();
	}

	void Camera::updateScreenSz(float width, float height) {
		screenWidth = width;
		screenHeight = height;
		screenWidthDiv2 = 0.5f * screenWidth;
		screenHeightDiv2 = 0.5f * screenHeight;
		recompFovHorizon();
		updateFrustumSidePlanes();
		recompProjRectAndMat();	
	}

	void Camera::updateNearZ(float _nearZ) {
		nearZ = _nearZ;
		updateFrustumNearPlane();
		recompProjRectAndMat();
	}

	void Camera::updateFarZ(float _farZ) {
		nearZ = _farZ;

		updateFrustumFarPlane();
		recompProjRectAndMat();
	}

	DirectX::XMVECTOR Camera::screenVecToWorldVec(float x, float y) {
		return x*rightVec + y*upVec;
	}

	DirectX::XMVECTOR Camera::cursorPosToRayDirection(float x, float y) {				
		XMFLOAT2 cursorPosOffsetedByMid = { x - screenWidthDiv2, screenHeightDiv2 - y };

		return XMVector3Normalize(nearZ * lookAtVec +
			cursorPosOffsetedByMid.x / screenWidthDiv2 * projRectWidthDiv2 * rightVec +
			cursorPosOffsetedByMid.y / screenHeightDiv2 * projRectHeightDiv2 * upVec);				
	}
	/* implementation of spaces transformations reversal (viewport -> ndc -> perspective projection -> view)
	   *****************************************************************************************************
		XMFLOAT2 cursorPosOffsetedByMid = { (x - screenWidthDiv2) / screenWidthDiv2, (screenHeightDiv2 - y) / screenHeightDiv2 };
		//std::cout << "(x,y) = (" << cursorPosOffsetedByMid.x << "," << cursorPosOffsetedByMid.y << ")" << std::endl;

		XMVECTOR rayClip = XMVectorSet(cursorPosOffsetedByMid.x, cursorPosOffsetedByMid.y, 1.0f, 1.0f);
		XMVECTOR rayEye = XMVector4Transform(rayClip, XMMatrixInverse(NULL, projMat));
		rayEye = XMVectorSet(XMVectorGetX(rayEye), XMVectorGetY(rayEye), 1.0f, 0.0f);

		return XMVector4Normalize(XMVector4Transform(rayEye, XMMatrixInverse(NULL, viewMat)));
	*/

	bool Camera::isBoundingSphereVisible(BoundingSphere const& boundingSphere) const {		
		//return true;
		XMVECTOR const& sphereC = boundingSphere.getCenter();
		XMVECTOR const& sphereR = XMVectorReplicate(boundingSphere.getRadius());
		XMVECTOR isVisible = XMVectorLessOrEqual(XMPlaneDotCoord(frustumFarPlane, sphereC), sphereR);
		isVisible = XMVectorAndInt(XMVectorLessOrEqual(XMPlaneDotCoord(frustumNearPlane, sphereC), sphereR), isVisible);
		isVisible = XMVectorAndInt(XMVectorLessOrEqual(XMPlaneDotCoord(frustumLeftPlane, sphereC), sphereR), isVisible);
		isVisible = XMVectorAndInt(XMVectorLessOrEqual(XMPlaneDotCoord(frustumRightPlane, sphereC), sphereR), isVisible);
		isVisible = XMVectorAndInt(XMVectorLessOrEqual(XMPlaneDotCoord(frustumTopPlane, sphereC), sphereR), isVisible);
		isVisible = XMVectorAndInt(XMVectorLessOrEqual(XMPlaneDotCoord(frustumBotPlane, sphereC), sphereR), isVisible);
		
		return XMVector4EqualInt(isVisible, XMVectorTrueInt());
	}	

	void Camera::recompFovHorizon() {
		fovHorizon = atanf(tanf(fovVert) * screenHeight / screenWidth);
	}

	void Camera::recompViewMat() {
		viewMat = DirectX::XMMatrixLookAtLH(pos, pos + lookAtVec, upVec);
	}

	void Camera::recompProjRectAndMat() {		
		projRectHeightDiv2 = nearZ * tan(0.5 * fovVert);
		float aspectRatio = screenWidthDiv2 / screenHeightDiv2;
		projRectWidthDiv2 = aspectRatio * projRectHeightDiv2;
		projMat = XMMatrixPerspectiveFovLH(fovVert, aspectRatio, nearZ, farZ);
	}

	void Camera::updateFrustumDs() {
		frustumTopPlane = XMPlaneFromPointNormal(pos, frustumTopPlane);
		frustumBotPlane = XMPlaneFromPointNormal(pos, frustumBotPlane);
		frustumLeftPlane = XMPlaneFromPointNormal(pos, frustumLeftPlane);
		frustumRightPlane = XMPlaneFromPointNormal(pos, frustumRightPlane);
		frustumNearPlane = XMPlaneFromPointNormal(pos + nearZ*lookAtVec, frustumNearPlane);
		frustumFarPlane = XMPlaneFromPointNormal(pos + farZ*lookAtVec, frustumFarPlane);
	}

	void Camera::updateFrustumSidePlanes() {
		frustumTopPlane = XMPlaneFromPointNormal(pos, XMVector3Rotate(upVec, XMQuaternionRotationAxis(rightVec, -0.5f * fovVert)));
		frustumBotPlane = XMPlaneFromPointNormal(pos, XMVector3Rotate(-upVec, XMQuaternionRotationAxis(rightVec, 0.5f * fovVert)));
		frustumLeftPlane = XMPlaneFromPointNormal(pos, XMVector3Rotate(-rightVec, XMQuaternionRotationAxis(upVec, -0.5f * fovHorizon)));
		frustumRightPlane = XMPlaneFromPointNormal(pos, XMVector3Rotate(rightVec, XMQuaternionRotationAxis(upVec, 0.5f * fovHorizon)));
	}

	void Camera::updateFrustumNearPlane() {
		frustumNearPlane = XMPlaneFromPointNormal(pos + nearZ * lookAtVec, -lookAtVec);		
	}

	void Camera::updateFrustumFarPlane() {		
		frustumFarPlane = XMPlaneFromPointNormal(pos + farZ * lookAtVec, lookAtVec);
	}
};
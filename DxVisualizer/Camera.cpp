#include "Camera.h"
#include <iostream>

using namespace DirectX;

namespace CoriumDirectX {

	Camera::Camera(float _fov, float _screenWidth, float _screenHeight, float _nearZ, float _farZ) :
		fov(_fov * XM_PI / 180.0f), screenWidth(_screenWidth), screenHeight(_screenHeight), nearZ(_nearZ), farZ(_farZ) {		
		screenWidthDiv2 = 0.5f * screenWidth;
		screenHeightDiv2 = 0.5f * screenHeight;
		recompProjRectAndMat();
		recompViewMat();
	}

	void Camera::panViaViewportDrag(float x, float y) {
		pos += -(x * CAMERA_TRANSLATION_PER_CURSOR_MOVE * rightVec) + (y * CAMERA_TRANSLATION_PER_CURSOR_MOVE * upVec);

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

		recompViewMat();
	}

	void Camera::zoom(float amount) {
		pos += amount * CAMERA_ZOOM_PER_MOUSE_WHEEL_TURN * lookAtVec;

		recompViewMat();
	}

	void Camera::updateFov(float _fov) {
		fov = _fov * XM_PI / 180.0f;
		recompProjRectAndMat();
	}

	void Camera::updateScreenSz(float width, float height) {
		screenWidth = width;
		screenHeight = height;
		screenWidthDiv2 = 0.5f * screenWidth;
		screenHeightDiv2 = 0.5f * screenHeight;
		recompProjRectAndMat();
	}

	void Camera::updateNearZ(float _nearZ) {
		nearZ = _nearZ;
		recompProjRectAndMat();
	}

	void Camera::updateFarZ(float _farZ) {
		nearZ = _farZ;
		recompProjRectAndMat();
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

	void Camera::recompViewMat() {
		viewMat = DirectX::XMMatrixLookAtLH(pos, pos + lookAtVec, upVec);
	}

	void Camera::recompProjRectAndMat() {		
		projRectHeightDiv2 = nearZ * tan(0.5 * fov);
		float aspectRatio = screenWidthDiv2 / screenHeightDiv2;
		projRectWidthDiv2 = aspectRatio * projRectHeightDiv2;
		projMat = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
	}

};
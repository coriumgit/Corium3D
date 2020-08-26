#pragma once

#include "BoundingSphere.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

const float CAMERA_TRANSLATION_PER_CURSOR_MOVE = 0.02f;
constexpr float CAMERA_ROT_PER_CURSOR_MOVE = 0.05f * DirectX::XM_PI / 180;
const float CAMERA_ZOOM_PER_MOUSE_WHEEL_TURN = 0.01f;

namespace CoriumDirectX {

    class Camera {
    public:
        Camera(float fov, float screenWidth, float screenHeight, float nearZ, float farZ);
        void panViaViewportDrag(float x, float y);
        void rotateViaViewportDrag(float x, float y);
        void rotate(DirectX::FXMVECTOR rotAxis, float ang);
        void zoom(float amount);
        void updateFov(float fov);
        void updateScreenSz(float width, float height);        
        void updateNearZ(float nearZ);
        void updateFarZ(float farZ);                

        float getFieldOfView() { return fovVert; }
        float getNearZ() { return nearZ; }
        float getFarZ() { return farZ; }
        DirectX::XMVECTOR getPos() { return pos; }
        DirectX::XMVECTOR getLookAtVec() { return lookAtVec; }
        DirectX::XMVECTOR getUpVec() { return upVec; }
        DirectX::FXMMATRIX getProjMat() { return projMat; }
        DirectX::FXMMATRIX getViewMat() { return viewMat; }
        DirectX::XMVECTOR cursorPosToRayDirection(float x, float y);        
        bool isBoundingSphereVisible(BoundingSphere const& boundingSphere) const;

    private:         
        // TODO: have camera be part of the renderer
        float fovVert;
        float fovHorizon;        
        float nearZ;
        float farZ;
        float screenWidth;
        float screenHeight;
        DirectX::XMVECTOR frustumTopPlane;
        DirectX::XMVECTOR frustumBotPlane;
        DirectX::XMVECTOR frustumLeftPlane;
        DirectX::XMVECTOR frustumRightPlane;
        DirectX::XMVECTOR frustumNearPlane;
        DirectX::XMVECTOR frustumFarPlane;

        DirectX::XMVECTOR pos = DirectX::XMVectorSet(0.0, 5.0, -5.0, 0.0f);
        DirectX::XMVECTOR lookAtVec = DirectX::XMVector3Normalize(DirectX::XMVectorSet(0.0, -1.0, 1.0, 0.0f));
        DirectX::XMVECTOR rightVec = DirectX::XMVectorSet(1.0, 0.0, 0.0, 0.0f);
        DirectX::XMVECTOR upVec = DirectX::XMVector3Cross(lookAtVec, rightVec);
        DirectX::XMMATRIX viewMat;
        DirectX::XMMATRIX projMat;

        // cached values
        float screenWidthDiv2;
        float screenHeightDiv2;
        float projRectWidthDiv2;
        float projRectHeightDiv2;

        void recompFovHorizon();
        void recompViewMat();        
        void recompProjRectAndMat();
        void updateFrustumDs();
        void updateFrustumSidePlanes();
        void updateFrustumNearPlane();
        void updateFrustumFarPlane();
    };

}
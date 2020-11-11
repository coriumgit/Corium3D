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

        float getFOV() const { return fovVert; }
        float getNearZ() const { return nearZ; }
        float getFarZ() const { return farZ; }
        DirectX::CXMVECTOR getPos() const { return pos; }
        DirectX::CXMVECTOR getLookAtVec() const { return lookAtVec; }
        DirectX::CXMVECTOR getUpVec() const { return upVec; }
        DirectX::CXMMATRIX getProjMat() const { return projMat; }
        DirectX::CXMMATRIX getViewMat() const { return viewMat; }
        DirectX::XMVECTOR screenVecToWorldVec(float x, float y) const;
        DirectX::XMVECTOR cursorPosToRayDirection(float x, float y) const;        
        bool isBoundingSphereVisible(BoundingSphere const& boundingSphere) const;
        bool isAabbVisible(DirectX::CXMVECTOR aabbMin, DirectX::CXMVECTOR aabbMax) const;

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
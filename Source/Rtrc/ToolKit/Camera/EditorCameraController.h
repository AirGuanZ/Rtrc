#pragma once

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Core/Variant.h>
#include <Rtrc/RHI/Window/WindowInput.h>
#include <Rtrc/ToolKit/Camera/Camera.h>

RTRC_BEGIN

class EditorCameraController
{
public:

    void SetCamera(Ref<Camera> camera);

    void UpdateControlDataFromCamera();

    float GetTrackballDistance() const;
    void SetTrackballDistance(float value);

    RTRC_SET_GET(float, PanningSpeed,      panningSpeed_)
    RTRC_SET_GET(float, RotatingSpeed,     rotatingSpeed_)
    RTRC_SET_GET(float, MoveSpeed,         moveSpeed_)
    RTRC_SET_GET(float, ZoomSpeed,         zoomSpeed_)

    void ClearInputStates();

    bool Update(const WindowInput &input, float dt);

private:

    static constexpr float PI = std::numbers::pi_v<float>;

    static Vector3f YawPitchToDirection(const Vector2f &yawPitch);
    static Vector2f DirectionToYawPitch(const Vector3f &direction);

    bool UpdateWalkMode(const WindowInput &input, float dt);
    bool UpdateTrackballMode(const WindowInput &input, float dt);

    struct WalkModeState
    {
        Vector3f targetPosition;
        Vector3f currentPosition;
    };

    struct TrackModeState
    {
        Vector3f targetCenter;
        Vector3f currentCenter;
    };

    using State = Variant<WalkModeState, TrackModeState>;

    Camera *camera_ = nullptr;

    float panningSpeed_ = 0.001f;
    float rotatingSpeed_ = 0.004f;
    float moveSpeed_ = 1;
    float zoomSpeed_ = 0.05f;

    State    state_;
    Vector2f targetYawPitch_ = Vector2f(0, 0);
    Vector2f currentYawPitch_ = Vector2f(0, 0);
    float    targetTrackballDistance_ = 1.0f;
    float    currentTrackballDistance_ = 1.0f;
    
    bool isPanning = false;
    bool isRotating = false;
    Vector3f trackballCenterWhenStartPanning_;
    Vector2f yawPitchWhenStartRotating_;
    Vector2f mousePosWhenStartPanning_;
    Vector2f mousePosWhenStartRotating_;
};

RTRC_END

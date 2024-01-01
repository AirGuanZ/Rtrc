#include <Rtrc/ToolKit/Camera/EditorCameraController.h>

RTRC_BEGIN

void EditorCameraController::SetCamera(Camera *camera)
{
    assert(!camera_);
    camera_ = camera;
    ClearInputStates();
}

void EditorCameraController::ClearInputStates()
{
    isPanning = false;
    isRotating = false;
}

bool EditorCameraController::Update(const WindowInput &input, float dt)
{
    enum Mode
    {
        Walk,
        Trackball
    };

    Mode mode;
    if(input.IsKeyPressed(KeyCode::LeftAlt))
    {
        mode = Trackball;
    }
    else if(input.IsKeyPressed(KeyCode::MouseRight))
    {
        mode = Walk;
    }
    else
    {
        isPanning = false;
        isRotating = false;
        return false;
    }

    if(mode == Trackball)
    {
        return UpdateTrackballMode(input);
    }
    return UpdateWalkMode(input, dt);
}

Vector3f EditorCameraController::YawPitchToDirection(const Vector2f &yawPitch)
{
    return
    {
        std::cos(yawPitch.x) * std::cos(yawPitch.y),
        std::sin(yawPitch.y),
        std::sin(yawPitch.x) * std::cos(yawPitch.y)
    };
}

Vector2f EditorCameraController::DirectionToYawPitch(const Vector3f &direction)
{
    const Vector3f nd = Normalize(direction);
    return { Atan2Safe(nd.z, nd.x), std::asin(nd.y) };
}

bool EditorCameraController::UpdateWalkMode(const WindowInput &input, float dt)
{
    isPanning = false;
    isRotating = false;

    bool isDirty = false;

    if(input.GetRelativeWheelScrollOffset() > 0)
        moveSpeed_ *= 1.1f;
    else if(input.GetRelativeWheelScrollOffset() < 0)
        moveSpeed_ *= 0.9f;
    moveSpeed_ = std::clamp(moveSpeed_, 0.1f, 100.0f);

    Vector3f moveDirection = {};
    if(const int front = input.IsKeyPressed(KeyCode::W) - input.IsKeyPressed(KeyCode::S))
    {
        const Vector3f &rawFront = camera_->GetFront();
        const Vector3f frontDirection = NormalizeIfNotZero(Vector3f(rawFront.x, 0, rawFront.z));
        moveDirection += static_cast<float>(front) * frontDirection;
    }
    if(const int left = input.IsKeyPressed(KeyCode::A) - input.IsKeyPressed(KeyCode::D))
    {
        const Vector3f &rawLeft = camera_->GetLeft();
        const Vector3f leftDirection = NormalizeIfNotZero(Vector3f(rawLeft.x, 0, rawLeft.z));
        moveDirection += static_cast<float>(left) * leftDirection;
    }
    if(const int up = input.IsKeyPressed(KeyCode::E) - input.IsKeyPressed(KeyCode::Q))
    {
        const Vector3f upDirection = { 0, 1, 0 };
        moveDirection += static_cast<float>(up) * upDirection;
    }
    if(moveDirection != Vector3f(0, 0, 0))
    {
        moveDirection = Normalize(moveDirection);
        isDirty = true;
    }
    const Vector3f position = camera_->GetPosition() + moveDirection * moveSpeed_ * dt;
    
    Vector2f yawPitch = DirectionToYawPitch(camera_->GetFront());
    if(input.GetCursorRelativePositionX() != 0)
    {
        yawPitch.x += rotatingSpeed_ * input.GetCursorRelativePositionX();
        while(yawPitch.x > 2 * PI)
            yawPitch.x -= 2 * PI;
        while(yawPitch.x < 0)
            yawPitch.x += 2 * PI;
        isDirty = true;
    }
    if(input.GetCursorRelativePositionY() != 0)
    {
        yawPitch.y -= rotatingSpeed_ * input.GetCursorRelativePositionY();
        yawPitch.y = std::clamp(yawPitch.y, -PI / 2 + 0.01f, PI / 2 - 0.01f);
        isDirty = true;
    }

    if(isDirty)
    {
        camera_->SetLookAt(position, Vector3f(0, 1, 0), position + YawPitchToDirection(yawPitch));
    }
    return isDirty;
}

bool EditorCameraController::UpdateTrackballMode(const WindowInput &input)
{
    // Move

    bool isDirty = false;
    Vector3f center = camera_->GetPosition() + camera_->GetFront() * trackballDistance_;

    if(input.IsKeyDown(KeyCode::MouseMiddle))
    {
        isPanning = true;
        trackballCenterWhenStartPanning_ = center;
        mousePosWhenStartPanning_ = { input.GetCursorAbsolutePositionX(), input.GetCursorAbsolutePositionY() };
    }

    if(isPanning && input.IsKeyPressed(KeyCode::MouseMiddle))
    {
        const float dx = input.GetCursorAbsolutePositionX() - mousePosWhenStartPanning_.x;
        const float dy = input.GetCursorAbsolutePositionY() - mousePosWhenStartPanning_.y;
        if(std::abs(dx) > 1e-3f || std::abs(dy) > 1e-3f)
        {
            const Vector3f dstToEye = -camera_->GetFront();
            const Vector3f ex = -Normalize(Cross(Vector3f(0, 1, 0), dstToEye));
            const Vector3f ey = -Normalize(Cross(dstToEye, ex));
            center = trackballCenterWhenStartPanning_
                   + panningSpeed_ * trackballDistance_ * (dx * ex + dy * ey);
            isDirty = true;
        }
    }
    else
        isPanning = false;

    // Rotate

    Vector2f yawPitch = DirectionToYawPitch(-camera_->GetFront());

    if(input.IsKeyDown(KeyCode::MouseLeft))
    {
        isRotating = true;
        mousePosWhenStartRotating_ = { input.GetCursorAbsolutePositionX(), input.GetCursorAbsolutePositionY() };
        trackballCenterToEyeWhenStartRotating_ = yawPitch;
    }

    if(isRotating && input.IsKeyPressed(KeyCode::MouseLeft))
    {
        const float dx = input.GetCursorAbsolutePositionX() - mousePosWhenStartRotating_.x;
        const float dy = input.GetCursorAbsolutePositionY() - mousePosWhenStartRotating_.y;
        if(std::abs(dx) > 1e-3f || std::abs(dy) > 1e-3f)
        {
            yawPitch.x = trackballCenterToEyeWhenStartRotating_.x + dx * rotatingSpeed_;
            yawPitch.y = trackballCenterToEyeWhenStartRotating_.y + dy * rotatingSpeed_;
            while(yawPitch.x > 2 * PI)
                yawPitch.x -= 2 * PI;
            while(yawPitch.x < 0)
                yawPitch.x += 2 * PI;
            yawPitch.y = std::clamp(yawPitch.y, -PI / 2 + 0.01f, PI / 2 - 0.01f);
            isDirty = true;
        }
    }
    else
    {
        isRotating = false;
    }

    // Zoom

    const int scroll = input.GetRelativeWheelScrollOffset();
    if(scroll < 0)
    {
        trackballDistance_ *= (1 + zoomSpeed);
        isDirty = true;
    }
    else if(scroll > 0)
    {
        trackballDistance_ *= (1 - zoomSpeed);
        isDirty = true;
    }
    trackballDistance_ = std::max(trackballDistance_, 0.01f);

    // Apply

    if(isDirty)
    {
        const Vector3f back = YawPitchToDirection(yawPitch);
        camera_->SetLookAt(center + trackballDistance_ * back, Vector3f(0, 1, 0), center);
    }
    return isDirty;
}

RTRC_END

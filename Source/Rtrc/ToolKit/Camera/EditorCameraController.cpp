#include <Rtrc/ToolKit/Camera/EditorCameraController.h>

RTRC_BEGIN

void EditorCameraController::SetCamera(Ref<Camera> camera)
{
    assert(!camera_);
    camera_ = camera;
    UpdateControlDataFromCamera();
    ClearInputStates();
}

void EditorCameraController::SetTrackballDistance(float value)
{
    targetTrackballDistance_ = value;
    currentTrackballDistance_ = value;
}

float EditorCameraController::GetTrackballDistance() const
{
    return targetTrackballDistance_;
}

void EditorCameraController::UpdateControlDataFromCamera()
{
    if(camera_)
    {
        camera_->UpdateDerivedData();
        state_ = WalkModeState{ camera_->GetPosition(), camera_->GetPosition() };
        targetYawPitch_ = DirectionToYawPitch(camera_->GetFront());
        currentYawPitch_ = targetYawPitch_;
    }
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

    if(mode == Trackball && state_.Is<WalkModeState>())
    {
        TrackModeState newState;
        newState.targetCenter = state_.As<WalkModeState>().targetPosition + YawPitchToDirection(targetYawPitch_) * targetTrackballDistance_;
        newState.currentCenter = state_.As<WalkModeState>().currentPosition + YawPitchToDirection(currentYawPitch_) * currentTrackballDistance_;
        currentTrackballDistance_ = targetTrackballDistance_;
        state_ = newState;
    }
    else if(mode == Walk && state_.Is<TrackModeState>())
    {
        WalkModeState newState;
        newState.targetPosition = state_.As<TrackModeState>().targetCenter - YawPitchToDirection(targetYawPitch_) * targetTrackballDistance_;
        newState.currentPosition = state_.As<TrackModeState>().currentCenter - YawPitchToDirection(currentYawPitch_) * currentTrackballDistance_;
        currentTrackballDistance_ = targetTrackballDistance_;
        state_ = newState;
    }

    if(mode == Trackball)
    {
        return UpdateTrackballMode(input, dt);
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
    auto &state = state_.As<WalkModeState>();

    isPanning = false;
    isRotating = false;

    bool isDirty = false;

    if(input.GetRelativeWheelScrollOffset() > 0)
    {
        moveSpeed_ *= 1.05f;
    }
    else if(input.GetRelativeWheelScrollOffset() < 0)
    {
        moveSpeed_ *= 0.95f;
    }
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
    const Vector3f position = state.targetPosition + moveDirection * moveSpeed_ * dt;
    
    Vector2f yawPitch = targetYawPitch_;
    if(input.GetCursorRelativePositionX() != 0)
    {
        yawPitch.x += rotatingSpeed_ * input.GetCursorRelativePositionX();
        //yawPitch.x -= std::floor(yawPitch.x / (2 * PI)) * (2 * PI);
        isDirty = true;
    }
    if(input.GetCursorRelativePositionY() != 0)
    {
        yawPitch.y -= rotatingSpeed_ * input.GetCursorRelativePositionY();
        yawPitch.y = std::clamp(yawPitch.y, -PI / 2 + 0.02f, PI / 2 - 0.02f);
        isDirty = true;
    }

    if(isDirty)
    {
        state.targetPosition = position;
        targetYawPitch_ = yawPitch;
    }

    const auto oldCurrentPosition = state.currentPosition;
    const auto oldCurrentYawPitch = currentYawPitch_;

    state.currentPosition += (state.targetPosition - state.currentPosition) * (1 - std::exp(-moveSpeed_ * 10000 * dt));
    currentYawPitch_ += (targetYawPitch_ - currentYawPitch_) * (1 - std::exp(-rotatingSpeed_ * 6000 * dt));
    camera_->SetLookAt(
        state.currentPosition,
        Vector3f(0, 1, 0),
        state.currentPosition + YawPitchToDirection(currentYawPitch_));

    isDirty |= oldCurrentPosition != state.currentPosition || oldCurrentYawPitch != currentYawPitch_;

    return isDirty;
}

bool EditorCameraController::UpdateTrackballMode(const WindowInput &input, float dt)
{
    auto &state = state_.As<TrackModeState>();

    // Move

    bool isDirty = false;
    Vector3f center = state.targetCenter;

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
            center = trackballCenterWhenStartPanning_ + panningSpeed_ * targetTrackballDistance_ * (dx * ex + dy * ey);
            isDirty = true;
        }
    }
    else
        isPanning = false;

    // Rotate

    Vector2f yawPitch = targetYawPitch_;

    if(input.IsKeyDown(KeyCode::MouseLeft))
    {
        isRotating = true;
        mousePosWhenStartRotating_ = { input.GetCursorAbsolutePositionX(), input.GetCursorAbsolutePositionY() };
        yawPitchWhenStartRotating_ = yawPitch;
    }

    if(isRotating && input.IsKeyPressed(KeyCode::MouseLeft))
    {
        const Vector2f oldYawPitch = yawPitch;
        if(input.GetCursorRelativePositionX() != 0 || input.GetCursorRelativePositionY() != 0)
        {
            const float dx = input.GetCursorAbsolutePositionX() - mousePosWhenStartRotating_.x;
            const float dy = input.GetCursorAbsolutePositionY() - mousePosWhenStartRotating_.y;
            if(std::abs(dx) > 1e-3f)
            {
                yawPitch.x = yawPitchWhenStartRotating_.x + dx * rotatingSpeed_;
                //yawPitch.x -= std::floor(yawPitch.x / (2 * PI)) * (2 * PI);
            }
            if(std::abs(dy) > 1e-3f)
            {
                yawPitch.y = yawPitchWhenStartRotating_.y - dy * rotatingSpeed_;
                yawPitch.y = std::clamp(yawPitch.y, -PI / 2 + 0.01f, PI / 2 - 0.01f);
            }
        }
        isDirty |= oldYawPitch != yawPitch;
    }
    else
    {
        isRotating = false;
    }

    // Zoom

    const int scroll = input.GetRelativeWheelScrollOffset();
    if(scroll < 0)
    {
        targetTrackballDistance_ *= (1 + zoomSpeed_);
        isDirty = true;
    }
    else if(scroll > 0)
    {
        targetTrackballDistance_ *= (1 - zoomSpeed_);
        isDirty = true;
    }
    targetTrackballDistance_ = std::max(targetTrackballDistance_, 0.01f);

    // Apply

    if(isDirty)
    {
        state.targetCenter = center;
        targetYawPitch_ = yawPitch;
    }

    const auto oldCurrentCenter = state.currentCenter;
    const auto oldCurrentYawPitch = currentYawPitch_;
    const auto oldCurrentTrackballDistance = currentTrackballDistance_;

    state.currentCenter += (state.targetCenter - state.currentCenter) * (1 - std::exp(-panningSpeed_ * 20000 * dt));
    currentYawPitch_ += (targetYawPitch_ - currentYawPitch_) * (1 - std::exp(-rotatingSpeed_ * 6000 * dt));
    currentTrackballDistance_ += (targetTrackballDistance_ - currentTrackballDistance_) * (1 - std::exp(-zoomSpeed_ * 1000 * dt));

    const Vector3f back = -YawPitchToDirection(currentYawPitch_);
    camera_->SetLookAt(
        state.currentCenter + currentTrackballDistance_ * back, Vector3f(0, 1, 0), state.currentCenter);

    isDirty |= oldCurrentCenter != state.currentCenter ||
               oldCurrentYawPitch != currentYawPitch_ ||
               oldCurrentTrackballDistance != currentTrackballDistance_;

    return isDirty;
}

RTRC_END

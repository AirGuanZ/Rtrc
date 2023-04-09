#include <Rtrc/Scene/Camera/CameraController.h>

RTRC_BEGIN

void FreeCameraController::SetCamera(Camera &camera)
{
    camera_ = &camera;
}

void FreeCameraController::SetMoveSpeed(float speed)
{
    moveSpeed_ = speed;
}

void FreeCameraController::SetRotateSpeed(float speed)
{
    rotateSpeed_ = speed;
}

bool FreeCameraController::UpdateCamera(const WindowInput &input, const Timer &timer)
{
    if(!camera_)
    {
        return false;
    }

    bool ret = false;

    // Move

    int forwardStep = 0;
    forwardStep += static_cast<int>(input.IsKeyPressed(KeyCode::W));
    forwardStep -= static_cast<int>(input.IsKeyPressed(KeyCode::S));

    int leftStep = 0;
    leftStep += static_cast<int>(input.IsKeyPressed(KeyCode::A));
    leftStep -= static_cast<int>(input.IsKeyPressed(KeyCode::D));

    int upStep = 0;
    upStep += static_cast<int>(input.IsKeyPressed(KeyCode::E));
    upStep -= static_cast<int>(input.IsKeyPressed(KeyCode::Q));

    Vector3f moveDirection =
        static_cast<float>(forwardStep) * camera_->GetForward() +
        static_cast<float>(leftStep)    * camera_->GetLeft() +
        static_cast<float>(upStep)      * camera_->GetUp();
    if(moveDirection != Vector3f(0, 0, 0))
    {
        moveDirection = Normalize(moveDirection);
        ret = true;
    }

    camera_->SetPosition(camera_->GetPosition() + moveSpeed_ * moveDirection * timer.GetDeltaSecondsF());

    // Rotate

    auto [radX, radY, radZ] = camera_->GetRotation();
    radX += rotateSpeed_ * input.GetCursorRelativePositionY();
    radX = std::clamp(radX, -PI / 2 + 0.01f, PI / 2 - 0.01f);
    radY -= rotateSpeed_ * input.GetCursorRelativePositionX();
    while(radY > 2 * PI)
    {
        radY -= 2 * PI;
    }
    while(radY < 0)
    {
        radY += 2 * PI;
    }
    camera_->SetRotation(Vector3f(radX, radY, radZ));

    ret |= (input.GetCursorRelativePositionY() != 0 || input.GetCursorRelativePositionX() != 0);
    return ret;
}

RTRC_END

#pragma once

#include <Core/Timer.h>
#include <RHI/Window/WindowInput.h>
#include <Rtrc/Scene/Camera/Camera.h>

RTRC_BEGIN

class FreeCameraController
{
public:

    void SetCamera(Camera &camera);
    void SetMoveSpeed(float speed);
    void SetRotateSpeed(float speed);

    // Returns true when actually updated
    bool UpdateCamera(const WindowInput &input, const Timer &timer);

private:

    float moveSpeed_ = 3.0f;
    float rotateSpeed_ = 0.0025f;
    Camera *camera_ = nullptr;
};

RTRC_END

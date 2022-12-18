#pragma once

#include <Rtrc/Renderer/Camera/Camera.h>
#include <Rtrc/Utility/Timer.h>

RTRC_BEGIN

class FreeCameraController
{
public:

    void SetCamera(Camera &camera);
    void SetMoveSpeed(float speed);
    void SetRotateSpeed(float speed);

    void UpdateCamera(const Input &input, const Timer &timer) const;

private:

    float moveSpeed_ = 3.0f;
    float rotateSpeed_ = 0.0025f;
    Camera *camera_ = nullptr;
};

RTRC_END

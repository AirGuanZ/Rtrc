#pragma once

#include <Standalone/Common.h>

class StandaloneApplication : public Rtrc::BuiltinRenderLoopApplication
{
protected:

    void InitializeLogic() override;
    void UpdateLogic() override;

private:

    Rtrc::FreeCameraController           cameraController_;
    std::vector<Box<Rtrc::MeshRenderer>> objects_;
    Box<Rtrc::Light>                     pointLight_;

    float sunAngle_ = 20.0f;
};

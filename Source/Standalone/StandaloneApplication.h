#pragma once

#include <Standalone/Common.h>

class StandaloneApplication : public Rtrc::Application
{
protected:

    void Initialize(const Rtrc::ApplicationInitializeContext &context) override;

    void Update(const Rtrc::ApplicationUpdateContext &context) override;

private:

    Rtrc::FreeCameraController        cameraController_;
    Box<Rtrc::StaticMeshRenderObject> cubeObjects_[2];
    Box<Rtrc::Light>                  pointLight_;

    float sunAngle_ = 20.0f;
};

#pragma once

#include "Common.h"

class StandaloneApplication : public Rtrc::Application
{
protected:

    void Initialize() override;
    void Update() override;
    void Destroy() override;
    void ResizeFrameBuffer(uint32_t width, uint32_t height) override;

private:

    Box<Rtrc::Scene>                                   activeScene_;
    Rtrc::Camera                                       activeCamera_;
    Box<Rtrc::Renderer::RealTimeRenderLoop>            renderLoop_;
    Rtrc::Renderer::RealTimeRenderLoop::RenderSettings activeRenderSettings_;

    Rtrc::FreeCameraController           cameraController_;
    std::vector<Box<Rtrc::MeshRenderer>> objects_;
    Box<Rtrc::Light>                     pointLight_;
    float                                sunAngle_ = 20.0f;
};

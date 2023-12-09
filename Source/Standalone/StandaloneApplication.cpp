#include <Standalone/StandaloneApplication.h>

void StandaloneApplication::InitializeLogic()
{
    auto activeCamera = GetActiveCamera();
    auto activeScene = GetActiveScene();

    activeCamera->SetPosition({ -2, 2, -5 });
    activeCamera->SetRotation({ 0.35f, 0.4f, 0 });
    activeCamera->CalculateDerivedData();

    cameraController_.SetCamera(*activeCamera);
    GetWindowInput().LockCursor(true);

    Rtrc::ResourceManager &resources = *GetResourceManager();

    {
        auto cubeMesh = resources.GetBuiltinMesh(Rtrc::BuiltinMesh::Cube);
        auto matInst = resources.CreateMaterialInstance("Surface/Diffuse");

        auto gray = GetDevice()->CreateColorTexture2D(180, 180, 180, 255, "Gray");
        auto grayHandle = GetBindlessTextureManager()->Allocate();
        grayHandle.Set(gray);

        matInst->Set("albedoTextureIndex", grayHandle);

        {
            auto object = activeScene->CreateMeshRenderer();
            object->SetMesh(cubeMesh);
            object->SetMaterial(matInst);
            object->SetFlags(Rtrc::MeshRenderer::Flags::OpaqueTlas);
            objects_.push_back(std::move(object));
        }

        {
            auto object = activeScene->CreateMeshRenderer();
            object->SetMesh(cubeMesh);
            object->SetMaterial(matInst);
            object->SetFlags(Rtrc::MeshRenderer::Flags::OpaqueTlas);
            object->GetMutableTransform().SetTranslation({ 0, 1.3f, 0 }).SetScale({ 0.3f, 0.3f, 0.3f });
            objects_.push_back(std::move(object));
        }
    }
    
    {
        pointLight_ = activeScene->CreateLight();
        pointLight_->SetType(Rtrc::Light::Type::Point);
        pointLight_->SetPosition({ 0, 2, -3 });
        pointLight_->SetColor({ 0.2f, 1, 0.3f });
        pointLight_->SetIntensity(20);
        pointLight_->SetDistFadeBegin(1.0f);
        pointLight_->SetDistFadeEnd(5.0f);
        pointLight_->SetFlags(Rtrc::Light::Flags::RayTracedShadow);
    }
}

void StandaloneApplication::UpdateLogic()
{
    Rtrc::WindowInput &input = GetWindowInput();
    Rtrc::ImGuiInstance &imgui = *GetImGuiInstance();
    if(input.IsKeyDown(Rtrc::KeyCode::Escape))
    {
        SetExitFlag(true);
    }

    if(input.IsKeyDown(Rtrc::KeyCode::F1))
    {
        input.LockCursor(!input.IsCursorLocked());
        imgui.SetInputEnabled(!input.IsCursorLocked());
    }

    if(IsGPUCapturerAvailable() && input.IsKeyDown(Rtrc::KeyCode::F10))
    {
        AddPendingCaptureFrames(1);
    }

    if(input.IsCursorLocked())
    {
        (void)cameraController_.UpdateCamera(input, GetFrameTimer());
    }
    GetActiveCamera()->SetProjection(Rtrc::Deg2Rad(60), GetWindow().GetFramebufferWOverH(), 0.1f, 100.0f);
    GetActiveCamera()->CalculateDerivedData();

    auto &renderSettings = GetRenderSettings();
    if(imgui.Begin("Rtrc Standalone Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        imgui.Text("Press F1 to show/hide cursor");
        if(IsGPUCapturerAvailable())
        {
            imgui.Text("Press F10 to capture one frame");
        }

        imgui.SliderAngle("Sun Direction", &sunAngle_, 1, 179);
        sunAngle_ = std::clamp(sunAngle_, Rtrc::Deg2Rad(1), Rtrc::Deg2Rad(179));

        if(pointLight_)
        {
            float shadowSoftness = pointLight_->GetSoftness();
            if(imgui.SliderFloat("Shadow Softness", &shadowSoftness, 0.0f, 0.999f))
            {
                pointLight_->SetSoftness(shadowSoftness);
            }
        }

        imgui.DragUInt("ReSTIR M", &renderSettings.ReSTIR_M, 1, 512);
        imgui.DragUInt("ReSTIR Max M", &renderSettings.ReSTIR_MaxM, 1, 512);
        imgui.DragUInt("ReSTIR N", &renderSettings.ReSTIR_N, 1, 512);
        imgui.DragFloat("ReSTIR Radius", &renderSettings.ReSTIR_Radius, 1, 0, 512);
        imgui.CheckBox("ReSTIR Enable Temporal Reuse", &renderSettings.ReSTIR_EnableTemporalReuse);
        imgui.DragFloat("ReSTIR SVGF Alpha", &renderSettings.ReSTIR_SVGFTemporalFilterAlpha, 1, 0, 1);
        imgui.DragUInt("ReSTIR SVGF Spatial Iterations", &renderSettings.ReSTIR_SVGFSpatialFilterIterations, 1, 10);

        imgui.CheckBox("Indirect Diffuse", &renderSettings.enableIndirectDiffuse);

        if(imgui.BeginCombo(
            "Visualization Mode",
            Rtrc::Renderer::RealTimeRenderLoop::GetVisualizationModeName(renderSettings.visualizationMode)))
        {
            for(int i = 0; i < Rtrc::EnumCount<Rtrc::Renderer::RealTimeRenderLoop::VisualizationMode>; ++i)
            {
                const auto mode = static_cast<Rtrc::Renderer::RealTimeRenderLoop::VisualizationMode>(i);
                const char *name = Rtrc::Renderer::RealTimeRenderLoop::GetVisualizationModeName(mode);
                const bool selected = renderSettings.visualizationMode == mode;
                if(imgui.Selectable(name, selected))
                {
                    renderSettings.visualizationMode = mode;
                }
            }
            imgui.EndCombo();
        }
    }
    imgui.End();

    const Rtrc::Vector3f sunDirection = -Rtrc::Vector3f(std::cos(sunAngle_), std::sin(sunAngle_), 0);
    GetActiveScene()->GetSky().SetSunDirection(sunDirection);
}

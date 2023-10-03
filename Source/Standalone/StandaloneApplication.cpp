#include <Standalone/StandaloneApplication.h>

void StandaloneApplication::Initialize(const Rtrc::ApplicationInitializeContext &context)
{
    context.activeCamera->SetPosition({ -2, 2, -5 });
    context.activeCamera->SetRotation({ 0.35f, 0.4f, 0 });
    context.activeCamera->CalculateDerivedData();

    cameraController_.SetCamera(*context.activeCamera);
    GetWindowInput().LockCursor(true);

    Rtrc::ResourceManager &resources = *context.resourceManager;

    {
        auto cubeMesh = resources.GetBuiltinMesh(Rtrc::BuiltinMesh::Cube);
        auto matInst = resources.CreateMaterialInstance("Surface/Diffuse");

        auto gray = context.device->CreateColorTexture2D(180, 180, 180, 255, "Gray");
        auto grayHandle = GetBindlessTextureManager().Allocate();
        grayHandle.Set(gray);

        matInst->Set("albedoTextureIndex", grayHandle);

        {
            auto object = context.activeScene->CreateMeshRenderer();
            object->SetMesh(cubeMesh);
            object->SetMaterial(matInst);
            object->SetFlags(Rtrc::MeshRenderer::Flags::OpaqueTlas);
            objects_.push_back(std::move(object));
        }

        {
            auto object = context.activeScene->CreateMeshRenderer();
            object->SetMesh(cubeMesh);
            object->SetMaterial(matInst);
            object->SetFlags(Rtrc::MeshRenderer::Flags::OpaqueTlas);
            object->GetMutableTransform().SetTranslation({ 0, 1.3f, 0 }).SetScale({ 0.3f, 0.3f, 0.3f });
            objects_.push_back(std::move(object));
        }
    }

    {
        pointLight_ = context.activeScene->CreateLight();
        pointLight_->SetType(Rtrc::Light::Type::Point);
        pointLight_->SetPosition({ 0, 2, -3 });
        pointLight_->SetColor({ 1, 1, 1 });
        pointLight_->SetIntensity(1);
        pointLight_->SetDistFadeBegin(1.0f);
        pointLight_->SetDistFadeEnd(5.0f);
        pointLight_->SetFlags(Rtrc::Light::Flags::RayTracedShadow);
    }
}

void StandaloneApplication::Update(const Rtrc::ApplicationUpdateContext &context)
{
    Rtrc::WindowInput &input = GetWindowInput();
    Rtrc::ImGuiInstance &imgui = *context.imgui;
    if(input.IsKeyDown(Rtrc::KeyCode::Escape))
    {
        SetExitFlag(true);
    }

    if(input.IsKeyDown(Rtrc::KeyCode::F1))
    {
        input.LockCursor(!input.IsCursorLocked());
        imgui.SetInputEnabled(!input.IsCursorLocked());
    }

    if(input.IsCursorLocked())
    {
        (void)cameraController_.UpdateCamera(input, *context.frameTimer);
    }
    context.activeCamera->SetProjection(Rtrc::Deg2Rad(60), GetWindow().GetFramebufferWOverH(), 0.1f, 100.0f);
    context.activeCamera->CalculateDerivedData();

    if(imgui.Begin("Rtrc Standalone Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        imgui.Text("Press F1 to show/hide cursor");
        imgui.SliderAngle("Sun Direction", &sunAngle_, 1, 179);
        sunAngle_ = std::clamp(sunAngle_, Rtrc::Deg2Rad(1), Rtrc::Deg2Rad(179));

        float shadowSoftness = pointLight_->GetSoftness();
        if(imgui.SliderFloat("Shadow Softness", &shadowSoftness, 0.0f, 0.999f))
        {
            pointLight_->SetSoftness(shadowSoftness);
        }

        imgui.CheckBox("Indirect Diffuse", &GetRenderSettings().enableIndirectDiffuse);

        if(imgui.BeginCombo("Visualization Mode", GetVisualizationModeName(GetRenderSettings().visualizationMode)))
        {
            for(int i = 0; i < Rtrc::EnumCount<Rtrc::Renderer::VisualizationMode>; ++i)
            {
                const auto mode = static_cast<Rtrc::Renderer::VisualizationMode>(i);
                const char *name = GetVisualizationModeName(mode);
                const bool selected = GetRenderSettings().visualizationMode == mode;
                if(imgui.Selectable(name, selected))
                {
                    GetRenderSettings().visualizationMode = mode;
                }
            }
            imgui.EndCombo();
        }
    }
    imgui.End();

    const Rtrc::Vector3f sunDirection = -Rtrc::Vector3f(std::cos(sunAngle_), std::sin(sunAngle_), 0);
    context.activeScene->GetSky().SetSunDirection(sunDirection);
}

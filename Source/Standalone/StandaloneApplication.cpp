#include <Standalone/StandaloneApplication.h>

void StandaloneApplication::Initialize(const Rtrc::ApplicationInitializeContext &context)
{
    context.activeCamera->SetPosition({ -2, 2, -5 });
    context.activeCamera->SetRotation({ 0.35f, 0.4f, 0 });
    context.activeCamera->CalculateDerivedData();

    cameraController_.SetCamera(*context.activeCamera);
    GetWindowInput().LockCursor(true);

    Rtrc::ResourceManager &resources = *context.resourceManager;
    resources.AddMaterialFiles($rtrc_get_files("Asset/Builtin/*/*.*"));

    {
        auto cubeMesh = resources.GetBuiltinResources().GetBuiltinMesh(Rtrc::BuiltinMesh::Cube);
        auto matInst = resources.CreateMaterialInstance("Builtin/Diffuse");
        auto skyBlue = context.device->CreateColorTexture2D(0, 255, 255, 255, "SkyBlue");
        matInst->Set("Albedo", skyBlue);

        cubeObjects_[0] = context.activeScene->CreateStaticMeshRenderer();
        cubeObjects_[0]->SetMesh(cubeMesh);
        cubeObjects_[0]->SetMaterial(matInst);
        cubeObjects_[0]->SetRayTracingFlags(Rtrc::StaticMeshRenderObject::RayTracingFlags::None);

        cubeObjects_[1] = context.activeScene->CreateStaticMeshRenderer();
        cubeObjects_[1]->SetMesh(cubeMesh);
        cubeObjects_[1]->SetMaterial(matInst);
        cubeObjects_[1]->SetRayTracingFlags(Rtrc::StaticMeshRenderObject::RayTracingFlags::None);
        cubeObjects_[1]->GetMutableTransform().SetTranslation({ 0, 1.3f, 0 }).SetScale({ 0.3f, 0.3f, 0.3f });
    }

    {
        pointLight_ = context.activeScene->CreateLight();
        pointLight_->SetType(Rtrc::Light::Type::Point);
        pointLight_->SetPosition({ 0, 2, -2 });
        pointLight_->SetColor({ 1, 1, 1 });
        pointLight_->SetIntensity(1);
        pointLight_->SetRange(3.0f);
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
    }
    imgui.End();

    const Rtrc::Vector3f sunDirection = -Rtrc::Vector3f(std::cos(sunAngle_), std::sin(sunAngle_), 0);
    context.activeScene->SetAtmosphereSunDirection(sunDirection);
}

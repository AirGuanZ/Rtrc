#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

class Application : public Uncopyable
{
    Window window_;
    Input *input_ = nullptr;
    
    Box<Device>       device_;
    Box<RG::Executer> executer_;

    Box<ImGuiInstance> imgui_;

    Box<MaterialManager> materials_;
    Box<MeshLoader>      meshes_;

    Box<BuiltinResourceManager> builtinResources_;
    Box<Renderer> renderer_;

    RC<Light> mainLight_;
    Box<Scene> scene_;
    Box<Camera> camera_;
    FreeCameraController cameraController_;

    Box<AtmosphereRenderer> atmosphere_;
    float sunAngle_ = Deg2Rad(179);

    Timer timer_;
    int fps_ = 0;

    void Initialize()
    {
        window_ = WindowBuilder()
            .SetSize(800, 800)
            .SetTitle("Rtrc Renderer")
            .SetMaximized(true)
            .Create();
        input_ = &window_.GetInput();

        device_ = Device::CreateGraphicsDevice(window_, RHI::Format::B8G8R8A8_UNorm, 3, RTRC_DEBUG, false);
        executer_ = MakeBox<RG::Executer>(device_.get());

        imgui_ = MakeBox<ImGuiInstance>(*device_, window_);

        materials_ = MakeBox<MaterialManager>();
        materials_->SetDevice(device_.get());
        materials_->SetRootDirectory("Asset");

        meshes_ = MakeBox<MeshLoader>();
        meshes_->SetCopyContext(&device_->GetCopyContext());
        meshes_->SetRootDirectory("Asset");

        builtinResources_ = MakeBox<BuiltinResourceManager>(*device_);
        renderer_ = MakeBox<Renderer>(*device_, *builtinResources_);

        scene_ = MakeBox<Scene>();
        camera_ = MakeBox<Camera>();
        camera_->SetPosition(Vector3f(-2, 2, -5));
        camera_->SetRotation(Vector3f(0.35, 0.4, 0));
        camera_->CalculateDerivedData();
        cameraController_.SetCamera(*camera_);

        atmosphere_ = MakeBox<AtmosphereRenderer>(*builtinResources_);
        atmosphere_->SetYOffset(2 * 1000);

        {
            auto cubeMesh = builtinResources_->GetBuiltinMesh(BuiltinMesh::Cube);
            auto matInst = materials_->CreateMaterialInstance("Builtin/Surface/Diffuse");
            matInst->Set("Albedo", device_->CreateColorTexture2D(0, 255, 255, 255));
            
            auto mesh = MakeRC<StaticMesh>();
            mesh->SetMesh(cubeMesh);
            mesh->SetMaterial(matInst);
            mesh->UpdateWorldMatrixRecursively(true);
            scene_->AddMesh(std::move(mesh));
        }

        {
            mainLight_ = MakeRC<Light>();
            mainLight_->SetType(Light::Type::Directional);
            mainLight_->GetDirectionalData().direction = Normalize(Vector3f(0.4, -1, 0.2));
            mainLight_->SetColor(Vector3f(0, 1, 1));
            mainLight_->SetIntensity(1);
            scene_->AddLight(mainLight_);
        }

        input_->LockCursor(true);
        imgui_->SetInputEnabled(false);

        window_.SetFocus();
        timer_.Restart();
    }

    void Frame()
    {
        // Input
        
        if(timer_.GetFps() != fps_)
        {
            fps_ = timer_.GetFps();
            window_.SetTitle(std::format("Rtrc Renderer. FPS: {}", timer_.GetFps()));
        }

        if(window_.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window_.SetCloseFlag(true);
        }

        if(input_->IsKeyDown(KeyCode::F1))
        {
            input_->LockCursor(!input_->IsCursorLocked());
            imgui_->SetInputEnabled(!input_->IsCursorLocked()); // Only enable gui input when cursor is not locked
        }

        auto &imgui = *imgui_;
        if(imgui.Begin("Rtrc Demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            imgui.TextUnformatted("F1     : Show/Hide Cursor");
            imgui.TextUnformatted("Escape : Exit");
            imgui.SliderAngle("Sun Direction", &sunAngle_, 1, 179, nullptr);
            sunAngle_ = std::clamp(sunAngle_, Deg2Rad(1), Deg2Rad(179));
        }
        imgui.End();

        // Camera

        const float wOverH = static_cast<float>(window_.GetFramebufferSize().x) / window_.GetFramebufferSize().y;
        camera_->SetProjection(Deg2Rad(60), wOverH, 0.1f, 100.0f);
        if(input_->IsCursorLocked())
        {
            cameraController_.UpdateCamera(*input_, timer_);
        }
        camera_->CalculateDerivedData();

        // Render graph

        Box<RG::RenderGraph> rg = device_->CreateRenderGraph();
        RG::TextureResource *renderTarget = rg->RegisterSwapchainTexture(device_->GetSwapchain());

        // Atmosphere

        {
            const Vector3f direction = -Vector3f(std::cos(sunAngle_), std::sin(sunAngle_), 0);
            atmosphere_->SetSunDirection(direction);
            mainLight_->GetDirectionalData().direction = direction;
        }
        const auto atmosphereRGData = atmosphere_->AddToRenderGraph(*rg, *camera_);

        // Deferred rendering

        Renderer::Parameters renderParameters;
        renderParameters.skyLut = atmosphereRGData.skyLut;
        const auto deferredRendererRGData = renderer_->AddToRenderGraph(
            renderParameters, rg.get(), renderTarget, *scene_, *camera_);

        // ImGui

        auto imguiPass = imgui_->AddToRenderGraph(renderTarget, rg.get());

        // Dependencies

        Connect(atmosphereRGData.outPass, deferredRendererRGData.inPass);
        Connect(deferredRendererRGData.outPass, imguiPass);
        imguiPass->SetSignalFence(device_->GetFrameFence());

        // Execute render graph & present

        executer_->Execute(rg);
        device_->Present();
    }

public:

    ~Application()
    {
        if(input_)
        {
            input_->LockCursor(false);
        }
    }

    void Run()
    {
        Initialize();
        device_->BeginRenderLoop();
        while(!window_.ShouldClose())
        {
            if(!device_->BeginFrame())
            {
                continue;
            }
            timer_.BeginFrame();
            imgui_->BeginFrame();
            Frame();
        }
        device_->EndRenderLoop();
    }
};

int main()
{
    EnableMemoryLeakReporter();
    try
    {
        Application().Run();
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}

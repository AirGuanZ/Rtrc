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

    Box<Scene> scene_;
    Box<Camera> camera_;
    FreeCameraController cameraController_;

    Box<AtmosphereRenderer> atmosphere_;

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
        atmosphere_->SetYOffset(256);

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
            auto light = MakeRC<Light>();
            light->SetType(Light::Type::Directional);
            light->GetDirectionalData().direction = Normalize(Vector3f(0.4, -1, 0.2));
            light->SetColor(Vector3f(0, 1, 1));
            light->SetIntensity(1);
            scene_->AddLight(light);
        }

        input_->LockCursor(true);
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

        if(input_->IsKeyDown(KeyCode::LeftCtrl))
        {
            input_->LockCursor(!input_->IsCursorLocked());
        }

        if(imgui_->Begin("Test Window"))
        {
            imgui_->Button("Test Button");
            imgui_->Text("Test Formatted text: {}", 2.5f);
        }
        imgui_->End();

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
            timer_.BeginFrame();
            if(!device_->BeginFrame())
            {
                continue;
            }
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

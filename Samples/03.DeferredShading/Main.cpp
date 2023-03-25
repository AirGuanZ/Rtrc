#include <Rtrc/Rtrc.h>

using namespace Rtrc;

class DeferredRendererApplication : public Uncopyable
{
    Window window_;
    WindowInput *input_ = nullptr;
    
    Box<Device>       device_;
    Box<RG::Executer> executer_;

    Box<ImGuiInstance>    imgui_;
    Box<ImGuiRenderer>    imguiRenderer_;
    Box<ResourceManager>  resources_;
    Box<Renderer::DeferredRenderer> renderer_;

    Box<Scene>                           scene_;
    std::vector<Box<StaticMeshRenderObject>> staticMeshRenderers_;
    Box<Light>                           mainLight_;

    Box<Camera>          camera_;
    FreeCameraController cameraController_;
    
    float sunAngle_ = Deg2Rad(179);

    Timer timer_;
    int lastFps_ = 0;

    void Initialize()
    {
        window_ = WindowBuilder()
            .SetSize(800, 800)
            .SetTitle("Rtrc Renderer")
            .SetMaximized(true)
            .Create();
        input_ = &window_.GetInput();

        device_ = Device::CreateGraphicsDevice(window_, RHI::Format::B8G8R8A8_UNorm, 3, RTRC_DEBUG, false);
        executer_ = MakeBox<RG::Executer>(device_);

        imgui_ = MakeBox<ImGuiInstance>(device_, window_);
        imguiRenderer_ = MakeBox<ImGuiRenderer>(*device_);

        resources_ = MakeBox<ResourceManager>(device_);
        resources_->AddMaterialFiles($rtrc_get_files("Asset/Builtin/*/*.*"));
        resources_->AddShaderIncludeDirectory("Asset");
        
        renderer_ = MakeBox<Renderer::DeferredRenderer>(*device_, resources_->GetBuiltinResources());

        scene_ = MakeBox<Scene>();
        camera_ = MakeBox<Camera>();
        camera_->SetPosition(Vector3f(-2, 2, -5));
        camera_->SetRotation(Vector3f(0.35, 0.4, 0));
        camera_->CalculateDerivedData();
        cameraController_.SetCamera(*camera_);

        renderer_->GetAtmosphereRenderer().SetYOffset(2 * 1000);

        {
            auto cubeMesh = resources_->GetBuiltinResources().GetBuiltinMesh(BuiltinMesh::Cube);
            auto matInst = resources_->CreateMaterialInstance("Builtin/Surface/Diffuse");
            matInst->Set("Albedo", device_->CreateColorTexture2D(0, 255, 255, 255));

            auto staticMeshRenderer = staticMeshRenderers_.emplace_back(scene_->CreateStaticMeshRenderer()).get();
            staticMeshRenderer->SetMesh(cubeMesh);
            staticMeshRenderer->SetMaterial(matInst);
            staticMeshRenderer->UpdateWorldMatrixRecursively(true);
        }

        {
            mainLight_ = scene_->CreateLight();
            mainLight_->SetType(Light::Type::Directional);
            mainLight_->SetDirection(Normalize(Vector3f(0.4, -1, 0.2)));
            mainLight_->SetColor(Vector3f(1));
            mainLight_->SetIntensity(1);
        }

        input_->LockCursor(true);
        imgui_->SetInputEnabled(false);

        window_.SetFocus();
        timer_.Restart();
    }

    void Frame()
    {
        // Input
        
        if(timer_.GetFps() != lastFps_)
        {
            lastFps_ = timer_.GetFps();
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

        {
            const float wOverH = window_.GetFramebufferWOverH();
            camera_->SetProjection(Deg2Rad(60), wOverH, 0.1f, 100.0f);
            if(input_->IsCursorLocked())
            {
                (void)cameraController_.UpdateCamera(*input_, timer_);
            }
            camera_->CalculateDerivedData();
        }

        // Atmosphere

        {
            const Vector3f direction = -Vector3f(std::cos(sunAngle_), std::sin(sunAngle_), 0);
            renderer_->GetAtmosphereRenderer().SetSunDirection(direction);
            mainLight_->SetDirection(direction);
        }

        // Render graph

        Box<RG::RenderGraph> graph = device_->CreateRenderGraph();
        RG::TextureResource *renderTarget = graph->RegisterSwapchainTexture(device_->GetSwapchain());
        
        // Deferred rendering

        Box<SceneProxy> sceneProxy = scene_->CreateSceneProxy();
        
        const auto deferredRendererRGData = renderer_->AddToRenderGraph(
            *graph, renderTarget, *sceneProxy, *camera_, timer_.GetDeltaSecondsF());

        // ImGui

        auto imguiRenderData = imgui_->Render();
        auto imguiPass = imguiRenderer_->AddToRenderGraph(imguiRenderData.get(), renderTarget, graph.get());

        //auto imguiPass = imgui_->AddToRenderGraph(renderTarget, graph.get());

        // Dependencies
        
        Connect(deferredRendererRGData.passOut, imguiPass);
        imguiPass->SetSignalFence(device_->GetFrameFence());

        // Execute render graph & present

        executer_->Execute(graph);
        device_->Present();
    }

public:

    ~DeferredRendererApplication()
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
        DeferredRendererApplication().Run();
    }
    catch(const Exception &e)
    {
        LogError("{}\n{}", e.what(), e.stacktrace());
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}

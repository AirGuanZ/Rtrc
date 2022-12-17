#include <iostream>

#include <Rtrc/Rtrc.h>

using namespace Rtrc;

class Application : public Uncopyable
{
    Window window_;
    Input *input_ = nullptr;
    
    Box<Device>       device_;
    Box<RG::Executer> executer_;

    Box<MaterialManager> materials_;
    Box<MeshLoader>      meshes_;

    Box<BuiltinResourceManager> builtinResources_;

    Box<Scene> scene_;
    Box<Camera> camera_;
    FreeCameraController cameraController_;

    void Initialize()
    {
        window_ = WindowBuilder()
            .SetSize(800, 800)
            .SetTitle("Rtrc Sample: Deferred Shading")
            .Create();
        input_ = &window_.GetInput();
        device_ = Device::CreateGraphicsDevice(window_);
        executer_ = MakeBox<RG::Executer>(device_.get());

        materials_ = MakeBox<MaterialManager>();
        materials_->SetDevice(device_.get());
        materials_->SetRootDirectory("Asset");

        meshes_ = MakeBox<MeshLoader>();
        meshes_->SetCopyContext(&device_->GetCopyContext());
        meshes_->SetRootDirectory("Asset");

        builtinResources_ = MakeBox<BuiltinResourceManager>(*device_);

        scene_ = MakeBox<Scene>();
        camera_ = MakeBox<Camera>();
        camera_->SetPosition(Vector3f(-2, 2, -5));
        camera_->SetRotation(Vector3f(0.35, 0.4, 0));
        camera_->CalculateDerivedData();
        cameraController_.SetCamera(*camera_);

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
    }

    void Frame()
    {
        if(window_.GetInput().IsKeyDown(KeyCode::Escape))
        {
            window_.SetCloseFlag(true);
        }

        const float wOverH = static_cast<float>(window_.GetFramebufferSize().x) / window_.GetFramebufferSize().y;
        camera_->SetProjection(Deg2Rad(60), wOverH, 0.1f, 100.0f);
        cameraController_.UpdateCamera(*input_);

        auto renderer = MakeBox<DeferredRenderer>(*device_, *builtinResources_);

        auto rg = device_->CreateRenderGraph();
        auto renderTarget = rg->RegisterSwapchainTexture(device_->GetSwapchain());
        auto deferredRendererRGData = renderer->AddToRenderGraph(rg.get(), renderTarget, *scene_, *camera_);
        deferredRendererRGData.outPass->SetSignalFence(device_->GetFrameFence());

        executer_->Execute(rg);
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
            Frame();
            device_->Present();
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

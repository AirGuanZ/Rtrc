//#pragma once
//
//#include <Rtrc/Graphics/Device/PerObjectBindingGroup.h>
//#include <Rtrc/Graphics/Pipeline/SubMaterialToGraphicsPipeline.h>
//#include <Rtrc/Graphics/RenderGraph/Graph.h>
//#include <Rtrc/Renderer/Camera.h>
//#include <Rtrc/Renderer/Scene.h>
//
//RTRC_BEGIN
//
//class DeferredRenderer : public Uncopyable
//{
//public:
//
//    DeferredRenderer(Device &device, MaterialManager &materialManager);
//
//    void Render(
//        RG::RenderGraph     *renderGraph,
//        RG::TextureResource *renderTarget,
//        const Scene         &scene,
//        const RenderCamera  &camera);
//
//private:
//
//    void DoRenderGBuffersPass(RG::PassContext &passContext);
//
//    void DoDeferredLightingPass(RG::PassContext &passContext);
//
//    Device &device_;
//    MaterialManager &materialManager_;
//
//    RG::RenderGraph     *renderGraph_  = nullptr;
//    RG::TextureResource *renderTarget_ = nullptr;
//
//    const Scene        *scene_  = nullptr;
//    const RenderCamera *camera_ = nullptr;
//
//    RG::TextureResource *normal_         = nullptr;
//    RG::TextureResource *albedoMetallic_ = nullptr;
//    RG::TextureResource *roughness_      = nullptr;
//    RG::TextureResource *depth_          = nullptr;
//
//    SubMaterialToGraphicsPipeline renderGBuffersPipelines_;
//    RC<GraphicsPipeline> deferredLightingPipeline_;
//
//    PerObjectBindingGroupPool staticMeshPerObjectBindingGroupPool_;
//};
//
//RTRC_END

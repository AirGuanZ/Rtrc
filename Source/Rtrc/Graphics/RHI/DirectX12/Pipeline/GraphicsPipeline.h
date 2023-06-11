#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/BindingLayout.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12GraphicsPipeline, GraphicsPipeline)
{
public:

    RTRC_D3D12_IMPL_SET_NAME(pipelineState_)

    DirectX12GraphicsPipeline(
        Ptr<BindingLayout>            bindingLayout,
        ComPtr<ID3D12RootSignature>   rootSignature,
        ComPtr<ID3D12PipelineState>   pipelineState,
        std::optional<Span<Viewport>> staticViewports,
        std::optional<Span<Scissor>>  staticScissors,
        std::vector<size_t>           vertexStrides,
        D3D12_PRIMITIVE_TOPOLOGY      topology)
        : bindingLayout_(std::move(bindingLayout))
        , rootSignature_(std::move(rootSignature))
        , pipelineState_(std::move(pipelineState))
        , vertexStrides_(std::move(vertexStrides))
        , topology_     (topology)
    {
        if(staticViewports)
        {
            staticViewports_.emplace();
            staticViewports_->reserve(staticViewports->size());
            std::ranges::transform(
                *staticViewports, std::back_inserter(*staticViewports_),
                [](const Viewport &viewport) { return TranslateViewport(viewport); });
        }

        if(staticScissors)
        {
            staticScissors_.emplace();
            staticScissors_->reserve(staticScissors->size());
            std::ranges::transform(
                *staticScissors, std::back_inserter(*staticScissors_),
                [](const Scissor &scissor) { return TranslateScissor(scissor); });
        }
    }

    const Ptr<BindingLayout> &GetBindingLayout() const RTRC_RHI_OVERRIDE
    {
        return bindingLayout_;
    }

    const ComPtr<ID3D12PipelineState> &_internalGetNativePipelineState() const
    {
        return pipelineState_;
    }

    const auto &_internalGetStaticViewports() const { return staticViewports_; }
    const auto &_internalGetStaticScissors() const { return staticScissors_; }
    const auto &_internalGetVertexStrides() const { return vertexStrides_; }
    const auto &_internalGetRootSignature() const { return rootSignature_; }
    auto _internalGetTopology() const { return topology_; }

private:

    Ptr<BindingLayout>          bindingLayout_;
    ComPtr<ID3D12RootSignature> rootSignature_;
    ComPtr<ID3D12PipelineState> pipelineState_;

    std::optional<std::vector<D3D12_VIEWPORT>> staticViewports_;
    std::optional<std::vector<D3D12_RECT>>     staticScissors_;
    std::vector<size_t>                        vertexStrides_;
    D3D12_PRIMITIVE_TOPOLOGY                   topology_;
};

RTRC_RHI_D3D12_END

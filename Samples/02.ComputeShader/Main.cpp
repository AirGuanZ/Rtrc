#include <Rtrc/Rtrc.h>

using namespace Rtrc;

$struct_begin(ScaleSetting)
    $variable(float, Factor)
$struct_end()

$group_begin(ScaleGroup)
    $binding(ConstantBuffer<ScaleSetting>, ScaleSettingCBuffer, CS)
    $binding(Texture2D<float4>,            InputTexture,        CS)
    $binding(RWTexture2D<float4>,          OutputTexture,       CS)
$group_end()

void Run()
{
    auto instance = CreateVulkanInstance(RHI::VulkanInstanceDesc{
        .debugMode = true
    });

    auto device = instance->CreateDevice(RHI::DeviceDesc{
        .graphicsQueue    = false,
        .computeQueue     = true,
        .transferQueue    = false,
        .supportSwapchain = false
    });

    const std::string shaderSource = File::ReadTextFile("Asset/02.ComputeShader/Shader.hlsl");

    auto computeShader = ShaderCompiler()
        .SetComputeShaderSource(shaderSource, "CSMain")
        .SetDebugMode(true)
        .SetTarget(ShaderCompiler::Target::Vulkan)
        .Compile(*device)->GetComputeShader();

    auto bindingGroupLayout = device->CreateBindingGroupLayout<ScaleGroup>();
    auto bindingLayout = device->CreateBindingLayout(RHI::BindingLayoutDesc{
        .groups = { bindingGroupLayout }
    });

    auto pipeline = (*device->CreateComputePipelineBuilder())
        .SetComputeShader(computeShader)
        .SetBindingLayout(bindingLayout)
        .CreatePipeline();


}

int main()
{
    
}

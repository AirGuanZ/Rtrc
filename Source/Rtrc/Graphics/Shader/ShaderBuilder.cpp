#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/ShaderBuilder.h>

RTRC_BEGIN

RC<Shader> ShaderBuilder::BuildShader(Ref<Device> device, Desc desc)
{
    auto shaderGroupInfo = MakeRC<ShaderGroupInfo>();
    shaderGroupInfo->rayGenShaderGroups_ = std::move(desc.rayGenShaderGroups);
    shaderGroupInfo->missShaderGroups_   = std::move(desc.missShaderGroups);
    shaderGroupInfo->hitShaderGroups_    = std::move(desc.hitShaderGroups);

    auto shaderBindingLayoutInfo = MakeRC<ShaderBindingLayoutInfo>();
    shaderBindingLayoutInfo->nameToBindingGroupLayoutIndex_ = std::move(desc.nameToBindingGroupLayoutIndex);
    shaderBindingLayoutInfo->bindingGroupLayouts_           = std::move(desc.bindingGroupLayouts);
    shaderBindingLayoutInfo->bindingGroupNames_             = std::move(desc.bindingGroupNames);
    shaderBindingLayoutInfo->bindingLayout_                 = std::move(desc.bindingLayout);
    shaderBindingLayoutInfo->bindingNameMap_                = std::move(desc.bindingNameMap);
    shaderBindingLayoutInfo->bindingGroupForInlineSamplers_ = std::move(desc.bindingGroupForInlineSamplers);
    shaderBindingLayoutInfo->pushConstantRanges_            = std::move(desc.pushConstantRanges);
    std::memcpy(
        shaderBindingLayoutInfo->builtinBindingGroupIndices_,
        desc.builtinBindingGroupIndices,
        sizeof(desc.builtinBindingGroupIndices));

    auto shaderInfo = MakeRC<ShaderInfo>();
    shaderInfo->category_                     = desc.category;
    shaderInfo->VSInput_                      = std::move(desc.VSInput);
    shaderInfo->computeShaderThreadGroupSize_ = desc.computeShaderThreadGroupSize;
    shaderInfo->shaderGroupInfo_              = std::move(shaderGroupInfo);
    shaderInfo->shaderBindingLayoutInfo_      = std::move(shaderBindingLayoutInfo);

    auto shader = MakeRC<Shader>();
    switch(shaderInfo->category_)
    {
    case ShaderCategory::ClassicalGraphics:
        shader->rawShaders_[Shader::VS_INDEX] = std::move(desc.vertexShader);
        shader->rawShaders_[Shader::FS_INDEX] = std::move(desc.fragmentShader);
        break;
    case ShaderCategory::Compute:
        shader->rawShaders_[Shader::CS_INDEX] = std::move(desc.computeShader);
        break;
    case ShaderCategory::RayTracing:
        shader->rawShaders_[Shader::RT_INDEX] = std::move(desc.rayTracingShader);
        break;
    case ShaderCategory::MeshGraphics:
        shader->rawShaders_[Shader::TS_INDEX] = std::move(desc.taskShader);
        shader->rawShaders_[Shader::MS_INDEX] = std::move(desc.meshShader);
        shader->rawShaders_[Shader::FS_INDEX] = std::move(desc.fragmentShader);
        break;
    }
    shader->info_ = std::move(shaderInfo);

    if(desc.category == ShaderCategory::Compute)
    {
        shader->computePipeline_ = device->CreateComputePipeline(shader);
    }

    return shader;
}

RTRC_END

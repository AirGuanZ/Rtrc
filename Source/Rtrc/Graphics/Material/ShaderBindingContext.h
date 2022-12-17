#pragma once

#include <Rtrc/Graphics/Device/BindingGroup.h>

RTRC_BEGIN

class CommandBuffer;
class Shader;

class ShaderBindingContext
{
public:

    void Clear();
    void Set(std::string name, RC<BindingGroup> bindingGroup);
    const RC<BindingGroup> &Get(std::string_view name) const;

    void ApplyGroupsToGraphicsPipeline(const CommandBuffer &commandBuffer, const RC<Shader> &shader);
    void ApplyGroupsToComputePipieline(const CommandBuffer &commandBuffer, const RC<Shader> &shader);

private:

    struct Record
    {
        RC<BindingGroup> group;
    };

    void ApplyGroupsToGraphicsPipeline(const RHI::CommandBufferPtr &commandBuffer, const RC<Shader> &shader);
    void ApplyGroupsToComputePipieline(const RHI::CommandBufferPtr &commandBuffer, const RC<Shader> &shader);
    
    std::map<std::string, Record, std::less<>> nameToRecord_;
};

RTRC_END

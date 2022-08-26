#pragma once

#include <optional>
#include <vector>

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

namespace ShaderBindingGroupRewrite
{

    struct Binding
    {
        std::string name;
        RHI::ShaderStageFlag stages;
    };

    using AliasedBindingNames = std::vector<Binding>;

    struct ParsedBindingGroup
    {
        std::string name;
        std::vector<AliasedBindingNames> bindings;
    };

    bool RewriteNextBindingGroup(std::string &source, ParsedBindingGroup &bindings);

} // namespace ShaderBindingGroupRewrite

namespace ShaderBindingParse
{
    struct ParsedBinding
    {
        size_t                  begPosInSource;
        size_t                  endPosInSource;
        RHI::BindingType        type;
        std::string             name;
        std::optional<uint32_t> arraySize;
    };

    void ParseBindings(std::string &source, std::vector<ParsedBinding> &bindings);

} // namespace ShaderBindingParse

RTRC_END

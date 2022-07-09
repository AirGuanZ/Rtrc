#pragma once

#include <Rtrc/Shader/Binding.h>

RTRC_BEGIN

std::string GenerateHLSLBindingDeclarationForVulkan(
    std::set<std::string>        &generatedStructName,
    const BindingGroupLayoutInfo &info,
    int                           setIndex);

RTRC_END

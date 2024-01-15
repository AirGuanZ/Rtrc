#pragma once

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Graphics/Shader/Compiler.h>
#include <Rtrc/Graphics/Shader/LocalShaderCache.h>
#include <Rtrc/Graphics/Shader/ShaderDatabase.h>

RTRC_BEGIN

class ShaderManager : public Uncopyable
{
public:

    ShaderManager();

    void SetDevice(Ref<Device> device);
    void SetDebug(bool debug);

    RC<ShaderTemplate> GetShaderTemplate(std::string_view name, bool persistent = true);
    RC<Shader>         GetShader        (std::string_view name, bool persistent = true);

    template<TemplateStringParameter Name> RC<ShaderTemplate> GetShaderTemplate();
    template<TemplateStringParameter Name> RC<Shader>         GetShader();

private:

    Ref<Device>      device_;
    ShaderDatabase   shaderDatabase_;
    LocalShaderCache localShaderCache_;
};

template<TemplateStringParameter Name>
RC<ShaderTemplate> ShaderManager::GetShaderTemplate()
{
    static LocalCachedShaderStorage storage;
    return localShaderCache_.Get(&storage, Name.GetString());
}

template<TemplateStringParameter Name>
RC<Shader> ShaderManager::GetShader()
{
    return GetShaderTemplate<Name>()->GetVariant(true);
}

RTRC_END

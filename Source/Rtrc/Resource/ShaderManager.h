#pragma once

#include <Core/SmartPointer/ObserverPtr.h>
#include <Graphics/Shader/Compiler.h>
#include <Graphics/Shader/ShaderDatabase.h>
#include <Rtrc/Resource/LocalCache/LocalShaderCache.h>

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

    Ref<Device> device_;
    ShaderDatabase      shaderDatabase_;
    LocalShaderCache    localShaderCache_;
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

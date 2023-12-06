#include <Rtrc/Resource/ShaderManager.h>

RTRC_BEGIN

ShaderManager::ShaderManager()
    : localShaderCache_(this)
{
    const auto workDir = absolute(std::filesystem::current_path()).lexically_normal();
    shaderDatabase_.AddIncludeDirectory(ReflectedStruct::GetGeneratedFilePath());
    shaderDatabase_.AddIncludeDirectory((workDir / "Source").string());
}

void ShaderManager::SetDevice(ObserverPtr<Device> device)
{
    assert(!device_);
    device_ = device;
    shaderDatabase_.SetDevice(device);
}

void ShaderManager::SetDebug(bool debug)
{
    shaderDatabase_.SetDebug(debug);
}

RC<ShaderTemplate> ShaderManager::GetShaderTemplate(std::string_view name, bool persistent)
{
    return shaderDatabase_.GetShaderTemplate(name, persistent);
}

RC<Shader> ShaderManager::GetShader(std::string_view name, bool persistent)
{
    return shaderDatabase_.GetShaderTemplate(name, persistent)->GetVariant(persistent);
}

RTRC_END

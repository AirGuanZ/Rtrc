#pragma once

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Resource/ConstantBuffer.h>

RTRC_BEGIN

class MaterialPropertySheet
{
public:

    explicit MaterialPropertySheet(RC<MaterialPropertyHostLayout> layout);

    void Set(std::string_view name, float value);
    void Set(std::string_view name, const Vector2f &value);
    void Set(std::string_view name, const Vector3f &value);
    void Set(std::string_view name, const Vector4f &value);

    void Set(std::string_view name, uint32_t value);
    void Set(std::string_view name, const Vector2u &value);
    void Set(std::string_view name, const Vector3u &value);
    void Set(std::string_view name, const Vector4u &value);

    void Set(std::string_view name, int32_t value);
    void Set(std::string_view name, const Vector2i &value);
    void Set(std::string_view name, const Vector3i &value);
    void Set(std::string_view name, const Vector4i &value);

    void Set(std::string_view name, RHI::TextureSRVPtr srv);
    void Set(std::string_view name, RHI::BufferSRVPtr srv);

private:

    RC<MaterialPropertyHostLayout> layout_;
    std::vector<unsigned char> valueBuffer_;
    std::vector<RHI::RHIObjectPtr> resources_;
};

class SubMaterialInstance
{
public:

    RC<SubMaterial> subMaterial_;
    RC<BindingGroup> bindingGroup_;
    Box<FrameConstantBuffer> constantBuffer_;
};

class MaterialInstance
{
public:



private:

    struct Record
    {
        int lastUsingFrame;
        RC<Shader> shader;
        Box<FrameConstantBuffer> constantBuffer;
        RC<BindingGroup> bindingGroup;
    };

    RC<Material> parentMaterial_;
    Box<MaterialPropertySheet> properties_;
    std::map<KeywordSet::ValueMask, Record> records_;
};

RTRC_END

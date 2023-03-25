#pragma once

#include <Rtrc/Graphics/RHI/RHI.h>
#include <Rtrc/Graphics/Mesh/MeshLayout.h>

struct SpvReflectShaderModule;

RTRC_BEGIN

struct ShaderIOVar
{
    VertexSemantic semantic;
    RHI::VertexAttributeType type;
    bool isBuiltin;
    int location;
};

struct ShaderStruct
{
    struct Member
    {
        enum class ElementType
        {
            Int,
            Float,
            Struct,
        };

        enum class MatrixType
        {
            Scalar,
            Vector,
            Matrix
        };

        ElementType elementType = ElementType::Float;
        MatrixType matrixType = MatrixType::Scalar;

        std::string name;
        GeneralPooledString pooledName;

        int rowSize = 0; // valid when matrixType == Vector/Matrix
        int colSize = 0; // valid when matrixType == Matrix
        StaticVector<int, 16> arraySizes;
        Box<ShaderStruct> structInfo; // valid when elementType == ElementType::Struct

        bool operator==(const Member &rhs) const
        {
            if(std::tie(elementType, pooledName, arraySizes) == std::tie(rhs.elementType, rhs.pooledName, rhs.arraySizes))
            {
                assert(!structInfo == !rhs.structInfo);
                return structInfo ? *structInfo == *rhs.structInfo : true;
            }
            return false;
        }
    };

    std::string typeName;
    std::vector<Member> members;

    bool operator==(const ShaderStruct &) const = default;
};

struct ShaderConstantBuffer
{
    std::string name;
    int group;
    int indexInGroup;
    RC<ShaderStruct> typeInfo;

    bool operator==(const ShaderConstantBuffer &rhs) const
    {
        if(std::tie(name, group, indexInGroup) == std::tie(rhs.name, rhs.group, rhs.indexInGroup))
        {
            assert(!typeInfo == !rhs.typeInfo);
            return typeInfo ? *typeInfo == *rhs.typeInfo : true;
        }
        return false;
    }
};

class ShaderReflection
{
public:

    virtual ~ShaderReflection() = default;

    virtual std::vector<ShaderIOVar> GetInputVariables() const = 0;
    virtual std::vector<ShaderConstantBuffer> GetConstantBuffers() const = 0;
    virtual Vector3i GetComputeShaderThreadGroupSize() const = 0;
    virtual std::vector<RHI::RawShaderEntry> GetEntries() const = 0;
};

class SPIRVReflection : public ShaderReflection, public Uncopyable
{
public:

    SPIRVReflection(Span<unsigned char> code, std::string entryPoint);

    bool IsBindingUsed(std::string_view name) const;

    std::vector<ShaderIOVar> GetInputVariables() const override;
    std::vector<ShaderConstantBuffer> GetConstantBuffers() const override;
    Vector3i GetComputeShaderThreadGroupSize() const override;
    std::vector<RHI::RawShaderEntry> GetEntries() const override;

private:

    struct DeleteShaderModule
    {
        void operator()(SpvReflectShaderModule *ptr) const;
    };

    std::string entry_;
    std::set<std::string, std::less<>> usedBindings_;
    std::unique_ptr<SpvReflectShaderModule, DeleteShaderModule> shaderModule_;
};

RTRC_END

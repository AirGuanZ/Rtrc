#pragma once

#include <bitset>
#include <map>
#include <stack>

#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/Uncopyable.h>
#include <Rtrc/Core/Variant.h>
#include <Rtrc/RHI/RHI.h>

#include "Common.h"

RTRC_EDSL_BEGIN

class StructTypeSet;

class RecordContext : public Uncopyable
{
public:

    enum class BuiltinValue
    {
        GroupID,
        GroupThreadID,
        DispatchThreadID,

        Count
    };

    static const char *GetBuiltinValueName(BuiltinValue value);

    RecordContext();
    ~RecordContext();

    template<typename T>
    void RegisterStruct();

    void BeginScope();
    void EndScope();

    void AppendLine(std::string s);
    template<typename...Args>
    void AppendLine(fmt::format_string<Args...> fmtStr, Args&&...args);

    std::string AllocateVariable();
    std::string AllocateFunction();

    void SetBuiltinValueRead(BuiltinValue value);
    bool GetBuiltinValueRead(BuiltinValue value);

    uint32_t GetStaticSampler(const RHI::SamplerDesc &desc);
    const std::map<RHI::SamplerDesc, uint32_t> &GetAllStaticSamplers() const;

    std::string BuildTypeDefinitions() const;
    std::string BuildRootScope(const std::string &indent) const;

private:

    struct Scope;
    using Element = Variant<Scope, std::string>;

    struct Scope
    {
        std::vector<Element> elements;
    };

    static std::string BuildScope(const std::string &indent, const Scope &scope);

    uint32_t nextVariableNameIndex_;
    uint32_t nextFunctionNameIndex_;

    Scope rootScope_;
    std::stack<Scope *> scopeStack_;

    Box<StructTypeSet> structs_;
    std::bitset<EnumCount<BuiltinValue>> builtinValueRead_;

    std::map<RHI::SamplerDesc, uint32_t> staticSamplerToIndex_;
};

void PushRecordContext(RecordContext &context);
void PopRecordContext();
RecordContext &GetCurrentRecordContext();

template <typename... Args>
void RecordContext::AppendLine(fmt::format_string<Args...> fmtStr, Args&&... args)
{
    this->AppendLine(fmt::format(fmtStr, std::forward<Args>(args)...));
}

RTRC_EDSL_END

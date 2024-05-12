#pragma once

#include <bitset>
#include <stack>

#include <Rtrc/Core/Uncopyable.h>
#include <Rtrc/Core/Variant.h>

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

    std::string BuildResult() const;

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
};

class ScopedRecordContext : public RecordContext
{
public:

    ScopedRecordContext();
    ~ScopedRecordContext();
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

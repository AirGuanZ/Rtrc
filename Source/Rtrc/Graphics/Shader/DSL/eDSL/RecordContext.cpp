#include <cassert>

#include "../DSL.h"

RTRC_EDSL_BEGIN

namespace RecordContextDetail
{

    thread_local std::stack<RecordContext *> gRecordContextStack;

} // namespace RecordContextDetail

RecordContext::RecordContext()
    : nextVariableNameIndex_(0)
    , nextFunctionNameIndex_(0)
{
    scopeStack_.push(&rootScope_);
    structs_ = MakeBox<StructTypeSet>();
}

RecordContext::~RecordContext() = default;

void RecordContext::BeginScope()
{
    auto currentScope = scopeStack_.top();
    currentScope->elements.emplace_back(Scope{});
    scopeStack_.push(&currentScope->elements.back().As<Scope>());
}

void RecordContext::EndScope()
{
    scopeStack_.pop();
}

void RecordContext::AppendLine(std::string s)
{
    scopeStack_.top()->elements.emplace_back(std::move(s));
}

std::string RecordContext::AllocateVariable()
{
    return fmt::format("var{}", nextVariableNameIndex_++);
}

std::string RecordContext::AllocateFunction()
{
    return fmt::format("func{}", nextFunctionNameIndex_++);
}

void RecordContext::SetBuiltinValueRead(BuiltinValue value)
{
    builtinValueRead_[std::to_underlying(value)] = true;
}

bool RecordContext::GetBuiltinValueRead(BuiltinValue value)
{
    return builtinValueRead_[std::to_underlying(value)];
}

std::string RecordContext::BuildResult() const
{
    ResolvedStructTypeSet resolvedStructTypeSet;
    structs_->ResolveInto(resolvedStructTypeSet);
    return resolvedStructTypeSet.GetDefinitions() + BuildScope("", rootScope_);
}

// TODO: optimize frequent string appending
std::string RecordContext::BuildScope(const std::string &indent, const Scope& scope)
{
    std::string result;
    for(auto& element : scope.elements)
    {
        element.Match(
            [&](const std::string &s)
            {
                result += indent + s + "\n";
            },
            [&](const Scope &s)
            {
                result += indent + "{" + "\n";
                result += BuildScope(indent + "    ", s);
                result += indent + "}" + "\n";
            });
    }
    return result;
}

ScopedRecordContext::ScopedRecordContext()
{
    PushRecordContext(*this);
}

ScopedRecordContext::~ScopedRecordContext()
{
    assert(&GetCurrentRecordContext() == this);
    PopRecordContext();
}

void PushRecordContext(RecordContext &context)
{
    RecordContextDetail::gRecordContextStack.push(&context);
}

void PopRecordContext()
{
    RecordContextDetail::gRecordContextStack.pop();
}

RecordContext &GetCurrentRecordContext()
{
    return *RecordContextDetail::gRecordContextStack.top();
}

RTRC_EDSL_END

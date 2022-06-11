#include <fmt/format.h>

#include <Rtrc/Shader/DSL/Function.h>

RTRC_DSL_BEGIN

namespace
{

    std::stack<FunctionContext *> functionContexts;

} // namespace anonymous

FunctionContext::FunctionContext()
    : nextVariableIndex_(0)
{
    blocks_.push(MakeRC<Block>());
}

std::string FunctionContext::NewVariable(const std::string &type)
{
    auto name = fmt::format("var{}", nextVariableIndex_++);
    definition_.push_back(fmt::format("{} {};", type, name));
    return name;
}

void FunctionContext::AddAssignment(const std::string &left, const std::string &right)
{
    StringStatement statement;
    statement.str = fmt::format("{} = {};", left, right);
    blocks_.top()->statements.push_back(MakeRC<Statement>(std::move(statement)));
}

FunctionContext *GetFunctionContext()
{
    return functionContexts.empty() ? nullptr : functionContexts.top();
}

void PushFunctionContext(FunctionContext *context)
{
    functionContexts.push(context);
}

void PopFunctionContext()
{
    functionContexts.pop();
}

RTRC_DSL_END

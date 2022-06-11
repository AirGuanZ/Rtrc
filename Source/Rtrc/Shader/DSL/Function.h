#pragma once

#include <stack>

#include <Rtrc/Shader/DSL/Statement.h>
#include <Rtrc/Utils/Uncopyable.h>

RTRC_DSL_BEGIN

class FunctionContext : public Uncopyable
{
public:

    FunctionContext();

    std::string NewVariable(const std::string &type);

    void AddAssignment(const std::string &left, const std::string &right);

    void AddStandaloneStatement(std::string stat);

private:

    size_t nextVariableIndex_;
    std::vector<std::string> definition_;
    std::stack<RC<Block>> blocks_;
};

FunctionContext *GetFunctionContext();

void PushFunctionContext(FunctionContext *context);

void PopFunctionContext();

RTRC_DSL_END

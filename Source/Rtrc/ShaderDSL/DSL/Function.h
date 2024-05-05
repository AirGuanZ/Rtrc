#pragma once

#include <Rtrc/ShaderDSL/Common.h>

RTRC_EDSL_BEGIN

struct FunctionBuilder;

template<typename Ret, typename...Args>
class Function
{
public:

    Function() = default;

    Ret operator()(const std::remove_reference_t<Args> &...args) const;

private:

    friend struct FunctionBuilder;

    explicit Function(std::string name);

    std::string functionName_;
};

struct FunctionBuilder
{
    template<typename Func>
    auto operator+(Func &&func)
    {
        return this->BuildFunction(std::function(std::forward<Func>(func)));
    }

    template<typename Ret, typename...Args>
    Function<Ret, std::remove_reference_t<Args>...> BuildFunction(std::function<Ret(Args...)> bodyFunc);
};

#define RTRC_EDSL_FUNCTION FunctionBuilder{}+[&]

RTRC_EDSL_END

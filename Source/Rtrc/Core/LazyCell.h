#pragma once

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template<typename tResultType, typename tOperator>
class LazyCellImpl
{
    const tOperator &op_;

public:

    using ResultType = tResultType;

    explicit LazyCellImpl(const tOperator &op)
        : op_(op)
    {
        
    }

    operator ResultType() const
    {
        return op_();
    }

    ResultType operator()() const
    {
        return op_();
    }
};

template<typename Operator>
auto LazyCell(const Operator &op)
{
    using ResultType = std::remove_cvref_t<decltype(op())>;
    return LazyCellImpl<ResultType, Operator>(op);
}

RTRC_END

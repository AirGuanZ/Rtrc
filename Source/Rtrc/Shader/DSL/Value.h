#pragma once

#include <cassert>

#include <fmt/format.h>

#include <Rtrc/Math/Vector2.h>
#include <Rtrc/Math/Vector3.h>
#include <Rtrc/Math/Vector4.h>
#include <Rtrc/Shader/DSL/Function.h>

RTRC_DSL_BEGIN

template<typename T>
class Value
{
public:

    virtual ~Value() = default;

    virtual const std::string &GetDSLString() const = 0;
};

template<typename T>
class Variable : public Value<T>
{
    static_assert(
        std::is_same_v<T, int32_t>  ||
        std::is_same_v<T, uint32_t> ||
        std::is_same_v<T, float>);

public:

    static Variable New(const Value<T> &value)
    {
        Variable ret;
        ret = value;
        return ret;
    }

    Variable()
    {
        if(auto f = GetFunctionContext())
        {
            name_ = f->NewVariable();
        }
        else
        {
            value_ = {};
        }
    }

    Variable(const T &value)
    {
        if(auto f = GetFunctionContext())
        {
            name_ = f->NewVariable();
            f->AddAssignment(name_, std::to_string(value));
        }
        else
        {
            value_ = value;
        }
    }

    Variable(const Value<T> &value)
        : Variable()
    {
        *this = value;
    }

    explicit Variable(std::string name)
        : name_(std::move(name))
    {
        if(!GetFunctionContext())
        {
            name_ = {};
            value_ = {};
        }
    }

    Variable(const Variable &other)
        : Variable()
    {
        *this = other;
    }

    Variable &operator=(const Variable &other)
    {
        return *this = static_cast<const Value<T>&>(other);
    }

    Variable(Variable &&) noexcept = delete;

    Variable &operator=(Variable &&) noexcept = delete;

    Variable &operator=(T value)
    {
        assert(!IsDSLVariable());
        value_ = value;
        return *this;
    }

    Variable &operator=(const Value<T> &value)
    {
        assert(IsDSLVariable());
        auto f = GetFunctionContext();
        assert(f);
        f->AddAssignment(name_, value.GetDSLString());
        return *this;
    }

    bool IsDSLVariable() const
    {
        return !name_.empty();
    }

    const T &GetActualValue() const
    {
        assert(!IsDSLVariable());
        return value_;
    }

    const std::string &GetDSLString() const override
    {
        assert(IsDSLVariable());
        return name_;
    }

private:

    T value_;
    std::string name_;
};

template<typename T>
class TemporalValue : public Value<T>
{
public:

    explicit TemporalValue(std::string dslString)
        : dslString_(std::move(dslString))
    {
        
    }

    TemporalValue(const TemporalValue &) = delete;

    TemporalValue &operator=(const TemporalValue &) = delete;

    TemporalValue(TemporalValue &&) noexcept = default;

    TemporalValue &operator=(TemporalValue &&) noexcept = default;

    const std::string &GetDSLString() const override
    {
        return dslString_;
    }

private:

    std::string dslString_;
};

template<typename Ret>
class Function
{
public:

    Function()
    {
        
    }

    explicit Function(std::string name)
        : name_(std::move(name))
    {
        
    }

    template<typename...Args>
    TemporalValue<Ret> operator()(const Args&...args) const
    {
        auto func = GetFunctionContext();

        std::string valStr = "(" + name_ + "(";
        auto appendArg = [&valStr, index = 0]<typename T>(const Value<T> &val) mutable
        {
            valStr += fmt::format("{}{}", index++ > 0 ? ", " : "", val.GetDSLString());
        };
        (appendArg(args), ...);
        valStr += "))";

        if constexpr(std::is_same_v<Ret, void>)
        {
            func->AddStandaloneStatement(valStr + ";");
            return TemporalValue<Ret>(std::string());
        }
        else
        {
            Variable<Ret> ret;
            func->AddAssignment(ret.GetDSLString(), valStr);
            return TemporalValue<Ret>(ret.GetDSLString());
        }
    }

private:

    std::string name_;
};

template<typename T>
TemporalValue<T> operator+(const Value<T> &a, const Value<T> &b)
{
    return TemporalValue<T>(fmt::format("({}) + ({})", a.GetDSLString(), b.GetDSLString()));
}

template<typename T>
TemporalValue<T> operator-(const Value<T> &a, const Value<T> &b)
{
    return TemporalValue<T>(fmt::format("({}) + ({})", a.GetDSLString(), b.GetDSLString()));
}

template<typename T>
TemporalValue<T> operator*(const Value<T> &a, const Value<T> &b)
{
    return TemporalValue<T>(fmt::format("({}) + ({})", a.GetDSLString(), b.GetDSLString()));
}

template<typename T>
TemporalValue<T> operator/(const Value<T> &a, const Value<T> &b)
{
    return TemporalValue<T>(fmt::format("({}) + ({})", a.GetDSLString(), b.GetDSLString()));
}

RTRC_DSL_END

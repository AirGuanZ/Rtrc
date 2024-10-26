#pragma once

#include <cassert>
#include <stack>

#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/Core/Variant.h>

RTRC_BEGIN

class SourceWriter
{
public:

    template<typename...Args>
    SourceWriter &operator()(std::format_string<Args...> fmtStr, Args&&...args)
    {
        std::string content = std::format(fmtStr, std::forward<Args>(args)...);
        if(!content.empty())
        {
            operations_.push_back(AppendOperation{ std::move(content) });
        }
        return *this;
    }

    template<typename T>
    SourceWriter &operator()(const T &msg)
    {
        return (*this)("{}", msg);
    }

    SourceWriter &NewLine(int n = 1)
    {
        operations_.push_back(NewLineOperation{ n });
        return *this;
    }

    SourceWriter &operator++()
    {
        operations_.push_back(IncIndentOperation{});
        return *this;
    }

    SourceWriter &operator--()
    {
        operations_.push_back(DecIndentOperation{});
        return *this;
    }

    SourceWriter &GetSubWritter()
    {
        SubWritterOperation opr;
        opr.writter = Rtrc::MakeBox<SourceWriter>();
        auto &ret = *opr.writter;
        operations_.push_back(std::move(opr));
        return ret;
    }

    void SetIndentLevel(int level)
    {
        operations_.push_back(SetIndentOperation{ level });
    }

    void PushIndentLevel()
    {
        operations_.push_back(PushIndentOperation{});
    }

    void PopIndentLevel()
    {
        operations_.push_back(PopIndentOperation{});
    }

    std::string ResolveResult(std::string_view indentUnit = "    ") const
    {
        std::vector<Operation> operations;
        FlattenOperations(operations_, operations);

        std::string result;

        std::string indentString;
        int indentLevel = 0;
        bool isNewLine = true;

        auto GetIndentString = [&]
        {
            while(indentString.size() < indentLevel * indentUnit.size())
            {
                indentString += indentUnit;
            }
            return std::string_view(indentString).substr(0, indentLevel * indentUnit.size());
        };

        std::stack<int> indentLevelStack;
        for(auto &opr : operations)
        {
            opr.Match(
                [&](const AppendOperation &op)
                {
                    if(isNewLine)
                    {
                        result += GetIndentString();
                        isNewLine = false;
                    }
                    result += op.content;
                },
                [&](const NewLineOperation &op)
                {
                    for(int i = 0; i < op.n; ++i)
                    {
                        result += "\n";
                        isNewLine = true;
                    }
                },
                [&](const IncIndentOperation &)
                {
                    ++indentLevel;
                },
                [&](const DecIndentOperation &)
                {
                    assert(indentLevel > 0);
                    --indentLevel;
                },
                [&](const SetIndentOperation &op)
                {
                    indentLevel = op.count;
                },
                [&](const PushIndentOperation &op)
                {
                    indentLevelStack.push(indentLevel);
                },
                [&](const PopIndentOperation &op)
                {
                    indentLevel = indentLevelStack.top();
                    indentLevelStack.pop();
                },
                [&](const SubWritterOperation &)
                {
                    Unreachable();
                });
        }

        return result;
    }

private:

    struct AppendOperation
    {
        std::string content;
    };

    struct NewLineOperation
    {
        int n;
    };

    struct IncIndentOperation
    {

    };

    struct DecIndentOperation
    {

    };

    struct SetIndentOperation
    {
        int count;
    };

    struct PushIndentOperation
    {
        
    };

    struct PopIndentOperation
    {
        
    };

    struct SubWritterOperation
    {
        Box<SourceWriter> writter;
    };

    using Operation = Variant<
        AppendOperation,
        NewLineOperation,
        IncIndentOperation,
        DecIndentOperation,
        SetIndentOperation,
        PushIndentOperation,
        PopIndentOperation,
        SubWritterOperation>;

    static void FlattenOperations(const std::vector<Operation> &input, std::vector<Operation> &output)
    {
        for(auto &opr : input)
        {
            opr.Match(
                [&](const SubWritterOperation &sw)
                {
                    FlattenOperations(sw.writter->operations_, output);
                },
                [&](const auto &op)
                {
                    output.push_back(op);
                });
        }
    }
    
    std::vector<Operation> operations_;
};

RTRC_END

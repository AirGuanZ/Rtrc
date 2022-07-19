#pragma once

#include <vector>

#include <Rtrc/Common.h>

RTRC_BEGIN

class ShaderBindingParser
{
public:

    using AliasedBindingNames = std::vector<std::string>;

    struct ParsedBindingGroup
    {
        std::string name;
        std::vector<AliasedBindingNames> bindings;
    };

    explicit ShaderBindingParser(std::string source);

    /*
        group GroupName
        {
            BindingName0,
            BindingName1,
            BindingName2,
            [ AliasedBindingName0, AliasedBindingName1(,) ](,)
        };
    */
    bool ProcessNextBindingGroup(ParsedBindingGroup &bindings);

    const std::string &GetFinalSource() const;

private:

    size_t FindNextBindingGroupKeyword();

    size_t pos_;
    std::string source_;
};

RTRC_END

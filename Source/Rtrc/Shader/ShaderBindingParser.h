#pragma once

#include <optional>
#include <vector>

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

class ShaderBindingGroupRewriter
{
public:

    using AliasedBindingNames = std::vector<std::string>;

    struct ParsedBindingGroup
    {
        std::string name;
        std::vector<AliasedBindingNames> bindings;
    };

    explicit ShaderBindingGroupRewriter(std::string source);

    /*
        group GroupName
        {
            BindingName0,
            BindingName1,
            BindingName2,
            [ AliasedBindingName0, AliasedBindingName1(,) ](,)
        };
    */
    bool RewriteNextBindingGroup(ParsedBindingGroup &bindings);

    const std::string &GetFinalSource() const;

private:

    size_t FindNextBindingGroupKeyword();

    size_t pos_;
    std::string source_;
};

class ShaderBindingParser
{
public:

    struct ParsedBinding
    {
        size_t             posInSource;
        RHI::BindingType   type;
        std::string        name;
        std::optional<int> arraySize;
    };

    explicit ShaderBindingParser(std::string source);

    bool FindNextBinding(ParsedBinding &outBinding);

private:

    size_t SkipWhitespaces(size_t pos) const;

    bool FindNextBindingKeyword(size_t &outBegPos, size_t &outEndPos, RHI::BindingType &outType);

    size_t FindEndOfTemplateParameter(size_t curPos) const;

    std::string FindBindingName(size_t curPos, size_t &outEndPos) const;
    
    std::optional<int> FindOptionalArraySize(size_t curPos) const;

    size_t pos_;
    std::string source_;
};

RTRC_END

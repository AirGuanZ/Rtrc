#pragma once

#include <Rtrc/Shader/ShaderCompiler.h>
#include <Rtrc/Utils/StringPool.h>

RTRC_BEGIN

struct KeywordStringTag { };

using Keyword       = PooledString<KeywordStringTag>;
using Keywords      = std::vector<Keyword>;
using KeywordValues = std::vector<std::pair<Keyword, std::string>>;

class KeywordsBuilder
{
public:

    KeywordsBuilder() = default;
    KeywordsBuilder(const KeywordsBuilder &) = default;
    KeywordsBuilder(KeywordsBuilder &&) noexcept = default;

    template<typename...Ks>
    explicit KeywordsBuilder(const Ks &...keywords)
    {
        (*this)(keywords...);
    }

    template<typename...Ks>
    KeywordsBuilder &operator()(const Ks &...keywords)
    {
        (this->Add(keywords), ...);
        return *this;
    }

    Keywords Build() const
    {
        return Keywords{ keywords_.begin(), keywords_.end() };
    }

    operator Keywords() const
    {
        return Build();
    }

private:

    template<typename T>
    void Add(const T &keyword)
    {
        keywords_.insert(Keyword(std::string_view(keyword)));
    }

    std::set<Keyword> keywords_;
};

class KeywordValuesBuilder
{
public:

    KeywordValuesBuilder() = default;
    KeywordValuesBuilder(const KeywordValuesBuilder &) = default;
    KeywordValuesBuilder(KeywordValuesBuilder &&) noexcept = default;

    KeywordValuesBuilder(const Keyword &keyword, std::string value)
    {
        (*this)(keyword, std::move(value));
    }
    
    KeywordValuesBuilder &operator()(const Keyword &keyword, std::string value)
    {
        keywordToValue_[keyword] = std::move(value);
        return *this;
    }

    KeywordValues Build() const
    {
        return KeywordValues{ keywordToValue_.begin(), keywordToValue_.end() };
    }

    operator KeywordValues() const
    {
        return Build();
    }

private:

    std::map<Keyword, std::string> keywordToValue_;
};

inline KeywordValues FilterValuesByKeywords(const Keywords &keywords, const KeywordValues &values)
{
    KeywordValues ret;
    size_t i = 0, j = 0;
    while(true)
    {
        if(i >= keywords.size() || j >= values.size())
        {
            break;
        }
        if(keywords[i] == values[j].first)
        {
            ret.push_back(values[j]);
            ++i, ++j;
        }
        else if(keywords[i] < values[j].first)
        {
            ++i;
        }
        else
        {
            assert(keywords[i] > values[j].first);
            ++j;
        }
    }
    return ret;
}

RTRC_END

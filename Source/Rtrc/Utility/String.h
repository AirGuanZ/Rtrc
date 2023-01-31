#pragma once

#include <iterator>
#include <string>
#include <vector>

#include <Rtrc/Common.h>

RTRC_BEGIN

inline bool IsLower(char c);
inline bool IsUpper(char c);

inline char ToLower(char c);
inline char ToUpper(char c);

inline void ToLower_(std::string &s);
inline void ToUpper_(std::string &s);

inline std::string ToLower(std::string_view str);
inline std::string ToUpper(std::string_view str);

constexpr bool StartsWith(std::string_view str, std::string_view prefix);
constexpr bool EndsWith(std::string_view str, std::string_view suffix);

inline void TrimLeft_(std::string &str);
inline void TrimRight_(std::string &str);
inline void Trim_(std::string &str);

inline std::string TrimLeft(std::string_view str);
inline std::string TrimRight(std::string_view str);
inline std::string Trim(std::string_view str);

// Begin and end are string iterators
template<typename TIt>
std::string Join(char joiner, TIt begin, TIt end);
template<typename TIt>
std::string Join(std::string_view joiner, TIt begin, TIt end);

// rm_empty_result: ignore empty result
inline std::vector<std::string> Split(std::string_view src, char splitter, bool removeEmptyResult = true);
inline std::vector<std::string> Split(std::string_view src, std::string_view splitter, bool removeEmptyResult = true);

// Returns number of output
template<typename TIt>
size_t Split(std::string_view src, char splitter, TIt outIter, bool removeEmptyResult = true);
template<typename TIt>
size_t Split(std::string_view src, std::string_view splitter, TIt outIter, bool removeEmptyResult = true);

inline size_t Replace_(std::string &str, std::string_view oldSeg, std::string_view newSeg);
inline std::string Replace(std::string_view str, std::string_view oldSeg, std::string_view newSeg);

// Support (unsigned) int, (unsigned) long, (unsigned) long long, float, double, long double
template<typename T>
T FromString(const std::string &str);

// align_left("xyz", 5) -> "xyz  "
inline std::string AlignLeft(std::string_view str, size_t width, char padder = ' ');

// align_right("xyz", 5) -> "  xyz"
inline std::string AlignRight(std::string_view str, size_t width, char padder = ' ');

constexpr uint32_t Hash(std::string_view str);

// ========================== impl ==========================

inline bool IsLower(char c)
{
    return 'a' <= c && c <= 'z';
}

inline bool IsUpper(char c)
{
    return 'A' <= c && c <= 'Z';
}

inline char ToLower(char c)
{
    return IsUpper(c) ? c - 'A' + 'a' : c;
}

inline char ToUpper(char c)
{
    return  IsLower(c) ? c - 'a' + 'A' : c;
}

inline void ToLower_(std::string &s)
{
    for(char &c : s)
        c = ToLower(c);
}

inline void ToUpper_(std::string &s)
{
    for(char &c : s)
        c = ToUpper(c);
}

inline std::string ToLower(std::string_view str)
{
    std::string ret(str);
    ToLower_(ret);
    return ret;
}

inline std::string ToUpper(std::string_view str)
{
    std::string ret(str);
    ToUpper_(ret);
    return ret;
}

constexpr bool StartsWith(std::string_view str, std::string_view prefix)
{
    return str.substr(0, prefix.size()) == prefix;
}

constexpr bool EndsWith(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

inline void TrimLeft_(std::string &str)
{
    if(str.empty())
        return;

    auto it = str.begin();
    while(it != str.end())
    {
        if(!std::isspace(*it))
            break;
        ++it;
    }

    str.erase(str.begin(), it);
}

inline void TrimRight_(std::string &str)
{
    if(str.empty())
        return;

    auto it = str.end();
    do
    {
        if(!std::isspace(*(it - 1)))
            break;
        --it;
    } while(it != str.begin());

    str.erase(it, str.end());
}

inline void Trim_(std::string &str)
{
    TrimLeft_(str);
    TrimRight_(str);
}

inline std::string TrimLeft(std::string_view str)
{
    std::string ret(str);
    TrimLeft_(ret);
    return ret;
}

inline std::string TrimRight(std::string_view str)
{
    std::string ret(str);
    TrimRight_(ret);
    return ret;
}

inline std::string Trim(std::string_view str)
{
    std::string ret(str);
    Trim_(ret);
    return ret;
}

template<typename TIt>
std::string Join(char joiner, TIt begin, TIt end)
{
    if(begin == end)
        return std::string();

    std::string ret = *begin++;
    while(begin != end)
    {
        ret.push_back(joiner);
        ret.append(*begin++);
    }

    return ret;
}

template<typename TIt>
std::string Join(std::string_view joiner, TIt begin, TIt end)
{
    if(begin == end)
        return std::string();

    std::string ret = *begin++;
    while(begin != end)
    {
        ret.append(joiner);
        ret.append(*begin++);
    }

    return ret;
}

template<typename TIt>
size_t Split(std::string_view src, char splitter, TIt outIter, bool removeEmptyResult)
{
    size_t beg = 0, ret = 0;
    while(beg < src.size())
    {
        size_t end = beg;
        while(end < src.size() && src[end] != splitter)
            ++end;
        if(end != beg || !removeEmptyResult)
        {
            ++ret;
            outIter = typename TIt::container_type::value_type(
                src.substr(beg, end - beg));
            ++outIter;
        }
        beg = end + 1;
    }
    return ret;
}

template<typename TIt>
size_t Split(std::string_view src, std::string_view splitter, TIt outIter, bool removeEmptyResult)
{
    size_t beg = 0, ret = 0;
    while(beg < src.size())
    {
        size_t end = src.find(splitter, beg);

        if(end == std::string::npos)
        {
            ++ret;
            outIter = typename TIt::container_type::value_type(
                src.substr(beg, src.size() - beg));
            ++outIter;
            break;
        }

        if(end != beg || !removeEmptyResult)
        {
            ++ret;
            outIter = typename TIt::container_type::value_type(
                src.substr(beg, end - beg));
            ++outIter;
        }

        beg = end + splitter.size();
    }
    return ret;
}

inline std::vector<std::string> Split(
    std::string_view src, char splitter, bool removeEmptyResult)
{
    std::vector<std::string> ret;
    Split(src, splitter, std::back_inserter(ret), removeEmptyResult);
    return ret;
}

inline std::vector<std::string> Split(
    std::string_view src, std::string_view splitter, bool removeEmptyResult)
{
    std::vector<std::string> ret;
    Split(src, splitter, std::back_inserter(ret), removeEmptyResult);
    return ret;
}

inline size_t Replace_(std::string &str, std::string_view oldSeg, std::string_view newSeg)
{
    if(oldSeg.empty())
        return 0;
    size_t ret = 0, pos = 0;
    while((pos = str.find(oldSeg, pos)) != std::string::npos)
    {
        str.replace(pos, oldSeg.size(), newSeg);
        pos += newSeg.size();
        ++ret;
    }
    return ret;
}

inline std::string Replace(std::string_view str, std::string_view oldSeg, std::string_view newSeg)
{
    std::string ret(str);
    Replace_(ret, oldSeg, newSeg);
    return ret;
}

namespace StringImpl
{

    template<typename T>
    T from_string_impl(const std::string &) { return T(0); }

    template<> inline int       from_string_impl<int>      (const std::string &str) { return std::stoi(str); }
    template<> inline long      from_string_impl<long>     (const std::string &str) { return std::stol(str); }
    template<> inline long long from_string_impl<long long>(const std::string &str) { return std::stoll(str); }

    template<> inline unsigned int from_string_impl<unsigned int>(const std::string &str)
    {
        const unsigned long ul = std::stoul(str);
        if constexpr(sizeof(unsigned long) != sizeof(unsigned int))
        {
            if(ul > (std::numeric_limits<unsigned int>::max)())
            {
                throw std::out_of_range(str);
            }
        }
        return static_cast<unsigned int>(ul);
    }
    template<> inline unsigned long      from_string_impl<unsigned long>     (const std::string &str) { return std::stoul(str); }
    template<> inline unsigned long long from_string_impl<unsigned long long>(const std::string &str) { return std::stoull(str); }

    template<> inline float       from_string_impl<float>      (const std::string &str) { return std::stof(str); }
    template<> inline double      from_string_impl<double>     (const std::string &str) { return std::stod(str); }
    template<> inline long double from_string_impl<long double>(const std::string &str) { return std::stold(str); }

} // namespace StringImpl

template<typename T>
T FromString(const std::string &str)
{
    return StringImpl::from_string_impl<T>(str);
}

inline std::string AlignLeft(std::string_view str, size_t width, char padder)
{
    if(str.length() >= width)
        return std::string(str);
    return std::string(str) + std::string(width - str.length(), padder);
}

inline std::string AlignRight(std::string_view str, size_t width, char padder)
{
    if(str.length() >= width)
        return std::string(str);
    return std::string(width - str.length(), padder) + std::string(str);
}

constexpr uint32_t Hash(std::string_view str)
{
    // https://stackoverflow.com/questions/2351087/what-is-the-best-32bit-hash-function-for-short-strings-tag-names
    uint32_t h = 0;
    for(char c : str)
    {
        h = 37 * h + static_cast<uint32_t>(c);
    }
    return h;
}

RTRC_END

#include <json.hpp>

#include <Rtrc/Core/Archive/JSONArchiveReader.h>

RTRC_BEGIN

struct JSONArchiveReader::Frame
{
    uint32_t version = 0;
    nlohmann::json json;
    int nextIndexInTuple = -1;
};

JSONArchiveReader::JSONArchiveReader(const std::string &json)
{
    frames_.emplace_back();
    frames_.back().json = nlohmann::json::parse(json);
}

JSONArchiveReader::~JSONArchiveReader()
{
    
}

void JSONArchiveReader::SetVersion(uint32_t version)
{
    assert(version >= frames_.back().version);
}

uint32_t JSONArchiveReader::GetVersion() const
{
    return frames_.back().version;
}

bool JSONArchiveReader::DidReadLastProperty() const
{
    return didReadLastProperty_;
}

bool JSONArchiveReader::BeginTransferObject(std::string_view name)
{
    auto pdst = static_cast<const nlohmann::json *>(GetChildJSON(name));
    if(!pdst || !pdst->is_object())
    {
        return false;
    }

    Frame newFrame;
    newFrame.json = *pdst;
    if(const auto jt = newFrame.json.find("_RtrcArchiveVersion");
       jt != newFrame.json.end() && jt->is_number_integer())
    {
        newFrame.version = *jt;
    }
    else
    {
        newFrame.version = 0;
    }
    frames_.push_back(std::move(newFrame));

    return true;
}

void JSONArchiveReader::EndTransferObject()
{
    didReadLastProperty_ = true;
    frames_.pop_back();
}

bool JSONArchiveReader::BeginTransferTuple(std::string_view name, int size)
{
    auto pdst = static_cast<const nlohmann::json *>(GetChildJSON(name));
    if(!pdst || !pdst->is_array() || static_cast<int>(pdst->size()) != size)
    {
        return false;
    }

    Frame newFrame;
    newFrame.json = *pdst;
    newFrame.nextIndexInTuple = 0;
    frames_.push_back(std::move(newFrame));

    return true;
}

void JSONArchiveReader::EndTransferTuple()
{
    EndTransferObject();
}

template<std::integral T>
void JSONArchiveReader::TransferBuiltin(std::string_view name, T &value)
{
    didReadLastProperty_ = false;

    auto pdst = static_cast<const nlohmann::json *>(GetChildJSON(name));
    if(!pdst)
    {
        return;
    }
    auto &dst = *pdst;

    if constexpr(std::is_same_v<T, bool>)
    {
        if(!dst.is_boolean())
        {
            return;
        }
        value = dst;
    }
    else
    {
        if constexpr(std::is_signed_v<T>)
        {
            if(!dst.is_number_integer())
            {
                return;
            }
        }
        else
        {
            if(!dst.is_number_unsigned())
            {
                return;
            }
        }

        if constexpr(sizeof(T) == 1)
        {
            const int rawValue = dst;
            if(rawValue < std::numeric_limits<T>::lowest() || rawValue >(std::numeric_limits<T>::max)())
            {
                return;
            }
            value = static_cast<char>(rawValue);
        }
        else
        {
            value = dst;
        }
    }

    didReadLastProperty_ = true;
}

template<std::floating_point T>
void JSONArchiveReader::TransferBuiltin(std::string_view name, T &value)
{
    didReadLastProperty_ = false;

    auto pdst = static_cast<const nlohmann::json *>(GetChildJSON(name));
    if(!pdst)
    {
        return;
    }
    auto &dst = *pdst;

    if(!dst.is_number())
    {
        return;
    }
    value = dst;

    didReadLastProperty_ = true;
}

void JSONArchiveReader::TransferBuiltin(std::string_view name, std::string &value)
{
    didReadLastProperty_ = false;

    auto pdst = static_cast<const nlohmann::json *>(GetChildJSON(name));
    if(!pdst)
    {
        return;
    }
    auto &dst = *pdst;

    if(!dst.is_string())
    {
        return;
    }
    value = dst;

    didReadLastProperty_ = true;
}

const void *JSONArchiveReader::GetChildJSON(std::string_view name)
{
    auto &frame = frames_.back();
    if(frame.nextIndexInTuple >= 0)
    {
        assert(frame.json.is_array());
        if(static_cast<int>(frame.json.size()) <= frame.nextIndexInTuple)
        {
            return nullptr;
        }
        return &frame.json[frame.nextIndexInTuple++];
    }
    assert(frame.json.is_object());
    const auto it = frame.json.find(name);
    if(it == frame.json.end())
    {
        return nullptr;
    }
    return &*it;
}

#define EXPLICIT_INSTANTIATE(T) template void JSONArchiveReader::TransferBuiltin<T>(std::string_view, T&)

EXPLICIT_INSTANTIATE(bool);
EXPLICIT_INSTANTIATE(char);
EXPLICIT_INSTANTIATE(signed char);
EXPLICIT_INSTANTIATE(unsigned char);
EXPLICIT_INSTANTIATE(short);
EXPLICIT_INSTANTIATE(unsigned short);
EXPLICIT_INSTANTIATE(int);
EXPLICIT_INSTANTIATE(unsigned int);
EXPLICIT_INSTANTIATE(long);
EXPLICIT_INSTANTIATE(unsigned long);
EXPLICIT_INSTANTIATE(long long);
EXPLICIT_INSTANTIATE(unsigned long long);
EXPLICIT_INSTANTIATE(float);
EXPLICIT_INSTANTIATE(double);
EXPLICIT_INSTANTIATE(long double);

#undef EXPLICIT_INSTANTIATE

RTRC_END

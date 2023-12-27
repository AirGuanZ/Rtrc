#include <cassert>

#include <json.hpp>

#include <Core/Archive/JSONArchiveWritter.h>

RTRC_BEGIN

struct JSONArchiveWritter::Frame
{
    std::string name;
    bool isVersionSet = false;
    int version = 0;
    nlohmann::json json;
};

JSONArchiveWritter::JSONArchiveWritter()
{
    frames_.emplace_back();
    frames_[0].name = "_RtrcArchiveRoot";
    frames_[0].json = nlohmann::json::object();
}

// Define this here for hiding definition of JSONArchiveWritter::Frame.
JSONArchiveWritter::~JSONArchiveWritter() = default;

void JSONArchiveWritter::SetVersion(int version)
{
    assert(version > 0);
    assert(!frames_.empty());
    auto &frame = frames_.back();
    assert(!frame.isVersionSet);
    frame.version = version;
    frame.isVersionSet = true;
}

int JSONArchiveWritter::GetVersion() const
{
    return frames_.back().version;
}

bool JSONArchiveWritter::BeginTransferObject(std::string_view name)
{
    auto &frame = frames_.emplace_back();
    frame.name = name;
    frame.json = nlohmann::json::object();
    return true;
}

void JSONArchiveWritter::EndTransferObject()
{
    assert(frames_.size() >= 2);
    auto &frame = frames_.back();
    if(frame.isVersionSet)
    {
        frame.json["_RtrcArchiveVersion"] = frame.version;
    }

    auto &parentFrame = frames_[frames_.size() - 2];
    if(parentFrame.json.is_array())
    {
        parentFrame.json.push_back(frame.json);
    }
    else
    {
        parentFrame.json[frame.name] = frame.json;
    }
    frames_.pop_back();
}

bool JSONArchiveWritter::BeginTransferTuple(std::string_view name, int size)
{
    auto &frame = frames_.emplace_back();
    frame.name = name;
    frame.json = nlohmann::json::array();
    return true;
}

void JSONArchiveWritter::EndTransferTuple()
{
    EndTransferObject();
}

template<std::integral T>
void JSONArchiveWritter::TransferBuiltin(std::string_view name, T value)
{
    auto &frame = frames_.back();
    if(frame.json.is_array())
    {
        frame.json.push_back(value);
    }
    else
    {
        frame.json[name] = value;
    }
}

template<std::floating_point T>
void JSONArchiveWritter::TransferBuiltin(std::string_view name, T value)
{
    auto &frame = frames_.back();
    if(frame.json.is_array())
    {
        frame.json.push_back(value);
    }
    else
    {
        frame.json[name] = value;
    }
}

void JSONArchiveWritter::TransferBuiltin(std::string_view name, std::string &value)
{
    auto &frame = frames_.back();
    if(frame.json.is_array())
    {
        frame.json.push_back(value);
    }
    else
    {
        frame.json[name] = value;
    }
}

std::string JSONArchiveWritter::ToString() const
{
    assert(frames_.size() == 1);
    return frames_[0].json.dump(4);
}

#define EXPLICIT_INSTANTIATE(T) \
    template void JSONArchiveWritter::TransferBuiltin<T>(std::string_view, T)

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

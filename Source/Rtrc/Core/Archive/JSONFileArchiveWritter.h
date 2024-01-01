#pragma once

#include <Rtrc/Core/Archive/JSONArchiveWritter.h>
#include <Rtrc/Core/Filesystem/File.h>

RTRC_BEGIN

class JSONFileArchiveWritter : public Uncopyable, public ArchiveCommon<JSONFileArchiveWritter>
{
public:

    explicit JSONFileArchiveWritter(std::string path);
    ~JSONFileArchiveWritter();

    void SetVersion(int version);
    int GetVersion() const;

    bool IsWriting() const;
    bool IsReading() const;
    bool DidReadLastProperty() const;

    bool BeginTransferObject(std::string_view name);
    void EndTransferObject();

    bool BeginTransferTuple(std::string_view name, int size);
    void EndTransferTuple();

    template<std::integral T>
    void TransferBuiltin(std::string_view name, T &value);
    template<std::floating_point T>
    void TransferBuiltin(std::string_view name, T &value);
    void TransferBuiltin(std::string_view name, std::string &value);

private:

    std::string path_;
    JSONArchiveWritter jsonWritter_;
};

inline JSONFileArchiveWritter::JSONFileArchiveWritter(std::string path)
    : path_(std::move(path))
{
    
}

inline JSONFileArchiveWritter::~JSONFileArchiveWritter()
{
    const std::string json = jsonWritter_.ToString();
    File::WriteTextFile(path_, json);
}

inline void JSONFileArchiveWritter::SetVersion(int version)
{
    jsonWritter_.SetVersion(version);
}

inline int JSONFileArchiveWritter::GetVersion() const
{
    return jsonWritter_.GetVersion();
}

inline bool JSONFileArchiveWritter::IsWriting() const
{
    return true;
}

inline bool JSONFileArchiveWritter::IsReading() const
{
    return false;
}

inline bool JSONFileArchiveWritter::DidReadLastProperty() const
{
    return false;
}

inline bool JSONFileArchiveWritter::BeginTransferObject(std::string_view name)
{
    return jsonWritter_.BeginTransferObject(name);
}

inline void JSONFileArchiveWritter::EndTransferObject()
{
    jsonWritter_.EndTransferObject();
}

inline bool JSONFileArchiveWritter::BeginTransferTuple(std::string_view name, int size)
{
    return jsonWritter_.BeginTransferTuple(name, size);
}

inline void JSONFileArchiveWritter::EndTransferTuple()
{
    jsonWritter_.EndTransferTuple();
}

template<std::integral T>
void JSONFileArchiveWritter::TransferBuiltin(std::string_view name, T &value)
{
    jsonWritter_.TransferBuiltin(name, value);
}

template<std::floating_point T>
void JSONFileArchiveWritter::TransferBuiltin(std::string_view name, T &value)
{
    jsonWritter_.TransferBuiltin(name, value);
}

inline void JSONFileArchiveWritter::TransferBuiltin(std::string_view name, std::string &value)
{
    jsonWritter_.TransferBuiltin(name, value);
}

RTRC_END

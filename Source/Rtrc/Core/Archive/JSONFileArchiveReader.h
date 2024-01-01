#pragma once

#include <Rtrc/Core/Archive/JSONArchiveReader.h>
#include <Rtrc/Core/Filesystem/File.h>

RTRC_BEGIN

class JSONFileArchiveReader : public Uncopyable, public ArchiveCommon<JSONFileArchiveReader>
{
public:

    explicit JSONFileArchiveReader(const std::string &path);
    
    void SetVersion(int version);
    int GetVersion() const;

    bool IsWriting() const { return false; }
    bool IsReading() const { return true; }
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

    Box<JSONArchiveReader> jsonReader_;
};

inline JSONFileArchiveReader::JSONFileArchiveReader(const std::string &path)
{
    const std::string content = File::ReadTextFile(path);
    jsonReader_ = MakeBox<JSONArchiveReader>(content);
}

inline void JSONFileArchiveReader::SetVersion(int version)
{
    jsonReader_->SetVersion(version);
}

inline int JSONFileArchiveReader::GetVersion() const
{
    return jsonReader_->GetVersion();
}

inline bool JSONFileArchiveReader::DidReadLastProperty() const
{
    return jsonReader_->DidReadLastProperty();
}

inline bool JSONFileArchiveReader::BeginTransferObject(std::string_view name)
{
    return jsonReader_->BeginTransferObject(name);
}

inline void JSONFileArchiveReader::EndTransferObject()
{
    jsonReader_->EndTransferObject();
}

inline bool JSONFileArchiveReader::BeginTransferTuple(std::string_view name, int size)
{
    return jsonReader_->BeginTransferTuple(name, size);
}

inline void JSONFileArchiveReader::EndTransferTuple()
{
    jsonReader_->EndTransferTuple();
}

template<std::integral T>
void JSONFileArchiveReader::TransferBuiltin(std::string_view name, T &value)
{
    jsonReader_->TransferBuiltin(name, value);
}

template<std::floating_point T>
void JSONFileArchiveReader::TransferBuiltin(std::string_view name, T &value)
{
    jsonReader_->TransferBuiltin(name, value);
}

inline void JSONFileArchiveReader::TransferBuiltin(std::string_view name, std::string &value)
{
    jsonReader_->TransferBuiltin(name, value);
}

RTRC_END

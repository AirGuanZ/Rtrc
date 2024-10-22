#pragma once

#include <Rtrc/Core/Archive/ArchiveInterface.h>
#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

class JSONArchiveReader : public Uncopyable, public ArchiveCommon<JSONArchiveReader>
{
public:

    explicit JSONArchiveReader(const std::string &json);
    ~JSONArchiveReader();

    void SetVersion(uint32_t version);
    uint32_t GetVersion() const;

    constexpr bool IsWriting() const { return false; }
    constexpr bool IsReading() const { return true; }
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

    const void *GetChildJSON(std::string_view name);

    struct Frame;
    std::vector<Frame> frames_;
    bool didReadLastProperty_ = false;
};

RTRC_END

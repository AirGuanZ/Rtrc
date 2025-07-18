#pragma once

#include <Rtrc/Core/Archive/ArchiveInterface.h>
#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

class JSONArchiveWritter : public Uncopyable, public ArchiveCommon<JSONArchiveWritter, false>
{
public:

    JSONArchiveWritter();
    ~JSONArchiveWritter();

    void SetVersion(uint32_t version);
    uint32_t GetVersion() const;

    constexpr bool DidReadLastProperty() const { return false; }

    bool BeginTransferObject(std::string_view name);
    void EndTransferObject();

    bool BeginTransferTuple(std::string_view name, int size);
    void EndTransferTuple();

    template<std::integral T>
    void TransferBuiltin(std::string_view name, T value);
    template<std::floating_point T>
    void TransferBuiltin(std::string_view name, T value);

    void TransferBuiltin(std::string_view name, const std::string &value);

    std::string ToString() const;
    
private:

    struct Frame;
    std::vector<Frame> frames_;
};

RTRC_END

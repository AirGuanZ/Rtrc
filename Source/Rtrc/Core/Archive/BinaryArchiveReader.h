#pragma once

#include <Rtrc/Core/Archive/ArchiveInterface.h>
#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

namespace BinaryArchiveReaderDetail
{

    template<bool ReadVersionNumber>
    struct VersionsMember
    {
        
    };

    template<>
    struct VersionsMember<true>
    {
        std::vector<uint32_t> versions_;
    };

} // namespace BinaryArchiveReaderDetail

template<bool ReadVersionNumber>
class BinaryArchiveReader :
    public Uncopyable,
    BinaryArchiveReaderDetail::VersionsMember<ReadVersionNumber>,
    public ArchiveCommon<BinaryArchiveReader<ReadVersionNumber>>
{
public:

    BinaryArchiveReader() = default;

    BinaryArchiveReader(const void *data, size_t size)
        : input_(static_cast<const std::byte*>(data), size)
    {

    }

    void SetVersion(uint32_t version) { }

    uint32_t GetVersion() const
    {
        if constexpr(ReadVersionNumber)
        {
            return this->versions_.back();
        }
        else
        {
            return 0;
        }
    }

    constexpr bool IsWriting() const { return false; }
    constexpr bool IsReading() const { return true; }
    constexpr bool DidReadLastProperty() const { return true; }

    bool BeginTransferObject(std::string_view name)
    {
        if constexpr(ReadVersionNumber)
        {
            this->versions_.push_back(ConsumeVersionNumber());
        }
        return true;
    }

    void EndTransferObject()
    {
        if constexpr(ReadVersionNumber)
        {
            this->versions_.pop_back();
        }
    }

    template<std::integral T>
    void TransferBuiltin(std::string_view name, T &value)
    {
        this->ConsumeNextField(value);
    }

    template<std::floating_point T>
    void TransferBuiltin(std::string_view name, T &value)
    {
        this->ConsumeNextField(value);
    }

    void TransferBuiltin(std::string_view name, std::string &value)
    {
        uint64_t size;
        ConsumeNextField(size);
        value.resize(size);
        ConsumeNextNBytes(value.data(), size);
    }

private:

    void ConsumeNextNBytes(void *output, size_t N)
    {
        if(input_.size() < sizeof(uint32_t))
        {
            throw Exception(fmt::format(
                "BinaryArchiveReader: trying to read next {} bytes, but only {} bytes are available",
                N, input_.size()));
        }
        std::memcpy(output, input_.GetData(), N);
        input_ = input_.GetSubSpan(N);
    }

    template<typename T>
    void ConsumeNextField(T &value)
    {
        static_assert(std::is_trivially_copy_constructible_v<T>);
        constexpr size_t advancedSize = UpAlignTo<size_t>(sizeof(T), 4);
        if(input_.size() < advancedSize)
        {
            throw Exception(fmt::format(
                "BinaryArchiveReader: trying to read next {} bytes, but only {} bytes are available",
                advancedSize, input_.size()));
        }
        std::memcpy(&value, input_.GetData(), sizeof(T));
        input_ = input_.GetSubSpan(advancedSize);
    }

    uint32_t ConsumeVersionNumber()
    {
        uint32_t result;
        ConsumeNextField(result);
        return result;
    }

    Span<std::byte> input_;
};

RTRC_END

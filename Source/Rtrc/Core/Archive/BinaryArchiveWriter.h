#pragma once

#include <Rtrc/Core/Archive/ArchiveInterface.h>
#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

template<bool WriteVersionNumber>
class BinaryArchiveWriter : public Uncopyable, public ArchiveCommon<BinaryArchiveWriter<WriteVersionNumber>>
{
public:

    BinaryArchiveWriter()
    {
        versions_.push_back(0);
    }

    void SetVersion(uint32_t version)
    {
        assert(!versions_.empty());
        versions_.back() = version;
    }

    uint32_t GetVersion() const
    {
        assert(!versions_.empty());
        return versions_.back();
    }

    constexpr bool IsWriting() const { return true; }
    constexpr bool IsReading() const { return false; }
    constexpr bool DidReadLastProperty() const { return false; }

    bool BeginTransferObject(std::string_view name)
    {
        versions_.emplace_back();
        if constexpr(WriteVersionNumber)
        {
            versions_.back().offset = output_.size();
            output_.resize(output_.size() + sizeof(uint32_t));
        }
        return true;
    }

    void EndTransferObject()
    {
        if constexpr(WriteVersionNumber)
        {
            const uint32_t version = versions_.back().version;
            std::memcpy(output_.data() + versions_.back().offset, &version, sizeof(uint32_t));
        }
        versions_.pop_back();
    }

    template<std::integral T>
    void TransferBuiltin(std::string_view name, T value)
    {
        const size_t offset = output_.size();
        output_.resize(output_.size() + (UpAlignTo<size_t>)(sizeof(T), 4));
        std::memcpy(output_.data() + offset, &value, sizeof(T));
    }

    template<std::floating_point T>
    void TransferBuiltin(std::string_view name, T value)
    {
        const size_t offset = output_.size();
        output_.resize(output_.size() + (std::max<size_t>)(sizeof(T), 4));
        std::memcpy(output_.data() + offset, &value, sizeof(T));
    }

    void TransferBuiltin(std::string_view name, std::string &value)
    {
        TransferBuiltin({}, static_cast<uint64_t>(value.size()));
        const size_t offset = output_.size();
        output_.resize(output_.size() + UpAlignTo<size_t>(value.size(), 4));
        std::memcpy(output_.data() + offset, value.data(), value.size());
    }

    Span<std::byte> GetResult() const
    {
        return output_;
    }

    std::vector<std::byte> GetResultWithOwnership() &&
    {
        return std::move(output_);
    }

private:

    struct VersionRecord
    {
        uint32_t version = 0;
        size_t offset = 0;
    };

    std::vector<VersionRecord> versions_;
    std::vector<std::byte> output_;
};

RTRC_END

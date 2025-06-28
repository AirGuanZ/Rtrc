#pragma once

#include <string>
#include <vector>

#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

namespace File
{

    std::vector<unsigned char> ReadBinaryFile(const std::string &filename);

    std::string ReadTextFile(const std::string &filename);

    void WriteTextFile(const std::string &filename, std::string_view content);

    uint64_t GetLastWriteTime(const std::string &filename);

    class AutoDeleteDirectory : public Uncopyable
    {
    public:

        AutoDeleteDirectory();
        explicit AutoDeleteDirectory(const std::filesystem::path &path);
        ~AutoDeleteDirectory();

        AutoDeleteDirectory(AutoDeleteDirectory &&other) noexcept;
        AutoDeleteDirectory &operator=(AutoDeleteDirectory &&other) noexcept;

        void Swap(AutoDeleteDirectory &other) noexcept;

    private:

        struct Impl;

        std::unique_ptr<Impl> impl_;
    };

} // namespace File

RTRC_END

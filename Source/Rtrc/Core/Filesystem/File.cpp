#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <Rtrc/Core/Filesystem/File.h>
#include <Rtrc/Core/ScopeGuard.h>

#if WIN32 || _WIN32
#include <Windows.h>
#endif

RTRC_BEGIN

namespace File
{

    std::vector<unsigned char> ReadBinaryFile(const std::string &filename)
    {
        std::ifstream fin(filename, std::ios::in | std::ios::binary);
        if(!fin)
        {
            throw Exception("Failed to open binary file: " + filename);
        }
        fin.seekg(0, std::ios::end);
        const auto len = fin.tellg();
        fin.seekg(0, std::ios::beg);
        std::vector<unsigned char> ret(static_cast<size_t>(len));
        fin.read(reinterpret_cast<char *>(ret.data()), len);
        return ret;
    }

    std::string ReadTextFile(const std::string &filename)
    {
        std::ifstream fin(filename, std::ios_base::in);
        if(!fin)
        {
            throw Exception("Failed to open text file: " + filename);
        }
        std::stringstream sst;
        sst << fin.rdbuf();
        return sst.str();
    }

    void WriteTextFile(const std::string &filename, std::string_view content)
    {
        std::ofstream fout(filename, std::ios_base::out | std::ios_base::trunc);
        if(!fout)
        {
            throw Exception("Fail to open output text file: " + filename);
        }
        fout << content;
    }

    uint64_t GetLastWriteTime(const std::string &filename)
    {
#if WIN32 || _WIN32
        const HANDLE fileHandle = CreateFileA(
            filename.c_str(), GENERIC_READ, 0,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if(fileHandle == INVALID_HANDLE_VALUE)
        {
            return {};
        }
        RTRC_SCOPE_EXIT{ CloseHandle(fileHandle); };

        FILETIME lastModifyTime;
        if(!GetFileTime(fileHandle, nullptr, nullptr, &lastModifyTime))
        {
            return {};
        }

        return static_cast<uint64_t>(lastModifyTime.dwLowDateTime) | (static_cast<uint64_t>(lastModifyTime.dwHighDateTime) << 32);
#else
        const auto lastWriteTime = last_write_time(path.GetPath());
        const auto lastWriteSystemTime = std::chrono::clock_cast<std::chrono::system_clock>(lastWriteTime);
        const auto lastWriteTimeT = std::chrono::system_clock::to_time_t(lastWriteSystemTime);
        return static_cast<uint64_t>(lastWriteTimeT);
#endif
    }

    struct AutoDeleteDirectory::Impl
    {
#if WIN32 || _WIN32
        HANDLE dirHandle;
#endif
    };

    AutoDeleteDirectory::AutoDeleteDirectory() = default;

    AutoDeleteDirectory::AutoDeleteDirectory(const std::filesystem::path &path)
    {
#if WIN32 || _WIN32
        auto impl = std::make_unique<Impl>();

        const std::string directory = path.string();
        const HANDLE dirHandle = CreateFileA(
            directory.c_str(), DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if(dirHandle == INVALID_HANDLE_VALUE)
        {
            RemoveDirectoryA(directory.c_str());
            throw Exception("Fail to set deletion flag on directory" + directory);
        }

        impl->dirHandle = dirHandle;
        impl_ = std::move(impl);

#else
        throw Exception("AutoDeleteDirectory: not implemented");
#endif
    }

    AutoDeleteDirectory::~AutoDeleteDirectory()
    {
        if(impl_)
        {
            assert(impl_->dirHandle != INVALID_HANDLE_VALUE);
            CloseHandle(impl_->dirHandle);
        }
    }

    AutoDeleteDirectory::AutoDeleteDirectory(AutoDeleteDirectory &&other) noexcept
    {
        Swap(other);
    }

    AutoDeleteDirectory &AutoDeleteDirectory::operator=(AutoDeleteDirectory &&other) noexcept
    {
        Swap(other);
        return *this;
    }

    void AutoDeleteDirectory::Swap(AutoDeleteDirectory &other) noexcept
    {
        impl_.swap(other.impl_);
    }

} // namespace File

RTRC_END

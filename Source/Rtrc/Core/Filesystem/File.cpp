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

} // namespace File

RTRC_END

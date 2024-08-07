#include <fstream>
#include <sstream>

#include <Rtrc/Core/Filesystem/File.h>

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

} // namespace File

RTRC_END

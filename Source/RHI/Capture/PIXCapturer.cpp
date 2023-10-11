#ifdef _MSC_VER
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#include <codecvt>
#include <locale>

#include <Core/String.h>
#include <Core/Uncopyable.h>
#include <RHI/Capture/GPUCapturer.h>

#if RTRC_PIX_CAPTURER

#include <Windows.h>
#include <WinPixEventRuntime/pix3.h>

RTRC_RHI_BEGIN

namespace PIXCapturerDetail
{

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> gConvertor;

    std::wstring ToWString(const std::string &s)
    {
        return gConvertor.from_bytes(s);
    }

    class PIXLibraryLoader
    {
        HMODULE handle_;

    public:

        PIXLibraryLoader()
        {
            handle_ = PIXLoadLatestWinPixGpuCapturerLibrary();
        }

        ~PIXLibraryLoader()
        {
            if(handle_)
            {
                FreeLibrary(handle_);
            }
        }
    };

    class PIXCapturer : public GPUCapturer, public Uncopyable
    {
    public:

        PIXCapturer()
        {
            static PIXLibraryLoader libraryLoader;
        }

        bool Begin(const std::string &outputFilename) override
        {
            std::string filename = outputFilename;
            if(!EndsWith(ToLower(filename), ".wpix"))
            {
                filename += ".wpix";
            }

            const std::wstring wfilename = ToWString(filename);
            PIXCaptureParameters params = {};
            params.GpuCaptureParameters.FileName = wfilename.c_str();
            return !FAILED(PIXBeginCapture(PIX_CAPTURE_GPU, &params));
        }

        bool End() override
        {
            return !FAILED(PIXEndCapture(false));
        }
    };

} // namespace PIXCapturerDetail

Box<GPUCapturer> CreatePIXCapturer()
{
    return MakeBox<PIXCapturerDetail::PIXCapturer>();
}

RTRC_RHI_END

#endif // #if RTRC_PIX_CAPTURER

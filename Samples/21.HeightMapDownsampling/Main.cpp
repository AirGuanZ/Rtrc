#include "Common.h"

int main()
{
    const auto source = ImageDynamic::Load("./Asset/Sample/21.HeightMapDownsampler/Input0.png").To<double>();
    assert(source.GetWidth() == source.GetHeight());
    assert(source.GetWidth() >= 2);

    const auto targetNative = Downsample_Native(source, source.GetSSize() / 10);
    const auto targetVolume = Downsample_PreserveVolume(source, source.GetSSize() / 10, 50, 0.2, true);

    targetNative.To<float>().Save("./Asset/Sample/21.HeightMapDownsampler/OutputNative.png");
    targetVolume.To<float>().Save("./Asset/Sample/21.HeightMapDownsampler/OutputVolume.png");
}

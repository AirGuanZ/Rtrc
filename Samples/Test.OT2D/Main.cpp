#include <Rtrc/Rtrc.h>
#include <Rtrc/ToolKit/OT2D/OT2D.h>

int main()
{
    auto densityImage = Rtrc::ImageDynamic::Load("Asset/Sample/Test.OT2D/InputDensity.png").To<double>();
    auto transportMap = Rtrc::OT2D().Solve(densityImage).To<Rtrc::Vector3f>();
    for(auto &v : transportMap)
    {
        v.z = 0;
    }
    transportMap.Save("Output.exr");
}

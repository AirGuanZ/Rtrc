#include <Rtrc/Rtrc.h>

using namespace Rtrc::eDSL;

$struct(A)
{
    $var(Rtrc::eDSL::u32, b);
};

int main()
{
    RecordContext context;
    PushRecordContext(context);
    RTRC_SCOPE_EXIT{ PopRecordContext(); };

    auto buffer = Buffer<u32>::CreateFromName("TestBuffer");
    auto buffer2 = StructuredBuffer<A>::CreateFromName("ABuffer");
    auto buffer3 = ByteAddressBuffer::CreateFromName("BBuffer");
    auto buffer4 = RWByteAddressBuffer::CreateFromName("CBuffer");

    auto Func = $function(i32 &a, i32 b)
    {
        a = 2 * a;
        A ret;
        ret.b = a;
        return ret;
    };

    auto Func2 = $function
    {
        A classA;
        classA.b = 999;

        A classB = classA;

        i32 i = 0;
        $while(i != 100)
        {
            classB.b = classB.b + 1u;
            i = i + 1;
        };

        A a = Func(i, 4);
        a.b = a.b + buffer[2];

        buffer2[2] = a;

        buffer4.Store(0, a);
    };

    fmt::print("{}", context.BuildResult());
}

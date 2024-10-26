#include <iostream>
#include <print>

#include <Rtrc/Core/Archive/Archive.h>
#include <Rtrc/Core/Math/Vector3.h>

using namespace Rtrc;

struct MyStruct
{
    uint32_t a = 0;
    uint32_t b = 0;
    Vector3i vec = { 1, 2, 3 };

    auto operator<=>(const MyStruct &) const = default;

    RTRC_NAIVE_TRANSFER(a, b, vec);
};

struct MyStruct2
{
    MyStruct s1;
    MyStruct sArray[2];

    auto operator<=>(const MyStruct2 &) const = default;

    RTRC_NAIVE_TRANSFER(s1, sArray);
};

int main()
{
    MyStruct2 s;
    s.s1.b = 4;

    JSONArchiveWritter jsonWriter;
    jsonWriter.Transfer("root", s);

    std::cout << jsonWriter.ToString() << "\n";

    {
        MyStruct2 s1;
        JSONArchiveReader jsonReader(jsonWriter.ToString());
        jsonReader.Transfer("root", s1);
        std::print("Serialize/deserialize using JSON string: {}\n", s == s1);
    }

    BinaryArchiveWriter<true> binaryWriter;
    binaryWriter.Transfer("root", s);

    {
        MyStruct2 s1;
        BinaryArchiveReader<true> binaryReader(binaryWriter.GetResult().GetData(), binaryWriter.GetResult().GetSize());
        binaryReader.Transfer("root", s1);
        std::print("Serialize/deserialize using binary data: {}\n", s == s1);
    }
}

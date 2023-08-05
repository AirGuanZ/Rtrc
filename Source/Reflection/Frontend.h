#pragma once

#include <set>
#include <string>
#include <vector>

#define DEBUG_OUTPUT 0

enum class FieldType
{
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UInt,
    UInt2,
    UInt3,
    UInt4,
    Float4x4,
    Others
};

struct Field
{
    FieldType type = FieldType::Others;
    std::string typeStr;
    std::string name;

    auto operator<=>(const Field &) const = default;
};

struct Struct
{
    std::string qualifiedName;
    std::set<std::string> annotations;
    std::vector<Field> fields;

    auto operator<=>(const Struct &) const = default;
};

struct SourceInfo
{
    std::vector<std::string> filenames;
    std::vector<std::string> includeDirs;
};

std::vector<Struct> ParseStructs(const SourceInfo &sourceInfo);

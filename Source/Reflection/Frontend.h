#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

enum class FieldKind
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
    FieldKind          kind = FieldKind::Others;
    std::string        typeStr;
    std::string        name;
    std::optional<int> arraySize;

    auto operator<=>(const Field &) const = default;
};

struct Struct
{
    std::string           qualifiedName;
    std::set<std::string> annotations;
    std::vector<Field>    fields;

    std::string sourceFilename;

    auto operator<=>(const Struct &) const = default;
};

struct Enum
{
    std::string                              qualifiedName;
    bool                                     isScoped = false;
    std::set<std::string>                    annotations;
    std::vector<std::pair<std::string, int>> values;

    auto operator<=>(const Enum &) const = default;
};

struct SourceInfo
{
    std::vector<std::string> filenames;
    std::vector<std::string> includeDirs;
};

void Parse(const SourceInfo &sourceInfo, std::vector<Struct> &outStructs, std::vector<Enum> &outEnums);

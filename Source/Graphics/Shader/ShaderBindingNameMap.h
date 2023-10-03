#pragma once

#include <map>

#include <Core/StringPool.h>
#include <Core/Uncopyable.h>

RTRC_BEGIN

class Compiler;

class ShaderBindingNameMap : public Uncopyable
{
public:

    int GetContainedBindingGroupIndex(std::string_view bindingName) const;

    int GetIndexInBindingGroup(std::string_view bindingName) const;

    const std::string &GetBindingName(int groupIndex, int indexInGroup) const;
    const GeneralPooledString &GetPooledBindingName(int groupIndex, int indexInGroup) const;

private:

    friend class ShaderCompiler;
    friend class Compiler;

    struct BindingIndexInfo
    {
        int bindingGroupIndex;
        int indexInBindingGroup;
    };

    // binding name -> (binding group index, index in group)
    std::map<std::string, BindingIndexInfo, std::less<>> nameToSetAndSlot_;

    std::vector<std::vector<std::string>>         allBindingNames_;
    std::vector<std::vector<GeneralPooledString>> allPooledBindingNames_;
};

inline int ShaderBindingNameMap::GetContainedBindingGroupIndex(std::string_view bindingName) const
{
    auto it = nameToSetAndSlot_.find(bindingName);
    return it != nameToSetAndSlot_.end() ? it->second.bindingGroupIndex : -1;
}

inline int ShaderBindingNameMap::GetIndexInBindingGroup(std::string_view bindingName) const
{
    auto it = nameToSetAndSlot_.find(bindingName);
    return it != nameToSetAndSlot_.end() ? it->second.indexInBindingGroup : -1;
}

inline const std::string &ShaderBindingNameMap::GetBindingName(int groupIndex, int indexInGroup) const
{
    return allBindingNames_[groupIndex][indexInGroup];
}

inline const GeneralPooledString &ShaderBindingNameMap::GetPooledBindingName(int groupIndex, int indexInGroup) const
{
    return allPooledBindingNames_[groupIndex][indexInGroup];
}

RTRC_END

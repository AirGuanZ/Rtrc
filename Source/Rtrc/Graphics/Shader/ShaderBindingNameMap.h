#pragma once

#include <map>
#include <optional>

#include <Rtrc/Utility/Uncopyable.h>

RTRC_BEGIN

class ShaderCompiler;

class ShaderBindingNameMap : public Uncopyable
{
public:

    int GetContainedBindingGroupIndex(std::string_view bindingName) const;

    int GetIndexInBindingGroup(std::string_view bindingName) const;

    const std::string &GetBindingName(int groupIndex, int indexInGroup) const;

private:

    friend class ShaderCompiler;

    struct BindingIndexInfo
    {
        int bindingGroupIndex;
        int indexInBindingGroup;
    };

    // binding name -> (binding group index, index in group)
    std::map<std::string, BindingIndexInfo, std::less<>> nameToSetAndSlot_;

    std::vector<std::vector<std::string>> allBindingNames_;
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

RTRC_END

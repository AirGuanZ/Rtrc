#pragma once

#include <Rtrc/Core/SourceWriter.h>

#include "RecordContext.h"
#include "StructTypeSet.h"

RTRC_EDSL_BEGIN

template <RtrcDSL_Struct T>
void StructTypeSet::Add()
{
    const std::string_view name = typeid(T).name();
    if(!registrations_.contains(name))
    {
        registrations_.emplace(name, [](ResolvedStructTypeSet &result) { result.Add<T>(); });
    }
}

inline void StructTypeSet::ResolveInto(ResolvedStructTypeSet& resolvedSet) const
{
    for(auto& r : registrations_)
    {
        r.second(resolvedSet);
    }
}

template <RtrcDSL_Struct T>
void ResolvedStructTypeSet::Add()
{
    const std::string_view cppTypeName = typeid(T).name();
    if(resolvedTypes_.contains(cppTypeName))
    {
        return;
    }

    SourceWriter sw;
    sw("struct {}", T::GetStaticTypeName()).NewLine();
    sw("{").NewLine();
    ++sw;

    StructDetail::ForEachMember<T>([&]<typename M>(M T::*, const char *name)
    {
        if constexpr(RtrcDSL_Struct<M>)
        {
            this->Add<M>();
        }
        sw("{} {};", M::GetStaticTypeName(), name).NewLine();
    });

    --sw;
    sw("};").NewLine();

    resolvedTypes_.insert(std::string(cppTypeName));
    structDefinitions_.append(sw.ResolveResult());
}

inline const std::string& ResolvedStructTypeSet::GetDefinitions() const
{
    return structDefinitions_;
}

template <typename T>
void RecordContext::RegisterStruct()
{
    structs_->Add<T>();
}

RTRC_EDSL_END

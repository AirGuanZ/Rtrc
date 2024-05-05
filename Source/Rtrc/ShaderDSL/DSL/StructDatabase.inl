#pragma once

#include <Rtrc/Core/SourceWriter.h>
#include <Rtrc/ShaderDSL/DSL/RecordContext.h>
#include <Rtrc/ShaderDSL/DSL/StructDatabase.h>

RTRC_EDSL_BEGIN

template <RtrcDSL_Struct T>
void StructDatabase::Add()
{
    const std::string_view cppTypeName = typeid(T).name();
    if(typeNameToDefinitionIndex_.contains(cppTypeName))
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

    typeNameToDefinitionIndex_.emplace(std::string(cppTypeName), static_cast<unsigned>(definitions_.size()));
    definitions_.push_back(sw.ResolveResult());
}

template <typename T>
void RecordContext::RegisterStruct()
{
    structs_->Add<T>();
}

RTRC_EDSL_END

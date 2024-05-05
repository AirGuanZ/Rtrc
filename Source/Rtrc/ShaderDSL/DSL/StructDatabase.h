#pragma once

#include <map>

#include <Rtrc/Core/Uncopyable.h>
#include <Rtrc/ShaderDSL/DSL/Struct.h>

RTRC_EDSL_BEGIN

class StructDatabase : public Uncopyable
{
public:

    template<RtrcDSL_Struct T>
    void Add();

    auto &GetDefinitions() const { return definitions_; }

private:

    std::vector<std::string> definitions_;
    std::map<std::string, uint32_t, std::less<>> typeNameToDefinitionIndex_;
};

RTRC_EDSL_END

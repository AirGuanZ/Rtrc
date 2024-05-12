#pragma once

#include <map>
#include <set>

#include <Rtrc/Core/Uncopyable.h>

#include "Struct.h"

RTRC_EDSL_BEGIN

class ResolvedStructTypeSet;

class StructTypeSet : public Uncopyable
{
public:

    template<RtrcDSL_Struct T>
    void Add();

    void ResolveInto(ResolvedStructTypeSet &resolvedSet) const;

private:

    using Registration = std::function<void(ResolvedStructTypeSet &)>;

    std::map<std::string, Registration, std::less<>> registrations_;
};

class ResolvedStructTypeSet : public Uncopyable
{
public:

    template<RtrcDSL_Struct T>
    void Add();

    const std::string &GetDefinitions() const;

private:

    std::set<std::string, std::less<>> resolvedTypes_;
    std::string structDefinitions_;
};

RTRC_EDSL_END

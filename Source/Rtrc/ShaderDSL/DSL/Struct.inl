#pragma once

#include <Rtrc/ShaderDSL/DSL/RecordContext.h>
#include <Rtrc/ShaderDSL/DSL/Struct.h>

RTRC_EDSL_BEGIN

template <typename ClassType>
ClassDetail::RegisterStructToRecordContext<ClassType>::RegisterStructToRecordContext()
{
    GetCurrentRecordContext().RegisterStruct<ClassType>();
}

RTRC_EDSL_END

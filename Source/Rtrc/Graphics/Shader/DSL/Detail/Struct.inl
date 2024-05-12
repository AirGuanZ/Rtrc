#pragma once

#include "RecordContext.h"
#include "Struct.h"

RTRC_EDSL_BEGIN

template <typename ClassType>
ClassDetail::RegisterStructToRecordContext<ClassType>::RegisterStructToRecordContext()
{
    GetCurrentRecordContext().RegisterStruct<ClassType>();
}

RTRC_EDSL_END

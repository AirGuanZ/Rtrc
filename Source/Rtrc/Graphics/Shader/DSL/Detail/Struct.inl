#pragma once

#include "RecordContext.h"
#include "Struct.h"

RTRC_EDSL_BEGIN

template <typename C, TemplateStringParameter Name>
eDSLStructDetail::CommonBase<C, Name>::CommonBase()
{
    GetCurrentRecordContext().RegisterStruct<C>();
}

RTRC_EDSL_END

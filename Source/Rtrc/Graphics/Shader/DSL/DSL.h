#pragma once

#include "Detail/RecordContext.h"

#include "Detail/Buffer.h"
#include "Detail/eVariable.h"
#include "Detail/eMatrix4x4.h"
#include "Detail/eNumber.h"
#include "Detail/eVector2.h"
#include "Detail/eVector3.h"
#include "Detail/eVector4.h"
#include "Detail/eVectorSwizzle.h"
#include "Detail/Function.h"
#include "Detail/If.h"
#include "Detail/NativeTypeMapping.h"
#include "Detail/Struct.h"
#include "Detail/StructTypeSet.h"
#include "Detail/While.h"

#include "Detail/Buffer.inl"
#include "Detail/eVariable.inl"
#include "Detail/eMatrix4x4.inl"
#include "Detail/eNumber.inl"
#include "Detail/eVector2.inl"
#include "Detail/eVector3.inl"
#include "Detail/eVector4.inl"
#include "Detail/eVectorSwizzle.inl"
#include "Detail/Function.inl"
#include "Detail/If.inl"
#include "Detail/Struct.inl"
#include "Detail/StructTypeSet.inl"
#include "Detail/While.inl"

#define $if RTRC_EDSL_IF
#define $else RTRC_EDSL_ELSE

#define $struct RTRC_EDSL_STRUCT
#define $var RTRC_EDSL_VAR

#define $while RTRC_EDSL_WHILE

#define $function RTRC_EDSL_FUNCTION

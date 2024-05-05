#pragma once

#include <Rtrc/ShaderDSL/DSL/RecordContext.h>

#include <Rtrc/ShaderDSL/DSL/eVariable.h>
#include <Rtrc/ShaderDSL/DSL/eNumber.h>
#include <Rtrc/ShaderDSL/DSL/eVector2.h>
#include <Rtrc/ShaderDSL/DSL/Function.h>
#include <Rtrc/ShaderDSL/DSL/If.h>
#include <Rtrc/ShaderDSL/DSL/Struct.h>
#include <Rtrc/ShaderDSL/DSL/StructDatabase.h>
#include <Rtrc/ShaderDSL/DSL/While.h>

#include <Rtrc/ShaderDSL/DSL/eVariable.inl>
#include <Rtrc/ShaderDSL/DSL/eNumber.inl>
#include <Rtrc/ShaderDSL/DSL/eVector2.inl>
#include <Rtrc/ShaderDSL/DSL/Function.inl>
#include <Rtrc/ShaderDSL/DSL/If.inl>
#include <Rtrc/ShaderDSL/DSL/Struct.inl>
#include <Rtrc/ShaderDSL/DSL/StructDatabase.inl>
#include <Rtrc/ShaderDSL/DSL/While.inl>

#define $if RTRC_EDSL_IF
#define $else RTRC_EDSL_ELSE

#define $struct RTRC_EDSL_STRUCT
#define $var RTRC_EDSL_VAR

#define $while RTRC_EDSL_WHILE

#define $function RTRC_EDSL_FUNCTION

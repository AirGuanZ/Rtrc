#pragma once

#include "eDSL/RecordContext.h"

#include "eDSL/Resource/eBuffer.h"
#include "eDSL/Resource/eBuffer.inl"

#include "eDSL/Resource/eTexture.h"
#include "eDSL/Resource/eTexture.inl"

#include "eDSL/Resource/eSamplerState.h"

#include "eDSL/Resource/eConstantBuffer.h"
#include "eDSL/Resource/eConstantBuffer.inl"

#include "eDSL/Resource/eRaytracingAccelerationStructure.h"

#include "eDSL/Value/eMatrix4x4.h"
#include "eDSL/Value/eNumber.h"
#include "eDSL/Value/eVector2.h"
#include "eDSL/Value/eVector3.h"
#include "eDSL/Value/eVector4.h"
#include "eDSL/Value/eVectorSwizzle.h"
#include "eDSL/Value/Struct.h"
#include "eDSL/Value/StructTypeSet.h"

#include "eDSL/Value/eMatrix4x4.inl"
#include "eDSL/Value/eNumber.inl"
#include "eDSL/Value/eVector2.inl"
#include "eDSL/Value/eVector3.inl"
#include "eDSL/Value/eVector4.inl"
#include "eDSL/Value/eVectorSwizzle.inl"
#include "eDSL/Value/Struct.inl"
#include "eDSL/Value/StructTypeSet.inl"

#include "eDSL/ControlFlow/If.h"
#include "eDSL/ControlFlow/While.h"
#include "eDSL/ControlFlow/If.inl"
#include "eDSL/ControlFlow/While.inl"

#include "eDSL/eVariable.h"
#include "eDSL/eVariable.inl"

#include "eDSL/NativeTypeMapping.h"

#define $if    RTRC_EDSL_IF
#define $else  RTRC_EDSL_ELSE
#define $while RTRC_EDSL_WHILE

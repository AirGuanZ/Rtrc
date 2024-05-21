#pragma once

#include "eDSL/RecordContext.h"

#include "eDSL/Resource/Buffer.h"
#include "eDSL/Resource/Buffer.inl"

#include "eDSL/Resource/Texture.h"
#include "eDSL/Resource/Texture.inl"

#include "eDSL/Resource/SamplerState.h"

#include "eDSL/Resource/ConstantBuffer.h"
#include "eDSL/Resource/ConstantBuffer.inl"

#include "eDSL/Resource/RaytracingAccelerationStructure.h"

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

#include "eDSL/Function.h"
#include "eDSL/Function.inl"

#include "eDSL/NativeTypeMapping.h"

#define $int   ::Rtrc::eDSL::i32
#define $uint  ::Rtrc::eDSL::u32
#define $float ::Rtrc::eDSL::f32;
#define $bool  ::Rtrc::eDSL::boolean

#define $float2 ::Rtrc::eDSL::float2
#define $float3 ::Rtrc::eDSL::float3
#define $float4 ::Rtrc::eDSL::float4

#define $int2 ::Rtrc::eDSL::int2
#define $int3 ::Rtrc::eDSL::int3
#define $int4 ::Rtrc::eDSL::int4

#define $uint2 ::Rtrc::eDSL::uint2
#define $uint3 ::Rtrc::eDSL::uint3
#define $uint4 ::Rtrc::eDSL::uint4

#define $if RTRC_EDSL_IF
#define $else RTRC_EDSL_ELSE

#define $while RTRC_EDSL_WHILE

#define $function RTRC_EDSL_FUNCTION

#pragma once

#include "Generated/Reflection.hlsl"

typedef Rtrc::Renderer::VXIRDetail::HashTableKey HashTableKey;
typedef Rtrc::Renderer::VXIRDetail::HashTableValue HashTableValue;

bool IsValidHashTableKey(HashTableKey key)
{
    return key.key != 0xffffffff;
}

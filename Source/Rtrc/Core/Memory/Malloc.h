#pragma once

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

void EnableMemoryLeakReporter();

void *AllocateMemory(size_t size);
void FreeMemory(void *pointer);

void *AllocateAlignedMemory(size_t size, size_t align);
void FreeAlignedMemory(void *pointer, size_t align);

RTRC_END

#pragma once

#include <Core/Archive/JSONArchiveReader.h>
#include <Core/Archive/JSONArchiveWritter.h>
#include <Core/Archive/JSONFileArchiveReader.h>
#include <Core/Archive/JSONFileArchiveWritter.h>
#include <Core/Macro/MacroForEach.h>

RTRC_BEGIN

#define RTRC_EXPLICIT_INSTANTIATION_ARCHIVE_TRANSFER(CLASS_NAME)                                      \
    template void CLASS_NAME::Transfer<Rtrc::JSONArchiveReader>(Rtrc::JSONArchiveReader &ar);         \
    template void CLASS_NAME::Transfer<Rtrc::JSONArchiveWritter>(Rtrc::JSONArchiveWritter &ar);       \
    template void CLASS_NAME::Transfer<Rtrc::JSONFileArchiveReader>(Rtrc::JSONFileArchiveReader &ar); \
    template void CLASS_NAME::Transfer<Rtrc::JSONFileArchiveWritter>(Rtrc::JSONFileArchiveWritter &ar)

RTRC_END

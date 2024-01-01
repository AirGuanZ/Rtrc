#pragma once

#include <Rtrc/Core/Archive/JSONArchiveReader.h>
#include <Rtrc/Core/Archive/JSONArchiveWritter.h>
#include <Rtrc/Core/Archive/JSONFileArchiveReader.h>
#include <Rtrc/Core/Archive/JSONFileArchiveWritter.h>
#include <Rtrc/Core/Macro/MacroForEach.h>

RTRC_BEGIN

#define RTRC_EXPLICIT_INSTANTIATION_ARCHIVE_TRANSFER(CLASS_NAME)                                      \
    template void CLASS_NAME::Transfer<Rtrc::JSONArchiveReader>(Rtrc::JSONArchiveReader &ar);         \
    template void CLASS_NAME::Transfer<Rtrc::JSONArchiveWritter>(Rtrc::JSONArchiveWritter &ar);       \
    template void CLASS_NAME::Transfer<Rtrc::JSONFileArchiveReader>(Rtrc::JSONFileArchiveReader &ar); \
    template void CLASS_NAME::Transfer<Rtrc::JSONFileArchiveWritter>(Rtrc::JSONFileArchiveWritter &ar)

RTRC_END

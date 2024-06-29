#pragma once

#include <Rtrc/Core/Archive/Archive.h>
#include <Rtrc/Core/Bool.h>
#include <Rtrc/Core/Container/RangeSet.h>
#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/Core/Filesystem/DirectoryFilter.h>
#include <Rtrc/Core/Filesystem/File.h>
#include <Rtrc/Core/Math/Angle.h>
#include <Rtrc/Core/Math/Common.h>
#include <Rtrc/Core/Math/Intersection.h>
#include <Rtrc/Core/Math/Rect.h>
#include <Rtrc/Core/Memory/Malloc.h>
#include <Rtrc/Core/Parallel.h>
#include <Rtrc/Core/Resource/GenerateMipmap.h>
#include <Rtrc/Core/Resource/ImageND.h>
#include <Rtrc/Core/Resource/ImageSampler.h>
#include <Rtrc/Core/Resource/MeshData.h>

#include <Rtrc/Geometry/HalfedgeMesh.h>
#include <Rtrc/Geometry/RawMesh.h>
#include <Rtrc/Geometry/SignpostsMesh.h>
#include <Rtrc/Geometry/TriangleBVH.h>
#include <Rtrc/Geometry/Utility.h>

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Device/PipelineCache.h>
#include <Rtrc/Graphics/Device/ShaderBindingTableBuilder.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>

#include <Rtrc/Graphics/Shader/DSL/BindingGroup.h>

#include <Rtrc/RHI/Window/WindowInput.h>
#include <Rtrc/RHI/Window/Window.h>

#include <Rtrc/ToolKit/ToolKit.h>

#pragma once

#include <Core/Archive/Archive.h>
#include <Core/Container/RangeSet.h>
#include <Core/Filesystem/DirectoryFilter.h>
#include <Core/Filesystem/File.h>
#include <Core/Math/Intersection.h>
#include <Core/Memory/Malloc.h>
#include <Core/Resource/Image.h>
#include <Core/Resource/MeshData.h>
#include <Graphics/Device/Device.h>
#include <Graphics/ImGui/Instance.h>
#include <Graphics/Misc/ShaderBindingTableBuilder.h>
#include <Graphics/RenderGraph/Executable.h>
#include <RHI/Window/WindowInput.h>
#include <RHI/Window/Window.h>
#include <Rtrc/Application/SimpleApplication.h>
#include <Rtrc/Renderer/RenderLoop/RealTimeRenderLoop.h>
#include <Rtrc/Renderer/Utility/PrepareThreadGroupCountForIndirectDispatch.h>
#include <Rtrc/Resource/Material/LegacyMaterialInstance.h>
#include <Rtrc/Resource/ResourceManager.h>
#include <Rtrc/Scene/Camera/CameraController.h>

#include <Rtrc/DFDM/DFDM.h>

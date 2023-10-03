#ifndef RTRC_COMMON_CONSTANTS_HLSL
#define RTRC_COMMON_CONSTANTS_HLSL

// Max size of variable-sized descriptor array for global textures
// See Application::GetBindlessTextureManager
#define GLOBAL_BINDLESS_TEXTURE_GROUP_MAX_SIZE 4096

// Max size of variable-sized descriptor array for global geometry buffers
// See RenderMeshes::GetGeometryBuffersBindingGroup
#define GLOBAL_BINDLESS_GEOMETRY_BUFFER_MAX_SIZE 2048

#endif // #ifndef RTRC_COMMON_CONSTANTS_HLSL

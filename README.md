# Rtrc

Personal graphics toolkits

![](./Documents/Gallery/01.png)

## Features

* DirectX12 & Vulkan RHI backends, supporting RTX, mesh shader and work graph
* Render graph with transient resource aliasing
* Explicit binding group management in shader
* (Experimental) compute shader DSL embedded in C++
* Physically-based atmosphere rendering
* Distortion-free displacement map optimizer
* Exact floating-point computation based on arithmetic expansion
* Exact predicates for computational geometry
* Constrained Delaunay triangulation
* Array-style halfedge mesh
* Exact mesh corefinement
* Discrete differential operators on triangle mesh

## Samples

| Name | Thumbnail |
|:-:|:-:|
| 01.TexturedQuad<br>Basic texture mapping | ![Samples01](./Documents/Samples_01.png) |
| 02.ComputeShader<br>Basic usage of compute shader | ![Samples00](./Documents/Samples_02.png) |
| 03.BindlessTexture<br>Basic usage of bindless resource array | ![Samples00](./Documents/Samples_03.png) |
| 04.RayTracingTriangle<br>Basic usage of ray tracing pipeline | ![Samples00](./Documents/Samples_04.png) |
| 05.PathTracing<br>Basic unidirectional path tracer using ray query | ![Samples00](./Documents/Samples_05.png) |
| 06.DistortionFreeDisplacementMap<br>https://cg.ivd.kit.edu/english/undistort.php | ![Samples00](./Documents/Samples_06.png) |
| 07.Gizmo<br>Basic gizmo elements | ![Samples00](./Documents/Samples_07.png) |
| 08.FeatureAwareDisplacementMap <br>https://airguanz.github.io/articles/2024.02.23.VDM-Baking | ![Samples00](./Documents/Samples_08.png) |
| 09.MeshShader<br>Basic usage of mesh shader | ![Samples00](./Documents/Samples_09.png) |
| 10.FastMarchingMethod<br>Basic fast marching method on 2d grid | ![Samples00](./Documents/Samples_10.png) |
| 11.GeodesicDistance<br>Geodesic distance field on voxels | ![Samples00](./Documents/Samples_11.png) |
| 12.ShaderDSL<br>Embedded DSL in C++ for writing shader | ![](/Documents/Samples_12.png) |
| 13.HorizonMap<br>Basic horizon shadow described in paper<br>'Interactive Horizon Mapping' | ![](./Documents/Samples_13.png) |
| 14.HalfedgeMesh<br>Halfedge data structure for triangle mesh | ![](./Documents/Samples_14.png) |
| 15.HeatMethod<br>Heat method for geodesic distance computation | ![](./Documents/Samples_15.png) |
| 16.MeshCorefinement<br>Embed all intersection lines between meshes | ![](./Documents/Samples_16.png) |
| 17.WorkGraph<br>Basic example of work graph | ![](./Documents/Samples_17.png) |
| 18.ShortestGeodesicPath<br>Find globally shortest geodesic path on triangle mesh | ![](./Documents/Samples_18.png) |
| 19.Archive<br>Serialize/deserialize to/from JSON/bytes | ![](./Documents/Samples_19.png) |
| 20.MultiLayerOIT<br>Multi-layered order-independent transparency | ![](./Documents/Samples_20.png) |
| 21.HeightMapDownsampling<br>Volume-preserving height map downsampling | ![](./Documents/Samples_21.png) |
| WIP | ... |

## Third-party Dependencies

[avir](https://github.com/avaneev/avir) for image resizing

[boost-multiprecision](https://github.com/boostorg/multiprecision) for multi-precision floating-point numbers

[bvh](https://github.com/madmann91/bvh) for BVH construction

[Catch2](https://github.com/catchorg/Catch2) for testing

[cista](https://github.com/felixguendling/cista) for counting class members

[cxxopts](https://github.com/jarro2783/cxxopts) for parsing command arguments

[cyCodeBase](http://www.cemyuksel.com/cyCodeBase/) for generating possion disk samples

[D3D12MemAlloc](https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator) for memory management in D3D12 backend

[DearImGui](https://github.com/ocornut/imgui) for creating GUI in samples

[DirectXAgilitySDK](https://devblogs.microsoft.com/directx/directx12agility/) (binary) for accessing D3D12 preview features

[DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler) (binary) for compiling shaders

[Eigen](https://eigen.tuxfamily.org/index.php?title=Main_Page) for solving linear systems

[fmt](https://github.com/fmtlib/fmt?tab=License-1-ov-file) for formatting strings

[geometry-central](https://github.com/nmwsharp/geometry-central) for computing local parameterization (logarithmic map)

[GLFW](https://www.glfw.org/) for managing windows and system events

[half](https://github.com/melowntech/half) for conversions between float16 and float32

[libigl](https://libigl.github.io/) for planar mesh parameterization in samples

[magic_enum](https://github.com/Neargye/magic_enum) for formatting enum values

[mimalloc](https://github.com/microsoft/mimalloc) for memory allocation

[nlohmann-json](https://github.com/nlohmann/json) for parsing & writing JSON strings

[oneapi-tbb](https://github.com/oneapi-src/oneTBB) for spin locks and thread-safe containers

[ryu](https://github.com/ulfjack/ryu/tree/master) for numer-to-string conversion

[sigslot](https://github.com/palacaze/sigslot) for thread-safe event broadcasting

[smhasher](https://github.com/rurban/smhasher) for hash operators

[spdlog](https://github.com/gabime/spdlog) for logging

[SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect) for reflecting SPIRV codes

[stb](https://github.com/nothings/stb) for image IO

[tinyexr](https://github.com/syoyo/tinyexr) for EXR image IO

[tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) for loading wavefront obj files

[unordered_dense](https://github.com/martinus/unordered_dense) for replacing `std::unordered_*`

[vk-bootstrap](https://github.com/charles-lunarg/vk-bootstrap) for Vulkan initialization

[volk](https://github.com/zeux/volk) for loading Vulkan entry points

[VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator) for memory management in Vulkan backend

[WinPIXEventRuntime](https://devblogs.microsoft.com/pix/winpixeventruntime/) (binary) for generating PIX captures

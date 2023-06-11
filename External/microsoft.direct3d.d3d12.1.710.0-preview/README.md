# DirectX 12 Agility SDK Redistributable NuGet Package

This package contains a copy of the DirectX 12 Agility SDK redistributable runtime and its associated development headers. 

For help getting started and other information for the Agility SDK, please see:

https://aka.ms/directx12agility

The included licenses apply to the following files:

- **LICENSE.txt** : applies to all files under `build/native/bin/`
- **LICENSE-CODE.txt** : applies to all files under `build/native/include/`

## Changelog

### Version 1.706.4 (preview)

- Fixes a device creation failure on WARP (Microsoft Basic Render) in Windows Server 2022.

### Version 1.606.4

- Fixes a device creation failure on WARP (Microsoft Basic Render) in Windows Server 2022.

### Version 1.706.3 (preview)

- Enhanced Barriers Preview 2 with GBV support
- Adds `ID3D12DebugCommandList3::AssertTextureLayout` and ` ID3D12DebugCommandQueue1::AssertResourceAccess` methods. 
- Independent Front/Back Stencil Refs and Masks 
- Triangle fans are back

### Version 1.606.3

New features:

- Shader Model 6.7.
- d3dconfig: settings import/export.
- d3dconfig: option to allow application control over storage filters
- DRED: 'markers only breadcrumbs' stores breadcrumbs only for PIX markers and events.

Bug fixes:

- Various debug layer stability fixes.

### Version 1.602.0

New features:

- Relaxed buffer/texture copy alignment
- Support for copying between different dimensions of textures
- Delayed input layout and vertex buffer alignment validation
- Negative height viewports flip y-axis intepretation
- Alpha/InvAlpha blend factors

Bugfixes

- Fixes a crash using GBV with shader patch mode TRACKING_ONLY. 
- Fixes false debug validation output resulting from depth slice state being confused with stencil slice state. 
- Fixes a bug causing promoted COPY_DEST to not decay back to COMMON. 
- Report live objects when encountering device removed from a kernel memory failure.  
  
### Version 1.600.10

Fixes threading bug (intermittent crash) in runtime for apps doing multithreaded creation of raytracing state objects.

Ideally this fix would have been made in isolation on top of the previous SDK release.   Unfortunately this was not possible due to build infrastructure changes - hopefully a one-time issue.  What this means is that this SDK's runtime reflects the current state of the D3D12 codebase including code churn unrelated to the bug fix, such as support for preview features that are disabled here and exposed in a separate preview SDK branch.  Even though preview features are disabled, the codebase is still different.  So there is some risk of bugs/regressions that have not been noticed yet in internal testing.  In particular there is a reasonable chance that the churn in the debug layer codebase might yield some debug validation issues, but there could be issues lurking in the runtime as well.  If you report any issues you observe to us, we will try to address them with a follow-up SDK release.

Whether you choose to use this bugfix release, with its extra code churn of no value to you, really boils down to how important the specific fix is to you (if at all), and perhaps how much capacity you have to do test this combination of runtime and your app.

### Version 1.4.10 

Fixes a debug layer issue where some ResourceBarrier calls transitioning DEPTH_READ to DEPTH_WRITE were dropped

### Version 1.4.9

Contains support for DirectX 12 Ultimate and Shader Model 6.6